/**
 * RailOps Visualizer — 50% Interim Milestone Simulation Engine
 *
 * Member 1: Train threads, PCB states (NEW → READY → RUNNING → WAITING → TERMINATED)
 * Member 2: In-memory resource table, exclusive mutexes, condition-variable wait queues
 * Member 3: Collision avoidance engine — checkSafe(trainId, resourceId)
 * Member 4: Signal state machine (RED / YELLOW / GREEN), hold() and clear() gating
 * Member 5: Simulation controller loop (RUNNING / STOPPED), FIFO event queue
 */

const State = {
  running: true,
  time: 0,
  tickMs: 100,
  speed: 1.0,
  timer: null,

  resources: {
    1: { id: 1, name: "Track 1", type: "EXCLUSIVE", holder: null, waitQueue: [] },
    2: { id: 2, name: "Junction 2", type: "EXCLUSIVE", holder: null, waitQueue: [] },
    3: { id: 3, name: "Track 3", type: "EXCLUSIVE", holder: null, waitQueue: [] },
    4: { id: 4, name: "Platform 4", type: "EXCLUSIVE", holder: null, waitQueue: [] }
  },

  signals: {
    1: { aspect: "GREEN" },
    2: { aspect: "GREEN" },
    3: { aspect: "GREEN" },
    4: { aspect: "GREEN" }
  },

  trains: [
    {
      id: 1, name: "T101", priority: 1, state: "RUNNING",
      route: [1, 2, 4], routeIdx: 0, progress: 0.05,
      currentRes: 1, speed: 0.02, color: "#5a96bb"
    },
    {
      id: 2, name: "T102", priority: 2, state: "RUNNING",
      route: [3, 2, 4], routeIdx: 0, progress: 0.02,
      currentRes: 3, speed: 0.016, color: "#bb923c"
    }
  ],

  logs: []
};

// Waypoint geometry matching the SVG viewBox
const Geo = {
  t1: { sx: 80, sy: 140, ex: 310, ey: 140 },
  t3: { sx: 80, sy: 270, ex: 310, ey: 270 },
  j2: {
    nEntry: { x: 310, y: 140 },
    sEntry: { x: 310, y: 270 },
    merge:  { x: 430, y: 205 },
    exit:   { x: 640, y: 205 }
  },
  p4: { sx: 640, sy: 205, ex: 920, ey: 205 }
};

let D = {};

function cacheDOM() {
  D = {
    btnPlay:    document.getElementById("btnPlay"),
    btnReset:   document.getElementById("btnReset"),
    btnConflict:document.getElementById("btnTriggerConflict"),
    btnStagger: document.getElementById("btnDemoStaggered"),
    btnContend: document.getElementById("btnDemoContention"),
    speedSel:   document.getElementById("speedSelect"),
    timeDisp:   document.getElementById("simTimeDisplay"),
    statusText: document.getElementById("simStatusText"),
    statusDot:  document.getElementById("simStatusDot"),
    pcbs:       document.getElementById("pcbContainer"),
    resTable:   document.getElementById("resTable"),
    evtFeed:    document.getElementById("eventFeed"),
    junc2Rail:  document.getElementById("junc2Rail"),
    plat4Box:   document.getElementById("plat4Box"),
    sqlModal:   document.getElementById("sqlModal"),
    btnSQL:     document.getElementById("btnViewSQL"),
    btnCloseSQL:document.getElementById("btnCloseSQL")
  };
}

// ── Startup ──
function init() {
  cacheDOM();
  bindEvents();
  State.resources[1].holder = 1;
  State.resources[3].holder = 2;
  log("ctrl", "Simulation controller started.");
  log("thread", "Threads T101 (pri 1) and T102 (pri 2) spawned.");
  log("resource", "Exclusive mutexes registered for Track 1, Junction 2, Track 3, Platform 4.");
  log("signal", "All signals initialised to GREEN.");
  renderAll();
  startClock();
}

function bindEvents() {
  D.btnPlay.addEventListener("click", toggleRun);
  D.btnReset.addEventListener("click", resetSim);
  D.btnConflict.addEventListener("click", demoContention);
  D.btnStagger.addEventListener("click", demoStaggered);
  D.btnContend.addEventListener("click", demoContention);
  D.speedSel.addEventListener("change", e => { State.speed = parseFloat(e.target.value); });
  D.btnSQL.addEventListener("click", () => D.sqlModal.classList.add("open"));
  D.btnCloseSQL.addEventListener("click", () => D.sqlModal.classList.remove("open"));

  window.addEventListener("keydown", e => {
    if (e.code === "Space") { e.preventDefault(); toggleRun(); }
    if (e.key === "r" || e.key === "R") resetSim();
  });
}

// ── Clock ──
function startClock() {
  if (State.timer) clearInterval(State.timer);
  State.timer = setInterval(() => { if (State.running) tick(); }, State.tickMs);
}

function toggleRun() {
  State.running = !State.running;
  if (State.running) {
    D.btnPlay.innerHTML = pauseIcon() + "Pause";
    D.btnPlay.classList.add("btn-accent");
    D.statusText.textContent = "Running";
    D.statusDot.classList.remove("stopped");
    log("ctrl", "Controller → Running.");
  } else {
    D.btnPlay.innerHTML = playIcon() + "Start";
    D.btnPlay.classList.remove("btn-accent");
    D.statusText.textContent = "Stopped";
    D.statusDot.classList.add("stopped");
    log("ctrl", "Controller → Stopped.");
  }
}

function pauseIcon() {
  return '<svg viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="4" width="4" height="16"/><rect x="14" y="4" width="4" height="16"/></svg>';
}
function playIcon() {
  return '<svg viewBox="0 0 24 24" fill="currentColor"><polygon points="5 3 19 12 5 21 5 3"/></svg>';
}

// ── Tick ──
function tick() {
  State.time += (State.tickMs / 1000) * State.speed;
  D.timeDisp.textContent = fmt(State.time);

  State.trains.forEach(tr => {
    if (tr.state === "TERMINATED") return;

    // Waiting trains: try to acquire next resource
    if (tr.state === "WAITING") {
      const nextRes = State.resources[tr.route[tr.routeIdx + 1]];
      if (nextRes && !nextRes.holder) {
        nextRes.holder = tr.id;
        nextRes.waitQueue = nextRes.waitQueue.filter(id => id !== tr.id);
        tr.state = "RUNNING";
        tr.currentRes = nextRes.id;
        tr.routeIdx++;
        tr.progress = 0;
        const sigId = tr.id === 1 ? 1 : 3;
        State.signals[sigId].aspect = "GREEN";
        log("signal", `clear(${tr.id}): S${sigId} → GREEN. ${tr.name} resumes.`);
        log("resource", `${tr.name} acquired mutex on ${nextRes.name}.`);
      }
      return;
    }

    if (tr.state !== "RUNNING") return;

    tr.progress += tr.speed * State.speed;
    const curRes = tr.route[tr.routeIdx];

    // Approaching junction — collision check
    if ((curRes === 1 || curRes === 3) && tr.progress >= 0.72) {
      const safe = checkSafe(tr.id, 2);
      if (!safe) {
        tr.state = "WAITING";
        tr.progress = 0.72;
        const junc = State.resources[2];
        if (!junc.waitQueue.includes(tr.id)) junc.waitQueue.push(tr.id);
        const sigId = tr.id === 1 ? 1 : 3;
        State.signals[sigId].aspect = "RED";
        log("collision", `checkSafe(${tr.id}, Junction 2) → false. Collision averted.`);
        log("signal", `hold(${tr.id}): S${sigId} → RED.`);
        log("thread", `${tr.name}: RUNNING → WAITING (cv_.wait).`);
        return;
      }
      if (!State.resources[2].holder) {
        State.resources[2].holder = tr.id;
        log("resource", `${tr.name} acquired mutex on Junction 2.`);
      }
    }

    // End of current block
    if (tr.progress >= 1.0) {
      if (curRes === 1 || curRes === 3) {
        State.resources[curRes].holder = null;
        log("resource", `${tr.name} released ${State.resources[curRes].name}.`);
        tr.routeIdx = 1;
        tr.currentRes = 2;
        tr.progress = 0;
      } else if (curRes === 2) {
        const plat = State.resources[4];
        if (!plat.holder) {
          plat.holder = tr.id;
          State.resources[2].holder = null;
          log("resource", `${tr.name} released Junction 2.`);
          tr.routeIdx = 2;
          tr.currentRes = 4;
          tr.progress = 0;
          log("thread", `${tr.name} entered Platform 4.`);
        } else {
          tr.state = "WAITING";
          tr.progress = 0.95;
          if (!plat.waitQueue.includes(tr.id)) plat.waitQueue.push(tr.id);
        }
      } else if (curRes === 4) {
        State.resources[4].holder = null;
        log("resource", `${tr.name} released Platform 4.`);
        tr.state = "TERMINATED";
        tr.currentRes = null;
        log("thread", `${tr.name} terminated.`);
      }
    }
  });

  renderAll();
}

// ── Member 3: collision avoidance ──
function checkSafe(trainId, targetId) {
  const target = State.resources[targetId];
  if (target.holder && target.holder !== trainId) return false;
  const rival = State.trains.find(t =>
    t.id !== trainId && (t.currentRes === 1 || t.currentRes === 3) && t.progress >= 0.68
  );
  if (rival) {
    const me = State.trains.find(t => t.id === trainId);
    if (me.priority > rival.priority) return false;
  }
  return true;
}

// ── Rendering ──
function renderAll() {
  renderTracks();
  renderTrains();
  renderPCBs();
  renderResources();
}

function renderTracks() {
  if (D.junc2Rail) {
    D.junc2Rail.setAttribute("class",
      State.resources[2].holder ? "trk-rail active" : "trk-rail"
    );
  }
  for (let s = 1; s <= 4; s++) {
    const g = document.getElementById(`sig${s}G`);
    const y = document.getElementById(`sig${s}Y`);
    const r = document.getElementById(`sig${s}R`);
    if (!g) continue;
    const a = State.signals[s].aspect;
    g.setAttribute("class", a === "GREEN" ? "sig-lamp-green" : "sig-lamp-off");
    y.setAttribute("class", a === "YELLOW" ? "sig-lamp-amber" : "sig-lamp-off");
    r.setAttribute("class", a === "RED" ? "sig-lamp-red" : "sig-lamp-off");
  }
  if (D.plat4Box) {
    D.plat4Box.setAttribute("class",
      State.resources[4].holder ? "plat-housing occupied" : "plat-housing"
    );
  }
}

function renderTrains() {
  State.trains.forEach(tr => {
    const el = document.getElementById(`train${tr.id}Carriage`);
    if (!el) return;
    if (tr.state === "TERMINATED") { el.style.display = "none"; return; }
    el.style.display = "block";

    let x = 80, y = 140, angle = 0;
    const p = Math.min(1, Math.max(0, tr.progress));

    if (tr.currentRes === 1) {
      x = Geo.t1.sx + p * (Geo.t1.ex - Geo.t1.sx);
      y = Geo.t1.sy;
    } else if (tr.currentRes === 3) {
      x = Geo.t3.sx + p * (Geo.t3.ex - Geo.t3.sx);
      y = Geo.t3.sy;
    } else if (tr.currentRes === 2) {
      if (p < 0.5) {
        const sub = p * 2;
        const sy = tr.id === 1 ? Geo.j2.nEntry.y : Geo.j2.sEntry.y;
        x = 310 + sub * (Geo.j2.merge.x - 310);
        y = sy + sub * (Geo.j2.merge.y - sy);
        angle = tr.id === 1 ? 12 : -12;
      } else {
        const sub = (p - 0.5) * 2;
        x = Geo.j2.merge.x + sub * (Geo.j2.exit.x - Geo.j2.merge.x);
        y = 205;
        angle = 0;
      }
    } else if (tr.currentRes === 4) {
      x = Geo.p4.sx + p * (Geo.p4.ex - Geo.p4.sx);
      y = Geo.p4.sy;
    }

    el.setAttribute("transform", `translate(${x}, ${y}) rotate(${angle})`);
  });
}

function renderPCBs() {
  if (!D.pcbs) return;
  D.pcbs.innerHTML = State.trains.map(tr => {
    const cls =
      tr.state === "RUNNING"    ? "state-running" :
      tr.state === "WAITING"    ? "state-waiting" :
      tr.state === "TERMINATED" ? "state-terminated" :
      "state-ready";
    const loc = tr.currentRes ? State.resources[tr.currentRes]?.name : "—";
    return `
      <div class="pcb-row">
        <div>
          <div class="pcb-name">
            <span style="color:${tr.color}">●</span> ${tr.name}
            <span class="pcb-meta">pri ${tr.priority}</span>
          </div>
          <div class="pcb-meta">${loc} · step ${tr.routeIdx + 1}/3</div>
        </div>
        <span class="state-tag ${cls}">${tr.state}</span>
      </div>`;
  }).join("");
}

function renderResources() {
  if (!D.resTable) return;
  D.resTable.innerHTML = Object.values(State.resources).map(r => {
    const held = r.holder
      ? `<span class="res-status res-locked">locked (T${r.holder})</span>`
      : `<span class="res-status res-free">free</span>`;
    const queue = r.waitQueue.length
      ? `<span class="res-queue">wait: T${r.waitQueue.join(", T")}</span>`
      : "";
    return `<div class="res-row">
      <span class="res-name">${r.name}${queue}</span>${held}
    </div>`;
  }).join("");
}

// ── Logging ──
function log(tag, msg) {
  const t = fmt(State.time);
  let cls = "evt-tag-thread";
  if (tag === "collision") cls = "evt-tag-collision";
  else if (tag === "signal")   cls = "evt-tag-signal";
  else if (tag === "resource") cls = "evt-tag-resource";
  else if (tag === "ctrl")     cls = "evt-tag-ctrl";

  State.logs.unshift({ t, tag, msg, cls });
  if (State.logs.length > 30) State.logs.pop();

  if (D.evtFeed) {
    D.evtFeed.innerHTML = State.logs.map(l =>
      `<div class="evt-line">
        <span class="evt-time">${l.t}</span>
        <span class="evt-tag ${l.cls}">${l.tag}</span>
        <span class="evt-msg">${l.msg}</span>
      </div>`
    ).join("");
  }
}

// ── Demos ──
function demoStaggered() {
  resetSim();
  State.trains[0].progress = 0.35;
  State.trains[1].progress = 0.05;
  log("ctrl", "Demo: sequential traffic (T101 leads).");
}

function demoContention() {
  resetSim();
  State.trains[0].progress = 0.65;
  State.trains[1].progress = 0.63;
  log("ctrl", "Demo: simultaneous convergence at Junction 2.");
  log("collision", "checkSafe() will evaluate competing requests…");
}

function resetSim() {
  State.time = 0;
  [1,2,3,4].forEach(id => {
    State.resources[id].holder = null;
    State.resources[id].waitQueue = [];
    State.signals[id].aspect = "GREEN";
  });
  State.resources[1].holder = 1;
  State.resources[3].holder = 2;

  State.trains[0].state = "RUNNING";
  State.trains[0].routeIdx = 0;
  State.trains[0].progress = 0.05;
  State.trains[0].currentRes = 1;

  State.trains[1].state = "RUNNING";
  State.trains[1].routeIdx = 0;
  State.trains[1].progress = 0.02;
  State.trains[1].currentRes = 3;

  log("ctrl", "Simulation reset.");
  renderAll();
}

function fmt(sec) {
  const m = Math.floor(sec / 60);
  const s = Math.floor(sec % 60);
  const d = Math.floor((sec % 1) * 10);
  return `${String(m).padStart(2,"0")}:${String(s).padStart(2,"0")}.${d}`;
}

window.addEventListener("DOMContentLoaded", init);

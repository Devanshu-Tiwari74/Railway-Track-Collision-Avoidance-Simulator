cmake_minimum_required(VERSION 3.16)
project(railway_sim CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(railway-sim ${SOURCES})
target_include_directories(railway-sim PRIVATE include)

find_package(Threads REQUIRED)
target_link_libraries(railway-sim PRIVATE Threads::Threads)

# libpqxx is optional at build time: without it the persistence worker logs to
# stdout. The final/graded build must have it so records land in PostgreSQL.
# Setup (Windows): vcpkg install libpqxx or pacman -S mingw-w64-ucrt-x86_64-libpqxx
if (WIN32 AND EXISTS "C:/msys64/ucrt64")
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/ucrt64")
endif()
find_package(libpqxx CONFIG QUIET)
if (libpqxx_FOUND)
    message(STATUS "libpqxx found -> PostgreSQL persistence enabled")
    target_link_libraries(railway-sim PRIVATE libpqxx::pqxx)
    target_compile_definitions(railway-sim PRIVATE HAVE_LIBPQXX=1)
else()
    message(WARNING "libpqxx NOT found -> persistence worker will log to stdout only")
endif()

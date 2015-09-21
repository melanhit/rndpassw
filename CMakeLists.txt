cmake_minimum_required(VERSION 2.8)
project(RNDPASSW C)

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

set(CMAKE_BUILD_TYPE Release)

include_directories(src)

add_subdirectory(src)
add_subdirectory(doc/man)

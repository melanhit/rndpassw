cmake_minimum_required(VERSION 2.8)
project(RNDPASSW C)

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG")

include_directories(src)

add_subdirectory(src)
add_subdirectory(doc/man)

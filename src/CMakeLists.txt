set(src
    rndpassw.c)

list(SORT src)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -DPIC -std=c90 -Wall -Werror -Wmissing-prototypes")

add_executable(rndpassw ${src})
install(TARGETS rndpassw
        RUNTIME DESTINATION bin)

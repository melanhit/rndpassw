set(src
    rndpassw.c)

list(SORT src)

add_executable(rndpassw ${src})
target_compile_options(rndpassw PUBLIC -fPIC -ansi -Wall)
install(TARGETS rndpassw
        RUNTIME DESTINATION bin)

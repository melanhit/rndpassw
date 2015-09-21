set(src
    rndpassw.c)

list(SORT src)

set(CFLAGS "-fPIC -DPIC -ansi -Wall -Werror -Wmissing-prototypes")
separate_arguments(CFLAGS UNIX_COMMAND ${CFLAGS})

add_executable(rndpassw ${src})

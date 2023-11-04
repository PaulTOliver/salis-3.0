#!/bin/sh

if [ "${1}" = "--dev" ] ; then
    echo "Building debug binary"
    gcc -ggdb -Wall -lncursesw -DNCURSES_WIDECHAR -Isrc \
        src/salis.c -o salis
else
    echo "Building optimized binary"
    gcc -O3 -Wall -lncursesw -DNCURSES_WIDECHAR -DNDEBUG -Isrc \
        src/salis.c -o salis
fi

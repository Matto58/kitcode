#!/bin/bash
echo "------ KITCODE: building lua"
(
cd lua
make all
)
echo "------ KITCODE: building kitcode"
g++ $(pkg-config --cflags --libs sdl3 sdl3-ttf gtk+-3.0) -o kitcode $(find src/*.cpp) \
    -Ilua/src -Llua/src -llua
exit $?
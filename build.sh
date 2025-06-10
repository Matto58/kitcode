#!/bin/bash
echo "------ KITCODE: building lua"
(
cd lua
make all
)
echo "------ KITCODE: building SDL_ttf"
(
cd SDL_ttf
cmake -S . -B build
cd build
make
)
echo "------ KITCODE: building kitcode"
g++ $(pkg-config --cflags --libs sdl3) -o kitcode $(find src/*.cpp) \
    -Ilua/src -Llua/src -llua \
    -ISDL_ttf/include -LSDL_ttf/build -lSDL3_ttf
exit $?
#!/bin/bash
g++ $(pkg-config --cflags --libs sdl3 sdl3-ttf) -o kitcode $(find src/*.cpp)
exit $?
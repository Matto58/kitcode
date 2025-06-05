#!/bin/bash
cd nfd/build/gmake_linux
make nfd
cd ../../..
g++ $(pkg-config --cflags --libs sdl3 sdl3-ttf gtk+-3.0) -o kitcode $(find src/*.cpp) -Infd/src/include -Lnfd/build/lib/Release/x64 -lnfd
exit $?
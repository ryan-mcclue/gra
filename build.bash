#! /usr/bin/env bash
# SPDX-License-Identifier: zlib-acknowledgement

[ ! -d build ] && mkdir build

# libraries in /lib/x86_64-linux-gnu, /usr/lib
# headers in /usr/include
g++ -DDEV_BUILD -g code/gra.cpp -o build/gra -lSDL2 -lSDL2_ttf

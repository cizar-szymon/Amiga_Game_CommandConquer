#!/bin/sh
# F0 smoke build (bebbo gcc 6.5.0b, libnix). From PowerShell:
#   docker run --rm -v "C:/Workprojects/AmigaGAMES/CommandConquer:/w" amixcom-build sh /w/tools/smoke/build.sh
set -e
mkdir -p /w/build/amiga
m68k-amigaos-g++ -std=gnu++11 -mcpu=68060 -mhard-float -O2 -noixemul -fno-exceptions -fno-rtti \
    -Wall -o /w/build/amiga/smoke /w/tools/smoke/smoke.cpp -lamiga
ls -la /w/build/amiga/smoke

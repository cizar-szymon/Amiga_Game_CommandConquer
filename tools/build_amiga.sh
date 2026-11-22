#!/bin/sh
# Build Vanilla Conquer (Tiberian Dawn) for the Amiga in the amixcom-build image.
# Sources are copied into a Docker volume (kept between runs, incremental make);
# the result is copied to build/amiga.
# From PowerShell:
#   docker run --rm -v cnc-build:/z -v "C:/Workprojects/AmigaGAMES/CommandConquer:/w" amixcom-build sh /w/tools/build_amiga.sh [make targets]
set -e
SRC=/w/vanilla-conquer
DST=/z/src
OUT=/z/out
mkdir -p "$DST" "$OUT"
tar --exclude=./.git --exclude=./build -cf - -C "$SRC" . | (cd "$DST" && tar -xf - --keep-newer-files 2>/dev/null || true)
cd "$OUT"
cmake "$DST" -DCMAKE_TOOLCHAIN_FILE=/w/tools/cmake/amiga-m68k.cmake -DCMAKE_BUILD_TYPE=${CNC_BUILD:-Release} \
  -DBUILD_VANILLATD=ON -DBUILD_VANILLARA=OFF -DSDL2=OFF -DSDL1=OFF -DOPENAL=OFF -DDSOUND=OFF -DDDRAW=OFF \
  -DNETWORKING=OFF -DBUILD_TESTS=OFF -DBUILD_TOOLS=OFF
make -j"$(nproc)" "$@" 2>&1 | tail -60

#!/bin/sh
# Build both games (Tiberian Dawn and Red Alert) in the amixcom-build image; the results go to build/amiga/vanillatd and
# build/amiga/vanillara. From Git Bash:
#   MSYS_NO_PATHCONV=1 docker run --rm -v cnc-build:/z -v "C:/Workprojects/AmigaGAMES/CommandConquer:/w" amixcom-build sh /w/tools/build_all.sh
set -e
sh /w/tools/build_amiga.sh VanillaTD > /tmp/td.log 2>&1 || { cat /tmp/td.log; exit 1; }
grep -E "error|Built target VanillaTD" /tmp/td.log || true
mkdir -p /z/outra
cd /z/outra
[ -f Makefile ] || cmake /z/src -DCMAKE_TOOLCHAIN_FILE=/w/tools/cmake/amiga-m68k.cmake -DCMAKE_BUILD_TYPE=${CNC_BUILD:-Release} \
  -DBUILD_VANILLATD=OFF -DBUILD_VANILLARA=ON -DSDL2=OFF -DSDL1=OFF -DOPENAL=OFF -DDSOUND=OFF -DDDRAW=OFF \
  -DNETWORKING=OFF -DBUILD_TESTS=OFF -DBUILD_TOOLS=OFF > /dev/null
make -j"$(nproc)" VanillaRA 2>&1 | grep -E "error|Built target VanillaRA" || true
mkdir -p /w/build/amiga
cp /z/out/vanillatd /w/build/amiga/vanillatd
cp /z/outra/vanillara /w/build/amiga/vanillara
ls -l /w/build/amiga/vanillatd /w/build/amiga/vanillara

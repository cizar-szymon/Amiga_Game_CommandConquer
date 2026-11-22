#!/bin/sh
# -O3 minus subsets of the O3 delta flags; and -O2 plus pairs
A="-fno-gcse-after-reload -fno-inline-functions -fno-ipa-cp-clone -fno-peel-loops -fno-predictive-commoning"
B="-fno-split-paths -fno-tree-loop-distribute-patterns -fno-tree-loop-vectorize -fno-tree-partial-pre -fno-tree-slp-vectorize -fno-unswitch-loops"
i=0
for fl in "$A $B" "$A" "$B" "-fno-tree-loop-vectorize -fno-tree-slp-vectorize -fno-tree-loop-distribute-patterns -fno-inline-functions" "-fno-inline-functions -fno-peel-loops -fno-unswitch-loops -fno-split-paths"; do
  i=$((i+1))
  m68k-amigaos-g++ -std=gnu++11 -mcpu=68060 -mhard-float -O3 $fl -noixemul -fno-exceptions -I/w/vanilla-conquer/common -o /w/build/amiga/lcwm$i /w/tools/smoke/lcwtest.cpp /w/vanilla-conquer/common/lcw.cpp 2>&1 | grep error
done

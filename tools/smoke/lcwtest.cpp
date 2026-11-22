// lcwtest.cpp - decode frame 0 (keyframe) of a shape with the game's LCW_Uncompress and write it to a file
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lcw.h"
int main(int argc, char** argv)
{
    FILE* f = fopen(argv[1], "rb");
    if (!f) return 5;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char* d = (unsigned char*)malloc(n);
    fread(d, 1, n, f); fclose(f);
    unsigned w = d[6] | (d[7] << 8), h = d[8] | (d[9] << 8);
    unsigned off = d[14] | (d[15] << 8) | (d[16] << 16);
    unsigned char* out = (unsigned char*)malloc(w * h);
    memset(out, 0xAA, w * h);
    int r = LCW_Uncompress(d + off, out, w * h);
    printf("w=%u h=%u off=%u ret=%d\n", w, h, off, r);
    FILE* o = fopen(argv[2], "wb"); fwrite(out, 1, w * h, o); fclose(o);
    return 0;
}

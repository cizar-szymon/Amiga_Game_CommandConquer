#!/usr/bin/env python3
"""Convert RAM:cnc-frame.bin (dump from F10 in the Amiga port) to PNG.
Format: 'CNCF', w(be16), h(be16), 768 palette bytes (6-bit VGA), w*h indexed pixels."""
import sys
from PIL import Image

src, dst = sys.argv[1], sys.argv[2]
d = open(src, "rb").read()
assert d[:4] == b"CNCF", "bad magic"
w = (d[4] << 8) | d[5]
h = (d[6] << 8) | d[7]
pal = d[8:8 + 768]
pix = d[8 + 768:8 + 768 + w * h]
rgb = []
for i in range(256):
    for k in range(3):
        v = pal[i * 3 + k] & 63
        rgb.append((v << 2) | (v >> 4))
im = Image.frombytes("P", (w, h), pix)
im.putpalette(rgb)
im.convert("RGB").save(dst)
print(dst, w, h)

#!/usr/bin/env python3
"""Decode frame N of a Westwood keyframe shape (LCW + XOR delta) to a PNG: shpframe.py <shp> <frame> <out.png> <pal.bin|->"""
import struct, sys

def lcw(src, n):
    out = bytearray(); i = 0
    while len(out) < n:
        c = src[i]; i += 1
        if not c & 0x80:
            cnt = (c >> 4) + 3; rel = src[i] + ((c & 15) << 8); i += 1
            p = len(out) - rel
            for k in range(cnt): out.append(out[p + k])
        elif not c & 0x40:
            if c == 0x80: break
            cnt = c & 0x3f; out += src[i:i + cnt]; i += cnt
        elif c == 0xfe:
            cnt = src[i] | src[i + 1] << 8; out += bytes([src[i + 2]]) * cnt; i += 3
        elif c == 0xff:
            cnt = src[i] | src[i + 1] << 8; p = src[i + 2] | src[i + 3] << 8; i += 4
            for k in range(cnt): out.append(out[p + k])
        else:
            cnt = (c & 0x3f) + 3; p = src[i] | src[i + 1] << 8; i += 2
            for k in range(cnt): out.append(out[p + k])
    return bytearray(out[:n])

def xor_delta(dst, src):
    p = 0; i = 0
    while True:
        c = src[i]; i += 1; cnt = c; val = None; run = False
        if not c & 0x80:
            if c == 0: cnt = src[i]; val = src[i + 1]; i += 2; run = True
        else:
            cnt = c & 0x7f
            if cnt: p += cnt; continue
            cnt = src[i] | src[i + 1] << 8; i += 2
            if cnt == 0: return
            if not cnt & 0x8000: p += cnt; continue
            if cnt & 0x4000: cnt &= 0x3fff; val = src[i]; i += 1; run = True
            else: cnt &= 0x3fff
        for _ in range(cnt):
            if run: dst[p] ^= val
            else: dst[p] ^= src[i]; i += 1
            p += 1

def frame(d, n):
    frames, x, y, w, h, big, flags = struct.unpack_from("<7H", d, 0)
    size = w * h; pal = 768 if flags & 1 else 0
    def ent(k):
        o, r = struct.unpack_from("<II", d, 14 + 8 * k); return o >> 24, o & 0xffffff, r
    # find the keyframe chain: frame n is a delta against key frame ref
    fl, off, ref = ent(n)
    if fl & 0x80:  # keyframe
        return w, h, lcw(d[off + pal:], size)
    r = n if fl & 0x40 else ref & 0xffff
    keyoff = struct.unpack_from("<I", d, 14 + 8 * r + 4)[0] & 0xffffff
    buf = lcw(d[keyoff + pal:], size)
    for j in range(r, n + 1):
        xor_delta(buf, d[(struct.unpack_from("<I", d, 14 + 8 * j)[0] & 0xffffff):])
    return w, h, buf

if __name__ == "__main__":
    from PIL import Image
    d = open(sys.argv[1], "rb").read(); n = int(sys.argv[2])
    w, h, buf = frame(d, n)
    flags = struct.unpack_from("<H", d, 12)[0]
    print("frame", n, w, h, "flags", flags, "zero pixels", buf.count(0))
    im = Image.frombytes("P", (w, h), bytes(buf))
    im.putpalette(open(sys.argv[4], "rb").read() if len(sys.argv) > 4 and sys.argv[4] != "-" else bytes(range(256)) * 3)
    im.resize((w * 3, h * 3), Image.NEAREST).convert("RGB").save(sys.argv[3])

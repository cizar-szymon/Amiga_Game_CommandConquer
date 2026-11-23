#!/usr/bin/env python3
"""Extract an ISO9660 image (primary tree): isoextract.py <iso> <outdir> [-l]"""
import os, struct, sys

iso, out = sys.argv[1], sys.argv[2]
listing = "-l" in sys.argv
f = open(iso, "rb")

def sector(n, count=1):
    f.seek(n * 2048)
    return f.read(2048 * count)

pvd = sector(16)
assert pvd[1:6] == b"CD001"
root = pvd[156:190]
rlba, rlen = struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0]

def walk(lba, size, path):
    data = sector(lba, (size + 2047) // 2048)
    pos = 0
    while pos < size:
        n = data[pos]
        if n == 0:
            pos = (pos // 2048 + 1) * 2048
            continue
        rec = data[pos:pos + n]
        pos += n
        elba = struct.unpack_from("<I", rec, 2)[0]
        esize = struct.unpack_from("<I", rec, 10)[0]
        flags = rec[25]
        name = rec[33:33 + rec[32]]
        if name in (b"\x00", b"\x01"):
            continue
        name = name.decode("latin-1").split(";")[0]
        full = os.path.join(path, name)
        if flags & 2:
            if listing:
                print(full + "/")
            else:
                os.makedirs(os.path.join(out, full), exist_ok=True)
            walk(elba, esize, full)
        else:
            if listing:
                print(full, esize)
            else:
                os.makedirs(os.path.join(out, path), exist_ok=True)
                with open(os.path.join(out, full), "wb") as o:
                    f.seek(elba * 2048)
                    left = esize
                    while left:
                        chunk = f.read(min(left, 1 << 20))
                        o.write(chunk)
                        left -= len(chunk)

walk(rlba, rlen, "")

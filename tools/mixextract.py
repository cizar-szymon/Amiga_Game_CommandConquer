#!/usr/bin/env python3
"""Extract one file from a Westwood TD MIX: mixextract.py <mix> <NAME> <out>"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mixfind_lib import tdhash

mix, name, out = sys.argv[1:4]
with open(mix, "rb") as f:
    count, size = struct.unpack("<HI", f.read(6))
    idx = f.read(12 * count)
    base = 6 + 12 * count
    h = tdhash(name)
    for i in range(count):
        fid, off, sz = struct.unpack_from("<III", idx, i * 12)
        if fid == h:
            f.seek(base + off)
            open(out, "wb").write(f.read(sz))
            print(name, sz, "bytes ->", out)
            break
    else:
        sys.exit("not found")

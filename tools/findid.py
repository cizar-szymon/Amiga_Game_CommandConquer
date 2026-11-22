#!/usr/bin/env python3
"""Search files under a directory for the Westwood id (4 bytes LE) of given names."""
import os, struct, sys
sys.path.insert(0, os.path.dirname(__file__))
from mixfind_lib import tdhash
root = sys.argv[1]
names = sys.argv[2:]
ids = {n: struct.pack("<I", tdhash(n)) for n in names}
for d, _, files in os.walk(root):
    for fn in files:
        p = os.path.join(d, fn)
        try:
            if os.path.getsize(p) > 700_000_000:
                continue
            data = open(p, "rb").read()
        except Exception:
            continue
        for n, b in ids.items():
            k = data.find(b)
            if k >= 0:
                print(os.path.relpath(p, root), n, "at", k)

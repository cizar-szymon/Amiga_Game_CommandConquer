#!/usr/bin/env python3
"""Find which MIX archive holds a file name (Westwood TD hash). usage: mixfind.py <dir> NAME..."""
import os, struct, sys

def tdhash(name):
    """Westwood TD id: CRC engine, 4-byte little-endian groups, rotl1 + group."""
    b = name.upper().encode("latin-1")
    crc = 0
    for i in range(0, len(b), 4):
        chunk = b[i:i + 4]
        grp = int.from_bytes(chunk + bytes(4 - len(chunk)), "little")
        crc = (((crc << 1) | (crc >> 31)) & 0xFFFFFFFF) + grp
        crc &= 0xFFFFFFFF
    return crc

def entries(path):
    with open(path, "rb") as f:
        head = f.read(6)
        count, size = struct.unpack("<HI", head)
        idx = f.read(12 * count)
    return {struct.unpack_from("<I", idx, i * 12)[0]: struct.unpack_from("<II", idx, i * 12 + 4) for i in range(count)}

root = sys.argv[1]
names = sys.argv[2:]
mixes = []
for d, _, files in os.walk(root):
    for fn in files:
        if fn.upper().endswith(".MIX"):
            mixes.append(os.path.join(d, fn))
for m in mixes:
    try:
        e = entries(m)
    except Exception as ex:
        print(m, "ERR", ex)
        continue
    hits = [n for n in names if tdhash(n) in e]
    print(os.path.relpath(m, root), len(e), "entries", "->", hits)

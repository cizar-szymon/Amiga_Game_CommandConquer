import sys
sys.path.insert(0, "tools")
from shpframe import frame
S = sys.argv[1]
d = open(S + "/HRADAR.GDI.shp", "rb").read()
f = open(S + "/sbc-uae.bin", "rb").read()
W = 640; pix = f[8 + 768:]
best = None
for n in (0, 21):
    w, h, buf = frame(d, n)
    # find the placement by brute force around the sidebar (x 480..520, y 0..20)
    for ox in range(478, 500):
        for oy in range(0, 20):
            bad = 0; tot = 0
            for y in range(0, h, 3):
                for x in range(0, w, 3):
                    tot += 1
                    if pix[(oy + y) * W + ox + x] != buf[y * w + x]: bad += 1
            if best is None or bad / tot < best[0]: best = (bad / tot, n, ox, oy)
print("best mismatch ratio %.3f frame %d at %d,%d" % best)
r, n, ox, oy = best
w, h, buf = frame(d, n)
diff = [(x, y) for y in range(h) for x in range(w) if pix[(oy + y) * W + ox + x] != buf[y * w + x]]
print(len(diff), "of", w * h, "differ; first:", diff[:12])
zero = sum(1 for (x, y) in diff if buf[y * w + x] == 0)
print("source pixel is index 0 in", zero, "of the differing pixels; screen index there:", sorted(set(pix[(oy + y) * W + ox + x] for x, y in diff))[:10])
dark = [(x, y, buf[y * w + x], pix[(oy + y) * W + ox + x]) for x, y in diff if pix[(oy + y) * W + ox + x] in (0, 1, 2, 3)]
print("diff pixels with dark screen index:", len(dark), dark[:15])
rows = {}
for x, y in diff: rows.setdefault(y, []).append(x)
print("rows with diffs:", len(rows), "first rows:", sorted(rows)[:10])
from collections import Counter
c = Counter((buf[y * w + x], pix[(oy + y) * W + ox + x]) for x, y in diff)
print("top (source,screen) pairs:", c.most_common(12))
pal = f[8:8 + 768]
for i in (12, 14, 49, 51, 52):
    print(i, [pal[i * 3 + k] for k in range(3)])
print("--- black (12) diff pixels as runs (row: x..x) ---")
runs = []
for y in sorted(rows):
    xs = sorted(x for x in rows[y] if pix[(oy + y) * W + ox + x] == 12)
    s = None
    for x in xs:
        if s is None: s = p = x
        elif x == p + 1: p = x
        else: runs.append((y, s, p)); s = p = x
    if s is not None: runs.append((y, s, p))
print(len(runs), "runs; first 25:", runs[:25])
print("screen abs x of run starts mod 16:", Counter(((ox + s) % 16) for y, s, p in runs).most_common(8))
print("abs x of run ends mod 16:", Counter(((ox + p) % 16) for y, s, p in runs).most_common(8))
print("lengths:", Counter(p - s + 1 for y, s, p in runs).most_common(10))

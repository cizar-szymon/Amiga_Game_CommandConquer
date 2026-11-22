#!/usr/bin/env python3
"""Build a battle test scenario from SCG01EA.INI: many GDI and Nod units in the open ground,
all on Hunt, view on the front line, no win/lose triggers. Usage: make_battle_scenario.py in.ini out.ini"""
import re, sys

src, dst = sys.argv[1], sys.argv[2]
t = open(src, encoding="latin-1").read().replace("\r\n", "\n")

def cell(x, y):
    return y * 64 + x

units, inf = [], []
n = 0
# GDI line (south, facing north = 0)
for i, typ in enumerate(["MTNK"] * 10 + ["HTNK"] * 4 + ["JEEP"] * 4 + ["APC"] * 2):
    x, y = 49 + (i % 10) * 1, 51 + (i // 10)
    units.append(f"GoodGuy,{typ},256,{cell(x + 1, y)},0,Hunt,None")
for i in range(24):
    x, y = 49 + (i % 12), 50 + (i // 12) * 0
    inf.append(f"GoodGuy,{'E1' if i % 3 else 'E2'},256,{cell(x + 1, 49)},{1 + i % 4},Hunt,0,None")
# Nod line (north, facing south = 128)
for i, typ in enumerate(["LTNK"] * 12 + ["BGGY"] * 6 + ["BIKE"] * 4):
    x, y = 49 + (i % 11), 45 + (i // 11)
    units.append(f"BadGuy,{typ},256,{cell(x + 1, y)},128,Hunt,None")
for i in range(28):
    x = 49 + (i % 14)
    inf.append(f"BadGuy,{'E1' if i % 4 else 'E3'},256,{cell(x + 1, 47)},{1 + i % 4},Hunt,128,None")

def section(name, lines):
    body = "\n".join(f"{k:03d}={v}" for k, v in enumerate(lines))
    return f"[{name}]\n{body}\n"

t = re.sub(r"\[UNITS\].*?\n\n", section("UNITS", units) + "\n", t, flags=re.S)
t = re.sub(r"\[INFANTRY\].*?\n\n", section("INFANTRY", inf) + "\n", t, flags=re.S)
t = re.sub(r"\[STRUCTURES\].*?\n\n", "[STRUCTURES]\n\n", t, flags=re.S)
t = re.sub(r"\[Triggers\].*?\n\n", "[Triggers]\n\n", t, flags=re.S)
t = re.sub(r"\[REINFORCEMENTS\].*?\n\n", "[REINFORCEMENTS]\n\n", t, flags=re.S)
t = re.sub(r"\[TeamTypes\].*?\n\n", "[TeamTypes]\n\n", t, flags=re.S)
t = re.sub(r"TacticalPos=\d+", "TacticalPos=%d" % cell(44, 41), t)
t = t.replace("Action=LANDING", "Action=")
t = t.replace("Win=CONSYARD", "Win=").replace("Lose=GAMEOVER", "Lose=")
open(dst, "w", encoding="latin-1", newline="\r\n").write(t)
print(len(units), "vehicles", len(inf), "infantry ->", dst)

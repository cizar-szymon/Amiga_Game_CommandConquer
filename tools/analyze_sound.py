#!/usr/bin/env python3
"""Analyse RAM:cnc-sound.raw (16 bit big endian stereo, 22050 Hz): level per second, clipping, silence, WAV export.
usage: analyze_sound.py in.raw [out.wav]"""
import array, sys, wave

rate = 22050
d = array.array("h")
d.frombytes(open(sys.argv[1], "rb").read())
if sys.byteorder == "little":
    d.byteswap()
n = len(d) // 2
print("frames", n, "= %.1f s" % (n / rate))
sec = rate * 2
peak = max(max(d), -min(d)) if len(d) else 0
clip = sum(1 for v in d if v >= 32767 or v <= -32768)
print("peak", peak, "clipped samples", clip)
for i in range(0, len(d), sec):
    blk = d[i:i + sec]
    if not blk:
        break
    rms = (sum(v * v for v in blk) / len(blk)) ** 0.5
    pk = max(max(blk), -min(blk))
    zc = sum(1 for k in range(1, len(blk)) if (blk[k - 1] < 0) != (blk[k] < 0))
    print("t=%3d s  rms %6.0f  peak %5d  zero crossings/s %6d" % (i // sec, rms, pk, zc // 2))
if len(sys.argv) > 2:
    w = wave.open(sys.argv[2], "wb")
    w.setnchannels(2)
    w.setsampwidth(2)
    w.setframerate(rate)
    le = array.array("h", d)
    if sys.byteorder == "big":
        le.byteswap()
    w.writeframes(le.tobytes())
    w.close()
    print("wrote", sys.argv[2])

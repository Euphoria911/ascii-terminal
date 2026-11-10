"""Writes demo.gray: an original looping metaball animation in the format badapple.exe plays.

120x90 raw 8-bit gray, 30 fps. No video needed:
    python demo.py
    badapple.exe demo.gray
"""
import math
import sys

kSrcW = 120  # must match kSrcW in badapple.c
kSrcH = 90   # must match kSrcH in badapple.c
kFps = 30
kSeconds = 10
kOut = sys.argv[1] if len(sys.argv) > 1 else "demo.gray"

# (center x amp, center y amp, x freq, y freq, phase, radius px) - integer freqs make a seamless loop
kBlobs = (
    (40, 26, 1, 2, 0.0, 10.0),
    (38, 28, 2, 1, 1.7, 9.0),
    (30, 24, 3, 2, 3.1, 8.0),
    (42, 20, 1, 3, 4.6, 9.0),
)


def smoothstep(t):
    t = 0.0 if t < 0.0 else 1.0 if t > 1.0 else t
    return t * t * (3.0 - 2.0 * t)


def main():
    n = kFps * kSeconds
    with open(kOut, "wb") as f:
        for k in range(n):
            a = 2.0 * math.pi * k / n
            centers = [
                (kSrcW / 2 + ax * math.cos(fx * a + ph),
                 kSrcH / 2 + ay * math.sin(fy * a + ph),
                 r * r)
                for ax, ay, fx, fy, ph, r in kBlobs
            ]
            frame = bytearray(kSrcW * kSrcH)
            for y in range(kSrcH):
                for x in range(kSrcW):
                    field = 0.0
                    for cx, cy, r2 in centers:
                        dx = x - cx
                        dy = y - cy
                        field += r2 / (dx * dx + dy * dy + 1.0)
                    frame[y * kSrcW + x] = int(255 * smoothstep((field - 0.35) / 1.25))
            f.write(frame)
    print(f"wrote {kOut}: {n} frames, {kSrcW}x{kSrcH}, {kFps} fps")


if __name__ == "__main__":
    main()

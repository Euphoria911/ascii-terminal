import math
import os
import sys
import time

kW = 80
kH = 24
kTickMs = 30
kSpinA = 0.04
kSpinB = 0.02
kLum = ".,-~:;=!*#$@"


def main():
    os.system("")  # enables ANSI/VT processing on legacy Windows consoles
    A = B = 0.0
    sys.stdout.write("\x1b[2J\x1b[?25l")
    try:
        while True:
            out = [" "] * (kW * kH)
            zbuf = [0.0] * (kW * kH)
            cA, sA = math.cos(A), math.sin(A)
            cB, sB = math.cos(B), math.sin(B)

            theta = 0.0
            while theta < 6.28:
                ct, st = math.cos(theta), math.sin(theta)
                phi = 0.0
                while phi < 6.28:
                    sp, cp = math.sin(phi), math.cos(phi)
                    h = ct + 2
                    D = 1 / (sp * h * sA + st * cA + 5)
                    t = sp * h * cA - st * sA
                    x = int(kW / 2 + 30 * D * (cp * h * cB - t * sB))
                    y = int(kH / 2 + 15 * D * (cp * h * sB + t * cB))
                    o = x + kW * y
                    N = int(8 * ((st * sA - sp * ct * cA) * cB - sp * ct * sA - st * cA - cp * ct * sB))
                    if 0 < y < kH - 1 and 0 < x < kW and D > zbuf[o]:
                        zbuf[o] = D
                        out[o] = kLum[max(N, 0)]
                    phi += 0.02
                theta += 0.07

            frame = "\n".join("".join(out[r * kW:(r + 1) * kW]) for r in range(kH))
            sys.stdout.write("\x1b[H" + frame)
            sys.stdout.flush()
            A += kSpinA
            B += kSpinB
            time.sleep(kTickMs / 1000)
    except KeyboardInterrupt:
        pass
    finally:
        sys.stdout.write("\x1b[?25h\x1b[0m\n")


if __name__ == "__main__":
    main()

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

enum { kW = 80, kH = 24, kTickMs = 30 };
static const float kSpinA = 0.04f;
static const float kSpinB = 0.02f;
static const char kLum[] = ".,-~:;=!*#$@";

static volatile LONG gRun = 1;

static BOOL WINAPI OnCtrl(DWORD type)
{
    (void)type;
    InterlockedExchange(&gRun, 0);
    return TRUE;
}

int main(void)
{
    static char  out[kW * kH];
    static float zbuf[kW * kH];
    static char  frame[4 + kW * kH + kH];

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  oldMode = 0;
    BOOL   haveMode = GetConsoleMode(hOut, &oldMode);
    if (haveMode)
        SetConsoleMode(hOut, oldMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleCtrlHandler(OnCtrl, TRUE);

    fputs("\x1b[2J\x1b[?25l", stdout);

    float A = 0.0f, B = 0.0f;
    while (gRun) {
        memset(out, ' ', sizeof out);
        memset(zbuf, 0, sizeof zbuf);
        float cA = cosf(A), sA = sinf(A);
        float cB = cosf(B), sB = sinf(B);

        for (float theta = 0.0f; theta < 6.28f; theta += 0.07f) {
            float ct = cosf(theta), st = sinf(theta);
            for (float phi = 0.0f; phi < 6.28f; phi += 0.02f) {
                float sp = sinf(phi), cp = cosf(phi);
                float h = ct + 2.0f;
                float D = 1.0f / (sp * h * sA + st * cA + 5.0f);
                float t = sp * h * cA - st * sA;
                int   x = (int)(kW / 2 + 30.0f * D * (cp * h * cB - t * sB));
                int   y = (int)(kH / 2 + 15.0f * D * (cp * h * sB + t * cB));
                int   o = x + kW * y;
                int   N = (int)(8.0f * ((st * sA - sp * ct * cA) * cB - sp * ct * sA - st * cA - cp * ct * sB));
                if (y > 0 && y < kH - 1 && x > 0 && x < kW && D > zbuf[o]) {
                    zbuf[o] = D;
                    out[o]  = kLum[N > 0 ? N : 0];
                }
            }
        }

        char *p = frame;
        memcpy(p, "\x1b[H", 3);
        p += 3;
        for (int r = 0; r < kH; r++) {
            memcpy(p, out + r * kW, kW);
            p += kW;
            if (r < kH - 1)
                *p++ = '\n';
        }
        fwrite(frame, 1, (size_t)(p - frame), stdout);
        fflush(stdout);

        A += kSpinA;
        B += kSpinB;
        Sleep(kTickMs);
    }

    fputs("\x1b[?25h\x1b[0m\n", stdout);
    fflush(stdout);
    if (haveMode)
        SetConsoleMode(hOut, oldMode);
    return 0;
}

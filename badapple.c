// badapple.c - ASCII video player. Input: raw 8-bit gray frames (kSrcW x kSrcH, kFps).
// Output: fits to the current console size, box-filter downsample, optional wav audio via MCI.
// build: cl /O2 /W4 /nologo badapple.c winmm.lib
// run:   badapple.exe frames.gray [audio.wav]      (q or ctrl+c exits)
#define _CRT_SECURE_NO_WARNINGS
#include <conio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

enum { kSrcW = 120, kSrcH = 90, kFps = 30, kAudioOffsetMs = 0, kInvert = 0, kLoop = 1 };
static const char kRamp[] = " .:-=+*#%@"; // dark -> bright; white pixel = dense glyph
static const float kCellAspect = 2.0f;    // terminal cell height / width

static volatile LONG gRun = 1;

static BOOL WINAPI OnCtrl(DWORD type)
{
    (void)type;
    InterlockedExchange(&gRun, 0);
    return TRUE;
}

typedef struct {
    int cols, rows;
    int *x0, *x1, *y0, *y1; // source rect per column / row
} Grid;

static void GridFree(Grid *g)
{
    free(g->x0); free(g->x1); free(g->y0); free(g->y1);
    memset(g, 0, sizeof *g);
}

// fit kSrcW:kSrcH (corrected for cell aspect) inside termCols x termRows
static void GridBuild(Grid *g, int termCols, int termRows)
{
    float vidAspect = (float)kSrcW / (float)kSrcH;          // 4:3
    float maxColsByRows = (float)termRows * kCellAspect * vidAspect;
    int cols = termCols < (int)maxColsByRows ? termCols : (int)maxColsByRows;
    if (cols < 8) cols = 8;
    int rows = (int)((float)cols / (kCellAspect * vidAspect) + 0.5f);
    if (rows < 2) rows = 2;
    if (rows > termRows) rows = termRows;

    GridFree(g);
    g->cols = cols;
    g->rows = rows;
    g->x0 = malloc(sizeof(int) * (size_t)cols);
    g->x1 = malloc(sizeof(int) * (size_t)cols);
    g->y0 = malloc(sizeof(int) * (size_t)rows);
    g->y1 = malloc(sizeof(int) * (size_t)rows);
    if (!g->x0 || !g->x1 || !g->y0 || !g->y1) { fputs("oom\n", stderr); exit(1); }
    for (int i = 0; i < cols; i++) {
        g->x0[i] = i * kSrcW / cols;
        g->x1[i] = (i + 1) * kSrcW / cols;
        if (g->x1[i] <= g->x0[i]) g->x1[i] = g->x0[i] + 1;
    }
    for (int j = 0; j < rows; j++) {
        g->y0[j] = j * kSrcH / rows;
        g->y1[j] = (j + 1) * kSrcH / rows;
        if (g->y1[j] <= g->y0[j]) g->y1[j] = g->y0[j] + 1;
    }
}

static void ConsoleSize(HANDLE hOut, int *cols, int *rows)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        *cols = 120; // redirected: fall back
        *rows = 30;
    }
}

static void McI(const char *cmd)
{
    mciSendStringA(cmd, NULL, 0, NULL);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fputs("usage: badapple frames.gray [audio.wav]\n", stderr);
        return 1;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", argv[1]); return 1; }
    _fseeki64(f, 0, SEEK_END);
    long long fsize = _ftelli64(f);
    const size_t frameBytes = (size_t)kSrcW * kSrcH;
    long long nFrames = fsize / (long long)frameBytes;
    if (nFrames < 1) { fputs("no frames in file\n", stderr); fclose(f); return 1; }

    uint8_t *src = malloc(frameBytes);
    if (!src) { fclose(f); return 1; }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  oldMode = 0;
    BOOL   haveMode = GetConsoleMode(hOut, &oldMode);
    if (haveMode)
        SetConsoleMode(hOut, oldMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleCtrlHandler(OnCtrl, TRUE);

    int haveAudio = 0;
    if (argc >= 3) {
        char cmd[1100];
        snprintf(cmd, sizeof cmd, "open \"%s\" type waveaudio alias ba", argv[2]);
        haveAudio = mciSendStringA(cmd, NULL, 0, NULL) == 0;
        if (!haveAudio) fputs("audio open failed, continuing silent\n", stderr);
    }

    fputs("\x1b[?1049h\x1b[?25l\x1b[2J", stdout); // alt screen, hide cursor
    fflush(stdout);

    LARGE_INTEGER freq, t0, now;
    QueryPerformanceFrequency(&freq);
    if (haveAudio) McI("play ba");
    QueryPerformanceCounter(&t0);

    Grid g = {0};
    char *buf = NULL;
    size_t bufCap = 0;
    int lastCols = 0, lastRows = 0;
    long long lastDrawn = -1;

    while (gRun) {
        if (_kbhit()) {
            int c = _getch();
            if (c == 'q' || c == 'Q' || c == 27) break;
        }

        QueryPerformanceCounter(&now);
        double ms = (double)(now.QuadPart - t0.QuadPart) * 1000.0 / (double)freq.QuadPart;
        long long idx = (long long)((ms - kAudioOffsetMs) * kFps / 1000.0);
        if (idx < 0) idx = 0;
        if (idx >= nFrames) {
            if (!kLoop) break;
            if (haveAudio) {
                McI("seek ba to start");
                McI("play ba");
            }
            QueryPerformanceCounter(&t0);
            lastDrawn = -1;
            continue;
        }
        if (idx == lastDrawn) { Sleep(1); continue; }

        int tc, tr;
        ConsoleSize(hOut, &tc, &tr);
        if (tc != lastCols || tr != lastRows) {
            GridBuild(&g, tc, tr);
            lastCols = tc;
            lastRows = tr;
            size_t need = 16 + (size_t)g.rows * ((size_t)g.cols + 16); // 16 = max cursor-position escape
            if (need > bufCap) {
                free(buf);
                buf = malloc(need);
                bufCap = need;
                if (!buf) break;
            }
            fputs("\x1b[2J", stdout);
        }

        if (_fseeki64(f, idx * (long long)frameBytes, SEEK_SET) != 0) break;
        if (fread(src, 1, frameBytes, f) != frameBytes) break;

        // center the picture: absolute cursor position per row
        int padL = (tc - g.cols) / 2;
        int padT = (tr - g.rows) / 2;
        char *p = buf;
        for (int j = 0; j < g.rows; j++) {
            p += sprintf(p, "\x1b[%d;%dH", padT + j + 1, padL + 1);
            for (int i = 0; i < g.cols; i++) {
                unsigned sum = 0, cnt = 0;
                for (int y = g.y0[j]; y < g.y1[j]; y++) {
                    const uint8_t *row = src + (size_t)y * kSrcW;
                    for (int x = g.x0[i]; x < g.x1[i]; x++) {
                        sum += row[x];
                        cnt++;
                    }
                }
                int lum = (int)(sum / cnt);
                if (kInvert) lum = 255 - lum;
                *p++ = kRamp[lum * (int)(sizeof kRamp - 2) / 255];
            }
        }
        fwrite(buf, 1, (size_t)(p - buf), stdout);
        fflush(stdout);
        lastDrawn = idx;
    }

    if (haveAudio) {
        McI("stop ba");
        McI("close ba");
    }
    fputs("\x1b[?25h\x1b[0m\x1b[?1049l", stdout);
    fflush(stdout);
    if (haveMode)
        SetConsoleMode(hOut, oldMode);
    GridFree(&g);
    free(buf);
    free(src);
    fclose(f);
    return 0;
}

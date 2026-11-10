# ascii-terminal

<p align="center">
  <img src="assets/donut.gif" alt="spinning ASCII donut in a terminal" width="640">
</p>

Two terminal animations for Windows, drawn in ASCII with ANSI escapes.

- **donut** - spinning 3D torus. Python and C versions, same math.
- **badapple** - ASCII video player in C. Fits to the console size, re-centers on resize, syncs to wav audio, loops.

## donut

```
python donut.py
```

```
cl /O2 /W4 /nologo donut.c
donut.exe
```

Spin speed and frame delay are constants at the top of each file. Ctrl+C exits.

## badapple

The video and audio are not in this repo. Bring your own copy of the video, then convert it once (needs `ffmpeg` on PATH):

```
powershell -File prep.ps1 -Video path\to\bad_apple.mp4
```

That writes `frames.gray` (raw 8-bit gray, 120x90, 30 fps) and `audio.wav`. Then:

```
cl /O2 /W4 /nologo badapple.c winmm.lib
badapple.exe frames.gray audio.wav
```

`q`, Esc or Ctrl+C quits. Audio is optional - leave out `audio.wav` to play silent.

Config constants at the top of `badapple.c`: `kSrcW`/`kSrcH` (must match the prep size), `kFps`, `kAudioOffsetMs`, `kInvert`, `kLoop`.

Needs Windows Terminal or any console with VT sequence support.

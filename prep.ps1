param(
    [Parameter(Mandatory)][string]$Video,
    [int]$Width = 120,   # must match kSrcW in badapple.c
    [int]$Height = 90,   # must match kSrcH in badapple.c
    [int]$Fps = 30       # must match kFps in badapple.c
)

if (-not (Get-Command ffmpeg -ErrorAction SilentlyContinue)) {
    Write-Error "ffmpeg not found on PATH"
    exit 1
}

ffmpeg -hide_banner -loglevel error -y -i $Video -vf "fps=$Fps,scale=${Width}:${Height}:flags=area,format=gray" -f rawvideo frames.gray
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

ffmpeg -hide_banner -loglevel error -y -i $Video -vn -ac 2 -ar 44100 -c:a pcm_s16le audio.wav
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "wrote frames.gray and audio.wav"

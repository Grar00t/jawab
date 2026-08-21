# build.ps1 - jawab v0.3 (MSVC with gcc fallback, AVX2-aware)
param(
    [switch]$Clean,
    [switch]$Run
)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

if ($Clean) {
    Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
    Remove-Item -Force jawab.exe -ErrorAction SilentlyContinue
    Write-Host "Cleaned."
    exit 0
}

New-Item -ItemType Directory -Force -Path build | Out-Null

if (Get-Command cl -ErrorAction SilentlyContinue) {
    cl /nologo /O2 /std:c11 /W4 /arch:AVX2 /D_CRT_SECURE_NO_WARNINGS `
        src\main.c src\jawab.c /Fe:jawab.exe /Fo:build\
} elseif (Get-Command gcc -ErrorAction SilentlyContinue) {
    gcc -O2 -std=c11 -Wall -Wextra -mavx2 src/main.c src/jawab.c -o jawab.exe -lm
} else {
    throw "no compiler found (cl / gcc)"
}

if ($LASTEXITCODE -ne 0) { throw "build failed" }
Write-Host "[jawab] build ok -> jawab.exe"

if ($Run) {
    & .\jawab.exe ask "sovereignty" corpus\seed.txt
}

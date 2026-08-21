# build.ps1 - jawab v0.2 (MSVC with gcc fallback)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

if (Get-Command cl -ErrorAction SilentlyContinue) {
    cl /nologo /O2 /std:c11 /W4 /D_CRT_SECURE_NO_WARNINGS src\main.c src\jawab.c /Fe:jawab.exe
} elseif (Get-Command gcc -ErrorAction SilentlyContinue) {
    gcc -O2 -std=c11 -Wall src/main.c src/jawab.c -o jawab.exe -lm
} else {
    throw "no compiler found (cl / gcc)"
}
Write-Host "[jawab] build ok"

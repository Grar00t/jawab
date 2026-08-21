# build.ps1 - Windows build script for jawab
# Usage: .\build.ps1 [-Clean] [-Run]

param(
    [switch]$Clean,
    [switch]$Run
)

$BuildDir = "build"
$Bin = Join-Path $BuildDir "jawab.exe"
$Sources = @("src/jawab.c", "src/main.c")

if ($Clean) {
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-Host "Cleaned $BuildDir"
    }
    exit 0
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

$cc = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $cc) {
    $cc = Get-Command clang -ErrorAction SilentlyContinue
}

if (-not $cc) {
    Write-Error "No C compiler found (gcc or clang). Install MinGW-w64 or LLVM and add it to PATH."
    exit 1
}

$compiler = $cc.Name
Write-Host "Using compiler: $compiler"

$flags = @("-std=c11", "-Wall", "-Wextra", "-O2", "-Isrc", "-o", $Bin) + $Sources

& $compiler.Replace(".exe","") @flags

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    exit $LASTEXITCODE
}

Write-Host "Build succeeded: $Bin"

if ($Run) {
    & $Bin "corpus/seed.txt"
}

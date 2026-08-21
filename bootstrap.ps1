# bootstrap.ps1 - zero-friction Windows setup for jawab
# Checks for git and a C compiler; installs whichever is missing via winget;
# clones the repo (if not already present); builds; runs a demo query.
#
# Usage (from an empty folder):
#   irm https://raw.githubusercontent.com/Grar00t/jawab/main/bootstrap.ps1 | iex
# or, if you already have the repo checked out:
#   .\bootstrap.ps1
#
# NOTE: this script has not been executed on a real Windows machine as part
# of its authoring -- it was written against documented winget/PowerShell
# behavior, not verified end-to-end. If a step fails, the error message will
# tell you which package/command to install manually.

$ErrorActionPreference = "Stop"

function Test-Cmd($name) {
    return [bool](Get-Command $name -ErrorAction SilentlyContinue)
}

function Refresh-Path {
    $machine = [System.Environment]::GetEnvironmentVariable("PATH", "Machine")
    $user    = [System.Environment]::GetEnvironmentVariable("PATH", "User")
    $env:PATH = "$machine;$user"
}

Write-Host "[bootstrap] checking prerequisites..." -ForegroundColor Cyan

if (-not (Test-Cmd winget)) {
    Write-Warning "[bootstrap] winget not found. Install App Installer from the Microsoft Store, then re-run this script."
    Write-Warning "[bootstrap] Falling back to manual instructions for git/gcc below."
}

# --- Git ---
if (Test-Cmd git) {
    Write-Host "[bootstrap] git: found" -ForegroundColor Green
} elseif (Test-Cmd winget) {
    Write-Host "[bootstrap] git: missing, installing via winget..." -ForegroundColor Yellow
    winget install --id Git.Git -e --silent --accept-package-agreements --accept-source-agreements
    Refresh-Path
    if (-not (Test-Cmd git)) {
        throw "git install via winget did not put 'git' on PATH. Restart your terminal and re-run, or install manually: https://git-scm.com/download/win"
    }
} else {
    throw "git not found and winget unavailable. Install manually: https://git-scm.com/download/win"
}

# --- C compiler (cl or gcc) ---
$hasCompiler = (Test-Cmd cl) -or (Test-Cmd gcc)
if ($hasCompiler) {
    Write-Host "[bootstrap] C compiler: found" -ForegroundColor Green
} elseif (Test-Cmd winget) {
    Write-Host "[bootstrap] no C compiler found, installing a prebuilt MinGW-w64 GCC via winget..." -ForegroundColor Yellow
    # WinLibs ships a ready-to-use gcc.exe with no separate MSYS2 setup step.
    # If this package id has moved, `winget search gcc` to find the current one.
    try {
        winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e --silent --accept-package-agreements --accept-source-agreements
    } catch {
        Write-Warning "[bootstrap] winget install of WinLibs GCC failed. Install a compiler manually:"
        Write-Warning "  - MSYS2 (https://www.msys2.org/) then: pacman -S mingw-w64-ucrt-x86_64-gcc"
        Write-Warning "  - or Visual Studio Build Tools (https://visualstudio.microsoft.com/downloads/) for cl.exe"
        throw
    }
    Refresh-Path
    if (-not ((Test-Cmd cl) -or (Test-Cmd gcc))) {
        throw "Compiler installed but not found on PATH yet. Close and reopen your terminal, then re-run this script."
    }
} else {
    throw "No C compiler (cl/gcc) found and winget unavailable. Install MSYS2 or Visual Studio Build Tools manually."
}

# --- Clone (skip if already inside or next to a jawab checkout) ---
if (Test-Path ".\build.ps1" -PathType Leaf) {
    Write-Host "[bootstrap] already inside a jawab checkout, skipping clone" -ForegroundColor Green
} elseif (Test-Path ".\jawab\build.ps1" -PathType Leaf) {
    Write-Host "[bootstrap] jawab\ already cloned, skipping clone" -ForegroundColor Green
    Set-Location jawab
} else {
    Write-Host "[bootstrap] cloning jawab..." -ForegroundColor Cyan
    git clone https://github.com/Grar00t/jawab.git
    Set-Location jawab
}

Write-Host "[bootstrap] building and running demo query..." -ForegroundColor Cyan
.\build.ps1 -Run

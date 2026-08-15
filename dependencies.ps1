#!/usr/bin/env pwsh
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$envName = "linux"

if ($env:WSL_DISTRO_NAME) {
    $envName = "wsl"
} elseif ($IsWindows -or $env:OS -eq "Windows_NT") {
    $envName = "windows"
}

function Test-Cmd {
    param([string]$Name)
    return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Test-PythonTk {
    $python = "python"
    if (Test-Cmd "python3") { $python = "python3" }
    elseif (Test-Cmd "py") { $python = "py" }

    try {
        if ($python -eq "py") {
            & py -c "import tkinter" | Out-Null
        } else {
            & $python -c "import tkinter" | Out-Null
        }
        return $true
    }
    catch {
        return $false
    }
}

function Print-Manual {
    param([string]$Environment)

    Write-Host "=== Instalacao manual ==="
    switch ($Environment) {
        "windows" {
            Write-Host "winget install Git.Git"
            Write-Host "winget install GnuWin32.Make"
            Write-Host "winget install --id Microsoft.VisualStudio.2022.BuildTools"
            Write-Host "choco install -y git make mingw nasm qemu"
            Write-Host "py -m pip install --user tk"
        }
        "wsl" {
            Write-Host "sudo apt-get update"
            Write-Host "sudo apt-get install -y build-essential git nasm qemu-system-x86 python3-tk"
        }
        default {
            Write-Host "sudo apt-get update"
            Write-Host "sudo apt-get install -y build-essential git nasm qemu-system-x86 python3-tk"
        }
    }
}

function Install-Linux {
    Write-Host "[deps] Detectado ambiente: $envName"

    $missing = @()
    foreach ($cmd in @("git", "make", "gcc", "nasm", "python3")) {
        if (-not (Test-Cmd $cmd)) {
            $missing += $cmd
        }
    }

    if (-not (Test-PythonTk)) {
        $missing += "python3-tk"
    }

    if ($missing.Count -eq 0) {
        Write-Host "[deps] Todas as dependencias basicas ja estao instaladas."
        return 0
    }

    if (Test-Cmd "apt-get") {
        Write-Host "[deps] Instalando: $($missing -join ' ')"
        sudo apt-get update
        sudo apt-get install -y $missing
        return 0
    }

    if (Test-Cmd "dnf") {
        Write-Host "[deps] Instalando: $($missing -join ' ')"
        sudo dnf install -y $missing
        return 0
    }

    if (Test-Cmd "pacman") {
        Write-Host "[deps] Instalando: $($missing -join ' ')"
        sudo pacman -S --noconfirm $missing
        return 0
    }

    if (Test-Cmd "apk") {
        Write-Host "[deps] Instalando: $($missing -join ' ')"
        sudo apk add --no-cache $missing
        return 0
    }

    Print-Manual $envName
    return 1
}

function Install-Windows {
    Write-Host "[deps] Detectado ambiente: windows"

    $missing = @()
    foreach ($cmd in @("git", "make", "gcc", "nasm")) {
        if (-not (Test-Cmd $cmd)) {
            $missing += $cmd
        }
    }

    if (-not (Test-Cmd "qemu-system-x86_64") -and -not (Test-Cmd "qemu-system-i386")) {
        $missing += "qemu"
    }

    if (-not (Test-PythonTk)) {
        $missing += "python-tk"
    }

    if ($missing.Count -eq 0) {
        Write-Host "[deps] Todas as dependencias basicas ja estao instaladas."
        return 0
    }

    if (Test-Cmd "winget") {
        Write-Host "[deps] Instalando via winget..."
        winget install --accept-source-agreements --accept-package-agreements -e --id Git.Git --id GnuWin32.Make --id Microsoft.VisualStudio.2022.BuildTools --id QEMU.QEMU --id Python.Python.3.12
        return 0
    }

    if (Test-Cmd "choco") {
        Write-Host "[deps] Instalando via choco..."
        choco install -y git make mingw nasm qemu python
        return 0
    }

    Print-Manual "windows"
    return 1
}

switch ($envName) {
    "windows" {
        exit (Install-Windows)
    }
    default {
        exit (Install-Linux)
    }
}

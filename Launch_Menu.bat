@echo off
title Noble Fermi - AAA Tactical FPS Launcher
echo ==============================================================
echo   NOBLE FERMI // MODERN MILITARY TACTICAL WARFARE
echo   Launching Start Menu...
echo ==============================================================

set "PKG_BIN=%~dp0Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.exe"
set "PKG_ROOT=%~dp0Packaged\Windows\NobleFermi.exe"
set "DEV_BIN=%~dp0Binaries\Win64\NobleFermi.exe"
set "UE_EDITOR=D:\epuc\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "UPROJECT=%~dp0NobleFermi.uproject"

REM 1. Priority: Launch optimized packaged game (instant boot, Tachyon laser splash, loading bar)
if exist "%PKG_BIN%" (
    echo [LAUNCHER] Launching Noble Fermi standalone release...
    cd /d "%~dp0Packaged\Windows\NobleFermi\Binaries\Win64"
    start "" "NobleFermi.exe" -d3d11 -windowed -ResX=1280 -ResY=720 -log %*
    exit /b 0
)

if exist "%PKG_ROOT%" (
    echo [LAUNCHER] Launching Noble Fermi packaged build...
    cd /d "%~dp0Packaged\Windows"
    start "" "NobleFermi.exe" -d3d11 -windowed -ResX=1280 -ResY=720 -log %*
    exit /b 0
)

if exist "%DEV_BIN%" (
    echo [LAUNCHER] Launching development binary...
    cd /d "%~dp0Binaries\Win64"
    start "" "NobleFermi.exe" -d3d11 -windowed -ResX=1280 -ResY=720 -log %*
    exit /b 0
)

REM 2. Fallback: Launch via Unreal Editor standalone mode with required Engine DLL path
if exist "%UE_EDITOR%" if exist "%UPROJECT%" (
    echo [LAUNCHER] Running via Unreal Engine editor standalone mode...
    set "DOTNET_ROOT=D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"
    set "PATH=D:\epuc\UE_5.8\Engine\Binaries\Win64;%~dp0Binaries\Win64;%DOTNET_ROOT%;%PATH%"
    cd /d "D:\epuc\UE_5.8\Engine\Binaries\Win64"
    start "" "%UE_EDITOR%" "%UPROJECT%" /Game/Maps/MenuMap?game=/Script/NobleFermi.FPSMenuGameMode -game -d3d11 -windowed -ResX=1280 -ResY=720 -log %*
    exit /b 0
)

echo [ERROR] No runnable game binary found!
pause

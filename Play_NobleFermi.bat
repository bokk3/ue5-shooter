@echo off
title Noble Fermi - Standalone Client (DX11)
echo ==============================================================
echo   NOBLE FERMI // MODERN MILITARY TACTICAL WARFARE
echo   Launching Standalone Client in High-Performance DX11 Mode...
echo ==============================================================

set "UE_EDITOR=D:\epuc\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "UPROJECT=%~dp0NobleFermi.uproject"
set "PKG_EXE=%~dp0Packaged\Windows\NobleFermi.exe"

if exist "%UE_EDITOR%" if exist "%UPROJECT%" (
    echo [LAUNCHER] Running latest live build via Unreal Engine standalone mode...
    start "" "%UE_EDITOR%" "%UPROJECT%" /Game/Maps/MenuMap?game=/Script/NobleFermi.FPSMenuGameMode -game -d3d11 -windowed -ResX=1280 -ResY=720 -log
    exit /b 0
)

if exist "%PKG_EXE%" (
    echo [LAUNCHER] Launching packaged build in DX11 mode...
    cd /d "%~dp0Packaged\Windows"
    start "" "NobleFermi.exe" -d3d11 -windowed -ResX=1280 -ResY=720 -log
    exit /b 0
)

echo [ERROR] No runnable game binary found!
pause

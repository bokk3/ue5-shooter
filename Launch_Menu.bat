@echo off
title Noble Fermi - AAA Tactical FPS Launcher
echo ==============================================================
echo   NOBLE FERMI // MODERN MILITARY TACTICAL WARFARE
echo   Configuring DLL search paths and Launching Start Menu...
echo ==============================================================

set "GAME_ROOT=%~dp0"
set "BIN_WIN64=%~dp0Packaged\Windows\NobleFermi\Binaries\Win64"
set "ENG_WIN64=%~dp0Packaged\Windows\Engine\Binaries\Win64"
set "PKG_ROOT=%~dp0Packaged\Windows"

set "PATH=%BIN_WIN64%;%ENG_WIN64%;%PKG_ROOT%;%GAME_ROOT%;%PATH%"

cd /d "%PKG_ROOT%"
start "" "NobleFermi.exe" -windowed -ResX=1280 -ResY=720 -log
exit

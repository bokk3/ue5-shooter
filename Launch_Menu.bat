@echo off
title Noble Fermi - AAA Tactical FPS Launcher
echo ==============================================================
echo   NOBLE FERMI // MODERN MILITARY TACTICAL WARFARE
echo   Launching Standalone Interactive Start Menu...
echo ==============================================================
cd /d "%~dp0Packaged\Windows"
start "" "NobleFermi.exe" -windowed -ResX=1280 -ResY=720 -log
exit

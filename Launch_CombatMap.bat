@echo off
title Noble Fermi - Tactical Combat Arena (Rust Tactical)
echo ==============================================================
echo   NOBLE FERMI // MODERN MILITARY TACTICAL WARFARE
echo   Map: Rust Tactical (60m x 60m Competitive Arena)
echo   GameMode: FPSMultiplayerGameMode (128-Tick, TaskForce vs OpFor)
echo   Launching Standalone Client with Visual Log Console...
echo ==============================================================
start "" "%~dp0Packaged\Windows\NobleFermi.exe" ?game=/Script/NobleFermi.FPSMultiplayerGameMode -windowed -ResX=1280 -ResY=720 -log
exit

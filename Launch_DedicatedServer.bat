@echo off
title Noble Fermi - Dedicated Server (128 Tickrate)
echo ==============================================================
echo   NOBLE FERMI // DEDICATED VPS SERVER INSTANCE
echo   Tickrate: 128 Hz ^| Port: 7777 ^| Max Players: 18
echo ==============================================================
"%~dp0Packaged\Windows\NobleFermi.exe" -server -nullrhi -log -PORT=7777 -TickRate=128 -MaxPlayers=18
pause

@echo off
REM ==============================================================================
REM Noble Fermi - Windows Dedicated Server Runner
REM ==============================================================================
title Noble Fermi Dedicated Server
set PORT=7777
set QUERY_PORT=27015
set MAX_PLAYERS=16
set TICK_RATE=128
set SERVER_NAME=Noble Fermi Windows Dedicated Server
set MAP_NAME=/Game/Maps/TDM_Crash

echo =================================================================
echo  Starting Noble Fermi Dedicated Server on Windows Server
echo  Port: %PORT% ^| Tickrate: %TICK_RATE%Hz
echo =================================================================

:START_SERVER
if exist NobleFermiServer.exe (
    set SERVER_EXE=NobleFermiServer.exe
    set SERVER_ARGS="%MAP_NAME%" -log -unattended -Port=%PORT% -QueryPort=%QUERY_PORT% -MaxPlayers=%MAX_PLAYERS% -TickRate=%TICK_RATE% -ServerName="%SERVER_NAME%"
) else (
    set SERVER_EXE=NobleFermi.exe
    set SERVER_ARGS="%MAP_NAME%" -server -nullrhi -log -unattended -Port=%PORT% -QueryPort=%QUERY_PORT% -MaxPlayers=%MAX_PLAYERS% -TickRate=%TICK_RATE% -ServerName="%SERVER_NAME%"
)
echo [%DATE% %TIME%] Launching %SERVER_EXE%...
%SERVER_EXE% %SERVER_ARGS%

echo [%DATE% %TIME%] Server closed. Restarting in 5 seconds...
timeout /t 5 /nobreak
goto START_SERVER

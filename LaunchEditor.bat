@echo off
REM ==============================================================================
REM Noble Fermi - Launch Unreal Editor 5.8
REM ==============================================================================

set "DOTNET_ROOT=D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"
set "PATH=%DOTNET_ROOT%;%PATH%"

echo Launching Noble Fermi in Unreal Editor 5.8...
start "" "D:\epuc\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0NobleFermi.uproject"

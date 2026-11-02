@echo off
set "DOTNET_ROOT=D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"
set "PATH=%DOTNET_ROOT%;%PATH%"

echo Building NobleFermiEditor Win64 Development...
"D:\epuc\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" NobleFermiEditor Win64 Development -Project="%~dp0NobleFermi.uproject" -WaitMutex -progress
if %ERRORLEVEL% NEQ 0 (
    echo Build failed with error %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
echo Build succeeded!

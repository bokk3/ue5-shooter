@echo off
REM ==============================================================================
REM Noble Fermi - Visual Studio Project Files Generator
REM Configures bundled .NET 10 runtime and invokes UnrealBuildTool.
REM ==============================================================================

set "DOTNET_ROOT=D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"
set "PATH=%DOTNET_ROOT%;%PATH%"

echo =================================================================
echo  Generating Visual Studio 2026 Solution for Noble Fermi (UE 5.8)
echo =================================================================

"D:\epuc\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="%~dp0NobleFermi.uproject" -game -engine -progress

if %ERRORLEVEL% EQU 0 (
    echo.
    echo SUCCESS: Visual Studio project files generated!
    echo You can now open NobleFermi.sln in Visual Studio or double-click NobleFermi.uproject to open in Unreal Editor.
) else (
    echo.
    echo ERROR: Failed to generate project files. Exit code: %ERRORLEVEL%
)

pause

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$code = @"
using System;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }
}
"@
Add-Type -TypeDefinition $code

$exePath = "C:\Users\Boris\Documents\antigravity\noble-fermi\Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.exe"
if (-not (Test-Path $exePath)) {
    $exePath = "C:\Users\Boris\Documents\antigravity\noble-fermi\Binaries\Win64\NobleFermi.exe"
}

Write-Host "Launching Noble Fermi at $exePath..."
$process = Start-Process -FilePath $exePath -ArgumentList "/Engine/Maps/Entry?game=/Script/NobleFermi.FPSMultiplayerGameMode -windowed -ResX=1280 -ResY=720 -log" -PassThru

Write-Host "Waiting 12 seconds for world generation, player pawn spawn, and HUD initialization..."
Start-Sleep -Seconds 12

$outDir = "C:\Users\Boris\Documents\antigravity\noble-fermi\Saved\Screenshots"
if (-not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}
$outPath = Join-Path $outDir "HUD_In_Action.png"

# Attempt window capture
$captured = $false
if ($process.MainWindowHandle -ne [IntPtr]::Zero) {
    [Win32]::SetForegroundWindow($process.MainWindowHandle)
    Start-Sleep -Milliseconds 500
    
    $rect = New-Object Win32+RECT
    if ([Win32]::GetWindowRect($process.MainWindowHandle, [ref]$rect)) {
        $width = $rect.Right - $rect.Left
        $height = $rect.Bottom - $rect.Top
        if ($width -gt 100 -and $height -gt 100) {
            Write-Host "Capturing window: $width x $height at ($($rect.Left), $($rect.Top))"
            $bmp = New-Object System.Drawing.Bitmap $width, $height
            $g = [System.Drawing.Graphics]::FromImage($bmp)
            $g.CopyFromScreen($rect.Left, $rect.Top, 0, 0, (New-Object System.Drawing.Size $width, $height))
            $bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
            $g.Dispose()
            $bmp.Dispose()
            $captured = $true
            Write-Host "Window screenshot saved to $outPath"
        }
    }
}

if (-not $captured) {
    Write-Host "Fallback to full desktop capture..."
    $screen = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bmp = New-Object System.Drawing.Bitmap $screen.Width, $screen.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($screen.Location, [System.Drawing.Point]::Empty, $screen.Size)
    $bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
    Write-Host "Desktop screenshot saved to $outPath"
}

Write-Host "Closing game process..."
Stop-Process -Id $process.Id -Force
Write-Host "Done!"

$exe = "c:\Users\Boris\Documents\antigravity\noble-fermi\Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.exe"
$shotDir = "c:\Users\Boris\Documents\antigravity\noble-fermi\Packaged\Windows\NobleFermi\Docs\Screenshots"
$artDir = "C:\Users\Boris\.gemini\antigravity\brain\c135ce77-4bef-4380-b69a-99d73fc7b5a7"

$shots = @(
    @{ Name = "hangar_grip_lowready.png"; Time = "7.0" },
    @{ Name = "hangar_grip_magcheck.png"; Time = "1.8" },
    @{ Name = "hangar_grip_barrelwipe.png"; Time = "4.0" }
)

foreach ($s in $shots) {
    $dest = Join-Path $shotDir $s.Name
    if (Test-Path $dest) { Remove-Item $dest -Force }
    Write-Host "Capturing $($s.Name) at InspectTime=$($s.Time)..."
    $argList = "-TakeMenuShot -MenuShotDelay=5.0 -InspectTime=$($s.Time) -ShotName=$($s.Name) -ResX=1920 -ResY=1080 -Windowed -NoSound"
    $p = Start-Process -FilePath $exe -ArgumentList $argList -PassThru
    $p.WaitForExit(45000) | Out-Null
    if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }
    if (Test-Path $dest) {
        Copy-Item -Force $dest (Join-Path $artDir $s.Name)
        $item = Get-Item $dest
        Write-Host "SUCCESS: $($s.Name) (time: $($item.LastWriteTime), size: $($item.Length)) copied to artifacts."
    } else {
        Write-Host "FAILED: $dest not created!"
    }
}

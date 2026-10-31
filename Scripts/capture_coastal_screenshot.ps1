$shotPath1 = "Packaged\Windows\NobleFermi\Docs\Screenshots\gameplay_coastal_operations.png"
$shotPath2 = "Docs\Screenshots\gameplay_coastal_operations.png"

if (Test-Path $shotPath1) { Remove-Item $shotPath1 -Force }
if (Test-Path $shotPath2) { Remove-Item $shotPath2 -Force }

Write-Host "Launching Packaged NobleFermi into Coastal Operations match to capture screenshot with GameplayShotDelay=7.0..."
$proc = Start-Process -FilePath "Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.exe" -ArgumentList "/Engine/Maps/Entry?game=/Script/NobleFermi.FPSMultiplayerGameMode?Theater=Coastal -TakeGameplayShot -GameplayShotDelay=7.0 -windowed -ResX=1920 -ResY=1080 -log" -PassThru

$timeout = 40
$elapsed = 0
$foundPath = $null

while ($elapsed -lt $timeout -and -not $proc.HasExited) {
    Start-Sleep -Seconds 1
    $elapsed++
    if (Test-Path $shotPath1) {
        $foundPath = $shotPath1
        Write-Host "Screenshot detected at $shotPath1 after $elapsed seconds!"
        Start-Sleep -Seconds 2
        break
    }
    if (Test-Path $shotPath2) {
        $foundPath = $shotPath2
        Write-Host "Screenshot detected at $shotPath2 after $elapsed seconds!"
        Start-Sleep -Seconds 2
        break
    }
}

if (-not $proc.HasExited) {
    Write-Host "Waiting up to 5 more seconds for clean engine exit..."
    $proc | Wait-Process -Timeout 5 -ErrorAction SilentlyContinue
}

if (-not $proc.HasExited) {
    Write-Host "Stopping process after $elapsed seconds..."
    Stop-Process -Id $proc.Id -Force
}

if ($foundPath -and (Test-Path $foundPath)) {
    Write-Host "Screenshot captured successfully at $foundPath"
    Copy-Item $foundPath "Docs\Screenshots\gameplay_coastal_operations.png" -Force
    Write-Host "Copied to Docs\Screenshots\gameplay_coastal_operations.png"
} elseif (Test-Path $shotPath1) {
    Copy-Item $shotPath1 "Docs\Screenshots\gameplay_coastal_operations.png" -Force
    Write-Host "Found at $shotPath1 and copied to Docs\Screenshots\gameplay_coastal_operations.png"
} else {
    Write-Host "Screenshot not found."
}

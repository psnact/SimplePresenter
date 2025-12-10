# SimplePresenter Deployment Script
# Packages the application with all required dependencies

param(
    [string]$QtPath = "C:\Qt\6.5.0\msvc2019_64",
    [string]$FFmpegPath = "C:\ffmpeg",
    [string]$BuildPath = ".\build\Release",
    [string]$DeployPath = ".\deploy"
)

Write-Host "SimplePresenter Deployment Script" -ForegroundColor Cyan
Write-Host "===================================" -ForegroundColor Cyan
Write-Host ""

# Verify paths exist
if (-not (Test-Path $QtPath)) {
    Write-Host "ERROR: Qt path not found: $QtPath" -ForegroundColor Red
    Write-Host "Please specify correct Qt path with -QtPath parameter" -ForegroundColor Yellow
    exit 1
}

if (-not (Test-Path $FFmpegPath)) {
    Write-Host "ERROR: FFmpeg path not found: $FFmpegPath" -ForegroundColor Red
    Write-Host "Please specify correct FFmpeg path with -FFmpegPath parameter" -ForegroundColor Yellow
    exit 1
}

if (-not (Test-Path "$BuildPath\SimplePresenter.exe")) {
    Write-Host "ERROR: SimplePresenter.exe not found in: $BuildPath" -ForegroundColor Red
    Write-Host "Please build the project first" -ForegroundColor Yellow
    exit 1
}

# Create deployment directory
Write-Host "Creating deployment directory..." -ForegroundColor Green
if (Test-Path $DeployPath) {
    Remove-Item -Recurse -Force $DeployPath
}
New-Item -ItemType Directory -Force -Path $DeployPath | Out-Null

# Copy executable
Write-Host "Copying executable..." -ForegroundColor Green
Copy-Item "$BuildPath\SimplePresenter.exe" -Destination $DeployPath

# Run Qt deployment tool
Write-Host "Running Qt deployment tool..." -ForegroundColor Green
$windeployqt = "$QtPath\bin\windeployqt.exe"
if (Test-Path $windeployqt) {
    & $windeployqt "$DeployPath\SimplePresenter.exe" --release --no-translations --no-system-d3d-compiler
} else {
    Write-Host "WARNING: windeployqt.exe not found, skipping Qt deployment" -ForegroundColor Yellow
}

# Copy FFmpeg DLLs
Write-Host "Copying FFmpeg DLLs..." -ForegroundColor Green
$ffmpegBin = "$FFmpegPath\bin"
if (Test-Path $ffmpegBin) {
    Get-ChildItem "$ffmpegBin\*.dll" | ForEach-Object {
        Copy-Item $_.FullName -Destination $DeployPath
    }
} else {
    Write-Host "WARNING: FFmpeg bin directory not found" -ForegroundColor Yellow
}

# Copy data directories
Write-Host "Copying data directories..." -ForegroundColor Green
if (Test-Path ".\data") {
    Copy-Item -Recurse ".\data" -Destination "$DeployPath\data"
} else {
    Write-Host "WARNING: data directory not found, creating empty structure" -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path "$DeployPath\data\bibles" | Out-Null
    New-Item -ItemType Directory -Force -Path "$DeployPath\data\songs" | Out-Null
    New-Item -ItemType Directory -Force -Path "$DeployPath\data\services" | Out-Null
    New-Item -ItemType Directory -Force -Path "$DeployPath\data\backgrounds" | Out-Null
}

# Copy documentation
Write-Host "Copying documentation..." -ForegroundColor Green
Copy-Item ".\README.md" -Destination $DeployPath -ErrorAction SilentlyContinue
Copy-Item ".\USAGE.md" -Destination $DeployPath -ErrorAction SilentlyContinue
Copy-Item ".\LICENSE" -Destination $DeployPath -ErrorAction SilentlyContinue

# Create README in deploy folder
$deployReadme = @"
SimplePresenter
===============

To run the application:
1. Double-click SimplePresenter.exe
2. Configure settings via Tools -> Settings
3. See USAGE.md for detailed instructions

Data Directories:
- data/bibles/    - Place Bible XML files here
- data/songs/     - Place song .txt files here
- data/services/  - Saved playlists (.service files)
- data/backgrounds/ - Background images and videos

For more information, see README.md and USAGE.md
"@

$deployReadme | Out-File -FilePath "$DeployPath\START_HERE.txt" -Encoding UTF8

Write-Host ""
Write-Host "Deployment complete!" -ForegroundColor Green
Write-Host "Output directory: $DeployPath" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Test the application in $DeployPath" -ForegroundColor White
Write-Host "2. Create installer or ZIP archive for distribution" -ForegroundColor White
Write-Host ""

# Calculate deployment size
$size = (Get-ChildItem -Recurse $DeployPath | Measure-Object -Property Length -Sum).Sum / 1MB
Write-Host "Deployment size: $([math]::Round($size, 2)) MB" -ForegroundColor Cyan

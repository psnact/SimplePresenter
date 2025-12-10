# SimplePresenter Build Script
# Automates the CMake configuration and build process

param(
    [string]$QtPath = "",
    [string]$FFmpegPath = "C:\ffmpeg",
    [string]$BuildType = "Release",
    [switch]$Clean,
    [switch]$Configure,
    [switch]$Build,
    [switch]$Run,
    [switch]$All
)

$ErrorActionPreference = "Stop"

Write-Host "SimplePresenter Build Script" -ForegroundColor Cyan
Write-Host "=============================" -ForegroundColor Cyan
Write-Host ""

# If no specific action specified, do all
if (-not ($Clean -or $Configure -or $Build -or $Run)) {
    $All = $true
}

if ($All) {
    $Configure = $true
    $Build = $true
}

# Auto-detect Qt path if not specified
if ([string]::IsNullOrEmpty($QtPath)) {
    Write-Host "Auto-detecting Qt installation..." -ForegroundColor Yellow
    
    # Common Qt installation locations
    $possiblePaths = @(
        "C:\Qt\6.8.0\msvc2022_64",
        "C:\Qt\6.7.0\msvc2022_64",
        "C:\Qt\6.6.0\msvc2022_64",
        "C:\Qt\6.5.0\msvc2022_64",
        "C:\Qt\6.8.0\msvc2019_64",
        "C:\Qt\6.7.0\msvc2019_64",
        "C:\Qt\6.6.0\msvc2019_64",
        "C:\Qt\6.5.0\msvc2019_64",
        "C:\Qt\6.4.0\msvc2019_64",
        "C:\Qt\6.3.0\msvc2019_64"
    )
    
    # Also check for Qt installations in C:\Qt\ and prefer the newest version
    if (Test-Path "C:\Qt") {
        $qtVersions = Get-ChildItem "C:\Qt" -Directory |
            Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } |
            Sort-Object { [version]$_.Name } -Descending

        foreach ($version in $qtVersions) {
            $possiblePaths += "$(($version.FullName))\msvc2022_64"
            $possiblePaths += "$(($version.FullName))\msvc2019_64"
        }
    }
    
    foreach ($path in $possiblePaths) {
        if (Test-Path $path) {
            $QtPath = $path
            Write-Host "Found Qt at: $QtPath" -ForegroundColor Green
            break
        }
    }
}

# Verify Qt path
if ([string]::IsNullOrEmpty($QtPath) -or -not (Test-Path $QtPath)) {
    Write-Host ""
    Write-Host "ERROR: Qt installation not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install Qt 6.5+ or specify the Qt path manually:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1 -QtPath 'C:\Path\To\Qt\6.x.x\msvcXXXX_64'" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Common Qt locations to check:" -ForegroundColor Yellow
    Write-Host "  - C:\Qt\6.8.0\msvc2022_64" -ForegroundColor White
    Write-Host "  - C:\Qt\6.7.0\msvc2022_64" -ForegroundColor White
    Write-Host "  - C:\Qt\6.5.0\msvc2019_64" -ForegroundColor White
    Write-Host ""
    Write-Host "Download Qt from: https://www.qt.io/download" -ForegroundColor Cyan
    Write-Host ""
    exit 1
}

Write-Host "Using Qt: $QtPath" -ForegroundColor Green

# Verify FFmpeg path
if (-not (Test-Path $FFmpegPath)) {
    Write-Host "WARNING: FFmpeg path not found: $FFmpegPath" -ForegroundColor Yellow
    Write-Host "Continuing anyway, but build may fail if FFmpeg is required" -ForegroundColor Yellow
}

$BuildDir = "build"

# Clean build directory
if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Green
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-Host "Build directory cleaned" -ForegroundColor Green
    } else {
        Write-Host "Build directory doesn't exist, nothing to clean" -ForegroundColor Yellow
    }
    Write-Host ""
}

# Configure with CMake
if ($Configure) {
    Write-Host "Configuring with CMake..." -ForegroundColor Green
    
    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    }
    
    Push-Location $BuildDir
    
    $cmakeArgs = @(
        "..",
        "-G", "Visual Studio 17 2022",
        "-A", "x64",
        "-DCMAKE_PREFIX_PATH=$QtPath"
    )
    
    if (Test-Path $FFmpegPath) {
        $cmakeArgs += "-DFFMPEG_ROOT=$FFmpegPath"
    }
    
    Write-Host "Running: cmake $($cmakeArgs -join ' ')" -ForegroundColor Cyan
    & cmake $cmakeArgs
    
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
        exit 1
    }
    
    Pop-Location
    Write-Host "Configuration complete" -ForegroundColor Green
    Write-Host ""
}

# Build with CMake
if ($Build) {
    Write-Host "Building project ($BuildType)..." -ForegroundColor Green
    
    if (-not (Test-Path $BuildDir)) {
        Write-Host "ERROR: Build directory doesn't exist. Run with -Configure first" -ForegroundColor Red
        exit 1
    }
    
    $buildArgs = @(
        "--build", $BuildDir,
        "--config", $BuildType,
        "--parallel"
    )
    
    Write-Host "Running: cmake $($buildArgs -join ' ')" -ForegroundColor Cyan
    & cmake $buildArgs
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Build failed" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Build complete" -ForegroundColor Green
    Write-Host ""
}

# Run the application
if ($Run) {
    $exePath = "$BuildDir\$BuildType\SimplePresenter.exe"
    
    if (-not (Test-Path $exePath)) {
        Write-Host "ERROR: Executable not found: $exePath" -ForegroundColor Red
        Write-Host "Build the project first with -Build" -ForegroundColor Yellow
        exit 1
    }
    
    Write-Host "Running SimplePresenter..." -ForegroundColor Green
    Write-Host ""
    
    # Add Qt and FFmpeg to PATH for DLL loading
    $env:PATH = "$QtPath\bin;$FFmpegPath\bin;$env:PATH"
    
    & $exePath
}

Write-Host ""
Write-Host "Done!" -ForegroundColor Green

# Show helpful next steps
if ($Build -and -not $Run) {
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "  Run application:  .\build.ps1 -Run" -ForegroundColor White
    Write-Host "  Deploy package:   .\deploy.ps1" -ForegroundColor White
}

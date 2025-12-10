# Find Qt Installations
# Helper script to locate Qt on your system

Write-Host "Searching for Qt installations..." -ForegroundColor Cyan
Write-Host ""

$found = $false

# Check common locations
$commonPaths = @(
    "C:\Qt",
    "C:\Program Files\Qt",
    "C:\Program Files (x86)\Qt",
    "$env:USERPROFILE\Qt",
    "D:\Qt"
)

foreach ($basePath in $commonPaths) {
    if (Test-Path $basePath) {
        Write-Host "Checking: $basePath" -ForegroundColor Yellow
        
        # Find version directories
        $versions = Get-ChildItem $basePath -Directory -ErrorAction SilentlyContinue | 
                    Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' }
        
        foreach ($version in $versions) {
            # Find compiler directories
            $compilers = Get-ChildItem $version.FullName -Directory -ErrorAction SilentlyContinue |
                        Where-Object { $_.Name -match 'msvc\d+_64' }
            
            foreach ($compiler in $compilers) {
                # Check if it has bin/qmake.exe
                $qmakePath = Join-Path $compiler.FullName "bin\qmake.exe"
                if (Test-Path $qmakePath) {
                    Write-Host "  Found: $($compiler.FullName)" -ForegroundColor Green
                    
                    # Get Qt version
                    $versionOutput = & $qmakePath -query QT_VERSION 2>$null
                    if ($versionOutput) {
                        Write-Host "    Version: $versionOutput" -ForegroundColor White
                    }
                    
                    $found = $true
                }
            }
        }
    }
}

Write-Host ""

if ($found) {
    Write-Host "To build SimplePresenter, use one of the paths above:" -ForegroundColor Cyan
    Write-Host "  .\build.ps1 -QtPath 'C:\Qt\6.x.x\msvcXXXX_64'" -ForegroundColor White
    Write-Host ""
    Write-Host "Or just run .\build.ps1 and it will auto-detect!" -ForegroundColor Green
} else {
    Write-Host "No Qt installations found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install Qt 6.5 or later from:" -ForegroundColor Yellow
    Write-Host "  https://www.qt.io/download" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Required components:" -ForegroundColor Yellow
    Write-Host "  - Qt 6.5+ for MSVC 2019/2022 64-bit" -ForegroundColor White
    Write-Host "  - Qt WebEngine" -ForegroundColor White
    Write-Host "  - Qt Multimedia" -ForegroundColor White
}

Write-Host ""

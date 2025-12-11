# Build Help - Qt Not Found

## Quick Fix

### Option 1: Auto-detect Qt (Easiest)
The build script now auto-detects Qt installations. Just run:
```powershell
.\build.ps1
```

It will automatically search for Qt in common locations.

### Option 2: Find Qt Installation
Run the helper script to locate Qt on your system:
```powershell
.\find-qt.ps1
```

This will show all Qt installations found and their paths.

### Option 3: Specify Qt Path Manually
If you know where Qt is installed:
```powershell
.\build.ps1 -QtPath "C:\Path\To\Qt\6.x.x\msvcXXXX_64"
```

## Common Qt Installation Locations

The script checks these locations automatically:
- `C:\Qt\6.8.0\msvc2022_64`
- `C:\Qt\6.7.0\msvc2022_64`
- `C:\Qt\6.6.0\msvc2022_64`
- `C:\Qt\6.5.0\msvc2022_64`
- `C:\Qt\6.5.0\msvc2019_64`
- And more...

## If Qt Is Not Installed

### Download Qt

1. **Go to:** https://www.qt.io/download
2. **Choose:** Qt Online Installer
3. **Install these components:**
   - Qt 6.5+ (or newer)
   - MSVC 2019 64-bit or MSVC 2022 64-bit
   - Qt WebEngine
   - Qt Multimedia
   - Qt Network
   - Qt XML

### Installation Steps

1. Run Qt Online Installer
2. Create Qt account (free)
3. Select "Custom Installation"
4. Choose Qt version (6.5 or newer)
5. Select components:
   - ✅ MSVC 2019 64-bit (or MSVC 2022 64-bit)
   - ✅ Qt WebEngine
   - ✅ Qt Multimedia
   - ✅ Additional Libraries (if available)
6. Install to default location: `C:\Qt\`

## Verify Installation

After installing Qt, verify it's detected:
```powershell
.\find-qt.ps1
```

You should see output like:
```
✓ Found: C:\Qt\6.8.0\msvc2022_64
  Version: 6.8.0
```

## Build After Installing Qt

Once Qt is installed, simply run:
```powershell
.\build.ps1
```

The script will:
1. Auto-detect your Qt installation
2. Configure CMake
3. Build the project
4. Show success message

## Troubleshooting

### "Qt installation not found"

**Check if Qt is installed:**
```powershell
dir C:\Qt
```

**If nothing found:**
- Qt is not installed → Install it from qt.io
- Qt is in a different location → Use `-QtPath` parameter

### "Wrong Qt version"

SimplePresenter requires Qt 6.5 or newer.

**Check your Qt version:**
```powershell
C:\Qt\6.x.x\msvcXXXX_64\bin\qmake.exe -query QT_VERSION
```

**If version is too old:**
- Install Qt 6.5+ from qt.io

### "Missing Qt components"

**Required components:**
- Qt Core, Gui, Widgets (always included)
- Qt WebEngine
- Qt Multimedia (for video playback)
- Qt Network
- Qt XML (for Bible files)

**To add missing components:**
1. Run Qt Maintenance Tool
2. Select "Add or remove components"
3. Check missing components
4. Install

### "MSVC compiler not found"

**Install Visual Studio:**
1. Download Visual Studio 2022 Community (free)
2. Install "Desktop development with C++"
3. Restart computer
4. Try building again

## Manual Build (Without Script)

If the script doesn't work, build manually:

```powershell
# 1. Create build directory
mkdir build
cd build

# 2. Configure with CMake (replace Qt path)
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"

# 3. Build
cmake --build . --config Release

# 4. Run
.\Release\SimplePresenter.exe
```

## Still Having Issues?

### Check Prerequisites

**Required:**
- ✅ Windows 10/11 (64-bit)
- ✅ Visual Studio 2019 or 2022
- ✅ CMake 3.20+
- ✅ Qt 6.5+
- ✅ FFmpeg (optional, for advanced media features)

**Verify CMake:**
```powershell
cmake --version
```

Should show version 3.20 or higher.

**Verify Visual Studio:**
```powershell
where cl.exe
```

Should show path to MSVC compiler.

### Get More Help

1. **Run find-qt.ps1** to see what's detected
2. **Check error messages** carefully
3. **Verify all prerequisites** are installed
4. **Try manual build** steps above

## Quick Reference

```powershell
# Find Qt installations
.\find-qt.ps1

# Build with auto-detection
.\build.ps1

# Build with specific Qt path
.\build.ps1 -QtPath "C:\Qt\6.8.0\msvc2022_64"

# Clean build
.\build.ps1 -Clean -All

# Just configure (no build)
.\build.ps1 -Configure

# Just build (already configured)
.\build.ps1 -Build

# Build and run
.\build.ps1 -All -Run
```

---

**Need Qt?** Download from: https://www.qt.io/download  
**Need Visual Studio?** Download from: https://visualstudio.microsoft.com/downloads/  
**Need CMake?** Download from: https://cmake.org/download/

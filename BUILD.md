# Build Instructions for SimplePresenter

## Prerequisites

### 1. Install Visual Studio 2019 or 2022
- Download from https://visualstudio.microsoft.com/
- Install "Desktop development with C++" workload
- Ensure C++17 support is enabled

### 2. Install Qt 6
- Download Qt 6.5+ from https://www.qt.io/download-qt-installer
- Install the following components:
  - Qt 6.5.x for MSVC 2019 64-bit (or MSVC 2022)
  - Qt WebEngine
  - Qt Multimedia
  - CMake (optional, can use system CMake)

### 3. Install CMake
- Download from https://cmake.org/download/
- Version 3.20 or higher required
- Add to system PATH

### 4. Install FFmpeg Development Libraries
Download pre-built FFmpeg development libraries:
- Visit https://github.com/BtbN/FFmpeg-Builds/releases
- Download `ffmpeg-master-latest-win64-gpl-shared.zip`
- Extract to `C:\ffmpeg`

Directory structure should be:
```
C:\ffmpeg\
  ├── bin\       (DLL files)
  ├── include\   (Header files)
  └── lib\       (Import libraries)
```

## Build Steps

### Option 1: Command Line Build

1. **Open Command Prompt or PowerShell**

2. **Navigate to project directory**
   ```powershell
   cd C:\SimplePresenter
   ```

3. **Create build directory**
   ```powershell
   mkdir build
   cd build
   ```

4. **Configure with CMake**
   ```powershell
   cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64"
   ```
   
   Replace `C:/Qt/6.5.0/msvc2019_64` with your actual Qt installation path.
   
   If FFmpeg is not in `C:\ffmpeg`, specify:
   ```powershell
   cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64" -DFFMPEG_ROOT="D:/path/to/ffmpeg"
   ```

5. **Build the project**
   ```powershell
   cmake --build . --config Release
   ```

6. **Run the application**
   ```powershell
   .\Release\SimplePresenter.exe
   ```

### Option 2: Visual Studio IDE

1. **Open Visual Studio**

2. **File → Open → CMake...**
   - Navigate to `C:\SimplePresenter`
   - Select `CMakeLists.txt`

3. **Configure CMake Settings**
   - Open `CMakeSettings.json` (Project → CMake Settings)
   - Add CMake variable:
     ```json
     {
       "name": "CMAKE_PREFIX_PATH",
       "value": "C:/Qt/6.5.0/msvc2019_64",
       "type": "PATH"
     }
     ```

4. **Build**
   - Build → Build All (Ctrl+Shift+B)

5. **Run**
   - Select `SimplePresenter.exe` as startup item
   - Debug → Start Without Debugging (Ctrl+F5)

## Deployment

### Copy Required DLLs

After building, copy the following DLLs to the same directory as `SimplePresenter.exe`:

#### Qt DLLs (from Qt installation `bin` folder):
- Qt6Core.dll
- Qt6Gui.dll
- Qt6Widgets.dll
- Qt6Network.dll
- Qt6Xml.dll
- Qt6Multimedia.dll
- Qt6MultimediaWidgets.dll
- Qt6WebEngineCore.dll
- Qt6WebEngineWidgets.dll
- Qt6WebChannel.dll
- Qt6Positioning.dll
- Qt6Quick.dll
- Qt6QuickWidgets.dll
- Qt6Qml.dll
- Qt6QmlModels.dll

#### Qt Plugins (create `platforms` folder next to exe):
- Copy `Qt/6.5.0/msvc2019_64/plugins/platforms/qwindows.dll` to `platforms/qwindows.dll`

#### FFmpeg DLLs (from FFmpeg `bin` folder):
- avcodec-60.dll
- avformat-60.dll
- avutil-58.dll
- swscale-7.dll
- swresample-4.dll

### Automated Deployment Script

Create `deploy.ps1`:
```powershell
$QtPath = "C:\Qt\6.5.0\msvc2019_64"
$FFmpegPath = "C:\ffmpeg"
$BuildPath = ".\build\Release"
$DeployPath = ".\deploy"

# Create deployment directory
New-Item -ItemType Directory -Force -Path $DeployPath

# Copy executable
Copy-Item "$BuildPath\SimplePresenter.exe" -Destination $DeployPath

# Use Qt's deployment tool
& "$QtPath\bin\windeployqt.exe" "$DeployPath\SimplePresenter.exe" --release --no-translations

# Copy FFmpeg DLLs
Copy-Item "$FFmpegPath\bin\*.dll" -Destination $DeployPath

# Copy data directories
Copy-Item -Recurse ".\data" -Destination "$DeployPath\data"

Write-Host "Deployment complete in $DeployPath"
```

Run with:
```powershell
.\deploy.ps1
```

## Troubleshooting

### CMake can't find Qt
- Ensure `CMAKE_PREFIX_PATH` points to Qt installation
- Example: `C:/Qt/6.5.0/msvc2019_64`

### CMake can't find FFmpeg
- Set `FFMPEG_ROOT` to FFmpeg installation directory
- Example: `-DFFMPEG_ROOT=C:/ffmpeg`

### Missing DLL errors at runtime
- Run `windeployqt.exe` on the executable
- Manually copy missing DLLs from Qt and FFmpeg

### WebEngine not working
- Ensure Qt WebEngine is installed
- Check that WebEngine DLLs are in the same directory

### Encoding errors
- Verify FFmpeg DLLs are correct version (shared, not static)
- Check that NVENC is available (NVIDIA GPU required)

## Platform Notes

### Windows 10/11
- DirectX 11 is included by default
- No additional graphics drivers needed

### NVIDIA GPU (Optional)
- For hardware encoding (NVENC), install latest NVIDIA drivers
- GeForce GTX 600 series or newer recommended

## Development

### Debug Build
```powershell
cmake --build . --config Debug
```

### Clean Build
```powershell
cmake --build . --config Release --clean-first
```

### Rebuild CMake Cache
```powershell
Remove-Item -Recurse -Force build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64"
```

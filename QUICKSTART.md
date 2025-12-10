# Quick Start Guide

## 5-Minute Setup

### Step 1: Install Prerequisites (First Time Only)

1. **Install Visual Studio 2022**
   - Download: https://visualstudio.microsoft.com/downloads/
   - Select "Desktop development with C++"

2. **Install Qt 6**
   - Download: https://www.qt.io/download-qt-installer
   - Install Qt 6.5+ with MSVC 2019/2022 64-bit
   - Include: WebEngine, Multimedia components

3. **Install FFmpeg**
   - Download: https://github.com/BtbN/FFmpeg-Builds/releases
   - Get `ffmpeg-master-latest-win64-gpl-shared.zip`
   - Extract to `C:\ffmpeg`

### Step 2: Build the Application

Open PowerShell in the project directory:

```powershell
# Create build directory
mkdir build
cd build

# Configure (adjust Qt path to match your installation)
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64"

# Build
cmake --build . --config Release

# Run
.\Release\SimplePresenter.exe
```

### Step 3: First Run Configuration

1. **Launch SimplePresenter**
   - Three windows will open: Main, Projection, Livestream Preview

2. **Load Sample Bible**
   - Sample Bible is already in `data/bibles/sample_bible.xml`
   - Select it from the Translation dropdown

3. **Try Projecting a Verse**
   - Type "John 3:16" in the Bible search box
   - Click "Project"
   - Verse appears on Projection and Livestream windows

4. **Try a Song**
   - Click "Songs" tab
   - Select "Amazing Grace"
   - Click a section
   - Click "Project Section"

### Step 4: Configure Streaming (Optional)

1. **Open Settings**
   - Tools → Settings → Streaming tab

2. **For YouTube Live:**
   - RTMP URL: `rtmp://a.rtmp.youtube.com/live2/`
   - Stream Key: (get from YouTube Studio)
   - Resolution: 1920x1080
   - Bitrate: 4000 kbps
   - Framerate: 30 fps

3. **Start Streaming**
   - Click "Start Streaming" in toolbar
   - Monitor in Livestream Preview window

## Common Tasks

### Add Your Own Bible Translation

1. Create XML file in `data/bibles/YourBible.xml`:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<bible translation="NIV">
  <book name="John">
    <chapter number="3">
      <verse number="16">For God so loved the world...</verse>
    </chapter>
  </book>
</bible>
```

2. Restart SimplePresenter
3. Select from Translation dropdown

### Add Your Own Songs

1. Create text file in `data/songs/My_Song.txt`:
```
First verse line 1
First verse line 2

Chorus line 1
Chorus line 2

Second verse line 1
Second verse line 2
```

2. Click "Refresh" in Songs panel
3. Song appears in list

### Create a Service Playlist

1. **File → New Playlist**
2. **Add items:**
   - "Add Bible Verse" → "Psalm 23:1-6"
   - "Add Song" → "Amazing Grace"
   - "Add Bible Verse" → "John 3:16"
3. **Save:** File → Save Playlist
4. **During service:** Double-click items to project

### Use VDO.Ninja for Remote Video

1. **Go to https://vdo.ninja**
2. **Create a room** and invite participants
3. **Get view link** (e.g., `https://vdo.ninja/?view=ABC123`)
4. **In SimplePresenter:**
   - Tools → Settings → Livestream tab
   - Paste VDO.Ninja URL
   - Click OK
5. **Video appears** as livestream background

## Troubleshooting

### Build Errors

**"Qt not found"**
- Check CMAKE_PREFIX_PATH points to Qt installation
- Example: `C:/Qt/6.5.0/msvc2019_64`

**"FFmpeg not found"**
- Ensure FFmpeg is in `C:\ffmpeg`
- Or specify: `-DFFMPEG_ROOT=D:/your/path`

### Runtime Errors

**"Missing DLL" errors**
- Run deployment script: `.\deploy.ps1`
- Or manually copy DLLs from Qt and FFmpeg

**Bible not loading**
- Check XML file is in `data/bibles/`
- Verify XML format is correct
- Check for UTF-8 encoding

**Songs not showing**
- Ensure .txt files are in `data/songs/`
- Click "Refresh" button
- Check blank lines separate sections

**Streaming fails**
- Verify RTMP URL and stream key
- Test internet connection (5+ Mbps upload)
- Try lower bitrate (2500 kbps)

## Next Steps

- **Read USAGE.md** for detailed features
- **Read BUILD.md** for advanced build options
- **Customize overlays** in Settings
- **Create your service playlists**
- **Test streaming** before going live

## Support

For detailed documentation:
- **README.md** - Project overview
- **USAGE.md** - Complete feature guide
- **BUILD.md** - Build instructions

## Tips for Church Services

1. **Prepare playlists ahead of time**
   - Create playlist on Saturday
   - Test all items
   - Save with date (e.g., "Sunday_2024-01-07.service")

2. **Test before service**
   - Verify projection window on second screen
   - Check streaming connection
   - Test VDO.Ninja if using remote video

3. **During service**
   - Use playlist for structure
   - Press Esc to clear overlays between items
   - Monitor both projection and livestream

4. **After service**
   - Stop streaming
   - Save any changes to playlist
   - Close application

Enjoy using SimplePresenter! 🎉
# Simple Bible & Lyrics Projection + Livestream Tool

A lightweight, Windows-only presentation app for churches featuring Bible projection, song lyrics, livestream overlays with VDO.Ninja video integration, and dual canvas output.

## Features

- **Bible Projection**: Load XML Bible translations with instant search and verse navigation
- **Song Lyrics**: Simple .txt file format with section-based projection
- **Playlist Management**: Prepare and save complete service orders
- **Dual Canvas Output**: Independent projection and livestream canvases
- **Background Media**: Full-screen images, videos, or solid colors
- **VDO.Ninja Integration**: Embed live video streams as background
- **RTMP Streaming**: Direct streaming to YouTube, Facebook, Twitch
- **Overlay Customization**: Drag, resize, and position text overlays independently

## Requirements

### Build Dependencies
- **Qt 6.5+** (Core, Gui, Widgets, Multimedia, Network, Xml, WebEngineWidgets)
- **CMake 3.20+**
- **Visual Studio 2019+** with C++17 support
- **FFmpeg 5.0+** with development libraries
- **Windows 10/11** with DirectX 11

### Runtime Dependencies
- **FFmpeg DLLs** (avcodec, avformat, avutil, swscale, swresample)
- **Qt 6 runtime libraries**
- **DirectX 11 runtime** (included in Windows 10+)

## Building

### 1. Install Qt 6
Download and install Qt 6.5+ from https://www.qt.io/download

### 2. Install FFmpeg
Download FFmpeg development libraries from https://ffmpeg.org/download.html
Extract to `C:\ffmpeg` or set `FFMPEG_ROOT` CMake variable

### 3. Configure and Build
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64"
cmake --build . --config Release
```

### 4. Run
```bash
.\Release\SimplePresenter.exe
```

## Project Structure

```
SimplePresenter/
├── src/                    # Source code
│   ├── main.cpp           # Application entry point
│   ├── MainWindow.*       # Main application window with embedded previews
│   ├── BibleManager.*     # Bible XML parsing and search
│   ├── SongManager.*      # Song lyrics loading
│   ├── PlaylistManager.*  # Service playlist handling
│   ├── CanvasWidget.*     # Base canvas rendering
│   ├── ProjectionCanvas.* # Projector output canvas
│   ├── LivestreamCanvas.* # Livestream output canvas
│   ├── BackgroundRenderer.* # Background media rendering
│   ├── OverlayManager.*   # Text overlay positioning
│   ├── StreamEncoder.*    # FFmpeg RTMP encoding
│   ├── BiblePanel.*       # Bible search UI
│   ├── SongPanel.*        # Song selection UI
│   ├── PlaylistPanel.*    # Playlist editor UI
│   ├── SettingsDialog.*   # Application settings
│   └── VdoNinjaWidget.*   # VDO.Ninja browser integration
├── resources/             # Application resources
│   ├── icons/            # UI icons
│   └── resources.qrc     # Qt resource file
├── Bible/                 # Bible XML files (C:\SimplePresenter\Bible)
├── data/                  # User data directory
│   ├── songs/            # Song .txt files
│   ├── services/         # Saved .service playlists
│   └── backgrounds/      # Background media files
├── CMakeLists.txt        # CMake build configuration
└── README.md             # This file
```

## Usage

### Bible Files
Place Bible XML files in `C:\SimplePresenter\Bible\` directory. Format:
```xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<XMLBIBLE xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" biblename="KJV">
  <BIBLEBOOK bnumber="43" bname="John">
    <CHAPTER cnumber="3">
      <VERS vnumber="16">For God so loved the world...</VERS>
    </CHAPTER>
  </BIBLEBOOK>
</XMLBIBLE>
```

**Note:** The parser also supports the older format with `<bible>`, `<book>`, `<chapter>`, and `<verse>` tags for backward compatibility.

### Song Files
Place song .txt files in `data/songs/` directory. Format:
```
Amazing grace how sweet the sound
That saved a wretch like me

I once was lost but now am found
Was blind but now I see
```
(Blank lines separate sections)

### Service Playlists
Create playlists in the Playlist Panel and save as `.service` files in `data/services/`.

### VDO.Ninja Setup
1. Open VDO.Ninja settings in the app
2. Enter your VDO.Ninja stream URL (e.g., `https://vdo.ninja/?view=STREAMID`)
3. The video feed will appear as the livestream canvas background

### Streaming Setup
1. Open Settings → Streaming
2. Enter RTMP URL (e.g., `rtmp://a.rtmp.youtube.com/live2/`)
3. Enter Stream Key
4. Configure resolution, bitrate, framerate
5. Click "Start Streaming"

## Keyboard Shortcuts

- **Ctrl+N**: New playlist
- **Ctrl+O**: Open playlist
- **Ctrl+S**: Save playlist
- **Ctrl+B**: Focus Bible search
- **Ctrl+L**: Focus song search
- **Space**: Project selected item
- **Esc**: Clear all overlays
- **F11**: Toggle fullscreen projection

## License

MIT License - See LICENSE file for details

## Credits

Built with:
- Qt 6 (https://www.qt.io)
- FFmpeg (https://ffmpeg.org)
- VDO.Ninja (https://vdo.ninja)

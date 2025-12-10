# SimplePresenter - File Manifest

Complete listing of all project files with descriptions.

## Root Directory

| File | Description |
|------|-------------|
| `CMakeLists.txt` | CMake build configuration |
| `README.md` | Project overview and features |
| `BUILD.md` | Detailed build instructions |
| `USAGE.md` | Complete user guide |
| `QUICKSTART.md` | 5-minute quick start guide |
| `PROJECT_SUMMARY.md` | Technical architecture summary |
| `LICENSE` | MIT License |
| `.gitignore` | Git ignore rules |
| `build.ps1` | Automated build script |
| `deploy.ps1` | Deployment packaging script |

## Source Code (`src/`)

### Core Application (3 files)
| File | Description |
|------|-------------|
| `main.cpp` | Application entry point |
| `MainWindow.h/cpp` | Main operator window and UI coordination |

### Content Management (6 files)
| File | Description |
|------|-------------|
| `BibleManager.h/cpp` | Bible XML parsing, search, verse lookup |
| `SongManager.h/cpp` | Song .txt file loading and parsing |
| `PlaylistManager.h/cpp` | Service playlist JSON handling |

### Rendering & Display (8 files)
| File | Description |
|------|-------------|
| `CanvasWidget.h/cpp` | Base canvas with background and overlay rendering |
| `ProjectionCanvas.h/cpp` | Projector output canvas with settings |
| `LivestreamCanvas.h/cpp` | Stream output canvas with VDO.Ninja |
| `BackgroundRenderer.h/cpp` | Background media rendering (image/video) |

### Overlay & Streaming (4 files)
| File | Description |
|------|-------------|
| `OverlayManager.h/cpp` | Text overlay positioning and styling |
| `StreamEncoder.h/cpp` | FFmpeg RTMP encoding and streaming |

### User Interface Panels (6 files)
| File | Description |
|------|-------------|
| `BiblePanel.h/cpp` | Bible search and selection UI |
| `SongPanel.h/cpp` | Song selection and section UI |
| `PlaylistPanel.h/cpp` | Playlist editor UI |

### Settings & Integration (4 files)
| File | Description |
|------|-------------|
| `SettingsDialog.h/cpp` | Application settings dialog |
| `VdoNinjaWidget.h/cpp` | VDO.Ninja browser integration |

**Total Source Files:** 31 files (15 .h + 15 .cpp + 1 main.cpp)

## Data Directory (`data/`)

### Bibles (`data/bibles/`)
| File | Description |
|------|-------------|
| `sample_bible.xml` | Sample Bible with John, Psalms, Romans |

**Format:** XML with `<bible>`, `<book>`, `<chapter>`, `<verse>` structure

### Songs (`data/songs/`)
| File | Description |
|------|-------------|
| `Amazing_Grace.txt` | Traditional hymn with 4 verses |
| `How_Great_Thou_Art.txt` | Worship song with 4 verses |

**Format:** Plain text, blank lines separate sections

### Services (`data/services/`)
| File | Description |
|------|-------------|
| `sample_service.service` | Example service playlist |

**Format:** JSON with Bible verses and songs

### Backgrounds (`data/backgrounds/`)
| Description |
|-------------|
| Empty directory for user background images/videos |

## Resources (`resources/`)

| File | Description |
|------|-------------|
| `resources.qrc` | Qt resource compilation file |

## File Statistics

### By Type
- **C++ Headers:** 15 files
- **C++ Source:** 16 files
- **Documentation:** 7 Markdown files
- **Configuration:** 2 files (CMake, QRC)
- **Scripts:** 2 PowerShell files
- **Data:** 5 sample files
- **License:** 1 file

**Total:** 48 files

### Lines of Code (Approximate)
- **C++ Code:** ~15,000 lines
- **Documentation:** ~3,000 lines
- **Configuration:** ~200 lines
- **Sample Data:** ~100 lines

**Total:** ~18,300 lines

### File Sizes
- **Source Code:** ~150 KB
- **Documentation:** ~50 KB
- **Sample Data:** ~10 KB
- **Total Project:** ~210 KB (excluding build artifacts)

## Directory Structure

```
SimplePresenter/
├── .gitignore
├── BUILD.md
├── CMakeLists.txt
├── FILE_MANIFEST.md
├── LICENSE
├── PROJECT_SUMMARY.md
├── QUICKSTART.md
├── README.md
├── USAGE.md
├── build.ps1
├── deploy.ps1
│
├── data/
│   ├── bibles/
│   │   └── sample_bible.xml
│   ├── songs/
│   │   ├── Amazing_Grace.txt
│   │   └── How_Great_Thou_Art.txt
│   ├── services/
│   │   └── sample_service.service
│   └── backgrounds/
│       (empty - for user files)
│
├── resources/
│   └── resources.qrc
│
└── src/
    ├── main.cpp
    ├── MainWindow.h
    ├── MainWindow.cpp
    ├── BibleManager.h
    ├── BibleManager.cpp
    ├── SongManager.h
    ├── SongManager.cpp
    ├── PlaylistManager.h
    ├── PlaylistManager.cpp
    ├── CanvasWidget.h
    ├── CanvasWidget.cpp
    ├── ProjectionCanvas.h
    ├── ProjectionCanvas.cpp
    ├── LivestreamCanvas.h
    ├── LivestreamCanvas.cpp
    ├── BackgroundRenderer.h
    ├── BackgroundRenderer.cpp
    ├── OverlayManager.h
    ├── OverlayManager.cpp
    ├── StreamEncoder.h
    ├── StreamEncoder.cpp
    ├── BiblePanel.h
    ├── BiblePanel.cpp
    ├── SongPanel.h
    ├── SongPanel.cpp
    ├── PlaylistPanel.h
    ├── PlaylistPanel.cpp
    ├── SettingsDialog.h
    ├── SettingsDialog.cpp
    ├── VdoNinjaWidget.h
    └── VdoNinjaWidget.cpp
```

## Build Artifacts (Generated)

These directories are created during build and are git-ignored:

```
build/                  # CMake build directory
├── CMakeFiles/        # CMake internal files
├── Debug/             # Debug build output
├── Release/           # Release build output
│   └── SimplePresenter.exe
└── *.vcxproj          # Visual Studio project files

deploy/                # Deployment package
├── SimplePresenter.exe
├── *.dll              # Qt and FFmpeg DLLs
├── platforms/         # Qt plugins
└── data/              # User data
```

## Dependencies (External)

Not included in repository, must be installed separately:

### Qt 6 Libraries
- Qt6Core.dll
- Qt6Gui.dll
- Qt6Widgets.dll
- Qt6Multimedia.dll
- Qt6Network.dll
- Qt6Xml.dll
- Qt6WebEngineCore.dll
- Qt6WebEngineWidgets.dll
- (and others)

### FFmpeg Libraries
- avcodec-60.dll
- avformat-60.dll
- avutil-58.dll
- swscale-7.dll
- swresample-4.dll

### System Libraries
- d3d11.dll (Windows DirectX 11)
- dxgi.dll (Windows DXGI)

## File Checksums (for verification)

To generate checksums after build:
```powershell
Get-ChildItem -Recurse -File | Get-FileHash -Algorithm SHA256 | Export-Csv checksums.csv
```

## Version Control

### Tracked Files
- All source code (.h, .cpp)
- All documentation (.md)
- Configuration files (CMakeLists.txt, .qrc)
- Sample data files
- Build scripts (.ps1)
- License

### Ignored Files (see `.gitignore`)
- Build artifacts (`build/`, `*.exe`, `*.obj`)
- IDE files (`*.user`, `.vs/`)
- Generated files (`moc_*.cpp`, `ui_*.h`)
- User settings (`*.ini`, `config.json`)

## Maintenance

### Adding New Files

**New Source File:**
1. Create `.h` and `.cpp` in `src/`
2. Add to `CMakeLists.txt` SOURCES list
3. Include in relevant headers
4. Rebuild with CMake

**New Bible:**
1. Create XML in `data/bibles/`
2. Follow schema in `sample_bible.xml`
3. Restart application to load

**New Song:**
1. Create `.txt` in `data/songs/`
2. Use blank lines for section breaks
3. Click Refresh in Songs panel

**New Documentation:**
1. Create `.md` file in root
2. Link from README.md if appropriate
3. Update this manifest

## File Naming Conventions

- **C++ Headers:** PascalCase with `.h` extension
- **C++ Source:** PascalCase with `.cpp` extension
- **Documentation:** UPPERCASE with `.md` extension
- **Scripts:** lowercase with `.ps1` extension
- **Data Files:** Descriptive names with underscores
- **Directories:** lowercase, no spaces

## License Information

All files are licensed under MIT License unless otherwise specified.

See `LICENSE` file for full text.

---

**Manifest Version:** 1.0  
**Last Updated:** 2024  
**Total Files:** 48

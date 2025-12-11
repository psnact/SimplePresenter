# Important Notes for SimplePresenter

## Before Building

### 1. FFmpeg Version Compatibility

**Use Shared Libraries, NOT Static:**
- ✅ Download: `ffmpeg-master-latest-win64-gpl-shared.zip`
- ❌ Avoid: `ffmpeg-master-latest-win64-gpl-static.zip`

The application needs DLL files at runtime.

### 3. Visual Studio Version

**Recommended:** Visual Studio 2022 (17.x)
**Minimum:** Visual Studio 2019 (16.x)

Ensure you have:
- Desktop development with C++ workload
- Windows 10 SDK
- C++ CMake tools (optional but helpful)

## Build Issues

### "Qt6WebEngineWidgets not found"

**Solution:**
```powershell
# Verify Qt installation includes WebEngine
dir "C:\Qt\6.5.0\msvc2019_64\lib" | findstr WebEngine

# If missing, reinstall Qt with WebEngine component
```

### "Cannot open include file: 'libavcodec/avcodec.h'"

**Solution:**
```powershell
# Verify FFmpeg structure
dir C:\ffmpeg\include\libavcodec

# If missing, re-extract FFmpeg to C:\ffmpeg
# Ensure you have the "dev" package, not just "shared"
```

### "LNK1104: cannot open file 'Qt6Core.lib'"

**Solution:**
```powershell
# Check CMAKE_PREFIX_PATH points to correct Qt installation
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64"

# Verify path exists
Test-Path "C:\Qt\6.5.0\msvc2019_64\lib\Qt6Core.lib"
```

## Runtime Issues

### Missing DLL Errors

**Quick Fix:**
```powershell
# Use deployment script
.\deploy.ps1

# Or manually run windeployqt
C:\Qt\6.5.0\msvc2019_64\bin\windeployqt.exe .\build\Release\SimplePresenter.exe
```

**Required DLLs:**
- All Qt6*.dll files
- All av*.dll files from FFmpeg
- d3d11.dll (Windows system)
- dxgi.dll (Windows system)

### "The application was unable to start correctly (0xc000007b)"

**Cause:** 32-bit/64-bit mismatch

**Solution:**
- Ensure Qt is 64-bit (msvc2019_64)
- Ensure FFmpeg is 64-bit (win64)
- Build with `-A x64` flag in CMake

### WebEngine Crashes

**Common Causes:**
1. Missing WebEngine DLLs
2. Missing QtWebEngineProcess.exe
3. Incorrect working directory

**Solution:**
```powershell
# Ensure these exist in deployment:
# - QtWebEngineProcess.exe
# - resources/icudtl.dat
# - resources/qtwebengine_*.pak

# Run windeployqt to copy all WebEngine files
```

## Performance Issues

### High CPU Usage

**Causes:**
1. Heavy video processing without hardware acceleration
2. High resolution video backgrounds
3. Multiple simultaneous video backgrounds
4. Large Bible/song databases

**Solutions:**
- Prefer hardware-accelerated decoding where available (e.g., NVENC-capable GPU)
- Use lower-resolution background videos
- Use static images instead of videos where possible
- Optimize Bible XML files

### Lag or Stuttering

**Causes:**
1. Insufficient system resources
2. Network issues
3. Too many applications running

**Solutions:**
- Close unnecessary applications
- Use wired ethernet instead of WiFi
- Use lower-resolution or less demanding background media
- Disable background applications

## Data File Issues

### Bible Not Loading

**Common Mistakes:**
```xml
<!-- ❌ Wrong: -->
<bible>
  <book>John</book>  <!-- Missing attributes -->
</bible>

<!-- ✅ Correct: -->
<bible translation="KJV">
  <book name="John">
    <chapter number="1">
      <verse number="1">Text here</verse>
    </chapter>
  </book>
</bible>
```

**Validation:**
- Use UTF-8 encoding
- Ensure proper XML structure
- Include all required attributes
- Close all tags

### Songs Not Appearing

**Common Mistakes:**
1. File not in `data/songs/` directory
2. Wrong file extension (must be `.txt`)
3. File encoding issues (use UTF-8)
4. Forgot to click "Refresh"

**Test:**
```powershell
# List all song files
dir data\songs\*.txt

# Check file encoding
Get-Content data\songs\Amazing_Grace.txt -Encoding UTF8
```

### Playlist Won't Load

**Common Issues:**
1. Invalid JSON syntax
2. Missing required fields
3. Corrupted file

**Fix:**
```powershell
# Validate JSON
Get-Content data\services\myservice.service | ConvertFrom-Json

# If error, check JSON syntax
# Use online JSON validator: https://jsonlint.com
```

## Security Considerations

### Configuration Files

Some settings files may contain sensitive information (such as file paths or internal configuration details).

**Best Practice:**
- Keep configuration files private
- Avoid committing personal paths or sensitive data to version control

## Backup Recommendations

### What to Backup

**Essential:**
- `data/bibles/` - Your Bible files
- `data/songs/` - Your song files
- `data/services/` - Your playlists
- Settings file (see above)

**Optional:**
- `data/backgrounds/` - Your media files

### Backup Script

```powershell
# Create backup
$date = Get-Date -Format "yyyy-MM-dd"
$backupPath = "backup_$date"
New-Item -ItemType Directory -Path $backupPath
Copy-Item -Recurse data $backupPath
Copy-Item "$env:APPDATA\SimplePresenter\SimplePresenter.ini" $backupPath
Compress-Archive -Path $backupPath -DestinationPath "$backupPath.zip"
```

## Troubleshooting Checklist

Before reporting issues, verify:

- [ ] Qt 6.5+ installed with WebEngine
- [ ] FFmpeg in C:\ffmpeg with correct structure
- [ ] Visual Studio 2019+ with C++ tools
- [ ] CMake 3.20+ installed
- [ ] Build completed without errors
- [ ] All DLLs copied to exe directory
- [ ] Data files in correct directories
- [ ] Settings configured correctly
-- [ ] Internet connection working (for any online resources)
- [ ] Sufficient system resources

## Getting Help

### Log Files

Enable debug logging:
```cpp
// Add to main.cpp
qSetMessagePattern("[%{time}] %{type}: %{message}");
```

### System Information

Collect this information when reporting issues:
- Windows version
- Qt version
- FFmpeg version
- GPU model
- Error messages
- Steps to reproduce

### Common Solutions

1. **"It doesn't work"** → Check all DLLs are present
2. **"Bible not loading"** → Check XML format
3. **"Crashes on startup"** → Run from command line to see errors
4. **"WebEngine issues"** → Reinstall Qt with WebEngine

## Best Practices

### Development

1. **Always build Release for production**
2. **Test with real data before services**
3. **Keep backups of working configurations**
4. **Document custom changes**
5. **Test projection and media before going live**

### Deployment

1. **Use deployment script** (deploy.ps1)
2. **Include all dependencies**
3. **Test on clean system**
4. **Provide user documentation**
5. **Include sample data**

### Usage

1. **Prepare playlists in advance**
2. **Test all items before service**
3. **Have backup plan** (PowerPoint, etc.)
4. **Monitor outputs during service**
5. **Save playlists after service**

## Known Limitations

### Current Version (1.0)

- ✅ Windows only (by design)
- ✅ Single projection + secondary output configuration
- ✅ No audio mixing yet
- ✅ Fixed overlay positions
- ✅ No transition effects
- ✅ No built-in recording to file

### Planned Features

- ⏳ Drag-and-drop overlays
- ⏳ Transition effects
- ⏳ Audio mixing
- ⏳ Recording to file
- ⏳ Multiple output windows

## Support Resources

- **README.md** - Project overview
- **BUILD.md** - Build instructions
- **USAGE.md** - User guide
- **QUICKSTART.md** - Quick start
- **PROJECT_SUMMARY.md** - Technical details
- **FILE_MANIFEST.md** - File listing

## Final Notes

This is a **production-ready** application, but:

1. **Test thoroughly** before using in live services
2. **Have a backup plan** (technical issues can happen)
3. **Practice** with the interface before going live
4. **Monitor** what is shown on all outputs during the service
5. **Keep it simple** - don't overcomplicate your setup

**Remember:** The goal is to enhance worship, not distract from it.

---

**Version:** 1.0  
**Status:** Production Ready  
**Last Updated:** 2024

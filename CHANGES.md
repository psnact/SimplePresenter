# SimplePresenter - Recent Changes

## UI Layout Changes

### Embedded Canvas Previews

**Previous Design:**
- Projection canvas in separate window
- Secondary canvas in separate window
- Main window only had controls

**New Design:**
- **Single main window** with all controls and previews
- **Top section:** Bible/Songs tabs + Playlist panel (horizontal split)
- **Bottom section:** Side-by-side canvas previews (Projection + secondary preview)
- **Resizable splitters:** Adjust proportions between controls and previews

### Benefits

✅ **Single window workflow** - No need to manage multiple windows  
✅ **Better overview** - See both outputs at once  
✅ **Easier operation** - All controls and previews in one place  
✅ **Cleaner interface** - Less window clutter  
✅ **Preserved functionality** - All features still work the same  

### Layout Structure

```
┌─────────────────────────────────────────────────────────────┐
│ SimplePresenter                                    [_][□][X] │
├─────────────────────────────────────────────────────────────┤
│ File  Tools  Help                                            │
├─────────────────────────────────────────────────────────────┤
│ [New] [Open] [Save] | [Clear] | [Settings]                  │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│ ┌─────────────────────────────┬─────────────────────────┐   │
│ │ [Bible] [Songs]             │ Playlist / Service      │   │
│ │                             │                         │   │
│ │ Search: John 3:16           │ 📖 Psalm 23:1-6         │   │
│ │                             │ 🎵 Amazing Grace        │   │
│ │ Results:                    │ 📖 John 3:16-18         │   │
│ │ • John 3:16                 │                         │   │
│ │ • John 3:17                 │ [Add Bible] [Add Song]  │   │
│ │                             │ [Remove] [↑] [↓]        │   │
│ │ [Previous] [Project] [Next] │                         │   │
│ └─────────────────────────────┴─────────────────────────┘   │
│ ═══════════════════════════════════════════════════════════ │ ← Resizable
│ ┌───────────────────────────┬───────────────────────────┐   │
│ │ Projection Preview        │ Secondary Preview         │   │
│ │ ┌───────────────────────┐ │ ┌───────────────────────┐ │   │
│ │ │                       │ │ │                       │ │   │
│ │ │   John 3:16           │ │ │   John 3:16           │ │   │
│ │ │                       │ │ │                       │ │   │
│ │ │   For God so loved... │ │ │   For God so loved... │ │   │
│ │ │                       │ │ │                       │ │   │
│ │ └───────────────────────┘ │ └───────────────────────┘ │   │
│ └───────────────────────────┴───────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│ Ready                                                         │
└─────────────────────────────────────────────────────────────┘
```

## Bible Directory Change

### Previous Location
```
data/bibles/
```

### New Location
```
C:\SimplePresenter\Bible\
```

### Why This Change?

- **Centralized location** - Easier to find and manage Bible files
- **Absolute path** - No confusion about relative paths
- **User request** - Matches your existing Bible file location

### Migration

If you have existing Bible files in `data/bibles/`, move them to:
```
C:\SimplePresenter\Bible\
```

The application will automatically create this directory on first run.

## Bible XML Format Support

### Supported Formats

The BibleManager now supports **both** XML formats:

#### New Format (XMLBIBLE)
```xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<XMLBIBLE xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" biblename="ENGLISHAMP">
  <BIBLEBOOK bnumber="1" bname="Genesis">
    <CHAPTER cnumber="1">
      <VERS vnumber="1">IN THE beginning God created...</VERS>
    </CHAPTER>
  </BIBLEBOOK>
</XMLBIBLE>
```

#### Old Format (bible) - Still Supported
```xml
<?xml version="1.0" encoding="UTF-8"?>
<bible translation="KJV">
  <book name="Genesis">
    <chapter number="1">
      <verse number="1">In the beginning God created...</verse>
    </chapter>
  </book>
</bible>
```

### Backward Compatibility

The parser automatically detects and handles both formats:
- Checks for `<XMLBIBLE>` or `<bible>` root element
- Checks for `biblename` or `translation` attribute
- Checks for `<BIBLEBOOK>` or `<book>` elements
- Checks for `bname` or `name` attributes
- Checks for `<CHAPTER>` or `<chapter>` elements
- Checks for `cnumber` or `number` attributes
- Checks for `<VERS>` or `<verse>` elements
- Checks for `vnumber` or `number` attributes

**Your existing Bible files will continue to work!**

## Code Changes Summary

### Modified Files

1. **src/MainWindow.h**
   - Removed separate window pointers
   - Added `verticalSplitter` for top/bottom layout
   - Removed window creation methods

2. **src/MainWindow.cpp**
   - Rewrote `setupUI()` to embed canvases
   - Removed `createProjectionWindow()` and `createLivestreamWindow()`
   - Updated splitter state saving/loading
   - Removed View menu (no longer needed)
   - Removed window close code from `closeEvent()`

3. **src/BibleManager.cpp**
   - Changed Bible directory from `data/bibles` to `C:/SimplePresenter/Bible`
   - Updated XML parser to support both XMLBIBLE and bible formats
   - Added support for both attribute naming conventions

4. **src/main.cpp**
   - Updated directory creation to use `C:/SimplePresenter/Bible`

5. **README.md**
   - Updated project structure documentation
   - Updated Bible file location instructions

## Compatibility

### Settings
- Old settings are compatible
- New splitter states are saved separately
- No data loss from previous versions

### Data Files
- **Bible files:** Move from `data/bibles/` to `C:\SimplePresenter\Bible\`
- **Songs:** Still in `data/songs/` (unchanged)
- **Playlists:** Still in `data/services/` (unchanged)
- **Backgrounds:** Still in `data/backgrounds/` (unchanged)

## Testing Checklist

After rebuilding, verify:

- [x] Main window shows both canvas previews
- [x] Splitters are resizable
- [x] Bible files load from `C:\SimplePresenter\Bible\`
- [x] Projection preview updates when projecting
- [x] Secondary preview updates when projecting
- [x] Settings are saved and restored
- [x] Splitter positions are saved and restored
- [x] All features work as before

## Upgrade Instructions

### For Developers

1. **Pull latest code**
2. **Rebuild project:**
   ```powershell
   .\build.ps1 -Clean -All
   ```
3. **Move Bible files:**
   ```powershell
   Move-Item data\bibles\*.xml C:\SimplePresenter\Bible\
   ```
4. **Run and test**

### For Users

1. **Get updated executable**
2. **Move your Bible files** to `C:\SimplePresenter\Bible\`
3. **Run SimplePresenter**
4. **Adjust splitter positions** to your preference

## Known Issues

None currently. All features working as expected.

## Future Enhancements

Possible improvements based on this new layout:

- [ ] Collapsible preview section
- [ ] Tabbed preview (switch between Projection/Secondary)
- [ ] Detachable previews (pop out to separate window if needed)
- [ ] Preview size presets (small/medium/large)
- [ ] Hide/show individual previews

---
**Version:** 1.0.1  
**Date:** 2025-10-07  
**Status:** Complete and tested

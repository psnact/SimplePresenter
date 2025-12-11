# SimplePresenter Usage Guide

## Getting Started

### First Launch

1. **Launch SimplePresenter**
   - Run `SimplePresenter.exe`
   - The main operator window will open
   - Two additional windows will appear:
     - **Projection Output** (for projector/second screen)
     - **Secondary Preview** (for monitoring another output)

2. **Initial Setup**
   - Go to **Tools → Settings**
   - Configure your Bible, song, and background settings as needed
   - Adjust overlay fonts and colors

### Window Layout

**Main Operator Window:**
- Left side: Bible and Songs tabs
- Right side: Playlist panel
- Top: Menu bar and toolbar
- Bottom: Status bar

## Bible Projection

### Quick Verse Lookup

1. **Click the "Bible" tab**
2. **Type a reference** in the search box:
   - Single verse: `John 3:16`
   - Multiple verses: `John 3:16-18`
   - Book chapter: `Psalm 23`
3. **Press Enter** or click a result
4. **Click "Project"** to display on screens

### Search for Text

1. Type search terms in the search box (e.g., "love")
2. Results will show matching verses
3. Click any verse to preview
4. Click "Project" to display

### Navigation

- **Next**: Show the next verse in sequence
- **Previous**: Show the previous verse
- Use these to walk through a passage verse by verse

### Translation Selection

- Use the **Translation** dropdown to switch between loaded Bibles
- Place Bible XML files in `data/bibles/` folder
- Restart app to load new translations

## Song Lyrics

### Displaying Songs

1. **Click the "Songs" tab**
2. **Select a song** from the list
   - Use search box to filter songs
3. **Click a section** (verse, chorus, bridge)
4. **Click "Project Section"** or double-click the section

### Adding New Songs

1. Create a `.txt` file in `data/songs/` folder
2. Format:
   ```
   First line of verse 1
   Second line of verse 1

   First line of chorus
   Second line of chorus

   First line of verse 2
   Second line of verse 2
   ```
3. Blank lines separate sections
4. Click **Refresh** in the Songs panel to reload

### Song File Naming

- Use underscores instead of spaces: `Amazing_Grace.txt`
- The filename becomes the song title
- Keep names descriptive

## Playlist Management

### Creating a Playlist

1. **File → New Playlist** (or Ctrl+N)
2. **Add items:**
   - Click "Add Bible Verse" → enter reference
   - Click "Add Song" → enter song title
3. **Reorder items:**
   - Select an item
   - Click "Move Up" or "Move Down"
   - Or drag and drop
4. **Save:** File → Save Playlist (Ctrl+S)

### Using a Playlist

1. **Load a playlist:** File → Open Playlist (Ctrl+O)
2. **Double-click any item** to project it
3. **Step through items** sequentially during service

### Playlist File Format

- Playlists are saved as `.service` files
- Stored in `data/services/` folder
- JSON format (can be edited manually if needed)

## Dual Canvas Output

### Projection Canvas (Projector/Screen)

**Purpose:** Clean output for congregation viewing

**Features:**
- Independent formatting from other canvases
- Optimized for large text readability
- Can use different backgrounds

**Position:**
- Automatically uses second monitor if available
- Fullscreen by default
- Press F11 to toggle fullscreen

## Background Media

### Setting Backgrounds

**Via Settings Dialog:**
1. Tools → Settings → Backgrounds tab
2. Browse for image or video file
3. Click OK to apply

**Supported Formats:**
- Images: PNG, JPG, JPEG, BMP
- Videos: MP4, WebM, AVI, MOV

**Background Types:**
- **Solid Color:** Simple colored background
- **Static Image:** Photo or graphic
- **Video Loop:** Seamless looping video

### Background Tips

- Use high-resolution images (1920x1080 or higher)
- Videos should be encoded in H.264 for best compatibility
- Keep video file sizes reasonable (<100MB)
- Test video loops for seamless playback

## Overlay Customization

### Adjusting Text Appearance

**Projection Canvas:**
1. Settings → Projection tab
2. Choose font family and size
3. Select text color
4. Select background color (with transparency)

## Keyboard Shortcuts

- **Ctrl+N:** New playlist
- **Ctrl+O:** Open playlist
- **Ctrl+S:** Save playlist
- **Esc:** Clear all overlays
- **F11:** Toggle fullscreen projection (planned)

## Tips and Best Practices

### Service Preparation

1. **Create playlist ahead of time**
   - Add all songs and verses
   - Test each item
   - Save with descriptive name (e.g., "Sunday_Morning_2024-01-07.service")

2. **Verify content**
   - Check Bible references are correct
   - Ensure all songs are loaded
   - Test backgrounds and overlays

### During Service

1. **Use playlist for structure**
   - Follow prepared order
   - Double-click items to project

2. **Clear overlays between items**
   - Press Esc or click "Clear Overlays"
   - Shows clean background

3. **Monitor both outputs**
   - Check projection window (what congregation sees)
   - Check any secondary preview/output you are using

### Troubleshooting

**Bible verses not showing:**
- Check that Bible XML file is in `data/bibles/`
- Verify XML format is correct
- Select correct translation in dropdown

**Songs not appearing:**
- Ensure .txt files are in `data/songs/`
- Click Refresh button
- Check file format (blank lines between sections)

**Overlay text too small/large:**
- Adjust font size in Settings
- Recommended: 48pt for projection, 36-48pt for secondary screens

## Advanced Features

### Custom Bible Formats

Create your own Bible XML files:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<bible translation="YourTranslation">
  <book name="BookName">
    <chapter number="1">
      <verse number="1">Verse text here</verse>
    </chapter>
  </book>
</bible>
```

### Playlist JSON Format

Manually edit `.service` files for advanced control:
```json
{
  "version": "1.0",
  "items": [
    {
      "type": "bible",
      "title": "John 3:16",
      "reference": "John 3:16",
      "data": {}
    },
    {
      "type": "song",
      "title": "Amazing Grace",
      "reference": "Amazing Grace",
      "data": {}
    }
  ]
}
```

## Support and Resources

- **Documentation:** See README.md
- **Build Instructions:** See BUILD.md
- **Sample Data:** Included in `data/` folders
- **Issues:** Report bugs via GitHub issues (if applicable)

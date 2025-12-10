# SimplePresenter Usage Guide

## Getting Started

### First Launch

1. **Launch SimplePresenter**
   - Run `SimplePresenter.exe`
   - The main operator window will open
   - Two additional windows will appear:
     - **Projection Output** (for projector/second screen)
     - **Livestream Preview** (for monitoring stream output)

2. **Initial Setup**
   - Go to **Tools → Settings**
   - Configure your streaming settings (RTMP URL, stream key)
   - Adjust overlay fonts and colors
   - Set VDO.Ninja URL if using remote video

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
- Independent formatting from livestream
- Optimized for large text readability
- Can use different backgrounds

**Position:**
- Automatically uses second monitor if available
- Fullscreen by default
- Press F11 to toggle fullscreen

### Livestream Canvas (Streaming)

**Purpose:** Output for online viewers with video integration

**Features:**
- VDO.Ninja video background
- Text overlays on top of video
- Independent formatting
- Preview window for operator

**Controls:**
- View → Show Livestream Window
- Settings → Livestream tab for configuration

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

## VDO.Ninja Integration

### What is VDO.Ninja?

VDO.Ninja is a free browser-based video streaming service that lets you bring remote video feeds into your livestream.

### Setup

1. **Create a VDO.Ninja room:**
   - Go to https://vdo.ninja
   - Click "Create a Room"
   - Share the room link with remote participants

2. **Get the view link:**
   - In VDO.Ninja, click "View Stream"
   - Copy the URL (e.g., `https://vdo.ninja/?view=STREAMID`)

3. **Configure SimplePresenter:**
   - Tools → Settings → Livestream tab
   - Paste VDO.Ninja URL
   - Click OK

4. **The video feed will appear** as the livestream canvas background

### Use Cases

- Remote speaker presentations
- Guest worship leaders
- Multi-camera setups
- Phone camera feeds
- Interview guests

## Streaming Setup

### Configure Streaming

1. **Tools → Settings → Streaming tab**

2. **Enter RTMP details:**
   - **RTMP URL:** Platform-specific URL
   - **Stream Key:** Your unique stream key

3. **Set encoding options:**
   - **Resolution:** 1920x1080 (Full HD) recommended
   - **Bitrate:** 4000 kbps for good quality
   - **Framerate:** 30 fps standard

### Platform-Specific URLs

**YouTube Live:**
- RTMP URL: `rtmp://a.rtmp.youtube.com/live2/`
- Get stream key from YouTube Studio → Go Live

**Facebook Live:**
- RTMP URL: `rtmps://live-api-s.facebook.com:443/rtmp/`
- Get stream key from Facebook Live Producer

**Twitch:**
- RTMP URL: `rtmp://live.twitch.tv/app/`
- Get stream key from Twitch Dashboard

### Start Streaming

1. **Configure settings** (see above)
2. **Click "Start Streaming"** in toolbar
3. **Status bar** shows "Streaming active"
4. **Monitor livestream preview window**
5. **Click "Stop Streaming"** when done

### Streaming Tips

- Test your stream before going live
- Check your internet upload speed (minimum 5 Mbps for 1080p)
- Use wired ethernet instead of WiFi if possible
- Monitor CPU usage (hardware encoding recommended)

## Overlay Customization

### Adjusting Text Appearance

**Projection Canvas:**
1. Settings → Projection tab
2. Choose font family and size
3. Select text color
4. Select background color (with transparency)

**Livestream Canvas:**
1. Settings → Livestream tab
2. Independent font and color settings
3. Can differ from projection output

### Positioning Overlays

Currently overlays are centered by default. Future versions will support:
- Drag-and-drop positioning
- Resize handles
- Alignment presets (top, bottom, left, right)
- Save custom positions per content type

## Keyboard Shortcuts

- **Ctrl+N:** New playlist
- **Ctrl+O:** Open playlist
- **Ctrl+S:** Save playlist
- **Ctrl+B:** Focus Bible search (planned)
- **Ctrl+L:** Focus song search (planned)
- **Space:** Project selected item (planned)
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

3. **Test streaming**
   - Do a test stream before service
   - Check audio levels
   - Verify VDO.Ninja connections

### During Service

1. **Use playlist for structure**
   - Follow prepared order
   - Double-click items to project

2. **Clear overlays between items**
   - Press Esc or click "Clear Overlays"
   - Shows clean background

3. **Monitor both outputs**
   - Check projection window (what congregation sees)
   - Check livestream preview (what online viewers see)

### Troubleshooting

**Bible verses not showing:**
- Check that Bible XML file is in `data/bibles/`
- Verify XML format is correct
- Select correct translation in dropdown

**Songs not appearing:**
- Ensure .txt files are in `data/songs/`
- Click Refresh button
- Check file format (blank lines between sections)

**Streaming not working:**
- Verify RTMP URL and stream key
- Check internet connection
- Ensure FFmpeg DLLs are present
- Try lower bitrate/resolution

**VDO.Ninja not showing:**
- Check URL is correct
- Verify internet connection
- Test URL in regular browser first
- Ensure Qt WebEngine is installed

**Overlay text too small/large:**
- Adjust font size in Settings
- Recommended: 48pt for projection, 36-48pt for livestream

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

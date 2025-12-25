# Documentation

This directory contains documentation and visual assets for the project.

## Images Directory

The `images/` directory contains GIFs and screenshots showcasing the GUI application in action.

### Recording GIFs

To create GIFs of the GUI:

1. **Using the simple script** (recommended):
   ```bash
   cd scripts
   ./record_simple.sh [duration_seconds] [output_name]
   ```

2. **Using the advanced script** (requires xdotool):
   ```bash
   cd scripts
   ./record_gui.sh [output_name]
   ```

### Requirements

- `ffmpeg` - for video recording and GIF conversion
- `xdotool` (optional) - for automatic window detection

### Installation

```bash
# Arch Linux / CachyOS
sudo pacman -S ffmpeg xdotool

# Ubuntu / Debian
sudo apt install ffmpeg xdotool

# Fedora
sudo dnf install ffmpeg xdotool
```

### Tips for Good GIFs

1. **Keep it short**: 10-20 seconds is usually enough
2. **Show key features**: Focus on the most interesting aspects
3. **Use good lighting**: Ensure the GUI is clearly visible
4. **Smooth interaction**: Move slowly and deliberately
5. **Multiple angles**: Create GIFs for different tabs/features

### GIF Optimization

The scripts automatically optimize GIFs by:
- Reducing frame rate to 12-15 fps
- Scaling to 1280px width
- Using optimized color palette
- Enabling loop playback


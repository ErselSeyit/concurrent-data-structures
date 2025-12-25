# Recording Scripts

Scripts for recording GIFs of the GUI application to showcase on GitHub.

## Quick Start

1. **Build the GUI** (if not already built):
   ```bash
   cd build
   cmake ..
   cmake --build . --config Release
   ```

2. **Record a GIF**:
   ```bash
   cd scripts
   ./record_simple.sh 20 gui_overview
   ```

3. **The GIF will be saved to**: `docs/images/gui_overview.gif`

## Scripts

### `record_simple.sh` (Recommended)

Simple recording script that's easy to use.

**Usage:**
```bash
./record_simple.sh [duration_seconds] [output_name]
```

**Examples:**
```bash
# Record for 20 seconds, save as gui_demo.gif
./record_simple.sh 20 gui_demo

# Record for 30 seconds, save as queue_operations.gif
./record_simple.sh 30 queue_operations
```

**Features:**
- Automatically starts the GUI
- Records for specified duration
- Converts to optimized GIF
- Works without xdotool (falls back to full screen)

### `record_gui.sh` (Advanced)

Advanced script with automatic window detection.

**Usage:**
```bash
./record_gui.sh [output_name]
```

**Requirements:**
- `xdotool` must be installed
- GUI window must be detectable

**Features:**
- Automatic window detection
- Records only the GUI window
- Higher quality output

## Creating Different GIFs

### 1. Overview GIF
```bash
./record_simple.sh 25 gui_overview
```
- Show all tabs
- Navigate through different sections
- Show the overall interface

### 2. Queue Operations GIF
```bash
./record_simple.sh 20 gui_queue
```
- Focus on Queue tab
- Enable auto producer/consumer
- Show queue visualization
- Interact with controls

### 3. Hash Map Operations GIF
```bash
./record_simple.sh 20 gui_hashmap
```
- Focus on Hash Map tab
- Insert, get, erase operations
- Show statistics

### 4. Thread Pool GIF
```bash
./record_simple.sh 20 gui_threadpool
```
- Focus on Thread Pool tab
- Submit multiple tasks
- Show active tasks graph

### 5. Performance Metrics GIF
```bash
./record_simple.sh 25 gui_performance
```
- Focus on Performance tab
- Show all graphs
- Explain metrics

## Tips

1. **Prepare before recording**: Know what you want to show
2. **Keep it focused**: One GIF per feature/tab
3. **Smooth movements**: Move mouse slowly
4. **Show results**: Let graphs update and show data
5. **Good timing**: 15-25 seconds is usually perfect

## Troubleshooting

### GUI doesn't start
- Make sure you built the GUI: `cd build && cmake --build .`
- Check if `./gui` runs manually

### Window not detected
- Install xdotool: `sudo pacman -S xdotool`
- Or use `record_simple.sh` which doesn't require it

### Poor quality GIF
- The scripts automatically optimize, but you can adjust:
  - Frame rate: Change `-r 15` in the script
  - Resolution: Change `scale=1280:-1` in the script

### Recording too large
- Reduce duration
- Lower frame rate (change `-r 15` to `-r 10`)
- Reduce scale (change `1280` to `960`)


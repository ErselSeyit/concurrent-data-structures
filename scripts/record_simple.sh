#!/bin/bash

# Simpler recording script using ffmpeg
# Usage: ./record_simple.sh [duration_seconds] [output_name]

DURATION=${1:-20}
OUTPUT_NAME=${2:-"gui_demo"}
OUTPUT_DIR="../docs/images"

mkdir -p "$OUTPUT_DIR"

echo "=========================================="
echo "GUI Recording Script"
echo "=========================================="
echo ""
echo "Instructions:"
echo "1. This script will start the GUI application"
echo "2. You have $DURATION seconds to interact with it"
echo "3. The recording will automatically stop"
echo "4. A GIF will be created in $OUTPUT_DIR"
echo ""
echo "Press Enter to start recording..."
read

# Start GUI in background
cd ../build
./gui &
GUI_PID=$!

# Wait for GUI to appear
sleep 2

# Get the GUI window
WINDOW_ID=$(xdotool search --name "Concurrent Data Structures Monitor" 2>/dev/null | tail -1)

if [ -z "$WINDOW_ID" ]; then
    echo "Warning: Could not automatically detect window"
    echo "Please manually specify the window area to record"
    echo "Or install xdotool: sudo pacman -S xdotool"
    echo ""
    echo "Recording entire screen for $DURATION seconds..."
    ffmpeg -f x11grab -s 1920x1080 -i :0.0 -t $DURATION -r 15 "$OUTPUT_DIR/${OUTPUT_NAME}_temp.mp4" -y
else
    # Get window geometry
    GEOMETRY=$(xdotool getwindowgeometry $WINDOW_ID 2>/dev/null | grep Geometry | awk '{print $2}')
    POS=$(xdotool getwindowgeometry $WINDOW_ID 2>/dev/null | grep Position | awk '{print $2}')
    
    if [ ! -z "$GEOMETRY" ] && [ ! -z "$POS" ]; then
        WIDTH=$(echo $GEOMETRY | cut -d'x' -f1)
        HEIGHT=$(echo $GEOMETRY | cut -d'x' -f2)
        X=$(echo $POS | cut -d',' -f1)
        Y=$(echo $POS | cut -d',' -f2)
        
        echo "Recording window: ${WIDTH}x${HEIGHT} at ($X, $Y)"
        ffmpeg -f x11grab -s ${WIDTH}x${HEIGHT} -i :0.0+${X},${Y} -t $DURATION -r 15 "$OUTPUT_DIR/${OUTPUT_NAME}_temp.mp4" -y
    else
        echo "Recording entire screen..."
        ffmpeg -f x11grab -s 1920x1080 -i :0.0 -t $DURATION -r 15 "$OUTPUT_DIR/${OUTPUT_NAME}_temp.mp4" -y
    fi
fi

# Stop GUI
kill $GUI_PID 2>/dev/null
wait $GUI_PID 2>/dev/null

# Convert to GIF
echo "Converting to GIF (this may take a moment)..."
ffmpeg -i "$OUTPUT_DIR/${OUTPUT_NAME}_temp.mp4" \
    -vf "fps=12,scale=1280:-1:flags=lanczos,split[s0][s1];[s0]palettegen=max_colors=256[p];[s1][p]paletteuse" \
    -loop 0 "$OUTPUT_DIR/${OUTPUT_NAME}.gif" -y

# Cleanup
rm -f "$OUTPUT_DIR/${OUTPUT_NAME}_temp.mp4"

echo ""
echo "✓ Recording complete!"
echo "✓ GIF saved to: $OUTPUT_DIR/${OUTPUT_NAME}.gif"


#!/bin/bash

# Script to record the GUI application and create GIFs
# Usage: ./record_gui.sh [output_name]

OUTPUT_NAME=${1:-"gui_demo"}
OUTPUT_DIR="../docs/images"
TEMP_VIDEO="/tmp/gui_recording.mp4"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

echo "Starting GUI recording..."
echo "The GUI window will open in 3 seconds..."
echo "Press Ctrl+C to stop recording"

sleep 3

# Start the GUI in background
cd ../build
./gui &
GUI_PID=$!

# Wait a moment for GUI to start
sleep 2

# Get window ID (assuming the GUI window is the most recent)
WINDOW_ID=$(xdotool search --name "Concurrent Data Structures Monitor" | tail -1)

if [ -z "$WINDOW_ID" ]; then
    echo "Could not find GUI window. Trying alternative method..."
    # Try to find window by class
    WINDOW_ID=$(xdotool search --class "gui" | tail -1)
fi

if [ -z "$WINDOW_ID" ]; then
    echo "Error: Could not find GUI window"
    kill $GUI_PID 2>/dev/null
    exit 1
fi

# Activate the window
xdotool windowactivate $WINDOW_ID
sleep 1

# Get window geometry
GEOMETRY=$(xdotool getwindowgeometry $WINDOW_ID | grep Geometry | awk '{print $2}')
WIDTH=$(echo $GEOMETRY | cut -d'x' -f1)
HEIGHT=$(echo $GEOMETRY | cut -d'x' -f2)
POS=$(xdotool getwindowgeometry $WINDOW_ID | grep Position | awk '{print $2}')
X=$(echo $POS | cut -d',' -f1)
Y=$(echo $POS | cut -d',' -f2)

echo "Recording window: ${WIDTH}x${HEIGHT} at position ($X, $Y)"
echo "Recording for 30 seconds (or press Ctrl+C to stop early)..."

# Record using ffmpeg
ffmpeg -f x11grab -s ${WIDTH}x${HEIGHT} -i :0.0+${X},${Y} -t 30 -r 30 "$TEMP_VIDEO" -y

# Stop the GUI
kill $GUI_PID 2>/dev/null
wait $GUI_PID 2>/dev/null

# Convert to GIF
echo "Converting to GIF..."
ffmpeg -i "$TEMP_VIDEO" -vf "fps=15,scale=1280:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" \
    -loop 0 "$OUTPUT_DIR/${OUTPUT_NAME}.gif" -y

# Cleanup
rm -f "$TEMP_VIDEO"

echo "Recording complete! GIF saved to: $OUTPUT_DIR/${OUTPUT_NAME}.gif"


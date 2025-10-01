#!/bin/bash

echo "Rose requires Microphone permission to record audio for local transcription."
echo "No Accessibility permission is required for global hotkeys."
echo ""
echo "Grant permission in System Settings:"
echo "  System Settings → Privacy & Security → Microphone → enable Rose"
echo ""
echo "Press Enter to open the Microphone privacy pane..."
read
open "x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone"

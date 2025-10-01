#!/bin/bash
set -euo pipefail

echo "Setting up Rose..."

echo "Installing dependencies..."
if ! command -v brew &> /dev/null; then
    echo "Homebrew not found. Please install Homebrew first."
    exit 1
fi

brew install portaudio cmake

echo "Downloading default Whisper model..."
./download_model.sh

echo "Setup complete! Run ./build.sh to build the application."

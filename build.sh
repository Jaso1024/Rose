#!/bin/bash
set -euo pipefail

echo "Building Rose..."

echo "Ensuring submodules are initialized..."
git submodule update --init --recursive

mkdir -p build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu)

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "To run (GUI): open ./build/Rose.app"
    echo "To run with logs: ./build/Rose.app/Contents/MacOS/Rose"
else
    echo "Build failed!"
    exit 1
fi

#!/bin/bash

# Build script for Multimode Clock Source
# Requires PICO_SDK_PATH to be set

set -e

# Check if PICO_SDK_PATH is set
if [ -z "$PICO_SDK_PATH" ]; then
    echo "Error: PICO_SDK_PATH environment variable is not set"
    echo "Please set it to your Pico SDK installation directory"
    echo "Example: export PICO_SDK_PATH=/path/to/pico-sdk"
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

# Change to build directory
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

# Build the project
echo "Building project..."
make -j$(nproc)

echo ""
echo "Build complete!"
echo "Output files:"
echo "  - multimode_clock_source.uf2 (flash to Pico)"
echo "  - multimode_clock_source.elf (debug file)"
echo ""
echo "To flash:"
echo "1. Hold BOOTSEL button on Pico while connecting USB"
echo "2. Copy multimode_clock_source.uf2 to RPI-RP2 drive"
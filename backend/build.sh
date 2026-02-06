#!/bin/bash

# Build script for Kanba C++ Backend

set -e

echo "Building Kanba C++ Backend..."

# Check for required tools
command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake is required but not installed."; exit 1; }
command -v make >/dev/null 2>&1 || { echo "ERROR: make is required but not installed."; exit 1; }

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo ""
    echo "CMake configuration failed. Make sure Drogon framework is installed."
    echo "See: https://github.com/drogonframework/drogon/wiki/ENG-02-Installation"
    exit 1
fi

# Build
make -j$(nproc)

echo ""
echo "Build complete! Run with: ./build/kanba-backend"

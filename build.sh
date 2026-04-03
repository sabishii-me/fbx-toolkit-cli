#!/bin/bash
# Local build script for Linux/macOS

CONFIG="${1:-Release}"
FBX_SDK_ROOT="${FBX_SDK_ROOT:-/usr/local/fbx}"

echo "=== FBX Toolkit Build Script ==="
echo ""

# Check if FBX SDK exists
if [ ! -f "$FBX_SDK_ROOT/include/fbxsdk.h" ]; then
    echo "ERROR: FBX SDK not found at: $FBX_SDK_ROOT"
    echo ""
    echo "Please either:"
    echo "  1. Install FBX SDK to /usr/local/fbx"
    echo "  2. Set environment variable: export FBX_SDK_ROOT=/your/path"
    echo ""
    echo "Download from: https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-3-7"
    exit 1
fi

echo "✓ Found FBX SDK at: $FBX_SDK_ROOT"
echo ""

# Create build directory
mkdir -p build

# Configure
echo "Configuring CMake..."
cmake -B build -DUSE_SYSTEM_FBX_SDK=ON -DFBX_SDK_ROOT="$FBX_SDK_ROOT" -DCMAKE_BUILD_TYPE=$CONFIG

if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed!"
    exit 1
fi

echo ""
echo "✓ Configuration successful"
echo ""

# Build
echo "Building project..."
cmake --build build --config $CONFIG

if [ $? -ne 0 ]; then
    echo "ERROR: Build failed!"
    exit 1
fi

echo ""
echo "✓ Build successful!"
echo ""

# Check output
EXE_PATH="build/bin/fbx-toolkit"
if [ -f "$EXE_PATH" ]; then
    echo "Executable: $EXE_PATH"
    echo ""
    echo "Testing executable..."
    "$EXE_PATH"
    echo ""
    echo "=== Build Complete ==="
else
    echo "WARNING: Executable not found at expected location: $EXE_PATH"
    echo "Searching for executable..."
    find build -name "fbx-toolkit" -type f
fi

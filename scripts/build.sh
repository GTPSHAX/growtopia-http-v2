#!/bin/bash
# Build script for Growtopia HTTP Server

set -e

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"

echo "Building Growtopia HTTP Server (${BUILD_TYPE})..."

# Configure
cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -B "${BUILD_DIR}" \
      -S .

# Build
cmake --build "${BUILD_DIR}" -j$(nproc)

echo "Build complete! Binary is at: bin/server"

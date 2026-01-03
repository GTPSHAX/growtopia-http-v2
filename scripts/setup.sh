#!/bin/bash
# Development setup script

set -e

echo "Setting up Growtopia HTTP development environment..."

# Check for required tools
command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake is required but not installed."; exit 1; }
command -v gcc >/dev/null 2>&1 || { echo "ERROR: gcc is required but not installed."; exit 1; }

# Optional tools
if ! command -v clang-format >/dev/null 2>&1; then
    echo "WARNING: clang-format not found. Code formatting will not be available."
fi

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "WARNING: clang-tidy not found. Static analysis will not be available."
fi

# Create build directory and configure
echo "Configuring build..."
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -B build \
      -S .

# Create symlink for compile_commands.json
ln -sf build/compile_commands.json .

echo "Setup complete! You can now build with: cmake --build build"
echo "Or use: ./scripts/build.sh"

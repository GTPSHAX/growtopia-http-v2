#!/bin/bash
# Development setup script

set -e

echo "Setting up Growtopia HTTP development environment..."

# Check and install required dependencies
if command -v apt-get >/dev/null 2>&1; then
    echo "Checking for required development libraries..."
    PACKAGES_TO_INSTALL=""
    
    # Check for OpenSSL development library
    if ! dpkg -l | grep -q libssl-dev; then
        echo "libssl-dev not found, will install..."
        PACKAGES_TO_INSTALL="$PACKAGES_TO_INSTALL libssl-dev"
    fi
    
    # Check for zlib development library
    if ! dpkg -l | grep -q zlib1g-dev; then
        echo "zlib1g-dev not found, will install..."
        PACKAGES_TO_INSTALL="$PACKAGES_TO_INSTALL zlib1g-dev"
    fi
    
    # Check for libuv development library
    if ! dpkg -l | grep -q libuv1-dev; then
        echo "libuv1-dev not found, will install..."
        PACKAGES_TO_INSTALL="$PACKAGES_TO_INSTALL libuv1-dev"
    fi
    
    # Install missing packages
    if [ ! -z "$PACKAGES_TO_INSTALL" ]; then
        echo "Installing required packages:$PACKAGES_TO_INSTALL"
        sudo apt-get update
        sudo apt-get install -y $PACKAGES_TO_INSTALL
    else
        echo "All required development libraries are already installed."
    fi
elif command -v yum >/dev/null 2>&1; then
    echo "Installing dependencies for RHEL/CentOS..."
    sudo yum install -y openssl-devel zlib-devel libuv-devel
elif command -v dnf >/dev/null 2>&1; then
    echo "Installing dependencies for Fedora..."
    sudo dnf install -y openssl-devel zlib-devel libuv-devel
elif command -v brew >/dev/null 2>&1; then
    echo "Installing dependencies for macOS..."
    brew install openssl zlib libuv
else
    echo "WARNING: Could not detect package manager. Please install manually:"
    echo "  - OpenSSL development library"
    echo "  - zlib development library"
    echo "  - libuv development library"
fi

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

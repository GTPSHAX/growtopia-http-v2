#!/bin/bash
# Run clang-tidy on all source files

set -e

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "ERROR: clang-tidy not found"
    exit 1
fi

if [ ! -f compile_commands.json ]; then
    echo "ERROR: compile_commands.json not found. Run cmake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON first."
    exit 1
fi

echo "Running clang-tidy..."

find src -type f -name "*.c" -exec clang-tidy {} \;

echo "Analysis complete!"

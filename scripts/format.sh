#!/bin/bash
# Run clang-format on all source files

set -e

if ! command -v clang-format >/dev/null 2>&1; then
    echo "ERROR: clang-format not found"
    exit 1
fi

echo "Formatting source files..."

find src include -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} \;

echo "Formatting complete!"

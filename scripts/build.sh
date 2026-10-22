#!/usr/bin/env bash
# ==============================================================
# Back2You - Automated Build Script for Linux & macOS
# ==============================================================

set -e

echo "[Back2You] Checking build environment..."

mkdir -p build
cd build

echo "[Back2You] Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..

echo "[Back2You] Compiling Back2You C++ server..."
cmake --build . --config Release -- -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

echo ""
echo "[Back2You] Build completed successfully! Executable: ./build/bin/back2you_server"
cd ..

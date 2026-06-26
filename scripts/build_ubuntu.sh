#!/usr/bin/env bash
set -euo pipefail

if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake is missing. Install with: sudo apt install cmake" >&2
  exit 1
fi

if ! pkg-config --exists opencv4; then
  echo "OpenCV development files were not found by pkg-config." >&2
  echo "Install with: sudo apt install libopencv-dev pkg-config" >&2
  exit 1
fi

mkdir -p build
cmake -S . -B build -DBUILD_VIDEO_PIPELINE=ON
cmake --build build

echo "Built:"
echo "  build/hockey_shot_analyzer"
echo "  build/video_to_landmarks"

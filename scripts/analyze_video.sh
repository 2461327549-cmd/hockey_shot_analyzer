#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 4 || $# -gt 5 ]]; then
  echo "Usage: $0 <video> <pose_model.onnx> <output_dir> <right|left> [scoring.conf]" >&2
  exit 1
fi

VIDEO_PATH="$1"
MODEL_PATH="$2"
OUTPUT_DIR="$3"
GOAL_DIRECTION="$4"
SCORING_CONFIG="${5:-config/scoring.conf}"

mkdir -p "$OUTPUT_DIR"

if [[ ! -x build/video_to_landmarks ]]; then
  echo "build/video_to_landmarks is missing. Build with CMake option BUILD_VIDEO_PIPELINE=ON first." >&2
  exit 1
fi

if [[ ! -x build/hockey_shot_analyzer ]]; then
  echo "build/hockey_shot_analyzer is missing. Build the analyzer first." >&2
  exit 1
fi

build/video_to_landmarks \
  "$VIDEO_PATH" \
  "$MODEL_PATH" \
  "$OUTPUT_DIR/landmarks.csv" \
  "$GOAL_DIRECTION"

build/hockey_shot_analyzer \
  "$OUTPUT_DIR/landmarks.csv" \
  "$OUTPUT_DIR/metrics.csv" \
  "$OUTPUT_DIR/report.json" \
  "$GOAL_DIRECTION" \
  "$OUTPUT_DIR/timeline.svg" \
  "$OUTPUT_DIR/report.html" \
  "$SCORING_CONFIG"

echo "Done: $OUTPUT_DIR/report.html"

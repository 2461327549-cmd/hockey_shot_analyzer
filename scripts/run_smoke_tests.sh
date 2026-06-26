#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p build outputs/tmp

echo "[1/5] Building analyzer"
if [[ -z "${CXX:-}" ]]; then
  if command -v g++ >/dev/null 2>&1; then
    CXX=g++
  elif command -v clang++ >/dev/null 2>&1; then
    CXX=clang++
  elif command -v c++ >/dev/null 2>&1; then
    CXX=c++
  else
    echo "No C++ compiler found. Install one with: sudo apt install build-essential" >&2
    exit 1
  fi
fi
"$CXX" -std=c++17 -Wall -Wextra -Wpedantic src/*.cpp -o build/hockey_shot_analyzer

echo "[2/5] Running right-direction analysis"
./build/hockey_shot_analyzer \
  samples/landmarks.csv \
  outputs/tmp/metrics_right.csv \
  outputs/tmp/report_right.json \
  right \
  outputs/tmp/timeline_right.svg \
  outputs/tmp/report_right.html \
  config/scoring.conf > outputs/tmp/right_stdout.txt

grep -q '"total_score": 77.10' outputs/tmp/report_right.json
grep -q '"goal_direction": "right"' outputs/tmp/report_right.json
grep -q 'phase' outputs/tmp/metrics_right.csv
test -s outputs/tmp/timeline_right.svg
test -s outputs/tmp/report_right.html

echo "[3/5] Running left-direction analysis"
./build/hockey_shot_analyzer \
  samples/landmarks.csv \
  outputs/tmp/metrics_left.csv \
  outputs/tmp/report_left.json \
  left > outputs/tmp/left_stdout.txt

grep -q '"total_score": 47.10' outputs/tmp/report_left.json
grep -q '"goal_direction": "left"' outputs/tmp/report_left.json

echo "[4/5] Checking CSV header validation"
BAD_CSV="outputs/tmp/bad_header.csv"
awk 'NR == 1 { sub("shoulder_x", "bad_shoulder_x") } { print }' samples/landmarks.csv > "$BAD_CSV"

set +e
./build/hockey_shot_analyzer "$BAD_CSV" outputs/tmp/bad_metrics.csv outputs/tmp/bad_report.json right > outputs/tmp/bad_stdout.txt 2> outputs/tmp/bad_stderr.txt
BAD_STATUS=$?
set -e

if [[ "$BAD_STATUS" -eq 0 ]]; then
  echo "Expected bad CSV to fail, but it succeeded" >&2
  exit 1
fi
grep -q "expected 'shoulder_x'" outputs/tmp/bad_stderr.txt

echo "[5/5] Smoke tests passed"

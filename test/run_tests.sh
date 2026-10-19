#!/usr/bin/env bash
set -euo pipefail

# Determine repository root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

echo "Compiling robotic arm unit tests..."
g++ -std=c++17 -Wall -Wextra -Isrc -o test/run_tests test/test_arm_kinematics.cpp src/arm_kinematics.cpp

echo "Executing unit tests..."
./test/run_tests

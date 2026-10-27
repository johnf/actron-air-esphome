#!/bin/bash
# Build and run the host tests for the keypad frame decoder.

set -euo pipefail

cd "$(dirname "$0")/.."

out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror \
  -I components/actron_air_esphome \
  tests/frame_decoder_test.cpp -o "$out/frame_decoder_test"

"$out/frame_decoder_test" tests/fixtures/keypad-capture.txt

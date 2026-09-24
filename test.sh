#!/bin/sh
# Configure, build, and run tests.
#
#   ./build-compile-test.sh          # debug preset only
#   ./build-compile-test.sh all      # debug, release, asan, tsan in sequence
#   ./build-compile-test.sh asan     # any single preset
set -e   # stop at the first failure instead of running tests on a broken build

cd "$(dirname "$0")"

mode="${1:-debug}"
case "$mode" in
    all) presets="debug release asan tsan" ;;
    *)   presets="$mode" ;;
esac

for preset in $presets; do
    echo "==> $preset"
    cmake --preset "$preset"
    cmake --build --preset "$preset"
    ctest --preset "$preset"
done

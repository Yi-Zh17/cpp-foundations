#!/bin/sh
# Configure, build, and run benchmarks.
#
#   ./build-compile-benchmark.sh          # release preset only (benchmarks are meaningless at -O0)
#   ./build-compile-benchmark.sh all      # debug, release, asan, tsan in sequence
#   ./build-compile-benchmark.sh debug    # any single preset
#
# Extra arguments are passed to the benchmark binary, e.g.
#   ./build-compile-benchmark.sh release --benchmark_filter=Record
set -e

cd "$(dirname "$0")"

mode="${1:-release}"
case "$mode" in
    all) presets="debug release asan tsan" ;;
    *)   presets="$mode" ;;
esac
[ $# -gt 0 ] && shift

for preset in $presets; do
    echo "==> $preset"
    cmake --preset "$preset"
    cmake --build --preset "$preset"
    "./build/$preset/benchmarks/foundations_bench" "$@"
done

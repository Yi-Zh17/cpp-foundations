#!/bin/sh
# Configure, build, and run benchmarks.
#
#   ./benchmark.sh                            # release preset, every benchmark file
#   ./benchmark.sh all                        # debug, release, asan, tsan in sequence
#   ./benchmark.sh debug                      # any single preset
#   ./benchmark.sh release -f hist            # only benchmark files whose name matches a regex
#
# -f is an extended regex matched against the bench_*.cpp file names in
# benchmarks/ (without .cpp). Only the matching executables are built. Anything
# after the options is passed to each benchmark binary, e.g.
#   ./benchmark.sh release -f hist --benchmark_filter=Record
set -e

cd "$(dirname "$0")"

mode=release
file=

usage() {
    sed -n '2,13p' "$0" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

# First argument is the preset if it does not look like an option.
case "${1:-}" in
    ""|-*) ;;
    *) mode="$1"; shift ;;
esac

while [ $# -gt 0 ]; do
    case "$1" in
        -f|--filter) [ $# -ge 2 ] || usage 1; file="$2"; shift 2 ;;
        --filter=*) file="${1#--filter=}"; shift ;;
        -h|--help) usage ;;
        --)        shift; break ;;
        *)         break ;;
    esac
done

# Resolve the regex to the list of matching benchmark files (targets share the name).
names=$(for f in benchmarks/bench_*.cpp; do basename "$f" .cpp; done)
if [ -n "$file" ]; then
    names=$(printf '%s\n' $names | grep -E -- "$file" || true)
    if [ -z "$names" ]; then
        echo "benchmark.sh: no benchmark file in benchmarks/ matches: $file" >&2
        exit 1
    fi
fi

case "$mode" in
    all) presets="debug release asan tsan" ;;
    *)   presets="$mode" ;;
esac

for preset in $presets; do
    echo "==> $preset"
    cmake --preset "$preset"
    # shellcheck disable=SC2086  # names is a deliberate word list
    cmake --build --preset "$preset" --target $names
    for name in $names; do
        echo "--> $name"
        "./build/$preset/benchmarks/$name" "$@"
    done
done

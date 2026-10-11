#!/bin/sh
# Configure, build, and run tests.
#
#   ./test.sh                          # debug preset, every test file
#   ./test.sh all                      # debug, release, asan, tsan in sequence
#   ./test.sh asan                     # any single preset
#   ./test.sh debug -f string          # only test files whose name matches a regex
#
# -f is an extended regex matched against the test_*.cpp file names in
# tests/ (without .cpp). Only the matching executables are built, so nothing
# else in the repo is compiled. Anything after the options is passed to ctest,
# e.g.  ./test.sh debug -f 'string|hist' --verbose
set -e   # stop at the first failure instead of running tests on a broken build

cd "$(dirname "$0")"

mode=debug
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

# Resolve the regex to the list of matching test files (targets share the name).
names=$(for f in tests/test_*.cpp; do basename "$f" .cpp; done)
if [ -n "$file" ]; then
    names=$(printf '%s\n' $names | grep -E -- "$file" || true)
    if [ -z "$names" ]; then
        echo "test.sh: no test file in tests/ matches: $file" >&2
        exit 1
    fi
    # ctest label regex anchored to exactly the matched files.
    labels="^($(printf '%s\n' $names | paste -sd '|' -))\$"
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
    if [ -n "$file" ]; then
        ctest --preset "$preset" -L "$labels" "$@"
    else
        ctest --preset "$preset" "$@"
    fi
done

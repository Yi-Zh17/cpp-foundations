#!/bin/sh
set -e   # stop at the first failure instead of running tests on a broken build

cd /Users/yi/self-study/cpp-foundations

cmake --preset debug
cmake --build --preset debug
ctest --preset debug

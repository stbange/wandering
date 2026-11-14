#!/usr/bin/env sh
set -eu
cmake -S . -B build-test -DWC_BUILD_GAME=OFF -DWC_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-test --parallel 4
ctest --test-dir build-test --output-on-failure
mkdir -p artifacts
gcovr --root . --filter src/core --fail-under-line 95 --html-details artifacts/coverage.html --json-summary artifacts/coverage-summary.json --txt - build-test

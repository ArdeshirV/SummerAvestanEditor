#!/usr/bin/env bash
# Build and run the focused regression tests using qmake + Qt Test.
set -euo pipefail

cd "$(dirname "$0")"

tests=(test_document test_config test_keyboard test_editor)
status=0

for test in "${tests[@]}"; do
    echo "===== ${test} ====="
    mkdir -p "build/${test}"
    (
        cd "build/${test}"
        qmake6 "../../${test}.pro" >/dev/null
        make -j"$(nproc 2>/dev/null || echo 2)" >/dev/null
        QT_QPA_PLATFORM=offscreen timeout 120 "./${test}"
    ) || status=1
done

exit "${status}"

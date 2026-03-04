#!/bin/bash
# CEELL OSAL Isolation Check
# REQ-OSAL-001
#
# Verifies that no source file outside ceell/osal/zephyr/ directly includes
# Zephyr headers. All Zephyr access must go through the OSAL layer.
#
# Usage: tools/check_osal_isolation.sh [--strict]
#   --strict: treat any match as error (default: just report)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

SEARCH_DIRS=(
    "$PROJECT_DIR/ceell/core/"
    "$PROJECT_DIR/ceell/network/"
    "$PROJECT_DIR/ceell/ota/"
    "$PROJECT_DIR/src/"
)

VIOLATIONS=0

echo "=== CEELL OSAL Isolation Check ==="
echo ""

for dir in "${SEARCH_DIRS[@]}"; do
    if [ ! -d "$dir" ]; then
        continue
    fi

    # Find any #include <zephyr/...> in .c and .h files
    while IFS= read -r match; do
        echo "VIOLATION: $match"
        VIOLATIONS=$((VIOLATIONS + 1))
    done < <(grep -rn '#include <zephyr/' "$dir" --include='*.c' --include='*.h' 2>/dev/null || true)
done

echo ""
if [ "$VIOLATIONS" -eq 0 ]; then
    echo "PASS: No direct Zephyr includes found outside OSAL layer"
    exit 0
else
    echo "FAIL: Found $VIOLATIONS direct Zephyr include(s) outside OSAL layer"
    echo "All Zephyr access must go through ceell/osal/ headers."
    exit 1
fi

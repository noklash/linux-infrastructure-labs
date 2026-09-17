#!/usr/bin/env bash

set -u

TARGET="${1:-README.md}"

echo "=== Target ==="
echo "$TARGET"
echo

echo "=== fswatch ==="
./fswatch info "$TARGET"

echo
echo "=== Linux stat ==="
stat "$TARGET"
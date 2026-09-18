#!/usr/bin/env bash

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

TARGET="$SCRIPT_DIR/target.txt"
LINK="$SCRIPT_DIR/link.txt"
PROGRAM="$SCRIPT_DIR/compare"

cleanup()
{
    rm -f "$TARGET" "$LINK" "$PROGRAM"
}

trap cleanup EXIT

cd "$SCRIPT_DIR" || exit 1

echo "=== Building experiment ==="

gcc \
    -Wall \
    -Wextra \
    -Wpedantic \
    -std=c17 \
    compare.c \
    -o compare

if [ "$?" -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

echo "Build successful."
echo

echo "=== Preparing filesystem state ==="

rm -f "$TARGET" "$LINK"

printf 'symbolic link experiment\n' > "$TARGET"

ln -s "$(basename "$TARGET")" "$LINK"

echo "Created:"
echo "  target: $TARGET"
echo "  link:   $LINK"
echo

echo "=== Filesystem objects ==="

ls -li "$TARGET" "$LINK"

echo

echo "=== Inspect target.txt ==="

./compare "$TARGET"

echo "=== Inspect link.txt while target exists ==="

./compare "$LINK"

echo "=== Removing target.txt ==="

rm "$TARGET"

echo

echo "=== Inspect broken symbolic link ==="

ls -li "$LINK"

echo

./compare "$LINK"

echo "=== Experiment complete ==="
#!/usr/bin/env bash

set -u

PASS=0
FAIL=0

TEST_DIR=$(mktemp -d)

cleanup() {
    rm -rf "$TEST_DIR"
}

trap cleanup EXIT

pass() {
    echo "PASS: $1"
    PASS=$((PASS + 1))
}

fail() {
    echo "FAIL: $1"
    FAIL=$((FAIL + 1))
}

assert_contains() {
    local description="$1"
    local output="$2"
    local expected="$3"

    if printf '%s\n' "$output" | grep -Fq "$expected"; then
        pass "$description"
    else
        fail "$description"
        echo "  expected: $expected"
        echo "  output:"
        printf '  %s\n' "$output"
    fi
}

assert_succeeds() {
    local description="$1"
    shift

    if "$@" >/dev/null 2>&1; then
        pass "$description"
    else
        fail "$description"
    fi
}

assert_fails() {
    local description="$1"
    shift

    if "$@" >/dev/null 2>&1; then
        fail "$description"
    else
        pass "$description"
    fi
}

echo "Running fswatch tests..."
echo

TEST_FILE="$TEST_DIR/test-file.txt"
TEST_DIR_PATH="$TEST_DIR/test-directory"
TARGET="$TEST_DIR/target.txt"
SYMLINK="$TEST_DIR/link.txt"
BROKEN_SYMLINK="$TEST_DIR/broken.txt"

printf 'hello filesystem\n' > "$TEST_FILE"
mkdir "$TEST_DIR_PATH"
printf 'symlink target\n' > "$TARGET"

ln -s "$TARGET" "$SYMLINK"
ln -s "$TEST_DIR/missing.txt" "$BROKEN_SYMLINK"

output=$(./fswatch info "$TEST_FILE")

assert_contains \
    "regular file detection" \
    "$output" \
    "Type: regular file"

assert_contains \
    "regular file link count" \
    "$output" \
    "Links: 1"

assert_contains \
    "regular file size" \
    "$output" \
    "Size: 17 bytes"

output=$(./fswatch info "$TEST_DIR_PATH")

assert_contains \
    "directory detection" \
    "$output" \
    "Type: directory"

assert_contains \
    "directory inode information" \
    "$output" \
    "Inode:"

assert_contains \
    "directory device information" \
    "$output" \
    "Device:"

assert_contains \
    "directory link count" \
    "$output" \
    "Links:"

output=$(./fswatch info "$SYMLINK")

assert_contains \
    "symbolic link detection" \
    "$output" \
    "Type: symbolic link"

assert_contains \
    "symbolic link target" \
    "$output" \
    "Target: $TARGET"

assert_contains \
    "symbolic link size" \
    "$output" \
    "Size: ${#TARGET} bytes"

assert_succeeds \
    "existing symbolic link succeeds" \
    ./fswatch info "$SYMLINK"

rm "$TARGET"

output=$(./fswatch info "$SYMLINK")

assert_contains \
    "broken symbolic link detection" \
    "$output" \
    "Type: symbolic link"

assert_contains \
    "broken symbolic link target" \
    "$output" \
    "Target: $TARGET"

assert_succeeds \
    "broken symbolic link succeeds" \
    ./fswatch info "$SYMLINK"

output=$(./fswatch info "$BROKEN_SYMLINK")

assert_contains \
    "independent broken symbolic link detection" \
    "$output" \
    "Type: symbolic link"

assert_contains \
    "independent broken symbolic link target" \
    "$output" \
    "Target: $TEST_DIR/missing.txt"

assert_succeeds \
    "independent broken symbolic link succeeds" \
    ./fswatch info "$BROKEN_SYMLINK"

assert_fails \
    "nonexistent path fails" \
    ./fswatch info "$TEST_DIR/does-not-exist"

echo
echo "Passed: $PASS"
echo "Failed: $FAIL"

if [ "$FAIL" -ne 0 ]; then
    exit 1
fi

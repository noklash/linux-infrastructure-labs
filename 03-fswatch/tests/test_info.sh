#!/usr/bin/env bash

set -u

PASS=0
FAIL=0

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

output=$(./fswatch info README.md)

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
    "Size: 23338 bytes"

output=$(./fswatch info .)

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

assert_fails \
    "nonexistent path fails" \
    ./fswatch info does-not-exist

echo
echo "Passed: $PASS"
echo "Failed: $FAIL"

if [ "$FAIL" -ne 0 ]; then
    exit 1
fi
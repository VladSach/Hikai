#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

rm -rf "$SCRIPT_DIR/bin"

rm -rf "$SCRIPT_DIR/engine/build"
rm -rf "$SCRIPT_DIR/engine/bin"

rm -rf "$SCRIPT_DIR/editor/build"
rm -rf "$SCRIPT_DIR/editor/bin"

rm -rf "$SCRIPT_DIR/sandbox/build"
rm -rf "$SCRIPT_DIR/sandbox/bin"

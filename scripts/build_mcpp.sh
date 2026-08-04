#!/usr/bin/env bash
# Build the mcpp-kaki GDExtension using mcpp.
# This script replaces the old CMake workflow.

set -euo pipefail

cd "$(dirname "$0")/.."

# ----------------------------------------------------------------------
# 1. Build with mcpp (godot-cpp comes from the compat:godot-cpp package,
#    which ships pre-generated bindings — no Python/SCons step needed).
# ----------------------------------------------------------------------
echo "[mcpp-build] Building with mcpp..."
mcpp build "$@"

# ----------------------------------------------------------------------
# 2. Copy the resulting shared library to bin/ where Godot expects it.
# ----------------------------------------------------------------------
mkdir -p bin

SO_PATH=$(find target -name 'libmcpp-kaki.so' -type f -printf '%T@ %p\n' | sort -rn | head -n1 | cut -d' ' -f2-)
if [ -z "$SO_PATH" ]; then
    echo "[mcpp-build] ERROR: libmcpp-kaki.so not found under target/" >&2
    exit 1
fi

# godot-cpp treats editor and template_debug identically (both have DEBUG_ENABLED).
# Only template_release differs; release builds need a separate mcpp profile.
for TARGET_NAME in \
    "libmcpp-kaki.linux.editor.x86_64.so" \
    "libmcpp-kaki.linux.template_debug.x86_64.so"; do
    echo "[mcpp-build] Copying $SO_PATH -> bin/$TARGET_NAME"
    cp "$SO_PATH" "bin/$TARGET_NAME"
done

echo "[mcpp-build] Done."

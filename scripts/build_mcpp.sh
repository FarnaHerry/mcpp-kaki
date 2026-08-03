#!/usr/bin/env bash
# Build the mcpp-kaki GDExtension using mcpp.
# This script replaces the old CMake workflow.

set -euo pipefail

cd "$(dirname "$0")/.."

# ----------------------------------------------------------------------
# 1. Generate godot-cpp bindings for the project Godot version.
# ----------------------------------------------------------------------
echo "[mcpp-build] Generating godot-cpp bindings..."
python3 scripts/generate_godot_bindings.py

# ----------------------------------------------------------------------
# 2. Build with mcpp.
# ----------------------------------------------------------------------
echo "[mcpp-build] Building with mcpp..."
mcpp build "$@"

# ----------------------------------------------------------------------
# 3. Copy the resulting shared library to bin/ where Godot expects it.
# ----------------------------------------------------------------------
mkdir -p bin

SO_PATH=$(find target -name 'libmcpp-kaki.so' -type f | head -n1)
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

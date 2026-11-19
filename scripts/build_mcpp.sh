#!/usr/bin/env bash
# Build the mcpp-kaki GDExtension using mcpp.
# This script replaces the old CMake workflow.

set -euo pipefail

cd "$(dirname "$0")/.."

# ----------------------------------------------------------------------
# 0. 依赖模块 BMI 扁平化（mcpp 2026.10.8.1 布局回归的 workaround）。
#    该版本把依赖包的模块 BMI 输出到 pcm.cache/<ns>.<pkg>/ 子目录，却仍只把
#    pcm.cache 扁平目录传给 clang 的 -fprebuilt-module-path；于是普通 .cpp 里
#    `import mcpp_kaki.xxx;` 会报 "failed to find module file for module 'godot_cpp'"。
#    桥接：把子目录里的 .pcm 软链到扁平层（幂等）；仅当首次编译失败才触发。
# ----------------------------------------------------------------------
bridge_dep_bmis() {
    local n=0 d pcm flat
    for d in target/*/*/pcm.cache; do
        [ -d "$d" ] || continue
        for pcm in "$d"/*/*.pcm; do
            [ -e "$pcm" ] || continue
            flat="$d/$(basename "$pcm")"
            [ -e "$flat" ] && continue
            ln -sfn "${pcm#"$d"/}" "$flat"
            n=$((n + 1))
        done
    done
    echo "[mcpp-build] 已扁平化 $n 个依赖 BMI（缓存布局 workaround）"
}

# ----------------------------------------------------------------------
# 1. Build with mcpp (godot-cpp comes from the compat:godot-cpp package,
#    which ships pre-generated bindings — no Python/SCons step needed).
# ----------------------------------------------------------------------
echo "[mcpp-build] Building with mcpp..."
if ! mcpp build "$@"; then
    echo "[mcpp-build] 首次编译失败，尝试依赖 BMI 扁平化后重试..." >&2
    bridge_dep_bmis
    mcpp build "$@"
fi

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

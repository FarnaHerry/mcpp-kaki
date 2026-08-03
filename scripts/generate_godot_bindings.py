#!/usr/bin/env python3
"""Generate godot-cpp C++ bindings for the Godot version used by this project.

This replaces the binding generation step that CMake/SCons normally perform
inside the godot-cpp submodule. It must be run before `mcpp build`.
"""

import json
import os
import re
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
GODOT_CPP_DIR = PROJECT_ROOT / "godot-cpp"
GDEXTENSION_DIR = GODOT_CPP_DIR / "gdextension"
OUTPUT_DIR = PROJECT_ROOT / "mcpp-gen"

DEFAULT_API_VERSION = "4.6"


def detect_godot_version() -> str:
    """Read config_version from project.godot if available, else default."""
    project_godot = PROJECT_ROOT / "project.godot"
    if project_godot.exists():
        text = project_godot.read_text(encoding="utf-8")
        match = re.search(r'config_version\s*=\s*(\d+)', text)
        if match:
            # Godot 4.6 uses config_version=5; map is not 1:1, so prefer explicit API file.
            pass

    # Prefer explicit API version file if present.
    api_files = sorted(GDEXTENSION_DIR.glob("extension_api-*.json"))
    if api_files:
        # Extract versions and pick the latest, or the one matching DEFAULT_API_VERSION.
        versions = []
        for f in api_files:
            m = re.search(r'extension_api-(\d+-\d+)\.json', f.name)
            if m:
                versions.append((m.group(1).replace('-', '.'), f))
        versions.sort(key=lambda x: [int(n) for n in x[0].split('.')])
        for ver, f in versions:
            if ver == DEFAULT_API_VERSION:
                return DEFAULT_API_VERSION, f
        return versions[-1][0], versions[-1][1]

    return DEFAULT_API_VERSION, GDEXTENSION_DIR / "extension_api.json"


def get_api_file(version: str) -> Path:
    named = GDEXTENSION_DIR / f"extension_api-{version.replace('.', '-')}.json"
    if named.exists():
        return named
    fallback = GDEXTENSION_DIR / "extension_api.json"
    if fallback.exists():
        return fallback
    raise FileNotFoundError(f"No extension_api JSON found for version {version}")


def main() -> int:
    version, auto_file = detect_godot_version()
    # Allow override via environment variable.
    version = os.environ.get("GODOT_API_VERSION", version)
    api_file = get_api_file(version)
    interface_file = GDEXTENSION_DIR / "gdextension_interface.json"

    print(f"Generating godot-cpp bindings for Godot {version}")
    print(f"  API file: {api_file}")
    print(f"  Output:   {OUTPUT_DIR}")

    if not api_file.exists():
        print(f"ERROR: API file not found: {api_file}", file=sys.stderr)
        return 1
    if not interface_file.exists():
        print(f"ERROR: Interface file not found: {interface_file}", file=sys.stderr)
        return 1

    script = (
        "from binding_generator import generate_bindings\n"
        f"generate_bindings(\n"
        f"    api_filepath='{api_file.as_posix()}',\n"
        f"    interface_filepath='{interface_file.as_posix()}',\n"
        f"    use_template_get_node='True',\n"
        f"    bits='64',\n"
        f"    precision='single',\n"
        f"    output_dir='{OUTPUT_DIR.as_posix()}')"
    )

    result = subprocess.run(
        [sys.executable, "-c", script],
        cwd=GODOT_CPP_DIR,
        text=True,
    )
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())

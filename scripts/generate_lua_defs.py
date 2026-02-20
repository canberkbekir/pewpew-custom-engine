#!/usr/bin/env python3
"""
Generates EmmyLua definitions from CBEngine's LuaBindings.cpp.

Usage:
    python scripts/generate_lua_defs.py

Output:
    CBEngine-Editor/resources/lua-definitions/CBEngine.lua

This does a best-effort parse of the C++ sol2 bindings. For complex changes,
you may need to edit the output file manually. Run this after modifying
LuaBindings.cpp to get a starting point, then review the diff.
"""

import re
import os
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent
BINDINGS_FILE = PROJECT_ROOT / "CBEngine" / "src" / "CBEngine" / "Scripting" / "LuaBindings.cpp"
OUTPUT_FILE = PROJECT_ROOT / "CBEngine-Editor" / "resources" / "lua-definitions" / "CBEngine.lua"


def extract_entity_methods(source: str) -> list[dict]:
    """Extract Entity method names and return types from C++ source."""
    methods = []
    # Match patterns like: et["MethodName"] = [](Entity& e, ...) -> ReturnType {
    # or "MethodName", [](Entity& e, ...) -> ReturnType {
    pattern = re.compile(
        r'(?:et\["(\w+)"\]|"(\w+)")\s*[,=]\s*\[[^\]]*\]\s*\(([^)]*)\)\s*(?:->\s*([\w:&<>]+))?\s*\{',
        re.MULTILINE
    )
    for m in pattern.finditer(source):
        name = m.group(1) or m.group(2)
        params = m.group(3)
        ret = m.group(4)
        methods.append({
            "name": name,
            "params": params.strip(),
            "return_type": ret.strip() if ret else None
        })
    return methods


def extract_scene_methods(source: str) -> list[dict]:
    """Extract Scene method names from C++ source."""
    methods = []
    pattern = re.compile(
        r'st\["(\w+)"\]\s*=\s*\[[^\]]*\]\s*\(([^)]*)\)\s*(?:->\s*([\w:&<>]+))?\s*\{',
        re.MULTILINE
    )
    for m in pattern.finditer(source):
        methods.append({
            "name": m.group(1),
            "params": m.group(2).strip(),
            "return_type": m.group(3).strip() if m.group(3) else None
        })
    return methods


def main():
    if not BINDINGS_FILE.exists():
        print(f"Error: {BINDINGS_FILE} not found")
        return

    source = BINDINGS_FILE.read_text(encoding="utf-8")

    entity_methods = extract_entity_methods(source)
    scene_methods = extract_scene_methods(source)

    print(f"Found {len(entity_methods)} Entity methods")
    print(f"Found {len(scene_methods)} Scene methods")
    print()
    print("Entity methods:")
    for m in entity_methods:
        ret = f" -> {m['return_type']}" if m["return_type"] else ""
        print(f"  {m['name']}({m['params']}){ret}")

    print()
    print("Scene methods:")
    for m in scene_methods:
        ret = f" -> {m['return_type']}" if m["return_type"] else ""
        print(f"  {m['name']}({m['params']}){ret}")

    print()
    print(f"Definitions file: {OUTPUT_FILE}")
    print("NOTE: This script prints what it found. The actual definitions file")
    print("should be maintained by hand (or use this output to update it).")
    print("The hand-written file has better docs and parameter names.")


if __name__ == "__main__":
    main()

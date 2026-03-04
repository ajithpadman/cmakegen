#!/usr/bin/env python3
"""Split a single combined metadata JSON file into the folder layout expected by ConfigLoader.
Usage: split_metadata_to_folder.py <input.json> <output_folder>
"""
import json
import os
import sys

def main():
    if len(sys.argv) != 3:
        sys.stderr.write("Usage: split_metadata_to_folder.py <input.json> <output_folder>\n")
        sys.exit(1)
    input_path = sys.argv[1]
    output_dir = sys.argv[2]
    os.makedirs(output_dir, exist_ok=True)
    with open(input_path, "r") as f:
        root = json.load(f)
    if "project" in root:
        with open(os.path.join(output_dir, "project.json"), "w") as f:
            json.dump(root["project"], f, indent=2)
    if "socs" in root:
        with open(os.path.join(output_dir, "socs.json"), "w") as f:
            json.dump(root["socs"], f, indent=2)
    if "boards" in root:
        with open(os.path.join(output_dir, "boards.json"), "w") as f:
            json.dump(root["boards"], f, indent=2)
    if "toolchains" in root:
        with open(os.path.join(output_dir, "toolchains.json"), "w") as f:
            json.dump(root["toolchains"], f, indent=2)
    if "isa_variants" in root:
        with open(os.path.join(output_dir, "isa_variants.json"), "w") as f:
            json.dump(root["isa_variants"], f, indent=2)
    if "build_variants" in root:
        with open(os.path.join(output_dir, "build_variants.json"), "w") as f:
            json.dump(root["build_variants"], f, indent=2)
    if "source_tree" in root and "components" in root["source_tree"]:
        with open(os.path.join(output_dir, "components.json"), "w") as f:
            json.dump(root["source_tree"]["components"], f, indent=2)
    if "preset_matrix" in root:
        with open(os.path.join(output_dir, "cmake_preset.json"), "w") as f:
            json.dump(root["preset_matrix"], f, indent=2)
    if "dependencies" in root:
        deps = root["dependencies"]
        out = {}
        if deps.get("tool_requires"):
            out["tool_requires"] = deps["tool_requires"]
        if deps.get("extra_requires"):
            out["extra_requires"] = deps["extra_requires"]
        with open(os.path.join(output_dir, "conanfile.json"), "w") as f:
            json.dump(out if out else {}, f, indent=2)

if __name__ == "__main__":
    main()

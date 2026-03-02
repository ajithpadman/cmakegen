# Release Notes

## 0.2.2

### New Features

- **Interactive metadata wizard (FTXUI)** — Added a fullscreen, cross-platform interactive CLI (`cmakegen init` / `--interactive`) to build schema-valid metadata JSON via a guided wizard (projects, SoCs, boards, toolchains, variants, components, Conan deps).

### Changes

- **Always build interactive mode** — FTXUI is now fetched via CMake `FetchContent` when not provided by Conan, and `cmakegen` always links the interactive UI.
- **Presets and builds** — Existing `linux` and `windows` CMake presets are wired to build and test the interactive-enabled binary.

### Compatibility

- The JSON schema is unchanged; existing metadata files continue to work. The wizard simply generates JSON compatible with the current schema.


## 0.2.1

### New Features

- **Metadata file copying for components** — Library components can now declare `metadata_extensions` to copy extra metadata files (such as linker scripts `*.ld`) into the generated source tree alongside source and header files.

### Changes

- **Schema and parser** — `schema/scaffold-schema.json`, `schema.hpp`, and `parser.cpp` were extended with an optional `metadata_extensions` field on `source_tree.components`.
- **Copy engine** — `CopyEngine` now respects `metadata_extensions` when walking component source trees, while still applying existing path filters.
- **Documentation and examples** — `README.md`, sample metadata files in `examples/`, and E2E fixtures were updated to reference the new field and the bumped project version `0.2.1`.

### Compatibility

- `metadata_extensions` is optional and does not affect existing metadata. Components without this field continue to behave as before.

# Release Notes

## 0.2.1

### New Features

- **Metadata file copying for components** — Library components can now declare `metadata_extensions` to copy extra metadata files (such as linker scripts `*.ld`) into the generated source tree alongside source and header files.

### Changes

- **Schema and parser** — `schema/scaffold-schema.json`, `schema.hpp`, and `parser.cpp` were extended with an optional `metadata_extensions` field on `source_tree.components`.
- **Copy engine** — `CopyEngine` now respects `metadata_extensions` when walking component source trees, while still applying existing path filters.
- **Documentation and examples** — `README.md`, sample metadata files in `examples/`, and E2E fixtures were updated to reference the new field and the bumped project version `0.2.1`.

### Compatibility

- `metadata_extensions` is optional and does not affect existing metadata. Components without this field continue to behave as before.

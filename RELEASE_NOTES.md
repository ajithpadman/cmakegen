# Release Notes

## 0.2.0

### New Features

- **Condition field for components** — Executable, library, and layer components now support an optional `condition` field. Build only for matching presets (ISA, SOC, BOARD, BUILD_VARIANT). Use when a component targets specific hardware or build configurations.

- **Filter modes** — `filters.filter_mode` selects how `include_paths` and `exclude_paths` interact:
  - `include_first` (default): Include then exclude. Exclude removes from included set.
  - `exclude_first`: Exclude then include. Include can rescue a subset of excluded paths.

- **Compound conditions** — Variant and component conditions support `and`, `or`, and `not` for complex logic (e.g. `SOC in [stm32h7] AND NOT BUILD_VARIANT equals release`).

### Changes

- **JSON schema** — `schema/scaffold-schema.json` updated with full definitions for `condition`, `path_filters`, `preset_exclude`, `compiler`, and nested structures.

- **YAML documentation removed** — YAML support is opt-in (`CMAKEGEN_USE_YAML=ON`). Default build supports JSON only; docs updated accordingly.

### Compatibility

- Metadata files remain compatible. New fields (`condition`, `filter_mode`) are optional.
- Default `filter_mode` is `include_first`, matching previous behavior.

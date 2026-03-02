# Interactive wizard test checklist

Use this checklist to manually verify the interactive FTXUI wizard (`cmakegen init` or `cmakegen -i`). Run through the flows below and confirm the generated metadata is valid and produces the expected scaffold.

## Setup

- Run: `cmakegen init -o test_metadata.json` or `cmakegen -i -o test_metadata.json`
- Use a terminal with a real TTY (the wizard does not work with piped stdin)

## Flows to cover

### 1. Project screen

- [ ] Enter project name and version
- [ ] Proceed to next section (e.g. "Next section" or equivalent)

### 2. SOCs / Boards / Toolchains / ISA variants / Build variants

- [ ] Add at least one SOC (id, display name, description, ISAs)
- [ ] Add at least one board (id, display name, linked SOCs, optional defines)
- [ ] Add at least one toolchain (id, compiler paths, optional flags/libs)
- [ ] Add at least one ISA variant (id, toolchain, display name)
- [ ] Add at least one build variant (id, optional flags)
- [ ] Check that IDs and display names are shown and editable as expected

### 3. Components

- [ ] **Executable:** Add one executable with source path, dest path, and dependencies
- [ ] **Library:** Add one library; enable "Hierarchical layout?" and optionally "Configure path filters?" (include/exclude paths, filter mode)
- [ ] **Variant:** Add one variant component; add at least one variation with "Edit condition" — exercise AND/OR/NOT, sub-conditions, and "Default"
- [ ] **External:** Add one external component with Conan reference
- [ ] **Layer:** Add one layer with subdirs (component IDs)
- [ ] Where the UI offers "Configure path filters?" or "Use git source?", toggle them and set values (git URL, tag/branch/commit; include/exclude paths)

### 4. Dependencies

- [ ] If the UI exposes tool_requires / extra_requires, add at least one entry for each

### 5. Output screen

- [ ] Set output file path (e.g. `test_metadata.json`)
- [ ] Save and confirm the file is written
- [ ] Confirm the wizard exits successfully

## Correctness checks

After saving:

1. **Validate:** Run `cmakegen --validate-only test_metadata.json` and confirm no validation errors.
2. **Generate:** Run `cmakegen -o out test_metadata.json` and confirm:
   - No errors during generation
   - `out/CMakeLists.txt` exists
   - `out/CMakePresets.json` exists (or equivalent preset output)
   - Preset naming and component list match what you configured (e.g. preset name pattern, components in source_tree)

This gives a repeatable manual path to catch UI regressions (wrong field binding, missing sections, or invalid JSON).

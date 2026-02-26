# CMakeGen – CMake Project Generator for Bare-Metal Embedded Systems

A C++ tool that generates a complete CMake project from JSON or YAML metadata. It supports multi-SOC, multi-ISA heterogeneous cores, multiple toolchains, CMake Presets, and Conan dependencies. Ideal for embedded projects targeting ARM, RISC-V, or other bare-metal platforms.

---

## Table of Contents

- [Building CMakeGen](#building-cmakegen)
- [Running CMakeGen](#running-cmakegen)
- [Generated Output](#generated-output)
- [Metadata JSON Reference](#metadata-json-reference)
- [Component Types](#component-types)
- [Workflow Example](#workflow-example)
- [Tests](#tests)

---

## Building CMakeGen

### Prerequisites

- CMake 3.23 or later
- C++17 compiler (GCC, Clang, or MSVC)
- Ninja (recommended) or Make
- Git (required when using `git` source for components)

### Build with FetchContent (no package manager)

```bash
cmake -B build -S . -G Ninja
cmake --build build
```

The executable is produced at `build/cmakegen`.

### Build with CMake Presets

The project provides presets for Linux and Windows cross-compilation:

| Preset | Description | Output |
|--------|-------------|--------|
| `linux` | Native Linux build | `build/linux/cmakegen` |
| `windows` | Cross-compile to Windows .exe | `build/windows/cmakegen.exe` |

```bash
# Linux (native)
cmake --preset linux
cmake --build --preset linux

# Windows (cross-compile from Linux)
cmake --preset windows
cmake --build --preset windows
```

**Windows cross-build prerequisite:** Install MinGW-w64 on your Linux host:

```bash
# Debian/Ubuntu
sudo apt install gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64
```

The Windows executable is built with static linking for a standalone `.exe` with no DLL dependencies.

### Install

Install to a prefix (default `/usr/local`):

```bash
cmake --preset linux
cmake --build --preset linux
cmake --install build/linux --prefix /usr/local
```

Or install only the runtime (executable + templates, excludes test dependencies):

```bash
cmake --install build/linux --prefix /usr/local --component runtime
```

The installed binary finds templates via `../share/cmakegen/templates` relative to the executable. Override with `CMAKEGEN_TEMPLATES_DIR` environment variable.

### Package (CPack)

Create distributable packages:

```bash
cmake --preset linux
cmake --build --preset linux
cd build/linux && cpack -G TGZ -B ../packages -C Release
```

Package formats: `TGZ` (tar.gz), `ZIP`, `DEB`, `RPM` (when available). The package includes the executable and templates.

### Build with Conan (optional)

If you have a Conan toolchain in `build/conan_toolchain.cmake`, CMake will use it automatically:

```bash
conan install . --output-folder=build --build=missing
cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
cmake --build build
```

### Optional: YAML support

To use YAML metadata files (`.yaml` / `.yml`) in addition to JSON:

```bash
cmake -B build -S . -G Ninja -DCMAKEGEN_USE_YAML=ON
cmake --build build
```

---

## Running CMakeGen

### Basic usage

```bash
./build/cmakegen [options] <metadata-file>
```

### Options

| Option | Short | Description |
|--------|-------|-------------|
| `metadata` | `-m`, `--metadata` | Path to metadata file (JSON or YAML) |
| `--output` | `-o` | Output directory for generated project (default: `./output`) |
| `--validate-only` | — | Validate metadata without generating |
| `--dry-run` | — | Print actions without executing |
| `--default-json` | — | Generate default JSON template. Optional path: write to file; else stdout |

### Examples

```bash
# Generate a default metadata template (to stdout)
./build/cmakegen --default-json

# Generate template to a file
./build/cmakegen --default-json metadata.json

# Generate a project into ./my_embedded_project
./build/cmakegen -o ./my_embedded_project metadata.json

# Validate metadata only
./build/cmakegen --validate-only metadata.json

# Dry run to preview actions
./build/cmakegen --dry-run -o ./out metadata.json
```

### Path resolution

- **Source paths** in the metadata are resolved relative to the **metadata file’s directory**.
- Use relative paths (e.g. `dummy_sources`, `../vendor/freertos`) or absolute paths.
- **Git sources**: Instead of `source`, use `git` to specify a repository URL. CMakeGen clones the repo into `.cmakegen_cache/<component_id>` and generates from there. Use `tag`, `branch`, or `commit` to pin a specific ref (default: `main`).
- **Environment variables**: Use `${VAR}` in any string value to expand variables. Lookup order: 1) `env` object in JSON, 2) system environment. Variables can reference others (e.g. `X: "${Y}_${Z}"`). The `preset_matrix` section is excluded (it uses `${preset}`, `${board}`, etc. as CMake preset placeholders).

---

## Generated Output

CMakeGen produces:

| Output | Description |
|--------|-------------|
| `CMakeLists.txt` | Root project file with `add_subdirectory` for top-level layers |
| `conanfile.txt` | Conan requires (external components + tool_requires) |
| `CMakePresets.json` | Configure and build presets for each combination |
| `toolchains/` | Toolchain `.cmake` files |
| `.cmakegen_cache/` | Temporary cache for git clones (created during generation, removed when complete) |

**Toolchain files:** When `build_variants` is defined, one file per `(toolchain_id, build_variant_id)` is generated (e.g. `arm-gcc-m7-debug.cmake`, `arm-gcc-m7-release.cmake`). Each preset uses the matching toolchain file. When `build_variants` is empty, toolchain files are named `{toolchain_id}.cmake` only.

---

## Metadata JSON Reference

The metadata file describes your embedded project: SOCs, boards, toolchains, and the software component tree. Use `source` for local paths (relative to the metadata file directory) or `git` for repository URLs.

### Top-level `env` (optional)

User-defined variables for `${VAR}` expansion. Keys are variable names; values can reference other variables.

| Lookup order | Source |
|--------------|--------|
| 1 | `env` object in JSON |
| 2 | System environment |

If a variable is not found in either, cmakegen reports an error.

**Example:**
```json
"env": {
  "VENDOR": "acme",
  "PLATFORM": "${VENDOR}_stm32",
  "SRC_ROOT": "/opt/${VENDOR}/sources"
}
```

Then use `${PLATFORM}`, `${SRC_ROOT}`, etc. in any string fields (except `preset_matrix`).

### Top-level structure

```json
{
  "schema_version": 1,
  "project": { ... },
  "socs": [ ... ],
  "boards": [ ... ],
  "toolchains": [ ... ],
  "isa_variants": [ ... ],
  "build_variants": [ ... ],
  "source_tree": { "components": [ ... ] },
  "dependencies": { ... },
  "preset_matrix": { ... }
}
```

**Required:** `project`, `source_tree`, `preset_matrix`

---

### `project`

Basic project identification and CMake version.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Project name (used in `project(...)`) |
| `version` | string | Yes | Project version |
| `cmake_minimum` | object | No | Minimum CMake version (default: 3.23) |
| `cmake_minimum.major` | int | No | Major version |
| `cmake_minimum.minor` | int | No | Minor version |
| `cmake_minimum.patch` | int | No | Patch version |

**Example:**
```json
"project": {
  "name": "my_firmware",
  "version": "1.0.0",
  "cmake_minimum": { "major": 3, "minor": 23, "patch": 0 }
}
```

---

### `socs`

List of System-on-Chip (SoC) definitions. Each SOC can have multiple ISA cores (e.g. Cortex-M7 + Cortex-M4).

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Unique SOC identifier (used in boards and conditions) |
| `display_name` | string | No | Human-readable name |
| `description` | string | No | Short description |
| `isas` | array | No | List of distinct ISA ids in this SOC (e.g. `["cortex-m7"]` or `["cortex-m7", "cortex-m4"]`). Maps to toolchain via `isa_variants`. |

**Example:**
```json
"socs": [
  {
    "id": "stm32h7",
    "display_name": "STM32H7",
    "description": "Cortex-M7",
    "isas": ["cortex-m7"]
  },
  {
    "id": "gd32vf103",
    "display_name": "GD32VF103",
    "isas": ["riscv32"]
  }
]
```

---

### `boards`

Physical boards. A board can use one or more SOCs (e.g. dual-core carrier).

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Unique board identifier |
| `socs` | array | Yes | List of SOC ids used on this board |
| `display_name` | string | No | Human-readable name |
| `defines` | array | No | Preprocessor defines for this board |

**Example:**
```json
"boards": [
  {
    "id": "nucleo-h743zi",
    "display_name": "Nucleo-H743ZI",
    "socs": ["stm32h7"],
    "defines": ["BOARD_NUCLEO_H743"]
  },
  {
    "id": "dual-carrier",
    "socs": ["stm32h7", "stm32g4"],
    "defines": ["BOARD_DUAL_SOC"]
  }
]
```

---

### `toolchains`

Cross-compiler toolchain definitions. Each toolchain maps to one or more ISA variants. Toolchain base flags are merged with build variant flags (add/remove) when generating per-preset toolchain files.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Unique toolchain identifier |
| `compiler` | object | No | Compiler executables |
| `compiler.c` | string | No | C compiler (e.g. `arm-none-eabi-gcc`) |
| `compiler.cxx` | string | No | C++ compiler |
| `compiler.asm` | string | No | Assembler (often same as C) |
| `flags` | object | No | Compiler/linker flags per type |
| `flags.c` | array | No | C flags |
| `flags.cxx` | array | No | C++ flags |
| `flags.asm` | array | No | ASM flags |
| `flags.linker` | array | No | Linker flags |
| `libs` | array | No | Libraries to link (e.g. `["c", "nosys"]`) |
| `lib_paths` | array | No | Library search paths |
| `defines` | array | No | Preprocessor defines |
| `sysroot` | string | No | Sysroot path |
| `display_name` | string | No | Human-readable name |

**Example:**
```json
"toolchains": [
  {
    "id": "arm-gcc-m7",
    "display_name": "ARM GCC Cortex-M7",
    "compiler": {
      "c": "arm-none-eabi-gcc",
      "cxx": "arm-none-eabi-g++",
      "asm": "arm-none-eabi-gcc"
    },
    "flags": {
      "c": ["-mcpu=cortex-m7", "-mthumb"],
      "cxx": ["-mcpu=cortex-m7", "-mthumb"],
      "linker": ["-mcpu=cortex-m7", "-mthumb", "-specs=nano.specs"]
    },
    "libs": ["c", "nosys"],
    "defines": ["ARM_MATH_CM7"],
    "sysroot": "/opt/arm-none-eabi"
  }
]
```

---

### `isa_variants`

Maps ISA identifiers to toolchains. Used to select the correct toolchain for each board/SOC combination.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | ISA identifier (must match `socs[].isas[]`) |
| `toolchain` | string | Yes | Toolchain id |
| `display_name` | string | No | Human-readable name |

**Example:**
```json
"isa_variants": [
  { "id": "cortex-m7", "toolchain": "arm-gcc-m7", "display_name": "Cortex-M7" },
  { "id": "riscv32", "toolchain": "riscv-gcc", "display_name": "RISC-V32" }
]
```

---

### `build_variants`

Build configurations (e.g. debug vs release). Each build variant is applied per toolchain: cmakegen generates one toolchain file per `(toolchain_id, build_variant_id)` pair (e.g. `arm-gcc-m7-debug.cmake`, `arm-gcc-m7-release.cmake`). Presets select the appropriate toolchain file.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Variant identifier |
| `flags` | object | No | Flags to add (default for all toolchains) |
| `flags.c` | array | No | C flags to add |
| `flags.cxx` | array | No | C++ flags to add |
| `flags.asm` | array | No | ASM flags to add |
| `flags.linker` | array | No | Linker flags to add |
| `remove_flags` | object | No | Flags to remove from the toolchain base |
| `remove_flags.c` | array | No | C flags to remove (e.g. `["-O3"]` to drop optimization) |
| `add_flags` | object | No | Per-toolchain overrides; key = toolchain id |
| `add_flags.<tc_id>.c` | array | No | C flags for this toolchain (replaces `flags.c`) |
| `inherits` | string | No | Inherit from another variant |

**Flag merge order:** `(toolchain_base - remove_flags) + (add_flags[tc_id] or flags)`

**Example:**
```json
"build_variants": [
  {
    "id": "debug",
    "flags": { "c": ["-g", "-O0"], "cxx": ["-g", "-O0"] },
    "add_flags": {
      "riscv-gcc": { "c": ["-g", "-Og"], "cxx": ["-g", "-Og"] }
    }
  },
  {
    "id": "release",
    "flags": { "c": ["-O3", "-DNDEBUG"], "cxx": ["-O3", "-DNDEBUG"] },
    "remove_flags": { "c": ["-Os"], "cxx": ["-Os"] }
  }
]
```

Use `remove_flags` to strip flags from the toolchain base (e.g. remove `-Os` when the toolchain has it by default). If the toolchain has no matching flags, `remove_flags` has no effect.

---

### `source_tree.components`

The software component tree. Each component has an `id`, a `type`, and type-specific fields.

#### Common fields (all components)

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Unique component identifier |
| `type` | string | One of: `executable`, `library`, `external`, `variant`, `layer` |
| `source` | string | Path to local source directory (relative to metadata file) |
| `git` | object | Git repository source (alternative to `source`). See [Git sources](#git-sources). |
| `dependencies` | array | Component ids this component depends on |
| `dest` | string | Output path relative to project root (e.g. `platform/drivers/hal`) |

#### Git sources

For executable and library components, you can use `git` instead of `source` to fetch sources from a Git repository. CMakeGen clones the repo into `.cmakegen_cache/<component_id>` and then generates as if it were a local folder.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `url` | string | Yes | Git repository URL (e.g. `https://github.com/FreeRTOS/FreeRTOS-Kernel.git`) |
| `tag` | string | No | Tag to checkout (shallow clone) |
| `branch` | string | No | Branch to checkout (shallow clone). Default: `main` if none specified |
| `commit` | string | No | Commit SHA to checkout (full clone) |

**Example:**
```json
{
  "id": "freertos_kernel",
  "type": "library",
  "library_type": "static",
  "structure": "hierarchical",
  "git": {
    "url": "https://github.com/FreeRTOS/FreeRTOS-Kernel.git",
    "tag": "V11.0.1"
  },
  "dest": "platform/rtos/freertos_kernel",
  "source_extensions": ["*.c"],
  "include_extensions": ["*.h"],
  "dependencies": ["hal"]
}
```

---

### `dependencies`

Conan and tool requirements for the generated project.

| Field | Type | Description |
|-------|------|-------------|
| `tool_requires` | array | Conan tool requirements (e.g. `["cmake/3.28.0"]`) |
| `extra_requires` | array | Additional Conan requirements |

**Example:**
```json
"dependencies": {
  "tool_requires": ["cmake/3.28.0"],
  "extra_requires": []
}
```

---

### `preset_matrix`

Defines how CMake Presets are generated from board × SOC × ISA × build variant.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `dimensions` | array | Yes | Dimensions to expand (e.g. `["board", "soc", "isa_variant", "build_variant"]`) |
| `naming` | string | Yes | Preset name pattern, e.g. `"{board}_{soc}_{isa}_{variant}"` |
| `binary_dir_pattern` | string | Yes | Build dir pattern, e.g. `"build/${preset}"` |
| `exclude` | array | No | Exclude specific combinations |
| `exclude[].board` | string | No | Exclude presets for this board |
| `exclude[].soc` | string | No | Exclude presets for this SOC |
| `exclude[].isa_variant` | string | No | Exclude presets for this ISA variant |
| `exclude[].build_variant` | string | No | Exclude presets for this build variant |

**Example:**
```json
"preset_matrix": {
  "dimensions": ["board", "soc", "isa_variant", "build_variant"],
  "exclude": [
    { "board": "nucleo-h743zi", "isa_variant": "cortex-m4" }
  ],
  "naming": "{board}_{soc}_{isa}_{variant}",
  "binary_dir_pattern": "build/${preset}"
}
```

---

## Component Types

### `executable`

Produces an executable target. Source files are copied and globbed.

| Field | Required | Description |
|-------|----------|-------------|
| `source` or `git` | Yes | Path to source directory, or git repository (see [Git sources](#git-sources)) |
| `dest` | Yes | Output directory |
| `source_extensions` | No | Glob patterns (default: `["*.c", "*.cpp", "*.cc"]`) |
| `include_extensions` | No | Include file patterns |
| `dependencies` | No | Linked libraries/components |

---

### `library`

Produces a static, shared, or interface library.

| Field | Required | Description |
|-------|----------|-------------|
| `source` or `git` | Yes | Path to source directory, or git repository (see [Git sources](#git-sources)) |
| `dest` | Yes | Output directory |
| `library_type` | No | `static`, `shared`, or `interface` (default: `static`) |
| `structure` | No | `hierarchical` for tree copy with filters |
| `source_extensions` | No | Glob patterns |
| `include_extensions` | No | Include patterns |
| `filters` | No | Copy filters |
| `filters.include_paths` | No | Glob patterns to include |
| `filters.exclude_paths` | No | Glob patterns to exclude |
| `dependencies` | No | Linked libraries |

**Hierarchical libraries** (`structure: "hierarchical"`): Copy an external tree (e.g. FreeRTOS-Kernel) with filters, preserving directory structure. Use `include_paths` and `exclude_paths` to control what is copied.

---

### `external`

Conan package dependency. No source copy; only adds to `conanfile.txt`.

| Field | Required | Description |
|-------|----------|-------------|
| `conan_ref` | Yes | Conan reference (e.g. `lwip/2.1.2`) |

---

### `variant`

Conditional subdirectory selection. Copies a source tree that contains multiple implementations (e.g. `stm32_uart/`, `generic_uart/`). The chosen subdir is selected at CMake configure time via `SOC`, `BOARD`, etc.

| Field | Required | Description |
|-------|----------|-------------|
| `source` | Yes | Path to directory containing variant subdirs |
| `dest` | Yes | Output directory |
| `variations` | Yes | List of `{ subdir, condition }` |
| `variations[].subdir` | Yes | Subdirectory name |
| `variations[].condition` | Yes | Condition object (see below) |
| `dependencies` | No | Dependencies |

**Condition format:**

- `{ "default": true }` — fallback when no other condition matches
- `{ "var": "SOC", "op": "equals", "value": "stm32h7" }`
- `{ "var": "SOC", "op": "in", "value": ["stm32h7", "stm32g4"] }`
- `{ "var": "SOC", "op": "not_in", "value": ["riscv32"] }`

Available variables: `SOC`, `BOARD`, `ISA_VARIANT`, `BUILD_VARIANT` (from CMake Presets).

---

### `layer`

Aggregates other components into a directory. Does not copy sources; only creates a `CMakeLists.txt` that adds subdirectories.

| Field | Required | Description |
|-------|----------|-------------|
| `dest` | Yes | Output directory |
| `subdirs` | Yes | Component ids to include (in order) |

---

## Workflow Example

1. **Create metadata** — Define project, SOCs, boards, toolchains, and components in `metadata.json`.

2. **Validate** — Check metadata before generating:
   ```bash
   ./build/cmakegen --validate-only metadata.json
   ```

3. **Generate** — Create the project:
   ```bash
   ./build/cmakegen -o ./my_project metadata.json
   ```

4. **Build generated project** — Use CMake Presets:
   ```bash
   cd my_project
   cmake --preset nucleo-h743zi_stm32h7_cortex-m7_debug
   cmake --build build/nucleo-h743zi_stm32h7_cortex-m7_debug
   ```

5. **Conan (if used)** — Install dependencies first:
   ```bash
   cd my_project
   conan install . --output-folder=build
   cmake --preset <preset_name>
   ```

---

## Tests

```bash
ctest --test-dir build
```

Or run a specific test:

```bash
ctest --test-dir build -R E2ETest -V
```

---

## Schema

The full JSON schema is in `schema/scaffold-schema.json`. Use it for validation or editor support.

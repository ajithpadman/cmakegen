#pragma once

#include "metadata/schema.hpp"
#include <filesystem>
#include <string>

namespace scaffolder {

/** Run add-project wizard and write project.json to folder. Returns true on success. */
bool run_add_project(const std::filesystem::path& folder);
/** Run add-soc wizard and append to socs.json. */
bool run_add_soc(const std::filesystem::path& folder);
/** Run add-board wizard and append to boards.json. */
bool run_add_board(const std::filesystem::path& folder);
/** Run add-toolchain wizard and append to toolchains.json. */
bool run_add_toolchain(const std::filesystem::path& folder);
/** Run add-isa-variant wizard and append to isa_variants.json. */
bool run_add_isa_variant(const std::filesystem::path& folder);
/** Run add-build-variant wizard and append to build_variants.json. */
bool run_add_build_variant(const std::filesystem::path& folder);
/** Run add-component wizard and append to components.json. */
bool run_add_component(const std::filesystem::path& folder);
/** Run add-conanfile wizard and write conanfile.json. */
bool run_add_conanfile(const std::filesystem::path& folder);
/** Run add-cmake-preset wizard and write cmake_preset.json. */
bool run_add_cmake_preset(const std::filesystem::path& folder);

}  // namespace scaffolder

#pragma once

#include "metadata/schema.hpp"
#include <filesystem>

namespace scaffolder {

/** Writes entity JSON files for add commands (project.json, socs.json, etc.). */
class ConfigWriter {
public:
    /** Write project.json (overwrites). */
    void write_project(const std::filesystem::path& folder, const Project& p) const;
    /** Append one SOC to socs.json (create file or append). */
    void append_soc(const std::filesystem::path& folder, const Soc& s) const;
    /** Append one Board to boards.json. */
    void append_board(const std::filesystem::path& folder, const Board& b) const;
    /** Append one Toolchain to toolchains.json (or write toolchains/<id>.json). */
    void append_toolchain(const std::filesystem::path& folder, const Toolchain& t) const;
    /** Append one IsaVariant to isa_variants.json. */
    void append_isa_variant(const std::filesystem::path& folder, const IsaVariant& iv) const;
    /** Append one BuildVariant to build_variants.json. */
    void append_build_variant(const std::filesystem::path& folder, const BuildVariant& bv) const;
    /** Append one component to components.json (or write components/<id>.json). */
    void append_component(const std::filesystem::path& folder, const SwComponent& c) const;
    /** Write cmake_preset.json (preset_matrix). */
    void write_preset_matrix(const std::filesystem::path& folder, const PresetMatrix& pm) const;
    /** Write conanfile.json (dependencies: tool_requires, extra_requires). */
    void write_dependencies(const std::filesystem::path& folder, const Dependencies& d) const;
};

}  // namespace scaffolder

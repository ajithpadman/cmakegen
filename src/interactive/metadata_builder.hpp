#pragma once

#include <optional>
#include <string>
#include <vector>
#include <map>

namespace scaffolder {

// Normalize a display name to a valid id: trim, replace spaces with '_',
// strip invalid chars to [a-zA-Z0-9_-], collapse consecutive '_'.
std::string normalize_id(const std::string& name);

// Split comma-separated string into trimmed non-empty tokens.
std::vector<std::string> split_comma_separated(const std::string& s);

// Data collected from interactive prompts (mirrors schema).
struct ProjectData {
    std::string name;
    std::string version = "0.1.0";
    int cmake_major = 3, cmake_minor = 23, cmake_patch = 0;
};

struct SocData {
    std::string id;
    std::string display_name;
    std::string description;
    std::vector<std::string> isas;
};

struct BoardData {
    std::string id;
    std::string display_name;
    std::vector<std::string> socs;  // SOC ids
    std::vector<std::string> defines;
};

struct CompilerData {
    std::string c;
    std::string cxx;
    std::string asm_;
};

struct ToolchainData {
    std::string id;
    std::string display_name;
    CompilerData compiler;
    std::map<std::string, std::vector<std::string>> flags;  // c, cxx, asm, linker
    std::vector<std::string> libs;
    std::vector<std::string> lib_paths;
    std::vector<std::string> defines;
    std::string sysroot;
};

struct IsaVariantData {
    std::string id;
    std::string toolchain;
    std::string display_name;
};

struct BuildVariantData {
    std::string id;
    std::map<std::string, std::vector<std::string>> flags;  // c, cxx, etc.
};

struct PathFiltersData {
    std::string filter_mode = "include_first";  // include_first | exclude_first
    std::vector<std::string> include_paths;
    std::vector<std::string> exclude_paths;
};

struct VariationData {
    std::string subdir;
    std::string cond_var;   // e.g. SOC
    std::string cond_op;   // in | equals
    std::vector<std::string> cond_value;
    bool is_default = false;
};

struct ComponentData {
    std::string id;
    std::string type;  // executable | library | external | variant | layer
    std::string library_type = "static";  // for library
    std::string structure;   // hierarchical or empty
    std::string source;
    std::string dest;
    std::vector<std::string> dependencies;
    std::string conan_ref;  // for external
    std::vector<std::string> subdirs;  // for layer (component ids)
    std::vector<std::string> source_extensions = {"*.c", "*.cpp"};
    std::vector<std::string> include_extensions = {"*.h", "*.hpp"};
    std::optional<PathFiltersData> filters;
    std::string git_url;
    std::string git_tag;
    std::string git_branch;
    std::string git_commit;
    std::vector<VariationData> variations;  // for variant
};

struct DependenciesData {
    std::vector<std::string> tool_requires;
    std::vector<std::string> extra_requires;
};

// Builds the full metadata JSON from collected data.
class MetadataBuilder {
public:
    ProjectData project;
    std::vector<SocData> socs;
    std::vector<BoardData> boards;
    std::vector<ToolchainData> toolchains;
    std::vector<IsaVariantData> isa_variants;
    std::vector<BuildVariantData> build_variants;
    std::vector<ComponentData> components;
    DependenciesData dependencies;

    // Preset matrix is derived; call after collecting boards/socs/isa/build_variants.
    void set_preset_matrix_defaults();

    // Produces schema-valid JSON string.
    std::string to_json() const;
};

}  // namespace scaffolder

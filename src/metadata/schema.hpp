#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <map>
#include <memory>

namespace scaffolder {

struct CmakeVersion {
    int major = 3;
    int minor = 23;
    int patch = 0;
};

struct Project {
    std::string name;
    std::string version;
    CmakeVersion cmake_minimum;
};

struct Soc {
    std::string id;
    std::string display_name;
    std::string description;
    std::vector<std::string> isas;
};

struct Board {
    std::string id;
    std::string display_name;
    std::vector<std::string> socs;
    std::vector<std::string> defines;
};

struct Compiler {
    std::string c;
    std::string cxx;
    std::string asm_;
};

struct Toolchain {
    std::string id;
    std::string display_name;
    Compiler compiler;
    std::map<std::string, std::vector<std::string>> flags;
    std::vector<std::string> libs;
    std::vector<std::string> lib_paths;
    std::vector<std::string> defines;
    std::string sysroot;
};

struct IsaVariant {
    std::string id;
    std::string toolchain;
    std::string display_name;
};

struct BuildVariant {
    std::string id;
    std::optional<std::string> inherits;
    std::map<std::string, std::vector<std::string>> flags;
    std::map<std::string, std::vector<std::string>> remove_flags;
    std::map<std::string, std::map<std::string, std::vector<std::string>>> add_flags;
};

struct GitSource {
    std::string url;
    std::optional<std::string> tag;
    std::optional<std::string> branch;
    std::optional<std::string> commit;
};

enum class FilterMode {
    IncludeFirst,   // Include then exclude: path in if (matches_include) AND (not matches_exclude)
    ExcludeFirst    // Exclude then include: path in if (not matches_exclude) OR (matches_include)
};

struct PathFilters {
    std::vector<std::string> exclude_paths;
    std::vector<std::string> include_paths;
    FilterMode filter_mode = FilterMode::IncludeFirst;
};

// Condition for variant components (recursive)
struct Condition {
    std::optional<std::string> var;
    std::optional<std::string> op;
    std::optional<std::variant<std::string, std::vector<std::string>>> value;
    std::optional<std::vector<std::shared_ptr<Condition>>> and_;
    std::optional<std::vector<std::shared_ptr<Condition>>> or_;
    std::optional<std::shared_ptr<Condition>> not_;
    std::optional<bool> default_;
};

struct Variation {
    std::string subdir;
    Condition condition;
};

struct SwComponent {
    std::string id;
    std::string type;  // executable, library, external, variant, layer
    std::optional<std::string> library_type;  // interface, static, shared
    std::optional<std::string> structure;  // hierarchical
    std::optional<std::string> source;  // local path (relative to metadata file)
    std::optional<GitSource> git;  // when set, clone repo and use as source instead of local path
    std::optional<std::string> dest;
    std::optional<Condition> condition;  // build only when condition matches (executable, library, layer)
    std::optional<PathFilters> filters;
    std::optional<std::vector<std::string>> source_extensions;
    std::optional<std::vector<std::string>> include_extensions;
    std::optional<std::vector<std::string>> metadata_extensions;
    std::optional<std::vector<std::string>> dependencies;
    std::optional<std::string> conan_ref;
    std::optional<std::vector<Variation>> variations;
    std::optional<std::vector<std::string>> subdirs;
};

struct SourceTree {
    std::vector<SwComponent> components;
};

struct Dependencies {
    std::vector<std::string> tool_requires;
    std::vector<std::string> extra_requires;
};

struct PresetExclude {
    std::optional<std::string> board;
    std::optional<std::string> soc;
    std::optional<std::string> isa_variant;
    std::optional<std::string> build_variant;
};

struct PresetMatrix {
    std::vector<std::string> dimensions;
    std::vector<PresetExclude> exclude;
    std::string naming;
    std::string binary_dir_pattern;
};

struct Metadata {
    int schema_version = 1;
    std::map<std::string, std::string> env;  // user-defined vars (expanded with ${VAR})
    Project project;
    std::vector<Soc> socs;
    std::vector<Board> boards;
    std::vector<Toolchain> toolchains;
    std::vector<IsaVariant> isa_variants;
    std::vector<BuildVariant> build_variants;
    SourceTree source_tree;
    Dependencies dependencies;
    PresetMatrix preset_matrix;
};

}  // namespace scaffolder

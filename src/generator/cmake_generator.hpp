#pragma once

#include "../metadata/schema.hpp"
#include "condition_evaluator.hpp"
#include "../resolver/path_resolver.hpp"
#include <filesystem>
#include <string>

namespace scaffolder {

class CmakeGenerator {
public:
    CmakeGenerator(const Metadata& metadata, PathResolver& resolver, const std::filesystem::path& output_root);
    void generate_all();

private:
    void generate_root_cmakelists();
    void generate_cmake_helpers();
    void generate_component_cmakelists(const SwComponent& comp);
    void generate_library(const SwComponent& comp, const std::filesystem::path& dest);
    void generate_hierarchical_library(const SwComponent& comp, const std::filesystem::path& dest);
    void generate_executable(const SwComponent& comp, const std::filesystem::path& dest);
    void generate_variant(const SwComponent& comp, const std::filesystem::path& dest);
    void generate_layer(const SwComponent& comp, const std::filesystem::path& dest);
    std::string collect_sources(const std::filesystem::path& dir, const std::vector<std::string>& exts);
    std::vector<std::filesystem::path> collect_include_dirs(const std::filesystem::path& dir, const std::vector<std::string>& exts);

    const Metadata& metadata_;
    PathResolver& resolver_;
    std::filesystem::path output_root_;
    ConditionEvaluator cond_eval_;
};

}  // namespace scaffolder

#pragma once

#include "../metadata/schema.hpp"
#include "filter.hpp"
#include "../resolver/path_resolver.hpp"
#include <filesystem>

namespace scaffolder {

class CopyEngine {
public:
    CopyEngine(PathResolver& resolver, const std::filesystem::path& output_root);
    void copy_component(const SwComponent& comp);

private:
    void copy_file(const std::filesystem::path& src, const std::filesystem::path& dest);
    void copy_tree(const SwComponent& comp, const std::filesystem::path& src, const std::filesystem::path& dest);
    PathResolver& resolver_;
    std::filesystem::path output_root_;
};

}  // namespace scaffolder

#pragma once

#include "../metadata/schema.hpp"
#include <filesystem>
#include <map>
#include <string>

namespace scaffolder {

class PathResolver {
public:
    explicit PathResolver(const std::filesystem::path& base_dir);
    std::filesystem::path resolve(const std::string& path) const;
    std::filesystem::path resolve_source(const SwComponent& comp) const;
    std::filesystem::path resolve_dest(const SwComponent& comp, const std::filesystem::path& output_root) const;

    void set_resolved_source(const std::string& comp_id, const std::filesystem::path& path);

private:
    std::filesystem::path base_dir_;
    std::map<std::string, std::filesystem::path> resolved_sources_;
};

}  // namespace scaffolder

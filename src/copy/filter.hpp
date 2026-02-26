#pragma once

#include "../metadata/schema.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace scaffolder {

class Filter {
public:
    explicit Filter(const PathFilters& filters);
    bool should_include(const std::filesystem::path& path, const std::filesystem::path& base) const;
    bool matches_extension(const std::filesystem::path& path, const std::vector<std::string>& extensions) const;

private:
    bool matches_glob(const std::string& pattern, const std::string& path_str) const;
    PathFilters filters_;
};

}  // namespace scaffolder

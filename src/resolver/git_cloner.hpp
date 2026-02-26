#pragma once

#include "../metadata/schema.hpp"
#include <filesystem>
#include <string>

namespace scaffolder {

class GitCloner {
public:
    explicit GitCloner(const std::filesystem::path& cache_dir);
    std::filesystem::path clone(const GitSource& git, const std::string& component_id);

private:
    std::string get_ref_spec(const GitSource& git) const;
    std::filesystem::path cache_dir_;
};

}  // namespace scaffolder

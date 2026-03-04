#pragma once

#include "metadata/schema.hpp"
#include <filesystem>
#include <stdexcept>
#include <string>

namespace scaffolder {

class ConfigLoadError : public std::runtime_error {
public:
    explicit ConfigLoadError(const std::string& msg) : std::runtime_error(msg) {}
};

/** Loads Metadata from a folder of JSON files (project.json, socs.json, etc.). */
class ConfigLoader {
public:
    /** Load and merge all JSON fragments from the folder, then parse into Metadata. */
    Metadata load(const std::filesystem::path& folder) const;

    /** Optional: set base path for resolving relative paths in env (default: folder). */
    void set_base_path(std::filesystem::path path) { base_path_ = std::move(path); }

private:
    std::filesystem::path base_path_;
};

}  // namespace scaffolder

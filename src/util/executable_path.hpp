#pragma once

#include <filesystem>

namespace scaffolder {

// Returns the directory containing the running executable, or empty path on failure.
std::filesystem::path get_executable_directory();

}  // namespace scaffolder

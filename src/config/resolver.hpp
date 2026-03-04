#pragma once

#include "metadata/schema.hpp"
#include <stdexcept>
#include <string>
#include <vector>

namespace scaffolder {

class ResolveError : public std::runtime_error {
public:
    explicit ResolveError(const std::string& msg) : std::runtime_error(msg) {}
};

/** Runs linking/cross-resolution: validates references between entities. Throws ResolveError if invalid. */
void resolve(const Metadata& metadata);

/** Collect all error messages without throwing. Returns empty if valid. */
std::vector<std::string> resolve_errors(const Metadata& metadata);

}  // namespace scaffolder

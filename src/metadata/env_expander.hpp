#pragma once

#include <map>
#include <set>
#include <string>
#include <stdexcept>

namespace scaffolder {

class EnvExpandError : public std::runtime_error {
public:
    explicit EnvExpandError(const std::string& msg) : std::runtime_error(msg) {}
};

// Resolves ${VAR} in strings. Lookup order: 1) env map (from JSON), 2) system env.
// Supports recursive expansion: X="${Y}_${Z}" where Y and Z are from env or system.
// Throws EnvExpandError if a variable is not found.
class EnvExpander {
public:
    explicit EnvExpander(std::map<std::string, std::string> env_from_json = {});

    std::string expand(const std::string& value) const;

private:
    std::string get_var(const std::string& name) const;
    std::string expand_recursive(const std::string& value, std::set<std::string>& expanding) const;

    std::map<std::string, std::string> env_;
};

}  // namespace scaffolder

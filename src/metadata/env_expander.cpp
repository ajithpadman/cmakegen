#include "metadata/env_expander.hpp"
#include <cstdlib>

namespace scaffolder {

EnvExpander::EnvExpander(std::map<std::string, std::string> env_from_json) : env_(std::move(env_from_json)) {}

std::string EnvExpander::get_var(const std::string& name) const {
    auto it = env_.find(name);
    if (it != env_.end()) return it->second;
    const char* sys = std::getenv(name.c_str());
    if (sys) return std::string(sys);
    throw EnvExpandError("Environment variable '" + name + "' not found (not in env nor system)");
}

std::string EnvExpander::expand_recursive(const std::string& value, std::set<std::string>& expanding) const {
    std::string result;
    result.reserve(value.size());
    size_t i = 0;
    while (i < value.size()) {
        if (value[i] == '$' && i + 1 < value.size() && value[i + 1] == '{') {
            size_t end = value.find('}', i + 2);
            if (end == std::string::npos) {
                throw EnvExpandError("Unclosed ${...} in: " + value);
            }
            std::string var_name = value.substr(i + 2, end - i - 2);
            if (var_name.empty()) {
                throw EnvExpandError("Empty variable name in ${}");
            }
            if (expanding.count(var_name)) {
                throw EnvExpandError("Circular reference in environment variable: " + var_name);
            }
            expanding.insert(var_name);
            std::string var_value = get_var(var_name);
            std::string expanded_value = expand_recursive(var_value, expanding);
            expanding.erase(var_name);
            result += expanded_value;
            i = end + 1;
        } else {
            result += value[i++];
        }
    }
    return result;
}

std::string EnvExpander::expand(const std::string& value) const {
    if (value.find("${") == std::string::npos) return value;
    std::set<std::string> expanding;
    return expand_recursive(value, expanding);
}

}  // namespace scaffolder

#pragma once

#include "schema.hpp"
#include <stdexcept>
#include <string>
#include <vector>

namespace scaffolder {

class ValidationError : public std::runtime_error {
public:
    explicit ValidationError(const std::string& msg) : std::runtime_error(msg) {}
};

class Validator {
public:
    void validate(const Metadata& metadata);
    std::vector<std::string> errors() const { return errors_; }

private:
    void validate_project(const Project& p);
    void validate_components(const std::vector<SwComponent>& components);
    void validate_preset_matrix(const PresetMatrix& pm);
    std::vector<std::string> errors_;
};

}  // namespace scaffolder

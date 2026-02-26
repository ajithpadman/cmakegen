#pragma once

#include "schema.hpp"
#include <filesystem>
#include <stdexcept>
#include <nlohmann/json_fwd.hpp>

namespace scaffolder {

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

class Parser {
public:
    Metadata parse_file(const std::filesystem::path& path);
    Metadata parse_string(const std::string& content, const std::string& format = "json");

private:
    void parse_from_json(const nlohmann::json& j);
    void parse_condition(const nlohmann::json& j, Condition& cond);
    Metadata metadata_;
};

}  // namespace scaffolder

#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>

namespace scaffolder {

class TemplateEngine {
public:
    TemplateEngine();
    std::string render(const std::string& template_name, const nlohmann::json& data) const;
    void render_to_file(const std::string& template_name, const nlohmann::json& data,
                        const std::filesystem::path& output_path) const;

private:
    std::filesystem::path templates_dir_;
    std::string load_template(const std::string& name) const;
};

}  // namespace scaffolder

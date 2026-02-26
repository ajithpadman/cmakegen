#include "generator/template_engine.hpp"
#include "util/executable_path.hpp"
#include <inja/inja.hpp>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace scaffolder {

static std::filesystem::path find_templates_dir() {
    if (const char* env = std::getenv("CMAKEGEN_TEMPLATES_DIR")) {
        std::filesystem::path p(env);
        if (std::filesystem::exists(p)) return p;
    }
#ifdef CMAKEGEN_TEMPLATES_DIR
    {
        std::filesystem::path p(CMAKEGEN_TEMPLATES_DIR);
        if (std::filesystem::exists(p)) return p;
    }
#endif
    auto exe_dir = get_executable_directory();
    if (!exe_dir.empty()) {
        auto installed = exe_dir / ".." / "share" / "cmakegen" / "templates";
        if (std::filesystem::exists(installed)) return std::filesystem::canonical(installed);
    }
    if (std::filesystem::exists("templates")) return std::filesystem::absolute("templates");
    return "templates";
}

TemplateEngine::TemplateEngine() : templates_dir_(find_templates_dir()) {}

std::string TemplateEngine::load_template(const std::string& name) const {
    std::filesystem::path p = templates_dir_ / name;
    std::ifstream f(p);
    if (!f) {
        throw std::runtime_error("Cannot load template: " + p.string());
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string TemplateEngine::render(const std::string& template_name, const nlohmann::json& data) const {
    std::string tmpl = load_template(template_name);
    try {
        return inja::render(tmpl, data);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Template '") + template_name + "': " + e.what());
    }
}

void TemplateEngine::render_to_file(const std::string& template_name, const nlohmann::json& data,
                                    const std::filesystem::path& output_path) const {
    std::string result = render(template_name, data);
    std::filesystem::create_directories(output_path.parent_path());
    std::ofstream f(output_path);
    f << result;
}

}  // namespace scaffolder

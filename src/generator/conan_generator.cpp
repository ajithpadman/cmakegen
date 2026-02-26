#include "generator/conan_generator.hpp"
#include "generator/template_engine.hpp"
#include <nlohmann/json.hpp>

namespace scaffolder {

ConanGenerator::ConanGenerator(const Metadata& metadata) : metadata_(metadata) {}

void ConanGenerator::generate(const std::filesystem::path& output_root) {
    nlohmann::json data;
    data["conan_requires"] = nlohmann::json::array();
    for (const auto& comp : metadata_.source_tree.components) {
        if (comp.type == "external" && comp.conan_ref) {
            data["conan_requires"].push_back(*comp.conan_ref);
        }
    }
    for (const auto& extra : metadata_.dependencies.extra_requires) {
        data["conan_requires"].push_back(extra);
    }
    data["tool_requires"] = metadata_.dependencies.tool_requires;

    TemplateEngine engine;
    engine.render_to_file("conanfile.jinja2", data, output_root / "conanfile.txt");
}

}  // namespace scaffolder

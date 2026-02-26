#include "generator/cmake_generator.hpp"
#include "generator/template_engine.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <map>

namespace scaffolder {

CmakeGenerator::CmakeGenerator(const Metadata& metadata, PathResolver& resolver, const std::filesystem::path& output_root)
    : metadata_(metadata), resolver_(resolver), output_root_(output_root) {}

void CmakeGenerator::generate_all() {
    generate_root_cmakelists();
    generate_cmake_helpers();
    for (const auto& comp : metadata_.source_tree.components) {
        if (comp.type == "external") continue;
        generate_component_cmakelists(comp);
    }
}

void CmakeGenerator::generate_root_cmakelists() {
    std::map<std::string, std::string> comp_dest;
    for (const auto& c : metadata_.source_tree.components) {
        if (c.dest && c.type != "external") {
            comp_dest[c.id] = *c.dest;
        }
    }

    std::vector<std::string> subdirs;
    const SwComponent* root_layer = nullptr;
    for (const auto& c : metadata_.source_tree.components) {
        if (c.type == "layer" && c.id == "root_layer") {
            root_layer = &c;
            break;
        }
    }

    if (root_layer && root_layer->subdirs) {
        for (const auto& sub : *root_layer->subdirs) {
            auto it = comp_dest.find(sub);
            if (it != comp_dest.end() && it->second != ".") {
                subdirs.push_back(it->second);
            }
        }
    } else {
        for (const auto& c : metadata_.source_tree.components) {
            if (c.type == "layer" && c.dest && *c.dest != ".") {
                subdirs.push_back(*c.dest);
            }
        }
    }

    nlohmann::json data;
    data["project"]["name"] = metadata_.project.name;
    data["project"]["version"] = metadata_.project.version;
    data["cmake_minimum"]["major"] = metadata_.project.cmake_minimum.major;
    data["cmake_minimum"]["minor"] = metadata_.project.cmake_minimum.minor;
    data["cmake_minimum"]["patch"] = metadata_.project.cmake_minimum.patch;
    data["subdirs"] = subdirs;

    TemplateEngine engine;
    engine.render_to_file("root_cmakelists.jinja2", data, output_root_ / "CMakeLists.txt");
}

void CmakeGenerator::generate_cmake_helpers() {
    std::filesystem::create_directories(output_root_ / "cmake");
    TemplateEngine engine;
    engine.render_to_file("cmake/AddHierarchicalLibrary.cmake", {}, output_root_ / "cmake" / "AddHierarchicalLibrary.cmake");
}

void CmakeGenerator::generate_component_cmakelists(const SwComponent& comp) {
    if (!comp.dest) return;
    if (comp.type == "layer" && (*comp.dest == "." || comp.dest->empty())) return;
    std::filesystem::path dest = output_root_ / *comp.dest;
    std::filesystem::create_directories(dest);

    if (comp.type == "library") {
        if (comp.structure == "hierarchical") {
            generate_hierarchical_library(comp, dest);
        } else {
            generate_library(comp, dest);
        }
    } else if (comp.type == "executable") {
        generate_executable(comp, dest);
    } else if (comp.type == "variant") {
        generate_variant(comp, dest);
    } else if (comp.type == "layer") {
        generate_layer(comp, dest);
    }
}

static nlohmann::json comp_to_json(const SwComponent& comp) {
    nlohmann::json j;
    j["id"] = comp.id;
    j["library_type"] = comp.library_type.value_or("static");
    j["source_extensions"] = comp.source_extensions.value_or(std::vector<std::string>{"*.c", "*.cpp", "*.cc"});
    j["include_extensions"] = comp.include_extensions.value_or(std::vector<std::string>{"*.h", "*.hpp"});
    j["dependencies"] = comp.dependencies.value_or(std::vector<std::string>{});
    return j;
}

void CmakeGenerator::generate_library(const SwComponent& comp, const std::filesystem::path& dest) {
    nlohmann::json data = comp_to_json(comp);
    TemplateEngine engine;
    engine.render_to_file("library_cmakelists.jinja2", data, dest / "CMakeLists.txt");
}

void CmakeGenerator::generate_hierarchical_library(const SwComponent& comp, const std::filesystem::path& dest) {
    nlohmann::json data = comp_to_json(comp);
    TemplateEngine engine;
    engine.render_to_file("hierarchical_library_cmakelists.jinja2", data, dest / "CMakeLists.txt");
}

void CmakeGenerator::generate_executable(const SwComponent& comp, const std::filesystem::path& dest) {
    nlohmann::json data = comp_to_json(comp);
    TemplateEngine engine;
    engine.render_to_file("executable_cmakelists.jinja2", data, dest / "CMakeLists.txt");
}

void CmakeGenerator::generate_variant(const SwComponent& comp, const std::filesystem::path& dest) {
    if (!comp.variations || comp.variations->empty()) return;

    nlohmann::json data;
    data["id"] = comp.id;
    data["variations"] = nlohmann::json::array();
    for (const auto& v : *comp.variations) {
        nlohmann::json var;
        var["subdir"] = v.subdir;
        var["condition_cmake"] = cond_eval_.to_cmake_if(v.condition);
        data["variations"].push_back(var);
    }

    TemplateEngine engine;
    engine.render_to_file("variant_cmakelists.jinja2", data, dest / "CMakeLists.txt");
}

void CmakeGenerator::generate_layer(const SwComponent& comp, const std::filesystem::path& dest) {
    std::map<std::string, std::string> comp_dest;
    for (const auto& c : metadata_.source_tree.components) {
        if (c.dest && c.type != "external") comp_dest[c.id] = *c.dest;
    }

    std::vector<std::string> subdirs;
    for (const auto& sub : comp.subdirs.value_or(std::vector<std::string>{})) {
        auto it = comp_dest.find(sub);
        if (it != comp_dest.end()) {
            std::string subpath = it->second;
            std::filesystem::path sub_full(output_root_ / subpath);
            std::filesystem::path rel;
            try {
                rel = std::filesystem::relative(sub_full, dest);
            } catch (...) {
                rel = subpath;
            }
            subdirs.push_back(rel.generic_string());
        }
    }

    nlohmann::json data;
    data["subdirs"] = subdirs;

    TemplateEngine engine;
    engine.render_to_file("layer_cmakelists.jinja2", data, dest / "CMakeLists.txt");
}

std::string CmakeGenerator::collect_sources(const std::filesystem::path& dir, const std::vector<std::string>& exts) {
    (void)dir;
    (void)exts;
    return "";
}

std::vector<std::filesystem::path> CmakeGenerator::collect_include_dirs(const std::filesystem::path& dir, const std::vector<std::string>& exts) {
    (void)dir;
    (void)exts;
    return {};
}

}  // namespace scaffolder

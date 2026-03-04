#include "config/config_loader.hpp"
#include "metadata/parser.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace scaffolder {

namespace {

std::string read_file(const std::filesystem::path& p) {
    std::ifstream f(p);
    if (!f) return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

nlohmann::json parse_json(const std::string& content) {
    if (content.empty()) return nlohmann::json::object();
    return nlohmann::json::parse(content);
}

void merge_into(nlohmann::json& root, const std::string& key, nlohmann::json value) {
    if (!value.is_null()) root[key] = std::move(value);
}

}  // namespace

Metadata ConfigLoader::load(const std::filesystem::path& folder) const {
    if (!std::filesystem::is_directory(folder)) {
        throw ConfigLoadError("Not a directory: " + folder.string());
    }

    nlohmann::json root = nlohmann::json::object();
    root["schema_version"] = 1;
    root["env"] = nlohmann::json::object();

    auto read_path = folder;
    if (!base_path_.empty()) read_path = base_path_;

    // project.json (required for a valid run; we allow missing and use defaults)
    {
        auto p = folder / "project.json";
        std::string content = read_file(p);
        if (!content.empty()) {
            auto j = parse_json(content);
            if (j.is_object()) root["project"] = std::move(j);
        }
    }

    // socs.json
    {
        auto p = folder / "socs.json";
        std::string content = read_file(p);
        if (!content.empty()) {
            auto j = parse_json(content);
            if (j.is_array()) root["socs"] = std::move(j);
        }
    }

    // boards.json
    {
        auto p = folder / "boards.json";
        std::string content = read_file(p);
        if (!content.empty()) {
            auto j = parse_json(content);
            if (j.is_array()) root["boards"] = std::move(j);
        }
    }

    // toolchains: single file or directory toolchains/*.json
    {
        auto single = folder / "toolchains.json";
        auto dir = folder / "toolchains";
        if (std::filesystem::is_regular_file(single)) {
            std::string content = read_file(single);
            if (!content.empty()) {
                auto j = parse_json(content);
                if (j.is_array()) root["toolchains"] = std::move(j);
            }
        } else if (std::filesystem::is_directory(dir)) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.path().extension() == ".json") {
                    std::string content = read_file(entry.path());
                    if (!content.empty()) {
                        auto j = parse_json(content);
                        if (j.is_object()) arr.push_back(std::move(j));
                    }
                }
            }
            if (!arr.empty()) root["toolchains"] = std::move(arr);
        }
    }

    // isa_variants.json
    {
        auto p = folder / "isa_variants.json";
        std::string content = read_file(p);
        if (!content.empty()) {
            auto j = parse_json(content);
            if (j.is_array()) root["isa_variants"] = std::move(j);
        }
    }

    // build_variants.json (or build_types.json)
    {
        auto p = folder / "build_variants.json";
        if (!std::filesystem::exists(p)) p = folder / "build_types.json";
        std::string content = read_file(p);
        if (!content.empty()) {
            auto j = parse_json(content);
            if (j.is_array()) root["build_variants"] = std::move(j);
        }
    }

    // components: single file or components/*.json
    {
        auto single = folder / "components.json";
        auto dir = folder / "components";
        if (std::filesystem::is_regular_file(single)) {
            std::string content = read_file(single);
            if (!content.empty()) {
                auto j = parse_json(content);
                root["source_tree"] = nlohmann::json::object();
                root["source_tree"]["components"] = j.is_array() ? std::move(j) : nlohmann::json::array();
            }
        } else if (std::filesystem::is_directory(dir)) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.path().extension() == ".json") {
                    std::string content = read_file(entry.path());
                    if (!content.empty()) {
                        auto j = parse_json(content);
                        if (j.is_object()) arr.push_back(std::move(j));
                    }
                }
            }
            root["source_tree"] = nlohmann::json::object();
            root["source_tree"]["components"] = std::move(arr);
        }
    }
    if (!root.contains("source_tree")) {
        root["source_tree"] = nlohmann::json::object();
        root["source_tree"]["components"] = nlohmann::json::array();
    }

    // cmake_preset.json (preset_matrix)
    {
        auto p = folder / "cmake_preset.json";
        if (!std::filesystem::exists(p)) p = folder / "presets.json";
        std::string content = read_file(p);
        if (!content.empty()) {
            auto j = parse_json(content);
            if (j.is_object()) {
                if (j.contains("preset_matrix"))
                    root["preset_matrix"] = j["preset_matrix"];
                else
                    root["preset_matrix"] = std::move(j);
            }
        }
    }
    if (!root.contains("preset_matrix")) {
        root["preset_matrix"] = nlohmann::json::object();
        root["preset_matrix"]["dimensions"] = nlohmann::json::array();
        root["preset_matrix"]["exclude"] = nlohmann::json::array();
        root["preset_matrix"]["naming"] = "{board}_{soc}_{isa}_{variant}";
        root["preset_matrix"]["binary_dir_pattern"] = "build/${preset}";
    }

    // conanfile.json -> dependencies
    {
        auto p = folder / "conanfile.json";
        std::string content = read_file(p);
        if (!content.empty()) {
            auto j = parse_json(content);
            if (j.is_object() && (j.contains("tool_requires") || j.contains("extra_requires"))) {
                root["dependencies"] = nlohmann::json::object();
                if (j.contains("tool_requires")) root["dependencies"]["tool_requires"] = j["tool_requires"];
                if (j.contains("extra_requires")) root["dependencies"]["extra_requires"] = j["extra_requires"];
            }
        }
    }

    Parser parser;
    return parser.parse_string(root.dump(), "json");
}

}  // namespace scaffolder

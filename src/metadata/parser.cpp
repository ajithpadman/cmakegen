#include "metadata/parser.hpp"
#include "metadata/env_expander.hpp"
#include <nlohmann/json.hpp>
#ifdef CMAKEGEN_HAS_YAML
#include <yaml-cpp/yaml.h>
#endif
#include <fstream>
#include <sstream>

namespace scaffolder {

#ifdef CMAKEGEN_HAS_YAML
namespace {

nlohmann::json yaml_to_json(const YAML::Node& node) {
    nlohmann::json j;
    switch (node.Type()) {
        case YAML::NodeType::Null:
            j = nullptr;
            break;
        case YAML::NodeType::Scalar: {
            std::string s = node.Scalar();
            if (s == "true" || s == "yes") j = true;
            else if (s == "false" || s == "no") j = false;
            else if (s.find('.') != std::string::npos) {
                try { j = std::stod(s); } catch (...) { j = s; }
            } else {
                try { j = std::stoll(s); } catch (...) { j = s; }
            }
            break;
        }
        case YAML::NodeType::Sequence:
            j = nlohmann::json::array();
            for (const auto& child : node) {
                j.push_back(yaml_to_json(child));
            }
            break;
        case YAML::NodeType::Map:
            j = nlohmann::json::object();
            for (const auto& kv : node) {
                j[kv.first.Scalar()] = yaml_to_json(kv.second);
            }
            break;
        case YAML::NodeType::Undefined:
            break;
    }
    return j;
}

}  // namespace
#endif

Metadata Parser::parse_file(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) {
        throw ParseError("Cannot open file: " + path.string());
    }
    std::stringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();
    std::string ext = path.extension().string();
    if (ext == ".yaml" || ext == ".yml") {
#ifdef CMAKEGEN_HAS_YAML
        return parse_string(content, "yaml");
#else
        throw ParseError("YAML support not compiled in. Use JSON or build with yaml-cpp.");
#endif
    }
    return parse_string(content, "json");
}

Metadata Parser::parse_string(const std::string& content, const std::string& format) {
    nlohmann::json j;
    if (format == "yaml" || format == "yml") {
#ifdef CMAKEGEN_HAS_YAML
        YAML::Node yaml = YAML::Load(content);
        j = yaml_to_json(yaml);
#else
        throw ParseError("YAML support not compiled in.");
#endif
    } else {
        j = nlohmann::json::parse(content);
    }
    parse_from_json(j);
    return metadata_;
}

void Parser::parse_condition(const nlohmann::json& j, Condition& cond) {
    if (j.contains("var")) cond.var = j["var"].get<std::string>();
    if (j.contains("op")) cond.op = j["op"].get<std::string>();
    if (j.contains("value")) {
        if (j["value"].is_array()) {
            std::vector<std::string> v;
            for (const auto& x : j["value"]) v.push_back(x.get<std::string>());
            cond.value = v;
        } else {
            cond.value = j["value"].get<std::string>();
        }
    }
    if (j.contains("and")) {
        cond.and_ = std::vector<std::shared_ptr<Condition>>();
        for (const auto& c : j["and"]) {
            auto sub = std::make_shared<Condition>();
            parse_condition(c, *sub);
            cond.and_->push_back(sub);
        }
    }
    if (j.contains("or")) {
        cond.or_ = std::vector<std::shared_ptr<Condition>>();
        for (const auto& c : j["or"]) {
            auto sub = std::make_shared<Condition>();
            parse_condition(c, *sub);
            cond.or_->push_back(sub);
        }
    }
    if (j.contains("not")) {
        cond.not_ = std::make_shared<Condition>();
        parse_condition(j["not"], **cond.not_);
    }
    if (j.contains("default")) cond.default_ = j["default"].get<bool>();
}

static void expand_json_strings(nlohmann::json& j, const EnvExpander& exp, bool skip_preset_matrix = false) {
    if (j.is_string()) {
        if (!skip_preset_matrix) {
            std::string s = j.get<std::string>();
            if (s.find("${") != std::string::npos) {
                j = exp.expand(s);
            }
        }
    } else if (j.is_object()) {
        for (auto it = j.begin(); it != j.end(); ++it) {
            bool skip = skip_preset_matrix || it.key() == "preset_matrix";
            expand_json_strings(it.value(), exp, skip);
        }
    } else if (j.is_array()) {
        for (auto& el : j) expand_json_strings(el, exp, skip_preset_matrix);
    }
}

void Parser::parse_from_json(const nlohmann::json& j) {
    metadata_ = Metadata{};
    if (j.contains("schema_version")) metadata_.schema_version = j["schema_version"].get<int>();

    std::map<std::string, std::string> env_from_json;
    if (j.contains("env")) {
        const auto& e = j["env"];
        if (e.is_object()) {
            for (auto it = e.begin(); it != e.end(); ++it) {
                if (it.value().is_string()) {
                    env_from_json[it.key()] = it.value().get<std::string>();
                }
            }
        } else if (e.is_array()) {
            for (const auto& item : e) {
                if (item.is_object() && item.contains("name") && item.contains("value")) {
                    env_from_json[item["name"].get<std::string>()] = item["value"].get<std::string>();
                }
            }
        }
    }
    EnvExpander exp(env_from_json);
    nlohmann::json j_expanded = j;
    expand_json_strings(j_expanded, exp);
    const nlohmann::json& jp = j_expanded;

    if (jp.contains("env")) {
        const auto& e = jp["env"];
        if (e.is_object()) {
            for (auto it = e.begin(); it != e.end(); ++it) {
                if (it.value().is_string()) {
                    metadata_.env[it.key()] = it.value().get<std::string>();
                }
            }
        }
    }

    if (jp.contains("project")) {
        const auto& p = jp["project"];
        metadata_.project.name = p.value("name", "");
        metadata_.project.version = p.value("version", "");
        if (p.contains("cmake_minimum")) {
            const auto& cm = p["cmake_minimum"];
            metadata_.project.cmake_minimum.major = cm.value("major", 3);
            metadata_.project.cmake_minimum.minor = cm.value("minor", 23);
            metadata_.project.cmake_minimum.patch = cm.value("patch", 0);
        }
    }

    if (jp.contains("socs")) {
        for (const auto& s : jp["socs"]) {
            Soc soc;
            soc.id = s.value("id", "");
            soc.display_name = s.value("display_name", "");
            soc.description = s.value("description", "");
            if (s.contains("isas")) {
                for (const auto& x : s["isas"]) soc.isas.push_back(x.get<std::string>());
            } else if (s.contains("isa_cores")) {
                for (const auto& ic : s["isa_cores"]) {
                    std::string isa = ic.value("isa", "");
                    if (!isa.empty()) soc.isas.push_back(isa);
                }
            }
            metadata_.socs.push_back(soc);
        }
    }

    if (jp.contains("boards")) {
        for (const auto& b : jp["boards"]) {
            Board board;
            board.id = b.value("id", "");
            board.display_name = b.value("display_name", "");
            if (b.contains("socs")) {
                for (const auto& s : b["socs"]) board.socs.push_back(s.get<std::string>());
            }
            if (b.contains("defines")) {
                for (const auto& d : b["defines"]) board.defines.push_back(d.get<std::string>());
            }
            metadata_.boards.push_back(board);
        }
    }

    if (jp.contains("toolchains")) {
        for (const auto& t : jp["toolchains"]) {
            Toolchain tc;
            tc.id = t.value("id", "");
            tc.display_name = t.value("display_name", "");
            if (t.contains("compiler")) {
                const auto& c = t["compiler"];
                tc.compiler.c = c.value("c", "");
                tc.compiler.cxx = c.value("cxx", "");
                tc.compiler.asm_ = c.contains("asm") ? c["asm"].get<std::string>() : c.value("asm", "");
            }
            if (t.contains("flags")) {
                for (auto it = t["flags"].begin(); it != t["flags"].end(); ++it) {
                    std::vector<std::string> v;
                    for (const auto& x : it.value()) v.push_back(x.get<std::string>());
                    tc.flags[it.key()] = v;
                }
            }
            if (t.contains("libs")) for (const auto& l : t["libs"]) tc.libs.push_back(l.get<std::string>());
            if (t.contains("lib_paths")) for (const auto& lp : t["lib_paths"]) tc.lib_paths.push_back(lp.get<std::string>());
            if (t.contains("defines")) for (const auto& d : t["defines"]) tc.defines.push_back(d.get<std::string>());
            tc.sysroot = t.value("sysroot", "");
            metadata_.toolchains.push_back(tc);
        }
    }

    if (jp.contains("isa_variants")) {
        for (const auto& iv : jp["isa_variants"]) {
            IsaVariant v;
            v.id = iv.value("id", "");
            v.toolchain = iv.value("toolchain", "");
            v.display_name = iv.value("display_name", "");
            metadata_.isa_variants.push_back(v);
        }
    }

    if (jp.contains("build_variants")) {
        for (const auto& bv : jp["build_variants"]) {
            BuildVariant v;
            v.id = bv.value("id", "");
            v.inherits = bv.contains("inherits") ? std::optional(bv["inherits"].get<std::string>()) : std::nullopt;
            if (bv.contains("flags")) {
                for (auto it = bv["flags"].begin(); it != bv["flags"].end(); ++it) {
                    std::vector<std::string> vec;
                    for (const auto& x : it.value()) vec.push_back(x.get<std::string>());
                    v.flags[it.key()] = vec;
                }
            }
            if (bv.contains("remove_flags")) {
                for (auto it = bv["remove_flags"].begin(); it != bv["remove_flags"].end(); ++it) {
                    std::vector<std::string> vec;
                    for (const auto& x : it.value()) vec.push_back(x.get<std::string>());
                    v.remove_flags[it.key()] = vec;
                }
            }
            if (bv.contains("add_flags")) {
                for (auto it = bv["add_flags"].begin(); it != bv["add_flags"].end(); ++it) {
                    std::map<std::string, std::vector<std::string>> tc_flags;
                    for (auto kt = it.value().begin(); kt != it.value().end(); ++kt) {
                        std::vector<std::string> vec;
                        for (const auto& x : kt.value()) vec.push_back(x.get<std::string>());
                        tc_flags[kt.key()] = vec;
                    }
                    v.add_flags[it.key()] = tc_flags;
                }
            }
            metadata_.build_variants.push_back(v);
        }
    }

    if (jp.contains("source_tree") && jp["source_tree"].contains("components")) {
        for (const auto& c : jp["source_tree"]["components"]) {
            SwComponent comp;
            comp.id = c.value("id", "");
            comp.type = c.value("type", "");
            comp.library_type = c.contains("library_type") ? std::optional(c["library_type"].get<std::string>()) : std::nullopt;
            comp.structure = c.contains("structure") ? std::optional(c["structure"].get<std::string>()) : std::nullopt;
            comp.source = c.contains("source") ? std::optional(c["source"].get<std::string>()) : std::nullopt;
            if (c.contains("git")) {
                const auto& g = c["git"];
                GitSource gs;
                gs.url = g.value("url", "");
                gs.tag = g.contains("tag") ? std::optional(g["tag"].get<std::string>()) : std::nullopt;
                gs.branch = g.contains("branch") ? std::optional(g["branch"].get<std::string>()) : std::nullopt;
                gs.commit = g.contains("commit") ? std::optional(g["commit"].get<std::string>()) : std::nullopt;
                comp.git = gs;
            }
            comp.dest = c.contains("dest") ? std::optional(c["dest"].get<std::string>()) : std::nullopt;
            if (c.contains("filters")) {
                PathFilters pf;
                if (c["filters"].contains("exclude_paths"))
                    for (const auto& x : c["filters"]["exclude_paths"]) pf.exclude_paths.push_back(x.get<std::string>());
                if (c["filters"].contains("include_paths"))
                    for (const auto& x : c["filters"]["include_paths"]) pf.include_paths.push_back(x.get<std::string>());
                comp.filters = pf;
            }
            if (c.contains("source_extensions")) {
                comp.source_extensions = std::vector<std::string>();
                for (const auto& x : c["source_extensions"]) comp.source_extensions->push_back(x.get<std::string>());
            }
            if (c.contains("include_extensions")) {
                comp.include_extensions = std::vector<std::string>();
                for (const auto& x : c["include_extensions"]) comp.include_extensions->push_back(x.get<std::string>());
            }
            if (c.contains("dependencies")) {
                comp.dependencies = std::vector<std::string>();
                for (const auto& d : c["dependencies"]) comp.dependencies->push_back(d.get<std::string>());
            }
            comp.conan_ref = c.contains("conan_ref") ? std::optional(c["conan_ref"].get<std::string>()) : std::nullopt;
            if (c.contains("variations")) {
                comp.variations = std::vector<Variation>();
                for (const auto& v : c["variations"]) {
                    Variation var;
                    var.subdir = v.value("subdir", "");
                    parse_condition(v["condition"], var.condition);
                    comp.variations->push_back(var);
                }
            }
            if (c.contains("subdirs")) {
                comp.subdirs = std::vector<std::string>();
                for (const auto& s : c["subdirs"]) comp.subdirs->push_back(s.get<std::string>());
            }
            metadata_.source_tree.components.push_back(comp);
        }
    }

    if (jp.contains("dependencies")) {
        const auto& d = jp["dependencies"];
        if (d.contains("tool_requires")) for (const auto& t : d["tool_requires"]) metadata_.dependencies.tool_requires.push_back(t.get<std::string>());
        if (d.contains("extra_requires")) for (const auto& e : d["extra_requires"]) metadata_.dependencies.extra_requires.push_back(e.get<std::string>());
    }

    if (jp.contains("preset_matrix")) {
        const auto& pm = jp["preset_matrix"];
        if (pm.contains("dimensions")) for (const auto& dim : pm["dimensions"]) metadata_.preset_matrix.dimensions.push_back(dim.get<std::string>());
        if (pm.contains("exclude")) {
            for (const auto& ex : pm["exclude"]) {
                PresetExclude pe;
                if (ex.contains("board")) pe.board = ex["board"].get<std::string>();
                if (ex.contains("soc")) pe.soc = ex["soc"].get<std::string>();
                if (ex.contains("isa_variant")) pe.isa_variant = ex["isa_variant"].get<std::string>();
                if (ex.contains("build_variant")) pe.build_variant = ex["build_variant"].get<std::string>();
                metadata_.preset_matrix.exclude.push_back(pe);
            }
        }
        metadata_.preset_matrix.naming = pm.value("naming", "{board}_{soc}_{isa}_{variant}");
        metadata_.preset_matrix.binary_dir_pattern = pm.value("binary_dir_pattern", "build/${preset}");
    }
}

}  // namespace scaffolder

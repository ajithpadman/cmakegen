#include "interactive/metadata_builder.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace scaffolder {

std::string normalize_id(const std::string& name) {
    std::string out;
    bool prev_underscore = false;
    for (unsigned char c : name) {
        if (std::isspace(c)) {
            if (!prev_underscore) { out += '_'; prev_underscore = true; }
        } else if (std::isalnum(c) || c == '_' || c == '-') {
            out += static_cast<char>(c);
            prev_underscore = (c == '_');
        } else {
            if (!prev_underscore) { out += '_'; prev_underscore = true; }
        }
    }
    // Trim leading/trailing underscores
    size_t start = out.find_first_not_of('_');
    if (start == std::string::npos) return "unknown";
    size_t end = out.find_last_not_of('_');
    out = out.substr(start, end - start + 1);
    return out.empty() ? "unknown" : out;
}

std::vector<std::string> split_comma_separated(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, ',')) {
        size_t start = token.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        size_t end = token.find_last_not_of(" \t");
        token = token.substr(start, end - start + 1);
        if (!token.empty()) out.push_back(token);
    }
    return out;
}

void MetadataBuilder::set_preset_matrix_defaults() {
    // Called after collection; dimensions/naming are derived in to_json().
}

static nlohmann::json vec_to_json(const std::vector<std::string>& v) {
    nlohmann::json a = nlohmann::json::array();
    for (const auto& s : v) a.push_back(s);
    return a;
}

static nlohmann::json condition_to_json(const std::shared_ptr<ConditionData>& c) {
    if (!c) return {{"default", true}};
    if (c->default_ && *c->default_) return {{"default", true}};
    if (c->and_) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& sub : *c->and_) arr.push_back(condition_to_json(sub));
        return {{"and", arr}};
    }
    if (c->or_) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& sub : *c->or_) arr.push_back(condition_to_json(sub));
        return {{"or", arr}};
    }
    if (c->not_) return {{"not", condition_to_json(*c->not_)}};
    nlohmann::json j;
    if (c->var) j["var"] = *c->var;
    if (c->op) j["op"] = *c->op;
    if (c->value) {
        std::visit([&j](const auto& v) {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>)
                j["value"] = v;
            else
                j["value"] = vec_to_json(v);
        }, *c->value);
    }
    return j;
}

static void condition_summary_impl(const std::shared_ptr<ConditionData>& c, std::ostringstream& out) {
    if (!c) { out << "(none)"; return; }
    if (c->default_ && *c->default_) { out << "Default"; return; }
    if (c->and_) {
        out << "AND(";
        for (size_t i = 0; i < c->and_->size(); ++i) {
            if (i) out << ", ";
            condition_summary_impl((*c->and_)[i], out);
        }
        out << ")";
        return;
    }
    if (c->or_) {
        out << "OR(";
        for (size_t i = 0; i < c->or_->size(); ++i) {
            if (i) out << ", ";
            condition_summary_impl((*c->or_)[i], out);
        }
        out << ")";
        return;
    }
    if (c->not_) {
        out << "NOT(";
        condition_summary_impl(*c->not_, out);
        out << ")";
        return;
    }
    if (c->var) out << *c->var;
    if (c->op) out << " " << *c->op << " ";
    if (c->value) {
        std::visit([&out](const auto& v) {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>)
                out << v;
            else {
                out << "[";
                for (size_t i = 0; i < v.size(); ++i) { if (i) out << ","; out << v[i]; }
                out << "]";
            }
        }, *c->value);
    }
}

std::string condition_summary(const std::shared_ptr<ConditionData>& cond) {
    std::ostringstream out;
    condition_summary_impl(cond, out);
    return out.str();
}

std::string MetadataBuilder::to_json() const {
    nlohmann::json j;
    j["schema_version"] = 1;
    j["env"] = nlohmann::json::object();

    j["project"] = {
        {"name", project.name},
        {"version", project.version},
        {"cmake_minimum", {{"major", project.cmake_major}, {"minor", project.cmake_minor}, {"patch", project.cmake_patch}}}
    };

    j["socs"] = nlohmann::json::array();
    for (const auto& s : socs) {
        j["socs"].push_back({
            {"id", s.id},
            {"display_name", s.display_name},
            {"description", s.description},
            {"isas", vec_to_json(s.isas)}
        });
    }

    j["boards"] = nlohmann::json::array();
    for (const auto& b : boards) {
        j["boards"].push_back({
            {"id", b.id},
            {"display_name", b.display_name},
            {"socs", vec_to_json(b.socs)},
            {"defines", vec_to_json(b.defines)}
        });
    }

    j["toolchains"] = nlohmann::json::array();
    for (const auto& t : toolchains) {
        nlohmann::json tc;
        tc["id"] = t.id;
        tc["display_name"] = t.display_name;
        tc["compiler"] = {{"c", t.compiler.c}, {"cxx", t.compiler.cxx}, {"asm", t.compiler.asm_}};
        tc["flags"] = nlohmann::json::object();
        for (const auto& [k, v] : t.flags) tc["flags"][k] = vec_to_json(v);
        tc["libs"] = vec_to_json(t.libs);
        tc["lib_paths"] = vec_to_json(t.lib_paths);
        tc["defines"] = vec_to_json(t.defines);
        tc["sysroot"] = t.sysroot;
        j["toolchains"].push_back(tc);
    }

    j["isa_variants"] = nlohmann::json::array();
    for (const auto& iv : isa_variants) {
        j["isa_variants"].push_back({
            {"id", iv.id},
            {"toolchain", iv.toolchain},
            {"display_name", iv.display_name.empty() ? iv.id : iv.display_name}
        });
    }

    j["build_variants"] = nlohmann::json::array();
    for (const auto& bv : build_variants) {
        nlohmann::json bvj;
        bvj["id"] = bv.id;
        bvj["flags"] = nlohmann::json::object();
        for (const auto& [k, v] : bv.flags) bvj["flags"][k] = vec_to_json(v);
        j["build_variants"].push_back(bvj);
    }

    j["source_tree"] = nlohmann::json::object();
    j["source_tree"]["components"] = nlohmann::json::array();
    for (const auto& c : components) {
        nlohmann::json comp;
        comp["id"] = c.id;
        comp["type"] = c.type;
        if (c.type == "library" && !c.library_type.empty())
            comp["library_type"] = c.library_type;
        if (!c.structure.empty()) comp["structure"] = c.structure;
        if (!c.source.empty()) comp["source"] = c.source;
        if (!c.dest.empty()) comp["dest"] = c.dest;
        comp["dependencies"] = vec_to_json(c.dependencies);
        if (c.type == "external" && !c.conan_ref.empty())
            comp["conan_ref"] = c.conan_ref;
        if (c.type == "layer" && !c.subdirs.empty())
            comp["subdirs"] = vec_to_json(c.subdirs);
        comp["source_extensions"] = vec_to_json(c.source_extensions);
        comp["include_extensions"] = vec_to_json(c.include_extensions);
        if (c.filters) {
            comp["filters"] = {
                {"filter_mode", c.filters->filter_mode},
                {"include_paths", vec_to_json(c.filters->include_paths)},
                {"exclude_paths", vec_to_json(c.filters->exclude_paths)}
            };
        }
        if (!c.git_url.empty()) {
            comp["git"] = {{"url", c.git_url}};
            if (!c.git_tag.empty()) comp["git"]["tag"] = c.git_tag;
            if (!c.git_branch.empty()) comp["git"]["branch"] = c.git_branch;
            if (!c.git_commit.empty()) comp["git"]["commit"] = c.git_commit;
        }
        if (c.type == "variant" && !c.variations.empty()) {
            comp["variations"] = nlohmann::json::array();
            for (const auto& v : c.variations) {
                comp["variations"].push_back({{"subdir", v.subdir}, {"condition", condition_to_json(v.condition)}});
            }
        }
        j["source_tree"]["components"].push_back(comp);
    }

    j["dependencies"] = {
        {"tool_requires", vec_to_json(dependencies.tool_requires)},
        {"extra_requires", vec_to_json(dependencies.extra_requires)}
    };

    // Preset matrix: derive dimensions from what we have
    std::vector<std::string> dims;
    if (!boards.empty()) dims.push_back("board");
    if (!socs.empty()) dims.push_back("soc");
    if (!isa_variants.empty()) dims.push_back("isa_variant");
    if (!build_variants.empty()) dims.push_back("build_variant");
    if (dims.empty()) dims.push_back("build_variant");

    std::string naming = "{board}_{soc}_{isa}_{variant}";
    if (dims.size() == 1 && dims[0] == "build_variant") naming = "{variant}";

    j["preset_matrix"] = {
        {"dimensions", vec_to_json(dims)},
        {"exclude", nlohmann::json::array()},
        {"naming", naming},
        {"binary_dir_pattern", "build/${preset}"}
    };

    return j.dump(2);
}

}  // namespace scaffolder

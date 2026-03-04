#include "config/config_writer.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace scaffolder {

namespace {

using json = nlohmann::json;

json to_json(const CmakeVersion& v) {
    return {{"major", v.major}, {"minor", v.minor}, {"patch", v.patch}};
}

json to_json(const Project& p) {
    json j = {{"name", p.name}, {"version", p.version}};
    j["cmake_minimum"] = to_json(p.cmake_minimum);
    return j;
}

json to_json(const Soc& s) {
    json j = {{"id", s.id}, {"display_name", s.display_name}, {"description", s.description}};
    j["isas"] = s.isas;
    return j;
}

json to_json(const Board& b) {
    json j = {{"id", b.id}, {"display_name", b.display_name}, {"socs", b.socs}, {"defines", b.defines}};
    return j;
}

json to_json(const Compiler& c) {
    return {{"c", c.c}, {"cxx", c.cxx}, {"asm", c.asm_}};
}

json to_json(const Toolchain& t) {
    json j = {{"id", t.id}, {"display_name", t.display_name}, {"compiler", to_json(t.compiler)}};
    j["flags"] = t.flags;
    j["libs"] = t.libs;
    j["lib_paths"] = t.lib_paths;
    j["defines"] = t.defines;
    j["sysroot"] = t.sysroot;
    return j;
}

json to_json(const IsaVariant& iv) {
    return {{"id", iv.id}, {"toolchain", iv.toolchain}, {"display_name", iv.display_name}};
}

json to_json(const BuildVariant& bv) {
    json j = {{"id", bv.id}};
    if (bv.inherits) j["inherits"] = *bv.inherits;
    j["flags"] = bv.flags;
    j["remove_flags"] = bv.remove_flags;
    j["add_flags"] = bv.add_flags;
    return j;
}

json to_json(const Condition& c) {
    json j = json::object();
    if (c.var) j["var"] = *c.var;
    if (c.op) j["op"] = *c.op;
    if (c.value) {
        std::visit([&j](const auto& v) {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) j["value"] = v;
            else j["value"] = v;
        }, *c.value);
    }
    if (c.and_) {
        json arr = json::array();
        for (const auto& sub : *c.and_) arr.push_back(to_json(*sub));
        j["and"] = arr;
    }
    if (c.or_) {
        json arr = json::array();
        for (const auto& sub : *c.or_) arr.push_back(to_json(*sub));
        j["or"] = arr;
    }
    if (c.not_) j["not"] = to_json(**c.not_);
    if (c.default_) j["default"] = *c.default_;
    return j;
}

json to_json(const GitSource& g) {
    json j = {{"url", g.url}};
    if (g.tag) j["tag"] = *g.tag;
    if (g.branch) j["branch"] = *g.branch;
    if (g.commit) j["commit"] = *g.commit;
    return j;
}

json to_json(const PathFilters& pf) {
    json j = {{"exclude_paths", pf.exclude_paths}, {"include_paths", pf.include_paths}};
    j["filter_mode"] = (pf.filter_mode == FilterMode::ExcludeFirst) ? "exclude_first" : "include_first";
    return j;
}

json to_json(const Variation& v) {
    return {{"subdir", v.subdir}, {"condition", to_json(v.condition)}};
}

json to_json(const SwComponent& c) {
    json j = {{"id", c.id}, {"type", c.type}};
    if (c.library_type) j["library_type"] = *c.library_type;
    if (c.structure) j["structure"] = *c.structure;
    if (c.source) j["source"] = *c.source;
    if (c.git) j["git"] = to_json(*c.git);
    if (c.dest) j["dest"] = *c.dest;
    if (c.condition) j["condition"] = to_json(*c.condition);
    if (c.filters) j["filters"] = to_json(*c.filters);
    if (c.source_extensions) j["source_extensions"] = *c.source_extensions;
    if (c.include_extensions) j["include_extensions"] = *c.include_extensions;
    if (c.metadata_extensions) j["metadata_extensions"] = *c.metadata_extensions;
    if (c.dependencies) j["dependencies"] = *c.dependencies;
    if (c.conan_ref) j["conan_ref"] = *c.conan_ref;
    if (c.variations) {
        json arr = json::array();
        for (const auto& v : *c.variations) arr.push_back(to_json(v));
        j["variations"] = arr;
    }
    if (c.subdirs) j["subdirs"] = *c.subdirs;
    return j;
}

json to_json(const PresetExclude& pe) {
    json j = json::object();
    if (pe.board) j["board"] = *pe.board;
    if (pe.soc) j["soc"] = *pe.soc;
    if (pe.isa_variant) j["isa_variant"] = *pe.isa_variant;
    if (pe.build_variant) j["build_variant"] = *pe.build_variant;
    return j;
}

json to_json(const PresetMatrix& pm) {
    json j = {{"dimensions", pm.dimensions}, {"naming", pm.naming}, {"binary_dir_pattern", pm.binary_dir_pattern}};
    json ex = json::array();
    for (const auto& e : pm.exclude) ex.push_back(to_json(e));
    j["exclude"] = ex;
    return j;
}

json to_json(const Dependencies& d) {
    return {{"tool_requires", d.tool_requires}, {"extra_requires", d.extra_requires}};
}

std::string read_file(const std::filesystem::path& p) {
    std::ifstream f(p);
    if (!f) return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void write_file(const std::filesystem::path& p, const std::string& content) {
    std::filesystem::create_directories(p.parent_path());
    std::ofstream f(p);
    if (!f) return;
    f << content;
}

json read_array_file(const std::filesystem::path& p) {
    std::string content = read_file(p);
    if (content.empty()) return json::array();
    try {
        json j = json::parse(content);
        return j.is_array() ? j : json::array();
    } catch (...) {
        return json::array();
    }
}

void write_array_file(const std::filesystem::path& p, const json& arr) {
    write_file(p, arr.dump(2));
}

}  // namespace

void ConfigWriter::write_project(const std::filesystem::path& folder, const Project& p) const {
    json j = to_json(p);
    write_file(folder / "project.json", j.dump(2));
}

void ConfigWriter::append_soc(const std::filesystem::path& folder, const Soc& s) const {
    auto p = folder / "socs.json";
    json arr = read_array_file(p);
    arr.push_back(to_json(s));
    write_array_file(p, arr);
}

void ConfigWriter::append_board(const std::filesystem::path& folder, const Board& b) const {
    auto p = folder / "boards.json";
    json arr = read_array_file(p);
    arr.push_back(to_json(b));
    write_array_file(p, arr);
}

void ConfigWriter::append_toolchain(const std::filesystem::path& folder, const Toolchain& t) const {
    auto p = folder / "toolchains.json";
    json arr = read_array_file(p);
    arr.push_back(to_json(t));
    write_array_file(p, arr);
}

void ConfigWriter::append_isa_variant(const std::filesystem::path& folder, const IsaVariant& iv) const {
    auto p = folder / "isa_variants.json";
    json arr = read_array_file(p);
    arr.push_back(to_json(iv));
    write_array_file(p, arr);
}

void ConfigWriter::append_build_variant(const std::filesystem::path& folder, const BuildVariant& bv) const {
    auto p = folder / "build_variants.json";
    json arr = read_array_file(p);
    arr.push_back(to_json(bv));
    write_array_file(p, arr);
}

void ConfigWriter::append_component(const std::filesystem::path& folder, const SwComponent& c) const {
    auto p = folder / "components.json";
    json arr = read_array_file(p);
    arr.push_back(to_json(c));
    write_array_file(p, arr);
}

void ConfigWriter::write_preset_matrix(const std::filesystem::path& folder, const PresetMatrix& pm) const {
    json j = to_json(pm);
    write_file(folder / "cmake_preset.json", j.dump(2));
}

void ConfigWriter::write_dependencies(const std::filesystem::path& folder, const Dependencies& d) const {
    json j = to_json(d);
    write_file(folder / "conanfile.json", j.dump(2));
}

}  // namespace scaffolder

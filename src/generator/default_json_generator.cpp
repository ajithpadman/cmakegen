#include "generator/default_json_generator.hpp"
#include <nlohmann/json.hpp>

namespace scaffolder {

static nlohmann::json flags_template() {
    return {
        {"c", nlohmann::json::array()},
        {"cxx", nlohmann::json::array()},
        {"asm", nlohmann::json::array()},
        {"linker", nlohmann::json::array()}
    };
}

std::string generate_default_json() {
    nlohmann::json j;
    j["schema_version"] = 1;
    j["env"] = nlohmann::json::object();

    j["project"] = {
        {"name", ""},
        {"version", "0.1.0"},
        {"cmake_minimum", {{"major", 3}, {"minor", 23}, {"patch", 0}}}
    };

    j["socs"] = nlohmann::json::array({
        {{"id", ""}, {"display_name", ""}, {"description", ""}, {"isas", nlohmann::json::array()}}
    });

    j["boards"] = nlohmann::json::array({
        {{"id", ""}, {"display_name", ""}, {"socs", nlohmann::json::array()}, {"defines", nlohmann::json::array()}}
    });

    j["toolchains"] = nlohmann::json::array({
        {
            {"id", ""},
            {"display_name", ""},
            {"compiler", {{"c", ""}, {"cxx", ""}, {"asm", ""}}},
            {"flags", flags_template()},
            {"libs", nlohmann::json::array()},
            {"lib_paths", nlohmann::json::array()},
            {"defines", nlohmann::json::array()},
            {"sysroot", ""}
        }
    });

    j["isa_variants"] = nlohmann::json::array({
        {{"id", ""}, {"toolchain", ""}, {"display_name", ""}}
    });

    j["build_variants"] = nlohmann::json::array({
        {
            {"id", "debug"},
            {"inherits", ""},
            {"flags", flags_template()},
            {"remove_flags", flags_template()},
            {"add_flags", {{"<toolchain_id>", flags_template()}}}
        },
        {
            {"id", "release"},
            {"inherits", ""},
            {"flags", flags_template()},
            {"remove_flags", flags_template()},
            {"add_flags", {{"<toolchain_id>", flags_template()}}}
        }
    });

    nlohmann::json lib_comp = {
        {"id", ""},
        {"type", "library"},
        {"library_type", "static"},
        {"structure", "hierarchical"},
        {"source", ""},
        {"dest", ""},
        {"filters", {
            {"exclude_paths", nlohmann::json::array()},
            {"include_paths", nlohmann::json::array()}
        }},
        {"source_extensions", nlohmann::json::array({"*.c", "*.cpp"})},
        {"include_extensions", nlohmann::json::array({"*.h", "*.hpp"})},
        {"dependencies", nlohmann::json::array()}
    };
    lib_comp["git"] = {{"url", ""}, {"tag", ""}, {"branch", ""}, {"commit", ""}};

    nlohmann::json exec_comp = {
        {"id", ""},
        {"type", "executable"},
        {"source", ""},
        {"dest", ""},
        {"source_extensions", nlohmann::json::array({"*.c", "*.cpp"})},
        {"include_extensions", nlohmann::json::array({"*.h", "*.hpp"})},
        {"dependencies", nlohmann::json::array()}
    };

    nlohmann::json ext_comp = {
        {"id", ""},
        {"type", "external"},
        {"conan_ref", ""}
    };

    nlohmann::json variant_comp = {
        {"id", ""},
        {"type", "variant"},
        {"source", ""},
        {"dest", ""},
        {"variations", nlohmann::json::array({
            {{"subdir", ""}, {"condition", {{"var", "SOC"}, {"op", "in"}, {"value", nlohmann::json::array()}}}},
            {{"subdir", ""}, {"condition", {{"default", true}}}}
        })},
        {"dependencies", nlohmann::json::array()}
    };

    nlohmann::json layer_comp = {
        {"id", ""},
        {"type", "layer"},
        {"dest", ""},
        {"subdirs", nlohmann::json::array()},
        {"dependencies", nlohmann::json::array()}
    };

    j["source_tree"] = {
        {"components", nlohmann::json::array({lib_comp, exec_comp, ext_comp, variant_comp, layer_comp})}
    };

    j["dependencies"] = {
        {"tool_requires", nlohmann::json::array()},
        {"extra_requires", nlohmann::json::array()}
    };

    j["preset_matrix"] = {
        {"dimensions", nlohmann::json::array({"board", "soc", "isa_variant", "build_variant"})},
        {"exclude", nlohmann::json::array({
            {{"board", ""}, {"soc", ""}, {"isa_variant", ""}, {"build_variant", ""}}
        })},
        {"naming", "{board}_{soc}_{isa}_{variant}"},
        {"binary_dir_pattern", "build/${preset}"}
    };

    return j.dump(2);
}

void write_default_json(std::ostream& out) {
    out << generate_default_json();
}

}  // namespace scaffolder

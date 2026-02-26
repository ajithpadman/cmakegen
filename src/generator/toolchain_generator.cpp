#include "generator/toolchain_generator.hpp"
#include "generator/template_engine.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

namespace scaffolder {

ToolchainGenerator::ToolchainGenerator(const Metadata& metadata) : metadata_(metadata) {}

static std::string infer_processor(const Toolchain& tc) {
    std::string c = tc.compiler.c;
    if (c.find("arm") != std::string::npos) return "arm";
    if (c.find("riscv") != std::string::npos) return "riscv";
    if (c.find("avr") != std::string::npos) return "avr";
    return "generic";
}

std::vector<std::string> ToolchainGenerator::merge_flags(const std::vector<std::string>& base,
                                                         const BuildVariant* bv,
                                                         const std::string& tc_id,
                                                         const std::string& flag_type) const {
    std::vector<std::string> result = base;

    if (bv) {
        auto it_remove = bv->remove_flags.find(flag_type);
        if (it_remove != bv->remove_flags.end()) {
            for (const auto& to_remove : it_remove->second) {
                result.erase(std::remove(result.begin(), result.end(), to_remove), result.end());
            }
        }

        std::vector<std::string> to_add;
        auto it_tc = bv->add_flags.find(tc_id);
        if (it_tc != bv->add_flags.end()) {
            auto it_type = it_tc->second.find(flag_type);
            if (it_type != it_tc->second.end()) to_add = it_type->second;
        }
        if (to_add.empty()) {
            auto it_def = bv->flags.find(flag_type);
            if (it_def != bv->flags.end()) to_add = it_def->second;
        }
        for (const auto& f : to_add) result.push_back(f);
    }

    return result;
}

void ToolchainGenerator::generate_toolchain(const Toolchain& tc, const BuildVariant* bv,
                                            const std::filesystem::path& output_dir) {
    nlohmann::json data;
    data["display_name"] = tc.display_name;
    data["processor"] = infer_processor(tc);
    data["compiler"] = {{"c", tc.compiler.c}, {"cxx", tc.compiler.cxx}, {"asm", tc.compiler.asm_}};

    data["flags"] = nlohmann::json::object();
    for (const std::string& flag_type : {"c", "cxx", "asm", "linker"}) {
        std::vector<std::string> base_vec;
        auto it = tc.flags.find(flag_type);
        if (it != tc.flags.end()) base_vec = it->second;
        std::vector<std::string> merged = merge_flags(base_vec, bv, tc.id, flag_type);
        if (!merged.empty()) data["flags"][flag_type] = merged;
    }

    data["sysroot"] = tc.sysroot;
    data["defines"] = tc.defines;
    data["lib_paths"] = tc.lib_paths;
    data["libs"] = tc.libs;

    std::string filename = tc.id;
    if (bv) filename += "-" + bv->id;
    filename += ".cmake";

    TemplateEngine engine;
    engine.render_to_file("toolchain.jinja2", data, output_dir / filename);
}

void ToolchainGenerator::generate_all(const std::filesystem::path& output_root) {
    std::filesystem::path toolchains_dir = output_root / "toolchains";
    std::filesystem::create_directories(toolchains_dir);

    if (metadata_.build_variants.empty()) {
        for (const auto& tc : metadata_.toolchains) {
            generate_toolchain(tc, nullptr, toolchains_dir);
        }
        return;
    }

    for (const auto& tc : metadata_.toolchains) {
        for (const auto& bv : metadata_.build_variants) {
            generate_toolchain(tc, &bv, toolchains_dir);
        }
    }
}

}  // namespace scaffolder

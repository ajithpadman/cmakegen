#include "generator/preset_generator.hpp"
#include "generator/template_engine.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

namespace scaffolder {

PresetGenerator::PresetGenerator(const Metadata& metadata) : metadata_(metadata) {}

std::string PresetGenerator::get_toolchain_for_isa(const std::string& isa) const {
    for (const auto& iv : metadata_.isa_variants) {
        if (iv.id == isa) return iv.toolchain;
    }
    return "";
}

bool PresetGenerator::is_excluded(const PresetCombination& c) const {
    for (const auto& ex : metadata_.preset_matrix.exclude) {
        if (ex.board && *ex.board != c.board) continue;
        if (ex.soc && *ex.soc != c.soc) continue;
        if (ex.isa_variant && *ex.isa_variant != c.isa_variant) continue;
        if (ex.build_variant && *ex.build_variant != c.build_variant) continue;
        return true;
    }
    return false;
}

std::vector<PresetCombination> PresetGenerator::compute_combinations() {
    std::vector<PresetCombination> result;
    for (const auto& board : metadata_.boards) {
        for (const auto& soc_id : board.socs) {
            const Soc* soc = nullptr;
            for (const auto& s : metadata_.socs) {
                if (s.id == soc_id) { soc = &s; break; }
            }
            if (!soc) continue;
            for (const auto& isa : soc->isas) {
                std::string isa_variant_id;
                for (const auto& iv : metadata_.isa_variants) {
                    if (iv.id == isa || iv.display_name.find(isa) != std::string::npos) {
                        isa_variant_id = iv.id;
                        break;
                    }
                }
                if (isa_variant_id.empty()) isa_variant_id = isa;
                for (const auto& bv : metadata_.build_variants) {
                    PresetCombination pc;
                    pc.board = board.id;
                    pc.soc = soc_id;
                    pc.isa_variant = isa_variant_id;
                    pc.build_variant = bv.id;
                    pc.toolchain_id = get_toolchain_for_isa(isa_variant_id);
                    pc.preset_name = board.id + "_" + soc_id + "_" + isa_variant_id + "_" + bv.id;
                    if (!is_excluded(pc)) result.push_back(pc);
                }
            }
        }
    }
    return result;
}

void PresetGenerator::generate(const std::filesystem::path& output_root) {
    auto combinations = compute_combinations();
    std::filesystem::path source_dir = std::filesystem::absolute(output_root);
    std::string binary_dir_pattern = metadata_.preset_matrix.binary_dir_pattern;

    nlohmann::json data;
    data["cmake_minimum"] = {
        {"major", metadata_.project.cmake_minimum.major},
        {"minor", metadata_.project.cmake_minimum.minor},
        {"patch", metadata_.project.cmake_minimum.patch}
    };
    data["configure_presets"] = nlohmann::json::array();
    data["build_presets"] = nlohmann::json::array();

    for (const auto& c : combinations) {
        std::string binary_dir = binary_dir_pattern;
        size_t pos = 0;
        while ((pos = binary_dir.find("${preset}", pos)) != std::string::npos) {
            binary_dir.replace(pos, 10, c.preset_name);
            pos += c.preset_name.size();
        }

        nlohmann::json cfg;
        cfg["name"] = c.preset_name;
        cfg["binaryDir"] = source_dir.generic_string() + "/" + binary_dir;
        std::string tc_file = c.toolchain_id;
        if (!metadata_.build_variants.empty()) tc_file += "-" + c.build_variant;
        tc_file += ".cmake";
        cfg["toolchainFile"] = source_dir.generic_string() + "/toolchains/" + tc_file;
        cfg["board"] = c.board;
        cfg["soc"] = c.soc;
        cfg["isa_variant"] = c.isa_variant;
        cfg["build_variant"] = c.build_variant;
        data["configure_presets"].push_back(cfg);

        data["build_presets"].push_back({{"name", c.preset_name}});
    }

    TemplateEngine engine;
    engine.render_to_file("cmake_presets.jinja2", data, output_root / "CMakePresets.json");
}

}  // namespace scaffolder

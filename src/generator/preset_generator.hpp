#pragma once

#include "../metadata/schema.hpp"
#include <filesystem>
#include <vector>
#include <string>

namespace scaffolder {

struct PresetCombination {
    std::string board;
    std::string soc;
    std::string isa_variant;
    std::string build_variant;
    std::string toolchain_id;
    std::string preset_name;
};

class PresetGenerator {
public:
    explicit PresetGenerator(const Metadata& metadata);
    void generate(const std::filesystem::path& output_root);

private:
    std::vector<PresetCombination> compute_combinations();
    bool is_excluded(const PresetCombination& c) const;
    std::string get_toolchain_for_isa(const std::string& isa) const;

    const Metadata& metadata_;
};

}  // namespace scaffolder

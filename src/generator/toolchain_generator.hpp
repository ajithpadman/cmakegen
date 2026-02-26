#pragma once

#include "../metadata/schema.hpp"
#include <filesystem>

namespace scaffolder {

class ToolchainGenerator {
public:
    explicit ToolchainGenerator(const Metadata& metadata);
    void generate_all(const std::filesystem::path& output_root);

private:
    void generate_toolchain(const Toolchain& tc, const BuildVariant* bv,
                            const std::filesystem::path& output_dir);
    std::vector<std::string> merge_flags(const std::vector<std::string>& base,
                                         const BuildVariant* bv, const std::string& tc_id,
                                         const std::string& flag_type) const;

    const Metadata& metadata_;
};

}  // namespace scaffolder

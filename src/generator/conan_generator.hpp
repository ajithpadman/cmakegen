#pragma once

#include "../metadata/schema.hpp"
#include <filesystem>

namespace scaffolder {

class ConanGenerator {
public:
    explicit ConanGenerator(const Metadata& metadata);
    void generate(const std::filesystem::path& output_root);

private:
    const Metadata& metadata_;
};

}  // namespace scaffolder

#include <gtest/gtest.h>
#include "metadata/parser.hpp"
#include "metadata/validator.hpp"
#include "resolver/path_resolver.hpp"
#include "copy/copy_engine.hpp"
#include "generator/cmake_generator.hpp"
#include "generator/toolchain_generator.hpp"
#include "generator/preset_generator.hpp"
#include "generator/conan_generator.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

TEST(IntegrationTest, FullPipelineMinimal) {
    std::string json = R"({
        "project": {"name": "p", "version": "0.1"},
        "source_tree": {"components": []},
        "preset_matrix": {"dimensions": ["board"], "exclude": [], "naming": "x", "binary_dir_pattern": "build"}
    })";

    scaffolder::Parser parser;
    auto meta = parser.parse_string(json, "json");

    scaffolder::Validator validator;
    EXPECT_NO_THROW(validator.validate(meta));

    fs::path tmp = fs::temp_directory_path() / "cmakegen_integ_test";
    fs::create_directories(tmp);
    fs::path output = tmp / "out";
    fs::create_directories(output);

    scaffolder::PathResolver resolver(tmp);
    scaffolder::CopyEngine copy_engine(resolver, output);
    scaffolder::CmakeGenerator cmake_gen(meta, resolver, output);
    scaffolder::ToolchainGenerator toolchain_gen(meta);
    scaffolder::PresetGenerator preset_gen(meta);
    scaffolder::ConanGenerator conan_gen(meta);

    cmake_gen.generate_all();
    toolchain_gen.generate_all(output);
    preset_gen.generate(output);
    conan_gen.generate(output);

    EXPECT_TRUE(fs::exists(output / "CMakeLists.txt"));
    EXPECT_TRUE(fs::exists(output / "conanfile.txt"));

    fs::remove_all(tmp);
}

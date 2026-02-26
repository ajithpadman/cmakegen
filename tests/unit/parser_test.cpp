#include <gtest/gtest.h>
#include <cstdlib>
#include "metadata/parser.hpp"
#include "metadata/env_expander.hpp"

TEST(ParserTest, ParseMinimalJson) {
    scaffolder::Parser parser;
    std::string json = R"({
        "project": {"name": "test", "version": "0.1"},
        "socs": [],
        "boards": [],
        "toolchains": [],
        "isa_variants": [],
        "build_variants": [],
        "source_tree": {"components": []},
        "dependencies": {},
        "preset_matrix": {"dimensions": ["board"], "naming": "x", "binary_dir_pattern": "build", "exclude": []}
    })";
    auto meta = parser.parse_string(json, "json");
    EXPECT_EQ(meta.project.name, "test");
    EXPECT_EQ(meta.project.version, "0.1");
}

TEST(ParserTest, ParseSocIsas) {
    scaffolder::Parser parser;
    std::string json = R"({
        "project": {"name": "p", "version": "0.1"},
        "socs": [
            {"id": "s1", "isas": ["cortex-m7", "cortex-m4"]},
            {"id": "s2", "isa_cores": [{"isa": "riscv32", "core_id": "rv0"}]}
        ],
        "source_tree": {"components": []},
        "preset_matrix": {"dimensions": ["board"], "naming": "x", "binary_dir_pattern": "build"}
    })";
    auto meta = parser.parse_string(json, "json");
    ASSERT_EQ(meta.socs.size(), 2u);
    EXPECT_EQ(meta.socs[0].isas[0], "cortex-m7");
    EXPECT_EQ(meta.socs[0].isas[1], "cortex-m4");
    EXPECT_EQ(meta.socs[1].isas[0], "riscv32");
}

TEST(ParserTest, ParseComponents) {
    scaffolder::Parser parser;
    std::string json = R"({
        "project": {"name": "p", "version": "0.1"},
        "source_tree": {
            "components": [
                {"id": "lib1", "type": "library", "library_type": "static", "dest": "libs/lib1"},
                {"id": "ext1", "type": "external", "conan_ref": "foo/1.0"}
            ]
        },
        "preset_matrix": {"dimensions": ["board"], "naming": "x", "binary_dir_pattern": "build"}
    })";
    auto meta = parser.parse_string(json, "json");
    ASSERT_EQ(meta.source_tree.components.size(), 2u);
    EXPECT_EQ(meta.source_tree.components[0].id, "lib1");
    EXPECT_EQ(meta.source_tree.components[0].type, "library");
    EXPECT_EQ(meta.source_tree.components[1].type, "external");
    EXPECT_EQ(meta.source_tree.components[1].conan_ref, "foo/1.0");
}

TEST(ParserTest, EnvExpansionFromJson) {
    scaffolder::Parser parser;
    std::string json = R"({
        "env": {"MY_VAR": "hello", "COMBINED": "${MY_VAR}_world"},
        "project": {"name": "${MY_VAR}", "version": "${COMBINED}"},
        "source_tree": {"components": []},
        "preset_matrix": {"dimensions": ["board"], "naming": "x", "binary_dir_pattern": "build"}
    })";
    auto meta = parser.parse_string(json, "json");
    EXPECT_EQ(meta.project.name, "hello");
    EXPECT_EQ(meta.project.version, "hello_world");
}

TEST(ParserTest, EnvExpansionFromSystem) {
#ifdef _WIN32
    _putenv_s("SCAFF_TEST_VAR", "from_system");
#else
    setenv("SCAFF_TEST_VAR", "from_system", 1);
#endif
    scaffolder::Parser parser;
    std::string json = R"({
        "project": {"name": "${SCAFF_TEST_VAR}", "version": "0.1"},
        "source_tree": {"components": []},
        "preset_matrix": {"dimensions": ["board"], "naming": "x", "binary_dir_pattern": "build"}
    })";
    auto meta = parser.parse_string(json, "json");
#ifdef _WIN32
    _putenv_s("SCAFF_TEST_VAR", "");
#else
    unsetenv("SCAFF_TEST_VAR");
#endif
    EXPECT_EQ(meta.project.name, "from_system");
}

TEST(ParserTest, EnvExpansionMissingVarThrows) {
    scaffolder::Parser parser;
    std::string json = R"({
        "project": {"name": "${MISSING_VAR_XYZ}", "version": "0.1"},
        "source_tree": {"components": []},
        "preset_matrix": {"dimensions": ["board"], "naming": "x", "binary_dir_pattern": "build"}
    })";
    EXPECT_THROW(parser.parse_string(json, "json"), scaffolder::EnvExpandError);
}

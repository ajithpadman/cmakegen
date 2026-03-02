#include <gtest/gtest.h>
#include "interactive/metadata_builder.hpp"
#include "metadata/parser.hpp"
#include "metadata/validator.hpp"
#include "resolver/path_resolver.hpp"
#include "copy/copy_engine.hpp"
#include "generator/cmake_generator.hpp"
#include "generator/toolchain_generator.hpp"
#include "generator/preset_generator.hpp"
#include "generator/conan_generator.hpp"
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

// --- normalize_id ---
TEST(MetadataBuilderTest, NormalizeIdEmpty) {
    EXPECT_EQ(scaffolder::normalize_id(""), "unknown");
}

TEST(MetadataBuilderTest, NormalizeIdSpaces) {
    EXPECT_EQ(scaffolder::normalize_id("a b c"), "a_b_c");
    EXPECT_EQ(scaffolder::normalize_id("  spaces  "), "spaces");
}

TEST(MetadataBuilderTest, NormalizeIdSpecialChars) {
    EXPECT_EQ(scaffolder::normalize_id("a@b#c"), "a_b_c");
    EXPECT_EQ(scaffolder::normalize_id("valid-id_123"), "valid-id_123");
}

TEST(MetadataBuilderTest, NormalizeIdLeadingTrailingUnderscores) {
    EXPECT_EQ(scaffolder::normalize_id("___x___"), "x");
    EXPECT_EQ(scaffolder::normalize_id("  only spaces  "), "only_spaces");
}

TEST(MetadataBuilderTest, NormalizeIdUnknownFallback) {
    EXPECT_EQ(scaffolder::normalize_id("___"), "unknown");
    EXPECT_EQ(scaffolder::normalize_id("   "), "unknown");
}

// --- split_comma_separated ---
TEST(MetadataBuilderTest, SplitCommaSeparatedEmpty) {
    auto v = scaffolder::split_comma_separated("");
    EXPECT_TRUE(v.empty());
}

TEST(MetadataBuilderTest, SplitCommaSeparatedSingle) {
    auto v = scaffolder::split_comma_separated("foo");
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], "foo");
}

TEST(MetadataBuilderTest, SplitCommaSeparatedMultiple) {
    auto v = scaffolder::split_comma_separated("a,b,c");
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], "a");
    EXPECT_EQ(v[1], "b");
    EXPECT_EQ(v[2], "c");
}

TEST(MetadataBuilderTest, SplitCommaSeparatedSpaces) {
    auto v = scaffolder::split_comma_separated(" a , b , c ");
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], "a");
    EXPECT_EQ(v[1], "b");
    EXPECT_EQ(v[2], "c");
}

// --- condition_summary ---
TEST(MetadataBuilderTest, ConditionSummaryNone) {
    std::shared_ptr<scaffolder::ConditionData> c = nullptr;
    EXPECT_EQ(scaffolder::condition_summary(c), "(none)");
}

TEST(MetadataBuilderTest, ConditionSummaryDefault) {
    auto c = std::make_shared<scaffolder::ConditionData>();
    c->default_ = true;
    EXPECT_EQ(scaffolder::condition_summary(c), "Default");
}

TEST(MetadataBuilderTest, ConditionSummaryLeaf) {
    auto c = std::make_shared<scaffolder::ConditionData>();
    c->var = "board";
    c->op = "eq";
    c->value = std::string("nucleo");
    EXPECT_EQ(scaffolder::condition_summary(c), "board eq nucleo");
}

TEST(MetadataBuilderTest, ConditionSummaryAnd) {
    auto sub = std::make_shared<scaffolder::ConditionData>();
    sub->var = "x";
    sub->op = "eq";
    sub->value = std::string("1");
    auto c = std::make_shared<scaffolder::ConditionData>();
    c->and_ = std::vector<std::shared_ptr<scaffolder::ConditionData>>{sub};
    EXPECT_EQ(scaffolder::condition_summary(c), "AND(x eq 1)");
}

TEST(MetadataBuilderTest, ConditionSummaryOr) {
    auto sub = std::make_shared<scaffolder::ConditionData>();
    sub->var = "y";
    sub->op = "in";
    sub->value = std::vector<std::string>{"a", "b"};
    auto c = std::make_shared<scaffolder::ConditionData>();
    c->or_ = std::vector<std::shared_ptr<scaffolder::ConditionData>>{sub};
    EXPECT_EQ(scaffolder::condition_summary(c), "OR(y in [a,b])");
}

TEST(MetadataBuilderTest, ConditionSummaryNot) {
    auto inner = std::make_shared<scaffolder::ConditionData>();
    inner->default_ = true;
    auto c = std::make_shared<scaffolder::ConditionData>();
    c->not_ = inner;
    EXPECT_EQ(scaffolder::condition_summary(c), "NOT(Default)");
}

TEST(MetadataBuilderTest, ConditionSummaryNestedAndOr) {
    auto leaf1 = std::make_shared<scaffolder::ConditionData>();
    leaf1->var = "a";
    leaf1->op = "eq";
    leaf1->value = std::string("1");
    auto leaf2 = std::make_shared<scaffolder::ConditionData>();
    leaf2->var = "b";
    leaf2->op = "eq";
    leaf2->value = std::string("2");
    auto and_cond = std::make_shared<scaffolder::ConditionData>();
    and_cond->and_ = std::vector<std::shared_ptr<scaffolder::ConditionData>>{leaf1, leaf2};
    auto c = std::make_shared<scaffolder::ConditionData>();
    c->or_ = std::vector<std::shared_ptr<scaffolder::ConditionData>>{and_cond};
    EXPECT_EQ(scaffolder::condition_summary(c), "OR(AND(a eq 1, b eq 2))");
}

// --- Condition -> JSON -> parse (round-trip) ---
TEST(MetadataBuilderTest, VariantConditionRoundTripAndNotDefault) {
    scaffolder::MetadataBuilder b;
    b.project.name = "p";
    b.project.version = "0.1";
    b.build_variants.push_back({});
    b.build_variants.back().id = "debug";

    scaffolder::ComponentData comp;
    comp.id = "my_variant";
    comp.type = "variant";
    comp.source = "src";
    comp.dest = "app";

    scaffolder::VariationData var;
    var.subdir = "debug_build";
    var.condition = std::make_shared<scaffolder::ConditionData>();
    var.condition->and_ = std::vector<std::shared_ptr<scaffolder::ConditionData>>{};
    auto leaf1 = std::make_shared<scaffolder::ConditionData>();
    leaf1->var = "board";
    leaf1->op = "eq";
    leaf1->value = std::string("nucleo");
    var.condition->and_->push_back(leaf1);
    auto leaf2 = std::make_shared<scaffolder::ConditionData>();
    leaf2->var = "build_variant";
    leaf2->op = "eq";
    leaf2->value = std::string("debug");
    var.condition->and_->push_back(leaf2);
    comp.variations.push_back(var);

    scaffolder::VariationData varDefault;
    varDefault.subdir = "default_build";
    varDefault.condition = std::make_shared<scaffolder::ConditionData>();
    varDefault.condition->default_ = true;
    comp.variations.push_back(varDefault);

    b.components.push_back(comp);

    std::string json = b.to_json();
    scaffolder::Parser parser;
    scaffolder::Metadata meta = parser.parse_string(json, "json");
    scaffolder::Validator validator;
    EXPECT_NO_THROW(validator.validate(meta));

    ASSERT_EQ(meta.source_tree.components.size(), 1u);
    ASSERT_TRUE(meta.source_tree.components[0].variations);
    ASSERT_EQ(meta.source_tree.components[0].variations->size(), 2u);

    const auto& cond0 = meta.source_tree.components[0].variations->at(0).condition;
    EXPECT_TRUE(cond0.and_);
    ASSERT_EQ(cond0.and_->size(), 2u);
    EXPECT_EQ((*cond0.and_)[0]->var, "board");
    ASSERT_TRUE((*cond0.and_)[0]->value);
    EXPECT_TRUE(std::holds_alternative<std::string>(*(*cond0.and_)[0]->value));
    EXPECT_EQ(std::get<std::string>(*(*cond0.and_)[0]->value), "nucleo");
    EXPECT_EQ((*cond0.and_)[1]->var, "build_variant");

    const auto& cond1 = meta.source_tree.components[0].variations->at(1).condition;
    EXPECT_TRUE(cond1.default_);
    EXPECT_EQ(*cond1.default_, true);
}

// --- Minimal project to_json + parse + validate ---
TEST(MetadataBuilderTest, MinimalProjectParseValidate) {
    scaffolder::MetadataBuilder b;
    b.project.name = "minimal";
    b.project.version = "0.1.0";

    std::string json = b.to_json();
    scaffolder::Parser parser;
    scaffolder::Metadata meta = parser.parse_string(json, "json");
    scaffolder::Validator validator;
    EXPECT_NO_THROW(validator.validate(meta));
    EXPECT_EQ(meta.project.name, "minimal");
    EXPECT_EQ(meta.project.version, "0.1.0");
    EXPECT_TRUE(meta.preset_matrix.dimensions.size() >= 1u);
}

// --- Full wizard-equivalent: all component types + parse + validate ---
TEST(MetadataBuilderTest, FullWizardEquivalentParseValidate) {
    scaffolder::MetadataBuilder b;
    b.project.name = "full_test";
    b.project.version = "1.0";

    scaffolder::SocData soc;
    soc.id = "stm32h7";
    soc.display_name = "STM32H7";
    soc.description = "Cortex-M7";
    soc.isas = {"cortex-m7"};
    b.socs.push_back(soc);

    scaffolder::BoardData board;
    board.id = "nucleo-h7";
    board.display_name = "Nucleo H7";
    board.socs = {"stm32h7"};
    b.boards.push_back(board);

    scaffolder::ToolchainData tc;
    tc.id = "arm-gcc";
    tc.display_name = "ARM GCC";
    tc.compiler.c = "arm-none-eabi-gcc";
    tc.compiler.cxx = "arm-none-eabi-g++";
    tc.compiler.asm_ = "arm-none-eabi-gcc";
    b.toolchains.push_back(tc);

    scaffolder::IsaVariantData isa;
    isa.id = "cortex-m7";
    isa.toolchain = "arm-gcc";
    isa.display_name = "Cortex-M7";
    b.isa_variants.push_back(isa);

    scaffolder::BuildVariantData bv;
    bv.id = "debug";
    b.build_variants.push_back(bv);

    // executable
    scaffolder::ComponentData exe;
    exe.id = "main_app";
    exe.type = "executable";
    exe.source = "apps/main";
    exe.dest = "apps/main_app";
    exe.dependencies = {"hal"};
    b.components.push_back(exe);

    // library with hierarchical + filters
    scaffolder::ComponentData lib;
    lib.id = "hal";
    lib.type = "library";
    lib.library_type = "static";
    lib.structure = "hierarchical";
    lib.source = "libs/hal";
    lib.dest = "libs/hal";
    lib.filters = scaffolder::PathFiltersData{};
    lib.filters->filter_mode = "include_first";
    lib.filters->include_paths = {"src"};
    lib.filters->exclude_paths = {"test"};
    b.components.push_back(lib);

    // external
    scaffolder::ComponentData ext;
    ext.id = "freertos";
    ext.type = "external";
    ext.conan_ref = "freertos/11.0";
    b.components.push_back(ext);

    // variant with variation
    scaffolder::ComponentData var_comp;
    var_comp.id = "optional_driver";
    var_comp.type = "variant";
    var_comp.source = "drivers";
    var_comp.dest = "drivers";
    scaffolder::VariationData vd;
    vd.subdir = "uart";
    vd.condition = std::make_shared<scaffolder::ConditionData>();
    vd.condition->var = "board";
    vd.condition->op = "eq";
    vd.condition->value = std::string("nucleo-h7");
    var_comp.variations.push_back(vd);
    b.components.push_back(var_comp);

    // layer
    scaffolder::ComponentData layer;
    layer.id = "stack";
    layer.type = "layer";
    layer.subdirs = {"main_app", "hal"};
    b.components.push_back(layer);

    // component with git
    scaffolder::ComponentData git_comp;
    git_comp.id = "vendor_lib";
    git_comp.type = "library";
    git_comp.source = ".";
    git_comp.dest = "vendor";
    git_comp.git_url = "https://example.com/repo.git";
    git_comp.git_tag = "v1.0";
    b.components.push_back(git_comp);

    b.dependencies.tool_requires = {"cmake/3.28"};
    b.dependencies.extra_requires = {"ninja/1.11"};

    std::string json = b.to_json();
    scaffolder::Parser parser;
    scaffolder::Metadata meta = parser.parse_string(json, "json");
    scaffolder::Validator validator;
    EXPECT_NO_THROW(validator.validate(meta));

    EXPECT_EQ(meta.project.name, "full_test");
    ASSERT_EQ(meta.source_tree.components.size(), 6u);
    EXPECT_EQ(meta.source_tree.components[0].type, "executable");
    EXPECT_EQ(meta.source_tree.components[0].id, "main_app");
    EXPECT_EQ(meta.source_tree.components[1].type, "library");
    EXPECT_EQ(meta.source_tree.components[1].structure, "hierarchical");
    EXPECT_TRUE(meta.source_tree.components[1].filters);
    EXPECT_EQ(meta.source_tree.components[2].type, "external");
    EXPECT_EQ(meta.source_tree.components[2].conan_ref, "freertos/11.0");
    EXPECT_EQ(meta.source_tree.components[3].type, "variant");
    EXPECT_EQ(meta.source_tree.components[4].type, "layer");
    EXPECT_TRUE(meta.source_tree.components[4].subdirs);
    EXPECT_EQ(meta.source_tree.components[5].git->url, "https://example.com/repo.git");
    EXPECT_EQ(meta.source_tree.components[5].git->tag, "v1.0");
    EXPECT_EQ(meta.dependencies.tool_requires.size(), 1u);
    EXPECT_EQ(meta.dependencies.extra_requires.size(), 1u);
}

// --- Preset matrix: single vs multi dimension ---
TEST(MetadataBuilderTest, PresetMatrixSingleDimension) {
    scaffolder::MetadataBuilder b;
    b.project.name = "p";
    b.build_variants.push_back({});
    b.build_variants.back().id = "debug";

    std::string json = b.to_json();
    EXPECT_TRUE(json.find("\"dimensions\"") != std::string::npos);
    EXPECT_TRUE(json.find("build_variant") != std::string::npos);
    EXPECT_TRUE(json.find("\"naming\"") != std::string::npos);
    // Single dimension typically uses naming like "{variant}"
    scaffolder::Parser parser;
    scaffolder::Metadata meta = parser.parse_string(json, "json");
    ASSERT_FALSE(meta.preset_matrix.dimensions.empty());
    EXPECT_EQ(meta.preset_matrix.dimensions[0], "build_variant");
}

TEST(MetadataBuilderTest, PresetMatrixMultiDimension) {
    scaffolder::MetadataBuilder b;
    b.project.name = "p";
    b.boards.push_back({});
    b.boards.back().id = "b1";
    b.boards.back().display_name = "B1";
    b.socs.push_back({});
    b.socs.back().id = "s1";
    b.socs.back().display_name = "S1";
    b.socs.back().isas = {"m7"};
    b.isa_variants.push_back({});
    b.isa_variants.back().id = "m7";
    b.isa_variants.back().toolchain = "gcc";
    b.isa_variants.back().display_name = "M7";
    b.build_variants.push_back({});
    b.build_variants.back().id = "debug";

    std::string json = b.to_json();
    scaffolder::Parser parser;
    scaffolder::Metadata meta = parser.parse_string(json, "json");
    EXPECT_EQ(meta.preset_matrix.dimensions.size(), 4u);
    EXPECT_EQ(meta.preset_matrix.dimensions[0], "board");
    EXPECT_EQ(meta.preset_matrix.dimensions[1], "soc");
    EXPECT_EQ(meta.preset_matrix.dimensions[2], "isa_variant");
    EXPECT_EQ(meta.preset_matrix.dimensions[3], "build_variant");
    EXPECT_TRUE(meta.preset_matrix.naming.find("board") != std::string::npos);
}

// --- Optional: generate from builder output (pipeline) ---
TEST(MetadataBuilderTest, MinimalBuilderGeneratesFiles) {
    scaffolder::MetadataBuilder b;
    b.project.name = "gen_test";
    b.project.version = "0.1";
    scaffolder::ComponentData comp;
    comp.id = "app";
    comp.type = "executable";
    comp.source = "src";
    comp.dest = "app";
    b.components.push_back(comp);

    std::string json = b.to_json();
    scaffolder::Parser parser;
    scaffolder::Metadata meta = parser.parse_string(json, "json");
    scaffolder::Validator validator;
    validator.validate(meta);

    fs::path tmp = fs::temp_directory_path() / "cmakegen_builder_test";
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

#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static fs::path find_cmakegen_exe() {
    if (fs::exists("build/cmakegen")) return "build/cmakegen";
    if (fs::exists("build/linux/cmakegen")) return "build/linux/cmakegen";
    if (fs::exists("cmakegen")) return "cmakegen";
    return "";
}

TEST(E2ETest, CmakegenRuns) {
    fs::path exe = find_cmakegen_exe();
    ASSERT_FALSE(exe.empty()) << "cmakegen executable not found";

    fs::path metadata = "tests/e2e/fixtures/minimal_metadata.json";
    ASSERT_TRUE(fs::exists(metadata)) << "Metadata fixture not found";

    fs::path output = fs::temp_directory_path() / "cmakegen_e2e_out";
    fs::create_directories(output);

    std::string cmd = exe.string() + " -o " + output.string() + " " + metadata.string();
    int ret = std::system(cmd.c_str());
    EXPECT_EQ(ret, 0);

    EXPECT_TRUE(fs::exists(output / "CMakeLists.txt"));
    EXPECT_TRUE(fs::exists(output / "conanfile.txt"));

    fs::remove_all(output);
}

TEST(E2ETest, FullMetadataAllComponentTypes) {
    const char* full_meta_env = std::getenv("E2E_FULL_METADATA");
    if (!full_meta_env || !fs::exists(full_meta_env)) {
        GTEST_SKIP() << "Full metadata not available (E2E_FULL_METADATA or file missing)";
    }

    fs::path exe = find_cmakegen_exe();
    ASSERT_FALSE(exe.empty()) << "cmakegen executable not found";

    fs::path metadata = full_meta_env;
    fs::path output = fs::temp_directory_path() / "cmakegen_full_e2e_out";
    fs::create_directories(output);

    std::string cmd = exe.string() + " -o " + output.string() + " " + metadata.string();
    int ret = std::system(cmd.c_str());
    EXPECT_EQ(ret, 0) << "cmakegen failed for full metadata";

    EXPECT_TRUE(fs::exists(output / "CMakeLists.txt"));
    EXPECT_TRUE(fs::exists(output / "conanfile.txt"));
    EXPECT_TRUE(fs::exists(output / "CMakePresets.json"));
    EXPECT_TRUE(fs::exists(output / "toolchains"));

    EXPECT_TRUE(fs::exists(output / "platform/drivers/hal/CMakeLists.txt"));
    EXPECT_TRUE(fs::exists(output / "platform/drivers/uart_driver/CMakeLists.txt"));
    EXPECT_TRUE(fs::exists(output / "platform/rtos/freertos_kernel/CMakeLists.txt"));
    EXPECT_TRUE(fs::exists(output / "apps/main_app/CMakeLists.txt"));

    EXPECT_TRUE(fs::exists(output / "platform/drivers/hal/lib.c") || fs::exists(output / "platform/drivers/hal/src/lib.c"));
    EXPECT_TRUE(fs::exists(output / "platform/rtos/freertos_kernel/tasks.c") || fs::exists(output / "platform/rtos/freertos_kernel/source/tasks.c"));

    fs::path hierarchical_cmake = output / "platform/rtos/freertos_kernel/CMakeLists.txt";
    ASSERT_TRUE(fs::exists(hierarchical_cmake)) << "Hierarchical component CMakeLists.txt not found";
    std::ifstream f(hierarchical_cmake);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    EXPECT_TRUE(content.find("add_hierarchical_library(freertos_kernel") != std::string::npos)
        << "Hierarchical CMakeLists should use add_hierarchical_library";
    EXPECT_TRUE(content.find("SOURCE_EXTENSIONS") != std::string::npos)
        << "Hierarchical CMakeLists should specify SOURCE_EXTENSIONS";
    EXPECT_TRUE(content.find("INCLUDE_EXTENSIONS") != std::string::npos)
        << "Hierarchical CMakeLists should specify INCLUDE_EXTENSIONS";

    EXPECT_TRUE(fs::exists(output / "cmake/AddHierarchicalLibrary.cmake"))
        << "cmake/AddHierarchicalLibrary.cmake helper should be deployed";

    fs::remove_all(output);
}

TEST(E2ETest, DefaultJsonGeneratesValidTemplate) {
    fs::path exe = find_cmakegen_exe();
    ASSERT_FALSE(exe.empty()) << "cmakegen executable not found";

    fs::path out_file = fs::temp_directory_path() / "cmakegen_default_json_test.json";
    std::string cmd = exe.string() + " --default-json " + out_file.string();
    int ret = std::system(cmd.c_str());
    EXPECT_EQ(ret, 0) << "default-json failed";

    ASSERT_TRUE(fs::exists(out_file)) << "Default JSON file not created";
    std::ifstream f(out_file);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    EXPECT_TRUE(content.find("\"project\"") != std::string::npos);
    EXPECT_TRUE(content.find("\"source_tree\"") != std::string::npos);
    EXPECT_TRUE(content.find("\"preset_matrix\"") != std::string::npos);

    fs::remove(out_file);
}

TEST(E2ETest, ArmCompilesCorrectly) {
    const char* arm_meta_env = std::getenv("E2E_ARM_COMPILE_METADATA");
    if (!arm_meta_env || !fs::exists(arm_meta_env)) {
        GTEST_SKIP() << "ARM compile metadata not available (E2E_ARM_COMPILE_METADATA or file missing)";
    }

    const fs::path arm_gcc = "/usr/bin/arm-none-eabi-gcc";
    if (!fs::exists(arm_gcc)) {
        GTEST_SKIP() << "ARM toolchain not found at " << arm_gcc.string();
    }

    fs::path exe = find_cmakegen_exe();
    ASSERT_FALSE(exe.empty()) << "cmakegen executable not found";

    fs::path output = fs::temp_directory_path() / "cmakegen_arm_compile_out";
    fs::create_directories(output);

    std::string gen_cmd = exe.string() + " -o " + output.string() + " " + arm_meta_env;
    int ret = std::system(gen_cmd.c_str());
    ASSERT_EQ(ret, 0) << "cmakegen failed for ARM compile metadata";

    ASSERT_TRUE(fs::exists(output / "CMakeLists.txt"));
    ASSERT_TRUE(fs::exists(output / "CMakePresets.json"));
    ASSERT_TRUE(fs::exists(output / "toolchains/arm-gcc-m7-debug.cmake"));

    const std::string preset = "nucleo-h743zi_stm32h7_cortex-m7_debug";
    std::string cfg_cmd = "cd " + output.string() + " && cmake --preset " + preset;
    ret = std::system(cfg_cmd.c_str());
    ASSERT_EQ(ret, 0) << "CMake configure failed for preset " << preset;

    std::string build_cmd = "cd " + output.string() + " && cmake --build --preset " + preset;
    ret = std::system(build_cmd.c_str());
    ASSERT_EQ(ret, 0) << "CMake build failed for preset " << preset;

    fs::path binary_dir = output / "build" / preset;
    fs::path main_app = binary_dir / "apps/main_app/main_app";
    if (!fs::exists(main_app)) {
        main_app = binary_dir / "apps/main_app/main_app.elf";
    }
    if (!fs::exists(main_app)) {
        main_app = binary_dir / "main_app";
    }
    EXPECT_TRUE(fs::exists(main_app)) << "main_app executable not found in " << binary_dir.string();

    fs::remove_all(output);
}

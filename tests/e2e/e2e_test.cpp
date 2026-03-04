#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

namespace fs = std::filesystem;

/** Run cmakegen generate -f <folder> -o <output>; returns exit code (0 = success). */
static int run_cmakegen_generate(const fs::path& exe, const fs::path& metadata_folder, const fs::path& output_dir) {
    std::string exe_s = exe.string();
    std::string folder_s = metadata_folder.string();
    std::string output_s = output_dir.string();
    std::vector<char> exe_buf(exe_s.begin(), exe_s.end());
    std::vector<char> folder_buf(folder_s.begin(), folder_s.end());
    std::vector<char> output_buf(output_s.begin(), output_s.end());
    exe_buf.push_back('\0');
    folder_buf.push_back('\0');
    output_buf.push_back('\0');
    const char* argv[] = { exe_buf.data(), "generate", "-o", output_buf.data(), "-f", folder_buf.data(), nullptr };  /* -o before -f for CLI11 parse order */
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execv(exe_buf.data(), const_cast<char* const*>(argv));
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static fs::path find_cmakegen_exe() {
    fs::path exe;
    const char* env_exe = std::getenv("E2E_CMAKEGEN_EXE");
    if (env_exe && fs::exists(env_exe)) {
        exe = env_exe;
    } else if (fs::exists("build/linux/cmakegen")) {
        exe = "build/linux/cmakegen";
    } else if (fs::exists("build/cmakegen")) {
        exe = "build/cmakegen";
    } else if (fs::exists("cmakegen")) {
        exe = "cmakegen";
    }
    if (exe.empty()) return "";
    if (!exe.is_absolute()) exe = fs::absolute(exe);
    return exe;
}

TEST(E2ETest, CmakegenRuns) {
    fs::path exe = find_cmakegen_exe();
    ASSERT_FALSE(exe.empty()) << "cmakegen executable not found";

    fs::path metadata_folder = "tests/e2e/fixtures/minimal_metadata";
    if (!metadata_folder.is_absolute())
        metadata_folder = fs::absolute(metadata_folder);
    ASSERT_TRUE(fs::is_directory(metadata_folder)) << "Metadata folder fixture not found";

    fs::path output = fs::temp_directory_path() / "cmakegen_e2e_out";
    fs::create_directories(output);

    int ret = run_cmakegen_generate(exe, metadata_folder, output);
    EXPECT_EQ(ret, 0);

    EXPECT_TRUE(fs::exists(output / "CMakeLists.txt"));
    EXPECT_TRUE(fs::exists(output / "conanfile.txt"));

    fs::remove_all(output);
}

TEST(E2ETest, FullMetadataAllComponentTypes) {
    const char* full_meta_env = std::getenv("E2E_FULL_METADATA");
    if (!full_meta_env || !fs::exists(full_meta_env)) {
        GTEST_SKIP() << "Full metadata not available (E2E_FULL_METADATA or folder missing)";
    }
    fs::path metadata_folder = full_meta_env;
    ASSERT_TRUE(fs::is_directory(metadata_folder)) << "E2E_FULL_METADATA must be a folder path";

    fs::path exe = find_cmakegen_exe();
    ASSERT_FALSE(exe.empty()) << "cmakegen executable not found";

    fs::path output = fs::temp_directory_path() / "cmakegen_full_e2e_out";
    fs::create_directories(output);

    int ret = run_cmakegen_generate(exe, metadata_folder, output);
    EXPECT_EQ(ret, 0) << "cmakegen generate failed for full metadata";

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

TEST(E2ETest, GenerateFromMinimalMetadataFolder) {
    fs::path exe = find_cmakegen_exe();
    ASSERT_FALSE(exe.empty()) << "cmakegen executable not found";

    fs::path metadata_folder = "tests/e2e/fixtures/minimal_metadata";
    ASSERT_TRUE(fs::is_directory(metadata_folder)) << "Metadata folder fixture not found";

    fs::path out_dir = fs::temp_directory_path() / "cmakegen_minimal_folder_out";
    fs::create_directories(out_dir);
    fs::path meta_abs = metadata_folder.is_absolute() ? metadata_folder : fs::absolute(metadata_folder);
    int ret = run_cmakegen_generate(exe, meta_abs, out_dir);
    EXPECT_EQ(ret, 0) << "generate failed";

    EXPECT_TRUE(fs::exists(out_dir / "CMakeLists.txt"));
    fs::remove_all(out_dir);
}

TEST(E2ETest, ArmCompilesCorrectly) {
    const char* arm_meta_env = std::getenv("E2E_ARM_COMPILE_METADATA");
    if (!arm_meta_env || !fs::exists(arm_meta_env)) {
        GTEST_SKIP() << "ARM compile metadata not available (E2E_ARM_COMPILE_METADATA or folder missing)";
    }
    fs::path metadata_folder = arm_meta_env;
    ASSERT_TRUE(fs::is_directory(metadata_folder)) << "E2E_ARM_COMPILE_METADATA must be a folder path";

    const fs::path arm_gcc = "/usr/bin/arm-none-eabi-gcc";
    if (!fs::exists(arm_gcc)) {
        GTEST_SKIP() << "ARM toolchain not found at " << arm_gcc.string();
    }

    fs::path exe = find_cmakegen_exe();
    ASSERT_FALSE(exe.empty()) << "cmakegen executable not found";

    fs::path output = fs::temp_directory_path() / "cmakegen_arm_compile_out";
    fs::create_directories(output);

    int ret = run_cmakegen_generate(exe, metadata_folder, output);
    ASSERT_EQ(ret, 0) << "cmakegen generate failed for ARM compile metadata";

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

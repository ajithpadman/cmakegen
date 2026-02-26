#include "util/executable_path.hpp"
#include <filesystem>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace scaffolder {

std::filesystem::path get_executable_directory() {
#if defined(_WIN32) || defined(_WIN64)
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(nullptr, path, MAX_PATH) == 0) return {};
    std::filesystem::path p(path);
    return p.parent_path();
#else
    char path[4096];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len <= 0) return {};
    path[len] = '\0';
    std::filesystem::path p(path);
    return p.parent_path();
#endif
}

}  // namespace scaffolder

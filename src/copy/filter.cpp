#include "copy/filter.hpp"
#include <algorithm>
#include <regex>

namespace scaffolder {

Filter::Filter(const PathFilters& filters) : filters_(filters) {}

bool Filter::matches_glob(const std::string& pattern, const std::string& path_str) const {
    std::string pat = pattern;
    std::replace(pat.begin(), pat.end(), '\\', '/');
    std::string regex_pat;
    for (size_t i = 0; i < pat.size(); ++i) {
        if (pat[i] == '*') {
            if (i + 1 < pat.size() && pat[i + 1] == '*') {
                regex_pat += ".*";
                ++i;
            } else {
                regex_pat += "[^/]*";
            }
        } else if (pat[i] == '?') {
            regex_pat += "[^/]";
        } else if (pat[i] == '.') {
            regex_pat += "\\.";
        } else {
            regex_pat += pat[i];
        }
    }
    try {
        std::regex re(regex_pat);
        return std::regex_match(path_str, re);
    } catch (...) {
        return path_str.find(pat) != std::string::npos;
    }
}

bool Filter::should_include(const std::filesystem::path& path, const std::filesystem::path& base) const {
    std::filesystem::path rel;
    try {
        rel = std::filesystem::relative(path, base);
    } catch (...) {
        rel = path;
    }
    std::string path_str = rel.generic_string();
    if (path_str.empty() || path_str == ".") return true;

    auto matches_include = [this, &path_str]() {
        if (filters_.include_paths.empty()) return true;
        for (const auto& inc : filters_.include_paths) {
            if (matches_glob(inc, path_str) || path_str.find(inc) != std::string::npos) return true;
        }
        return false;
    };
    auto matches_exclude = [this, &path_str]() {
        for (const auto& exc : filters_.exclude_paths) {
            if (matches_glob(exc, path_str) || path_str.find(exc) != std::string::npos) return true;
        }
        return false;
    };

    bool inc = matches_include();
    bool exc = matches_exclude();

    if (filters_.filter_mode == FilterMode::ExcludeFirst) {
        return !exc || inc;
    } else {
        return inc && !exc;
    }
}

bool Filter::matches_extension(const std::filesystem::path& path, const std::vector<std::string>& extensions) const {
    std::string ext = path.extension().string();
    if (ext.empty()) return false;
    for (const auto& e : extensions) {
        if (e.empty()) continue;
        if (e[0] == '*') {
            std::string suffix = e.substr(1);
            if (suffix == ext || (suffix[0] == '.' && ext == suffix)) return true;
        } else {
            std::string e_ext = e[0] == '.' ? e : "." + e;
            if (ext == e_ext) return true;
        }
    }
    return false;
}

}  // namespace scaffolder

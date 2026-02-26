#include "resolver/git_cloner.hpp"
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace scaffolder {

GitCloner::GitCloner(const std::filesystem::path& cache_dir) : cache_dir_(cache_dir) {}

std::string GitCloner::get_ref_spec(const GitSource& git) const {
    if (git.tag) return *git.tag;
    if (git.branch) return *git.branch;
    if (git.commit) return *git.commit;
    return "main";
}

std::filesystem::path GitCloner::clone(const GitSource& git, const std::string& component_id) {
    std::filesystem::path dest = cache_dir_ / component_id;
    if (std::filesystem::exists(dest)) return dest;

    std::filesystem::create_directories(cache_dir_);
    std::string ref = get_ref_spec(git);
    std::string cmd;

    if (git.tag || git.branch) {
        cmd = "git clone --depth 1 --branch \"" + ref + "\" \"" + git.url + "\" \"" + dest.string() + "\"";
    } else {
        cmd = "git clone \"" + git.url + "\" \"" + dest.string() + "\" && cd \"" + dest.string() + "\" && git checkout " + ref;
    }

    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        throw std::runtime_error("git clone failed for " + component_id + ": " + git.url + " (ref: " + ref + ")");
    }
    return dest;
}

}  // namespace scaffolder

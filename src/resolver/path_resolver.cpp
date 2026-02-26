#include "resolver/path_resolver.hpp"

namespace scaffolder {

PathResolver::PathResolver(const std::filesystem::path& base_dir) : base_dir_(base_dir) {}

std::filesystem::path PathResolver::resolve(const std::string& path) const {
    std::filesystem::path p(path);
    if (!p.is_absolute()) {
        p = base_dir_ / p;
    }
    return std::filesystem::absolute(p);
}

void PathResolver::set_resolved_source(const std::string& comp_id, const std::filesystem::path& path) {
    resolved_sources_[comp_id] = path;
}

std::filesystem::path PathResolver::resolve_source(const SwComponent& comp) const {
    auto it = resolved_sources_.find(comp.id);
    if (it != resolved_sources_.end()) return it->second;
    if (!comp.source) return {};
    return resolve(*comp.source);
}

std::filesystem::path PathResolver::resolve_dest(const SwComponent& comp, const std::filesystem::path& output_root) const {
    if (!comp.dest) return {};
    std::string d = *comp.dest;
    if (d == "." || d.empty()) return output_root;
    return output_root / d;
}

}  // namespace scaffolder

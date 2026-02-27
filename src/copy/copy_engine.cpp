#include "copy/copy_engine.hpp"
#include <fstream>
#include <iostream>

namespace scaffolder {

CopyEngine::CopyEngine(PathResolver& resolver, const std::filesystem::path& output_root)
    : resolver_(resolver), output_root_(output_root) {}

void CopyEngine::copy_file(const std::filesystem::path& src, const std::filesystem::path& dest) {
    std::filesystem::create_directories(dest.parent_path());
    std::filesystem::copy_file(src, dest, std::filesystem::copy_options::overwrite_existing);
}

void CopyEngine::copy_tree(const SwComponent& comp, const std::filesystem::path& src, const std::filesystem::path& dest) {
    PathFilters pf = comp.filters.value_or(PathFilters{});
    Filter filter(pf);

    auto src_ext = comp.source_extensions.value_or(std::vector<std::string>{"*.c", "*.cpp", "*.cc"});
    auto inc_ext = comp.include_extensions.value_or(std::vector<std::string>{"*.h", "*.hpp"});
    auto meta_ext = comp.metadata_extensions.value_or(std::vector<std::string>{});

    for (auto it = std::filesystem::recursive_directory_iterator(src, std::filesystem::directory_options::skip_permission_denied);
         it != std::filesystem::recursive_directory_iterator(); ++it) {
        const auto& entry = *it;
        if (!filter.should_include(entry.path(), src)) continue;

        if (entry.is_directory()) continue;

        bool is_source = filter.matches_extension(entry.path(), src_ext);
        bool is_include = filter.matches_extension(entry.path(), inc_ext);
        bool is_metadata = !meta_ext.empty() && filter.matches_extension(entry.path(), meta_ext);
        if (!is_source && !is_include && !is_metadata) continue;

        std::filesystem::path rel = std::filesystem::relative(entry.path(), src);
        std::filesystem::path dest_path = dest / rel;
        copy_file(entry.path(), dest_path);
    }
}

void CopyEngine::copy_component(const SwComponent& comp) {
    if (comp.type == "external" || comp.type == "layer") return;
    if ((!comp.source && !comp.git) || !comp.dest) return;

    if (comp.type == "variant") {
        std::filesystem::path src = resolver_.resolve_source(comp);
        std::filesystem::path dest_path = resolver_.resolve_dest(comp, output_root_);
        if (std::filesystem::exists(src)) {
            PathFilters pf = comp.filters.value_or(PathFilters{});
            Filter filter(pf);
            for (auto it = std::filesystem::recursive_directory_iterator(src, std::filesystem::directory_options::skip_permission_denied);
                 it != std::filesystem::recursive_directory_iterator(); ++it) {
                if (!it->is_regular_file()) continue;
                if (!filter.should_include(it->path(), src)) continue;
                std::filesystem::path rel = std::filesystem::relative(it->path(), src);
                copy_file(it->path(), dest_path / rel);
            }
        }
        return;
    }

    std::filesystem::path src = resolver_.resolve_source(comp);
    std::filesystem::path dest_path = resolver_.resolve_dest(comp, output_root_);

    if (!std::filesystem::exists(src)) return;

    if (comp.structure == "hierarchical") {
        copy_tree(comp, src, dest_path);
    } else {
        copy_tree(comp, src, dest_path);
    }
}

}  // namespace scaffolder

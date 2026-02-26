#include "metadata/validator.hpp"
#include <algorithm>
#include <unordered_set>

namespace scaffolder {

void Validator::validate(const Metadata& metadata) {
    errors_.clear();
    validate_project(metadata.project);
    validate_components(metadata.source_tree.components);
    validate_preset_matrix(metadata.preset_matrix);
    if (!errors_.empty()) {
        std::string msg;
        for (const auto& e : errors_) msg += e + "; ";
        throw ValidationError(msg);
    }
}

void Validator::validate_project(const Project& p) {
    if (p.name.empty()) errors_.push_back("project.name is required");
    if (p.version.empty()) errors_.push_back("project.version is required");
}

void Validator::validate_components(const std::vector<SwComponent>& components) {
    std::unordered_set<std::string> ids;
    for (const auto& c : components) {
        if (c.id.empty()) errors_.push_back("component id is required");
        if (ids.count(c.id)) errors_.push_back("duplicate component id: " + c.id);
        ids.insert(c.id);

        if (c.type == "executable" && !c.source && !c.git) errors_.push_back("executable " + c.id + " requires source or git");
        if (c.type == "library" && !c.source && !c.git && c.structure != "hierarchical") {
            if (c.library_type != "interface") errors_.push_back("library " + c.id + " requires source or git");
        }
        if (c.type == "variant" && !c.source && !c.git) errors_.push_back("variant " + c.id + " requires source or git");
        if (c.git && c.git->url.empty()) errors_.push_back("component " + c.id + " git.url is required");
        if (c.type == "external" && !c.conan_ref) errors_.push_back("external " + c.id + " requires conan_ref");
        if (c.type == "variant" && (!c.variations || c.variations->empty())) errors_.push_back("variant " + c.id + " requires variations");
        if (c.type == "layer" && (!c.subdirs || c.subdirs->empty())) errors_.push_back("layer " + c.id + " requires subdirs");
    }

    for (const auto& c : components) {
        if (c.dependencies) {
            for (const auto& dep : *c.dependencies) {
                if (!ids.count(dep)) errors_.push_back("component " + c.id + " depends on unknown " + dep);
            }
        }
        if (c.subdirs) {
            for (const auto& sub : *c.subdirs) {
                if (!ids.count(sub)) errors_.push_back("layer " + c.id + " references unknown subdir " + sub);
            }
        }
    }
}

void Validator::validate_preset_matrix(const PresetMatrix& pm) {
    if (pm.dimensions.empty()) errors_.push_back("preset_matrix.dimensions is required");
}

}  // namespace scaffolder

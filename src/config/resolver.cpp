#include "config/resolver.hpp"
#include <sstream>

namespace scaffolder {

namespace {

std::vector<std::string> errors;

void add_error(const std::string& msg) {
    errors.push_back(msg);
}

bool id_in(const std::string& id, const std::vector<std::string>& ids) {
    for (const auto& x : ids) if (x == id) return true;
    return false;
}

}  // namespace

std::vector<std::string> resolve_errors(const Metadata& metadata) {
    errors.clear();

    std::vector<std::string> soc_ids;
    for (const auto& s : metadata.socs) soc_ids.push_back(s.id);

    std::vector<std::string> tc_ids;
    for (const auto& t : metadata.toolchains) tc_ids.push_back(t.id);

    std::vector<std::string> bv_ids;
    for (const auto& bv : metadata.build_variants) bv_ids.push_back(bv.id);

    std::vector<std::string> comp_ids;
    for (const auto& c : metadata.source_tree.components) comp_ids.push_back(c.id);

    // ISA variants: toolchain must exist
    for (const auto& iv : metadata.isa_variants) {
        if (!iv.toolchain.empty() && !id_in(iv.toolchain, tc_ids)) {
            add_error("ISA variant '" + iv.id + "' references toolchain '" + iv.toolchain + "' which does not exist.");
        }
    }

    // Boards: each soc must exist
    for (const auto& b : metadata.boards) {
        for (const auto& soc_id : b.socs) {
            if (!id_in(soc_id, soc_ids)) {
                add_error("Board '" + b.id + "' references SOC '" + soc_id + "' which does not exist.");
            }
        }
    }

    // Components: dependencies must reference existing component ids (library or external)
    for (const auto& c : metadata.source_tree.components) {
        if (c.dependencies) {
            for (const auto& dep : *c.dependencies) {
                if (!id_in(dep, comp_ids)) {
                    add_error("Component '" + c.id + "' dependency '" + dep + "' does not exist.");
                }
            }
        }
        if (c.type == "layer" && c.subdirs) {
            for (const auto& sub : *c.subdirs) {
                if (!id_in(sub, comp_ids)) {
                    add_error("Layer '" + c.id + "' subdir '" + sub + "' does not exist.");
                }
            }
        }
    }

    // Preset matrix: exclude rules reference existing ids
    for (const auto& ex : metadata.preset_matrix.exclude) {
        if (ex.board) {
            bool found = false;
            for (const auto& b : metadata.boards) if (b.id == *ex.board) { found = true; break; }
            if (!found) add_error("Preset exclude references board '" + *ex.board + "' which does not exist.");
        }
        if (ex.soc) {
            if (!id_in(*ex.soc, soc_ids)) add_error("Preset exclude references SOC '" + *ex.soc + "' which does not exist.");
        }
        if (ex.isa_variant) {
            bool found = false;
            for (const auto& iv : metadata.isa_variants) if (iv.id == *ex.isa_variant) { found = true; break; }
            if (!found) add_error("Preset exclude references isa_variant '" + *ex.isa_variant + "' which does not exist.");
        }
        if (ex.build_variant) {
            if (!id_in(*ex.build_variant, bv_ids)) add_error("Preset exclude references build_variant '" + *ex.build_variant + "' which does not exist.");
        }
    }

    // Build variants: inherits must exist
    for (const auto& bv : metadata.build_variants) {
        if (bv.inherits && !id_in(*bv.inherits, bv_ids)) {
            add_error("Build variant '" + bv.id + "' inherits '" + *bv.inherits + "' which does not exist.");
        }
    }

    return errors;
}

void resolve(const Metadata& metadata) {
    auto errs = resolve_errors(metadata);
    if (errs.empty()) return;
    std::ostringstream os;
    for (size_t i = 0; i < errs.size(); ++i) {
        if (i) os << "\n";
        os << errs[i];
    }
    throw ResolveError(os.str());
}

}  // namespace scaffolder

#pragma once

#include "interactive/metadata_builder.hpp"
#include <string>

namespace scaffolder {

// Runs the interactive FTXUI wizard, fills `builder`, and writes JSON to `output_path`.
// Returns true on success (user completed and file written), false on cancel or error.
bool run_interactive(MetadataBuilder& builder, const std::string& output_path);

}  // namespace scaffolder

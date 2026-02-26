#pragma once

#include <string>
#include <ostream>

namespace scaffolder {

// Generates a default metadata JSON template with empty/default values for user to fill in.
std::string generate_default_json();

void write_default_json(std::ostream& out);

}  // namespace scaffolder

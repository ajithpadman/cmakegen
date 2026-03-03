#include "interactive/prompt_runner.hpp"
#include "interactive/wizard.hpp"

namespace scaffolder {

bool run_interactive(MetadataBuilder& builder, const std::string& output_path) {
    MetadataWizard wizard(builder, output_path);
    return wizard.Run();
}

}  // namespace scaffolder

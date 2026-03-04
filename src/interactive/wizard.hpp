#pragma once

#include "interactive/metadata_builder.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace ftxui {
class ScreenInteractive;
}

namespace scaffolder {

/** Run the full condition editor (simple + compound AND/OR/NOT). Used by init wizard and add-component variant flow. */
void run_condition_editor(ftxui::ScreenInteractive& screen, std::shared_ptr<ConditionData>& cond);

enum class WizardState {
    Project,
    Socs,
    Boards,
    Toolchains,
    IsaVariants,
    BuildVariants,
    Components,
    Dependencies,
    Output,
    Done
};

struct WizardContext {
    MetadataBuilder* builder = nullptr;
    std::string output_path;
    bool generated = false;
};

class IWizardScreen {
public:
    virtual ~IWizardScreen() = default;
    virtual WizardState Run(ftxui::ScreenInteractive& screen, WizardContext& ctx) = 0;
};

class MetadataWizard {
public:
    MetadataWizard(MetadataBuilder& builder, std::string output_path);
    ~MetadataWizard();
    bool Run();

private:
    MetadataBuilder& builder_;
    std::string output_path_;
    std::unordered_map<WizardState, std::unique_ptr<IWizardScreen>> screens_;
};

}  // namespace scaffolder

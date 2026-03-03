#include "interactive/wizard.hpp"
#include "interactive/metadata_builder.hpp"
#include "metadata/parser.hpp"
#include "metadata/validator.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <fstream>

namespace scaffolder {

using namespace ftxui;

namespace {

// --- ConditionEditorScreen (sub-flow, not IWizardScreen) ---
class ConditionEditorScreen {
public:
    static void Run(ftxui::ScreenInteractive& screen, std::shared_ptr<ConditionData>& cond) {
        std::vector<std::string> type_names = { "Simple (var op value)", "AND (all of)", "OR (at least one)", "NOT (negate)", "Default" };
        std::vector<std::string> op_names = { "in", "equals", "not_in" };
        int type_sel = 0;
        std::string simple_var, simple_value;
        int simple_op_sel = 0;
        bool exit_done = false;
        bool exit_reset = false;
        std::shared_ptr<ConditionData>* exit_edit_sub = nullptr;
        size_t exit_remove_idx = 0;
        bool exit_remove = false;
        bool exit_add_sub = false;
        bool exit_after_create = false;

        for (;;) {
            exit_done = false;
            exit_reset = false;
            exit_edit_sub = nullptr;
            exit_remove = false;
            exit_add_sub = false;
            exit_after_create = false;

            if (cond) {
                if (cond->default_ && *cond->default_) type_sel = 4;
                else if (cond->and_) type_sel = 1;
                else if (cond->or_) type_sel = 2;
                else if (cond->not_) type_sel = 3;
                else type_sel = 0;
                if (type_sel == 0 && cond->var) simple_var = *cond->var;
                if (type_sel == 0 && cond->op) {
                    std::string op = *cond->op;
                    if (op == "in") simple_op_sel = 0;
                    else if (op == "equals") simple_op_sel = 1;
                    else simple_op_sel = 2;
                }
                if (type_sel == 0 && cond->value) {
                    std::visit([&simple_value](const auto& v) {
                        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) simple_value = v;
                        else { simple_value.clear(); for (size_t i = 0; i < v.size(); ++i) simple_value += (i ? "," : "") + v[i]; }
                    }, *cond->value);
                }
            }

            auto type_radio = Radiobox(&type_names, &type_sel);
            auto simple_var_i = Input(&simple_var, "e.g. SOC");
            auto simple_op_radio = Radiobox(&op_names, &simple_op_sel);
            auto simple_value_i = Input(&simple_value, "comma-separated");
            auto create_btn = Button("Create condition", [&] {
                cond = std::make_shared<ConditionData>();
                if (type_sel == 0) {
                    cond->var = simple_var;
                    cond->op = op_names[simple_op_sel];
                    auto v = split_comma_separated(simple_value);
                    if (v.size() == 1) cond->value = v[0];
                    else cond->value = v;
                } else if (type_sel == 1) cond->and_ = std::vector<std::shared_ptr<ConditionData>>();
                else if (type_sel == 2) cond->or_ = std::vector<std::shared_ptr<ConditionData>>();
                else if (type_sel == 3) cond->not_ = std::make_shared<ConditionData>();
                else if (type_sel == 4) cond->default_ = true;
                exit_after_create = true;
                screen.Exit();
            });
            auto done_btn = Button("Done", [&] {
                if (cond && type_sel == 0) {
                    cond->var = simple_var;
                    cond->op = op_names[simple_op_sel];
                    auto v = split_comma_separated(simple_value);
                    if (v.size() == 1) cond->value = v[0];
                    else cond->value = v;
                }
                exit_done = true;
                screen.Exit();
            });
            auto reset_btn = Button("Reset", [&] { cond = nullptr; exit_reset = true; screen.Exit(); });
            auto back_btn = Button("Back (cancel)", [&] { exit_done = true; screen.Exit(); });

            Components content_children;
            if (!cond) {
                content_children = { type_radio, create_btn, back_btn };
            } else {
                content_children.push_back(type_radio);
                if (type_sel == 0) {
                    content_children.push_back(simple_var_i);
                    content_children.push_back(simple_op_radio);
                    content_children.push_back(simple_value_i);
                } else if (type_sel == 1 && cond->and_) {
                    for (size_t i = 0; i < cond->and_->size(); ++i) {
                        auto* sub = &(*cond->and_)[i];
                        content_children.push_back(Button("Edit sub " + std::to_string(i + 1) + ": " + condition_summary(*sub), [&, sub] { exit_edit_sub = sub; screen.Exit(); }));
                        content_children.push_back(Button("Remove", [&, i] { exit_remove = true; exit_remove_idx = i; screen.Exit(); }));
                    }
                    content_children.push_back(Button("Add sub-condition", [&] { exit_add_sub = true; screen.Exit(); }));
                } else if (type_sel == 2 && cond->or_) {
                    for (size_t i = 0; i < cond->or_->size(); ++i) {
                        auto* sub = &(*cond->or_)[i];
                        content_children.push_back(Button("Edit sub " + std::to_string(i + 1) + ": " + condition_summary(*sub), [&, sub] { exit_edit_sub = sub; screen.Exit(); }));
                        content_children.push_back(Button("Remove", [&, i] { exit_remove = true; exit_remove_idx = i; screen.Exit(); }));
                    }
                    content_children.push_back(Button("Add sub-condition", [&] { exit_add_sub = true; screen.Exit(); }));
                } else if (type_sel == 3 && cond->not_) {
                    content_children.push_back(Button("Edit sub: " + condition_summary(*cond->not_), [&] { exit_edit_sub = &*cond->not_; screen.Exit(); }));
                }
                content_children.push_back(done_btn);
                content_children.push_back(reset_btn);
            }

            auto content = Container::Vertical(content_children);
            auto render = Renderer(content, [&]() -> Element {
                Elements lines;
                lines.push_back(text("=== Edit condition ===") | bold);
                for (const auto& c : content_children) lines.push_back(c->Render());
                return vbox(std::move(lines)) | border | frame;
            });

            screen.Loop(render);

            if (exit_after_create) continue;
            if (exit_done) return;
            if (exit_reset) { cond = nullptr; return; }
            if (exit_edit_sub) { Run(screen, *exit_edit_sub); continue; }
            if (exit_remove && cond) {
                if (type_sel == 1 && cond->and_ && exit_remove_idx < cond->and_->size())
                    cond->and_->erase(cond->and_->begin() + static_cast<ptrdiff_t>(exit_remove_idx));
                if (type_sel == 2 && cond->or_ && exit_remove_idx < cond->or_->size())
                    cond->or_->erase(cond->or_->begin() + static_cast<ptrdiff_t>(exit_remove_idx));
                continue;
            }
            if (exit_add_sub && cond && type_sel != 3) {
                if (type_sel == 1) { if (!cond->and_) cond->and_ = std::vector<std::shared_ptr<ConditionData>>(); cond->and_->push_back(std::make_shared<ConditionData>()); }
                if (type_sel == 2) { if (!cond->or_) cond->or_ = std::vector<std::shared_ptr<ConditionData>>(); cond->or_->push_back(std::make_shared<ConditionData>()); }
                continue;
            }
        }
    }
};

// --- ProjectScreen ---
class ProjectScreen : public IWizardScreen {
public:
    WizardState Run(ftxui::ScreenInteractive& screen, WizardContext& ctx) override {
        MetadataBuilder& b = *ctx.builder;
        std::string name, version = "0.1.0";
        int cmake_sel = 0;
        std::vector<std::string> cmake_opt = {"Use default (3.23.0)", "Custom (skip for now)"};

        auto name_input = Input(&name, "Project name");
        auto version_input = Input(&version, "0.1.0");
        auto cmake_radio = Radiobox(&cmake_opt, &cmake_sel);
        auto next_btn = Button("Next", [&] {
            b.project.name = name.empty() ? "my_project" : name;
            b.project.version = version.empty() ? "0.1.0" : version;
            if (cmake_sel == 1) { b.project.cmake_major = 3; b.project.cmake_minor = 23; b.project.cmake_patch = 0; }
            screen.Exit();
        });

        auto layout = Container::Vertical({ name_input, version_input, cmake_radio, next_btn });
        auto render = Renderer(layout, [&]() -> Element {
            return vbox({
                text("=== Project ===") | bold,
                hbox(text("Name: "), name_input->Render()),
                hbox(text("Version: "), version_input->Render()),
                text("CMake minimum: "),
                cmake_radio->Render(),
                next_btn->Render() | border
            }) | border;
        });
        screen.Loop(render);
        return WizardState::Socs;
    }
};

// --- SocsScreen ---
class SocsScreen : public IWizardScreen {
public:
    WizardState Run(ftxui::ScreenInteractive& screen, WizardContext& ctx) override {
        MetadataBuilder& b = *ctx.builder;
        std::string name, desc, isas;

        auto name_input = Input(&name, "e.g. STM32H7 Series");
        auto desc_input = Input(&desc, "optional");
        auto isas_input = Input(&isas, "e.g. cortex-m7");
        auto add_btn = Button("Add SOC", [&] {
            if (!name.empty()) {
                SocData s;
                s.display_name = name;
                s.id = normalize_id(name);
                s.description = desc;
                s.isas = split_comma_separated(isas);
                b.socs.push_back(s);
                name.clear(); desc.clear(); isas.clear();
            }
        });
        auto next_btn = Button("Next section", screen.ExitLoopClosure());

        auto layout = Container::Vertical({
            name_input, desc_input, isas_input,
            Container::Horizontal({ add_btn, next_btn })
        });
        auto render = Renderer(layout, [&]() -> Element {
            return vbox({
                text("=== SOCs (optional) ===") | bold,
                text("SOC name: "), name_input->Render(),
                text("Description (optional): "), desc_input->Render(),
                text("ISAs (comma-separated): "), isas_input->Render(),
                text("Added: " + std::to_string(b.socs.size()) + " SOC(s)"),
                hbox(add_btn->Render() | border, text(" "), next_btn->Render() | border)
            }) | border;
        });
        screen.Loop(render);
        return WizardState::Boards;
    }
};

// --- BoardsScreen ---
class BoardsScreen : public IWizardScreen {
public:
    WizardState Run(ftxui::ScreenInteractive& screen, WizardContext& ctx) override {
        MetadataBuilder& b = *ctx.builder;
        std::string name, soc_ids, defines;

        auto name_input = Input(&name, "e.g. Nucleo H743ZI");
        auto soc_input = Input(&soc_ids, "comma-separated SOC ids");
        auto defines_input = Input(&defines, "e.g. BOARD_NUCLEO_H743");
        auto add_btn = Button("Add board", [&] {
            if (!name.empty()) {
                BoardData brd;
                brd.display_name = name;
                brd.id = normalize_id(name);
                brd.socs = split_comma_separated(soc_ids);
                brd.defines = split_comma_separated(defines);
                b.boards.push_back(brd);
                name.clear(); defines.clear(); soc_ids.clear();
            }
        });
        auto next_btn = Button("Next section", screen.ExitLoopClosure());

        auto layout = Container::Vertical({
            name_input, soc_input, defines_input,
            Container::Horizontal({ add_btn, next_btn })
        });
        auto render = Renderer(layout, [&]() -> Element {
            return vbox({
                text("=== Boards (optional) ===") | bold,
                text("Board name: "), name_input->Render(),
                text("SOC(s) (comma-separated ids): "), soc_input->Render(),
                text("Defines (comma-separated): "), defines_input->Render(),
                text("Added: " + std::to_string(b.boards.size()) + " board(s)"),
                hbox(add_btn->Render() | border, text(" "), next_btn->Render() | border)
            }) | border;
        });
        screen.Loop(render);
        return WizardState::Toolchains;
    }
};

// --- ToolchainsScreen ---
class ToolchainsScreen : public IWizardScreen {
public:
    WizardState Run(ftxui::ScreenInteractive& screen, WizardContext& ctx) override {
        MetadataBuilder& b = *ctx.builder;
        std::string tname, c_comp = "arm-none-eabi-gcc", cxx_comp, asm_comp;
        std::string c_flags = "-mcpu=cortex-m7,-mthumb", cxx_flags, asm_flags, link_flags = "-mcpu=cortex-m7,-mthumb,-specs=nano.specs";
        std::string libs = "c,nosys", defs, sysroot;

        auto next_btn = Button("Next section", screen.ExitLoopClosure());
        auto add_btn = Button("Add toolchain", [&] {
            if (!tname.empty()) {
                ToolchainData t;
                t.display_name = tname;
                t.id = normalize_id(tname);
                t.compiler.c = c_comp.empty() ? "gcc" : c_comp;
                if (!cxx_comp.empty()) {
                    t.compiler.cxx = cxx_comp;
                } else {
                    std::string c = t.compiler.c;
                    size_t pos = c.rfind("gcc");
                    if (pos != std::string::npos)
                        t.compiler.cxx = c.substr(0, pos) + "g++" + c.substr(pos + 3);
                    else
                        t.compiler.cxx = c;
                }
                t.compiler.asm_ = asm_comp.empty() ? t.compiler.c : asm_comp;
                t.flags["c"] = split_comma_separated(c_flags);
                t.flags["cxx"] = split_comma_separated(cxx_flags.empty() ? c_flags : cxx_flags);
                t.flags["asm"] = split_comma_separated(asm_flags.empty() ? c_flags : asm_flags);
                t.flags["linker"] = split_comma_separated(link_flags);
                t.libs = split_comma_separated(libs);
                t.defines = split_comma_separated(defs);
                t.sysroot = sysroot;
                b.toolchains.push_back(t);
                tname.clear();
            }
        });

        auto tname_i = Input(&tname, "e.g. ARM GCC M7");
        auto c_i = Input(&c_comp, "arm-none-eabi-gcc");
        auto cxx_i = Input(&cxx_comp, "same as C");
        auto asm_i = Input(&asm_comp, "same as C");
        auto cf_i = Input(&c_flags, "-mcpu=cortex-m7,-mthumb");
        auto cxxf_i = Input(&cxx_flags, "same as C");
        auto asmf_i = Input(&asm_flags, "same as C");
        auto lf_i = Input(&link_flags, "linker flags");
        auto libs_i = Input(&libs, "c,nosys");
        auto defs_i = Input(&defs, "optional");
        auto sys_i = Input(&sysroot, "optional");

        auto layout = Container::Vertical({
            tname_i, c_i, cxx_i, asm_i, cf_i, cxxf_i, asmf_i, lf_i, libs_i, defs_i, sys_i,
            Container::Horizontal({ add_btn, next_btn })
        });
        auto render = Renderer(layout, [&]() -> Element {
            return vbox({
                text("=== Toolchains (optional) ===") | bold,
                text("Toolchain name: "), tname_i->Render(),
                text("C compiler: "), c_i->Render(),
                text("C++ compiler: "), cxx_i->Render(),
                text("ASM compiler: "), asm_i->Render(),
                text("C flags (comma): "), cf_i->Render(),
                text("CXX flags: "), cxxf_i->Render(),
                text("ASM flags: "), asmf_i->Render(),
                text("Linker flags: "), lf_i->Render(),
                text("Libs (comma): "), libs_i->Render(),
                text("Defines: "), defs_i->Render(),
                text("Sysroot: "), sys_i->Render(),
                text("Added: " + std::to_string(b.toolchains.size()) + " toolchain(s)"),
                hbox(add_btn->Render() | border, text(" "), next_btn->Render() | border)
            }) | border | frame | vscroll_indicator;
        });
        screen.Loop(render);
        return WizardState::IsaVariants;
    }
};

// --- IsaVariantsScreen ---
class IsaVariantsScreen : public IWizardScreen {
public:
    WizardState Run(ftxui::ScreenInteractive& screen, WizardContext& ctx) override {
        MetadataBuilder& b = *ctx.builder;
        std::string id, display_name;
        std::vector<std::string> tc_list;
        for (const auto& t : b.toolchains) tc_list.push_back(t.id);
        int tc_sel = 0;

        auto id_input = Input(&id, "e.g. cortex-m7");
        auto disp_input = Input(&display_name, "optional");
        Component tc_radio = Renderer([]() -> Element { return text("(no toolchains)"); });
        if (!tc_list.empty())
            tc_radio = Radiobox(&tc_list, &tc_sel);

        auto add_btn = Button("Add ISA variant", [&] {
            if (!id.empty() && !b.toolchains.empty() && tc_sel >= 0 && tc_sel < (int)b.toolchains.size()) {
                IsaVariantData iv;
                iv.id = normalize_id(id);
                iv.toolchain = b.toolchains[tc_sel].id;
                iv.display_name = display_name.empty() ? iv.id : display_name;
                b.isa_variants.push_back(iv);
                id.clear(); display_name.clear();
            }
        });
        auto next_btn = Button("Next section", screen.ExitLoopClosure());

        auto layout = Container::Vertical({
            id_input, disp_input, tc_radio,
            Container::Horizontal({ add_btn, next_btn })
        });
        auto render = Renderer(layout, [&]() -> Element {
            return vbox({
                text("=== ISA variants (optional) ===") | bold,
                text("ISA variant id: "), id_input->Render(),
                text("Display name: "), disp_input->Render(),
                text("Toolchain: "), tc_radio->Render(),
                text("Added: " + std::to_string(b.isa_variants.size()) + " ISA variant(s)"),
                hbox(add_btn->Render() | border, text(" "), next_btn->Render() | border)
            }) | border;
        });
        screen.Loop(render);
        return WizardState::BuildVariants;
    }
};

// --- BuildVariantsScreen ---
class BuildVariantsScreen : public IWizardScreen {
public:
    WizardState Run(ftxui::ScreenInteractive& screen, WizardContext& ctx) override {
        MetadataBuilder& b = *ctx.builder;
        std::vector<std::string> opt = { "Use default debug/release", "Skip (no build variants)" };
        int sel = 0;
        auto radio = Radiobox(&opt, &sel);
        auto next_btn = Button("Next", [&] {
            if (sel == 0) {
                BuildVariantData debug, release;
                debug.id = "debug";
                debug.flags["c"] = {"-g", "-O0"};
                debug.flags["cxx"] = {"-g", "-O0"};
                release.id = "release";
                release.flags["c"] = {"-O3", "-DNDEBUG"};
                release.flags["cxx"] = {"-O3", "-DNDEBUG"};
                b.build_variants.push_back(debug);
                b.build_variants.push_back(release);
            }
            screen.Exit();
        });

        auto layout = Container::Vertical({ radio, next_btn });
        auto render = Renderer(layout, [&]() -> Element {
            return vbox({
                text("=== Build variants ===") | bold,
                radio->Render(),
                next_btn->Render() | border
            }) | border;
        });
        screen.Loop(render);
        return WizardState::Components;
    }
};

// --- DependenciesScreen ---
class DependenciesScreen : public IWizardScreen {
public:
    WizardState Run(ftxui::ScreenInteractive& screen, WizardContext& ctx) override {
        MetadataBuilder& b = *ctx.builder;
        std::string tool_r, extra_r;
        auto tool_i = Input(&tool_r, "e.g. cmake/3.28.0");
        auto extra_i = Input(&extra_r, "optional");
        auto next_btn = Button("Next", [&] {
            b.dependencies.tool_requires = split_comma_separated(tool_r);
            b.dependencies.extra_requires = split_comma_separated(extra_r);
            screen.Exit();
        });

        auto layout = Container::Vertical({ tool_i, extra_i, next_btn });
        auto render = Renderer(layout, [&]() -> Element {
            return vbox({
                text("=== Conan dependencies (optional) ===") | bold,
                text("tool_requires (comma): "), tool_i->Render(),
                text("extra_requires: "), extra_i->Render(),
                next_btn->Render() | border
            }) | border;
        });
        screen.Loop(render);
        return WizardState::Output;
    }
};

// --- OutputScreen ---
class OutputScreen : public IWizardScreen {
public:
    WizardState Run(ftxui::ScreenInteractive& screen, WizardContext& ctx) override {
        MetadataBuilder& b = *ctx.builder;
        std::string path = ctx.output_path.empty() ? "metadata.json" : ctx.output_path;

        auto path_input = Input(&path, "metadata.json");
        auto gen_btn = Button("Generate JSON", [&] {
            b.set_preset_matrix_defaults();
            std::string json = b.to_json();
            std::ofstream f(path);
            if (f) {
                f << json;
                ctx.output_path = path;
                ctx.generated = true;
            }
            screen.Exit();
        });
        auto cancel_btn = Button("Cancel", screen.ExitLoopClosure());

        auto layout = Container::Vertical({ path_input, Container::Horizontal({ gen_btn, cancel_btn }) });
        auto render = Renderer(layout, [&]() -> Element {
            return vbox({
                text("=== Output ===") | bold,
                text("Output file path: "),
                path_input->Render(),
                hbox(gen_btn->Render() | border, text(" "), cancel_btn->Render() | border)
            }) | border;
        });
        screen.Loop(render);
        return WizardState::Done;
    }
};

// --- ComponentsScreen ---
class ComponentsScreen : public IWizardScreen {
public:
    WizardState Run(ftxui::ScreenInteractive& screen, WizardContext& ctx) override {
        MetadataBuilder& b = *ctx.builder;
        std::vector<std::string> type_opt = { "No components (empty list)", "Add components" };
        int type_sel = 0;
        auto type_radio = Radiobox(&type_opt, &type_sel);
        auto next_btn = Button("Next", [&] { screen.Exit(); });

        auto layout = Container::Vertical({ type_radio, next_btn });
        auto render = Renderer(layout, [&]() -> Element {
            return vbox({
                text("=== Source tree ===") | bold,
                text("Add source components now?"),
                type_radio->Render(),
                next_btn->Render() | border
            }) | border;
        });
        screen.Loop(render);

        if (type_sel != 1)
            return WizardState::Dependencies;

        std::vector<std::string> comp_types = { "executable", "library", "external", "variant", "layer" };
        int comp_type_sel = 0;
        std::string comp_id, source, dest, deps_str, conan_ref, subdirs_str;
        std::string git_url, git_tag, git_branch, git_commit;
        std::string filter_inc, filter_exc;
        bool show_filters = false;
        bool use_git = false;
        bool has_more = true;
        std::vector<std::string> lib_opts = { "static", "shared", "interface" };
        std::vector<std::string> fm_opt = { "include_first", "exclude_first" };
        bool reset_form = true;
        int lib_sel = 0;
        int fm_sel = 0;
        std::vector<VariationData> var_list;
        std::string var_subdir;
        std::shared_ptr<ConditionData> current_condition;
        int var_edit_index = -1;
        size_t var_delete_index = 0;
        bool var_do_delete = false;
        bool lib_hierarchical = false;

        while (has_more) {
            if (reset_form) {
                comp_type_sel = 0;
                comp_id.clear(); source.clear(); dest.clear(); deps_str.clear(); conan_ref.clear();
                subdirs_str.clear(); git_url.clear(); git_tag.clear(); git_branch.clear(); git_commit.clear();
                filter_inc.clear(); filter_exc.clear();
                show_filters = false; use_git = false; lib_hierarchical = false;
                var_list.clear(); var_subdir.clear(); current_condition = nullptr;
                lib_sel = 0; fm_sel = 0;
            }
            reset_form = true;
            var_edit_index = -1;
            var_do_delete = false;

            auto type_radio2 = Radiobox(&comp_types, &comp_type_sel);
            auto id_i = Input(&comp_id, "component id");
            auto exec_src_i = Input(&source, ".");
            auto exec_dest_i = Input(&dest, "e.g. apps/main");
            auto exec_deps_i = Input(&deps_str, "comma-separated");
            auto exec_show_filters_cb = Checkbox("Configure path filters?", &show_filters);
            auto exec_filter_inc_i = Input(&filter_inc, "src,inc");
            auto exec_filter_exc_i = Input(&filter_exc, "test,legacy");
            auto exec_fm_radio = Radiobox(&fm_opt, &fm_sel);
            auto exec_filter_block = Container::Vertical({ exec_filter_inc_i, exec_filter_exc_i, exec_fm_radio });
            auto exec_section = Container::Vertical({
                exec_src_i, exec_dest_i, exec_deps_i, exec_show_filters_cb,
                Maybe(exec_filter_block, [&] { return show_filters; })
            });

            auto lib_radio = Radiobox(&lib_opts, &lib_sel);
            auto lib_hierarchical_cb = Checkbox("Hierarchical layout?", &lib_hierarchical);
            auto lib_src_i = Input(&source, ".");
            auto lib_dest_i = Input(&dest, "e.g. libs/core");
            auto lib_deps_i = Input(&deps_str, "comma-separated");
            auto lib_show_filters_cb = Checkbox("Configure path filters?", &show_filters);
            auto lib_filter_inc_i = Input(&filter_inc, "src,inc");
            auto lib_filter_exc_i = Input(&filter_exc, "test,legacy");
            auto lib_fm_radio = Radiobox(&fm_opt, &fm_sel);
            auto lib_filter_block = Container::Vertical({ lib_filter_inc_i, lib_filter_exc_i, lib_fm_radio });
            auto lib_use_git_cb = Checkbox("Use git source?", &use_git);
            auto lib_git_url_i = Input(&git_url, "url");
            auto lib_git_tag_i = Input(&git_tag, "tag");
            auto lib_git_branch_i = Input(&git_branch, "branch");
            auto lib_git_commit_i = Input(&git_commit, "commit");
            auto lib_git_block = Container::Vertical({ lib_git_url_i, lib_git_tag_i, lib_git_branch_i, lib_git_commit_i });
            auto lib_section = Container::Vertical({
                lib_radio, lib_hierarchical_cb, lib_src_i, lib_dest_i, lib_deps_i, lib_show_filters_cb,
                Maybe(lib_filter_block, [&] { return show_filters; }),
                lib_use_git_cb, Maybe(lib_git_block, [&] { return use_git; })
            });

            auto conan_i = Input(&conan_ref, "e.g. freertos/11.0.1");
            auto ext_section = Container::Vertical({ conan_i });

            auto var_src_i = Input(&source, ".");
            auto var_dest_i = Input(&dest, "e.g. apps/variant");
            auto var_subdir_i = Input(&var_subdir, "subdir name");
            auto edit_cond_btn = Button("Edit condition", [&] { ConditionEditorScreen::Run(screen, current_condition); });
            auto add_var_btn = Button("Add this variation", [&] {
                VariationData v;
                v.subdir = var_subdir;
                v.condition = current_condition;
                var_list.push_back(v);
                var_subdir.clear();
                current_condition = nullptr;
            });
            auto var_deps_i = Input(&deps_str, "comma-separated");
            Components var_variation_components = { var_subdir_i, edit_cond_btn, add_var_btn };
            std::vector<Component> var_edit_btns, var_del_btns;
            for (size_t i = 0; i < var_list.size(); ++i) {
                var_edit_btns.push_back(Button("Edit #" + std::to_string(i + 1) + " " + var_list[i].subdir + ": " + condition_summary(var_list[i].condition), [&, i] { var_edit_index = static_cast<int>(i); screen.Exit(); }));
                var_del_btns.push_back(Button("Delete #" + std::to_string(i + 1), [&, i] { var_do_delete = true; var_delete_index = i; screen.Exit(); }));
                var_variation_components.push_back(var_edit_btns.back());
                var_variation_components.push_back(var_del_btns.back());
            }
            var_variation_components.push_back(var_deps_i);
            auto var_section = Container::Vertical({
                var_src_i, var_dest_i,
                Container::Vertical(var_variation_components)
            });

            auto layer_dest_i = Input(&dest, "e.g. layers/platform");
            auto subdirs_i = Input(&subdirs_str, "comma-separated component ids");
            auto layer_deps_i = Input(&deps_str, "comma-separated");
            auto layer_section = Container::Vertical({ layer_dest_i, subdirs_i, layer_deps_i });

            auto add_comp_btn = Button("Add this component", [&] {
                ComponentData c;
                c.id = comp_id.empty() ? "comp" : normalize_id(comp_id);
                c.type = comp_types[comp_type_sel];
                c.dependencies = split_comma_separated(deps_str);
                if (c.type == "executable" || c.type == "library" || c.type == "variant") {
                    c.source = source;
                    c.dest = dest;
                }
                if (c.type == "layer") {
                    c.dest = dest;
                    c.subdirs = split_comma_separated(subdirs_str);
                }
                if (c.type == "library") {
                    c.library_type = lib_opts[lib_sel];
                    if (lib_hierarchical) c.structure = "hierarchical";
                    if (show_filters && (!filter_inc.empty() || !filter_exc.empty())) {
                        PathFiltersData pf;
                        pf.filter_mode = fm_sel == 0 ? "include_first" : "exclude_first";
                        pf.include_paths = split_comma_separated(filter_inc);
                        pf.exclude_paths = split_comma_separated(filter_exc);
                        c.filters = pf;
                    }
                    if (use_git && !git_url.empty()) {
                        c.git_url = git_url;
                        c.git_tag = git_tag;
                        c.git_branch = git_branch;
                        c.git_commit = git_commit;
                    }
                }
                if (c.type == "executable" && show_filters && (!filter_inc.empty() || !filter_exc.empty())) {
                    PathFiltersData pf;
                    pf.filter_mode = fm_sel == 0 ? "include_first" : "exclude_first";
                    pf.include_paths = split_comma_separated(filter_inc);
                    pf.exclude_paths = split_comma_separated(filter_exc);
                    c.filters = pf;
                }
                if (c.type == "external") c.conan_ref = conan_ref;
                if (c.type == "variant") c.variations = var_list;
                b.components.push_back(c);
                screen.Exit();
            });
            auto done_btn = Button("Done adding components", [&] { has_more = false; screen.Exit(); });

            auto comp_layout = Container::Vertical({
                type_radio2, id_i,
                Maybe(exec_section, [&] { return comp_type_sel == 0; }),
                Maybe(lib_section, [&] { return comp_type_sel == 1; }),
                Maybe(ext_section, [&] { return comp_type_sel == 2; }),
                Maybe(var_section, [&] { return comp_type_sel == 3; }),
                Maybe(layer_section, [&] { return comp_type_sel == 4; }),
                Container::Horizontal({ add_comp_btn, done_btn })
            });

            auto comp_render = Renderer(comp_layout, [&]() -> Element {
                Elements lines;
                lines.push_back(text("Component type: "));
                lines.push_back(type_radio2->Render());
                std::string hint;
                if (comp_type_sel == 0) hint = "executable: app with source/dest and optional filters";
                else if (comp_type_sel == 1) hint = "library: static/shared/interface, optional filters and git";
                else if (comp_type_sel == 2) hint = "external: Conan package ref";
                else if (comp_type_sel == 3) hint = "variant: source/dest + variations (subdir + condition)";
                else hint = "layer: dest + subdir component ids";
                lines.push_back(text("  " + hint));
                lines.push_back(text("Id: "));
                lines.push_back(id_i->Render());
                if (comp_type_sel == 0) {
                    lines.push_back(text("Source: ")); lines.push_back(exec_src_i->Render());
                    lines.push_back(text("Dest: ")); lines.push_back(exec_dest_i->Render());
                    lines.push_back(text("Dependencies: ")); lines.push_back(exec_deps_i->Render());
                    lines.push_back(exec_show_filters_cb->Render());
                    if (show_filters) {
                        lines.push_back(text("Filter include: ")); lines.push_back(exec_filter_inc_i->Render());
                        lines.push_back(text("Filter exclude: ")); lines.push_back(exec_filter_exc_i->Render());
                        lines.push_back(text("Filter mode: ")); lines.push_back(exec_fm_radio->Render());
                    }
                } else if (comp_type_sel == 1) {
                    lines.push_back(text("Library type: ")); lines.push_back(lib_radio->Render());
                    lines.push_back(lib_hierarchical_cb->Render());
                    lines.push_back(text("Source: ")); lines.push_back(lib_src_i->Render());
                    lines.push_back(text("Dest: ")); lines.push_back(lib_dest_i->Render());
                    lines.push_back(text("Dependencies: ")); lines.push_back(lib_deps_i->Render());
                    lines.push_back(lib_show_filters_cb->Render());
                    if (show_filters) {
                        lines.push_back(text("Filter include: ")); lines.push_back(lib_filter_inc_i->Render());
                        lines.push_back(text("Filter exclude: ")); lines.push_back(lib_filter_exc_i->Render());
                        lines.push_back(text("Filter mode: ")); lines.push_back(lib_fm_radio->Render());
                    }
                    lines.push_back(lib_use_git_cb->Render());
                    if (use_git) {
                        lines.push_back(text("Git URL: ")); lines.push_back(lib_git_url_i->Render());
                        lines.push_back(text("Git tag: ")); lines.push_back(lib_git_tag_i->Render());
                        lines.push_back(text("Git branch: ")); lines.push_back(lib_git_branch_i->Render());
                        lines.push_back(text("Git commit: ")); lines.push_back(lib_git_commit_i->Render());
                    }
                } else if (comp_type_sel == 2) {
                    lines.push_back(text("Conan ref: ")); lines.push_back(conan_i->Render());
                } else if (comp_type_sel == 3) {
                    lines.push_back(text("Source: ")); lines.push_back(var_src_i->Render());
                    lines.push_back(text("Dest: ")); lines.push_back(var_dest_i->Render());
                    lines.push_back(text("Variation subdir: ")); lines.push_back(var_subdir_i->Render());
                    lines.push_back(text("Condition: " + condition_summary(current_condition)));
                    lines.push_back(edit_cond_btn->Render() | border);
                    lines.push_back(add_var_btn->Render() | border);
                    for (size_t i = 0; i < var_edit_btns.size(); ++i) {
                        lines.push_back(text("  " + std::to_string(i + 1) + ". " + var_list[i].subdir + ": " + condition_summary(var_list[i].condition)));
                        lines.push_back(var_edit_btns[i]->Render());
                        lines.push_back(var_del_btns[i]->Render());
                    }
                    lines.push_back(text("Dependencies: ")); lines.push_back(var_deps_i->Render());
                } else {
                    lines.push_back(text("Dest: ")); lines.push_back(layer_dest_i->Render());
                    lines.push_back(text("Subdirs (component ids): ")); lines.push_back(subdirs_i->Render());
                    lines.push_back(text("Dependencies: ")); lines.push_back(layer_deps_i->Render());
                }
                lines.push_back(hbox(add_comp_btn->Render() | border, text(" "), done_btn->Render() | border));
                return vbox(std::move(lines)) | border | frame | vscroll_indicator;
            });

            screen.Loop(comp_render);
            if (!has_more) break;
            if (comp_type_sel == 3 && var_edit_index >= 0 && static_cast<size_t>(var_edit_index) < var_list.size()) {
                ConditionEditorScreen::Run(screen, var_list[static_cast<size_t>(var_edit_index)].condition);
                reset_form = false;
                continue;
            }
            if (comp_type_sel == 3 && var_do_delete && var_delete_index < var_list.size()) {
                var_list.erase(var_list.begin() + static_cast<ptrdiff_t>(var_delete_index));
                reset_form = false;
                continue;
            }
        }
        return WizardState::Dependencies;
    }
};

}  // namespace

MetadataWizard::MetadataWizard(MetadataBuilder& builder, std::string output_path)
    : builder_(builder), output_path_(std::move(output_path)) {
    screens_[WizardState::Project] = std::make_unique<ProjectScreen>();
    screens_[WizardState::Socs] = std::make_unique<SocsScreen>();
    screens_[WizardState::Boards] = std::make_unique<BoardsScreen>();
    screens_[WizardState::Toolchains] = std::make_unique<ToolchainsScreen>();
    screens_[WizardState::IsaVariants] = std::make_unique<IsaVariantsScreen>();
    screens_[WizardState::BuildVariants] = std::make_unique<BuildVariantsScreen>();
    screens_[WizardState::Components] = std::make_unique<ComponentsScreen>();
    screens_[WizardState::Dependencies] = std::make_unique<DependenciesScreen>();
    screens_[WizardState::Output] = std::make_unique<OutputScreen>();
}

MetadataWizard::~MetadataWizard() = default;

bool MetadataWizard::Run() {
    auto screen = ScreenInteractive::Fullscreen();
    WizardContext ctx;
    ctx.builder = &builder_;
    ctx.output_path = output_path_;
    ctx.generated = false;

    WizardState state = WizardState::Project;
    while (state != WizardState::Done) {
        auto it = screens_.find(state);
        if (it == screens_.end() || !it->second)
            break;
        state = it->second->Run(screen, ctx);
    }

    if (ctx.generated) {
        try {
            Parser parser;
            Metadata meta = parser.parse_string(builder_.to_json(), "json");
            Validator validator;
            validator.validate(meta);
        } catch (...) {
            // Validation failed; file was still written
        }
    }
    return ctx.generated;
}

}  // namespace scaffolder

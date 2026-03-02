#include "interactive/prompt_runner.hpp"
#include "metadata/parser.hpp"
#include "metadata/validator.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <fstream>

namespace scaffolder {

using namespace ftxui;

namespace {

void run_project_screen(ScreenInteractive& screen, MetadataBuilder& b) {
    std::string name, version = "0.1.0";
    int cmake_ok = 0;  // 0 = use default (3.23.0)

    auto name_input = Input(&name, "Project name");
    auto version_input = Input(&version, "0.1.0");
    std::vector<std::string> cmake_opt = {"Use default (3.23.0)", "Custom (skip for now)"};
    int cmake_sel = 0;
    auto cmake_radio = Radiobox(&cmake_opt, &cmake_sel);

    auto next_btn = Button("Next", [&] {
        b.project.name = name.empty() ? "my_project" : name;
        b.project.version = version.empty() ? "0.1.0" : version;
        if (cmake_sel == 1) { b.project.cmake_major = 3; b.project.cmake_minor = 23; b.project.cmake_patch = 0; }
        screen.Exit();
    });

    auto layout = Container::Vertical({
        name_input,
        version_input,
        cmake_radio,
        next_btn
    });

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
}

void run_socs_screen(ScreenInteractive& screen, MetadataBuilder& b) {
    std::string name, desc, isas;
    int action = 0;  // 0 Add, 1 Next section

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
        name_input,
        desc_input,
        isas_input,
        Container::Horizontal({ add_btn, next_btn })
    });

    auto render = Renderer(layout, [&]() -> Element {
        return vbox({
            text("=== SOCs (optional) ===") | bold,
            text("SOC name: "),
            name_input->Render(),
            text("Description (optional): "),
            desc_input->Render(),
            text("ISAs (comma-separated): "),
            isas_input->Render(),
            text("Added: " + std::to_string(b.socs.size()) + " SOC(s)"),
            hbox(add_btn->Render() | border, text(" "), next_btn->Render() | border)
        }) | border;
    });

    screen.Loop(render);
}

void run_boards_screen(ScreenInteractive& screen, MetadataBuilder& b) {
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
        name_input,
        soc_input,
        defines_input,
        Container::Horizontal({ add_btn, next_btn })
    });

    auto render = Renderer(layout, [&]() -> Element {
        return vbox({
            text("=== Boards (optional) ===") | bold,
            text("Board name: "),
            name_input->Render(),
            text("SOC(s) (comma-separated ids): "),
            soc_input->Render(),
            text("Defines (comma-separated): "),
            defines_input->Render(),
            text("Added: " + std::to_string(b.boards.size()) + " board(s)"),
            hbox(add_btn->Render() | border, text(" "), next_btn->Render() | border)
        }) | border;
    });

    screen.Loop(render);
}

void run_toolchains_screen(ScreenInteractive& screen, MetadataBuilder& b) {
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
}

void run_isa_variants_screen(ScreenInteractive& screen, MetadataBuilder& b) {
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
        id_input,
        disp_input,
        tc_radio,
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
}

void run_build_variants_screen(ScreenInteractive& screen, MetadataBuilder& b) {
    int use_default = 0;  // 0 = Yes (debug/release), 1 = No

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
}

void run_components_screen(ScreenInteractive& screen, MetadataBuilder& b) {
    int add_components = 0;  // 0 = No, 1 = Yes
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

    if (type_sel != 1) return;

    // Component loop: pick type then fill
    std::vector<std::string> comp_types = { "executable", "library", "external", "variant", "layer" };
    int comp_type_sel = 0;
    std::string comp_id, source, dest, deps_str, conan_ref, lib_type = "static";
    std::string subdirs_str;
    std::string git_url, git_tag, git_branch, git_commit;
    std::string filter_inc, filter_exc;
    bool has_more = true;

    while (has_more) {
        comp_type_sel = 0;
        comp_id.clear(); source.clear(); dest.clear(); deps_str.clear();
        conan_ref.clear(); subdirs_str.clear(); lib_type = "static";
        git_url.clear(); git_tag.clear(); git_branch.clear(); git_commit.clear();
        filter_inc.clear(); filter_exc.clear();

        auto type_radio2 = Radiobox(&comp_types, &comp_type_sel);
        auto id_i = Input(&comp_id, "component id");
        auto src_i = Input(&source, ".");
        auto dest_i = Input(&dest, "e.g. apps/main");
        auto deps_i = Input(&deps_str, "comma-separated");
        auto conan_i = Input(&conan_ref, "e.g. freertos/11.0.1");
        auto subdirs_i = Input(&subdirs_str, "comma-separated component ids");
        std::vector<std::string> lib_opts = { "static", "shared", "interface" };
        int lib_sel = 0;
        auto lib_radio = Radiobox(&lib_opts, &lib_sel);
        auto git_url_i = Input(&git_url, "url");
        auto filter_inc_i = Input(&filter_inc, "src,inc");
        auto filter_exc_i = Input(&filter_exc, "test,legacy");
        std::vector<std::string> fm_opt = { "include_first", "exclude_first" };
        int fm_sel = 0;
        auto fm_radio = Radiobox(&fm_opt, &fm_sel);

        auto add_comp_btn = Button("Add this component", [&] {
            ComponentData c;
            c.id = comp_id.empty() ? "comp" : normalize_id(comp_id);
            c.type = comp_types[comp_type_sel];
            c.source = source;
            c.dest = dest;
            c.dependencies = split_comma_separated(deps_str);
            if (c.type == "library") {
                c.library_type = lib_opts[lib_sel];
                if (!filter_inc.empty() || !filter_exc.empty()) {
                    PathFiltersData pf;
                    pf.filter_mode = fm_sel == 0 ? "include_first" : "exclude_first";
                    pf.include_paths = split_comma_separated(filter_inc);
                    pf.exclude_paths = split_comma_separated(filter_exc);
                    c.filters = pf;
                }
                if (!git_url.empty()) { c.git_url = git_url; c.git_tag = git_tag; c.git_branch = git_branch; c.git_commit = git_commit; }
            }
            if (c.type == "executable" && (!filter_inc.empty() || !filter_exc.empty())) {
                PathFiltersData pf;
                pf.filter_mode = fm_sel == 0 ? "include_first" : "exclude_first";
                pf.include_paths = split_comma_separated(filter_inc);
                pf.exclude_paths = split_comma_separated(filter_exc);
                c.filters = pf;
            }
            if (c.type == "external") c.conan_ref = conan_ref;
            if (c.type == "layer") c.subdirs = split_comma_separated(subdirs_str);
            b.components.push_back(c);
            screen.Exit();
        });
        auto done_btn = Button("Done adding components", [&] { has_more = false; screen.Exit(); });

        auto comp_layout = Container::Vertical({
            type_radio2,
            id_i, src_i, dest_i, deps_i,
            conan_i, subdirs_i, lib_radio,
            git_url_i, filter_inc_i, filter_exc_i, fm_radio,
            Container::Horizontal({ add_comp_btn, done_btn })
        });

        auto comp_render = Renderer(comp_layout, [&]() -> Element {
            return vbox({
                text("Component type: "), type_radio2->Render(),
                text("Id: "), id_i->Render(),
                text("Source: "), src_i->Render(),
                text("Dest: "), dest_i->Render(),
                text("Dependencies: "), deps_i->Render(),
                text("Conan ref (external): "), conan_i->Render(),
                text("Subdirs (layer): "), subdirs_i->Render(),
                text("Library type: "), lib_radio->Render(),
                text("Git URL: "), git_url_i->Render(),
                text("Filter include paths: "), filter_inc_i->Render(),
                text("Filter exclude paths: "), filter_exc_i->Render(),
                text("Filter mode: "), fm_radio->Render(),
                hbox(add_comp_btn->Render() | border, text(" "), done_btn->Render() | border)
            }) | border | frame | vscroll_indicator;
        });

        screen.Loop(comp_render);
        if (!has_more) break;
    }
}

void run_dependencies_screen(ScreenInteractive& screen, MetadataBuilder& b) {
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
}

bool run_output_screen(ScreenInteractive& screen, MetadataBuilder& b, const std::string& default_path) {
    std::string path = default_path.empty() ? "metadata.json" : default_path;
    auto path_input = Input(&path, "metadata.json");
    bool generated = false;

    auto gen_btn = Button("Generate JSON", [&] {
        b.set_preset_matrix_defaults();
        std::string json = b.to_json();
        std::ofstream f(path);
        if (f) {
            f << json;
            generated = true;
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
    return generated;
}

}  // namespace

bool run_interactive(MetadataBuilder& builder, const std::string& output_path) {
    auto screen = ScreenInteractive::Fullscreen();

    run_project_screen(screen, builder);
    run_socs_screen(screen, builder);
    run_boards_screen(screen, builder);
    run_toolchains_screen(screen, builder);
    run_isa_variants_screen(screen, builder);
    run_build_variants_screen(screen, builder);
    run_components_screen(screen, builder);
    run_dependencies_screen(screen, builder);
    bool ok = run_output_screen(screen, builder, output_path);

    if (ok) {
        try {
            Parser parser;
            Metadata meta = parser.parse_string(builder.to_json(), "json");
            Validator validator;
            validator.validate(meta);
        } catch (...) {
            // Validation failed; file was still written
        }
    }

    return ok;
}

}  // namespace scaffolder

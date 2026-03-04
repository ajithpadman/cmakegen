#include "interactive/add_runner.hpp"
#include "interactive/condition_parser.hpp"
#include "interactive/metadata_builder.hpp"
#include "config/config_writer.hpp"
#include "metadata/schema.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <filesystem>

namespace scaffolder {

using namespace ftxui;

namespace {

struct VariantRow {
    std::string subdir;
    std::string condition_text;
};

static Project to_schema(const ProjectData& p) {
    Project out;
    out.name = p.name;
    out.version = p.version;
    out.cmake_minimum.major = p.cmake_major;
    out.cmake_minimum.minor = p.cmake_minor;
    out.cmake_minimum.patch = p.cmake_patch;
    return out;
}

static Soc to_schema(const SocData& s) {
    Soc out;
    out.id = s.id;
    out.display_name = s.display_name;
    out.description = s.description;
    out.isas = s.isas;
    return out;
}

static Board to_schema(const BoardData& b) {
    Board out;
    out.id = b.id;
    out.display_name = b.display_name;
    out.socs = b.socs;
    out.defines = b.defines;
    return out;
}

static Compiler to_schema(const CompilerData& c) {
    Compiler out;
    out.c = c.c;
    out.cxx = c.cxx;
    out.asm_ = c.asm_;
    return out;
}

static Toolchain to_schema(const ToolchainData& t) {
    Toolchain out;
    out.id = t.id;
    out.display_name = t.display_name;
    out.compiler = to_schema(t.compiler);
    out.flags = t.flags;
    out.libs = t.libs;
    out.lib_paths = t.lib_paths;
    out.defines = t.defines;
    out.sysroot = t.sysroot;
    return out;
}

static IsaVariant to_schema(const IsaVariantData& iv) {
    IsaVariant out;
    out.id = iv.id;
    out.toolchain = iv.toolchain;
    out.display_name = iv.display_name;
    return out;
}

static BuildVariant to_schema(const BuildVariantData& bv) {
    BuildVariant out;
    out.id = bv.id;
    out.flags = bv.flags;
    return out;
}

}  // namespace

bool run_add_project(const std::filesystem::path& folder) {
    ProjectData data;
    data.version = "0.1.0";
    std::string name, version = data.version;
    int cmajor = 3, cminor = 23, cpatch = 0;

    auto screen = ScreenInteractive::Fullscreen();
    auto name_i = Input(&name, "Project name");
    auto version_i = Input(&version, "0.1.0");
    auto done_btn = Button("Done", [&] {
        data.name = name.empty() ? "my_project" : name;
        data.version = version.empty() ? "0.1.0" : version;
        data.cmake_major = cmajor;
        data.cmake_minor = cminor;
        data.cmake_patch = cpatch;
        screen.Exit();
    });

    auto layout = Container::Vertical({ name_i, version_i, done_btn });
    auto render = Renderer(layout, [&] {
        return vbox({
            text("Add project") | bold,
            text("Name:"), name_i->Render(),
            text("Version:"), version_i->Render(),
            done_btn->Render() | border
        }) | border;
    });
    screen.Loop(render);

    Project p = to_schema(data);
    ConfigWriter w;
    w.write_project(folder, p);
    return true;
}

bool run_add_soc(const std::filesystem::path& folder) {
    std::string name, desc, isas;
    auto screen = ScreenInteractive::Fullscreen();
    auto name_i = Input(&name, "e.g. STM32H7 Series");
    auto desc_i = Input(&desc, "optional");
    auto isas_i = Input(&isas, "e.g. cortex-m7");
    auto done_btn = Button("Done", [&] { screen.Exit(); });

    auto layout = Container::Vertical({ name_i, desc_i, isas_i, done_btn });
    auto render = Renderer(layout, [&] {
        return vbox({
            text("Add SOC") | bold,
            text("SOC name:"), name_i->Render(),
            text("Description:"), desc_i->Render(),
            text("ISAs (comma-separated):"), isas_i->Render(),
            done_btn->Render() | border
        }) | border;
    });
    screen.Loop(render);

    SocData s;
    s.display_name = name;
    s.id = normalize_id(name);
    s.description = desc;
    s.isas = split_comma_separated(isas);
    ConfigWriter w;
    w.append_soc(folder, to_schema(s));
    return true;
}

bool run_add_board(const std::filesystem::path& folder) {
    std::string name, soc_ids, defines;
    auto screen = ScreenInteractive::Fullscreen();
    auto name_i = Input(&name, "e.g. Nucleo H743ZI");
    auto soc_i = Input(&soc_ids, "comma-separated SOC ids");
    auto def_i = Input(&defines, "e.g. BOARD_NUCLEO_H743");
    auto done_btn = Button("Done", [&] { screen.Exit(); });

    auto layout = Container::Vertical({ name_i, soc_i, def_i, done_btn });
    auto render = Renderer(layout, [&] {
        return vbox({
            text("Add board") | bold,
            text("Board name:"), name_i->Render(),
            text("SOC(s):"), soc_i->Render(),
            text("Defines:"), def_i->Render(),
            done_btn->Render() | border
        }) | border;
    });
    screen.Loop(render);

    BoardData b;
    b.display_name = name;
    b.id = normalize_id(name);
    b.socs = split_comma_separated(soc_ids);
    b.defines = split_comma_separated(defines);
    ConfigWriter w;
    w.append_board(folder, to_schema(b));
    return true;
}

bool run_add_toolchain(const std::filesystem::path& folder) {
    std::string tname, c_comp = "arm-none-eabi-gcc", cxx_comp, asm_comp;
    std::string c_flags = "-mcpu=cortex-m7,-mthumb", cxx_flags, asm_flags, link_flags = "-mcpu=cortex-m7,-mthumb,-specs=nano.specs";
    std::string libs = "c,nosys", defs, sysroot;

    auto screen = ScreenInteractive::Fullscreen();
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
    auto done_btn = Button("Done", [&] { screen.Exit(); });

    auto layout = Container::Vertical({ tname_i, c_i, cxx_i, asm_i, cf_i, cxxf_i, asmf_i, lf_i, libs_i, defs_i, sys_i, done_btn });
    auto render = Renderer(layout, [&] {
        return vbox({
            text("Add toolchain") | bold,
            text("Name:"), tname_i->Render(),
            text("C compiler:"), c_i->Render(),
            text("CXX compiler:"), cxx_i->Render(),
            text("ASM compiler:"), asm_i->Render(),
            text("C flags:"), cf_i->Render(),
            text("CXX flags:"), cxxf_i->Render(),
            text("ASM flags:"), asmf_i->Render(),
            text("Linker flags:"), lf_i->Render(),
            text("Libs:"), libs_i->Render(),
            text("Defines:"), defs_i->Render(),
            text("Sysroot:"), sys_i->Render(),
            done_btn->Render() | border
        }) | border | frame | vscroll_indicator;
    });
    screen.Loop(render);

    ToolchainData t;
    t.display_name = tname;
    t.id = normalize_id(tname);
    t.compiler.c = c_comp.empty() ? "gcc" : c_comp;
    if (!cxx_comp.empty()) t.compiler.cxx = cxx_comp;
    else {
        std::string c = t.compiler.c;
        size_t pos = c.rfind("gcc");
        if (pos != std::string::npos) t.compiler.cxx = c.substr(0, pos) + "g++" + c.substr(pos + 3);
        else t.compiler.cxx = c;
    }
    t.compiler.asm_ = asm_comp.empty() ? t.compiler.c : asm_comp;
    t.flags["c"] = split_comma_separated(c_flags);
    t.flags["cxx"] = split_comma_separated(cxx_flags.empty() ? c_flags : cxx_flags);
    t.flags["asm"] = split_comma_separated(asm_flags.empty() ? c_flags : asm_flags);
    t.flags["linker"] = split_comma_separated(link_flags);
    t.libs = split_comma_separated(libs);
    t.defines = split_comma_separated(defs);
    t.sysroot = sysroot;
    ConfigWriter w;
    w.append_toolchain(folder, to_schema(t));
    return true;
}

bool run_add_isa_variant(const std::filesystem::path& folder) {
    std::string id, display_name;
    int tc_sel = 0;
    std::vector<std::string> tc_list;  // would need to load from folder; for now empty
    auto screen = ScreenInteractive::Fullscreen();
    auto id_i = Input(&id, "e.g. cortex-m7");
    auto disp_i = Input(&display_name, "optional");
    auto done_btn = Button("Done", [&] { screen.Exit(); });

    auto layout = Container::Vertical({ id_i, disp_i, done_btn });
    auto render = Renderer(layout, [&] {
        return vbox({
            text("Add ISA variant") | bold,
            text("ISA variant id:"), id_i->Render(),
            text("Display name:"), disp_i->Render(),
            done_btn->Render() | border
        }) | border;
    });
    screen.Loop(render);

    IsaVariantData iv;
    iv.id = normalize_id(id);
    iv.display_name = display_name.empty() ? iv.id : display_name;
    iv.toolchain = tc_list.empty() ? "" : (tc_sel >= 0 && tc_sel < (int)tc_list.size() ? tc_list[tc_sel] : "");
    ConfigWriter w;
    w.append_isa_variant(folder, to_schema(iv));
    return true;
}

bool run_add_build_variant(const std::filesystem::path& folder) {
    std::string id = "debug";
    std::string c_flags = "-g,-O0", cxx_flags = "-g,-O0";
    int sel = 0;
    std::vector<std::string> opt = { "Debug (default flags)", "Release (default flags)", "Custom" };
    auto screen = ScreenInteractive::Fullscreen();
    auto id_i = Input(&id, "debug or release");
    auto radio = Radiobox(&opt, &sel);
    auto cf_i = Input(&c_flags, "-g,-O0");
    auto cxxf_i = Input(&cxx_flags, "-g,-O0");
    auto done_btn = Button("Done", [&] { screen.Exit(); });

    auto layout = Container::Vertical({ id_i, radio, cf_i, cxxf_i, done_btn });
    auto render = Renderer(layout, [&] {
        return vbox({
            text("Add build variant") | bold,
            text("Id:"), id_i->Render(),
            radio->Render(),
            text("C flags:"), cf_i->Render(),
            text("CXX flags:"), cxxf_i->Render(),
            done_btn->Render() | border
        }) | border;
    });
    screen.Loop(render);

    BuildVariantData bv;
    bv.id = id.empty() ? "debug" : normalize_id(id);
    if (sel == 0) { bv.flags["c"] = {"-g", "-O0"}; bv.flags["cxx"] = {"-g", "-O0"}; }
    else if (sel == 1) { bv.flags["c"] = {"-O3", "-DNDEBUG"}; bv.flags["cxx"] = {"-O3", "-DNDEBUG"}; }
    else { bv.flags["c"] = split_comma_separated(c_flags); bv.flags["cxx"] = split_comma_separated(cxx_flags); }
    ConfigWriter w;
    w.append_build_variant(folder, to_schema(bv));
    return true;
}

bool run_add_component(const std::filesystem::path& folder) {
    std::string comp_id, source = ".", dest, deps_str, conan_ref, subdirs_str;
    std::string filter_inc, filter_exc;
    std::vector<std::string> types = { "executable", "library", "external", "variant", "layer" };
    std::vector<std::string> lib_opts = { "static", "shared", "interface" };
    std::vector<std::string> fm_opt = { "include_first", "exclude_first" };
    int type_sel = 0;
    int lib_sel = 0;
    int fm_sel = 0;
    bool lib_hierarchical = false;
    bool show_filters = false;
    std::vector<VariantRow> var_list;
    std::string var_subdir;
    std::string condition_text;
    auto screen = ScreenInteractive::Fullscreen();
    auto type_r = Radiobox(&types, &type_sel);
    auto id_i = Input(&comp_id, "component id");
    auto lib_type_r = Radiobox(&lib_opts, &lib_sel);
    auto lib_hierarchical_cb = Checkbox("Hierarchical layout?", &lib_hierarchical);
    auto lib_show_filters_cb = Checkbox("Configure path filters?", &show_filters);
    auto lib_filter_inc_i = Input(&filter_inc, "src,inc");
    auto lib_filter_exc_i = Input(&filter_exc, "test,legacy");
    auto lib_fm_r = Radiobox(&fm_opt, &fm_sel);
    auto lib_filter_block = Container::Vertical({ lib_filter_inc_i, lib_filter_exc_i, lib_fm_r });
    auto lib_block = Container::Vertical({
        lib_type_r, lib_hierarchical_cb,
        lib_show_filters_cb,
        Maybe(lib_filter_block, [&] { return show_filters; })
    });
    auto src_i = Input(&source, ".");
    auto dest_i = Input(&dest, "e.g. apps/main");
    auto subdirs_i = Input(&subdirs_str, "comma-separated component ids");
    auto deps_i = Input(&deps_str, "comma-separated");
    auto conan_i = Input(&conan_ref, "e.g. freertos/11.0.1");
    auto var_subdir_i = Input(&var_subdir, "e.g. stm32h7");
    auto condition_text_i = Input(&condition_text, "e.g. SOC equals stm32h7");
    auto add_var_btn = Button("Add this variation", [&] {
        if (!var_subdir.empty() && !condition_text.empty()) {
            var_list.push_back(VariantRow{var_subdir, condition_text});
            var_subdir.clear();
            condition_text.clear();
        }
    });
    auto var_section = Container::Vertical({
        var_subdir_i,
        condition_text_i,
        add_var_btn
    });
    auto done_btn = Button("Done", [&] { screen.Exit(); });

    auto layout = Container::Vertical({
        type_r, id_i,
        Maybe(lib_block, [&] { return type_sel == 1; }),
        src_i, dest_i,
        Maybe(var_section, [&] { return type_sel == 3; }),
        Maybe(subdirs_i, [&] { return type_sel == 4; }),
        deps_i, conan_i, done_btn
    });
    layout |= CatchEvent([&](ftxui::Event) { screen.RequestAnimationFrame(); return false; });

    auto render = Renderer(layout, [&] {
        Elements out = {
            text("Add component") | bold,
            text("Type:"), type_r->Render(),
            text("Id:"), id_i->Render()
        };
        if (type_sel == 1) {
            out.push_back(text("Library type:"));
            out.push_back(lib_type_r->Render());
            out.push_back(lib_hierarchical_cb->Render());
            out.push_back(lib_show_filters_cb->Render());
            if (show_filters) {
                out.push_back(text("Filter include:"));
                out.push_back(lib_filter_inc_i->Render());
                out.push_back(text("Filter exclude:"));
                out.push_back(lib_filter_exc_i->Render());
                out.push_back(text("Filter mode:"));
                out.push_back(lib_fm_r->Render());
            }
        }
        out.push_back(text("Source:"));
        out.push_back(src_i->Render());
        out.push_back(text("Dest:"));
        out.push_back(dest_i->Render());
        if (type_sel == 3) {
            out.push_back(text("Variation subdir:"));
            out.push_back(var_subdir_i->Render());
            out.push_back(text("Condition (e.g. SOC equals stm32h7, or SOC in (a,b)):"));
            out.push_back(condition_text_i->Render());
            out.push_back(add_var_btn->Render() | border);
            if (!var_list.empty()) {
                out.push_back(text("Variations added:"));
                for (size_t i = 0; i < var_list.size(); ++i)
                    out.push_back(text("  " + std::to_string(i + 1) + ". " + var_list[i].subdir + " : " + var_list[i].condition_text));
            }
        }
        if (type_sel == 4) {
            out.push_back(text("Subdirs (component ids):"));
            out.push_back(subdirs_i->Render());
        }
        out.push_back(text("Dependencies:"));
        out.push_back(deps_i->Render());
        out.push_back(text("Conan ref (external):"));
        out.push_back(conan_i->Render());
        out.push_back(done_btn->Render() | border);
        return vbox(std::move(out)) | border | frame | vscroll_indicator;
    });
    screen.Loop(render);

    SwComponent c;
    c.id = comp_id.empty() ? "comp" : normalize_id(comp_id);
    c.type = types[type_sel];
    c.source = source;
    c.dest = dest.empty() ? std::nullopt : std::optional(dest);
    c.dependencies = split_comma_separated(deps_str);
    if (c.dependencies->empty()) c.dependencies = std::nullopt;
    if (c.type == "external" && !conan_ref.empty()) c.conan_ref = conan_ref;
    if (c.type == "library") {
        c.library_type = lib_opts[lib_sel];
        if (lib_hierarchical) c.structure = "hierarchical";
        if (show_filters && (!filter_inc.empty() || !filter_exc.empty())) {
            PathFilters pf;
            pf.include_paths = split_comma_separated(filter_inc);
            pf.exclude_paths = split_comma_separated(filter_exc);
            pf.filter_mode = (fm_sel == 0) ? FilterMode::IncludeFirst : FilterMode::ExcludeFirst;
            c.filters = pf;
        }
    }
    if (c.type == "layer") {
        auto sd = split_comma_separated(subdirs_str);
        c.subdirs = sd.empty() ? std::nullopt : std::make_optional(std::move(sd));
    }
    if (c.type == "variant") {
        if (var_list.empty() && (!var_subdir.empty() || !condition_text.empty())) {
            var_list.push_back(VariantRow{var_subdir, condition_text});
        }
        if (!var_list.empty()) {
            std::vector<Variation> variations;
            for (const auto& row : var_list) {
                Variation v;
                v.subdir = row.subdir;
                if (!row.condition_text.empty()) {
                    std::string err;
                    auto cd = parse_condition_text(row.condition_text, &err);
                    if (cd) {
                        v.condition = condition_data_to_schema(cd);
                    }
                }
                variations.push_back(std::move(v));
            }
            c.variations = std::move(variations);
        }
    }
    ConfigWriter w;
    w.append_component(folder, c);
    return true;
}

bool run_add_conanfile(const std::filesystem::path& folder) {
    std::string tool_r, extra_r;
    auto screen = ScreenInteractive::Fullscreen();
    auto tool_i = Input(&tool_r, "e.g. cmake/3.28.0");
    auto extra_i = Input(&extra_r, "optional");
    auto done_btn = Button("Done", [&] { screen.Exit(); });

    auto layout = Container::Vertical({ tool_i, extra_i, done_btn });
    auto render = Renderer(layout, [&] {
        return vbox({
            text("Add conanfile (dependencies)") | bold,
            text("tool_requires (comma):"), tool_i->Render(),
            text("extra_requires:"), extra_i->Render(),
            done_btn->Render() | border
        }) | border;
    });
    screen.Loop(render);

    Dependencies d;
    d.tool_requires = split_comma_separated(tool_r);
    d.extra_requires = split_comma_separated(extra_r);
    ConfigWriter w;
    w.write_dependencies(folder, d);
    return true;
}

bool run_add_cmake_preset(const std::filesystem::path& folder) {
    std::string naming = "{board}_{soc}_{isa}_{variant}", binary_dir = "build/${preset}";
    auto screen = ScreenInteractive::Fullscreen();
    auto naming_i = Input(&naming, "naming pattern");
    auto binary_i = Input(&binary_dir, "build/${preset}");
    auto done_btn = Button("Done", [&] { screen.Exit(); });

    auto layout = Container::Vertical({ naming_i, binary_i, done_btn });
    auto render = Renderer(layout, [&] {
        return vbox({
            text("Add CMake preset matrix") | bold,
            text("Naming:"), naming_i->Render(),
            text("Binary dir pattern:"), binary_i->Render(),
            done_btn->Render() | border
        }) | border;
    });
    screen.Loop(render);

    PresetMatrix pm;
    pm.dimensions = {"board", "soc", "isa_variant", "build_variant"};
    pm.naming = naming;
    pm.binary_dir_pattern = binary_dir;
    ConfigWriter w;
    w.write_preset_matrix(folder, pm);
    return true;
}

}  // namespace scaffolder

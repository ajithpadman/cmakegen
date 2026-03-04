#include "config/config_loader.hpp"
#include "config/resolver.hpp"
#include "metadata/validator.hpp"
#include "resolver/path_resolver.hpp"
#include "resolver/git_cloner.hpp"
#include "copy/copy_engine.hpp"
#include "generator/cmake_generator.hpp"
#include "generator/toolchain_generator.hpp"
#include "generator/preset_generator.hpp"
#include "generator/conan_generator.hpp"
#include "interactive/add_runner.hpp"
#include <CLI/CLI.hpp>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    CLI::App app{"CMake project generator for bare-metal embedded systems"};
    app.footer("Use 'add' to create JSON metadata interactively, then 'generate' to validate and scaffold.");

    std::string metadata_folder = "./metadata";
    std::string output_dir = "./output";

    auto* add_cmd = app.add_subcommand("add", "Add one entity (interactive). Writes JSON to metadata folder.");
    add_cmd->add_option("-o,--output", metadata_folder, "Metadata folder (default: ./metadata)")
        ->default_val("./metadata");

    std::string add_entity;
    add_cmd->add_option("entity", add_entity,
            "project | soc | board | toolchain | isa-variant | build_type | component | conanfile | cmake-preset")
        ->required();

    auto* gen_cmd = app.add_subcommand("generate", "Load metadata folder, validate, resolve, and generate full source tree.");
    gen_cmd->add_option("-f,--folder,folder", metadata_folder, "Path to folder containing JSON metadata files")
        ->required();
    gen_cmd->add_option("-o,--output", output_dir, "Output directory for scaffolded project")
        ->default_val("./output");

    CLI11_PARSE(app, argc, argv);

    if (add_cmd->parsed()) {
        fs::path folder = metadata_folder;
        std::filesystem::create_directories(folder);

        bool ok = false;
        if (add_entity == "project") ok = scaffolder::run_add_project(folder);
        else if (add_entity == "soc") ok = scaffolder::run_add_soc(folder);
        else if (add_entity == "board") ok = scaffolder::run_add_board(folder);
        else if (add_entity == "toolchain") ok = scaffolder::run_add_toolchain(folder);
        else if (add_entity == "isa-variant") ok = scaffolder::run_add_isa_variant(folder);
        else if (add_entity == "build_type") ok = scaffolder::run_add_build_variant(folder);
        else if (add_entity == "component") ok = scaffolder::run_add_component(folder);
        else if (add_entity == "conanfile") ok = scaffolder::run_add_conanfile(folder);
        else if (add_entity == "cmake-preset") ok = scaffolder::run_add_cmake_preset(folder);
        else {
            std::cerr << "Unknown entity: " << add_entity << "\n";
            return 1;
        }
        if (ok) {
            std::cout << "Written to " << folder.string() << "\n";
            return 0;
        }
        std::cerr << "Add cancelled or failed.\n";
        return 1;
    }

    if (gen_cmd->parsed()) {
        try {
            fs::path meta_path(metadata_folder);
            scaffolder::ConfigLoader loader;
            scaffolder::Metadata metadata = loader.load(meta_path);

            scaffolder::Validator validator;
            validator.validate(metadata);

            scaffolder::resolve(metadata);

            fs::path output_path(output_dir);
            fs::create_directories(output_path);
            fs::path base_dir = meta_path.is_absolute() ? meta_path : fs::absolute(meta_path);

            scaffolder::PathResolver path_resolver(base_dir);

            fs::path cache_dir = output_path / ".cmakegen_cache";
            scaffolder::GitCloner git_cloner(cache_dir);
            for (const auto& comp : metadata.source_tree.components) {
                if (comp.git && comp.git->url.size() > 0) {
                    fs::path cloned = git_cloner.clone(*comp.git, comp.id);
                    path_resolver.set_resolved_source(comp.id, cloned);
                }
            }

            scaffolder::CopyEngine copy_engine(path_resolver, output_path);
            for (const auto& comp : metadata.source_tree.components) {
                copy_engine.copy_component(comp);
            }

            scaffolder::CmakeGenerator cmake_gen(metadata, path_resolver, output_path);
            scaffolder::ToolchainGenerator toolchain_gen(metadata);
            scaffolder::PresetGenerator preset_gen(metadata);
            scaffolder::ConanGenerator conan_gen(metadata);

            cmake_gen.generate_all();
            toolchain_gen.generate_all(output_path);
            preset_gen.generate(output_path);
            conan_gen.generate(output_path);

            if (fs::exists(cache_dir)) {
                fs::remove_all(cache_dir);
            }

            std::cout << "Scaffolding complete: " << output_path.string() << "\n";
            return 0;
        } catch (const scaffolder::ConfigLoadError& e) {
            std::cerr << "Config load error: " << e.what() << "\n";
            return 1;
        } catch (const scaffolder::ResolveError& e) {
            std::cerr << "Resolve error: " << e.what() << "\n";
            return 1;
        } catch (const scaffolder::ValidationError& e) {
            std::cerr << "Validation error: " << e.what() << "\n";
            return 1;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    std::cerr << app.help();
    return 1;
}

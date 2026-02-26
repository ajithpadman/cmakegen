#include "metadata/parser.hpp"
#include "metadata/validator.hpp"
#include "metadata/env_expander.hpp"
#include "resolver/path_resolver.hpp"
#include "resolver/git_cloner.hpp"
#include "copy/copy_engine.hpp"
#include "generator/cmake_generator.hpp"
#include "generator/toolchain_generator.hpp"
#include "generator/preset_generator.hpp"
#include "generator/conan_generator.hpp"
#include "generator/default_json_generator.hpp"
#include <CLI/CLI.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    CLI::App app{"CMake project generator for bare-metal embedded systems"};
    app.footer("Metadata file must be JSON. Use --default-json to generate a template.");

    std::string metadata_file;
    std::string output_dir = "./output";
    std::string default_json_output;
    bool validate_only = false;
    bool dry_run = false;

    app.add_option("metadata,-m,--metadata", metadata_file, "Metadata file (JSON)")
        ->required(false);

    app.add_option("-o,--output", output_dir, "Output directory for scaffolded project")
        ->default_val("./output");

    app.add_flag("--validate-only", validate_only, "Only validate schema, don't scaffold");

    app.add_flag("--dry-run", dry_run, "Print actions without executing");

    auto* default_opt = app.add_option("--default-json", default_json_output,
                   "Generate default JSON template. Write to file if path given, else stdout")
        ->expected(0, 1)
        ->default_str("");

    CLI11_PARSE(app, argc, argv);

    if (default_opt->count() > 0) {
        std::ostream* out = &std::cout;
        std::ofstream file_out;
        if (!default_json_output.empty()) {
            file_out.open(default_json_output);
            if (!file_out) {
                std::cerr << "Error: Cannot write to " << default_json_output << "\n";
                return 1;
            }
            out = &file_out;
        }
        scaffolder::write_default_json(*out);
        if (file_out.is_open()) {
            std::cout << "Default JSON template written to " << default_json_output << "\n";
        }
        return 0;
    }

    if (metadata_file.empty()) {
        std::cerr << app.help();
        return 1;
    }

    try {
        scaffolder::Parser parser;
        scaffolder::Metadata metadata = parser.parse_file(metadata_file);

        scaffolder::Validator validator;
        validator.validate(metadata);

        if (validate_only) {
            std::cout << "Validation passed.\n";
            return 0;
        }

        fs::path output_path(output_dir);
        fs::path base_dir = fs::path(metadata_file).parent_path();

        if (!dry_run) {
            fs::create_directories(output_path);
        }

        scaffolder::PathResolver path_resolver(base_dir);

        if (!dry_run) {
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
        }

        scaffolder::CmakeGenerator cmake_gen(metadata, path_resolver, output_path);
        scaffolder::ToolchainGenerator toolchain_gen(metadata);
        scaffolder::PresetGenerator preset_gen(metadata);
        scaffolder::ConanGenerator conan_gen(metadata);

        if (!dry_run) {
            cmake_gen.generate_all();
            toolchain_gen.generate_all(output_path);
            preset_gen.generate(output_path);
            conan_gen.generate(output_path);

            fs::path cache_dir = output_path / ".cmakegen_cache";
            if (fs::exists(cache_dir)) {
                fs::remove_all(cache_dir);
            }
        } else {
            std::cout << "Dry run: would scaffold to " << output_path << "\n";
        }

        std::cout << "Scaffolding complete.\n";
        return 0;
    } catch (const scaffolder::ParseError& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return 1;
    } catch (const scaffolder::ValidationError& e) {
        std::cerr << "Validation error: " << e.what() << "\n";
        return 1;
    } catch (const scaffolder::EnvExpandError& e) {
        std::cerr << "Environment expansion error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

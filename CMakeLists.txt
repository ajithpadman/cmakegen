cmake_minimum_required(VERSION 3.23)
project(cmakegen VERSION 0.2.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Dependencies: Conan toolchain if present, else FetchContent
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

include(FetchContent)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# yaml-cpp for YAML support
option(CMAKEGEN_USE_YAML "Enable YAML metadata support" OFF)
if(CMAKEGEN_USE_YAML)
    find_package(yaml-cpp QUIET)
    if(NOT yaml-cpp_FOUND)
        FetchContent_Declare(yaml-cpp
            GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
            GIT_TAG yaml-cpp-0.6.3
        )
        FetchContent_MakeAvailable(yaml-cpp)
    endif()
endif()

FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.1
)
FetchContent_MakeAvailable(CLI11)

# FTXUI for interactive CLI (always built: from Conan when available, else FetchContent)
find_package(ftxui QUIET)
if(NOT ftxui_FOUND)
    set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
        GIT_TAG v6.1.9
    )
    FetchContent_MakeAvailable(ftxui)
endif()

FetchContent_Declare(inja
    GIT_REPOSITORY https://github.com/pantor/inja.git
    GIT_TAG v3.4.0
)
set(INJA_USE_EMBEDDED_JSON OFF CACHE BOOL "" FORCE)
set(INJA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(inja)

# Copy templates to build directory for runtime access
file(COPY ${CMAKE_SOURCE_DIR}/templates DESTINATION ${CMAKE_BINARY_DIR})

# ANTLR4 C++ runtime (for condition expression parser)
set(WITH_DEMO False CACHE BOOL "" FORCE)
set(ANTLR4_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(antlr4_runtime
    GIT_REPOSITORY https://github.com/antlr/antlr4
    GIT_TAG 4.13.1
    SOURCE_SUBDIR runtime/Cpp
)
FetchContent_MakeAvailable(antlr4_runtime)

# ANTLR4 code generation from grammar (requires Java; downloads jar if ANTLR4_JAR not set)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(Antlr4Cpp)
antlr4cpp_generate(ConditionExpr ${CMAKE_SOURCE_DIR}/grammar/ConditionExpr.g4
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/generated
    VISITOR
)
set(ANTLR4_ConditionExpr_SOURCES ${ANTLR4CPP_ConditionExpr_SOURCES})
set(ANTLR4_ConditionExpr_INCLUDE_DIR ${ANTLR4CPP_ConditionExpr_INCLUDE_DIR})

# Library
add_library(cmakegen_lib
    src/config/config_loader.cpp
    src/config/config_writer.cpp
    src/config/resolver.cpp
    src/metadata/parser.cpp
    src/metadata/validator.cpp
    src/metadata/env_expander.cpp
    src/util/executable_path.cpp
    src/resolver/git_cloner.cpp
    src/copy/copy_engine.cpp
    src/copy/filter.cpp
    src/generator/template_engine.cpp
    src/generator/cmake_generator.cpp
    src/generator/condition_evaluator.cpp
    src/generator/preset_generator.cpp
    src/generator/toolchain_generator.cpp
    src/generator/conan_generator.cpp
    src/generator/default_json_generator.cpp
    src/resolver/path_resolver.cpp
    src/interactive/metadata_builder.cpp
    src/interactive/condition_parser.cpp
    src/interactive/wizard.cpp
    ${ANTLR4_ConditionExpr_SOURCES}
)
target_include_directories(cmakegen_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ANTLR4_ConditionExpr_INCLUDE_DIR}
    ${antlr4_runtime_SOURCE_DIR}/runtime/Cpp/runtime/src
)
target_link_libraries(cmakegen_lib PUBLIC
    antlr4_static
    nlohmann_json::nlohmann_json
    fmt::fmt
    pantor::inja
    ftxui::component
)
target_compile_definitions(cmakegen_lib PUBLIC
    CMAKEGEN_TEMPLATES_DIR="${CMAKE_BINARY_DIR}/templates"
)
if(CMAKEGEN_USE_YAML)
    find_package(yaml-cpp QUIET)
    if(NOT yaml-cpp_FOUND)
        FetchContent_Declare(yaml-cpp
            GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
            GIT_TAG yaml-cpp-0.6.3
        )
        FetchContent_MakeAvailable(yaml-cpp)
    endif()
    if(TARGET yaml-cpp)
        target_link_libraries(cmakegen_lib PUBLIC yaml-cpp)
        target_compile_definitions(cmakegen_lib PUBLIC CMAKEGEN_HAS_YAML=1)
    endif()
endif()

# Main executable (interactive mode always included)
add_executable(cmakegen
    src/main.cpp
    src/interactive/add_runner.cpp
)
target_link_libraries(cmakegen PRIVATE cmakegen_lib CLI11::CLI11 ftxui::component)

# Install (runtime component only - excludes test deps)
include(GNUInstallDirs)
install(TARGETS cmakegen
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime
)
install(DIRECTORY templates/
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmakegen/templates
    COMPONENT runtime
    FILES_MATCHING PATTERN "*.jinja2"
)

# CPack (package only runtime: executable + templates)
set(CPACK_COMPONENTS_ALL runtime)
set(CPACK_PACKAGE_NAME "cmakegen")
set(CPACK_PACKAGE_VENDOR "CMakeGen")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CMake project generator for bare-metal embedded systems")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()
include(CPack)

# Tests
enable_testing()
add_subdirectory(tests)

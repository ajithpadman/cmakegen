from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.build import check_min_cppstd


class CmakegenConan(ConanFile):
    name = "cmakegen"
    version = "0.2.0"
    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("nlohmann_json/3.11.3")
        self.requires("yaml-cpp/0.8.0")
        self.requires("fmt/10.2.1")
        self.requires("gtest/1.14.0", visible=True)

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.23]")

    def validate(self):
        check_min_cppstd(self, "17")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

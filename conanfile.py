from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps

class cppmessgageQueue(ConanFile):
    name = "cpp-messgage-queue"
    version = "1.0"
    license = "na"
    author = "rsikder"
    url = ""
    description = ""
    topics = ("fmt", "example", "conan")
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"
    requires = ("fmt/10.1.1", "gtest/1.15.0")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["cpp-messgage-queue"]

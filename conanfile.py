from conans import ConanFile, CMake, tools

class SimGraphicsNativeConan(ConanFile):
  name = "SimGraphicsNative"
  version = "1.1.3"
  settings = "os", "compiler", "build_type", "arch"
  requires = (
    "vulkan_headers/1.1.130@wumo/stable",
    "vma/master@wumo/stable",
    "glfw/3.3@bincrafters/stable",
    "glm/0.9.9.6",
    "par_lib/master@wumo/stable",
    "tinygltf_lib/2.2.0@wumo/stable",
    "tinyobjloader/2.0.0-rc1@wumo/stable",
    "fmt/5.3.0@bincrafters/stable",
    "glslang/7.13.3496@wumo/stable",
    "imgui/1.73@wumo/stable",
    "gli/master@wumo/stable"
  )
  generators = "cmake"
  scm = {
    "type": "git",
    "subfolder": name,
    "url": "auto",
    "revision": "auto"
  }
  options = {
    "shared": [True, False],
    "basic": [True, False],
    "deferred": [True, False],
    "raytracing": [True, False],
  }
  default_options = {
    "shared": True,
    "basic": True,
    "deferred": False,
    "raytracing": False,
  }

  def build(self):
    cmake = CMake(self)
    cmake.definitions["BUILD_TEST"] = False
    cmake.definitions["BUILD_SHARED"] = self.options.shared
    cmake.definitions["BUILD_BASIC_RENDERER"] = self.options.basic
    cmake.definitions["BUILD_DEFERRED_RENDERER"] = self.options.deferred
    cmake.definitions["BUILD_RAYTRACING_RENDERER"] = self.options.raytracing
    cmake.configure(source_folder=self.name)
    cmake.build()

  def imports(self):
    self.copy("*.dll", dst="bin", src="bin")
    self.copy("*.dll", dst="bin", src="lib")
    self.copy("*.dylib", dst="bin", src="lib")
    self.copy("*.pdb", dst="bin", src="bin")
    self.copy("*", dst="bin/assets/public", src="resources")

  def package(self):
    self.copy("*.h", dst="include", src=f"{self.name}/src")
    self.copy("*.dll", dst="bin", src="bin", keep_path=False)
    self.copy("*.so", dst="bin", src="bin", keep_path=False)
    self.copy("*.so", dst="lib", src="lib", keep_path=False)
    self.copy("*.dylib", dst="bin", src="bin", keep_path=False)
    self.copy("*.a", dst="lib", src="lib", keep_path=False)
    self.copy("*.lib", dst="lib", src="lib", keep_path=False)
    self.copy("*", dst=f"resources/{self.name}",
              src=f"{self.name}/assets/public/{self.name}", symlinks=True)

  def package_info(self):
    self.cpp_info.libs = tools.collect_libs(self)

cmake_minimum_required(VERSION 3.14)
project(SimGraphicsNative LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/symlink.cmake)
symlink(assets bin/assets)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets/public/SimGraphicsNative)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan.cmake)
conan_cmake_run(BASIC_SETUP CONANFILE conanfile.py BUILD missing)

option(BUILD_TEST "Build test" ON)
option(BUILD_SHARED "Build shared lib" ON)

set(sources
  src/sim/util/range.h
  src/sim/graphics/base/config.h
  src/sim/graphics/base/debug_marker.h
  src/sim/graphics/base/device.cpp
  src/sim/graphics/base/device.h
  src/sim/graphics/base/glm_common.h
  src/sim/graphics/base/input.h
  src/sim/graphics/base/swapchain.cpp
  src/sim/graphics/base/swapchain.h
  src/sim/graphics/base/vkcommon.h
  src/sim/graphics/base/vulkan_base.cpp
  src/sim/graphics/base/vulkan_base.h
  
  src/sim/graphics/base/pipeline/descriptors.cpp
  src/sim/graphics/base/pipeline/descriptors.h
  src/sim/graphics/base/pipeline/descriptor_pool_maker.cpp
  src/sim/graphics/base/pipeline/descriptor_pool_maker.h
  src/sim/graphics/base/pipeline/pipeline.cpp
  src/sim/graphics/base/pipeline/pipeline.h
  src/sim/graphics/base/pipeline/render_pass.cpp
  src/sim/graphics/base/pipeline/render_pass.h
  src/sim/graphics/base/pipeline/sampler.h
  src/sim/graphics/base/pipeline/shader.cpp
  src/sim/graphics/base/pipeline/shader.h
  
  src/sim/graphics/base/resource/buffers.cpp
  src/sim/graphics/base/resource/buffers.h
  src/sim/graphics/base/resource/images.cpp
  src/sim/graphics/base/resource/images.h
  src/sim/graphics/base/resource/texture/texture_2d.cpp
  src/sim/graphics/base/resource/texture/texture_cube.cpp
  src/sim/graphics/base/resource/texture/texture_maker.cpp
  src/sim/graphics/base/resource/texture/image_copy.cpp
  src/sim/graphics/base/resource/texture/image_layout.cpp
  src/sim/graphics/base/resource/texture/image_upload.cpp
  src/sim/graphics/base/resource/texture/image_save.cpp
  src/sim/graphics/base/resource/texture/sampler_maker.cpp
  
  src/sim/graphics/util/fps_meter.cpp
  src/sim/graphics/util/fps_meter.h
  src/sim/graphics/util/colors.h
  
  src/sim/util/syntactic_sugar.h
  src/sim/util/syntactic_sugar.cpp
  )

set(oceanFFT
  src/sim/graphics/util/ocean/ocean_fft.cpp
  )

set(basicRendererSrc
  src/sim/graphics/renderer/basic/basic_model_manager.cpp
  src/sim/graphics/renderer/basic/basic_model_manager.h
  src/sim/graphics/renderer/basic/basic_renderer.cpp
  src/sim/graphics/renderer/basic/basic_renderer.h
  src/sim/graphics/renderer/basic/model_config.h
  src/sim/graphics/renderer/basic/perspective_camera.h
  src/sim/graphics/renderer/basic/builder/primitive_builder.cpp
  src/sim/graphics/renderer/basic/builder/primitive_builder.h
  src/sim/graphics/renderer/basic/ptr.h
  
  src/sim/graphics/renderer/basic/model/aabb.cpp
  src/sim/graphics/renderer/basic/model/aabb.h
  src/sim/graphics/renderer/basic/model/basic_model.h
  src/sim/graphics/renderer/basic/model/model_buffer.cpp
  src/sim/graphics/renderer/basic/model/model_buffer.h
  src/sim/graphics/renderer/basic/model/draw_queue.cpp
  src/sim/graphics/renderer/basic/model/draw_queue.h
  src/sim/graphics/renderer/basic/model/light.cpp
  src/sim/graphics/renderer/basic/model/light.h
  src/sim/graphics/renderer/basic/model/material.cpp
  src/sim/graphics/renderer/basic/model/material.h
  src/sim/graphics/renderer/basic/model/mesh.cpp
  src/sim/graphics/renderer/basic/model/mesh.h
  src/sim/graphics/renderer/basic/model/model.cpp
  src/sim/graphics/renderer/basic/model/model.h
  src/sim/graphics/renderer/basic/model/model_instance.cpp
  src/sim/graphics/renderer/basic/model/model_instance.h
  src/sim/graphics/renderer/basic/model/node.cpp
  src/sim/graphics/renderer/basic/model/node.h
  src/sim/graphics/renderer/basic/model/primitive.cpp
  src/sim/graphics/renderer/basic/model/primitive.h
  src/sim/graphics/renderer/basic/model/transform.cpp
  src/sim/graphics/renderer/basic/model/transform.h
  src/sim/graphics/renderer/basic/model/vertex.cpp
  src/sim/graphics/renderer/basic/model/vertex.h
  
  src/sim/graphics/renderer/basic/terrain/terrain_manager.cpp
  src/sim/graphics/renderer/basic/terrain/terrain_manager.h
  
  src/sim/graphics/renderer/basic/loader/gltf_loader.cpp
  src/sim/graphics/renderer/basic/loader/gltf_loader.h
  
  src/sim/graphics/renderer/basic/util/panning_camera.cpp
  src/sim/graphics/renderer/basic/util/panning_camera.h
  
  src/sim/graphics/renderer/basic/ibl/envmap_generator.h
  src/sim/graphics/renderer/basic/ibl/generate_brdflut.cpp
  src/sim/graphics/renderer/basic/ibl/generate_envmap.cpp
  
  src/sim/graphics/renderer/basic/sky/sky_renderer.cpp
  src/sim/graphics/renderer/basic/sky/sky_renderer.h
  src/sim/graphics/renderer/basic/sky/sky_model.h
  src/sim/graphics/renderer/basic/sky/sky_model.cpp
  src/sim/graphics/renderer/basic/sky/constants.h
  src/sim/graphics/renderer/basic/sky/compute_transmittance.cpp
  src/sim/graphics/renderer/basic/sky/compute_direct_irradiance.cpp
  src/sim/graphics/renderer/basic/sky/compute_indirect_irradiance.cpp
  src/sim/graphics/renderer/basic/sky/compute_multiple_scattering.cpp
  src/sim/graphics/renderer/basic/sky/compute_scattering_density.cpp
  src/sim/graphics/renderer/basic/sky/compute_single_scattering.cpp
  
  src/sim/graphics/renderer/basic/renderpass/infinite_plane.cpp
  src/sim/graphics/renderer/basic/renderpass/infinite_plane.h
  src/sim/graphics/renderer/basic/renderpass/deferred_pass.cpp
  src/sim/graphics/renderer/basic/renderpass/opaque_pass.cpp
  src/sim/graphics/renderer/basic/renderpass/translucent_pass.cpp
  src/sim/graphics/renderer/basic/renderpass/terrain_pass.cpp
  )

list(APPEND sources ${basicRendererSrc})

if(BUILD_SHARED)
  add_library(SimGraphicsNative SHARED ${sources})
else()
  add_library(SimGraphicsNative STATIC ${sources})
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/shader.cmake)
compileshader(${CMAKE_CURRENT_SOURCE_DIR}/src/sim/graphics/shaders
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sim/graphics/compiledShaders)
add_dependencies(SimGraphicsNative compileShader)

target_include_directories(SimGraphicsNative PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/src)
target_compile_definitions(SimGraphicsNative
  PUBLIC
  $<$<CONFIG:DEBUG>:DEBUG>
  )
target_link_libraries(SimGraphicsNative
  PUBLIC
  ${CONAN_LIBS}
  $<$<PLATFORM_ID:Linux>:dl>)

if(BUILD_TEST)
  file(GLOB_RECURSE test-sources CONFIGURE_DEPENDS "test/*.cpp")
  foreach(file ${test-sources})
    get_filename_component(comp ${file} NAME_WE)
    add_executable(${comp} ${file})
    target_include_directories(${comp} PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/test)
    target_link_libraries(${comp} PRIVATE SimGraphicsNative)
  endforeach()
endif()

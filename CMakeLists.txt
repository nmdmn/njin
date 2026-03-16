cmake_minimum_required(VERSION 3.24...3.30)
project(njin VERSION 0.1.0 LANGUAGES CXX)

include(cmake/njin-setup.cmake)
setup_build_type()
prevent_in_src_builds()
no_system_includes()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Dependencies ─────────────────────────────────────────────────
find_package(Vulkan REQUIRED)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# GLFW — window + input + Vulkan surface
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# GLM — math (header only)
FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
  GIT_SHALLOW    TRUE
)

# VMA — Vulkan Memory Allocator (header only)
FetchContent_Declare(vma
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        v3.1.0
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(glfw glm vma)

# ── Executable ────────────────────────────────────────────────────────────────
file(GLOB HEADERS src/*.hh)
file(GLOB SOURCES src/*.cc)

set(NJIN_NAME ${PROJECT_NAME})
set(NJIN_VERSION ${PROJECT_VERSION})
set(NJIN_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(NJIN_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(NJIN_VERSION_PATCH ${PROJECT_VERSION_PATCH})
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(NJIN_SHADER_FOLDER shaders)
else()
  set(NJIN_SHADER_FOLDER ${CMAKE_SOURCE_DIR}/shaders)
endif()
configure_file(src/config.hh.in src/config.hh)

add_executable(${NJIN_NAME}
  ${HEADERS}
  ${SOURCES}
)

target_include_directories(${NJIN_NAME} PRIVATE
  src/
  ${vma_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/src
)

target_link_libraries(${NJIN_NAME} PRIVATE
  Vulkan::Vulkan
  glfw
  glm
)

# GLM: use Vulkan-native depth range (0..1) and disable alignment assertions
target_compile_definitions(${NJIN_NAME} PRIVATE
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_RADIANS
  GLM_FORCE_LEFT_HANDED
)

# ── Compile GLSL shaders → SPIR-V ────────────────────────────────────────────
# glslangValidator ships with the glslang brew package
find_program(GLSLANG glslangValidator REQUIRED)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SPV_DIR    ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SPV_DIR})

# Collect all shader files
file(GLOB SHADERS
  ${SHADER_DIR}/*.vert
  ${SHADER_DIR}/*.frag
  ${SHADER_DIR}/*.comp
)

foreach(SHADER ${SHADERS})
  get_filename_component(SHADER_NAME ${SHADER} NAME)
  set(SPV_OUT ${SPV_DIR}/${SHADER_NAME}.spv)

  add_custom_command(
    OUTPUT  ${SPV_OUT}
    COMMAND ${GLSLANG} -V ${SHADER} -o ${SPV_OUT}
    DEPENDS ${SHADER}
    COMMENT "Compiling ${SHADER_NAME} → SPIR-V"
  )
  list(APPEND SPV_FILES ${SPV_OUT})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_FILES})
add_dependencies(njin shaders)

# Copy SPIR-V files next to binary after build
add_custom_command(TARGET njin POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${SPV_DIR} $<TARGET_FILE_DIR:njin>/shaders
)

set(IGGY3D_SHADER_SOURCE_ROOT
    "${PROJECT_SOURCE_DIR}/shaders/vulkan/src"
    CACHE PATH "Vulkan shader source root")

if(IGGY3D_SHADER_OUTPUT_DIR)
  set(IGGY3D_SHADER_BINARY_ROOT "${IGGY3D_SHADER_OUTPUT_DIR}")
else()
  set(IGGY3D_SHADER_BINARY_ROOT "${PROJECT_BINARY_DIR}/generated/shaders/vulkan")
endif()
set(IGGY3D_SHADER_BINARY_ROOT "${IGGY3D_SHADER_BINARY_ROOT}" CACHE PATH
    "Generated Vulkan SPIR-V root" FORCE)

set(_iggy3d_glslang_hints)
if(DEFINED ENV{VULKAN_SDK})
  list(APPEND _iggy3d_glslang_hints "$ENV{VULKAN_SDK}/bin")
endif()

if(IGGY3D_GLSLANG_VALIDATOR)
  set(IGGY3D_GLSLANG_VALIDATOR_EXE "${IGGY3D_GLSLANG_VALIDATOR}" CACHE FILEPATH
      "glslangValidator executable" FORCE)
else()
  find_program(IGGY3D_GLSLANG_VALIDATOR_EXE
    NAMES glslangValidator glslang
    HINTS ${_iggy3d_glslang_hints})
endif()

set(IGGY3D_SHADER_COMPILER_AVAILABLE OFF)
if(IGGY3D_GLSLANG_VALIDATOR_EXE)
  set(IGGY3D_SHADER_COMPILER_AVAILABLE ON)
endif()

if(IGGY3D_ENABLE_VULKAN_SHADERS)
  if(IGGY3D_SHADER_COMPILER_AVAILABLE)
    message(STATUS "iggy3d.shader.compiler=${IGGY3D_GLSLANG_VALIDATOR_EXE}")
    message(STATUS "iggy3d.shader.target_env=${IGGY3D_SHADER_TARGET_ENV}")
  elseif(IGGY3D_REQUIRE_GLSLANG)
    message(FATAL_ERROR "IGGY3D_REQUIRE_GLSLANG=ON but glslangValidator was not found")
  else()
    message(STATUS "iggy3d.shader.compiler=unavailable")
  endif()
endif()

set(IGGY3D_VULKAN_SHADER_OUTPUTS)

function(iggy3d_add_vulkan_shader)
  set(options)
  set(oneValueArgs TARGET STAGE SOURCE OUTPUT_NAME)
  set(multiValueArgs)
  cmake_parse_arguments(IGGY_SHADER "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT IGGY_SHADER_TARGET OR NOT IGGY_SHADER_STAGE OR NOT IGGY_SHADER_SOURCE OR
     NOT IGGY_SHADER_OUTPUT_NAME)
    message(FATAL_ERROR "iggy3d_add_vulkan_shader requires TARGET, STAGE, SOURCE, OUTPUT_NAME")
  endif()
  if(NOT TARGET "${IGGY_SHADER_TARGET}")
    add_custom_target("${IGGY_SHADER_TARGET}")
  endif()
  if(NOT IGGY3D_ENABLE_VULKAN_SHADERS OR NOT IGGY3D_SHADER_COMPILER_AVAILABLE)
    return()
  endif()

  set(source_path "${IGGY_SHADER_SOURCE}")
  if(NOT IS_ABSOLUTE "${source_path}")
    set(source_path "${PROJECT_SOURCE_DIR}/${source_path}")
  endif()
  set(output_path "${IGGY3D_SHADER_BINARY_ROOT}/${IGGY_SHADER_OUTPUT_NAME}")

  add_custom_command(
    OUTPUT "${output_path}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${IGGY3D_SHADER_BINARY_ROOT}"
    COMMAND "${IGGY3D_GLSLANG_VALIDATOR_EXE}" -V
            --target-env "${IGGY3D_SHADER_TARGET_ENV}"
            -S "${IGGY_SHADER_STAGE}"
            -o "${output_path}"
            "${source_path}"
    DEPENDS "${source_path}"
    VERBATIM
    USES_TERMINAL
    COMMENT "Compiling Vulkan shader ${IGGY_SHADER_OUTPUT_NAME}")
  add_custom_target("${IGGY_SHADER_TARGET}_${IGGY_SHADER_OUTPUT_NAME}" DEPENDS "${output_path}")
  add_dependencies("${IGGY_SHADER_TARGET}" "${IGGY_SHADER_TARGET}_${IGGY_SHADER_OUTPUT_NAME}")
  set(IGGY3D_VULKAN_SHADER_OUTPUTS "${IGGY3D_VULKAN_SHADER_OUTPUTS};${output_path}" PARENT_SCOPE)
endfunction()

function(iggy3d_add_vulkan_shader_set)
  set(options)
  set(oneValueArgs TARGET)
  set(multiValueArgs SOURCES)
  cmake_parse_arguments(IGGY_SHADER_SET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT IGGY_SHADER_SET_TARGET)
    message(FATAL_ERROR "iggy3d_add_vulkan_shader_set requires TARGET")
  endif()
  if(NOT TARGET "${IGGY_SHADER_SET_TARGET}")
    add_custom_target("${IGGY_SHADER_SET_TARGET}")
  endif()
  foreach(shader_source IN LISTS IGGY_SHADER_SET_SOURCES)
    get_filename_component(shader_name "${shader_source}" NAME)
    if(shader_name MATCHES "\\.vert\\.glsl$")
      string(REGEX REPLACE "\\.glsl$" ".spv" shader_output "${shader_name}")
      iggy3d_add_vulkan_shader(
        TARGET "${IGGY_SHADER_SET_TARGET}"
        STAGE vert
        SOURCE "${shader_source}"
        OUTPUT_NAME "${shader_output}")
    elseif(shader_name MATCHES "\\.frag\\.glsl$")
      string(REGEX REPLACE "\\.glsl$" ".spv" shader_output "${shader_name}")
      iggy3d_add_vulkan_shader(
        TARGET "${IGGY_SHADER_SET_TARGET}"
        STAGE frag
        SOURCE "${shader_source}"
        OUTPUT_NAME "${shader_output}")
    else()
      message(FATAL_ERROR "unsupported Vulkan shader source stage: ${shader_source}")
    endif()
  endforeach()
endfunction()

add_custom_target(iggy3d_vulkan_shaders)

iggy3d_add_vulkan_shader_set(
  TARGET iggy3d_vulkan_shaders
  SOURCES
    shaders/vulkan/src/first_room.vert.glsl
    shaders/vulkan/src/first_room.frag.glsl
    shaders/vulkan/src/material_unlit_textured.vert.glsl
    shaders/vulkan/src/material_unlit_textured.frag.glsl
    shaders/vulkan/src/static_mesh_instanced.vert.glsl
    shaders/vulkan/src/static_mesh_instanced_textured.vert.glsl)

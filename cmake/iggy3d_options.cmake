option(IGGY3D_WARNINGS_AS_ERRORS "Treat iggy3d warnings as errors" OFF)
option(IGGY3D_ENABLE_VULKAN "Enable the Creative Vulkan renderer" ON)
option(IGGY3D_USE_SYSTEM_SDL3 "Use system SDL3" ON)
option(IGGY3D_ENABLE_VULKAN_SHADERS "Build Vulkan shader artifacts" ON)
option(IGGY3D_REQUIRE_GLSLANG
       "Fail configure/build when glslangValidator is unavailable" OFF)

set(IGGY3D_GLSLANG_VALIDATOR "" CACHE FILEPATH
    "Path to glslangValidator executable")
set(IGGY3D_SHADER_TARGET_ENV "vulkan1.3" CACHE STRING
    "glslang --target-env value")
set(IGGY3D_SHADER_OUTPUT_DIR "" CACHE PATH
    "Generated Vulkan SPIR-V output root")

message(STATUS "IGGY3D_WARNINGS_AS_ERRORS=${IGGY3D_WARNINGS_AS_ERRORS}")
message(STATUS "iggy3d.vulkan.enabled=${IGGY3D_ENABLE_VULKAN}")
message(STATUS "iggy3d.vulkan.shaders=${IGGY3D_ENABLE_VULKAN_SHADERS}")

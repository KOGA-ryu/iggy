find_package(Vulkan QUIET)
find_package(PkgConfig QUIET)
find_program(GLSLC_EXECUTABLE glslc)

if(PkgConfig_FOUND)
  pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
endif()

if(Vulkan_FOUND AND SDL2_FOUND AND GLSLC_EXECUTABLE)
  set(IGGY_NATIVE_PLAY_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/native_play_shaders")
  set(IGGY_NATIVE_PLAY_VERTEX_SHADER "${IGGY_NATIVE_PLAY_SHADER_OUTPUT_DIR}/cube.vert.spv")
  set(IGGY_NATIVE_PLAY_FRAGMENT_SHADER "${IGGY_NATIVE_PLAY_SHADER_OUTPUT_DIR}/cube.frag.spv")

  add_custom_command(
    OUTPUT "${IGGY_NATIVE_PLAY_VERTEX_SHADER}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${IGGY_NATIVE_PLAY_SHADER_OUTPUT_DIR}"
    COMMAND "${GLSLC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/apps/native_play/shaders/cube.vert" -o "${IGGY_NATIVE_PLAY_VERTEX_SHADER}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/apps/native_play/shaders/cube.vert"
    VERBATIM
  )

  add_custom_command(
    OUTPUT "${IGGY_NATIVE_PLAY_FRAGMENT_SHADER}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${IGGY_NATIVE_PLAY_SHADER_OUTPUT_DIR}"
    COMMAND "${GLSLC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/apps/native_play/shaders/cube.frag" -o "${IGGY_NATIVE_PLAY_FRAGMENT_SHADER}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/apps/native_play/shaders/cube.frag"
    VERBATIM
  )

  add_custom_target(iggy_native_play_shaders
    DEPENDS
      "${IGGY_NATIVE_PLAY_VERTEX_SHADER}"
      "${IGGY_NATIVE_PLAY_FRAGMENT_SHADER}"
  )

  add_executable(iggy_native_play
    apps/native_play/IggyNativePlay.cpp
    apps/native_play/NativeProductSession.cpp
    apps/native_play/NativeVulkanRenderer.cpp
  )
  add_dependencies(iggy_native_play iggy_native_play_shaders)

  target_link_libraries(iggy_native_play PRIVATE
    iggy_engine
    Vulkan::Vulkan
    PkgConfig::SDL2
  )

  target_include_directories(iggy_native_play PRIVATE src)
  target_compile_definitions(iggy_native_play PRIVATE
    IGGY_NATIVE_PLAY_SHADER_DIR="${IGGY_NATIVE_PLAY_SHADER_OUTPUT_DIR}"
    IGGY_NATIVE_PLAY_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/apps/native_play/assets"
  )
else()
  if(NOT Vulkan_FOUND)
    message(STATUS "Vulkan not found; skipping iggy_native_play")
  endif()
  if(NOT SDL2_FOUND)
    message(STATUS "SDL2 not found; skipping iggy_native_play")
  endif()
  if(NOT GLSLC_EXECUTABLE)
    message(STATUS "glslc not found; skipping iggy_native_play")
  endif()
endif()

cmake_minimum_required(VERSION 3.24)
project(Paths VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CTest)

add_library(paths_model
  src/runtime/first_move/HuntSession.cpp
  src/runtime/first_move/LayeredQuestionSession.cpp)
target_include_directories(paths_model PUBLIC src)

option(PATHS_BUILD_NATIVE "Build the SDL3/Vulkan app and input tests" ON)
if(PATHS_BUILD_NATIVE)
  find_package(SDL3 CONFIG REQUIRED)
  find_package(Vulkan REQUIRED)
  add_library(paths_imgui
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_sdl3.cpp
    third_party/imgui/backends/imgui_impl_vulkan.cpp)
  target_include_directories(paths_imgui SYSTEM PUBLIC
    third_party/imgui third_party/imgui/backends)
  target_link_libraries(paths_imgui PRIVATE SDL3::SDL3 Vulkan::Vulkan)

  add_library(paths_ui src/ui/FirstMoveUi.cpp src/ui/LayeredQuestionUi.cpp)
  target_include_directories(paths_ui PUBLIC src src/ui)
  target_link_libraries(paths_ui PUBLIC paths_model PRIVATE paths_imgui SDL3::SDL3)

  add_library(paths_native src/platform/NativeVulkanHost.cpp
    vendor/iggy3d/src/render/RenderDiagnostics.cpp
    vendor/iggy3d/src/render/vulkan/VulkanMemoryAllocator.cpp
    vendor/iggy3d/src/render/vulkan/FrameCapture.cpp)
  target_include_directories(paths_native PUBLIC src PRIVATE vendor/iggy3d/src)
  target_compile_definitions(paths_native PRIVATE IGGY3D_HAS_VULKAN=1)
  target_link_libraries(paths_native PRIVATE paths_imgui SDL3::SDL3 Vulkan::Vulkan)
  if(APPLE)
    # MoltenVK is an external SDK dependency. Resolve its installed manifest
    # at configure time; explicit Vulkan driver environment overrides win.
    find_library(PATHS_MOLTENVK_LIBRARY NAMES MoltenVK)
    if(PATHS_MOLTENVK_LIBRARY)
      file(REAL_PATH "${PATHS_MOLTENVK_LIBRARY}" PATHS_MOLTENVK_REAL_LIBRARY)
      get_filename_component(PATHS_MOLTENVK_LIB_DIR "${PATHS_MOLTENVK_REAL_LIBRARY}" DIRECTORY)
      find_file(PATHS_VULKAN_ICD NAMES MoltenVK_icd.json
        HINTS "${PATHS_MOLTENVK_LIB_DIR}/.." "$ENV{VULKAN_SDK}"
        PATH_SUFFIXES etc/vulkan/icd.d share/vulkan/icd.d)
      if(PATHS_VULKAN_ICD)
        target_compile_definitions(paths_native PRIVATE PATHS_DEFAULT_VULKAN_ICD="${PATHS_VULKAN_ICD}")
      endif()
    endif()
  endif()

  add_executable(paths app/main.cpp)
  target_link_libraries(paths PRIVATE paths_ui paths_native)
endif()

if(BUILD_TESTING)
  add_executable(paths_hunt_tests tests/first_move_hunt_tests.cpp)
  target_link_libraries(paths_hunt_tests PRIVATE paths_model)
  add_test(NAME paths_hunt_tests COMMAND paths_hunt_tests)
  add_executable(paths_guided_tests tests/first_move_layered_question_tests.cpp)
  target_link_libraries(paths_guided_tests PRIVATE paths_model)
  add_test(NAME paths_guided_tests COMMAND paths_guided_tests)
  if(PATHS_BUILD_NATIVE)
    add_executable(paths_input_tests tests/first_move_input_tests.cpp)
    target_link_libraries(paths_input_tests PRIVATE paths_ui paths_imgui SDL3::SDL3)
    add_test(NAME paths_input_tests COMMAND paths_input_tests)
  endif()
endif()

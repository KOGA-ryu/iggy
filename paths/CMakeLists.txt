cmake_minimum_required(VERSION 3.24)
project(Paths VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CTest)

add_library(paths_scene
  src/scene/GalleryScene.cpp
  src/scene/TargetMotion.cpp
  vendor/iggy3d/src/core/math/Vec3.cpp
  vendor/iggy3d/src/core/math/Mat4.cpp
  vendor/iggy3d/src/core/math/Aabb3.cpp
  vendor/iggy3d/src/core/math/EulerRotation.cpp
  vendor/iggy3d/src/app/iggy3d/creative/camera/Fly.cpp
  vendor/iggy3d/src/app/iggy3d/creative/camera/ViewportNavigation.cpp
  vendor/iggy3d/src/app/iggy3d/creative/play/RuntimeMovingPlatforms.cpp)
target_include_directories(paths_scene PUBLIC src vendor/iggy3d/src)

add_library(paths_math_objects
  src/runtime/math_objects/MathObjects.cpp
  src/scene/MathObjectScene.cpp)
target_link_libraries(paths_math_objects PUBLIC paths_scene)

add_library(paths_model
  src/runtime/first_move/HuntSession.cpp
  src/runtime/first_move/LinearEquation.cpp
  src/runtime/first_move/MathNotation.cpp
  src/runtime/first_move/LayeredQuestionSession.cpp)
target_include_directories(paths_model PUBLIC src)

add_library(paths_json INTERFACE)
target_include_directories(paths_json SYSTEM INTERFACE third_party)
add_library(paths_content src/content/QuestionContentIO.cpp)
target_link_libraries(paths_content PUBLIC paths_model PRIVATE paths_json)
add_library(paths_math_corpus src/content/MathCorpus.cpp)
target_include_directories(paths_math_corpus PUBLIC src)
target_link_libraries(paths_math_corpus PUBLIC paths_model PRIVATE paths_json)

add_library(paths_sorter_model src/runtime/sorter/EquationSorterSession.cpp)
target_include_directories(paths_sorter_model PUBLIC src)
add_library(paths_sorter_content src/content/EquationSorterContentIO.cpp src/content/StudyProgressIO.cpp)
target_link_libraries(paths_sorter_content PUBLIC paths_sorter_model PRIVATE paths_json)

add_library(paths_gallery_model src/runtime/gallery/GallerySession.cpp)
target_link_libraries(paths_gallery_model PUBLIC paths_model paths_scene)
target_link_libraries(paths_sorter_model PUBLIC paths_gallery_model)
target_link_libraries(paths_sorter_content PRIVATE paths_content)

add_library(paths_gallery_menu src/ui/GalleryMenu.cpp)
target_link_libraries(paths_gallery_menu PUBLIC paths_gallery_model PRIVATE paths_content)

option(PATHS_BUILD_NATIVE "Build the SDL3/Vulkan app and input tests" ON)
if(PATHS_BUILD_NATIVE)
  find_package(SDL3 CONFIG REQUIRED)
  find_package(Vulkan REQUIRED)
  find_program(PATHS_GLSLANG_VALIDATOR NAMES glslangValidator glslang
    HINTS "$ENV{VULKAN_SDK}/bin" REQUIRED)
  foreach(stage vert frag)
    set(shader "${CMAKE_CURRENT_SOURCE_DIR}/shaders/first_room.${stage}.glsl")
    set(spirv "${CMAKE_CURRENT_BINARY_DIR}/generated/first_room.${stage}.spv")
    set(header "${CMAKE_CURRENT_BINARY_DIR}/generated/first_room_${stage}.hpp")
    add_custom_command(OUTPUT "${header}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
      COMMAND "${PATHS_GLSLANG_VALIDATOR}" -V --target-env vulkan1.1 -S ${stage} -o "${spirv}" "${shader}"
      COMMAND "${CMAKE_COMMAND}" "-DINPUT=${spirv}" "-DOUTPUT=${header}" "-DNAME=first_room_${stage}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedSpirv.cmake"
      DEPENDS "${shader}" cmake/EmbedSpirv.cmake
      BYPRODUCTS "${spirv}" VERBATIM)
    list(APPEND PATHS_SHADER_HEADERS "${header}")
  endforeach()
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
    ${PATHS_SHADER_HEADERS}
    vendor/iggy3d/src/render/RenderDiagnostics.cpp
    vendor/iggy3d/src/render/vulkan/VulkanMemoryAllocator.cpp
    vendor/iggy3d/src/render/vulkan/FrameCapture.cpp)
  target_include_directories(paths_native PUBLIC src PRIVATE vendor/iggy3d/src "${CMAKE_CURRENT_BINARY_DIR}/generated")
  target_compile_definitions(paths_native PRIVATE IGGY3D_HAS_VULKAN=1)
  target_link_libraries(paths_native PRIVATE paths_scene paths_imgui SDL3::SDL3 Vulkan::Vulkan)
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
  add_executable(math_lab app/math_lab_main.cpp)
  target_link_libraries(math_lab PRIVATE paths_math_objects paths_native paths_imgui SDL3::SDL3)
  add_executable(gallery app/gallery_main.cpp)
  target_link_libraries(gallery PRIVATE paths_native paths_gallery_menu paths_imgui SDL3::SDL3)
  add_library(paths_notation_ui src/ui/MathNotationUi.cpp)
  target_link_libraries(paths_notation_ui PUBLIC paths_model PRIVATE paths_imgui)
  add_library(paths_sorter_ui src/ui/EquationSorterUi.cpp src/ui/MathCorpusUi.cpp)
  target_link_libraries(paths_sorter_ui PUBLIC paths_sorter_model paths_notation_ui paths_math_corpus PRIVATE paths_imgui)
  add_executable(sorter app/sorter_main.cpp)
  target_link_libraries(sorter PRIVATE paths_sorter_ui paths_sorter_content paths_native paths_imgui paths_json SDL3::SDL3)
  add_custom_target(paths_sorter_content_files
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:sorter>/content/sorter"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/content/sorter"
      "$<TARGET_FILE_DIR:sorter>/content/sorter"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:sorter>/content/cards"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/content/cards"
      "$<TARGET_FILE_DIR:sorter>/content/cards"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/content/references"
      "$<TARGET_FILE_DIR:sorter>/content/references"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/content/corpus"
      "$<TARGET_FILE_DIR:sorter>/content/corpus"
    VERBATIM)
  add_dependencies(sorter paths_sorter_content_files)
  # Deploy editable content next to the executable, including multi-config builds.
  add_custom_target(paths_gallery_content
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/content" "$<TARGET_FILE_DIR:gallery>/content"
    VERBATIM)
  add_dependencies(gallery paths_gallery_content)
endif()

if(BUILD_TESTING)
  add_executable(paths_corpus_tests tests/math_corpus_tests.cpp)
  target_link_libraries(paths_corpus_tests PRIVATE paths_math_corpus paths_sorter_content paths_json)
  target_compile_definitions(paths_corpus_tests PRIVATE CORPUS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/corpus/toc.json"
    SORTER_STUDY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/study_practice_v1.json")
  add_test(NAME paths_corpus_tests COMMAND paths_corpus_tests)
  add_executable(paths_math_spectral_roots_tests tests/math_spectral_roots_tests.cpp)
  target_link_libraries(paths_math_spectral_roots_tests PRIVATE paths_math_objects)
  add_test(NAME paths_math_spectral_roots_tests COMMAND paths_math_spectral_roots_tests)
  add_executable(paths_math_statistics_tests tests/math_statistics_tests.cpp)
  target_link_libraries(paths_math_statistics_tests PRIVATE paths_math_objects)
  add_test(NAME paths_math_statistics_tests COMMAND paths_math_statistics_tests)
  add_executable(paths_math_shell_tensor_probability_tests tests/math_shell_tensor_probability_tests.cpp)
  target_link_libraries(paths_math_shell_tensor_probability_tests PRIVATE paths_math_objects)
  add_test(NAME paths_math_shell_tensor_probability_tests COMMAND paths_math_shell_tensor_probability_tests)
  add_executable(paths_math_number_field_tests tests/math_number_field_tests.cpp)
  target_link_libraries(paths_math_number_field_tests PRIVATE paths_math_objects)
  add_test(NAME paths_math_number_field_tests COMMAND paths_math_number_field_tests)
  add_executable(paths_math_exploration_tests tests/math_exploration_tests.cpp)
  target_link_libraries(paths_math_exploration_tests PRIVATE paths_math_objects)
  add_test(NAME paths_math_exploration_tests COMMAND paths_math_exploration_tests)
  add_executable(paths_math_layers_tests tests/math_layers_tests.cpp)
  target_link_libraries(paths_math_layers_tests PRIVATE paths_math_objects)
  add_test(NAME paths_math_layers_tests COMMAND paths_math_layers_tests)
  add_executable(paths_math_object_tests tests/math_object_tests.cpp)
  target_link_libraries(paths_math_object_tests PRIVATE paths_math_objects)
  add_test(NAME paths_math_object_tests COMMAND paths_math_object_tests)
  add_executable(paths_practice_save_tests tests/practice_save_tests.cpp)
  target_link_libraries(paths_practice_save_tests PRIVATE paths_sorter_content paths_json)
  target_compile_definitions(paths_practice_save_tests PRIVATE
    SORTER_STUDY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/study_practice_v1.json"
    SORTER_P036_SAVE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/practice_p036_v1.json")
  add_test(NAME paths_practice_save_tests COMMAND paths_practice_save_tests)
  add_test(NAME paths_practice_save_write COMMAND paths_practice_save_tests write "${CMAKE_CURRENT_BINARY_DIR}/practice-save-test.json")
  add_test(NAME paths_practice_save_read COMMAND paths_practice_save_tests read "${CMAKE_CURRENT_BINARY_DIR}/practice-save-test.json")
  set_tests_properties(paths_practice_save_write PROPERTIES FIXTURES_SETUP practice_saved)
  set_tests_properties(paths_practice_save_read PROPERTIES FIXTURES_REQUIRED practice_saved)
  add_executable(paths_math_moves_tests tests/math_moves_tests.cpp)
  target_link_libraries(paths_math_moves_tests PRIVATE paths_sorter_content paths_content paths_json)
  target_compile_definitions(paths_math_moves_tests PRIVATE SORTER_STUDY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/study_practice_v1.json")
  add_test(NAME paths_math_moves_tests COMMAND paths_math_moves_tests)
  add_executable(paths_matrix_practice_tests tests/matrix_practice_tests.cpp)
  target_link_libraries(paths_matrix_practice_tests PRIVATE paths_sorter_content paths_content paths_json)
  target_compile_definitions(paths_matrix_practice_tests PRIVATE MATRIX_PRACTICE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME paths_matrix_practice_tests COMMAND paths_matrix_practice_tests)
  add_executable(paths_sorter_tests tests/equation_sorter_tests.cpp)
  target_link_libraries(paths_sorter_tests PRIVATE paths_sorter_model)
  add_test(NAME paths_sorter_tests COMMAND paths_sorter_tests)
  add_executable(paths_sorter_content_tests tests/equation_sorter_content_tests.cpp)
  target_link_libraries(paths_sorter_content_tests PRIVATE paths_sorter_content paths_json)
  target_compile_definitions(paths_sorter_content_tests PRIVATE
    SORTER_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/equations_v1.json"
    SORTER_SOLUTIONS="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/equation_sorter_v1.solutions.json")
  add_test(NAME paths_sorter_content_tests COMMAND paths_sorter_content_tests)
  add_executable(paths_sorter_mixed_tests tests/equation_sorter_mixed_tests.cpp)
  target_link_libraries(paths_sorter_mixed_tests PRIVATE paths_sorter_content paths_json)
  target_compile_definitions(paths_sorter_mixed_tests PRIVATE
    SORTER_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/equations_v1.json"
    SORTER_MIXED_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/mixed_foundations_v1.json")
  add_test(NAME paths_sorter_mixed_tests COMMAND paths_sorter_mixed_tests)
  add_executable(paths_sorter_solve_tests tests/equation_sorter_solve_tests.cpp)
  target_link_libraries(paths_sorter_solve_tests PRIVATE paths_sorter_content paths_content paths_json)
  target_compile_definitions(paths_sorter_solve_tests PRIVATE
    SORTER_STUDY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/study_practice_v1.json"
    SORTER_BRACKET_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/bracket_practice_v1.json"
    SORTER_MIXED_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/mixed_foundations_v1.json")
  add_test(NAME paths_sorter_solve_tests COMMAND paths_sorter_solve_tests)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME paths_corpus_toc_tests COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/corpus_toc_tests.py")
  add_test(NAME paths_bracket_recipe_tests COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/bracket_recipe_tests.py")
  add_test(NAME paths_matrix_practice_recipe_tests COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/matrix_practice_recipe_tests.py")
  add_executable(paths_gallery_menu_tests tests/gallery_menu_tests.cpp)
  target_link_libraries(paths_gallery_menu_tests PRIVATE paths_gallery_menu paths_content paths_json)
  target_compile_definitions(paths_gallery_menu_tests PRIVATE PATHS_TEST_PACK="${CMAKE_CURRENT_SOURCE_DIR}/content/packs/gallery_foundation.json")
  add_test(NAME paths_gallery_menu_tests COMMAND paths_gallery_menu_tests)
  add_executable(paths_gallery_tests tests/gallery_session_tests.cpp)
  target_link_libraries(paths_gallery_tests PRIVATE paths_gallery_model paths_content)
  target_compile_definitions(paths_gallery_tests PRIVATE PATHS_TEST_PACK="${CMAKE_CURRENT_SOURCE_DIR}/content/packs/gallery_foundation.json")
  add_test(NAME paths_gallery_tests COMMAND paths_gallery_tests)
  add_executable(paths_scene_tests tests/gallery_scene_tests.cpp)
  target_link_libraries(paths_scene_tests PRIVATE paths_scene)
  add_test(NAME paths_scene_tests COMMAND paths_scene_tests)
  add_executable(paths_hunt_tests tests/first_move_hunt_tests.cpp)
  target_link_libraries(paths_hunt_tests PRIVATE paths_model)
  add_test(NAME paths_hunt_tests COMMAND paths_hunt_tests)
  add_executable(paths_guided_tests tests/first_move_layered_question_tests.cpp)
  target_link_libraries(paths_guided_tests PRIVATE paths_content)
  target_compile_definitions(paths_guided_tests PRIVATE PATHS_TEST_PACK="${CMAKE_CURRENT_SOURCE_DIR}/content/packs/gallery_foundation.json")
  add_test(NAME paths_guided_tests COMMAND paths_guided_tests)
  add_executable(paths_content_tests tests/question_content_tests.cpp)
  target_link_libraries(paths_content_tests PRIVATE paths_content paths_gallery_model paths_json)
  target_compile_definitions(paths_content_tests PRIVATE PATHS_TEST_PACK="${CMAKE_CURRENT_SOURCE_DIR}/content/packs/gallery_foundation.json")
  add_test(NAME paths_content_tests COMMAND paths_content_tests)
  if(PATHS_BUILD_NATIVE)
    add_test(NAME paths_math_function_cli COMMAND math_lab --validate --object function --level 3 --set function=2 --set at=0.5 --set degree=5 --check)
    add_test(NAME paths_math_matrix_cli COMMAND math_lab --validate --object linear --level 3 --preset 2 --check)
    add_test(NAME paths_math_surface_cli COMMAND math_lab --validate --object surface --level 3 --set surface=1 --set constraint=1 --set circle_angle=0 --check)
    add_test(NAME paths_math_symmetry_cli COMMAND math_lab --validate --object symmetry --level 1 --turn y --identity --turn x --turn z --undo-turn --turn y --check)
    add_test(NAME paths_math_harmonics_cli COMMAND math_lab --validate --object harmonics --level 3 --set waveform=0 --set probe_frequency=2 --advance 0.2 --check)
    add_test(NAME paths_math_oscillator_cli COMMAND math_lab --validate --object oscillator --level 3 --advance 8 --check)
    add_test(NAME paths_math_modular_cli COMMAND math_lab --validate --object modular --level 1 --set modulus=7 --set step=3 --reset-walk --step 1 --step -1 --step 1 --step 1 --step 1 --step 1 --step 1 --step 1 --check)
    add_test(NAME paths_math_gaussian_cli COMMAND math_lab --validate --object gaussian --level 3 --set quotient=0 --check)
    add_test(NAME paths_math_field_cli COMMAND math_lab --validate --object field --level 2 --set field=2 --set path=4 --reverse-path --set path_time=0 --advance 1 --check)
    add_test(NAME paths_math_flux_cli COMMAND math_lab --validate --object flux --level 3 --set flux_field=2 --set flux_orientation=1 --set boundary_time=0 --advance 1 --check)
    add_test(NAME paths_math_tensor_cli COMMAND math_lab --validate --object tensor --level 2 --set tensor_v0=1 --set tensor_v1=1 --set tensor_v2=0 --set tensor_w0=1 --set tensor_w1=-1 --set tensor_w2=0 --check)
    add_test(NAME paths_math_probability_cli COMMAND math_lab --validate --object probability --walk-step --reset-probability-walk --level 3 --set chain=1 --set chain_steps=12 --check)
    add_test(NAME paths_math_binomial_cli COMMAND math_lab --validate --object binomial --trial-step --reset-trials --level 3 --set trials=12 --set success_chance=0.5 --check)
    add_test(NAME paths_math_bayes_cli COMMAND math_lab --validate --object bayes --level 3 --set positive_evidence=3 --set negative_evidence=0 --check)
    add_test(NAME paths_math_covariance_cli COMMAND math_lab --validate --object covariance --level 3 --set whiten=1 --check)
    add_test(NAME paths_math_spherical_cli COMMAND math_lab --validate --object spherical --level 3 --set heat_time=0.5 --check)
    add_test(NAME paths_math_quadratic_cli COMMAND math_lab --validate --object quadratic --level 3 --set form_yaw=0 --set form_pitch=0 --set form_x=0 --set form_y=0 --set form_z=1 --check)
    add_test(NAME paths_math_roots_cli COMMAND math_lab --validate --object roots --level 3 --set root_n=8 --set root_auto=5 --check)
    set_tests_properties(paths_math_function_cli paths_math_matrix_cli paths_math_surface_cli paths_math_symmetry_cli paths_math_harmonics_cli paths_math_oscillator_cli paths_math_modular_cli paths_math_gaussian_cli paths_math_field_cli paths_math_flux_cli paths_math_tensor_cli paths_math_probability_cli paths_math_binomial_cli paths_math_bayes_cli paths_math_covariance_cli paths_math_spherical_cli paths_math_quadratic_cli paths_math_roots_cli
      PROPERTIES PASS_REGULAR_EXPRESSION "feedback=2")
    add_executable(paths_sorter_input_tests tests/equation_sorter_input_tests.cpp)
    target_link_libraries(paths_sorter_input_tests PRIVATE paths_sorter_ui paths_sorter_content paths_imgui)
    target_compile_definitions(paths_sorter_input_tests PRIVATE
      CORPUS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/corpus/toc.json"
      SORTER_STUDY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/study_practice_v1.json"
      SORTER_BRACKET_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/bracket_practice_v1.json"
      SORTER_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/equations_v1.json"
      SORTER_MIXED_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/content/sorter/mixed_foundations_v1.json")
    add_test(NAME paths_sorter_input_tests COMMAND paths_sorter_input_tests)
    add_test(NAME paths_content_startup_tests COMMAND "${CMAKE_COMMAND}"
      "-DGALLERY=$<TARGET_FILE:gallery>"
      "-DSOURCE_CONTENT=${CMAKE_CURRENT_SOURCE_DIR}/content"
      "-DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/content-startup-test"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/question_content_startup.cmake")
    add_executable(paths_input_tests tests/first_move_input_tests.cpp)
    target_link_libraries(paths_input_tests PRIVATE paths_ui paths_imgui SDL3::SDL3)
    add_test(NAME paths_input_tests COMMAND paths_input_tests)
  endif()
endif()

function(iggy3d_add_unit_test test_name source_file)
  set(full_source "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
  if(NOT EXISTS "${full_source}")
    message(FATAL_ERROR "missing iggy3d unit test source: ${source_file}")
  endif()
  add_executable("${test_name}" "${source_file}")
  target_link_libraries("${test_name}" PRIVATE iggy3d)
  iggy3d_apply_warnings("${test_name}")
  add_test(NAME "${test_name}" COMMAND "$<TARGET_FILE:${test_name}>")
  set_tests_properties("${test_name}" PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()

function(iggy3d_add_acceptance_test test_name source_file)
  set(full_source "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
  if(NOT EXISTS "${full_source}")
    message(FATAL_ERROR "missing iggy3d acceptance test source: ${source_file}")
  endif()
  add_executable("${test_name}" "${source_file}")
  target_link_libraries("${test_name}" PRIVATE iggy3d)
  iggy3d_apply_warnings("${test_name}")
  add_test(NAME "${test_name}" COMMAND "$<TARGET_FILE:${test_name}>")
  set_tests_properties("${test_name}" PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    LABELS "acceptance;runtime;iggy3d")
endfunction()

iggy3d_add_unit_test(math_tests tests/unit/math_tests.cpp)
set_tests_properties(math_tests PROPERTIES LABELS "unit;core;iggy3d")

iggy3d_add_unit_test(package_loader_tests tests/unit/package_loader_tests.cpp)
set_tests_properties(package_loader_tests PROPERTIES LABELS "unit;content;iggy3d")

iggy3d_add_unit_test(world_state_tests tests/unit/world_state_tests.cpp)
set_tests_properties(world_state_tests PROPERTIES LABELS "unit;runtime;world;iggy3d")

iggy3d_add_unit_test(clock_tests tests/unit/clock_tests.cpp)
set_tests_properties(clock_tests PROPERTIES LABELS "unit;runtime;clock;iggy3d")

iggy3d_add_unit_test(camera_mode_policy_tests tests/unit/camera_mode_policy_tests.cpp)
set_tests_properties(camera_mode_policy_tests PROPERTIES LABELS "unit;runtime;camera;iggy3d")

iggy3d_add_unit_test(command_admission_tests tests/unit/command_admission_tests.cpp)
set_tests_properties(command_admission_tests PROPERTIES LABELS "unit;runtime;command;iggy3d")

iggy3d_add_unit_test(session_state_tests tests/unit/session_state_tests.cpp)
set_tests_properties(session_state_tests PROPERTIES LABELS "unit;runtime;session;iggy3d")

iggy3d_add_unit_test(movement_system_tests tests/unit/movement_system_tests.cpp)
set_tests_properties(movement_system_tests PROPERTIES LABELS "unit;runtime;movement;iggy3d")

iggy3d_add_unit_test(target_reach_tests tests/unit/target_reach_tests.cpp)
set_tests_properties(target_reach_tests PROPERTIES LABELS "unit;runtime;targeting;iggy3d")

iggy3d_add_unit_test(inventory_system_tests tests/unit/inventory_system_tests.cpp)
set_tests_properties(inventory_system_tests PROPERTIES LABELS "unit;runtime;inventory;iggy3d")

iggy3d_add_unit_test(objective_system_tests tests/unit/objective_system_tests.cpp)
set_tests_properties(objective_system_tests PROPERTIES LABELS "unit;runtime;objective;iggy3d")

iggy3d_add_unit_test(interaction_system_tests tests/unit/interaction_system_tests.cpp)
set_tests_properties(interaction_system_tests PROPERTIES LABELS "unit;runtime;interaction;iggy3d")

iggy3d_add_unit_test(session_tick_tests tests/unit/session_tick_tests.cpp)
set_tests_properties(session_tick_tests PROPERTIES LABELS "unit;runtime;session;iggy3d")

iggy3d_add_unit_test(session_runner_tests tests/unit/session_runner_tests.cpp)
set_tests_properties(session_runner_tests PROPERTIES LABELS "unit;runtime;session;iggy3d")

iggy3d_add_unit_test(save_load_tests tests/unit/save_load_tests.cpp)
set_tests_properties(save_load_tests PROPERTIES LABELS "unit;runtime;save;iggy3d")

iggy3d_add_unit_test(projection_tests tests/unit/projection_tests.cpp)
set_tests_properties(projection_tests PROPERTIES LABELS "unit;runtime;projection;iggy3d")

iggy3d_add_unit_test(render_boundary_tests tests/unit/render_boundary_tests.cpp)
set_tests_properties(render_boundary_tests PROPERTIES LABELS "unit;render;boundary;iggy3d")

iggy3d_add_unit_test(render_config_tests tests/unit/render_config_tests.cpp)
set_tests_properties(render_config_tests PROPERTIES LABELS "unit;render;config;iggy3d")

iggy3d_add_unit_test(render_diagnostics_tests tests/unit/render_diagnostics_tests.cpp)
set_tests_properties(render_diagnostics_tests PROPERTIES LABELS "unit;render;diagnostics;iggy3d")

iggy3d_add_unit_test(render_projection_input_tests tests/unit/render_projection_input_tests.cpp)
set_tests_properties(render_projection_input_tests PROPERTIES LABELS "unit;render;frame_input;iggy3d")

iggy3d_add_unit_test(render_camera_frame_tests tests/unit/render_camera_frame_tests.cpp)
set_tests_properties(render_camera_frame_tests PROPERTIES LABELS "unit;render;camera;iggy3d")

iggy3d_add_unit_test(render_null_renderer_tests tests/unit/render_null_renderer_tests.cpp)
set_tests_properties(render_null_renderer_tests PROPERTIES LABELS "unit;render;null;iggy3d")

iggy3d_add_unit_test(render_replay_invariance_tests tests/unit/render_replay_invariance_tests.cpp)
set_tests_properties(render_replay_invariance_tests PROPERTIES
  LABELS "unit;render;replay;invariance;iggy3d")

iggy3d_add_unit_test(package_runtime_lookup_tests tests/unit/package_runtime_lookup_tests.cpp)
set_tests_properties(package_runtime_lookup_tests PROPERTIES LABELS "unit;render;package;iggy3d")

if(IGGY3D_ENABLE_VULKAN_SMOKE)
  add_executable(vulkan_platform_smoke tests/smoke/vulkan_platform_smoke.cpp)
  target_link_libraries(vulkan_platform_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_platform_smoke)
  if(IGGY3D_REQUIRE_VULKAN_SMOKE)
    target_compile_definitions(vulkan_platform_smoke PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
  endif()
  add_test(NAME vulkan_platform_smoke_window
           COMMAND "$<TARGET_FILE:vulkan_platform_smoke>" --mode window_only)
  add_test(NAME vulkan_platform_smoke_extensions
           COMMAND "$<TARGET_FILE:vulkan_platform_smoke>" --mode extension_query)
  set_tests_properties(vulkan_platform_smoke_window vulkan_platform_smoke_extensions PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    LABELS "smoke;vulkan;render;iggy3d")
endif()

iggy3d_add_acceptance_test(complete_runtime_demo_tests tests/acceptance/complete_runtime_demo_tests.cpp)
if(TARGET iggy3d_headless_demo)
  target_compile_definitions(complete_runtime_demo_tests
    PRIVATE
      IGGY3D_HEADLESS_DEMO_PATH="$<TARGET_FILE:iggy3d_headless_demo>")
endif()

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

iggy3d_add_unit_test(room_asset_loader_tests tests/unit/room_asset_loader_tests.cpp)
set_tests_properties(room_asset_loader_tests PROPERTIES LABELS "unit;content;room_asset;iggy3d")

iggy3d_add_unit_test(editable_room_document_tests tests/unit/editable_room_document_tests.cpp)
set_tests_properties(editable_room_document_tests PROPERTIES
  LABELS "unit;content;authoring;room_edit;iggy3d")

iggy3d_add_unit_test(save_file_store_tests tests/unit/save_file_store_tests.cpp)
set_tests_properties(save_file_store_tests PROPERTIES
  LABELS "unit;runtime;save;files;iggy3d")

iggy3d_add_unit_test(frontend_state_tests tests/unit/frontend_state_tests.cpp)
set_tests_properties(frontend_state_tests PROPERTIES LABELS "unit;app;frontend;iggy3d")

iggy3d_add_unit_test(frontend_route_tests tests/unit/frontend_route_tests.cpp)
set_tests_properties(frontend_route_tests PROPERTIES LABELS "unit;app;frontend;route;iggy3d")

iggy3d_add_unit_test(menu_input_tests tests/unit/menu_input_tests.cpp)
set_tests_properties(menu_input_tests PROPERTIES LABELS "unit;app;frontend;menu_input;iggy3d")

iggy3d_add_unit_test(pause_menu_tests tests/unit/pause_menu_tests.cpp)
set_tests_properties(pause_menu_tests PROPERTIES LABELS "unit;app;frontend;pause_menu;iggy3d")

iggy3d_add_unit_test(starter_screen_tests tests/unit/starter_screen_tests.cpp)
set_tests_properties(starter_screen_tests PROPERTIES LABELS "unit;app;frontend;starter;iggy3d")

iggy3d_add_unit_test(vertical_faded_selector_tests tests/unit/vertical_faded_selector_tests.cpp)
set_tests_properties(vertical_faded_selector_tests PROPERTIES
  LABELS "unit;app;frontend;selector;iggy3d")

iggy3d_add_unit_test(settings_menu_tests tests/unit/settings_menu_tests.cpp)
set_tests_properties(settings_menu_tests PROPERTIES LABELS "unit;app;frontend;settings;iggy3d")

iggy3d_add_unit_test(product_app_options_tests tests/unit/product_app_options_tests.cpp)
set_tests_properties(product_app_options_tests PROPERTIES
  LABELS "unit;app;product;options;iggy3d")

iggy3d_add_unit_test(product_window_renderer_lifecycle_tests
  tests/unit/product_window_renderer_lifecycle_tests.cpp)
set_tests_properties(product_window_renderer_lifecycle_tests PROPERTIES
  LABELS "unit;app;product;renderer;window;iggy3d")

iggy3d_add_unit_test(product_vulkan_room_frame_tests
  tests/unit/product_vulkan_room_frame_tests.cpp)
set_tests_properties(product_vulkan_room_frame_tests PROPERTIES
  LABELS "unit;app;product;renderer;vulkan;room;iggy3d")

iggy3d_add_unit_test(product_vulkan_menu_frame_tests
  tests/unit/product_vulkan_menu_frame_tests.cpp)
set_tests_properties(product_vulkan_menu_frame_tests PROPERTIES
  LABELS "unit;app;product;renderer;vulkan;menu;iggy3d")

iggy3d_add_unit_test(dev_tools_menu_tests tests/unit/dev_tools_menu_tests.cpp)
set_tests_properties(dev_tools_menu_tests PROPERTIES LABELS "unit;app;frontend;dev_tools;iggy3d")

iggy3d_add_unit_test(product_primitive_draw_list_tests tests/unit/product_primitive_draw_list_tests.cpp)
set_tests_properties(product_primitive_draw_list_tests PROPERTIES
  LABELS "unit;app;product;draw_list;iggy3d")

iggy3d_add_unit_test(product_viewport_framing_tests tests/unit/product_viewport_framing_tests.cpp)
set_tests_properties(product_viewport_framing_tests PROPERTIES
  LABELS "unit;app;product;viewport;iggy3d")

iggy3d_add_unit_test(product_render_bridge_tests tests/unit/product_render_bridge_tests.cpp)
set_tests_properties(product_render_bridge_tests PROPERTIES
  LABELS "unit;app;product;render_bridge;iggy3d")

iggy3d_add_unit_test(product_gameplay_feedback_tests tests/unit/product_gameplay_feedback_tests.cpp)
set_tests_properties(product_gameplay_feedback_tests PROPERTIES
  LABELS "unit;app;product;feedback;iggy3d")

iggy3d_add_unit_test(product_gameplay_controller_tests
  tests/unit/product_gameplay_controller_tests.cpp)
set_tests_properties(product_gameplay_controller_tests PROPERTIES
  LABELS "unit;app;product;gameplay;controls;iggy3d")

iggy3d_add_unit_test(product_interaction_mode_hud_tests
  tests/unit/product_interaction_mode_hud_tests.cpp)
set_tests_properties(product_interaction_mode_hud_tests PROPERTIES
  LABELS "unit;app;product;input;hud;iggy3d")

iggy3d_add_unit_test(product_mouse_capture_policy_tests
  tests/unit/product_mouse_capture_policy_tests.cpp)
set_tests_properties(product_mouse_capture_policy_tests PROPERTIES
  LABELS "unit;app;product;input;mouse;iggy3d")

iggy3d_add_unit_test(product_top_down_map_overlay_tests
  tests/unit/product_top_down_map_overlay_tests.cpp)
set_tests_properties(product_top_down_map_overlay_tests PROPERTIES
  LABELS "unit;app;product;viewport;top_down_map;iggy3d")

iggy3d_add_unit_test(product_ui_draw_list_tests
  tests/unit/product_ui_draw_list_tests.cpp)
set_tests_properties(product_ui_draw_list_tests PROPERTIES
  LABELS "unit;app;product;ui;draw_list;iggy3d")

iggy3d_add_unit_test(product_automation_command_registry_tests
  tests/unit/product_automation_command_registry_tests.cpp)
set_tests_properties(product_automation_command_registry_tests PROPERTIES
  LABELS "unit;app;product;automation;registry;iggy3d")

iggy3d_add_unit_test(product_builtin_dungeon_tests
  tests/unit/product_builtin_dungeon_tests.cpp)
set_tests_properties(product_builtin_dungeon_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;product;world_setup;ascii_room;iggy3d")

iggy3d_add_unit_test(product_dungeon_draft_tests
  tests/unit/product_dungeon_draft_tests.cpp)
set_tests_properties(product_dungeon_draft_tests PROPERTIES
  LABELS "unit;product;world_setup;ascii_room;draft;iggy3d")

iggy3d_add_unit_test(product_gameplay_tape_tests tests/unit/product_gameplay_tape_tests.cpp)
set_tests_properties(product_gameplay_tape_tests PROPERTIES
  LABELS "unit;app;product;gameplay;tape;iggy3d")

iggy3d_add_unit_test(product_gameplay_tape_runner_tests
  tests/unit/product_gameplay_tape_runner_tests.cpp)
set_tests_properties(product_gameplay_tape_runner_tests PROPERTIES
  LABELS "unit;app;product;gameplay;tape;runtime;iggy3d")

iggy3d_add_unit_test(product_interaction_mode_tests
  tests/unit/product_interaction_mode_tests.cpp)
set_tests_properties(product_interaction_mode_tests PROPERTIES
  LABELS "unit;app;product;input;interaction_mode;iggy3d")

iggy3d_add_unit_test(product_interaction_mode_state_tests
  tests/unit/product_interaction_mode_state_tests.cpp)
set_tests_properties(product_interaction_mode_state_tests PROPERTIES
  LABELS "unit;app;product;input;interaction_mode;state;iggy3d")

iggy3d_add_unit_test(product_controller_action_map_tests
  tests/unit/product_controller_action_map_tests.cpp)
set_tests_properties(product_controller_action_map_tests PROPERTIES
  LABELS "unit;app;product;input;controller;action_map;iggy3d")

iggy3d_add_unit_test(product_controller_action_routing_tests
  tests/unit/product_controller_action_routing_tests.cpp)
set_tests_properties(product_controller_action_routing_tests PROPERTIES
  LABELS "unit;app;product;input;controller;action_routing;iggy3d")

iggy3d_add_unit_test(product_movement_debug_hud_tests
  tests/unit/product_movement_debug_hud_tests.cpp)
set_tests_properties(product_movement_debug_hud_tests PROPERTIES
  LABELS "unit;app;product;movement;debug_hud;iggy3d")

iggy3d_add_unit_test(product_npc_behavior_debug_hud_tests
  tests/unit/product_npc_behavior_debug_hud_tests.cpp)
set_tests_properties(product_npc_behavior_debug_hud_tests PROPERTIES
  LABELS "unit;app;product;npc;debug_hud;iggy3d")

iggy3d_add_unit_test(product_frontend_router_tests tests/unit/product_frontend_router_tests.cpp)
set_tests_properties(product_frontend_router_tests PROPERTIES
  LABELS "unit;app;product;frontend;router;iggy3d")

iggy3d_add_unit_test(product_menu_transitions_tests tests/unit/product_menu_transitions_tests.cpp)
set_tests_properties(product_menu_transitions_tests PROPERTIES
  LABELS "unit;app;product;frontend;transitions;iggy3d")

iggy3d_add_unit_test(product_new_world_menu_action_tests
  tests/unit/product_new_world_menu_action_tests.cpp)
set_tests_properties(product_new_world_menu_action_tests PROPERTIES
  LABELS "unit;app;product;frontend;world_setup;draft;iggy3d")

iggy3d_add_unit_test(product_world_creation_tests tests/unit/product_world_creation_tests.cpp)
set_tests_properties(product_world_creation_tests PROPERTIES
  LABELS "unit;app;product;world_creation;iggy3d")

iggy3d_add_unit_test(product_save_bridge_tests tests/unit/product_save_bridge_tests.cpp)
set_tests_properties(product_save_bridge_tests PROPERTIES
  LABELS "unit;app;product;save;bridge;iggy3d")

iggy3d_add_unit_test(product_save_catalog_tests tests/unit/product_save_catalog_tests.cpp)
set_tests_properties(product_save_catalog_tests PROPERTIES
  LABELS "unit;app;product;save;catalog;iggy3d")

iggy3d_add_unit_test(product_package_session_seed_tests tests/unit/product_package_session_seed_tests.cpp)
set_tests_properties(product_package_session_seed_tests PROPERTIES
  LABELS "unit;app;product;package;session_seed;iggy3d")

iggy3d_add_unit_test(product_npc_profile_assignment_tests
  tests/unit/product_npc_profile_assignment_tests.cpp)
set_tests_properties(product_npc_profile_assignment_tests PROPERTIES
  LABELS "unit;app;product;npc;profile;assignment;iggy3d")

iggy3d_add_unit_test(product_saved_room_marker_binding_tests
  tests/unit/product_saved_room_marker_binding_tests.cpp)
set_tests_properties(product_saved_room_marker_binding_tests PROPERTIES
  LABELS "unit;app;product;save;room_marker;runtime;iggy3d")

iggy3d_add_unit_test(product_ascii_room_authoring_tests tests/unit/product_ascii_room_authoring_tests.cpp)
set_tests_properties(product_ascii_room_authoring_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;authoring;iggy3d")

iggy3d_add_unit_test(product_ascii_room_editing_tests tests/unit/product_ascii_room_editing_tests.cpp)
set_tests_properties(product_ascii_room_editing_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;authoring;editable_room;controller;iggy3d")

iggy3d_add_unit_test(product_ascii_room_activation_tests tests/unit/product_ascii_room_activation_tests.cpp)
set_tests_properties(product_ascii_room_activation_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;activation;iggy3d")

iggy3d_add_unit_test(product_active_room_state_tests tests/unit/product_active_room_state_tests.cpp)
set_tests_properties(product_active_room_state_tests PROPERTIES
  LABELS "unit;app;product;active_room;ascii_room;iggy3d")

iggy3d_add_unit_test(product_active_room_collision_tests tests/unit/product_active_room_collision_tests.cpp)
set_tests_properties(product_active_room_collision_tests PROPERTIES
  LABELS "unit;app;product;active_room;collision;ascii_room;iggy3d")

iggy3d_add_unit_test(ascii_room_source_tests tests/unit/ascii_room_source_tests.cpp)
set_tests_properties(ascii_room_source_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;source;iggy3d")

iggy3d_add_unit_test(ascii_room_grid_tests tests/unit/ascii_room_grid_tests.cpp)
set_tests_properties(ascii_room_grid_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;grid;iggy3d")

iggy3d_add_unit_test(ascii_room_to_authored_room_tests tests/unit/ascii_room_to_authored_room_tests.cpp)
set_tests_properties(ascii_room_to_authored_room_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;authored_room;iggy3d")

iggy3d_add_unit_test(ascii_room_to_editable_room_tests tests/unit/ascii_room_to_editable_room_tests.cpp)
set_tests_properties(ascii_room_to_editable_room_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;authoring;editable_room;iggy3d")

iggy3d_add_unit_test(editable_room_to_authored_room_tests
  tests/unit/editable_room_to_authored_room_tests.cpp)
set_tests_properties(editable_room_to_authored_room_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;authoring;editable_room;save;iggy3d")

iggy3d_add_unit_test(product_room_authoring_controller_tests
  tests/unit/product_room_authoring_controller_tests.cpp)
set_tests_properties(product_room_authoring_controller_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;authoring;editable_room;controller;iggy3d")

iggy3d_add_unit_test(product_room_geometry_optimization_tests
  tests/unit/product_room_geometry_optimization_tests.cpp)
set_tests_properties(product_room_geometry_optimization_tests PROPERTIES
  LABELS "unit;app;product;room_geometry;optimization;editable_room;iggy3d")

iggy3d_add_unit_test(product_room_editor_action_controller_tests
  tests/unit/product_room_editor_action_controller_tests.cpp)
set_tests_properties(product_room_editor_action_controller_tests PROPERTIES
  LABELS "unit;app;product;room_editor;controller;cursor;editable_room;iggy3d")

iggy3d_add_unit_test(room_editor_input_tests
  tests/unit/room_editor_input_tests.cpp)
set_tests_properties(room_editor_input_tests PROPERTIES
  LABELS "unit;app;input;product;room_editor;cursor;iggy3d")

iggy3d_add_unit_test(product_room_editor_cursor_tests
  tests/unit/product_room_editor_cursor_tests.cpp)
set_tests_properties(product_room_editor_cursor_tests PROPERTIES
  LABELS "unit;app;product;room_editor;cursor;editable_room;iggy3d")

iggy3d_add_unit_test(product_room_editor_overlay_tests
  tests/unit/product_room_editor_overlay_tests.cpp)
set_tests_properties(product_room_editor_overlay_tests PROPERTIES
  LABELS "unit;app;product;room_editor;overlay;cursor;iggy3d")

iggy3d_add_unit_test(product_room_editor_object_palette_tests
  tests/unit/product_room_editor_object_palette_tests.cpp)
set_tests_properties(product_room_editor_object_palette_tests PROPERTIES
  LABELS "unit;app;product;room_editor;object;palette;iggy3d")

iggy3d_add_unit_test(product_room_editor_preview_tests
  tests/unit/product_room_editor_preview_tests.cpp)
set_tests_properties(product_room_editor_preview_tests PROPERTIES
  LABELS "unit;app;product;room_editor;preview;optimization;editable_room;iggy3d")

iggy3d_add_unit_test(product_window_input_frame_tests
  tests/unit/product_window_input_frame_tests.cpp)
set_tests_properties(product_window_input_frame_tests PROPERTIES
  LABELS "unit;app;product;input;window;room_editor;mouse;preview;iggy3d")

iggy3d_add_unit_test(product_room_editing_state_tests
  tests/unit/product_room_editing_state_tests.cpp)
set_tests_properties(product_room_editing_state_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;authoring;editable_room;controller;active_room;collision;iggy3d")

iggy3d_add_unit_test(product_room_visual_proof_tests
  tests/unit/product_room_visual_proof_tests.cpp)
set_tests_properties(product_room_visual_proof_tests PROPERTIES
  LABELS "unit;app;product;room_visual_proof;draw_list;iggy3d")

iggy3d_add_unit_test(ascii_room_fixture_tests tests/unit/ascii_room_fixture_tests.cpp)
set_tests_properties(ascii_room_fixture_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;fixture;iggy3d")

iggy3d_add_unit_test(ascii_room_to_room_asset_tests tests/unit/ascii_room_to_room_asset_tests.cpp)
set_tests_properties(ascii_room_to_room_asset_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;room_asset;iggy3d")

iggy3d_add_unit_test(ascii_room_asset_text_tests tests/unit/ascii_room_asset_text_tests.cpp)
set_tests_properties(ascii_room_asset_text_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;asset_text;iggy3d")

iggy3d_add_unit_test(ascii_room_asset_text_fixture_tests tests/unit/ascii_room_asset_text_fixture_tests.cpp)
set_tests_properties(ascii_room_asset_text_fixture_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;asset_text;fixture;iggy3d")

iggy3d_add_unit_test(ascii_room_package_fixture_tests tests/unit/ascii_room_package_fixture_tests.cpp)
set_tests_properties(ascii_room_package_fixture_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;package;fixture;iggy3d")

iggy3d_add_unit_test(ascii_room_runtime_collision_tests tests/unit/ascii_room_runtime_collision_tests.cpp)
set_tests_properties(ascii_room_runtime_collision_tests PROPERTIES
  LABELS "unit;app;product;ascii_room;runtime;collision;iggy3d")

iggy3d_add_unit_test(save_slot_model_tests tests/unit/save_slot_model_tests.cpp)
set_tests_properties(save_slot_model_tests PROPERTIES LABELS "unit;app;frontend;save;iggy3d")

iggy3d_add_unit_test(save_browser_tests tests/unit/save_browser_tests.cpp)
set_tests_properties(save_browser_tests PROPERTIES LABELS "unit;app;frontend;save;iggy3d")

iggy3d_add_unit_test(world_setup_model_tests tests/unit/world_setup_model_tests.cpp)
set_tests_properties(world_setup_model_tests PROPERTIES
  LABELS "unit;app;frontend;world_setup;iggy3d")

iggy3d_add_unit_test(world_state_tests tests/unit/world_state_tests.cpp)
set_tests_properties(world_state_tests PROPERTIES LABELS "unit;runtime;world;iggy3d")

iggy3d_add_unit_test(clock_tests tests/unit/clock_tests.cpp)
set_tests_properties(clock_tests PROPERTIES LABELS "unit;runtime;clock;iggy3d")

iggy3d_add_unit_test(camera_mode_policy_tests tests/unit/camera_mode_policy_tests.cpp)
set_tests_properties(camera_mode_policy_tests PROPERTIES LABELS "unit;runtime;camera;iggy3d")

iggy3d_add_unit_test(gamepad_system_controls_tests tests/unit/gamepad_system_controls_tests.cpp)
set_tests_properties(gamepad_system_controls_tests PROPERTIES
  LABELS "unit;app;input;gamepad;iggy3d")

iggy3d_add_unit_test(command_admission_tests tests/unit/command_admission_tests.cpp)
set_tests_properties(command_admission_tests PROPERTIES LABELS "unit;runtime;command;iggy3d")

iggy3d_add_unit_test(npc_behavior_system_tests tests/unit/npc_behavior_system_tests.cpp)
set_tests_properties(npc_behavior_system_tests PROPERTIES
  LABELS "unit;runtime;ai;npc_behavior;iggy3d")

iggy3d_add_unit_test(npc_behavior_profile_tests tests/unit/npc_behavior_profile_tests.cpp)
set_tests_properties(npc_behavior_profile_tests PROPERTIES
  LABELS "unit;runtime;ai;npc_behavior;profile;iggy3d")

iggy3d_add_unit_test(npc_behavior_debug_snapshot_tests
  tests/unit/npc_behavior_debug_snapshot_tests.cpp)
set_tests_properties(npc_behavior_debug_snapshot_tests PROPERTIES
  LABELS "unit;runtime;ai;npc_behavior;debug;iggy3d")

iggy3d_add_unit_test(combat_system_tests tests/unit/combat_system_tests.cpp)
set_tests_properties(combat_system_tests PROPERTIES LABELS "unit;runtime;combat;iggy3d")

iggy3d_add_unit_test(combat_command_tests tests/unit/combat_command_tests.cpp)
set_tests_properties(combat_command_tests PROPERTIES LABELS "unit;runtime;command;combat;iggy3d")

iggy3d_add_unit_test(collision_query_tests tests/unit/collision_query_tests.cpp)
set_tests_properties(collision_query_tests PROPERTIES LABELS "unit;runtime;collision;iggy3d")

iggy3d_add_unit_test(entity_hit_query_tests tests/unit/entity_hit_query_tests.cpp)
set_tests_properties(entity_hit_query_tests PROPERTIES LABELS "unit;runtime;collision;entity;iggy3d")

iggy3d_add_unit_test(movement_policy_tests tests/unit/movement_policy_tests.cpp)
set_tests_properties(movement_policy_tests PROPERTIES LABELS "unit;runtime;movement;policy;iggy3d")

iggy3d_add_unit_test(movement_kinematics_tests tests/unit/movement_kinematics_tests.cpp)
set_tests_properties(movement_kinematics_tests PROPERTIES LABELS "unit;runtime;movement;math;iggy3d")

iggy3d_add_unit_test(movement_traversal_tests tests/unit/movement_traversal_tests.cpp)
set_tests_properties(movement_traversal_tests PROPERTIES LABELS "unit;runtime;movement;traversal;iggy3d")

iggy3d_add_unit_test(movement_traversal_slots_tests tests/unit/movement_traversal_slots_tests.cpp)
set_tests_properties(movement_traversal_slots_tests PROPERTIES
  LABELS "unit;runtime;movement;traversal;slots;iggy3d")

iggy3d_add_unit_test(player_motor_tests tests/unit/player_motor_tests.cpp)
set_tests_properties(player_motor_tests PROPERTIES LABELS "unit;runtime;player;movement;iggy3d")

iggy3d_add_unit_test(player_physics_move_planner_tests
  tests/unit/player_physics_move_planner_tests.cpp)
set_tests_properties(player_physics_move_planner_tests PROPERTIES
  LABELS "unit;runtime;player;physics_move;iggy3d")

iggy3d_add_unit_test(runtime_debug_snapshot_tests tests/unit/runtime_debug_snapshot_tests.cpp)
set_tests_properties(runtime_debug_snapshot_tests PROPERTIES LABELS "unit;runtime;debug;iggy3d")

iggy3d_add_unit_test(debug_hud_text_tests tests/unit/debug_hud_text_tests.cpp)
set_tests_properties(debug_hud_text_tests PROPERTIES LABELS "unit;render;debug;hud;iggy3d")

iggy3d_add_unit_test(bean_mesh_tests tests/unit/bean_mesh_tests.cpp)
set_tests_properties(bean_mesh_tests PROPERTIES LABELS "unit;render;mesh;bean;iggy3d")

iggy3d_add_unit_test(session_state_tests tests/unit/session_state_tests.cpp)
set_tests_properties(session_state_tests PROPERTIES LABELS "unit;runtime;session;iggy3d")

iggy3d_add_unit_test(movement_system_tests tests/unit/movement_system_tests.cpp)
set_tests_properties(movement_system_tests PROPERTIES LABELS "unit;runtime;movement;iggy3d")

iggy3d_add_unit_test(projectile_system_tests tests/unit/projectile_system_tests.cpp)
set_tests_properties(projectile_system_tests PROPERTIES LABELS "unit;runtime;projectile;iggy3d")

iggy3d_add_unit_test(ability_system_tests tests/unit/ability_system_tests.cpp)
set_tests_properties(ability_system_tests PROPERTIES LABELS "unit;runtime;ability;iggy3d")

iggy3d_add_unit_test(ability_command_tests tests/unit/ability_command_tests.cpp)
set_tests_properties(ability_command_tests PROPERTIES LABELS "unit;runtime;ability;command;iggy3d")

iggy3d_add_unit_test(target_reach_tests tests/unit/target_reach_tests.cpp)
set_tests_properties(target_reach_tests PROPERTIES LABELS "unit;runtime;targeting;iggy3d")

iggy3d_add_unit_test(inventory_system_tests tests/unit/inventory_system_tests.cpp)
set_tests_properties(inventory_system_tests PROPERTIES LABELS "unit;runtime;inventory;iggy3d")

iggy3d_add_unit_test(objective_system_tests tests/unit/objective_system_tests.cpp)
set_tests_properties(objective_system_tests PROPERTIES LABELS "unit;runtime;objective;iggy3d")

iggy3d_add_unit_test(interaction_system_tests tests/unit/interaction_system_tests.cpp)
set_tests_properties(interaction_system_tests PROPERTIES LABELS "unit;runtime;interaction;iggy3d")

iggy3d_add_unit_test(object_traits_tests tests/unit/object_traits_tests.cpp)
set_tests_properties(object_traits_tests PROPERTIES LABELS "unit;runtime;object;traits;iggy3d")

iggy3d_add_unit_test(physics_body_store_tests tests/unit/physics_body_store_tests.cpp)
set_tests_properties(physics_body_store_tests PROPERTIES
  LABELS "unit;runtime;physics;body_store;iggy3d")

iggy3d_add_unit_test(physics_body_delta_accumulator_tests
  tests/unit/physics_body_delta_accumulator_tests.cpp)
set_tests_properties(physics_body_delta_accumulator_tests PROPERTIES
  LABELS "unit;runtime;physics;body_delta;iggy3d")

iggy3d_add_unit_test(physics_collider_bake_tests
  tests/unit/physics_collider_bake_tests.cpp)
set_tests_properties(physics_collider_bake_tests PROPERTIES
  LABELS "unit;runtime;physics;collider_bake;iggy3d")

iggy3d_add_unit_test(physics_collision_queries_tests
  tests/unit/physics_collision_queries_tests.cpp)
set_tests_properties(physics_collision_queries_tests PROPERTIES
  LABELS "unit;runtime;physics;collision_queries;iggy3d")

iggy3d_add_unit_test(physics_kinematic_motor_tests
  tests/unit/physics_kinematic_motor_tests.cpp)
set_tests_properties(physics_kinematic_motor_tests PROPERTIES
  LABELS "unit;runtime;physics;kinematic_motor;iggy3d")

iggy3d_add_unit_test(physics_spatial_surface_collider_bake_tests
  tests/unit/physics_spatial_surface_collider_bake_tests.cpp)
set_tests_properties(physics_spatial_surface_collider_bake_tests PROPERTIES
  LABELS "unit;runtime;physics;spatial_surface_bake;iggy3d")

iggy3d_add_unit_test(physics_shape_store_tests tests/unit/physics_shape_store_tests.cpp)
set_tests_properties(physics_shape_store_tests PROPERTIES
  LABELS "unit;runtime;physics;shape_store;iggy3d")

iggy3d_add_unit_test(physics_material_traits_tests
  tests/unit/physics_material_traits_tests.cpp)
set_tests_properties(physics_material_traits_tests PROPERTIES
  LABELS "unit;runtime;physics;material_traits;iggy3d")

iggy3d_add_unit_test(physics_aabb_collider_tests
  tests/unit/physics_aabb_collider_tests.cpp)
set_tests_properties(physics_aabb_collider_tests PROPERTIES
  LABELS "unit;runtime;physics;aabb;collider;iggy3d")

iggy3d_add_unit_test(physics_aabb_collision_batch_tests
  tests/unit/physics_aabb_collision_batch_tests.cpp)
set_tests_properties(physics_aabb_collision_batch_tests PROPERTIES
  LABELS "unit;runtime;physics;aabb;collision_batch;iggy3d")

iggy3d_add_unit_test(physics_aabb_step_tests
  tests/unit/physics_aabb_step_tests.cpp)
set_tests_properties(physics_aabb_step_tests PROPERTIES
  LABELS "unit;runtime;physics;aabb;step;iggy3d")

iggy3d_add_unit_test(physics_aabb_lab_tests
  tests/unit/physics_aabb_lab_tests.cpp)
set_tests_properties(physics_aabb_lab_tests PROPERTIES
  LABELS "unit;runtime;physics;aabb;lab;iggy3d")

iggy3d_add_unit_test(physics_aabb_contact_tests
  tests/unit/physics_aabb_contact_tests.cpp)
set_tests_properties(physics_aabb_contact_tests PROPERTIES
  LABELS "unit;runtime;physics;aabb;contact;iggy3d")

iggy3d_add_unit_test(physics_aabb_contact_solver_tests
  tests/unit/physics_aabb_contact_solver_tests.cpp)
set_tests_properties(physics_aabb_contact_solver_tests PROPERTIES
  LABELS "unit;runtime;physics;aabb;contact_solver;iggy3d")

iggy3d_add_unit_test(physics_broadphase_tests
  tests/unit/physics_broadphase_tests.cpp)
set_tests_properties(physics_broadphase_tests PROPERTIES
  LABELS "unit;runtime;physics;broadphase;aabb;iggy3d")

iggy3d_add_unit_test(physics_step_tests tests/unit/physics_step_tests.cpp)
set_tests_properties(physics_step_tests PROPERTIES
  LABELS "unit;runtime;physics;step;iggy3d")

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

  add_executable(product_gameplay_controls_smoke
    tests/smoke/product_gameplay_controls_smoke.cpp)
  target_link_libraries(product_gameplay_controls_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(product_gameplay_controls_smoke)
  target_compile_definitions(product_gameplay_controls_smoke
    PRIVATE
      IGGY3D_PRODUCT_APP_PATH="$<TARGET_FILE:iggy3d_app>")
  add_dependencies(product_gameplay_controls_smoke iggy3d_app)
  add_test(NAME product_gameplay_controls_smoke
           COMMAND "$<TARGET_FILE:product_gameplay_controls_smoke>")
  set_tests_properties(product_gameplay_controls_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    SKIP_RETURN_CODE 77
    LABELS "smoke;product;gameplay;controls;frontend;no_window;iggy3d")

  add_executable(product_menu_transition_smoke
    tests/smoke/product_menu_transition_smoke.cpp)
  target_link_libraries(product_menu_transition_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(product_menu_transition_smoke)
  target_compile_definitions(product_menu_transition_smoke
    PRIVATE
      IGGY3D_PRODUCT_APP_PATH="$<TARGET_FILE:iggy3d_app>")
  add_dependencies(product_menu_transition_smoke iggy3d_app)
  add_test(NAME product_menu_transition_smoke
           COMMAND "$<TARGET_FILE:product_menu_transition_smoke>")
  set_tests_properties(product_menu_transition_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    SKIP_RETURN_CODE 77
    LABELS "smoke;product;frontend;transitions;no_window;iggy3d")

  add_executable(product_ascii_package_smoke
    tests/smoke/product_ascii_package_smoke.cpp)
  target_link_libraries(product_ascii_package_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(product_ascii_package_smoke)
  target_compile_definitions(product_ascii_package_smoke
    PRIVATE
      IGGY3D_PRODUCT_APP_PATH="$<TARGET_FILE:iggy3d_app>")
  add_dependencies(product_ascii_package_smoke iggy3d_app)
  add_test(NAME product_ascii_package_smoke
           COMMAND "$<TARGET_FILE:product_ascii_package_smoke>")
  set_tests_properties(product_ascii_package_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    SKIP_RETURN_CODE 77
    LABELS "smoke;product;ascii_room;package;no_window;iggy3d")

  add_executable(product_ascii_authoring_smoke
    tests/smoke/product_ascii_authoring_smoke.cpp)
  target_link_libraries(product_ascii_authoring_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(product_ascii_authoring_smoke)
  target_compile_definitions(product_ascii_authoring_smoke
    PRIVATE
      IGGY3D_PRODUCT_APP_PATH="$<TARGET_FILE:iggy3d_app>")
  add_dependencies(product_ascii_authoring_smoke iggy3d_app)
  add_test(NAME product_ascii_authoring_smoke
           COMMAND "$<TARGET_FILE:product_ascii_authoring_smoke>")
  set_tests_properties(product_ascii_authoring_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    SKIP_RETURN_CODE 77
    LABELS "smoke;product;ascii_room;authoring;automation;no_window;iggy3d")

  add_executable(product_room_editing_automation_smoke
    tests/smoke/product_room_editing_automation_smoke.cpp)
  target_link_libraries(product_room_editing_automation_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(product_room_editing_automation_smoke)
  target_compile_definitions(product_room_editing_automation_smoke
    PRIVATE
      IGGY3D_PRODUCT_APP_PATH="$<TARGET_FILE:iggy3d_app>")
  add_dependencies(product_room_editing_automation_smoke iggy3d_app)
  add_test(NAME product_room_editing_automation_smoke
           COMMAND "$<TARGET_FILE:product_room_editing_automation_smoke>")
  set_tests_properties(product_room_editing_automation_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    SKIP_RETURN_CODE 77
    LABELS "smoke;product;ascii_room;authoring;editable_room;automation;no_window;iggy3d")

  add_executable(product_ascii_gameplay_loop_smoke
    tests/smoke/product_ascii_gameplay_loop_smoke.cpp)
  target_link_libraries(product_ascii_gameplay_loop_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(product_ascii_gameplay_loop_smoke)
  add_test(NAME product_ascii_gameplay_loop_smoke
           COMMAND "$<TARGET_FILE:product_ascii_gameplay_loop_smoke>")
  set_tests_properties(product_ascii_gameplay_loop_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    LABELS "smoke;product;ascii_room;gameplay;runtime;no_window;iggy3d")

  add_executable(product_menu_usefulness_smoke
    tests/smoke/product_menu_usefulness_smoke.cpp)
  target_link_libraries(product_menu_usefulness_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(product_menu_usefulness_smoke)
  target_compile_definitions(product_menu_usefulness_smoke
    PRIVATE
      IGGY3D_PRODUCT_APP_PATH="$<TARGET_FILE:iggy3d_app>")
  add_dependencies(product_menu_usefulness_smoke iggy3d_app)
  add_test(NAME product_menu_usefulness_smoke
           COMMAND "$<TARGET_FILE:product_menu_usefulness_smoke>")
  set_tests_properties(product_menu_usefulness_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    SKIP_RETURN_CODE 77
    LABELS "smoke;product;frontend;menu;usefulness;no_window;iggy3d")

  function(iggy3d_add_product_app_automation_smoke test_name source_file extra_labels)
    add_executable("${test_name}" "${source_file}")
    target_link_libraries("${test_name}" PRIVATE iggy3d)
    iggy3d_apply_warnings("${test_name}")
    target_compile_definitions("${test_name}"
      PRIVATE
        IGGY3D_PRODUCT_APP_PATH="$<TARGET_FILE:iggy3d_app>")
    add_dependencies("${test_name}" iggy3d_app)
    add_test(NAME "${test_name}" COMMAND "$<TARGET_FILE:${test_name}>")
    set_tests_properties("${test_name}" PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      SKIP_RETURN_CODE 77
      LABELS "smoke;product;frontend;automation;no_window;iggy3d;${extra_labels}")
  endfunction()

  iggy3d_add_product_app_automation_smoke(
    product_automation_menu_smoke
    tests/smoke/product_automation_menu_smoke.cpp
    "menu")
  iggy3d_add_product_app_automation_smoke(
    product_world_setup_smoke
    tests/smoke/product_world_setup_smoke.cpp
    "world_setup;save")
  iggy3d_add_product_app_automation_smoke(
    product_save_load_smoke
    tests/smoke/product_save_load_smoke.cpp
    "save;load")
  iggy3d_add_product_app_automation_smoke(
    product_save_delete_recover_smoke
    tests/smoke/product_save_delete_recover_smoke.cpp
    "save;delete;recover")
  iggy3d_add_product_app_automation_smoke(
    product_pause_save_smoke
    tests/smoke/product_pause_save_smoke.cpp
    "pause;save")
  iggy3d_add_product_app_automation_smoke(
    product_startup_lifecycle_smoke
    tests/smoke/product_startup_lifecycle_smoke.cpp
    "startup;world_setup;save;load;lifecycle")
  iggy3d_add_product_app_automation_smoke(
    product_ascii_map_smoke
    tests/smoke/product_ascii_map_smoke.cpp
    "ascii_room;map;world_setup;save;load")
  iggy3d_add_product_app_automation_smoke(
    product_room_visual_proof_smoke
    tests/smoke/product_room_visual_proof_smoke.cpp
    "ascii_room;map;save;load;visual_proof")
  iggy3d_add_product_app_automation_smoke(
    product_continued_room_movement_smoke
    tests/smoke/product_continued_room_movement_smoke.cpp
    "ascii_room;map;save;load;gameplay;movement;collision")
  iggy3d_add_product_app_automation_smoke(
    product_editor_floor_save_continue_smoke
    tests/smoke/product_editor_floor_save_continue_smoke.cpp
    "ascii_room;map;save;load;room_editor;floor")
  iggy3d_add_product_app_automation_smoke(
    product_editor_combined_save_continue_smoke
    tests/smoke/product_editor_combined_save_continue_smoke.cpp
    "ascii_room;map;save;load;room_editor;floor;wall")
  iggy3d_add_product_app_automation_smoke(
    product_editor_wall_direction_hotkey_smoke
    tests/smoke/product_editor_wall_direction_hotkey_smoke.cpp
    "ascii_room;map;save;load;room_editor;wall")
  iggy3d_add_product_app_automation_smoke(
    product_controller_input_smoke
    tests/smoke/product_controller_input_smoke.cpp
    "input;controller;interaction_mode")

  add_executable(product_gameplay_tape_smoke
    tests/smoke/product_gameplay_tape_smoke.cpp)
  target_link_libraries(product_gameplay_tape_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(product_gameplay_tape_smoke)
  target_compile_definitions(product_gameplay_tape_smoke
    PRIVATE
      IGGY3D_PRODUCT_APP_PATH="$<TARGET_FILE:iggy3d_app>")
  add_dependencies(product_gameplay_tape_smoke iggy3d_app)
  add_test(NAME product_gameplay_tape_smoke
           COMMAND "$<TARGET_FILE:product_gameplay_tape_smoke>")
  set_tests_properties(product_gameplay_tape_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    SKIP_RETURN_CODE 77
    LABELS "smoke;product;gameplay;tape;ascii_room;no_window;iggy3d")

function(iggy3d_add_render_packet4_unit_test test_name source_file)
  set(full_source "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
  if(NOT EXISTS "${full_source}")
    message(FATAL_ERROR "missing iggy3d packet4 unit test source: ${source_file}")
  endif()
  if(IGGY3D_BUILD_VULKAN_BACKEND)
    add_executable("${test_name}" "${source_file}")
  else()
    add_executable("${test_name}" "${source_file}"
      src/render/vulkan/VulkanResult.cpp
      src/render/vulkan/VulkanFeatureSupport.cpp)
  endif()
  target_link_libraries("${test_name}" PRIVATE iggy3d)
  iggy3d_apply_warnings("${test_name}")
  add_test(NAME "${test_name}" COMMAND "$<TARGET_FILE:${test_name}>")
  set_tests_properties("${test_name}" PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    LABELS "unit;render;vulkan;iggy3d")
endfunction()

iggy3d_add_render_packet4_unit_test(render_result_mapping_tests
  tests/unit/render_result_mapping_tests.cpp)
iggy3d_add_render_packet4_unit_test(render_reason_code_tests
  tests/unit/render_reason_code_tests.cpp)
iggy3d_add_render_packet4_unit_test(render_unsupported_device_policy_tests
  tests/unit/render_unsupported_device_policy_tests.cpp)

function(iggy3d_add_render_packet6_unit_test test_name source_file)
  set(full_source "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
  if(NOT EXISTS "${full_source}")
    message(FATAL_ERROR "missing iggy3d packet6 unit test source: ${source_file}")
  endif()
  if(IGGY3D_BUILD_VULKAN_BACKEND)
    add_executable("${test_name}" "${source_file}")
  else()
    add_executable("${test_name}" "${source_file}"
      src/render/vulkan/VulkanResult.cpp
      src/render/vulkan/ShaderModule.cpp
      src/render/vulkan/PipelineLayout.cpp
      src/render/vulkan/FirstRoomPipeline.cpp
      src/render/vulkan/VulkanMemoryAllocator.cpp
      src/render/vulkan/BufferImageResources.cpp
      src/render/vulkan/DescriptorSets.cpp)
  endif()
  target_link_libraries("${test_name}" PRIVATE iggy3d)
  target_compile_definitions("${test_name}"
    PRIVATE
      IGGY3D_SHADER_SOURCE_ROOT_VALUE="${IGGY3D_SHADER_SOURCE_ROOT}"
      IGGY3D_SHADER_BINARY_ROOT_VALUE="${IGGY3D_SHADER_BINARY_ROOT}"
      IGGY3D_SHADER_TARGET_ENV_VALUE="${IGGY3D_SHADER_TARGET_ENV}")
  if(IGGY3D_SHADER_COMPILER_AVAILABLE)
    target_compile_definitions("${test_name}" PRIVATE IGGY3D_SHADER_COMPILER_AVAILABLE=1)
  endif()
  iggy3d_apply_warnings("${test_name}")
  add_test(NAME "${test_name}" COMMAND "$<TARGET_FILE:${test_name}>")
  set_tests_properties("${test_name}" PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    LABELS "unit;render;vulkan;packet6;iggy3d")
endfunction()

iggy3d_add_render_packet6_unit_test(render_shader_interface_tests
  tests/unit/render_shader_interface_tests.cpp)
iggy3d_add_render_packet6_unit_test(render_vertex_format_tests
  tests/unit/render_vertex_format_tests.cpp)
iggy3d_add_render_packet6_unit_test(render_shader_build_policy_tests
  tests/unit/render_shader_build_policy_tests.cpp)
iggy3d_add_render_packet6_unit_test(render_memory_budget_policy_tests
  tests/unit/render_memory_budget_policy_tests.cpp)
iggy3d_add_render_packet6_unit_test(render_room_mesh_geometry_tests
  tests/unit/render_room_mesh_geometry_tests.cpp)

add_executable(package_headless_smoke tests/smoke/package_headless_smoke.cpp)
target_link_libraries(package_headless_smoke PRIVATE iggy3d)
iggy3d_apply_warnings(package_headless_smoke)
if(TARGET iggy3d_headless_demo)
  target_compile_definitions(package_headless_smoke
    PRIVATE
      IGGY3D_HEADLESS_DEMO_PATH="$<TARGET_FILE:iggy3d_headless_demo>")
endif()
add_test(NAME package_headless_smoke COMMAND "$<TARGET_FILE:package_headless_smoke>")
set_tests_properties(package_headless_smoke PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "smoke;package;headless;iggy3d")

if(TARGET iggy3d_collision_probe)
  add_executable(collision_probe_smoke tests/smoke/collision_probe_smoke.cpp)
  target_link_libraries(collision_probe_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(collision_probe_smoke)
  target_compile_definitions(collision_probe_smoke
    PRIVATE
      IGGY3D_COLLISION_PROBE_PATH="$<TARGET_FILE:iggy3d_collision_probe>")
  add_test(NAME collision_probe_smoke COMMAND "$<TARGET_FILE:collision_probe_smoke>")
  set_tests_properties(collision_probe_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    LABELS "smoke;runtime;collision;package;iggy3d")
endif()

if(IGGY3D_ENABLE_VULKAN_SMOKE)
  function(iggy3d_add_vulkan_backend_smoke_sources target_name)
    if(NOT IGGY3D_BUILD_VULKAN_BACKEND)
      target_sources("${target_name}" PRIVATE
        src/render/vulkan/VulkanResult.cpp
        src/render/vulkan/VulkanFunctions.cpp
        src/render/vulkan/VulkanFeatureSupport.cpp
        src/render/vulkan/DebugValidation.cpp
        src/render/vulkan/InstanceDeviceSurface.cpp
        src/render/vulkan/Swapchain.cpp
        src/render/vulkan/FrameSync.cpp
        src/render/vulkan/CommandRecording.cpp
        src/render/vulkan/RenderLoop.cpp
        src/render/vulkan/ShaderModule.cpp
        src/render/vulkan/PipelineLayout.cpp
        src/render/vulkan/FirstRoomPipeline.cpp
        src/render/vulkan/VulkanMemoryAllocator.cpp
        src/render/vulkan/BufferImageResources.cpp
        src/render/vulkan/DescriptorSets.cpp
        src/render/vulkan/FrameCapture.cpp
        src/render/vulkan/VulkanBackend.cpp)
    endif()
  endfunction()

  function(iggy3d_add_vulkan_packet6_smoke_sources target_name)
    if(NOT IGGY3D_BUILD_VULKAN_BACKEND)
      target_sources("${target_name}" PRIVATE
        src/render/vulkan/VulkanResult.cpp
        src/render/vulkan/VulkanFunctions.cpp
        src/render/vulkan/VulkanFeatureSupport.cpp
        src/render/vulkan/DebugValidation.cpp
        src/render/vulkan/InstanceDeviceSurface.cpp
        src/render/vulkan/ShaderModule.cpp
        src/render/vulkan/PipelineLayout.cpp
        src/render/vulkan/FirstRoomPipeline.cpp
        src/render/vulkan/VulkanMemoryAllocator.cpp
        src/render/vulkan/BufferImageResources.cpp
        src/render/vulkan/DescriptorSets.cpp)
    endif()
  endfunction()

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
    SKIP_RETURN_CODE 77
    LABELS "smoke;vulkan;render;iggy3d")

  add_executable(vulkan_device_smoke tests/smoke/vulkan_device_smoke.cpp)
  iggy3d_add_vulkan_backend_smoke_sources(vulkan_device_smoke)
  target_link_libraries(vulkan_device_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_device_smoke)

  add_executable(vulkan_feature_baseline_smoke tests/smoke/vulkan_feature_baseline_smoke.cpp)
  if(NOT IGGY3D_BUILD_VULKAN_BACKEND)
    target_sources(vulkan_feature_baseline_smoke PRIVATE
      src/render/vulkan/VulkanFeatureSupport.cpp)
  endif()
  target_link_libraries(vulkan_feature_baseline_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_feature_baseline_smoke)

  add_executable(vulkan_validation_smoke tests/smoke/vulkan_validation_smoke.cpp)
  if(NOT IGGY3D_BUILD_VULKAN_BACKEND)
    target_sources(vulkan_validation_smoke PRIVATE
      src/render/vulkan/DebugValidation.cpp)
  endif()
  target_link_libraries(vulkan_validation_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_validation_smoke)
  if(IGGY3D_REQUIRE_VULKAN_SMOKE)
    target_compile_definitions(vulkan_device_smoke PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
    target_compile_definitions(vulkan_feature_baseline_smoke PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
    target_compile_definitions(vulkan_validation_smoke PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
  endif()
  if(IGGY3D_REQUIRE_VALIDATION_LAYERS)
    target_compile_definitions(vulkan_validation_smoke PRIVATE IGGY3D_REQUIRE_VALIDATION_LAYERS_ENABLED=1)
  endif()
  add_test(NAME vulkan_device_smoke COMMAND "$<TARGET_FILE:vulkan_device_smoke>")
  add_test(NAME vulkan_feature_baseline_smoke COMMAND "$<TARGET_FILE:vulkan_feature_baseline_smoke>")
  add_test(NAME vulkan_validation_smoke COMMAND "$<TARGET_FILE:vulkan_validation_smoke>")
  set_tests_properties(vulkan_device_smoke vulkan_feature_baseline_smoke vulkan_validation_smoke
    PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      SKIP_RETURN_CODE 77
      LABELS "smoke;vulkan;render;iggy3d")

  add_executable(vulkan_swapchain_smoke tests/smoke/vulkan_swapchain_smoke.cpp)
  iggy3d_add_vulkan_backend_smoke_sources(vulkan_swapchain_smoke)
  target_link_libraries(vulkan_swapchain_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_swapchain_smoke)

  add_executable(vulkan_resize_minimize_smoke tests/smoke/vulkan_resize_minimize_smoke.cpp)
  iggy3d_add_vulkan_backend_smoke_sources(vulkan_resize_minimize_smoke)
  target_link_libraries(vulkan_resize_minimize_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_resize_minimize_smoke)

  add_executable(vulkan_empty_frame_smoke tests/smoke/vulkan_empty_frame_smoke.cpp)
  iggy3d_add_vulkan_backend_smoke_sources(vulkan_empty_frame_smoke)
  target_link_libraries(vulkan_empty_frame_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_empty_frame_smoke)

  add_executable(vulkan_sync_smoke tests/smoke/vulkan_sync_smoke.cpp)
  iggy3d_add_vulkan_backend_smoke_sources(vulkan_sync_smoke)
  target_link_libraries(vulkan_sync_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_sync_smoke)

  if(IGGY3D_REQUIRE_VULKAN_SMOKE)
    target_compile_definitions(vulkan_swapchain_smoke PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
    target_compile_definitions(vulkan_resize_minimize_smoke PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
    target_compile_definitions(vulkan_empty_frame_smoke PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
    target_compile_definitions(vulkan_sync_smoke PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
  endif()
  add_test(NAME vulkan_swapchain_smoke COMMAND "$<TARGET_FILE:vulkan_swapchain_smoke>")
  add_test(NAME vulkan_resize_minimize_smoke COMMAND "$<TARGET_FILE:vulkan_resize_minimize_smoke>")
  add_test(NAME vulkan_empty_frame_smoke COMMAND "$<TARGET_FILE:vulkan_empty_frame_smoke>")
  add_test(NAME vulkan_sync_smoke COMMAND "$<TARGET_FILE:vulkan_sync_smoke>")
  set_tests_properties(vulkan_swapchain_smoke vulkan_resize_minimize_smoke
    vulkan_empty_frame_smoke vulkan_sync_smoke
    PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      SKIP_RETURN_CODE 77
      LABELS "smoke;vulkan;render;iggy3d")

  add_executable(vulkan_pipeline_smoke tests/smoke/vulkan_pipeline_smoke.cpp)
  iggy3d_add_vulkan_packet6_smoke_sources(vulkan_pipeline_smoke)
  target_link_libraries(vulkan_pipeline_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_pipeline_smoke)

  add_executable(vulkan_memory_smoke tests/smoke/vulkan_memory_smoke.cpp)
  iggy3d_add_vulkan_packet6_smoke_sources(vulkan_memory_smoke)
  target_link_libraries(vulkan_memory_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_memory_smoke)

  add_executable(vulkan_descriptor_smoke tests/smoke/vulkan_descriptor_smoke.cpp)
  iggy3d_add_vulkan_packet6_smoke_sources(vulkan_descriptor_smoke)
  target_link_libraries(vulkan_descriptor_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_descriptor_smoke)

  add_executable(vulkan_material_smoke tests/smoke/vulkan_material_smoke.cpp)
  iggy3d_add_vulkan_packet6_smoke_sources(vulkan_material_smoke)
  target_link_libraries(vulkan_material_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_material_smoke)

  foreach(packet6_smoke vulkan_pipeline_smoke vulkan_memory_smoke
                         vulkan_descriptor_smoke vulkan_material_smoke)
    target_compile_definitions("${packet6_smoke}"
      PRIVATE
        IGGY3D_SHADER_SOURCE_ROOT_VALUE="${IGGY3D_SHADER_SOURCE_ROOT}"
        IGGY3D_SHADER_BINARY_ROOT_VALUE="${IGGY3D_SHADER_BINARY_ROOT}"
        IGGY3D_SHADER_TARGET_ENV_VALUE="${IGGY3D_SHADER_TARGET_ENV}")
    if(IGGY3D_SHADER_COMPILER_AVAILABLE)
      target_compile_definitions("${packet6_smoke}" PRIVATE IGGY3D_SHADER_COMPILER_AVAILABLE=1)
    endif()
    if(IGGY3D_REQUIRE_VULKAN_SMOKE)
      target_compile_definitions("${packet6_smoke}" PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
    endif()
    if(IGGY3D_ENABLE_VULKAN_SHADERS AND IGGY3D_SHADER_COMPILER_AVAILABLE)
      add_dependencies("${packet6_smoke}" iggy3d_vulkan_shaders)
    endif()
  endforeach()

  add_test(NAME vulkan_pipeline_smoke COMMAND "$<TARGET_FILE:vulkan_pipeline_smoke>")
  add_test(NAME vulkan_memory_smoke COMMAND "$<TARGET_FILE:vulkan_memory_smoke>")
  add_test(NAME vulkan_descriptor_smoke COMMAND "$<TARGET_FILE:vulkan_descriptor_smoke>")
  add_test(NAME vulkan_material_smoke COMMAND "$<TARGET_FILE:vulkan_material_smoke>")
  set_tests_properties(vulkan_pipeline_smoke vulkan_memory_smoke
    vulkan_descriptor_smoke vulkan_material_smoke
    PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      SKIP_RETURN_CODE 77
      LABELS "smoke;vulkan;render;packet6;iggy3d")

  add_executable(vulkan_first_room_smoke tests/smoke/vulkan_first_room_smoke.cpp)
  iggy3d_add_vulkan_backend_smoke_sources(vulkan_first_room_smoke)
  target_link_libraries(vulkan_first_room_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_first_room_smoke)

  add_executable(vulkan_screenshot_smoke tests/smoke/vulkan_screenshot_smoke.cpp)
  iggy3d_add_vulkan_backend_smoke_sources(vulkan_screenshot_smoke)
  target_link_libraries(vulkan_screenshot_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_screenshot_smoke)

  add_executable(vulkan_frame_hash_smoke tests/smoke/vulkan_frame_hash_smoke.cpp)
  iggy3d_add_vulkan_backend_smoke_sources(vulkan_frame_hash_smoke)
  target_link_libraries(vulkan_frame_hash_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_frame_hash_smoke)

  add_executable(vulkan_diagnostics_smoke tests/smoke/vulkan_diagnostics_smoke.cpp)
    if(NOT IGGY3D_BUILD_VULKAN_BACKEND)
    target_sources(vulkan_diagnostics_smoke PRIVATE
      src/render/vulkan/VulkanMemoryAllocator.cpp
      src/render/vulkan/FrameCapture.cpp)
  endif()
  target_link_libraries(vulkan_diagnostics_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_diagnostics_smoke)

  add_executable(vulkan_device_lost_smoke tests/smoke/vulkan_device_lost_smoke.cpp)
  if(NOT IGGY3D_BUILD_VULKAN_BACKEND)
    target_sources(vulkan_device_lost_smoke PRIVATE
      src/render/vulkan/VulkanResult.cpp)
  endif()
  target_link_libraries(vulkan_device_lost_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_device_lost_smoke)

  add_executable(vulkan_optional_unsupported_smoke
    tests/smoke/vulkan_optional_unsupported_smoke.cpp)
  target_link_libraries(vulkan_optional_unsupported_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_optional_unsupported_smoke)

  add_executable(vulkan_strict_unsupported_smoke
    tests/smoke/vulkan_strict_unsupported_smoke.cpp)
  target_link_libraries(vulkan_strict_unsupported_smoke PRIVATE iggy3d)
  iggy3d_apply_warnings(vulkan_strict_unsupported_smoke)

  add_executable(package_shader_lookup_smoke tests/smoke/package_shader_lookup_smoke.cpp)
  target_link_libraries(package_shader_lookup_smoke PRIVATE iggy3d)
  target_compile_definitions(package_shader_lookup_smoke
    PRIVATE
      IGGY3D_SHADER_BINARY_ROOT_VALUE="${IGGY3D_SHADER_BINARY_ROOT}")
  iggy3d_apply_warnings(package_shader_lookup_smoke)

  add_executable(package_vulkan_dependency_smoke
    tests/smoke/package_vulkan_dependency_smoke.cpp)
  target_link_libraries(package_vulkan_dependency_smoke PRIVATE iggy3d)
  target_compile_definitions(package_vulkan_dependency_smoke
    PRIVATE
      IGGY3D_SHADER_BINARY_ROOT_VALUE="${IGGY3D_SHADER_BINARY_ROOT}")
  iggy3d_apply_warnings(package_vulkan_dependency_smoke)

  add_executable(macos_vulkan_dependency_probe
    tests/smoke/macos_vulkan_dependency_probe.cpp)
  target_link_libraries(macos_vulkan_dependency_probe PRIVATE iggy3d)
  target_compile_definitions(macos_vulkan_dependency_probe
    PRIVATE
      IGGY3D_SDL3_SOURCE_VALUE="${IGGY3D_SDL3_SOURCE}"
      IGGY3D_VULKAN_LOADER_VALUE="${IGGY3D_VULKAN_LOADER}"
      IGGY3D_VULKAN_SDK_ROOT_VALUE="${IGGY3D_VULKAN_SDK_ROOT}"
      IGGY3D_VULKAN_SDK_SOURCE_VALUE="${IGGY3D_VULKAN_SDK_SOURCE}"
      IGGY3D_VULKAN_ICD_PATH_VALUE="${IGGY3D_VULKAN_ICD_PATH}"
      IGGY3D_GLSLC_PATH_VALUE="${IGGY3D_GLSLC_EXE}")
  if(IGGY3D_REQUIRE_VULKAN_SMOKE)
    target_compile_definitions(macos_vulkan_dependency_probe
      PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
  endif()
  iggy3d_apply_warnings(macos_vulkan_dependency_probe)

  foreach(packet7_smoke vulkan_first_room_smoke vulkan_screenshot_smoke
                         vulkan_frame_hash_smoke package_shader_lookup_smoke
                         package_vulkan_dependency_smoke)
    target_compile_definitions("${packet7_smoke}"
      PRIVATE
        IGGY3D_SHADER_SOURCE_ROOT_VALUE="${IGGY3D_SHADER_SOURCE_ROOT}"
        IGGY3D_SHADER_BINARY_ROOT_VALUE="${IGGY3D_SHADER_BINARY_ROOT}"
        IGGY3D_SHADER_TARGET_ENV_VALUE="${IGGY3D_SHADER_TARGET_ENV}")
    if(IGGY3D_SHADER_COMPILER_AVAILABLE)
      target_compile_definitions("${packet7_smoke}" PRIVATE IGGY3D_SHADER_COMPILER_AVAILABLE=1)
    endif()
    if(IGGY3D_ENABLE_VULKAN_SHADERS AND IGGY3D_SHADER_COMPILER_AVAILABLE)
      add_dependencies("${packet7_smoke}" iggy3d_vulkan_shaders)
    endif()
  endforeach()

  foreach(packet7_strict_smoke vulkan_first_room_smoke vulkan_screenshot_smoke
                                vulkan_frame_hash_smoke)
    if(IGGY3D_REQUIRE_VULKAN_SMOKE)
      target_compile_definitions("${packet7_strict_smoke}"
        PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
    endif()
  endforeach()

  add_test(NAME vulkan_first_room_smoke COMMAND "$<TARGET_FILE:vulkan_first_room_smoke>")
  add_test(NAME vulkan_screenshot_smoke COMMAND "$<TARGET_FILE:vulkan_screenshot_smoke>")
  add_test(NAME vulkan_frame_hash_smoke COMMAND "$<TARGET_FILE:vulkan_frame_hash_smoke>")
  add_test(NAME vulkan_diagnostics_smoke COMMAND "$<TARGET_FILE:vulkan_diagnostics_smoke>")
  add_test(NAME vulkan_device_lost_smoke COMMAND "$<TARGET_FILE:vulkan_device_lost_smoke>")
  add_test(NAME vulkan_optional_unsupported_smoke
           COMMAND "$<TARGET_FILE:vulkan_optional_unsupported_smoke>")
  add_test(NAME vulkan_strict_unsupported_smoke
           COMMAND "$<TARGET_FILE:vulkan_strict_unsupported_smoke>")
  add_test(NAME package_shader_lookup_smoke
           COMMAND "$<TARGET_FILE:package_shader_lookup_smoke>")
  add_test(NAME package_vulkan_dependency_smoke
           COMMAND "$<TARGET_FILE:package_vulkan_dependency_smoke>")
  add_test(NAME macos_vulkan_dependency_probe
           COMMAND "$<TARGET_FILE:macos_vulkan_dependency_probe>")

  set_tests_properties(vulkan_first_room_smoke vulkan_screenshot_smoke
    vulkan_frame_hash_smoke vulkan_optional_unsupported_smoke package_shader_lookup_smoke
    package_vulkan_dependency_smoke macos_vulkan_dependency_probe
    PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      SKIP_RETURN_CODE 77
      LABELS "smoke;vulkan;render;packet7;iggy3d")
  set_tests_properties(vulkan_diagnostics_smoke vulkan_device_lost_smoke
    PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      LABELS "smoke;vulkan;render;packet7;iggy3d")
  set_tests_properties(vulkan_strict_unsupported_smoke
    PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      WILL_FAIL TRUE
      LABELS "smoke;vulkan;render;packet7;iggy3d")
endif()

iggy3d_add_acceptance_test(complete_runtime_demo_tests tests/acceptance/complete_runtime_demo_tests.cpp)
if(TARGET iggy3d_headless_demo)
  target_compile_definitions(complete_runtime_demo_tests
    PRIVATE
      IGGY3D_HEADLESS_DEMO_PATH="$<TARGET_FILE:iggy3d_headless_demo>")
endif()

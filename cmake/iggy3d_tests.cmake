# iggy3d test spine — creative-only tree.
# Resurrected from the full-tree suite at the quarantine parent commit: every
# test whose include-closure lives entirely in the surviving library. The
# product-app, ascii, package, and tool suites stay quarantined with their code.

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

iggy3d_add_unit_test(math_tests tests/unit/math_tests.cpp)
set_tests_properties(math_tests PROPERTIES LABELS "unit;core;iggy3d")

iggy3d_add_unit_test(static_mesh_asset_tests
  tests/unit/static_mesh_asset_tests.cpp)
set_tests_properties(static_mesh_asset_tests PROPERTIES
  LABELS "unit;content;asset;glb;render;creative;iggy3d")

iggy3d_add_unit_test(creative_asset_room_bake_tests
  tests/unit/creative_asset_room_bake_tests.cpp)
set_tests_properties(creative_asset_room_bake_tests PROPERTIES
  LABELS "unit;content;asset;room_bake;physics;creative;iggy3d")

iggy3d_add_unit_test(render_projectile_overlay_projection_tests
  tests/unit/render_projectile_overlay_projection_tests.cpp)

iggy3d_add_unit_test(product_creative_wireframe_debug_line_tests
  tests/unit/product_creative_wireframe_debug_line_tests.cpp)
set_tests_properties(product_creative_wireframe_debug_line_tests PROPERTIES
  LABELS "unit;app;product;creative;wireframe;debug_line;iggy3d")

iggy3d_add_unit_test(product_map_maker_grid_tests
  tests/unit/product_map_maker_grid_tests.cpp)
set_tests_properties(product_map_maker_grid_tests PROPERTIES
  LABELS "unit;app;product;map_maker;grid;iggy3d")

iggy3d_add_unit_test(creative_core_tests
  tests/unit/creative_core_tests.cpp)
set_tests_properties(creative_core_tests PROPERTIES
  LABELS "unit;app;creative;core;iggy3d")

iggy3d_add_unit_test(creative_geometry_tests
  tests/unit/creative_geometry_tests.cpp)
set_tests_properties(creative_geometry_tests PROPERTIES
  LABELS "unit;app;creative;geometry;iggy3d")

iggy3d_add_unit_test(creative_room_tests
  tests/unit/creative_room_tests.cpp)
set_tests_properties(creative_room_tests PROPERTIES
  LABELS "unit;app;creative;room;iggy3d")

iggy3d_add_unit_test(creative_object_descriptor_tests
  tests/unit/creative_object_descriptor_tests.cpp)
set_tests_properties(creative_object_descriptor_tests PROPERTIES
  LABELS "unit;app;creative;object_descriptor;iggy3d")

iggy3d_add_unit_test(creative_placement_compatibility_tests
  tests/unit/creative_placement_compatibility_tests.cpp)
set_tests_properties(creative_placement_compatibility_tests PROPERTIES
  LABELS "unit;app;creative;placement;compatibility;iggy3d")

iggy3d_add_unit_test(creative_document_mutation_tests
  tests/unit/creative_document_mutation_tests.cpp)
set_tests_properties(creative_document_mutation_tests PROPERTIES
  LABELS "unit;app;creative;document;mutation;iggy3d")

iggy3d_add_unit_test(creative_document_create_tests
  tests/unit/creative_document_create_tests.cpp)
set_tests_properties(creative_document_create_tests PROPERTIES
  LABELS "unit;app;creative;document;create;iggy3d")

iggy3d_add_unit_test(creative_document_remove_tests
  tests/unit/creative_document_remove_tests.cpp)
set_tests_properties(creative_document_remove_tests PROPERTIES
  LABELS "unit;app;creative;document;remove;iggy3d")

iggy3d_add_unit_test(creative_logic_link_tests
  tests/unit/creative_logic_link_tests.cpp)
set_tests_properties(creative_logic_link_tests PROPERTIES
  LABELS "unit;app;creative;document;logic;clipboard;iggy3d")

iggy3d_add_unit_test(creative_document_dirty_tests
  tests/unit/creative_document_dirty_tests.cpp)
set_tests_properties(creative_document_dirty_tests PROPERTIES
  LABELS "unit;app;creative;document;dirty;iggy3d")

iggy3d_add_unit_test(creative_document_identity_tests
  tests/unit/creative_document_identity_tests.cpp)
set_tests_properties(creative_document_identity_tests PROPERTIES
  LABELS "unit;app;creative;document;identity;iggy3d")

iggy3d_add_unit_test(creative_document_persistence_state_tests
  tests/unit/creative_document_persistence_state_tests.cpp)
set_tests_properties(creative_document_persistence_state_tests PROPERTIES
  LABELS "unit;app;creative;document;persistence;iggy3d")

iggy3d_add_unit_test(creative_document_path_tests
  tests/unit/creative_document_path_tests.cpp)
set_tests_properties(creative_document_path_tests PROPERTIES
  LABELS "unit;app;creative;document;path;iggy3d")

iggy3d_add_unit_test(creative_recipe_tests
  tests/unit/creative_recipe_tests.cpp)
set_tests_properties(creative_recipe_tests PROPERTIES
  LABELS "unit;app;creative;recipe;document;history;iggy3d")

iggy3d_add_unit_test(creative_building_recipe_tests
  tests/unit/creative_building_recipe_tests.cpp)
set_tests_properties(creative_building_recipe_tests PROPERTIES
  LABELS "unit;app;creative;recipe;building;iggy3d")

iggy3d_add_unit_test(creative_structural_surface_recipe_tests
  tests/unit/creative_structural_surface_recipe_tests.cpp)
set_tests_properties(creative_structural_surface_recipe_tests PROPERTIES
  LABELS "unit;app;creative;recipe;structural;surface;iggy3d")

iggy3d_add_unit_test(creative_structural_roof_recipe_tests
  tests/unit/creative_structural_roof_recipe_tests.cpp)
set_tests_properties(creative_structural_roof_recipe_tests PROPERTIES
  LABELS "unit;app;creative;recipe;structural;roof;iggy3d")

iggy3d_add_unit_test(creative_structural_wall_recipe_tests
  tests/unit/creative_structural_wall_recipe_tests.cpp)
set_tests_properties(creative_structural_wall_recipe_tests PROPERTIES
  LABELS "unit;app;creative;recipe;structural;wall;opening;iggy3d")

iggy3d_add_unit_test(creative_terrain_recipe_tests
  tests/unit/creative_terrain_recipe_tests.cpp)
set_tests_properties(creative_terrain_recipe_tests PROPERTIES
  LABELS "unit;app;creative;recipe;terrain;history;iggy3d")

iggy3d_add_unit_test(creative_terrain_grounding_tests
  tests/unit/creative_terrain_grounding_tests.cpp)
set_tests_properties(creative_terrain_grounding_tests PROPERTIES
  LABELS "unit;app;creative;recipe;terrain;grounding;iggy3d")

iggy3d_add_unit_test(creative_world_layout_tests
  tests/unit/creative_world_layout_tests.cpp)
set_tests_properties(creative_world_layout_tests PROPERTIES
  LABELS "unit;app;creative;world;layout;recipe;history;iggy3d")

iggy3d_add_unit_test(creative_world_layout_terrain_impact_tests
  tests/unit/creative_world_layout_terrain_impact_tests.cpp)
set_tests_properties(creative_world_layout_terrain_impact_tests PROPERTIES
  LABELS "unit;app;creative;world;layout;terrain;impact;iggy3d")

iggy3d_add_unit_test(creative_world_layout_terrain_reconciliation_tests
  tests/unit/creative_world_layout_terrain_reconciliation_tests.cpp)
set_tests_properties(creative_world_layout_terrain_reconciliation_tests PROPERTIES
  LABELS "unit;app;creative;world;layout;terrain;reconciliation;iggy3d")

iggy3d_add_unit_test(creative_world_layout_room_tests
  tests/unit/creative_world_layout_room_tests.cpp)
set_tests_properties(creative_world_layout_room_tests PROPERTIES
  LABELS "unit;app;creative;world;layout;room;topology;iggy3d")

iggy3d_add_unit_test(creative_world_layout_vertical_connector_tests
  tests/unit/creative_world_layout_vertical_connector_tests.cpp)
set_tests_properties(creative_world_layout_vertical_connector_tests PROPERTIES
  LABELS "unit;app;creative;world;layout;vertical;stair;iggy3d")

iggy3d_add_unit_test(creative_world_layout_codec_tests
  tests/unit/creative_world_layout_codec_tests.cpp)
set_tests_properties(creative_world_layout_codec_tests PROPERTIES
  LABELS "unit;app;creative;world;layout;codec;iggy3d")

iggy3d_add_unit_test(creative_world_layout_persistence_tests
  tests/unit/creative_world_layout_persistence_tests.cpp)
set_tests_properties(creative_world_layout_persistence_tests PROPERTIES
  LABELS "unit;app;creative;world;layout;save;iggy3d")

iggy3d_add_unit_test(creative_document_save_section_tests
  tests/unit/creative_document_save_section_tests.cpp)
set_tests_properties(creative_document_save_section_tests PROPERTIES
  LABELS "unit;app;creative;document;save;iggy3d")

iggy3d_add_unit_test(creative_facade_tests
  tests/unit/creative_facade_tests.cpp)
set_tests_properties(creative_facade_tests PROPERTIES
  LABELS "unit;app;creative;facade;iggy3d")

iggy3d_add_unit_test(creative_facade_mutation_tests
  tests/unit/creative_facade_mutation_tests.cpp)
set_tests_properties(creative_facade_mutation_tests PROPERTIES
  LABELS "unit;app;creative;facade;mutation;iggy3d")

iggy3d_add_unit_test(creative_spatial_projection_tests
  tests/unit/creative_spatial_projection_tests.cpp)
set_tests_properties(creative_spatial_projection_tests PROPERTIES
  LABELS "unit;app;creative;spatial_projection;iggy3d")

iggy3d_add_unit_test(creative_document_wireframe_tests
  tests/unit/creative_document_wireframe_tests.cpp)
set_tests_properties(creative_document_wireframe_tests PROPERTIES
  LABELS "unit;app;creative;document;wireframe;iggy3d")

iggy3d_add_unit_test(creative_tools_tests
  tests/unit/creative_tools_tests.cpp)
set_tests_properties(creative_tools_tests PROPERTIES
  LABELS "unit;app;creative;tools;iggy3d")

iggy3d_add_unit_test(creative_group_tests
  tests/unit/creative_group_tests.cpp)
set_tests_properties(creative_group_tests PROPERTIES
  LABELS "unit;app;creative;tools;group;hierarchy;iggy3d")

iggy3d_add_unit_test(creative_terrain_profile_tests
  tests/unit/creative_terrain_profile_tests.cpp)
set_tests_properties(creative_terrain_profile_tests PROPERTIES
  LABELS "unit;app;creative;tools;terrain;profile;iggy3d")

iggy3d_add_unit_test(creative_terrain_path_tests
  tests/unit/creative_terrain_path_tests.cpp)
set_tests_properties(creative_terrain_path_tests PROPERTIES
  LABELS "unit;app;creative;tools;terrain;path;iggy3d")

iggy3d_add_unit_test(creative_terrain_region_tests
  tests/unit/creative_terrain_region_tests.cpp)
set_tests_properties(creative_terrain_region_tests PROPERTIES
  LABELS "unit;app;creative;tools;terrain;region;iggy3d")

iggy3d_add_unit_test(creative_terrain_stamp_tests
  tests/unit/creative_terrain_stamp_tests.cpp)
set_tests_properties(creative_terrain_stamp_tests PROPERTIES
  LABELS "unit;app;creative;tools;terrain;clipboard;iggy3d")

iggy3d_add_unit_test(creative_pattern_tests
  tests/unit/creative_pattern_tests.cpp)
set_tests_properties(creative_pattern_tests PROPERTIES
  LABELS "unit;app;creative;tools;pattern;iggy3d")

iggy3d_add_unit_test(creative_shape_brush_tests
  tests/unit/creative_shape_brush_tests.cpp)
set_tests_properties(creative_shape_brush_tests PROPERTIES
  LABELS "unit;app;creative;tools;shape_brush;iggy3d")

iggy3d_add_unit_test(creative_connected_fill_tests
  tests/unit/creative_connected_fill_tests.cpp)
set_tests_properties(creative_connected_fill_tests PROPERTIES
  LABELS "unit;app;creative;tools;connected_fill;iggy3d")

iggy3d_add_unit_test(creative_surface_extrude_tests
  tests/unit/creative_surface_extrude_tests.cpp)
set_tests_properties(creative_surface_extrude_tests PROPERTIES
  LABELS "unit;app;creative;tools;surface_extrude;iggy3d")

iggy3d_add_unit_test(creative_editor_pattern_tests
  tests/unit/creative_editor_pattern_tests.cpp)
target_sources(creative_editor_pattern_tests PRIVATE
  apps/iggy3d_creative/EditorPattern.cpp
  apps/iggy3d_creative/EditorEdits.cpp
  apps/iggy3d_creative/EditorTransform.cpp
  apps/iggy3d_creative/EditorTransformOverlay.cpp
  apps/iggy3d_creative/EditorPreviewProxies.cpp)
target_include_directories(creative_editor_pattern_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/apps/iggy3d_creative")
set_tests_properties(creative_editor_pattern_tests PROPERTIES
  LABELS "unit;app;creative;editor;tools;pattern;iggy3d")

iggy3d_add_unit_test(creative_interaction_tests
  tests/unit/creative_interaction_tests.cpp)
target_include_directories(creative_interaction_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/apps/iggy3d_creative")
set_tests_properties(creative_interaction_tests PROPERTIES
  LABELS "unit;app;creative;input;interaction;iggy3d")

iggy3d_add_unit_test(creative_ui_input_tests
  tests/unit/creative_ui_input_tests.cpp)
set_tests_properties(creative_ui_input_tests PROPERTIES
  LABELS "unit;app;creative;input;ui;iggy3d")

iggy3d_add_unit_test(creative_ui_widgets_tests
  tests/unit/creative_ui_widgets_tests.cpp)
set_tests_properties(creative_ui_widgets_tests PROPERTIES
  LABELS "unit;app;creative;ui;widgets;iggy3d")

iggy3d_add_unit_test(creative_control_profile_tests
  tests/unit/creative_control_profile_tests.cpp)
set_tests_properties(creative_control_profile_tests PROPERTIES
  LABELS "unit;app;creative;input;controls;profile;iggy3d")

iggy3d_add_unit_test(creative_screen_projection_tests
  tests/unit/creative_screen_projection_tests.cpp)
set_tests_properties(creative_screen_projection_tests PROPERTIES
  LABELS "unit;app;creative;render;screen_projection;iggy3d")

add_executable(creative_editor_placement_tests
  tests/unit/creative_editor_placement_tests.cpp)
target_link_libraries(creative_editor_placement_tests PRIVATE iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_placement_tests)
add_test(NAME creative_editor_placement_tests
  COMMAND "$<TARGET_FILE:creative_editor_placement_tests>")
set_tests_properties(creative_editor_placement_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties(creative_editor_placement_tests PROPERTIES
  LABELS "unit;app;creative;editor;placement;preview;history;iggy3d")

add_executable(creative_editor_room_placement_tests
  tests/unit/creative_editor_room_placement_tests.cpp)
target_link_libraries(creative_editor_room_placement_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_room_placement_tests)
add_test(NAME creative_editor_room_placement_tests
  COMMAND "$<TARGET_FILE:creative_editor_room_placement_tests>")
set_tests_properties(creative_editor_room_placement_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;room;recipe;preview;history;iggy3d")

add_executable(creative_editor_placement_clearance_tests
  tests/unit/creative_editor_placement_clearance_tests.cpp)
target_link_libraries(creative_editor_placement_clearance_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_placement_clearance_tests)
add_test(NAME creative_editor_placement_clearance_tests
  COMMAND "$<TARGET_FILE:creative_editor_placement_clearance_tests>")
set_tests_properties(creative_editor_placement_clearance_tests PROPERTIES
  LABELS "unit;app;creative;editor;placement;clearance;iggy3d")

add_executable(creative_editor_moving_platform_preview_tests
  tests/unit/creative_editor_moving_platform_preview_tests.cpp)
target_link_libraries(creative_editor_moving_platform_preview_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_moving_platform_preview_tests)
add_test(NAME creative_editor_moving_platform_preview_tests
  COMMAND "$<TARGET_FILE:creative_editor_moving_platform_preview_tests>")
set_tests_properties(creative_editor_moving_platform_preview_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;moving_platform;preview;iggy3d")

iggy3d_add_unit_test(creative_attachment_snap_tests
  tests/unit/creative_attachment_snap_tests.cpp)
set_tests_properties(creative_attachment_snap_tests PROPERTIES
  LABELS "unit;app;creative;placement;attachment;iggy3d")

add_executable(creative_editor_attachment_tests
  tests/unit/creative_editor_attachment_tests.cpp)
target_link_libraries(creative_editor_attachment_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_attachment_tests)
add_test(NAME creative_editor_attachment_tests
  COMMAND "$<TARGET_FILE:creative_editor_attachment_tests>")
set_tests_properties(creative_editor_attachment_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;attachment;history;iggy3d")

add_executable(creative_asset_scatter_tests
  tests/unit/creative_asset_scatter_tests.cpp)
target_link_libraries(creative_asset_scatter_tests PRIVATE
  iggy3d_creative_app)
target_include_directories(creative_asset_scatter_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/apps/iggy3d_creative")
iggy3d_apply_warnings(creative_asset_scatter_tests)
add_test(NAME creative_asset_scatter_tests
  COMMAND "$<TARGET_FILE:creative_asset_scatter_tests>")
set_tests_properties(creative_asset_scatter_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties(creative_asset_scatter_tests PROPERTIES
  LABELS "unit;app;creative;editor;asset;scatter;placement;history;iggy3d")

add_executable(creative_editor_controls_tests
  tests/unit/creative_editor_controls_tests.cpp)
target_link_libraries(creative_editor_controls_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_controls_tests)
add_test(NAME creative_editor_controls_tests
  COMMAND "$<TARGET_FILE:creative_editor_controls_tests>")
set_tests_properties(creative_editor_controls_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties(creative_editor_controls_tests PROPERTIES
  LABELS "unit;app;creative;editor;controls;persistence;iggy3d")

add_executable(creative_editor_action_hints_tests
  tests/unit/creative_editor_action_hints_tests.cpp)
target_link_libraries(creative_editor_action_hints_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_action_hints_tests)
add_test(NAME creative_editor_action_hints_tests
  COMMAND "$<TARGET_FILE:creative_editor_action_hints_tests>")
set_tests_properties(creative_editor_action_hints_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties(creative_editor_action_hints_tests PROPERTIES
  LABELS "unit;app;creative;editor;controls;hints;ui;iggy3d")

add_executable(creative_editor_group_tests
  tests/unit/creative_editor_group_tests.cpp)
target_link_libraries(creative_editor_group_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_group_tests)
add_test(NAME creative_editor_group_tests
  COMMAND "$<TARGET_FILE:creative_editor_group_tests>")
set_tests_properties(creative_editor_group_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;group;history;iggy3d")

add_executable(creative_editor_logic_link_tests
  tests/unit/creative_editor_logic_link_tests.cpp)
target_link_libraries(creative_editor_logic_link_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_logic_link_tests)
add_test(NAME creative_editor_logic_link_tests
  COMMAND "$<TARGET_FILE:creative_editor_logic_link_tests>")
set_tests_properties(creative_editor_logic_link_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;logic;history;iggy3d")

add_executable(creative_editor_terrain_tests
  tests/unit/creative_editor_terrain_tests.cpp)
target_link_libraries(creative_editor_terrain_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_terrain_tests)
add_test(NAME creative_editor_terrain_tests
  COMMAND "$<TARGET_FILE:creative_editor_terrain_tests>")
set_tests_properties(creative_editor_terrain_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;terrain;history;preview;iggy3d")

add_executable(creative_editor_terrain_generation_preview_tests
  tests/unit/creative_editor_terrain_generation_preview_tests.cpp)
target_link_libraries(creative_editor_terrain_generation_preview_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_terrain_generation_preview_tests)
add_test(NAME creative_editor_terrain_generation_preview_tests
  COMMAND "$<TARGET_FILE:creative_editor_terrain_generation_preview_tests>")
set_tests_properties(creative_editor_terrain_generation_preview_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;terrain;generation;preview;iggy3d")

add_executable(creative_editor_terrain_contour_tests
  tests/unit/creative_editor_terrain_contour_tests.cpp)
target_link_libraries(creative_editor_terrain_contour_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_terrain_contour_tests)
add_test(NAME creative_editor_terrain_contour_tests
  COMMAND "$<TARGET_FILE:creative_editor_terrain_contour_tests>")
set_tests_properties(creative_editor_terrain_contour_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;terrain;contour;preview;iggy3d")

iggy3d_add_unit_test(creative_catalog_tests
  tests/unit/creative_catalog_tests.cpp)
set_tests_properties(creative_catalog_tests PROPERTIES
  LABELS "unit;app;creative;input;catalog;iggy3d")

add_executable(creative_editor_asset_reload_tests
  tests/unit/creative_editor_asset_reload_tests.cpp)
target_link_libraries(creative_editor_asset_reload_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_asset_reload_tests)
add_test(NAME creative_editor_asset_reload_tests
  COMMAND "$<TARGET_FILE:creative_editor_asset_reload_tests>")
set_tests_properties(creative_editor_asset_reload_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;assets;catalog;history;iggy3d")

add_executable(creative_authored_asset_tests
  tests/unit/creative_authored_asset_tests.cpp)
target_link_libraries(creative_authored_asset_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_authored_asset_tests)
add_test(NAME creative_authored_asset_tests
  COMMAND "$<TARGET_FILE:creative_authored_asset_tests>")
set_tests_properties(creative_authored_asset_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;assets;catalog;history;prefab;iggy3d")

add_executable(creative_viewport_layout_tests
  tests/unit/creative_viewport_layout_tests.cpp)
target_link_libraries(creative_viewport_layout_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_viewport_layout_tests)
add_test(NAME creative_viewport_layout_tests
  COMMAND "$<TARGET_FILE:creative_viewport_layout_tests>")
set_tests_properties(creative_viewport_layout_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;render;frame_input;iggy3d")

add_executable(creative_desktop_ui_command_tests
  tests/unit/creative_desktop_ui_command_tests.cpp)
target_link_libraries(creative_desktop_ui_command_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_desktop_ui_command_tests)
add_test(NAME creative_desktop_ui_command_tests
  COMMAND "$<TARGET_FILE:creative_desktop_ui_command_tests>")
set_tests_properties(creative_desktop_ui_command_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;desktop;command;iggy3d")

add_executable(creative_desktop_model_tests
  tests/unit/creative_desktop_model_tests.cpp)
target_link_libraries(creative_desktop_model_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_desktop_model_tests)
add_test(NAME creative_desktop_model_tests
  COMMAND "$<TARGET_FILE:creative_desktop_model_tests>")
set_tests_properties(creative_desktop_model_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;desktop;model;iggy3d")

add_executable(creative_editor_world_layout_tests
  tests/unit/creative_editor_world_layout_tests.cpp)
target_link_libraries(creative_editor_world_layout_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_world_layout_tests)
add_test(NAME creative_editor_world_layout_tests
  COMMAND "$<TARGET_FILE:creative_editor_world_layout_tests>")
set_tests_properties(creative_editor_world_layout_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;world;layout;iggy3d")

add_executable(creative_editor_world_layout_topography_tests
  tests/unit/creative_editor_world_layout_topography_tests.cpp)
target_link_libraries(creative_editor_world_layout_topography_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_world_layout_topography_tests)
add_test(NAME creative_editor_world_layout_topography_tests
  COMMAND "$<TARGET_FILE:creative_editor_world_layout_topography_tests>")
set_tests_properties(creative_editor_world_layout_topography_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;world;layout;terrain;topography;iggy3d")

add_executable(creative_world_layout_diagnostics_tests
  tests/unit/creative_world_layout_diagnostics_tests.cpp)
target_link_libraries(creative_world_layout_diagnostics_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_world_layout_diagnostics_tests)
add_test(NAME creative_world_layout_diagnostics_tests
  COMMAND "$<TARGET_FILE:creative_world_layout_diagnostics_tests>")
set_tests_properties(creative_world_layout_diagnostics_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;world;layout;diagnostics;iggy3d")

add_executable(creative_world_layout_hierarchy_tests
  tests/unit/creative_world_layout_hierarchy_tests.cpp)
target_link_libraries(creative_world_layout_hierarchy_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_world_layout_hierarchy_tests)
add_test(NAME creative_world_layout_hierarchy_tests
  COMMAND "$<TARGET_FILE:creative_world_layout_hierarchy_tests>")
set_tests_properties(creative_world_layout_hierarchy_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;world;layout;hierarchy;iggy3d")

add_executable(creative_world_layout_properties_tests
  tests/unit/creative_world_layout_properties_tests.cpp)
target_link_libraries(creative_world_layout_properties_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_world_layout_properties_tests)
add_test(NAME creative_world_layout_properties_tests
  COMMAND "$<TARGET_FILE:creative_world_layout_properties_tests>")
set_tests_properties(creative_world_layout_properties_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;world;layout;properties;iggy3d")

add_executable(creative_world_layout_source_history_tests
  tests/unit/creative_world_layout_source_history_tests.cpp)
target_link_libraries(creative_world_layout_source_history_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_world_layout_source_history_tests)
add_test(NAME creative_world_layout_source_history_tests
  COMMAND "$<TARGET_FILE:creative_world_layout_source_history_tests>")
set_tests_properties(creative_world_layout_source_history_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;world;layout;history;iggy3d")

add_executable(creative_editor_play_mode_tests
  tests/unit/creative_editor_play_mode_tests.cpp)
target_link_libraries(creative_editor_play_mode_tests PRIVATE
  iggy3d_creative_app)
iggy3d_apply_warnings(creative_editor_play_mode_tests)
add_test(NAME creative_editor_play_mode_tests
  COMMAND "$<TARGET_FILE:creative_editor_play_mode_tests>")
set_tests_properties(creative_editor_play_mode_tests PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  LABELS "unit;app;creative;editor;play;runtime;session;iggy3d")

iggy3d_add_unit_test(creative_volume_tests
  tests/unit/creative_volume_tests.cpp)
set_tests_properties(creative_volume_tests PROPERTIES
  LABELS "unit;app;creative;tools;volume;iggy3d")

iggy3d_add_unit_test(creative_voxel_field_tests
  tests/unit/creative_voxel_field_tests.cpp)
set_tests_properties(creative_voxel_field_tests PROPERTIES
  LABELS "unit;app;creative;document;voxel;iggy3d")

iggy3d_add_unit_test(creative_terrain_field_tests
  tests/unit/creative_terrain_field_tests.cpp)
set_tests_properties(creative_terrain_field_tests PROPERTIES
  LABELS "unit;app;creative;document;terrain;iggy3d")

iggy3d_add_unit_test(creative_terrain_contour_tests
  tests/unit/creative_terrain_contour_tests.cpp)
set_tests_properties(creative_terrain_contour_tests PROPERTIES
  LABELS "unit;app;creative;document;terrain;contour;iggy3d")

iggy3d_add_unit_test(creative_terrain_generation_tests
  tests/unit/creative_terrain_generation_tests.cpp)
set_tests_properties(creative_terrain_generation_tests PROPERTIES
  LABELS "unit;app;creative;document;recipe;terrain;generation;iggy3d")

iggy3d_add_unit_test(creative_terrain_composition_tests
  tests/unit/creative_terrain_composition_tests.cpp)
set_tests_properties(creative_terrain_composition_tests PROPERTIES
  LABELS "unit;app;creative;document;recipe;terrain;composition;iggy3d")

iggy3d_add_unit_test(creative_terrain_operation_tests
  tests/unit/creative_terrain_operation_tests.cpp)
set_tests_properties(creative_terrain_operation_tests PROPERTIES
  LABELS "unit;app;creative;document;recipe;terrain;operation;iggy3d")

iggy3d_add_unit_test(creative_terrain_paint_tests
  tests/unit/creative_terrain_paint_tests.cpp)
set_tests_properties(creative_terrain_paint_tests PROPERTIES
  LABELS "unit;app;creative;document;terrain;paint;iggy3d")

iggy3d_add_unit_test(creative_select_tests
  tests/unit/creative_select_tests.cpp)
set_tests_properties(creative_select_tests PROPERTIES
  LABELS "unit;app;creative;select;iggy3d")

iggy3d_add_unit_test(creative_measure_tests
  tests/unit/creative_measure_tests.cpp)
set_tests_properties(creative_measure_tests PROPERTIES
  LABELS "unit;app;creative;measure;iggy3d")

iggy3d_add_unit_test(creative_snap_tests
  tests/unit/creative_snap_tests.cpp)
set_tests_properties(creative_snap_tests PROPERTIES
  LABELS "unit;app;creative;snap;iggy3d")

iggy3d_add_unit_test(creative_document_snap_tests
  tests/unit/creative_document_snap_tests.cpp)
set_tests_properties(creative_document_snap_tests PROPERTIES
  LABELS "unit;app;creative;document;snap;iggy3d")

iggy3d_add_unit_test(creative_ghost_tests
  tests/unit/creative_ghost_tests.cpp)
set_tests_properties(creative_ghost_tests PROPERTIES
  LABELS "unit;app;creative;ghost;iggy3d")

iggy3d_add_unit_test(creative_world_service_tests
  tests/unit/creative_world_service_tests.cpp)
set_tests_properties(creative_world_service_tests PROPERTIES
  LABELS "unit;app;product;creative;world;service;iggy3d")

iggy3d_add_unit_test(creative_map_template_tests
  tests/unit/creative_map_template_tests.cpp)
set_tests_properties(creative_map_template_tests PROPERTIES
  LABELS "unit;app;creative;world;map_template;iggy3d")

iggy3d_add_unit_test(creative_map_validation_tests
  tests/unit/creative_map_validation_tests.cpp)
set_tests_properties(creative_map_validation_tests PROPERTIES
  LABELS "unit;app;creative;validation;iggy3d")

iggy3d_add_unit_test(creative_play_preparation_tests
  tests/unit/creative_play_preparation_tests.cpp)
set_tests_properties(creative_play_preparation_tests PROPERTIES
  LABELS "unit;app;creative;play;validation;iggy3d")

iggy3d_add_unit_test(map_demo_tests tests/unit/map_demo_tests.cpp)
set_tests_properties(map_demo_tests PROPERTIES
  LABELS "unit;app;creative;play;runtime;movement;iggy3d")

iggy3d_add_unit_test(creative_runtime_sandbox_tests
  tests/unit/creative_runtime_sandbox_tests.cpp)
set_tests_properties(creative_runtime_sandbox_tests PROPERTIES
  LABELS "unit;app;creative;play;runtime;session;iggy3d")

iggy3d_add_unit_test(creative_runtime_moving_platform_tests
  tests/unit/creative_runtime_moving_platform_tests.cpp)
set_tests_properties(creative_runtime_moving_platform_tests PROPERTIES
  LABELS "unit;app;creative;play;runtime;physics;iggy3d")

iggy3d_add_unit_test(product_save_catalog_tests tests/unit/product_save_catalog_tests.cpp)
set_tests_properties(product_save_catalog_tests PROPERTIES
  LABELS "unit;app;product;save;catalog;iggy3d")

iggy3d_add_unit_test(world_state_tests tests/unit/world_state_tests.cpp)
set_tests_properties(world_state_tests PROPERTIES LABELS "unit;runtime;world;iggy3d")

iggy3d_add_unit_test(clock_tests tests/unit/clock_tests.cpp)
set_tests_properties(clock_tests PROPERTIES LABELS "unit;runtime;clock;iggy3d")

iggy3d_add_unit_test(camera_mode_policy_tests tests/unit/camera_mode_policy_tests.cpp)
set_tests_properties(camera_mode_policy_tests PROPERTIES LABELS "unit;runtime;camera;iggy3d")

iggy3d_add_unit_test(command_admission_tests tests/unit/command_admission_tests.cpp)
set_tests_properties(command_admission_tests PROPERTIES LABELS "unit;runtime;command;iggy3d")

iggy3d_add_unit_test(npc_behavior_system_tests tests/unit/npc_behavior_system_tests.cpp)
set_tests_properties(npc_behavior_system_tests PROPERTIES
  LABELS "unit;runtime;ai;npc_behavior;iggy3d")

iggy3d_add_unit_test(npc_behavior_profile_tests tests/unit/npc_behavior_profile_tests.cpp)
set_tests_properties(npc_behavior_profile_tests PROPERTIES
  LABELS "unit;runtime;ai;npc_behavior;profile;iggy3d")

iggy3d_add_unit_test(npc_alert_fsm_tests tests/unit/npc_alert_fsm_tests.cpp)
set_tests_properties(npc_alert_fsm_tests PROPERTIES
  LABELS "unit;runtime;ai;npc_behavior;alert;iggy3d")

iggy3d_add_unit_test(npc_patrol_system_tests tests/unit/npc_patrol_system_tests.cpp)
set_tests_properties(npc_patrol_system_tests PROPERTIES
  LABELS "unit;runtime;ai;npc_behavior;patrol;iggy3d")

iggy3d_add_unit_test(npc_investigate_system_tests tests/unit/npc_investigate_system_tests.cpp)
set_tests_properties(npc_investigate_system_tests PROPERTIES
  LABELS "unit;runtime;ai;npc_behavior;investigate;iggy3d")

iggy3d_add_unit_test(npc_sound_perception_tests tests/unit/npc_sound_perception_tests.cpp)
set_tests_properties(npc_sound_perception_tests PROPERTIES
  LABELS "unit;runtime;ai;npc_behavior;sound;iggy3d")

iggy3d_add_unit_test(segment_occlusion_tests tests/unit/segment_occlusion_tests.cpp)
set_tests_properties(segment_occlusion_tests PROPERTIES
  LABELS "unit;runtime;ai;segment_occlusion;iggy3d")

iggy3d_add_unit_test(reasoning_graph_tests tests/unit/reasoning_graph_tests.cpp)
set_tests_properties(reasoning_graph_tests PROPERTIES
  LABELS "unit;runtime;ai;reasoning;iggy3d")

iggy3d_add_unit_test(reachability_tests tests/unit/reachability_tests.cpp)
set_tests_properties(reachability_tests PROPERTIES
  LABELS "unit;core;grid;reachability;iggy3d")

iggy3d_add_unit_test(grid_footprint_tests tests/unit/grid_footprint_tests.cpp)
set_tests_properties(grid_footprint_tests PROPERTIES
  LABELS "unit;core;grid;footprint;iggy3d")

iggy3d_add_unit_test(greedy_mesh_tests tests/unit/greedy_mesh_tests.cpp)
set_tests_properties(greedy_mesh_tests PROPERTIES
  LABELS "unit;core;grid;mesh;iggy3d")

iggy3d_add_unit_test(aabb_grid_index_tests tests/unit/aabb_grid_index_tests.cpp)
set_tests_properties(aabb_grid_index_tests PROPERTIES
  LABELS "unit;core;spatial;index;iggy3d")

iggy3d_add_unit_test(oriented_box_tests tests/unit/oriented_box_tests.cpp)
set_tests_properties(oriented_box_tests PROPERTIES
  LABELS "unit;core;math;obb;iggy3d")

iggy3d_add_unit_test(aabb_ray_tests tests/unit/aabb_ray_tests.cpp)
set_tests_properties(aabb_ray_tests PROPERTIES
  LABELS "unit;core;math;ray;aabb;iggy3d")

iggy3d_add_unit_test(snap_kernel_tests tests/unit/snap_kernel_tests.cpp)
set_tests_properties(snap_kernel_tests PROPERTIES
  LABELS "unit;core;math;snap;iggy3d")

iggy3d_add_unit_test(frustum_tests tests/unit/frustum_tests.cpp)
set_tests_properties(frustum_tests PROPERTIES
  LABELS "unit;core;math;frustum;iggy3d")

iggy3d_add_unit_test(vec3_math_tests tests/unit/vec3_math_tests.cpp)
set_tests_properties(vec3_math_tests PROPERTIES
  LABELS "unit;core;math;vec3;iggy3d")

iggy3d_add_unit_test(reasoning_graph_readout tests/unit/reasoning_graph_readout.cpp)
set_tests_properties(reasoning_graph_readout PROPERTIES
  LABELS "unit;runtime;ai;reasoning;readout;iggy3d")

iggy3d_add_unit_test(reasoning_route_tests tests/unit/reasoning_route_tests.cpp)
set_tests_properties(reasoning_route_tests PROPERTIES
  LABELS "unit;runtime;ai;reasoning;route;iggy3d")

iggy3d_add_unit_test(guard_decision_tests tests/unit/guard_decision_tests.cpp)
set_tests_properties(guard_decision_tests PROPERTIES
  LABELS "unit;runtime;ai;reasoning;decision;iggy3d")

iggy3d_add_unit_test(guard_decision_readout tests/unit/guard_decision_readout.cpp)
set_tests_properties(guard_decision_readout PROPERTIES
  LABELS "unit;runtime;ai;reasoning;decision;readout;iggy3d")

iggy3d_add_unit_test(combat_system_tests tests/unit/combat_system_tests.cpp)
set_tests_properties(combat_system_tests PROPERTIES LABELS "unit;runtime;combat;iggy3d")

iggy3d_add_unit_test(entity_hit_query_tests tests/unit/entity_hit_query_tests.cpp)
set_tests_properties(entity_hit_query_tests PROPERTIES LABELS "unit;runtime;collision;entity;iggy3d")

iggy3d_add_unit_test(movement_policy_tests tests/unit/movement_policy_tests.cpp)
set_tests_properties(movement_policy_tests PROPERTIES LABELS "unit;runtime;movement;policy;iggy3d")

iggy3d_add_unit_test(movement_kinematics_tests tests/unit/movement_kinematics_tests.cpp)
set_tests_properties(movement_kinematics_tests PROPERTIES LABELS "unit;runtime;movement;math;iggy3d")

iggy3d_add_unit_test(player_motor_tests tests/unit/player_motor_tests.cpp)
set_tests_properties(player_motor_tests PROPERTIES LABELS "unit;runtime;player;movement;iggy3d")

iggy3d_add_unit_test(player_physics_move_planner_tests
  tests/unit/player_physics_move_planner_tests.cpp)
set_tests_properties(player_physics_move_planner_tests PROPERTIES
  LABELS "unit;runtime;player;physics_move;iggy3d")

iggy3d_add_unit_test(debug_hud_text_tests tests/unit/debug_hud_text_tests.cpp)
set_tests_properties(debug_hud_text_tests PROPERTIES LABELS "unit;render;debug;hud;iggy3d")

iggy3d_add_unit_test(bean_mesh_tests tests/unit/bean_mesh_tests.cpp)
set_tests_properties(bean_mesh_tests PROPERTIES LABELS "unit;render;mesh;bean;iggy3d")

iggy3d_add_unit_test(scenario_seed_conversion_tests
  tests/unit/scenario_seed_conversion_tests.cpp)
set_tests_properties(scenario_seed_conversion_tests PROPERTIES
  LABELS "unit;runtime;session;content;seed;iggy3d")

iggy3d_add_unit_test(movement_system_tests tests/unit/movement_system_tests.cpp)
set_tests_properties(movement_system_tests PROPERTIES LABELS "unit;runtime;movement;iggy3d")

iggy3d_add_unit_test(clamber_motor_tests tests/unit/clamber_motor_tests.cpp)
set_tests_properties(clamber_motor_tests PROPERTIES LABELS "unit;runtime;movement;session;iggy3d")

iggy3d_add_unit_test(ability_command_tests tests/unit/ability_command_tests.cpp)
set_tests_properties(ability_command_tests PROPERTIES LABELS "unit;runtime;ability;command;iggy3d")

iggy3d_add_unit_test(target_reach_tests tests/unit/target_reach_tests.cpp)
set_tests_properties(target_reach_tests PROPERTIES LABELS "unit;runtime;targeting;iggy3d")

iggy3d_add_unit_test(inventory_system_tests tests/unit/inventory_system_tests.cpp)
set_tests_properties(inventory_system_tests PROPERTIES LABELS "unit;runtime;inventory;iggy3d")

iggy3d_add_unit_test(objective_system_tests tests/unit/objective_system_tests.cpp)
set_tests_properties(objective_system_tests PROPERTIES LABELS "unit;runtime;objective;iggy3d")

iggy3d_add_unit_test(objective_outcome_tests tests/unit/objective_outcome_tests.cpp)
set_tests_properties(objective_outcome_tests PROPERTIES LABELS "unit;runtime;objective;iggy3d")

iggy3d_add_unit_test(interaction_system_tests tests/unit/interaction_system_tests.cpp)
set_tests_properties(interaction_system_tests PROPERTIES LABELS "unit;runtime;interaction;iggy3d")

iggy3d_add_unit_test(physics_collision_queries_tests
  tests/unit/physics_collision_queries_tests.cpp)
set_tests_properties(physics_collision_queries_tests PROPERTIES
  LABELS "unit;runtime;physics;collision_queries;iggy3d")

iggy3d_add_unit_test(physics_frame_stats_tests
  tests/unit/physics_frame_stats_tests.cpp)
set_tests_properties(physics_frame_stats_tests PROPERTIES
  LABELS "unit;runtime;physics;frame_stats;iggy3d")

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

iggy3d_add_unit_test(physics_aabb_collider_tests
  tests/unit/physics_aabb_collider_tests.cpp)
set_tests_properties(physics_aabb_collider_tests PROPERTIES
  LABELS "unit;runtime;physics;aabb;collider;iggy3d")

iggy3d_add_unit_test(physics_broadphase_tests
  tests/unit/physics_broadphase_tests.cpp)
set_tests_properties(physics_broadphase_tests PROPERTIES
  LABELS "unit;runtime;physics;broadphase;aabb;iggy3d")

iggy3d_add_unit_test(session_tick_tests tests/unit/session_tick_tests.cpp)
set_tests_properties(session_tick_tests PROPERTIES LABELS "unit;runtime;session;iggy3d")

iggy3d_add_unit_test(save_load_tests tests/unit/save_load_tests.cpp)
set_tests_properties(save_load_tests PROPERTIES LABELS "unit;runtime;save;iggy3d")

iggy3d_add_unit_test(save_creative_document_section_tests
  tests/unit/save_creative_document_section_tests.cpp)
set_tests_properties(save_creative_document_section_tests PROPERTIES
  LABELS "unit;runtime;save;creative;iggy3d")

iggy3d_add_unit_test(render_boundary_tests tests/unit/render_boundary_tests.cpp)
set_tests_properties(render_boundary_tests PROPERTIES LABELS "unit;render;boundary;iggy3d")

iggy3d_add_unit_test(render_config_tests tests/unit/render_config_tests.cpp)
set_tests_properties(render_config_tests PROPERTIES LABELS "unit;render;config;iggy3d")

iggy3d_add_unit_test(render_diagnostics_tests tests/unit/render_diagnostics_tests.cpp)
set_tests_properties(render_diagnostics_tests PROPERTIES LABELS "unit;render;diagnostics;iggy3d")

iggy3d_add_unit_test(render_projection_input_tests tests/unit/render_projection_input_tests.cpp)
set_tests_properties(render_projection_input_tests PROPERTIES LABELS "unit;render;frame_input;iggy3d")

iggy3d_add_unit_test(render_content_viewport_tests tests/unit/render_content_viewport_tests.cpp)
set_tests_properties(render_content_viewport_tests PROPERTIES LABELS "unit;render;frame_input;iggy3d")

iggy3d_add_unit_test(render_camera_frame_tests tests/unit/render_camera_frame_tests.cpp)
set_tests_properties(render_camera_frame_tests PROPERTIES LABELS "unit;render;camera;iggy3d")

iggy3d_add_unit_test(package_runtime_lookup_tests tests/unit/package_runtime_lookup_tests.cpp)
set_tests_properties(package_runtime_lookup_tests PROPERTIES LABELS "unit;render;package;iggy3d")

# Render policy/shader unit tests. The Vulkan backend is always compiled into
# the library now, so the old packet4/packet6 conditional-source helpers reduce
# to plain registrations with the historical labels.
function(iggy3d_add_render_policy_unit_test test_name source_file)
  iggy3d_add_unit_test("${test_name}" "${source_file}")
  set_tests_properties("${test_name}" PROPERTIES LABELS "unit;render;vulkan;iggy3d")
endfunction()

function(iggy3d_add_render_shader_unit_test test_name source_file)
  iggy3d_add_unit_test("${test_name}" "${source_file}")
  set_tests_properties("${test_name}" PROPERTIES LABELS "unit;render;vulkan;packet6;iggy3d")
  target_compile_definitions("${test_name}"
    PRIVATE
      IGGY3D_SHADER_SOURCE_ROOT_VALUE="${IGGY3D_SHADER_SOURCE_ROOT}"
      IGGY3D_SHADER_BINARY_ROOT_VALUE="${IGGY3D_SHADER_BINARY_ROOT}"
      IGGY3D_SHADER_TARGET_ENV_VALUE="${IGGY3D_SHADER_TARGET_ENV}")
  if(IGGY3D_SHADER_COMPILER_AVAILABLE)
    target_compile_definitions("${test_name}" PRIVATE IGGY3D_SHADER_COMPILER_AVAILABLE=1)
  endif()
endfunction()

iggy3d_add_render_policy_unit_test(render_result_mapping_tests
  tests/unit/render_result_mapping_tests.cpp)
iggy3d_add_render_policy_unit_test(render_command_recording_tests
  tests/unit/render_command_recording_tests.cpp)
iggy3d_add_render_policy_unit_test(render_reason_code_tests
  tests/unit/render_reason_code_tests.cpp)
iggy3d_add_render_policy_unit_test(render_unsupported_device_policy_tests
  tests/unit/render_unsupported_device_policy_tests.cpp)
iggy3d_add_render_shader_unit_test(render_vertex_format_tests
  tests/unit/render_vertex_format_tests.cpp)
iggy3d_add_render_shader_unit_test(render_shader_build_policy_tests
  tests/unit/render_shader_build_policy_tests.cpp)
iggy3d_add_render_shader_unit_test(render_memory_budget_policy_tests
  tests/unit/render_memory_budget_policy_tests.cpp)

# ---- Vulkan smokes (self-skipping: exit 77 when no usable Vulkan device) ----
function(iggy3d_add_vulkan_smoke target_name)
  add_executable("${target_name}" "tests/smoke/${target_name}.cpp")
  target_link_libraries("${target_name}" PRIVATE iggy3d)
  iggy3d_apply_warnings("${target_name}")
  if(IGGY3D_REQUIRE_VULKAN_SMOKE)
    target_compile_definitions("${target_name}" PRIVATE IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED=1)
  endif()
endfunction()

set(IGGY3D_VULKAN_SMOKES
    vulkan_platform_smoke vulkan_device_smoke vulkan_feature_baseline_smoke
    vulkan_validation_smoke vulkan_swapchain_smoke vulkan_resize_minimize_smoke
    vulkan_empty_frame_smoke vulkan_sync_smoke vulkan_pipeline_smoke
    vulkan_memory_smoke vulkan_material_smoke vulkan_diagnostics_smoke
    vulkan_optional_unsupported_smoke vulkan_strict_unsupported_smoke)
foreach(smoke IN LISTS IGGY3D_VULKAN_SMOKES)
  iggy3d_add_vulkan_smoke("${smoke}")
endforeach()

if(IGGY3D_REQUIRE_VALIDATION_LAYERS)
  target_compile_definitions(vulkan_validation_smoke
    PRIVATE IGGY3D_REQUIRE_VALIDATION_LAYERS_ENABLED=1)
endif()

foreach(packet6_smoke vulkan_pipeline_smoke vulkan_memory_smoke vulkan_material_smoke)
  target_compile_definitions("${packet6_smoke}"
    PRIVATE
      IGGY3D_SHADER_SOURCE_ROOT_VALUE="${IGGY3D_SHADER_SOURCE_ROOT}"
      IGGY3D_SHADER_BINARY_ROOT_VALUE="${IGGY3D_SHADER_BINARY_ROOT}"
      IGGY3D_SHADER_TARGET_ENV_VALUE="${IGGY3D_SHADER_TARGET_ENV}")
  if(IGGY3D_SHADER_COMPILER_AVAILABLE)
    target_compile_definitions("${packet6_smoke}" PRIVATE IGGY3D_SHADER_COMPILER_AVAILABLE=1)
  endif()
  if(IGGY3D_ENABLE_VULKAN_SHADERS AND IGGY3D_SHADER_COMPILER_AVAILABLE)
    add_dependencies("${packet6_smoke}" iggy3d_vulkan_shaders)
  endif()
endforeach()

add_test(NAME vulkan_platform_smoke_window
         COMMAND "$<TARGET_FILE:vulkan_platform_smoke>" --mode window_only)
add_test(NAME vulkan_platform_smoke_extensions
         COMMAND "$<TARGET_FILE:vulkan_platform_smoke>" --mode extension_query)
set(IGGY3D_VULKAN_SMOKE_TESTS vulkan_platform_smoke_window vulkan_platform_smoke_extensions)
foreach(smoke IN LISTS IGGY3D_VULKAN_SMOKES)
  if(NOT smoke STREQUAL "vulkan_platform_smoke")
    add_test(NAME "${smoke}" COMMAND "$<TARGET_FILE:${smoke}>")
    list(APPEND IGGY3D_VULKAN_SMOKE_TESTS "${smoke}")
  endif()
endforeach()
set_tests_properties(${IGGY3D_VULKAN_SMOKE_TESTS} PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  SKIP_RETURN_CODE 77
  LABELS "smoke;vulkan;render;iggy3d")
# The strict smoke exercises the fail-closed path on purpose: failure is the pass.
set_tests_properties(vulkan_strict_unsupported_smoke PROPERTIES WILL_FAIL TRUE)

# T-0 capture regression guard (docs/creative_desktop_ui_plan.md §6): spawns
# the real i3dc binary twice in --capture mode and asserts artifact quartet +
# run-to-run hash determinism + external_ui_recorded=0. Self-skips (77)
# without a usable Vulkan device, like every other smoke.
add_executable(creative_capture_stability_smoke
  tests/smoke/creative_capture_stability_smoke.cpp)
target_link_libraries(creative_capture_stability_smoke PRIVATE iggy3d)
iggy3d_apply_warnings(creative_capture_stability_smoke)
target_compile_definitions(creative_capture_stability_smoke PRIVATE
  I3DC_BINARY_PATH="$<TARGET_FILE:i3dc>")
add_dependencies(creative_capture_stability_smoke i3dc)
add_test(NAME creative_capture_stability_smoke
         COMMAND "$<TARGET_FILE:creative_capture_stability_smoke>")
set_tests_properties(creative_capture_stability_smoke PROPERTIES
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  SKIP_RETURN_CODE 77
  LABELS "smoke;vulkan;render;creative;iggy3d")

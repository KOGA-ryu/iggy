#include "ProductAutomationSmokeSupport.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

bool expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool validAsciiPreview(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "ascii_room.build") &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_action",
                                 "ascii_room.build") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_status",
                                 "product_ascii_room_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_reason_code",
                                 "product_ascii_room_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_failed_stage",
                                 "none") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_room_id",
                                 "automation_training_room") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_source_name",
                                 "automation/training_room.iggyroom.txt") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields, "ascii_room_preview_width", "7") &&
         iggy3d::smoke::hasField(fields, "ascii_room_preview_height", "5") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_floor_count",
                                 "15") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_wall_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_marker_count",
                                 "5") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_elevated_floor_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_ramp_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_blocked_slope_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_static_mesh_count",
                                 "36") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_anchor_count",
                                 "5") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_spatial_surface_count",
                                 "56") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_asset_text_written",
                                 "true") &&
         iggy3d::smoke::positiveIntegerField(
             fields, "ascii_room_preview_asset_text_bytes") &&
         iggy3d::smoke::hasField(fields, "active_room_loaded", "false") &&
         iggy3d::smoke::hasField(fields, "active_room_status", "not_loaded") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_ready",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_status",
                                 "not_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_reason_code",
                                 "not_loaded") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "runtime_session_created", "false");
}

bool invalidAsciiPreview(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationCommandFailed(fields, "ascii_room.build") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_status",
                                 "ascii_room_missing_player_spawn") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_reason_code",
                                 "ascii_room_missing_player_spawn") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_failed_stage",
                                 "grid") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_room_id",
                                 "missing_spawn_room") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_source_name",
                                 "automation/missing_spawn.iggyroom.txt") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_ready",
                                 "false") &&
         iggy3d::smoke::hasField(fields, "ascii_room_preview_width", "3") &&
         iggy3d::smoke::hasField(fields, "ascii_room_preview_height", "2") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_elevated_floor_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_ramp_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_blocked_slope_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_asset_text_written",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_asset_text_bytes",
                                 "0") &&
         iggy3d::smoke::hasField(fields, "active_room_loaded", "false") &&
         iggy3d::smoke::hasField(fields, "active_room_status", "not_loaded") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_ready",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_status",
                                 "not_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_reason_code",
                                 "not_loaded") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "runtime_session_created", "false");
}

bool terrainAsciiPreview(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "ascii_room.build") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_status",
                                 "product_ascii_room_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_reason_code",
                                 "product_ascii_room_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields, "ascii_room_preview_width", "7") &&
         iggy3d::smoke::hasField(fields, "ascii_room_preview_height", "4") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_floor_count",
                                 "10") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_wall_count",
                                 "18") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_marker_count",
                                 "4") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_elevated_floor_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_ramp_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_blocked_slope_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_static_mesh_count",
                                 "28") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_anchor_count",
                                 "4") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_spatial_surface_count",
                                 "46") &&
         iggy3d::smoke::hasField(fields, "active_room_loaded", "false") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false");
}

bool activatedAsciiRoom(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "ascii_room.activate") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_status",
                                 "product_ascii_room_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_status",
                                 "ascii_room_activated") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_reason_code",
                                 "ascii_room_activated") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_room_id",
                                 "automation_activation_room") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_package_id",
                                 "iggy3d.ascii_room_authoring") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_scenario_id",
                                 "automation_activation_room.runtime_loop") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_session_created",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_player_spawned",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_player_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_entity_count",
                                 "5") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_npc_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_pickup_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_door_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_marker_entity_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_objective_count",
                                 "2") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_wall_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_marker_count",
                                 "5") &&
         iggy3d::smoke::positiveIntegerField(
             fields, "ascii_room_activation_runtime_hash") &&
         iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_status",
                                 "active_room_loaded") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_reason_code",
                                 "active_room_loaded") &&
         iggy3d::smoke::hasField(fields, "active_room_source", "ascii_room") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_id",
                                 "automation_activation_room") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_has_authored_room",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_authored_floor_count",
                                 "15") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_authored_wall_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_static_mesh_count",
                                 "36") &&
         iggy3d::smoke::hasField(fields, "active_room_anchor_count", "5") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_spatial_surface_count",
                                 "56") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_walkable_surface_count",
                                 "15") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_actor_blocker_count",
                                 "21") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_projectile_blocker_count",
                                 "21") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_status",
                                 "active_room_collision_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_reason_code",
                                 "active_room_collision_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_room_id",
                                 "automation_activation_room") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_spatial_surface_count",
                                 "56") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_query_surface_count",
                                 "56") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_walkable_surface_count",
                                 "15") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_actor_blocker_count",
                                 "21") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_projectile_blocker_count",
                                 "21") &&
         iggy3d::smoke::hasField(
             fields, "active_room_collision_runtime_owned_surface_count", "1") &&
         iggy3d::smoke::hasField(
             fields, "active_room_collision_runtime_filtered_surface_count", "0") &&
         iggy3d::smoke::hasField(
             fields, "active_room_collision_door_blocker_count", "1") &&
         iggy3d::smoke::hasField(
             fields, "active_room_collision_active_door_blocker_count", "1") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "runtime_session_created", "true") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields, "scene_item_count", "5") &&
         iggy3d::smoke::hasField(fields, "player_visible", "true") &&
         iggy3d::smoke::hasField(fields, "room_visible", "true") &&
         iggy3d::smoke::hasField(fields, "objective_visible", "true") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_item_count",
                                 "41") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_door_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_open_door_visible",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_closed_door_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_door_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_open_door_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_closed_door_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_room_geometry_count",
                                 "35") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_floor_tile_count",
                                 "15") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_elevated_floor_tile_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_ramp_tile_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_blocked_slope_tile_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_wall_tile_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_mesh_cpu_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_mesh_backend_presented",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_mesh_source",
                                 "scene_room_projection") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_asset_id",
                                 "automation_activation_room") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_floor_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_wall_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_grid_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_source_mesh_count",
                                 "35") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_vertex_count",
                                 "2040") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_index_count",
                                 "18360") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_draw_count",
                                 "255") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_floor_draw_count",
                                 "15") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_wall_draw_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_grid_line_draw_count",
                                 "220") &&
         iggy3d::smoke::hasField(fields,
                                 "product_vulkan_room_grid_truncated",
                                 "false") &&
         iggy3d::smoke::positiveIntegerField(
             fields, "product_vulkan_room_geometry_signature") &&
         iggy3d::smoke::positiveIntegerField(fields, "runtime_state_hash");
}

bool invalidAsciiActivation(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationCommandFailed(fields,
                                                "ascii_room.activate") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_status",
                                 "ascii_room_missing_player_spawn") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_reason_code",
                                 "ascii_room_missing_player_spawn") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_session_created",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_player_spawned",
                                 "false") &&
         iggy3d::smoke::hasField(fields, "active_room_loaded", "false") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_status",
                                 "ascii_room_missing_player_spawn") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_ready",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_status",
                                 "active_room_collision_unavailable") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_reason_code",
                                 "ascii_room_missing_player_spawn") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "runtime_session_created", "false");
}

bool activatedAsciiRoomOpenMove(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "game.move_x") &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_action",
                                 "game.move_x") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_collision_surfaces_used",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_collision_surface_count",
                                 "56") &&
         iggy3d::smoke::hasField(fields,
                                 "input_action_last",
                                 "game.move_x") &&
         iggy3d::smoke::hasField(fields,
                                 "input_action_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_kind",
                                 "move") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tick_advanced",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tick_reason_code",
                                 "ok") &&
         iggy3d::smoke::hasField(fields,
                                 "player_position_changed",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_attempted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_blocked",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_status",
                                 "moved") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_debug_available",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_reason_code",
                                 "movement_ok") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_blocked_reason",
                                 "movement_ok") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_hit_surface_id",
                                 "none") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_ground_snap_applied",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_clamped",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_collision_sweep_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_policy_band",
                                 "flat") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_speed_multiplier",
                                 "1.000") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_line_count",
                                 "7") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_status",
                                 "moved") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_blocked",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_reason_code",
                                 "movement_ok") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_hit_surface_id",
                                 "none") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_policy_band",
                                 "flat") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_speed_multiplier",
                                 "1.000");
}

bool activatedAsciiRoomWallMoveAccepted(
    const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "game.move_y") &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_action",
                                 "game.move_y") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_collision_surfaces_used",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_collision_surface_count",
                                 "56") &&
         iggy3d::smoke::hasField(fields,
                                 "input_action_last",
                                 "game.move_y") &&
         iggy3d::smoke::hasField(fields,
                                 "input_action_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_kind",
                                 "move") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tick_advanced",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tick_reason_code",
                                 "ok") &&
         iggy3d::smoke::hasField(fields,
                                 "player_position_changed",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_attempted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_blocked",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_status",
                                 "moved") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_debug_available",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_reason_code",
                                 "movement_ok") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_blocked_reason",
                                 "movement_ok") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_hit_surface_id",
                                 "none") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_ground_snap_applied",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_clamped",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_collision_sweep_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_policy_band",
                                 "flat") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_line_count",
                                 "7") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_status",
                                 "moved") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_blocked",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_reason_code",
                                 "movement_ok") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_hit_surface_id",
                                 "none") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_policy_band",
                                 "flat");
}

bool activatedAsciiRoomRampMoveAccepted(
    const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "game.move_x") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_spatial_surface_count",
                                 "37") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_collision_surface_count",
                                 "37") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_room_geometry_count",
                                 "21") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_floor_tile_count",
                                 "4") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_elevated_floor_tile_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_ramp_tile_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_blocked_slope_tile_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_wall_tile_count",
                                 "16") &&
         iggy3d::smoke::hasField(fields,
                                 "input_action_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_kind",
                                 "move") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tick_advanced",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "player_position_changed",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_blocked",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_status",
                                 "moved") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_debug_available",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_reason_code",
                                 "movement_ok") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_blocked_reason",
                                 "movement_ok") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_ground_snap_applied",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_policy_band",
                                 "moderate") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_slope_travel_direction",
                                 "uphill") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_slope_angle_degrees",
                                 "26.565") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_speed_multiplier",
                                 "0.750") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_final_y",
                                 "0.250") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_status",
                                 "moved") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_blocked",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_policy_band",
                                 "moderate") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_speed_multiplier",
                                 "0.750");
}

bool activatedAsciiRoomSteepMoveRejected(
    const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "game.move_x") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_spatial_surface_count",
                                 "37") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_room_geometry_count",
                                 "21") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_floor_tile_count",
                                 "4") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_elevated_floor_tile_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_ramp_tile_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_blocked_slope_tile_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_wall_tile_count",
                                 "16") &&
         iggy3d::smoke::hasField(fields,
                                 "input_action_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_kind",
                                 "move") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tick_advanced",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "player_position_changed",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_blocked",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_status",
                                 "blocked") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_debug_available",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_reason_code",
                                 "slope_rejected") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_blocked_reason",
                                 "slope_rejected") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_hit_surface_id",
                                 "none") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_ground_snap_applied",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_clamped",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_collision_sweep_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_policy_band",
                                 "blocked") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_slope_angle_degrees",
                                 "45.000") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_speed_multiplier",
                                 "0.000") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_status",
                                 "blocked") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_blocked",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_reason_code",
                                 "slope_rejected") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_policy_band",
                                 "blocked") &&
         iggy3d::smoke::hasField(fields,
                                 "movement_debug_hud_speed_multiplier",
                                 "0.000");
}

bool activatedAsciiRoomInteractKey(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "game.interact") &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_action",
                                 "game.interact") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_player_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_entity_count",
                                 "2") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_npc_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_pickup_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_door_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_marker_entity_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_objective_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "input_action_last",
                                 "game.interact") &&
         iggy3d::smoke::hasField(fields,
                                 "input_action_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "target_discovered",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_status",
                                 "found") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_action",
                                 "interact") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_entity_id",
                                 "2") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_stable_name",
                                 "marker_key_r1_c2") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_kind",
                                 "pickup") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_distance_meters",
                                 "1.000") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_supports_command",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_status",
                                 "succeeded") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_target_active_after",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_inventory_changed",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_item_id",
                                 "marker_key_r1_c2") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_item_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_objective_changed",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_event_count",
                                 "3") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_reach_gate",
                                 "pass") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_last_rejection",
                                 "none") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_kind",
                                 "interact") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_status",
                                 "accepted") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tick_advanced",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tick_reason_code",
                                 "ok") &&
         iggy3d::smoke::hasField(fields, "interaction_executed", "true") &&
         iggy3d::smoke::hasField(fields, "attack_executed", "false") &&
         iggy3d::smoke::hasField(fields,
                                 "product_feedback_target_status",
                                 "discovered") &&
         iggy3d::smoke::hasField(fields,
                                 "product_feedback_reach_status",
                                 "pass") &&
         iggy3d::smoke::hasField(fields,
                                 "product_feedback_command_kind",
                                 "interact") &&
         iggy3d::smoke::hasField(fields,
                                 "product_feedback_command_status",
                                 "accepted");
}

bool activatedAsciiRoomInteractTreasure(
    const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "game.interact") &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_action",
                                 "game.interact") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_status",
                                 "found") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_action",
                                 "interact") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_stable_name",
                                 "marker_treasure_r1_c2") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_kind",
                                 "pickup") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_status",
                                 "succeeded") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_target_active_after",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_inventory_changed",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_item_id",
                                 "marker_treasure_r1_c2") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_item_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_objective_changed",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_event_count",
                                 "3") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_reach_gate",
                                 "pass") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_status",
                                 "accepted") &&
         iggy3d::smoke::hasField(fields, "interaction_executed", "true") &&
         iggy3d::smoke::hasField(fields, "attack_executed", "false");
}

bool activatedAsciiRoomInteractDoor(
    const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_key",
                                 "game.interact") &&
         iggy3d::smoke::hasField(fields,
                                 "automation_control_last_action",
                                 "game.interact") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_door_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_pickup_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_status",
                                 "found") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_action",
                                 "interact") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_stable_name",
                                 "marker_door_r1_c2") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_kind",
                                 "door") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_status",
                                 "succeeded") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_target_active_after",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_inventory_changed",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_item_id",
                                 "none") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_item_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_objective_changed",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_event_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_reach_gate",
                                 "pass") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_status",
                                 "accepted") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tick_reason_code",
                                 "ok") &&
         iggy3d::smoke::hasField(
             fields, "active_room_collision_runtime_owned_surface_count", "1") &&
         iggy3d::smoke::hasField(
             fields, "active_room_collision_runtime_filtered_surface_count", "1") &&
         iggy3d::smoke::hasField(
             fields, "active_room_collision_door_blocker_count", "1") &&
         iggy3d::smoke::hasField(
             fields, "active_room_collision_active_door_blocker_count", "0") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_door_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_open_door_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_closed_door_visible",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_door_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_open_door_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_closed_door_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields, "interaction_executed", "true") &&
         iggy3d::smoke::hasField(fields, "attack_executed", "false");
}

bool activatedAsciiRoomSecretDoorRequiresKey(
    const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::automationCommandFailed(fields, "game.interact") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_door_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_activation_pickup_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_status",
                                 "found") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_action",
                                 "interact") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_stable_name",
                                 "marker_secret_door_r1_c2") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_kind",
                                 "door") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_distance_meters",
                                 "1.000") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_target_supports_command",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_kind",
                                 "interact") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_status",
                                 "rejected") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_command_accepted",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_last_rejection",
                                 "required_item_missing") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_reach_gate",
                                 "not_attempted") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_status",
                                 "rejected") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_target_active_after",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_inventory_changed",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_outcome_objective_changed",
                                 "false") &&
         iggy3d::smoke::hasField(fields, "interaction_executed", "false") &&
         iggy3d::smoke::hasField(fields, "attack_executed", "false") &&
         iggy3d::smoke::hasField(
             fields, "active_room_collision_runtime_filtered_surface_count", "0") &&
         iggy3d::smoke::hasField(
             fields, "active_room_collision_active_door_blocker_count", "1") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_open_door_count",
                                 "0") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_closed_door_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "product_feedback_command_kind",
                                 "interact") &&
         iggy3d::smoke::hasField(fields,
                                 "product_feedback_command_status",
                                 "rejected") &&
         iggy3d::smoke::hasField(fields,
                                 "product_feedback_rejection_reason",
                                 "required_item_missing");
}

}  // namespace

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);

  int validExitCode = 77;
  iggy3d::smoke::ReceiptFields validFields;
  const bool validReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_valid",
          "ascii_room.room_id=automation_training_room\n"
          "ascii_room.source_name=automation/training_room.iggyroom.txt\n"
          "ascii_room.text=#######\\n#P..N.#\\n#.+.$.#\\n#..E..#\\n#######\\n\n"
          "ascii_room.build=true\n",
          "",
          validFields,
          validExitCode);

  int invalidExitCode = 77;
  iggy3d::smoke::ReceiptFields invalidFields;
  const bool invalidReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_invalid",
          "ascii_room.room_id=missing_spawn_room\n"
          "ascii_room.source_name=automation/missing_spawn.iggyroom.txt\n"
          "ascii_room.text=...\\n...\\n\n"
          "ascii_room.build=true\n",
          "",
          invalidFields,
          invalidExitCode);

  int terrainPreviewExitCode = 77;
  iggy3d::smoke::ReceiptFields terrainPreviewFields;
  const bool terrainPreviewReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_terrain_preview",
          "ascii_room.room_id=automation_terrain_preview_room\n"
          "ascii_room.source_name=automation/terrain_preview_room.iggyroom.txt\n"
          "ascii_room.text=#######\\n#P1>!N#\\n#..$E.#\\n#######\\n\n"
          "ascii_room.build=true\n",
          "",
          terrainPreviewFields,
          terrainPreviewExitCode);

  int activateExitCode = 77;
  iggy3d::smoke::ReceiptFields activateFields;
  const bool activateReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_activate",
          "ascii_room.room_id=automation_activation_room\n"
          "ascii_room.source_name=automation/activation_room.iggyroom.txt\n"
          "ascii_room.text=#######\\n#P..N.#\\n#.+.$.#\\n#..E..#\\n#######\\n\n"
          "ascii_room.activate=true\n",
          "",
          activateFields,
          activateExitCode);

  int invalidActivateExitCode = 77;
  iggy3d::smoke::ReceiptFields invalidActivateFields;
  const bool invalidActivateReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_invalid_activate",
          "ascii_room.room_id=missing_spawn_activation_room\n"
          "ascii_room.source_name=automation/missing_spawn_activation.iggyroom.txt\n"
          "ascii_room.text=...\\n...\\n\n"
          "ascii_room.activate=true\n",
          "",
          invalidActivateFields,
          invalidActivateExitCode);

  int openMoveExitCode = 77;
  iggy3d::smoke::ReceiptFields openMoveFields;
  const bool openMoveReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_open_move",
          "ascii_room.room_id=automation_open_move_room\n"
          "ascii_room.source_name=automation/open_move_room.iggyroom.txt\n"
          "ascii_room.text=#######\\n#P..N.#\\n#.+.$.#\\n#..E..#\\n#######\\n\n"
          "ascii_room.activate=true\n"
          "game.move_x=1\n",
          "",
          openMoveFields,
          openMoveExitCode);

  int wallMoveExitCode = 77;
  iggy3d::smoke::ReceiptFields wallMoveFields;
  const bool wallMoveReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_wall_move",
          "ascii_room.room_id=automation_wall_move_room\n"
          "ascii_room.source_name=automation/wall_move_room.iggyroom.txt\n"
          "ascii_room.text=#######\\n#P..N.#\\n#.+.$.#\\n#..E..#\\n#######\\n\n"
          "ascii_room.activate=true\n"
          "game.move_y=-1\n",
          "",
          wallMoveFields,
          wallMoveExitCode);

  int rampMoveExitCode = 77;
  iggy3d::smoke::ReceiptFields rampMoveFields;
  const bool rampMoveReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_ramp_move",
          "ascii_room.room_id=automation_ramp_move_room\n"
          "ascii_room.source_name=automation/ramp_move_room.iggyroom.txt\n"
          "ascii_room.text=#######\\n#P>..$#\\n#######\\n\n"
          "ascii_room.activate=true\n"
          "game.move_x=1\n",
          "",
          rampMoveFields,
          rampMoveExitCode);

  int steepMoveExitCode = 77;
  iggy3d::smoke::ReceiptFields steepMoveFields;
  const bool steepMoveReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_steep_move",
          "ascii_room.room_id=automation_steep_move_room\n"
          "ascii_room.source_name=automation/steep_move_room.iggyroom.txt\n"
          "ascii_room.text=#######\\n#P!..$#\\n#######\\n\n"
          "ascii_room.activate=true\n"
          "game.move_x=1\n",
          "",
          steepMoveFields,
          steepMoveExitCode);

  int interactExitCode = 77;
  iggy3d::smoke::ReceiptFields interactFields;
  const bool interactReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_interact_key",
          "ascii_room.room_id=automation_interact_key_room\n"
          "ascii_room.source_name=automation/interact_key_room.iggyroom.txt\n"
          "ascii_room.text=#####\\n#PK.#\\n#####\\n\n"
          "ascii_room.activate=true\n"
          "game.interact=true\n",
          "",
          interactFields,
          interactExitCode);

  int treasureExitCode = 77;
  iggy3d::smoke::ReceiptFields treasureFields;
  const bool treasureReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_interact_treasure",
          "ascii_room.room_id=automation_interact_treasure_room\n"
          "ascii_room.source_name=automation/interact_treasure_room.iggyroom.txt\n"
          "ascii_room.text=#####\\n#P$.#\\n#####\\n\n"
          "ascii_room.activate=true\n"
          "game.interact=true\n",
          "",
          treasureFields,
          treasureExitCode);

  int doorExitCode = 77;
  iggy3d::smoke::ReceiptFields doorFields;
  const bool doorReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_interact_door",
          "ascii_room.room_id=automation_interact_door_room\n"
          "ascii_room.source_name=automation/interact_door_room.iggyroom.txt\n"
          "ascii_room.text=######\\n#P+K.#\\n######\\n\n"
          "ascii_room.activate=true\n"
          "game.interact=true\n",
          "",
          doorFields,
          doorExitCode);

  int lockedSecretDoorExitCode = 77;
  iggy3d::smoke::ReceiptFields lockedSecretDoorFields;
  const bool lockedSecretDoorReceipt =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_authoring_locked_secret_door",
          "ascii_room.room_id=automation_locked_secret_room\n"
          "ascii_room.source_name=automation/locked_secret_room.iggyroom.txt\n"
          "ascii_room.text=#####\\n#PsK#\\n#####\\n\n"
          "ascii_room.activate=true\n"
          "game.interact=true\n",
          "",
          lockedSecretDoorFields,
          lockedSecretDoorExitCode);

  const bool validPassed =
      validExitCode == 0 && validReceipt &&
      iggy3d::smoke::productReceipt(validFields) &&
      validAsciiPreview(validFields);
  const bool invalidPassed =
      invalidExitCode == 0 && invalidReceipt &&
      iggy3d::smoke::productReceipt(invalidFields) &&
      invalidAsciiPreview(invalidFields);
  const bool terrainPreviewPassed =
      terrainPreviewExitCode == 0 && terrainPreviewReceipt &&
      iggy3d::smoke::productReceipt(terrainPreviewFields) &&
      terrainAsciiPreview(terrainPreviewFields);
  const bool activatePassed =
      activateExitCode == 0 && activateReceipt &&
      iggy3d::smoke::productReceipt(activateFields) &&
      activatedAsciiRoom(activateFields);
  const bool invalidActivatePassed =
      invalidActivateExitCode == 0 && invalidActivateReceipt &&
      iggy3d::smoke::productReceipt(invalidActivateFields) &&
      invalidAsciiActivation(invalidActivateFields);
  const bool openMovePassed =
      openMoveExitCode == 0 && openMoveReceipt &&
      iggy3d::smoke::productReceipt(openMoveFields) &&
      activatedAsciiRoomOpenMove(openMoveFields);
  const bool wallMovePassed =
      wallMoveExitCode == 0 && wallMoveReceipt &&
      iggy3d::smoke::productReceipt(wallMoveFields) &&
      activatedAsciiRoomWallMoveAccepted(wallMoveFields);
  const bool rampMovePassed =
      rampMoveExitCode == 0 && rampMoveReceipt &&
      iggy3d::smoke::productReceipt(rampMoveFields) &&
      activatedAsciiRoomRampMoveAccepted(rampMoveFields);
  const bool steepMovePassed =
      steepMoveExitCode == 0 && steepMoveReceipt &&
      iggy3d::smoke::productReceipt(steepMoveFields) &&
      activatedAsciiRoomSteepMoveRejected(steepMoveFields);
  const bool interactPassed =
      interactExitCode == 0 && interactReceipt &&
      iggy3d::smoke::productReceipt(interactFields) &&
      activatedAsciiRoomInteractKey(interactFields);
  const bool treasurePassed =
      treasureExitCode == 0 && treasureReceipt &&
      iggy3d::smoke::productReceipt(treasureFields) &&
      activatedAsciiRoomInteractTreasure(treasureFields);
  const bool doorPassed =
      doorExitCode == 0 && doorReceipt &&
      iggy3d::smoke::productReceipt(doorFields) &&
      activatedAsciiRoomInteractDoor(doorFields);
  const bool lockedSecretDoorPassed =
      lockedSecretDoorExitCode == 0 && lockedSecretDoorReceipt &&
      iggy3d::smoke::productReceipt(lockedSecretDoorFields) &&
      activatedAsciiRoomSecretDoorRequiresKey(lockedSecretDoorFields);

  const bool ok = expect(appAvailable, "app binary exists") &&
                  expect(validReceipt, "valid receipt parsed") &&
                  expect(validPassed, "valid ascii room preview") &&
                  expect(invalidReceipt, "invalid receipt parsed") &&
                  expect(invalidPassed, "invalid ascii room rejected") &&
                  expect(terrainPreviewReceipt,
                         "terrain preview receipt parsed") &&
                  expect(terrainPreviewPassed, "terrain preview ready") &&
                  expect(activateReceipt, "activation receipt parsed") &&
                  expect(activatePassed, "valid ascii room activated") &&
                  expect(invalidActivateReceipt,
                         "invalid activation receipt parsed") &&
                  expect(invalidActivatePassed,
                         "invalid ascii activation rejected") &&
                  expect(openMoveReceipt, "open move receipt parsed") &&
                  expect(openMovePassed, "open move accepted") &&
                  expect(wallMoveReceipt, "wall move receipt parsed") &&
                  expect(wallMovePassed, "wall move accepted") &&
                  expect(rampMoveReceipt, "ramp move receipt parsed") &&
                  expect(rampMovePassed, "ramp move accepted") &&
                  expect(steepMoveReceipt, "steep move receipt parsed") &&
                  expect(steepMovePassed, "steep move rejected") &&
                  expect(interactReceipt, "interact receipt parsed") &&
                  expect(interactPassed, "interact key accepted") &&
                  expect(treasureReceipt, "treasure receipt parsed") &&
                  expect(treasurePassed, "interact treasure accepted") &&
                  expect(doorReceipt, "door receipt parsed") &&
                  expect(doorPassed, "interact door accepted") &&
                  expect(lockedSecretDoorReceipt,
                         "locked secret door receipt parsed") &&
                  expect(lockedSecretDoorPassed,
                         "locked secret door requires key");

  std::cout << "smoke=product_ascii_authoring\n";
  std::cout << "valid_preview=" << (validPassed ? "true" : "false") << "\n";
  std::cout << "invalid_preview_rejected="
            << (invalidPassed ? "true" : "false") << "\n";
  std::cout << "terrain_preview=" << (terrainPreviewPassed ? "true" : "false")
            << "\n";
  std::cout << "activated_gameplay=" << (activatePassed ? "true" : "false")
            << "\n";
  std::cout << "interact_key=" << (interactPassed ? "true" : "false")
            << "\n";
  std::cout << "interact_treasure="
            << (treasurePassed ? "true" : "false") << "\n";
  std::cout << "interact_door=" << (doorPassed ? "true" : "false") << "\n";
  std::cout << "locked_secret_door_requires_key="
            << (lockedSecretDoorPassed ? "true" : "false") << "\n";
  std::cout << "invalid_activation_rejected="
            << (invalidActivatePassed ? "true" : "false") << "\n";
  std::cout << "open_move_accepted=" << (openMovePassed ? "true" : "false")
            << "\n";
  std::cout << "wall_move_accepted=" << (wallMovePassed ? "true" : "false")
            << "\n";
  std::cout << "ramp_move_accepted=" << (rampMovePassed ? "true" : "false")
            << "\n";
  std::cout << "steep_move_rejected=" << (steepMovePassed ? "true" : "false")
            << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result="
            << (ok ? "pass" : (appAvailable ? "fail" : "skip")) << "\n";
  std::cout << "reason_code="
            << (ok ? "product_ascii_authoring_pass"
                   : (appAvailable ? "product_ascii_authoring_failed"
                                   : "product_app_unavailable"))
            << "\n";
  if (ok) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}

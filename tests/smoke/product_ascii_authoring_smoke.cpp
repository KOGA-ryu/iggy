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
                                 "ascii_room_preview_static_mesh_count",
                                 "35") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_anchor_count",
                                 "5") &&
         iggy3d::smoke::hasField(fields,
                                 "ascii_room_preview_spatial_surface_count",
                                 "55") &&
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
                                 "ascii_room_activation_objective_count",
                                 "1") &&
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
                                 "35") &&
         iggy3d::smoke::hasField(fields, "active_room_anchor_count", "5") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_spatial_surface_count",
                                 "55") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_walkable_surface_count",
                                 "15") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_actor_blocker_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_projectile_blocker_count",
                                 "20") &&
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
                                 "55") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_query_surface_count",
                                 "55") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_walkable_surface_count",
                                 "15") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_actor_blocker_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_projectile_blocker_count",
                                 "20") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "runtime_session_created", "true") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields, "scene_item_count", "5") &&
         iggy3d::smoke::hasField(fields, "player_visible", "true") &&
         iggy3d::smoke::hasField(fields, "room_visible", "true") &&
         iggy3d::smoke::hasField(fields, "objective_visible", "true") &&
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
                                 "55") &&
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
                                 "1.000");
}

bool activatedAsciiRoomWallMoveBlocked(
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
                                 "55") &&
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
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_attempted",
                                 "true") &&
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
                                 "blocked_by_collision") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_blocked_reason",
                                 "blocked_by_collision") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_hit_surface_id",
                                 "wall_r0_c1_actor_blocker") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_ground_snap_applied",
                                 "false") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_clamped",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_collision_sweep_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_policy_band",
                                 "none");
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

  const bool validPassed =
      validExitCode == 0 && validReceipt &&
      iggy3d::smoke::productReceipt(validFields) &&
      validAsciiPreview(validFields);
  const bool invalidPassed =
      invalidExitCode == 0 && invalidReceipt &&
      iggy3d::smoke::productReceipt(invalidFields) &&
      invalidAsciiPreview(invalidFields);
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
      activatedAsciiRoomWallMoveBlocked(wallMoveFields);

  const bool ok = expect(appAvailable, "app binary exists") &&
                  expect(validReceipt, "valid receipt parsed") &&
                  expect(validPassed, "valid ascii room preview") &&
                  expect(invalidReceipt, "invalid receipt parsed") &&
                  expect(invalidPassed, "invalid ascii room rejected") &&
                  expect(activateReceipt, "activation receipt parsed") &&
                  expect(activatePassed, "valid ascii room activated") &&
                  expect(invalidActivateReceipt,
                         "invalid activation receipt parsed") &&
                  expect(invalidActivatePassed,
                         "invalid ascii activation rejected") &&
                  expect(openMoveReceipt, "open move receipt parsed") &&
                  expect(openMovePassed, "open move accepted") &&
                  expect(wallMoveReceipt, "wall move receipt parsed") &&
                  expect(wallMovePassed, "wall move blocked");

  std::cout << "smoke=product_ascii_authoring\n";
  std::cout << "valid_preview=" << (validPassed ? "true" : "false") << "\n";
  std::cout << "invalid_preview_rejected="
            << (invalidPassed ? "true" : "false") << "\n";
  std::cout << "activated_gameplay=" << (activatePassed ? "true" : "false")
            << "\n";
  std::cout << "invalid_activation_rejected="
            << (invalidActivatePassed ? "true" : "false") << "\n";
  std::cout << "open_move_accepted=" << (openMovePassed ? "true" : "false")
            << "\n";
  std::cout << "wall_move_blocked=" << (wallMovePassed ? "true" : "false")
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

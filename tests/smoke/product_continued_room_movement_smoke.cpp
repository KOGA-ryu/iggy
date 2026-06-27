#include <filesystem>
#include <iostream>

#include "ProductAutomationSmokeSupport.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return condition;
}

bool saveExitReceipt(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::productReceipt(fields) &&
         iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "product_save_source",
                                 "pause_save_and_exit") &&
         iggy3d::smoke::hasField(fields, "product_save_save_id", "save_001") &&
         iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
         iggy3d::smoke::hasField(fields, "room_editor_last_operation",
                                 "editor.place");
}

bool starterSeesSave(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::productReceipt(fields) &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "save_count", "1") &&
         iggy3d::smoke::hasField(fields, "compatible_save_count", "1");
}

bool continuedEditedRoomMovementBlocked(
    const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::productReceipt(fields) &&
         iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields, "product_save_load_status",
                                 "product_save_loaded") &&
         iggy3d::smoke::hasField(fields, "product_save_load_source",
                                 "continue") &&
         iggy3d::smoke::hasField(fields, "product_save_load_save_id",
                                 "save_001") &&
         iggy3d::smoke::hasField(fields, "active_room_source",
                                 "saved_authored_room") &&
         iggy3d::smoke::hasField(fields, "active_room_id",
                                 "custom_dungeon_draft") &&
         iggy3d::smoke::hasField(fields, "active_room_authored_floor_count",
                                 "58") &&
         iggy3d::smoke::hasField(fields, "active_room_authored_wall_count",
                                 "62") &&
         iggy3d::smoke::hasField(fields, "active_room_collision_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_spatial_surface_count",
                                 "182") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_query_surface_count",
                                 "182") &&
         iggy3d::smoke::hasField(fields, "gameplay_collision_surfaces_used",
                                 "true") &&
         iggy3d::smoke::hasField(fields, "gameplay_collision_surface_count",
                                 "182") &&
         iggy3d::smoke::hasField(fields, "automation_control_last_key",
                                 "game.move_x") &&
         iggy3d::smoke::hasField(fields, "input_action_last",
                                 "game.move_x") &&
         iggy3d::smoke::hasField(fields, "input_action_accepted", "true") &&
         iggy3d::smoke::hasField(fields, "gameplay_command_kind", "move") &&
         iggy3d::smoke::hasField(fields, "gameplay_command_accepted", "true") &&
         iggy3d::smoke::hasField(fields, "gameplay_tick_advanced", "true") &&
         iggy3d::smoke::hasField(fields, "gameplay_tick_reason_code", "ok") &&
         iggy3d::smoke::hasField(fields, "player_position_changed", "false") &&
         iggy3d::smoke::hasField(fields, "gameplay_movement_attempted",
                                 "true") &&
         iggy3d::smoke::hasField(fields, "gameplay_movement_blocked", "true") &&
         iggy3d::smoke::hasField(fields, "gameplay_movement_status",
                                 "blocked") &&
         iggy3d::smoke::hasField(fields, "gameplay_movement_debug_available",
                                 "true") &&
         iggy3d::smoke::hasField(fields, "gameplay_movement_reason_code",
                                 "blocked_by_collision") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_blocked_reason",
                                 "blocked_by_collision") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_collision_sweep_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields, "movement_debug_hud_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields, "movement_debug_hud_status",
                                 "blocked") &&
         iggy3d::smoke::hasField(fields, "movement_debug_hud_reason_code",
                                 "blocked_by_collision");
}

}  // namespace

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  if (!iggy3d::smoke::productAppAvailable(binary)) {
    std::cerr << "product app unavailable\n";
    return 77;
  }

  const std::filesystem::path saveRoot =
      iggy3d::smoke::cleanSaveRoot("continued_room_movement");
  const std::filesystem::path saveFile = saveRoot / "save_001.iggy3d.save";

  iggy3d::smoke::ReceiptFields fields;
  int exitCode = 77;
  const bool saved =
      iggy3d::smoke::runProductCase(
          binary,
          "continued_room_movement_save_exit",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "menu.next_tab=true\n"
          "menu.input=down\n"
          "menu.right=true\n"
          "world.draft_move=right\n"
          "world.draft_paint=#\n"
          "world.create=true\n"
          "system.pause=true\n"
          "menu.down=true\n"
          "pause.execute=true\n"
          "editor.input=editor.nudge_x_pos,editor.next_tool,editor.place\n"
          "menu.back=true\n"
          "pause.select=save_and_exit\n"
          "menu.confirm=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && saveExitReceipt(fields) && std::filesystem::exists(saveFile);

  fields.clear();
  const bool rebootStarter =
      saved &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "continued_room_movement_reboot_starter",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && starterSeesSave(fields);

  fields.clear();
  const bool continuedMove =
      rebootStarter &&
      iggy3d::smoke::runProductCase(
          binary,
          "continued_room_movement_continue_and_move",
          "frontend.select=continue\nfrontend.execute=true\n"
          "game.move_x=1\n",
          iggy3d::smoke::saveRootArg(saveRoot) + " --debug-overlay",
          fields,
          exitCode) &&
      exitCode == 0 && continuedEditedRoomMovementBlocked(fields);

  const bool ok =
      expect(saved, "editor input save and exit") &&
      expect(rebootStarter, "fresh starter sees edited save") &&
      expect(continuedMove, "continued edited room blocks move via restored collision");

  std::cout << "continued_room_movement_save_exit="
            << (saved ? "true" : "false") << '\n';
  std::cout << "continued_room_movement_reboot_starter="
            << (rebootStarter ? "true" : "false") << '\n';
  std::cout << "continued_room_movement_blocked_by_restored_collision="
            << (continuedMove ? "true" : "false") << '\n';
  std::cout << "result=" << (ok ? "pass" : "fail") << '\n';

  return ok ? 0 : 1;
}

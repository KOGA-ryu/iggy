#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "ProductAutomationSmokeSupport.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return condition;
}

bool fileContains(const std::filesystem::path& path, const std::string& text) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return false;
  }
  const std::string bytes((std::istreambuf_iterator<char>(input)),
                          std::istreambuf_iterator<char>());
  return bytes.find(text) != std::string::npos;
}

bool saveExitWithCombinedEdits(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::productReceipt(fields) &&
         iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "product_save_source",
                                 "pause_save_and_exit") &&
         iggy3d::smoke::hasField(fields, "product_save_save_id", "save_001") &&
         iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
         iggy3d::smoke::hasField(fields, "room_editing_authored_floor_count",
                                 "59") &&
         iggy3d::smoke::hasField(fields, "room_editing_authored_wall_count",
                                 "62") &&
         iggy3d::smoke::hasField(fields, "active_room_authored_floor_count",
                                 "59") &&
         iggy3d::smoke::hasField(fields, "active_room_authored_wall_count",
                                 "62") &&
         iggy3d::smoke::hasField(fields, "room_editor_last_operation",
                                 "editor.place") &&
         iggy3d::smoke::hasField(fields, "room_editor_last_operation_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields, "room_editor_last_primitive_id",
                                 "edit_floor_1") &&
         iggy3d::smoke::hasField(fields, "room_editor_hud_tool", "floor") &&
         iggy3d::smoke::hasField(fields, "room_editor_hud_last_operation",
                                 "editor.place") &&
         iggy3d::smoke::hasField(fields,
                                 "room_editor_hud_last_operation_accepted",
                                 "true") &&
         iggy3d::smoke::hasField(fields, "room_editor_hud_last_primitive_id",
                                 "edit_floor_1");
}

bool starterSeesSave(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::productReceipt(fields) &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "save_count", "1") &&
         iggy3d::smoke::hasField(fields, "compatible_save_count", "1");
}

bool continueRestoresCombinedEdits(const iggy3d::smoke::ReceiptFields& fields) {
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
         iggy3d::smoke::hasField(fields, "product_save_load_authored_floor_count",
                                 "59") &&
         iggy3d::smoke::hasField(fields, "product_save_load_authored_wall_count",
                                 "62") &&
         iggy3d::smoke::hasField(fields, "active_room_source",
                                 "saved_authored_room") &&
         iggy3d::smoke::hasField(fields, "active_room_id",
                                 "custom_dungeon_draft") &&
         iggy3d::smoke::hasField(fields, "active_room_authored_floor_count",
                                 "59") &&
         iggy3d::smoke::hasField(fields, "active_room_authored_wall_count",
                                 "62") &&
         iggy3d::smoke::hasField(fields, "active_room_collision_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_spatial_surface_count",
                                 "183") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_query_surface_count",
                                 "183") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_walkable_surface_count",
                                 "59") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_actor_blocker_count",
                                 "62") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_projectile_blocker_count",
                                 "62") &&
         iggy3d::smoke::hasField(fields, "product_draw_floor_tile_count",
                                 "59") &&
         iggy3d::smoke::hasField(fields, "product_draw_wall_tile_count",
                                 "62") &&
         iggy3d::smoke::hasField(fields, "product_vulkan_room_mesh_cpu_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields, "product_vulkan_room_mesh_source",
                                 "scene_room_projection") &&
         iggy3d::smoke::hasField(fields, "product_vulkan_room_asset_id",
                                 "custom_dungeon_draft") &&
         iggy3d::smoke::hasField(fields, "product_vulkan_room_floor_draw_count",
                                 "12") &&
         iggy3d::smoke::hasField(fields, "product_vulkan_room_wall_draw_count",
                                 "22") &&
         iggy3d::smoke::positiveIntegerField(
             fields, "product_vulkan_room_geometry_signature");
}

}  // namespace

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  if (!iggy3d::smoke::productAppAvailable(binary)) {
    std::cerr << "product app unavailable\n";
    return 77;
  }

  const std::filesystem::path saveRoot =
      iggy3d::smoke::cleanSaveRoot("editor_combined_save_continue");
  const std::filesystem::path saveFile = saveRoot / "save_001.iggy3d.save";

  iggy3d::smoke::ReceiptFields fields;
  int exitCode = 77;
  const bool saved =
      iggy3d::smoke::runProductCase(
          binary,
          "editor_combined_save_continue_save_exit",
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
          "editor.input=editor.nudge_x_pos,editor.select_wall_tool,editor.place,editor.select_floor_tool,editor.place\n"
          "menu.back=true\n"
          "pause.select=save_and_exit\n"
          "menu.confirm=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && saveExitWithCombinedEdits(fields) &&
      std::filesystem::exists(saveFile) &&
      fileContains(saveFile, "authoredRoom.floor.count=59\n") &&
      fileContains(saveFile, "authoredRoom.floor.58.id=edit_floor_1\n") &&
      fileContains(saveFile, "authoredRoom.wall.count=62\n") &&
      fileContains(saveFile, "authoredRoom.wall.61.id=edit_wall_1\n");

  fields.clear();
  const bool rebootStarter =
      saved &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "editor_combined_save_continue_reboot_starter",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && starterSeesSave(fields);

  fields.clear();
  const bool continued =
      rebootStarter &&
      iggy3d::smoke::runProductCase(
          binary,
          "editor_combined_save_continue_continue",
          "frontend.select=continue\nfrontend.execute=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && continueRestoresCombinedEdits(fields);

  const bool ok =
      expect(saved, "editor input combined save and exit") &&
      expect(rebootStarter, "fresh starter sees combined edited save") &&
      expect(continued, "continue restores combined edited room");

  std::cout << "editor_combined_save_continue_save_exit="
            << (saved ? "true" : "false") << '\n';
  std::cout << "editor_combined_save_continue_reboot_starter="
            << (rebootStarter ? "true" : "false") << '\n';
  std::cout << "editor_combined_save_continue_restored="
            << (continued ? "true" : "false") << '\n';
  std::cout << "result=" << (ok ? "pass" : "fail") << '\n';

  return ok ? 0 : 1;
}

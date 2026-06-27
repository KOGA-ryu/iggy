#include <cmath>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "runtime/save/SaveCodec.hpp"
#include "runtime/save/SaveFileStore.hpp"
#include "AutomationSmokeSupport.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 0.001F;
}

iggy3d::SaveDecodeResult decodeSavedRoom(const std::filesystem::path& saveFile) {
  const iggy3d::SaveFileReadResult read = iggy3d::readSaveFile(saveFile);
  if (!read.ok) {
    return {};
  }
  return iggy3d::decodeSaveEnvelope(read.encodedText);
}

const iggy3d::SaveAuthoredRoomWallRecord* findWallById(
    const iggy3d::SaveAuthoredRoomSection& room,
    std::string_view id) {
  for (const iggy3d::SaveAuthoredRoomWallRecord& wall : room.walls) {
    if (wall.id == id) {
      return &wall;
    }
  }
  return nullptr;
}

bool upWallAtEditedCell(const iggy3d::SaveAuthoredRoomWallRecord& wall) {
  return near(wall.startMeters.x, 0.5F) && near(wall.startMeters.y, 0.0F) &&
         near(wall.startMeters.z, -0.5F) && near(wall.endMeters.x, 1.5F) &&
         near(wall.endMeters.y, 0.0F) && near(wall.endMeters.z, -0.5F);
}

bool rightWallAtEditedCell(const iggy3d::SaveAuthoredRoomWallRecord& wall) {
  return near(wall.startMeters.x, 1.5F) && near(wall.startMeters.y, 0.0F) &&
         near(wall.startMeters.z, -0.5F) && near(wall.endMeters.x, 1.5F) &&
         near(wall.endMeters.y, 0.0F) && near(wall.endMeters.z, 0.5F);
}

bool saveExitWithRotatedWall(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::productReceipt(fields) &&
         iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "product_save_source",
                                 "pause_save_and_exit") &&
         iggy3d::smoke::hasField(fields, "product_save_save_id", "save_001") &&
         iggy3d::smoke::hasField(fields, "room_editing_authored_floor_count",
                                 "58") &&
         iggy3d::smoke::hasField(fields, "room_editing_authored_wall_count",
                                 "63") &&
         iggy3d::smoke::hasField(fields, "room_editor_tool", "wall") &&
         iggy3d::smoke::hasField(fields, "room_editor_wall_direction", "right") &&
         iggy3d::smoke::hasField(fields, "room_editor_last_operation",
                                 "editor.place") &&
         iggy3d::smoke::hasField(fields, "room_editor_last_primitive_id",
                                 "edit_wall_2") &&
         iggy3d::smoke::hasField(fields, "room_editor_hud_tool", "wall") &&
         iggy3d::smoke::hasField(fields, "room_editor_hud_wall_direction",
                                 "right") &&
         iggy3d::smoke::hasField(fields, "room_editor_hud_last_primitive_id",
                                 "edit_wall_2");
}

bool starterSeesSave(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::productReceipt(fields) &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
         iggy3d::smoke::hasField(fields, "save_count", "1") &&
         iggy3d::smoke::hasField(fields, "compatible_save_count", "1");
}

bool continueRestoresRotatedWalls(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::productReceipt(fields) &&
         iggy3d::smoke::automationApplied(fields) &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields, "product_save_load_status",
                                 "product_save_loaded") &&
         iggy3d::smoke::hasField(fields, "product_save_load_source",
                                 "continue") &&
         iggy3d::smoke::hasField(fields, "product_save_load_authored_floor_count",
                                 "58") &&
         iggy3d::smoke::hasField(fields, "product_save_load_authored_wall_count",
                                 "63") &&
         iggy3d::smoke::hasField(fields, "active_room_source",
                                 "saved_authored_room") &&
         iggy3d::smoke::hasField(fields, "active_room_id",
                                 "custom_dungeon_draft") &&
         iggy3d::smoke::hasField(fields, "active_room_authored_floor_count",
                                 "58") &&
         iggy3d::smoke::hasField(fields, "active_room_authored_wall_count",
                                 "63") &&
         iggy3d::smoke::hasField(fields, "active_room_collision_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_walkable_surface_count",
                                 "58") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_actor_blocker_count",
                                 "63") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_projectile_blocker_count",
                                 "63") &&
         iggy3d::smoke::hasField(fields, "product_draw_floor_tile_count",
                                 "58") &&
         iggy3d::smoke::hasField(fields, "product_draw_wall_tile_count",
                                 "63") &&
         iggy3d::smoke::hasField(fields, "product_vulkan_room_mesh_cpu_ready",
                                 "true") &&
         iggy3d::smoke::hasField(fields, "product_vulkan_room_mesh_source",
                                 "scene_room_projection") &&
         iggy3d::smoke::positiveIntegerField(
             fields, "product_vulkan_room_wall_draw_count") &&
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
      iggy3d::smoke::cleanSaveRoot("editor_wall_direction_hotkey");
  const std::filesystem::path saveFile = saveRoot / "save_001.iggy3d.save";

  iggy3d::smoke::ReceiptFields fields;
  int exitCode = 77;
  const bool saved =
      iggy3d::smoke::runProductCase(
          binary,
          "editor_wall_direction_hotkey_save_exit",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "world.draft_cell=1,2,#\n"
          "world.create=true\n"
          "system.pause=true\n"
          "menu.down=true\n"
          "pause.execute=true\n"
          "editor.input=editor.nudge_x_pos,editor.select_wall_tool,editor.place,editor.rotate_wall_direction,editor.place\n"
          "menu.back=true\n"
          "pause.select=save_and_exit\n"
          "menu.confirm=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && saveExitWithRotatedWall(fields) &&
      std::filesystem::exists(saveFile);

  const iggy3d::SaveDecodeResult decoded = decodeSavedRoom(saveFile);
  const iggy3d::SaveAuthoredRoomWallRecord* upWall =
      decoded.status == iggy3d::SaveCodecStatus::Ok
          ? findWallById(decoded.envelope.authoredRoom, "edit_wall_1")
          : nullptr;
  const iggy3d::SaveAuthoredRoomWallRecord* rightWall =
      decoded.status == iggy3d::SaveCodecStatus::Ok
          ? findWallById(decoded.envelope.authoredRoom, "edit_wall_2")
          : nullptr;
  const bool savedGeometry =
      saved && decoded.status == iggy3d::SaveCodecStatus::Ok &&
      decoded.envelope.authoredRoom.present &&
      decoded.envelope.authoredRoom.id == "custom_dungeon_draft" &&
      decoded.envelope.authoredRoom.floors.size() == 58U &&
      decoded.envelope.authoredRoom.walls.size() == 63U && upWall != nullptr &&
      rightWall != nullptr && upWallAtEditedCell(*upWall) &&
      rightWallAtEditedCell(*rightWall);

  fields.clear();
  const bool rebootStarter =
      savedGeometry &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "editor_wall_direction_hotkey_reboot_starter",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && starterSeesSave(fields);

  fields.clear();
  const bool continued =
      rebootStarter &&
      iggy3d::smoke::runProductCase(
          binary,
          "editor_wall_direction_hotkey_continue",
          "frontend.select=continue\nfrontend.execute=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && continueRestoresRotatedWalls(fields);

  const bool ok = expect(saved, "editor rotated wall save and exit") &&
                  expect(savedGeometry, "saved wall orientations differ") &&
                  expect(rebootStarter, "fresh starter sees rotated wall save") &&
                  expect(continued, "continue restores rotated walls");

  std::cout << "editor_wall_direction_hotkey_save_exit="
            << (saved ? "true" : "false") << '\n';
  std::cout << "editor_wall_direction_hotkey_saved_geometry="
            << (savedGeometry ? "true" : "false") << '\n';
  std::cout << "editor_wall_direction_hotkey_reboot_starter="
            << (rebootStarter ? "true" : "false") << '\n';
  std::cout << "editor_wall_direction_hotkey_restored="
            << (continued ? "true" : "false") << '\n';
  std::cout << "result=" << (ok ? "pass" : "fail") << '\n';

  return ok ? 0 : 1;
}

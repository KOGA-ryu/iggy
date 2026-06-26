#include "ProductAutomationSmokeSupport.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view kMapPath =
    "fixtures/rooms/ascii/loop_keep.iggyroom.txt";

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return {};
  }
  return std::string((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
}

std::string automationTextValue(std::string_view text) {
  std::string out;
  for (const char ch : text) {
    if (ch == '\n') {
      out += "\\n";
    } else if (ch != '\r') {
      out.push_back(ch);
    }
  }
  return out;
}

bool fileContains(const std::filesystem::path& path, std::string_view needle) {
  std::ifstream input(path);
  if (!input) {
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(input)),
                         std::istreambuf_iterator<char>());
  return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);
  const std::filesystem::path mapPath{kMapPath};
  const bool mapAvailable = std::filesystem::exists(mapPath);
  const std::string mapText = mapAvailable ? readTextFile(mapPath) : std::string{};
  const std::filesystem::path defaultSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_loop_keep_default");
  const std::filesystem::path selectedSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_gatehouse_selected");
  const std::filesystem::path customSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_custom_draft");
  const std::filesystem::path cursorPaintRejectedSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_cursor_paint_rejected");
  const std::filesystem::path customCursorPaintSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_custom_draft_cursor_paint");
  const std::filesystem::path customEditSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_custom_draft_edit_active");
  const std::filesystem::path customPauseEditSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_custom_draft_pause_edit_room");
  const std::filesystem::path customCursorEditorPlaceSaveRoot =
      iggy3d::smoke::cleanSaveRoot(
          "ascii_map_custom_draft_pause_editor_cursor_place_wall");
  const std::filesystem::path customEditorInputPlaceSaveRoot =
      iggy3d::smoke::cleanSaveRoot(
          "ascii_map_custom_draft_editor_input_place_wall");
  const std::filesystem::path customCursorEditSaveExitRoot =
      iggy3d::smoke::cleanSaveRoot(
          "ascii_map_custom_draft_cursor_edit_save_exit");
  const std::filesystem::path customLiveEditSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_custom_draft_live_edit");
  const std::filesystem::path customLiveEditPauseSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_custom_draft_live_edit_save");
  const std::filesystem::path customLiveEditSaveExitRoot =
      iggy3d::smoke::cleanSaveRoot(
          "ascii_map_custom_draft_live_edit_save_exit");
  const std::filesystem::path saveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_loop_keep");

  int exitCode = 77;
  iggy3d::smoke::ReceiptFields fields;
  const bool createDefaultDungeonWorld =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_loop_keep_default_create",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.create=true\n",
          iggy3d::smoke::saveRootArg(defaultSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "world_setup_title", "Loop Keep") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_enabled",
                              "true") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_id",
                              "loop_keep_ascii") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_source_name",
                              kMapPath) &&
      iggy3d::smoke::hasField(fields, "world_creation_status",
                              "world_creation_initial_save_written") &&
      iggy3d::smoke::hasField(fields, "world_creation_world_title",
                              "Loop Keep") &&
      iggy3d::smoke::hasField(fields, "world_creation_ascii_room_requested",
                              "true") &&
      iggy3d::smoke::hasField(fields, "world_creation_ascii_room_id",
                              "loop_keep_ascii") &&
      iggy3d::smoke::hasField(fields, "world_creation_ascii_room_source_name",
                              kMapPath) &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_title",
                              "Loop Keep") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_status",
                              "product_ascii_room_ready") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_room_id",
                              "loop_keep_ascii") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_width", "17") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_height", "7") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_floor_count", "59") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_wall_count", "60") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_marker_count", "5") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "ascii_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id", "loop_keep_ascii") &&
      iggy3d::smoke::hasField(fields, "active_room_authored_floor_count",
                              "59") &&
      iggy3d::smoke::hasField(fields, "active_room_authored_wall_count",
                              "60") &&
      std::filesystem::exists(defaultSaveRoot / "save_001.iggy3d.save") &&
      fileContains(defaultSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.id=loop_keep_ascii\n") &&
      fileContains(defaultSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.floor.count=59\n") &&
      fileContains(defaultSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.wall.count=60\n");

  fields.clear();
  const bool createSelectedDungeonWorld =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_gatehouse_selected_create",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "menu.down=true\n"
          "world.create=true\n",
          iggy3d::smoke::saveRootArg(selectedSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "world_setup_title", "Gatehouse") &&
      iggy3d::smoke::hasField(fields, "world_setup_status",
                              "world_setup_create_requested") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_enabled",
                              "true") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_id",
                              "gatehouse_ascii") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_source_name",
                              "fixtures/rooms/ascii/gatehouse.iggyroom.txt") &&
      iggy3d::smoke::hasField(fields, "world_creation_status",
                              "world_creation_initial_save_written") &&
      iggy3d::smoke::hasField(fields, "world_creation_world_title",
                              "Gatehouse") &&
      iggy3d::smoke::hasField(fields, "world_creation_ascii_room_requested",
                              "true") &&
      iggy3d::smoke::hasField(fields, "world_creation_ascii_room_id",
                              "gatehouse_ascii") &&
      iggy3d::smoke::hasField(fields, "world_creation_ascii_room_source_name",
                              "fixtures/rooms/ascii/gatehouse.iggyroom.txt") &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_title",
                              "Gatehouse") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_status",
                              "product_ascii_room_ready") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_room_id",
                              "gatehouse_ascii") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "ascii_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id", "gatehouse_ascii") &&
      std::filesystem::exists(selectedSaveRoot / "save_001.iggy3d.save") &&
      fileContains(selectedSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.id=gatehouse_ascii\n") &&
      fileContains(selectedSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.sourceFile=fixtures/rooms/ascii/gatehouse.iggyroom.txt\n");

  fields.clear();
  const bool createCustomDraftWorld =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_create",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "world.draft_cell=1,2,#\n"
          "world.create=true\n",
          iggy3d::smoke::saveRootArg(customSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "world_setup_title", "Custom Draft") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_source_name",
                              "custom_dungeon_draft.iggyroom.txt") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_modified",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_status",
                              "dungeon_draft_cell_painted") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_reason_code",
                              "dungeon_draft_cell_painted") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_cursor_row",
                              "1") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_cursor_column",
                              "2") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_last_glyph",
                              "#") &&
      iggy3d::smoke::hasField(fields, "world_creation_world_title",
                              "Custom Draft") &&
      iggy3d::smoke::hasField(fields, "world_creation_ascii_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "world_creation_ascii_room_source_name",
                              "custom_dungeon_draft.iggyroom.txt") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_status",
                              "product_ascii_room_ready") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_wall_count", "61") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "ascii_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id", "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields, "active_room_authored_wall_count",
                              "61") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_cpu_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_backend_presented",
                              "false") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_source",
                              "scene_room_projection") &&
      iggy3d::smoke::hasField(fields, "product_vulkan_room_asset_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_floor_visible",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_wall_visible",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_grid_visible",
                              "true") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_vertex_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_index_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_draw_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_floor_draw_count") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_wall_draw_count",
                              "21") &&
      iggy3d::smoke::positiveIntegerField(
          fields, "product_vulkan_room_grid_line_draw_count") &&
      iggy3d::smoke::positiveIntegerField(
          fields, "product_vulkan_room_geometry_signature") &&
      std::filesystem::exists(customSaveRoot / "save_001.iggy3d.save") &&
      fileContains(customSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.id=custom_dungeon_draft\n") &&
      fileContains(customSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.sourceFile=custom_dungeon_draft.iggyroom.txt\n") &&
      fileContains(customSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.wall.count=61\n");

  fields.clear();
  const bool cursorPaintRequiresEditMode =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_cursor_paint_requires_edit_mode",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.draft_paint=#\n",
          iggy3d::smoke::saveRootArg(cursorPaintRejectedSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationCommandFailed(fields, "world.draft_paint") &&
      iggy3d::smoke::hasField(fields, "automation_control_last_action",
                              "world.draft_paint") &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "new_world") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_edit_mode",
                              "false") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_modified",
                              "false") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_status",
                              "dungeon_draft_edit_mode_off") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_reason_code",
                              "dungeon_draft_edit_mode_off") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_last_glyph",
                              "none") &&
      iggy3d::smoke::hasField(fields, "world_creation_status",
                              "not_requested") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "false");

  fields.clear();
  const bool createCursorPaintDraftWorld =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_cursor_paint_create",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Cursor Draft\n"
          "menu.next_tab=true\n"
          "menu.input=down\n"
          "menu.right=true\n"
          "world.draft_move=right\n"
          "world.draft_paint=#\n"
          "world.create=true\n",
          iggy3d::smoke::saveRootArg(customCursorPaintSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "world_setup_title", "Cursor Draft") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_edit_mode",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_modified",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_cursor_row",
                              "1") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_cursor_column",
                              "2") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_status",
                              "dungeon_draft_cell_painted") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_reason_code",
                              "dungeon_draft_cell_painted") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_dungeon_draft_last_glyph",
                              "#") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_source_name",
                              "custom_dungeon_draft.iggyroom.txt") &&
      iggy3d::smoke::hasField(fields, "world_creation_status",
                              "world_creation_initial_save_written") &&
      iggy3d::smoke::hasField(fields, "world_creation_world_title",
                              "Cursor Draft") &&
      iggy3d::smoke::hasField(fields, "world_creation_ascii_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "world_creation_ascii_room_source_name",
                              "custom_dungeon_draft.iggyroom.txt") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_status",
                              "product_ascii_room_ready") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_wall_count", "61") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "ascii_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_wall_count",
                              "61") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_cpu_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields, "product_vulkan_room_asset_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_wall_draw_count",
                              "21") &&
      std::filesystem::exists(customCursorPaintSaveRoot /
                              "save_001.iggy3d.save") &&
      fileContains(customCursorPaintSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.id=custom_dungeon_draft\n") &&
      fileContains(customCursorPaintSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.floor.count=58\n") &&
      fileContains(customCursorPaintSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.wall.count=61\n");

  fields.clear();
  const bool startCustomDraftActiveRoomEditing =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_start_active_editing",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "world.draft_cell=1,2,#\n"
          "world.create=true\n"
          "room_edit.start_active=true\n",
          iggy3d::smoke::saveRootArg(customEditSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields,
                              "automation_control_last_key",
                              "room_edit.start_active") &&
      iggy3d::smoke::hasField(fields,
                              "automation_control_last_action",
                              "room_edit.start_active") &&
      iggy3d::smoke::hasField(fields, "room_editing_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_status",
                              "product_room_editing_ready") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_reason_code",
                              "product_room_editing_ready") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation",
                              "room_edit.start_active") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_status",
                              "product_room_editing_started_from_active_room") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_reason_code",
                              "product_room_editing_started_from_active_room") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_accepted",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_active_room_loaded",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_authored_wall_count",
                              "61") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_active_room_static_mesh_count",
                              "119") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_active_room_spatial_surface_count",
                              "180") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_surface_count",
                              "180") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_walkable_surface_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_actor_blocker_count",
                              "61") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_projectile_blocker_count",
                              "61") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "editable_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id", "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_wall_count",
                              "61") &&
      iggy3d::smoke::hasField(fields, "active_room_collision_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_query_surface_count",
                              "180") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_walkable_surface_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "61");

  fields.clear();
  const bool pauseEditCustomDraftActiveRoom =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_pause_edit_room",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "world.draft_cell=1,2,#\n"
          "world.create=true\n"
          "system.pause=true\n"
          "pause.select=edit_room\n"
          "pause.execute=true\n",
          iggy3d::smoke::saveRootArg(customPauseEditSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action",
                              "edit_room") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "input_owner", "editor") &&
      iggy3d::smoke::hasField(fields, "gameplay_input_suppressed",
                              "true") &&
      iggy3d::smoke::hasField(fields, "pause_menu_open", "false") &&
      iggy3d::smoke::hasField(fields, "room_editing_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_status",
                              "product_room_editing_ready") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_reason_code",
                              "product_room_editing_ready") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation",
                              "pause_edit_room") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_status",
                              "product_room_editing_started_from_active_room") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_reason_code",
                              "product_room_editing_started_from_active_room") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_accepted",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_active_room_loaded",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_authored_wall_count",
                              "61") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_surface_count",
                              "180") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_walkable_surface_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_actor_blocker_count",
                              "61") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_projectile_blocker_count",
                              "61") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "editable_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_wall_count",
                              "61") &&
      iggy3d::smoke::hasField(fields, "active_room_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_query_surface_count",
                              "180") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_walkable_surface_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "61") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_projectile_blocker_count",
                              "61");

  fields.clear();
  const bool pauseEditorCursorPlaceWall =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_pause_editor_cursor_place_wall",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "world.draft_cell=1,2,#\n"
          "world.create=true\n"
          "system.pause=true\n"
          "pause.select=edit_room\n"
          "pause.execute=true\n"
          "room_editor.move=right\n"
          "room_editor.tool=wall\n"
          "room_editor.wall_direction=up\n"
          "room_editor.place=true\n",
          iggy3d::smoke::saveRootArg(customCursorEditorPlaceSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "input_owner", "editor") &&
      iggy3d::smoke::hasField(fields, "gameplay_input_suppressed",
                              "true") &&
      iggy3d::smoke::hasField(fields, "automation_control_last_key",
                              "room_editor.place") &&
      iggy3d::smoke::hasField(fields, "automation_control_last_action",
                              "room_editor.place") &&
      iggy3d::smoke::hasField(fields, "room_editing_ready", "true") &&
      iggy3d::smoke::hasField(fields, "room_editing_last_operation",
                              "room_editor.place") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_status",
                              "product_room_editing_edit_applied") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_accepted",
                              "true") &&
      iggy3d::smoke::hasField(fields, "room_editor_cursor_ready", "true") &&
      iggy3d::smoke::hasField(fields, "room_editor_grid_x", "1") &&
      iggy3d::smoke::hasField(fields, "room_editor_grid_z", "0") &&
      iggy3d::smoke::hasField(fields, "room_editor_story_index", "0") &&
      iggy3d::smoke::hasField(fields, "room_editor_cell_size_meters",
                              "1.000") &&
      iggy3d::smoke::hasField(fields, "room_editor_tool", "wall") &&
      iggy3d::smoke::hasField(fields, "room_editor_wall_direction", "up") &&
      iggy3d::smoke::hasField(fields, "room_editor_status",
                              "room_editor_command_applied") &&
      iggy3d::smoke::hasField(fields, "room_editor_reason_code",
                              "room_editor_command_applied") &&
      iggy3d::smoke::hasField(fields, "room_editor_last_operation",
                              "room_editor.place") &&
      iggy3d::smoke::hasField(fields,
                              "room_editor_last_operation_accepted",
                              "true") &&
      iggy3d::smoke::hasField(fields, "room_editor_last_primitive_id",
                              "edit_wall_1") &&
      iggy3d::smoke::hasField(fields, "room_editor_overlay_visible",
                              "true") &&
      iggy3d::smoke::hasField(fields, "room_editor_overlay_status",
                              "room_editor_overlay_ready") &&
      iggy3d::smoke::hasField(fields, "room_editor_overlay_reason_code",
                              "room_editor_overlay_ready") &&
      iggy3d::smoke::hasField(fields, "room_editor_overlay_item_count",
                              "1") &&
      iggy3d::smoke::hasField(fields, "room_editor_overlay_world_x",
                              "1.000") &&
      iggy3d::smoke::hasField(fields, "room_editor_overlay_world_y",
                              "0.150") &&
      iggy3d::smoke::hasField(fields, "room_editor_overlay_world_z",
                              "-0.500") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "editable_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_room_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_query_surface_count",
                              "182") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_walkable_surface_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_projectile_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "product_draw_room_editor_cursor_visible",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_draw_room_editor_cursor_count",
                              "1") &&
      iggy3d::smoke::hasField(fields, "product_render_bridge_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields, "product_view_frame_ready", "true") &&
      iggy3d::smoke::positiveIntegerField(
          fields, "product_view_frame_item_count") &&
      iggy3d::smoke::hasField(
          fields, "product_render_bridge_room_editor_cursor_visible",
          "true") &&
      iggy3d::smoke::hasField(
          fields, "product_render_bridge_room_editor_cursor_count", "1") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_cpu_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_source",
                              "scene_room_projection") &&
      iggy3d::smoke::hasField(fields, "product_vulkan_room_asset_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_wall_draw_count",
                              "62") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_vertex_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_index_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_draw_count") &&
      iggy3d::smoke::positiveIntegerField(
          fields, "product_vulkan_room_geometry_signature");

  fields.clear();
  const bool editorInputCursorPlaceWall =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_editor_input_place_wall",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "world.draft_cell=1,2,#\n"
          "world.create=true\n"
          "system.pause=true\n"
          "pause.select=edit_room\n"
          "pause.execute=true\n"
          "editor.input=editor.nudge_x_pos,editor.next_tool,editor.place\n",
          iggy3d::smoke::saveRootArg(customEditorInputPlaceSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "input_owner", "editor") &&
      iggy3d::smoke::hasField(fields, "gameplay_input_suppressed", "true") &&
      iggy3d::smoke::hasField(fields, "input_action_last",
                              "editor.place") &&
      iggy3d::smoke::hasField(fields, "automation_control_last_key",
                              "editor.input") &&
      iggy3d::smoke::hasField(fields, "automation_control_last_action",
                              "editor.place") &&
      iggy3d::smoke::hasField(fields, "room_editing_ready", "true") &&
      iggy3d::smoke::hasField(fields, "room_editor_cursor_ready", "true") &&
      iggy3d::smoke::hasField(fields, "room_editor_grid_x", "1") &&
      iggy3d::smoke::hasField(fields, "room_editor_grid_z", "0") &&
      iggy3d::smoke::hasField(fields, "room_editor_tool", "wall") &&
      iggy3d::smoke::hasField(fields, "room_editor_status",
                              "room_editor_command_applied") &&
      iggy3d::smoke::hasField(fields, "room_editor_last_operation",
                              "editor.place") &&
      iggy3d::smoke::hasField(fields,
                              "room_editor_last_operation_accepted",
                              "true") &&
      iggy3d::smoke::hasField(fields, "room_editor_last_primitive_id",
                              "edit_wall_1") &&
      iggy3d::smoke::hasField(fields, "room_editor_overlay_visible",
                              "true") &&
      iggy3d::smoke::hasField(fields, "room_editor_overlay_item_count",
                              "1") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "editable_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_room_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_projectile_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "product_draw_room_editor_cursor_visible",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_draw_room_editor_cursor_count",
                              "1") &&
      iggy3d::smoke::hasField(
          fields, "product_render_bridge_room_editor_cursor_visible",
          "true") &&
      iggy3d::smoke::hasField(
          fields, "product_render_bridge_room_editor_cursor_count", "1") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_cpu_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_wall_draw_count",
                              "62") &&
      iggy3d::smoke::positiveIntegerField(
          fields, "product_vulkan_room_geometry_signature");

  fields.clear();
  const std::filesystem::path customCursorEditSaveExitFile =
      customCursorEditSaveExitRoot / "save_001.iggy3d.save";
  const bool saveAndExitCursorEditedCustomDraftActiveRoom =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_cursor_edit_save_exit",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "world.draft_cell=1,2,#\n"
          "world.create=true\n"
          "system.pause=true\n"
          "menu.down=true\n"
          "pause.execute=true\n"
          "room_editor.move=right\n"
          "room_editor.tool=wall\n"
          "room_editor.wall_direction=up\n"
          "room_editor.place=true\n"
          "menu.back=true\n"
          "pause.select=save_and_exit\n"
          "menu.confirm=true\n",
          iggy3d::smoke::saveRootArg(customCursorEditSaveExitRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "launch_status",
                              "pause_save_and_exit_written") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "input_owner", "starter") &&
      iggy3d::smoke::hasField(fields, "room_editor_status",
                              "room_editor_command_applied") &&
      iggy3d::smoke::hasField(fields, "room_editor_last_operation",
                              "room_editor.place") &&
      iggy3d::smoke::hasField(fields,
                              "room_editor_last_operation_accepted",
                              "true") &&
      iggy3d::smoke::hasField(fields, "room_editor_last_primitive_id",
                              "edit_wall_1") &&
      iggy3d::smoke::hasField(fields, "product_save_status",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_reason_code",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_durable_reason",
                              "durable_save_file_written") &&
      iggy3d::smoke::hasField(fields, "product_save_source",
                              "pause_save_and_exit") &&
      iggy3d::smoke::hasField(fields, "product_save_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_session_saved", "true") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_transition_returned_to_title",
                              "true") &&
      std::filesystem::exists(customCursorEditSaveExitFile) &&
      fileContains(customCursorEditSaveExitFile,
                   "authoredRoom.id=custom_dungeon_draft\n") &&
      fileContains(customCursorEditSaveExitFile,
                   "authoredRoom.floor.count=58\n") &&
      fileContains(customCursorEditSaveExitFile,
                   "authoredRoom.wall.count=62\n") &&
      fileContains(customCursorEditSaveExitFile, "edit_wall_1\n");

  fields.clear();
  const bool rebootCursorEditedCustomDraftStarter =
      saveAndExitCursorEditedCustomDraftActiveRoom &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "ascii_map_custom_draft_cursor_edit_reboot_starter",
          iggy3d::smoke::saveRootArg(customCursorEditSaveExitRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "input_owner", "starter") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "product_save_status",
                              "not_requested") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "not_requested") &&
      std::filesystem::exists(customCursorEditSaveExitFile);

  fields.clear();
  const bool continueCursorEditedCustomDraftActiveRoom =
      rebootCursorEditedCustomDraftStarter &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_cursor_edit_save_exit_continue",
          "frontend.select=continue\nfrontend.execute=true\n",
          iggy3d::smoke::saveRootArg(customCursorEditSaveExitRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_reason_code",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_source",
                              "continue") &&
      iggy3d::smoke::hasField(fields, "product_save_load_save_id",
                              "save_001") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_session_loaded",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_room_present",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source",
                              "saved_authored_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_room_collision_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_query_surface_count",
                              "182") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_walkable_surface_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_projectile_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_cpu_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_source",
                              "scene_room_projection") &&
      iggy3d::smoke::hasField(fields, "product_vulkan_room_asset_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_wall_draw_count",
                              "22") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_vertex_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_index_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_draw_count") &&
      iggy3d::smoke::positiveIntegerField(
          fields, "product_vulkan_room_geometry_signature");

  fields.clear();
  const bool liveEditCustomDraftActiveRoom =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_live_edit",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "world.draft_cell=1,2,#\n"
          "world.create=true\n"
          "room_edit.start_active=true\n"
          "room_edit.add_wall=live_wall_1,0,0,0,1,0,0,0,2.5,1\n",
          iggy3d::smoke::saveRootArg(customLiveEditSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields,
                              "automation_control_last_key",
                              "room_edit.add_wall") &&
      iggy3d::smoke::hasField(fields,
                              "automation_control_last_action",
                              "room_edit.add_wall") &&
      iggy3d::smoke::hasField(fields, "room_editing_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_status",
                              "product_room_editing_ready") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation",
                              "room_edit.add_wall") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_status",
                              "product_room_editing_edit_applied") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_reason_code",
                              "product_room_editing_edit_applied") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_accepted",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_primitive_id",
                              "live_wall_1") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_active_room_loaded",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_active_room_static_mesh_count",
                              "120") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_active_room_spatial_surface_count",
                              "182") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_surface_count",
                              "182") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_walkable_surface_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_actor_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_projectile_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "editable_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id", "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_room_collision_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_query_surface_count",
                              "182") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_walkable_surface_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_cpu_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_source",
                              "scene_room_projection") &&
      iggy3d::smoke::hasField(fields, "product_vulkan_room_asset_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_wall_draw_count",
                              "62") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_vertex_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_index_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_draw_count") &&
      iggy3d::smoke::positiveIntegerField(
          fields, "product_vulkan_room_grid_line_draw_count") &&
      iggy3d::smoke::positiveIntegerField(
          fields, "product_vulkan_room_geometry_signature");

  fields.clear();
  const std::filesystem::path customLiveEditPauseSaveFile =
      customLiveEditPauseSaveRoot / "save_001.iggy3d.save";
  const bool saveLiveEditedCustomDraftActiveRoom =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_live_edit_save",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "world.draft_cell=1,2,#\n"
          "world.create=true\n"
          "room_edit.start_active=true\n"
          "room_edit.add_wall=live_wall_1,0,0,0,1,0,0,0,2.5,1\n"
          "system.pause=true\n"
          "pause.select=save\n"
          "pause.execute=true\n",
          iggy3d::smoke::saveRootArg(customLiveEditPauseSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "pause") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "save") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "pause_menu_open", "true") &&
      iggy3d::smoke::hasField(fields, "room_editing_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_status",
                              "product_room_editing_ready") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation",
                              "room_edit.add_wall") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_status",
                              "product_room_editing_edit_applied") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_operation_accepted",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_last_primitive_id",
                              "live_wall_1") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_active_room_loaded",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_surface_count",
                              "182") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_actor_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "room_editing_collision_projectile_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "editable_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id", "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_room_collision_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_query_surface_count",
                              "182") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "product_save_status",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_reason_code",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_durable_reason",
                              "durable_save_file_written") &&
      iggy3d::smoke::hasField(fields, "product_save_source", "pause_save") &&
      iggy3d::smoke::hasField(fields, "product_save_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_session_saved", "true") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      std::filesystem::exists(customLiveEditPauseSaveFile) &&
      fileContains(customLiveEditPauseSaveFile,
                   "authoredRoom.id=custom_dungeon_draft\n") &&
      fileContains(customLiveEditPauseSaveFile,
                   "authoredRoom.floor.count=58\n") &&
      fileContains(customLiveEditPauseSaveFile,
                   "authoredRoom.wall.count=62\n") &&
      fileContains(customLiveEditPauseSaveFile, "live_wall_1\n");

  fields.clear();
  const bool continueLiveEditedCustomDraftActiveRoom =
      saveLiveEditedCustomDraftActiveRoom &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_live_edit_continue",
          "frontend.select=continue\nfrontend.execute=true\n",
          iggy3d::smoke::saveRootArg(customLiveEditPauseSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_reason_code",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_source",
                              "continue") &&
      iggy3d::smoke::hasField(fields, "product_save_load_save_id",
                              "save_001") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_session_loaded",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_room_present",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source",
                              "saved_authored_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_room_collision_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_query_surface_count",
                              "182") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_walkable_surface_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_projectile_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_cpu_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_source",
                              "scene_room_projection") &&
      iggy3d::smoke::hasField(fields, "product_vulkan_room_asset_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_wall_draw_count",
                              "23") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_vertex_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_index_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_draw_count") &&
      iggy3d::smoke::positiveIntegerField(
          fields, "product_vulkan_room_geometry_signature");

  fields.clear();
  const std::filesystem::path customLiveEditSaveExitFile =
      customLiveEditSaveExitRoot / "save_001.iggy3d.save";
  const bool saveAndExitLiveEditedCustomDraftActiveRoom =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_live_edit_save_exit",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "world.draft_cell=1,2,#\n"
          "world.create=true\n"
          "room_edit.start_active=true\n"
          "room_edit.add_wall=live_wall_1,0,0,0,1,0,0,0,2.5,1\n"
          "system.pause=true\n"
          "pause.select=save_and_exit\n"
          "pause.execute=true\n",
          iggy3d::smoke::saveRootArg(customLiveEditSaveExitRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "launch_status",
                              "pause_save_and_exit_written") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "input_owner", "starter") &&
      iggy3d::smoke::hasField(fields, "product_save_status",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_reason_code",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_durable_reason",
                              "durable_save_file_written") &&
      iggy3d::smoke::hasField(fields, "product_save_source",
                              "pause_save_and_exit") &&
      iggy3d::smoke::hasField(fields, "product_save_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_session_saved", "true") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_transition_returned_to_title",
                              "true") &&
      std::filesystem::exists(customLiveEditSaveExitFile) &&
      fileContains(customLiveEditSaveExitFile,
                   "authoredRoom.id=custom_dungeon_draft\n") &&
      fileContains(customLiveEditSaveExitFile,
                   "authoredRoom.floor.count=58\n") &&
      fileContains(customLiveEditSaveExitFile,
                   "authoredRoom.wall.count=62\n") &&
      fileContains(customLiveEditSaveExitFile, "live_wall_1\n");

  fields.clear();
  const bool rebootLiveEditedCustomDraftStarter =
      saveAndExitLiveEditedCustomDraftActiveRoom &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "ascii_map_custom_draft_live_edit_reboot_starter",
          iggy3d::smoke::saveRootArg(customLiveEditSaveExitRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "input_owner", "starter") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "product_save_status",
                              "not_requested") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "not_requested") &&
      std::filesystem::exists(customLiveEditSaveExitFile);

  fields.clear();
  const bool continueSaveAndExitLiveEditedCustomDraftActiveRoom =
      rebootLiveEditedCustomDraftStarter &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_custom_draft_live_edit_save_exit_continue",
          "frontend.select=continue\nfrontend.execute=true\n",
          iggy3d::smoke::saveRootArg(customLiveEditSaveExitRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_reason_code",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_source",
                              "continue") &&
      iggy3d::smoke::hasField(fields, "product_save_load_save_id",
                              "save_001") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_session_loaded",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_room_present",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source",
                              "saved_authored_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "active_room_collision_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_query_surface_count",
                              "182") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_walkable_surface_count",
                              "58") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_projectile_blocker_count",
                              "62") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_cpu_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_mesh_source",
                              "scene_room_projection") &&
      iggy3d::smoke::hasField(fields, "product_vulkan_room_asset_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields,
                              "product_vulkan_room_wall_draw_count",
                              "23") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_vertex_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_index_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_draw_count") &&
      iggy3d::smoke::positiveIntegerField(
          fields, "product_vulkan_room_geometry_signature");

  fields.clear();
  const std::string createControl =
      std::string{"frontend.select=new_world\nfrontend.execute=true\n"} +
      "world.title=Loop Keep\n"
      "world.ascii_room_id=loop_keep_ascii\n"
      "world.ascii_room_source_name=" + std::string{kMapPath} + "\n"
      "world.ascii_room_text=" + automationTextValue(mapText) + "\n"
      "world.create=true\n";

  const bool createMapWorld =
      appAvailable && mapAvailable &&
      iggy3d::smoke::runProductCase(binary,
                                    "ascii_map_loop_keep_create",
                                    createControl,
                                    iggy3d::smoke::saveRootArg(saveRoot),
                                    fields,
                                    exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "world_setup_title", "Loop Keep") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_enabled",
                              "true") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_id",
                              "loop_keep_ascii") &&
      iggy3d::smoke::hasField(fields, "world_setup_ascii_room_source_name",
                              kMapPath) &&
      iggy3d::smoke::hasField(fields, "world_creation_status",
                              "world_creation_initial_save_written") &&
      iggy3d::smoke::hasField(fields, "world_creation_world_title",
                              "Loop Keep") &&
      iggy3d::smoke::hasField(fields, "world_creation_ascii_room_requested",
                              "true") &&
      iggy3d::smoke::hasField(fields, "world_creation_ascii_room_id",
                              "loop_keep_ascii") &&
      iggy3d::smoke::hasField(fields, "world_creation_ascii_room_source_name",
                              kMapPath) &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_id",
                              "save_001") &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_title",
                              "Loop Keep") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_status",
                              "product_ascii_room_ready") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_room_id",
                              "loop_keep_ascii") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_source_name",
                              kMapPath) &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_width", "17") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_height", "7") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_floor_count", "59") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_wall_count", "60") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_marker_count", "5") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_static_mesh_count",
                              "120") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_anchor_count", "5") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "ascii_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id", "loop_keep_ascii") &&
      iggy3d::smoke::hasField(fields, "active_room_authored_floor_count",
                              "59") &&
      iggy3d::smoke::hasField(fields, "active_room_authored_wall_count",
                              "60") &&
      iggy3d::smoke::hasField(fields, "active_room_authored_marker_count",
                              "5") &&
      iggy3d::smoke::hasField(fields, "active_room_collision_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_walkable_surface_count",
                              "59") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "61") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_door_blocker_count",
                              "1") &&
      std::filesystem::exists(saveRoot / "save_001.iggy3d.save") &&
      fileContains(saveRoot / "save_001.iggy3d.save",
                   "authoredRoom.id=loop_keep_ascii\n") &&
      fileContains(saveRoot / "save_001.iggy3d.save",
                   "authoredRoom.floor.count=59\n") &&
      fileContains(saveRoot / "save_001.iggy3d.save",
                   "authoredRoom.wall.count=60\n") &&
      fileContains(saveRoot / "save_001.iggy3d.save",
                   "authoredRoom.marker.count=5\n");

  fields.clear();
  const bool continueMapWorld =
      createMapWorld &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_map_loop_keep_continue",
          "frontend.select=continue\nfrontend.execute=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_source",
                              "continue") &&
      iggy3d::smoke::hasField(fields, "product_save_load_save_id",
                              "save_001") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_room_present",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_room_id",
                              "loop_keep_ascii") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_floor_count",
                              "59") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_wall_count",
                              "60") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_marker_count",
                              "5") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source",
                              "saved_authored_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id", "loop_keep_ascii") &&
      iggy3d::smoke::hasField(fields, "active_room_authored_floor_count",
                              "59") &&
      iggy3d::smoke::hasField(fields, "active_room_authored_wall_count",
                              "60") &&
      iggy3d::smoke::hasField(fields, "active_room_authored_marker_count",
                              "5") &&
      iggy3d::smoke::hasField(fields, "active_room_collision_ready", "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_walkable_surface_count",
                              "59") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "61") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_door_blocker_count",
                              "1") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_vertex_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_index_count") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_vulkan_room_draw_count");

  const bool passed = createDefaultDungeonWorld && createSelectedDungeonWorld &&
                      createCustomDraftWorld && cursorPaintRequiresEditMode &&
                      createCursorPaintDraftWorld &&
                      startCustomDraftActiveRoomEditing &&
                      pauseEditCustomDraftActiveRoom &&
                      pauseEditorCursorPlaceWall &&
                      editorInputCursorPlaceWall &&
                      saveAndExitCursorEditedCustomDraftActiveRoom &&
                      rebootCursorEditedCustomDraftStarter &&
                      continueCursorEditedCustomDraftActiveRoom &&
                      liveEditCustomDraftActiveRoom &&
                      saveLiveEditedCustomDraftActiveRoom &&
                      continueLiveEditedCustomDraftActiveRoom &&
                      saveAndExitLiveEditedCustomDraftActiveRoom &&
                      rebootLiveEditedCustomDraftStarter &&
                      continueSaveAndExitLiveEditedCustomDraftActiveRoom &&
                      createMapWorld &&
                      continueMapWorld;
  const bool ok = expect(appAvailable, "product app exists") &&
                  expect(mapAvailable, "loop keep map fixture exists") &&
                  expect(createDefaultDungeonWorld,
                         "default new world creates loop keep dungeon") &&
                  expect(createSelectedDungeonWorld,
                         "new world selector creates gatehouse dungeon") &&
                  expect(createCustomDraftWorld,
                         "new world custom draft creates edited dungeon") &&
                  expect(cursorPaintRequiresEditMode,
                         "cursor draft paint requires edit mode") &&
                  expect(createCursorPaintDraftWorld,
                         "new world cursor paint creates edited dungeon") &&
                  expect(startCustomDraftActiveRoomEditing,
                         "custom draft active room enters editing") &&
                  expect(pauseEditCustomDraftActiveRoom,
                         "custom draft pause edit room enters editing") &&
                  expect(pauseEditorCursorPlaceWall,
                         "custom draft pause editor cursor places wall") &&
                  expect(editorInputCursorPlaceWall,
                         "custom draft editor input places wall") &&
                  expect(saveAndExitCursorEditedCustomDraftActiveRoom,
                         "custom draft cursor edit saves and exits") &&
                  expect(rebootCursorEditedCustomDraftStarter,
                         "custom draft cursor edit reboot starter") &&
                  expect(continueCursorEditedCustomDraftActiveRoom,
                         "custom draft cursor edit save-exit continues") &&
                  expect(liveEditCustomDraftActiveRoom,
                         "custom draft active room live edit") &&
                  expect(saveLiveEditedCustomDraftActiveRoom,
                         "custom draft active room live edit persists") &&
                  expect(continueLiveEditedCustomDraftActiveRoom,
                         "custom draft active room live edit continues") &&
                  expect(saveAndExitLiveEditedCustomDraftActiveRoom,
                         "custom draft active room live edit saves and exits") &&
                  expect(rebootLiveEditedCustomDraftStarter,
                         "custom draft active room live edit reboot starter") &&
                  expect(continueSaveAndExitLiveEditedCustomDraftActiveRoom,
                         "custom draft active room live edit save-exit continues") &&
                  expect(createMapWorld, "create map world") &&
                  expect(continueMapWorld, "continue map world");

  std::cout << "smoke=product_ascii_map\n";
  std::cout << "map_path=" << kMapPath << "\n";
  std::cout << "create_default_dungeon_world="
            << (createDefaultDungeonWorld ? "true" : "false") << "\n";
  std::cout << "create_selected_dungeon_world="
            << (createSelectedDungeonWorld ? "true" : "false") << "\n";
  std::cout << "create_custom_draft_world="
            << (createCustomDraftWorld ? "true" : "false") << "\n";
  std::cout << "cursor_paint_requires_edit_mode="
            << (cursorPaintRequiresEditMode ? "true" : "false") << "\n";
  std::cout << "create_cursor_paint_draft_world="
            << (createCursorPaintDraftWorld ? "true" : "false") << "\n";
  std::cout << "start_custom_draft_active_room_editing="
            << (startCustomDraftActiveRoomEditing ? "true" : "false") << "\n";
  std::cout << "pause_edit_custom_draft_active_room="
            << (pauseEditCustomDraftActiveRoom ? "true" : "false") << "\n";
  std::cout << "pause_editor_cursor_place_wall="
            << (pauseEditorCursorPlaceWall ? "true" : "false") << "\n";
  std::cout << "editor_input_cursor_place_wall="
            << (editorInputCursorPlaceWall ? "true" : "false") << "\n";
  std::cout << "save_and_exit_cursor_edited_custom_draft_active_room="
            << (saveAndExitCursorEditedCustomDraftActiveRoom ? "true"
                                                             : "false")
            << "\n";
  std::cout << "reboot_cursor_edited_custom_draft_starter="
            << (rebootCursorEditedCustomDraftStarter ? "true" : "false")
            << "\n";
  std::cout << "continue_cursor_edited_custom_draft_active_room="
            << (continueCursorEditedCustomDraftActiveRoom ? "true" : "false")
            << "\n";
  std::cout << "live_edit_custom_draft_active_room="
            << (liveEditCustomDraftActiveRoom ? "true" : "false") << "\n";
  std::cout << "save_live_edited_custom_draft_active_room="
            << (saveLiveEditedCustomDraftActiveRoom ? "true" : "false")
            << "\n";
  std::cout << "continue_live_edited_custom_draft_active_room="
            << (continueLiveEditedCustomDraftActiveRoom ? "true" : "false")
            << "\n";
  std::cout << "save_and_exit_live_edited_custom_draft_active_room="
            << (saveAndExitLiveEditedCustomDraftActiveRoom ? "true" : "false")
            << "\n";
  std::cout << "reboot_live_edited_custom_draft_starter="
            << (rebootLiveEditedCustomDraftStarter ? "true" : "false")
            << "\n";
  std::cout << "continue_save_and_exit_live_edited_custom_draft_active_room="
            << (continueSaveAndExitLiveEditedCustomDraftActiveRoom ? "true"
                                                                   : "false")
            << "\n";
  std::cout << "create_map_world=" << (createMapWorld ? "true" : "false")
            << "\n";
  std::cout << "continue_map_world=" << (continueMapWorld ? "true" : "false")
            << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed && ok ? "pass" : (appAvailable ? "fail" : "skip"))
            << "\n";
  std::cout << "reason_code="
            << (passed && ok ? "product_ascii_map_pass"
                             : (appAvailable ? "product_ascii_map_failed"
                                             : "product_app_unavailable"))
            << "\n";
  if (passed && ok) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}

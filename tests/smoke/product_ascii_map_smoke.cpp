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
  const std::filesystem::path customEditSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_custom_draft_edit_active");
  const std::filesystem::path customLiveEditSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_map_custom_draft_live_edit");
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
                              "61") &&
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
                      createCustomDraftWorld && startCustomDraftActiveRoomEditing &&
                      liveEditCustomDraftActiveRoom && createMapWorld &&
                      continueMapWorld;
  const bool ok = expect(appAvailable, "product app exists") &&
                  expect(mapAvailable, "loop keep map fixture exists") &&
                  expect(createDefaultDungeonWorld,
                         "default new world creates loop keep dungeon") &&
                  expect(createSelectedDungeonWorld,
                         "new world selector creates gatehouse dungeon") &&
                  expect(createCustomDraftWorld,
                         "new world custom draft creates edited dungeon") &&
                  expect(startCustomDraftActiveRoomEditing,
                         "custom draft active room enters editing") &&
                  expect(liveEditCustomDraftActiveRoom,
                         "custom draft active room live edit") &&
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
  std::cout << "start_custom_draft_active_room_editing="
            << (startCustomDraftActiveRoomEditing ? "true" : "false") << "\n";
  std::cout << "live_edit_custom_draft_active_room="
            << (liveEditCustomDraftActiveRoom ? "true" : "false") << "\n";
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

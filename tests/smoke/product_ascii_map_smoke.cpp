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
                      createMapWorld && continueMapWorld;
  const bool ok = expect(appAvailable, "product app exists") &&
                  expect(mapAvailable, "loop keep map fixture exists") &&
                  expect(createDefaultDungeonWorld,
                         "default new world creates loop keep dungeon") &&
                  expect(createSelectedDungeonWorld,
                         "new world selector creates gatehouse dungeon") &&
                  expect(createMapWorld, "create map world") &&
                  expect(continueMapWorld, "continue map world");

  std::cout << "smoke=product_ascii_map\n";
  std::cout << "map_path=" << kMapPath << "\n";
  std::cout << "create_default_dungeon_world="
            << (createDefaultDungeonWorld ? "true" : "false") << "\n";
  std::cout << "create_selected_dungeon_world="
            << (createSelectedDungeonWorld ? "true" : "false") << "\n";
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

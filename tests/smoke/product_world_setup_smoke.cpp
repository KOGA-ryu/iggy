#include "AutomationSmokeSupport.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

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

  int exitCode = 77;
  iggy3d::smoke::ReceiptFields fields;
  const std::filesystem::path saveRoot =
      iggy3d::smoke::cleanSaveRoot("world_setup");

  const bool newWorld =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "new_world",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Chapter One\nworld.create=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action",
                              "create_and_enter") &&
      iggy3d::smoke::hasField(fields, "frontend_launch_requested", "true") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "world_setup_title", "Chapter One") &&
      iggy3d::smoke::hasField(fields, "world_setup_status",
                              "world_setup_create_requested") &&
      iggy3d::smoke::hasField(fields, "world_creation_status",
                              "world_creation_initial_save_written") &&
      iggy3d::smoke::hasField(fields, "world_creation_reason_code",
                              "world_creation_initial_save_written") &&
      iggy3d::smoke::hasField(fields, "world_creation_world_id", "world_0001") &&
      iggy3d::smoke::hasField(fields, "world_creation_world_title",
                              "Chapter One") &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_requested",
                              "true") &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_written",
                              "true") &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_id",
                              "save_001") &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_title",
                              "Chapter One") &&
      iggy3d::smoke::hasField(fields, "world_creation_route_after_create",
                              "gameplay") &&
      iggy3d::smoke::hasField(fields, "product_save_status",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_reason_code",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_durable_reason",
                              "durable_save_file_written") &&
      iggy3d::smoke::hasField(fields, "product_save_source", "initial_world") &&
      iggy3d::smoke::hasField(fields, "product_save_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_session_saved", "true") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      std::filesystem::exists(saveRoot / "save_001.iggy3d.save") &&
      iggy3d::smoke::hasField(fields, "product_transition_last_action",
                              "launch_gameplay") &&
      iggy3d::smoke::hasField(fields, "product_transition_status",
                              "gameplay_active");

  fields.clear();
  const std::filesystem::path asciiSaveRoot =
      iggy3d::smoke::cleanSaveRoot("world_setup_ascii_room");
  const bool asciiWorld =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "new_world_ascii_room",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=ASCII Chapter\n"
          "world.ascii_room_id=ascii_chapter_room\n"
          "world.ascii_room_source_name=worlds/ascii_chapter.iggyroom.txt\n"
          "world.ascii_room_text=#######\\n#P..N.#\\n#.+.$.#\\n#..E..#\\n#######\\n\n"
          "world.create=true\n",
          iggy3d::smoke::saveRootArg(asciiSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "world_setup_title", "ASCII Chapter") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_ascii_room_enabled",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_ascii_room_text_present",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_ascii_room_id",
                              "ascii_chapter_room") &&
      iggy3d::smoke::hasField(fields,
                              "world_setup_ascii_room_source_name",
                              "worlds/ascii_chapter.iggyroom.txt") &&
      iggy3d::smoke::hasField(fields,
                              "world_creation_ascii_room_requested",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "world_creation_ascii_room_id",
                              "ascii_chapter_room") &&
      iggy3d::smoke::hasField(fields,
                              "world_creation_ascii_room_source_name",
                              "worlds/ascii_chapter.iggyroom.txt") &&
      iggy3d::smoke::hasField(fields, "world_creation_world_title",
                              "ASCII Chapter") &&
      iggy3d::smoke::hasField(fields, "world_creation_status",
                              "world_creation_initial_save_written") &&
      iggy3d::smoke::hasField(fields,
                              "world_creation_initial_save_title",
                              "ASCII Chapter") &&
      iggy3d::smoke::hasField(fields, "product_save_status",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_source", "initial_world") &&
      iggy3d::smoke::hasField(fields, "product_save_save_id", "save_001") &&
      std::filesystem::exists(asciiSaveRoot / "save_001.iggy3d.save") &&
      fileContains(asciiSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.present=true\n") &&
      fileContains(asciiSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.id=ascii_chapter_room\n") &&
      fileContains(asciiSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.marker.count=5\n") &&
      fileContains(asciiSaveRoot / "save_001.iggy3d.save",
                   "authoredRoom.marker.0.id=marker_player_spawn_r1_c1\n") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_status",
                              "product_ascii_room_ready") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_room_id",
                              "ascii_chapter_room") &&
      iggy3d::smoke::hasField(fields, "ascii_room_preview_source_name",
                              "worlds/ascii_chapter.iggyroom.txt") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "ascii_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id", "ascii_chapter_room") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_transition_status",
                              "gameplay_active");

  fields.clear();
  const bool loadAsciiWorld =
      asciiWorld && appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "continue_ascii_room",
          "frontend.select=continue\nfrontend.execute=true\n",
          iggy3d::smoke::saveRootArg(asciiSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_source",
                              "continue") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_room_present",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_room_id",
                              "ascii_chapter_room") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_floor_count",
                              "15") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_wall_count",
                              "20") &&
      iggy3d::smoke::hasField(fields,
                              "product_save_load_authored_marker_count",
                              "5") &&
      iggy3d::smoke::hasField(fields, "saved_marker_bind_status",
                              "saved_marker_bind_noop") &&
      iggy3d::smoke::hasField(fields, "saved_marker_bind_reason_code",
                              "saved_marker_bind_noop") &&
      iggy3d::smoke::hasField(fields, "saved_marker_bind_requested",
                              "true") &&
      iggy3d::smoke::hasField(fields, "saved_marker_bind_session_replaced",
                              "false") &&
      iggy3d::smoke::hasField(fields, "saved_marker_bind_room_id",
                              "ascii_chapter_room") &&
      iggy3d::smoke::hasField(fields, "saved_marker_bind_marker_count",
                              "5") &&
      iggy3d::smoke::hasField(fields, "saved_marker_bind_seed_entity_count",
                              "5") &&
      iggy3d::smoke::hasField(fields,
                              "saved_marker_bind_added_entity_count",
                              "0") &&
      iggy3d::smoke::hasField(fields,
                              "saved_marker_bind_existing_entity_count",
                              "5") &&
      iggy3d::smoke::hasField(fields,
                              "saved_marker_bind_added_objective_count",
                              "0") &&
      iggy3d::smoke::hasField(fields,
                              "saved_marker_bind_existing_objective_count",
                              "2") &&
      iggy3d::smoke::hasField(fields,
                              "saved_marker_bind_added_combatant_count",
                              "0") &&
      iggy3d::smoke::hasField(fields,
                              "saved_marker_bind_existing_combatant_count",
                              "2") &&
      iggy3d::smoke::hasField(fields, "saved_marker_bind_pickup_count",
                              "1") &&
      iggy3d::smoke::hasField(fields, "saved_marker_bind_door_count",
                              "1") &&
      iggy3d::smoke::hasField(fields,
                              "saved_marker_bind_marker_entity_count",
                              "1") &&
      iggy3d::smoke::hasField(fields, "saved_marker_bind_npc_count",
                              "1") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_source",
                              "saved_authored_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id",
                              "ascii_chapter_room") &&
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
                              "active_room_authored_marker_count",
                              "5") &&
      iggy3d::smoke::hasField(fields, "active_room_anchor_count", "5") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_walkable_surface_count",
                              "15") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_actor_blocker_count",
                              "21") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_door_blocker_count",
                              "1") &&
      iggy3d::smoke::hasField(fields,
                              "product_transition_status",
                              "gameplay_active");

  std::cout << "smoke=product_world_setup\n";
  std::cout << "new_world=" << (newWorld ? "true" : "false") << "\n";
  std::cout << "ascii_world=" << (asciiWorld ? "true" : "false") << "\n";
  std::cout << "load_ascii_world=" << (loadAsciiWorld ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result="
            << (newWorld && asciiWorld && loadAsciiWorld
                    ? "pass"
                    : (appAvailable ? "fail" : "skip")) << "\n";
  std::cout << "reason_code="
            << (newWorld && asciiWorld && loadAsciiWorld
                    ? "product_world_setup_pass"
                    : (appAvailable ? "product_world_setup_failed"
                                    : "product_app_unavailable"))
            << "\n";
  if (newWorld && asciiWorld && loadAsciiWorld) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}

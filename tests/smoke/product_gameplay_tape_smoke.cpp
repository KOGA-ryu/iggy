#include "AutomationSmokeSupport.hpp"

#include "app/iggy3d/ascii_room/Authoring.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool nonNoneField(const iggy3d::smoke::ReceiptFields& fields,
                  std::string_view key) {
  const auto it = fields.find(std::string(key));
  return it != fields.end() && !it->second.empty() && it->second != "none";
}

bool selectedPhysicsRoomTapePassed(const iggy3d::smoke::ReceiptFields& fields,
                                   std::string_view roomId,
                                   std::string_view querySurfaceCount,
                                   std::string_view targetStableName) {
  return iggy3d::smoke::productReceipt(fields) &&
         iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
         iggy3d::smoke::hasField(fields, "window_created", "false") &&
         iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
         iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
         iggy3d::smoke::hasField(fields, "active_room_id", roomId) &&
         iggy3d::smoke::hasField(fields, "active_room_collision_ready", "true") &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_room_id",
                                 roomId) &&
         iggy3d::smoke::hasField(fields,
                                 "active_room_collision_query_surface_count",
                                 querySurfaceCount) &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tape_status",
                                 "gameplay_tape_completed") &&
         iggy3d::smoke::hasField(fields, "gameplay_tape_step_count", "1") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tape_executed_step_count",
                                 "1") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tape_last_action",
                                 "move") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tape_last_target",
                                 targetStableName) &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_tape_last_movement_block",
                                 "movement_ok") &&
         iggy3d::smoke::hasField(fields,
                                 "physics_movement_planner_enabled",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "physics_movement_planner_requested",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "physics_movement_planner_used",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "physics_movement_planner_status",
                                 "physics_movement_planner_used") &&
         iggy3d::smoke::hasField(fields,
                                 "physics_movement_planner_reason_code",
                                 "physics_movement_planner_used") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_attempted",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_debug_available",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "gameplay_movement_reason_code",
                                 "movement_ok") &&
         iggy3d::smoke::positiveIntegerField(
             fields, "gameplay_movement_collision_sweep_count") &&
         iggy3d::smoke::hasField(fields,
                                 "physics_debug_hud_visible",
                                 "true") &&
         iggy3d::smoke::hasField(fields,
                                 "physics_debug_hud_line_count",
                                 "4") &&
         iggy3d::smoke::hasField(fields,
                                 "physics_debug_hud_status",
                                 "physics_debug_ready") &&
         iggy3d::smoke::hasField(fields,
                                 "product_draw_physics_debug_visible",
                                 "true") &&
         iggy3d::smoke::positiveIntegerField(
             fields, "product_draw_physics_debug_item_count") &&
         iggy3d::smoke::positiveIntegerField(
             fields, "product_draw_physics_aabb_debug_count") &&
         iggy3d::smoke::hasField(fields,
                                 "product_render_bridge_physics_debug_visible",
                                 "true") &&
         iggy3d::smoke::positiveIntegerField(
             fields, "product_render_bridge_physics_debug_item_count") &&
         iggy3d::smoke::positiveIntegerField(
             fields, "product_render_bridge_physics_aabb_debug_count");
}

std::string generatedPackageText() {
  return "[package]\n"
         "id = \"iggy3d.ascii_gameplay_loop\"\n"
         "schema_version = 1\n"
         "required_runtime_schema = 1\n"
         "scenario = \"scenario.iggy3d.toml\"\n"
         "\n"
         "[[assets]]\n"
         "id = \"room.ascii_gameplay_loop\"\n"
         "path = \"assets/rooms/ascii_gameplay_loop.room.iggy3d.toml\"\n"
         "\n"
         "[[assets]]\n"
         "id = \"mesh.ascii_room_primitives\"\n"
         "path = \"assets/meshes/ascii_room_primitives.meshes.iggy3d.toml\"\n"
         "\n"
         "[[assets]]\n"
         "id = \"material.ascii_room\"\n"
         "path = \"assets/materials/ascii_room.materials.iggy3d.toml\"\n";
}

std::string generatedScenarioText() {
  return "[scenario]\n"
         "id = \"ascii_gameplay_loop.runtime_loop\"\n"
         "\n"
         "[defaults]\n"
         "fixed_tick_rate_hz = 20\n"
         "interaction_range_meters = 1.500\n"
         "movement_distance_meters = 3.000\n"
         "slow_time_scale = 0.250\n"
         "initial_clock = \"Normal\"\n"
         "default_realtime_camera = \"FirstPerson\"\n"
         "default_tactical_camera = \"TacticalOverhead\"\n";
}

std::string generatedPassiveNpcScenarioText() {
  return generatedScenarioText() +
         "\n"
         "[[ai_actors]]\n"
         "actor = \"marker_npc_spawn_r1_c2\"\n"
         "behavior_profile_id = \"passive\"\n";
}

std::string generatedGhostNpcScenarioText() {
  return generatedScenarioText() +
         "\n"
         "[[ai_actors]]\n"
         "actor = \"marker_npc_spawn_r1_c2\"\n"
         "behavior_profile_id = \"ghost_profile\"\n";
}

std::string generatedMeshText() {
  return "[[primitive_meshes]]\n"
         "id = \"floor_rect\"\n"
         "kind = \"box\"\n"
         "size_ft = [3.280840, 0.328084, 3.280840]\n"
         "material = \"debug_floor\"\n"
         "\n"
         "[[primitive_meshes]]\n"
         "id = \"wall_segment\"\n"
         "kind = \"box\"\n"
         "size_ft = [3.280840, 8.202100, 3.280840]\n"
         "material = \"debug_wall\"\n"
         "\n"
         "[[primitive_meshes]]\n"
         "id = \"door_panel\"\n"
         "kind = \"box\"\n"
         "size_ft = [3.280840, 8.202100, 0.328084]\n"
         "material = \"debug_wall\"\n";
}

std::string generatedMaterialText() {
  return "[[materials]]\n"
         "id = \"debug_floor\"\n"
         "kind = \"vertex_color\"\n"
         "color = [0.30, 0.32, 0.34]\n"
         "\n"
         "[[materials]]\n"
         "id = \"debug_wall\"\n"
         "kind = \"vertex_color\"\n"
         "color = [0.42, 0.43, 0.46]\n";
}

std::string gameplayTapeText() {
  return "move marker_key_r1_c2\n"
         "expect_reject required_item_missing interact marker_secret_door_r1_c3\n"
         "interact marker_key_r1_c2\n"
         "interact marker_secret_door_r1_c3\n"
         "move marker_treasure_r1_c4\n"
         "expect_reject required_item_missing interact marker_exit_r1_c5\n"
         "interact marker_treasure_r1_c4\n"
         "interact marker_exit_r1_c5\n";
}

std::string wallCollisionTapeText() {
  return "expect_blocked blocked_by_collision move marker_key_r1_c3\n";
}

std::string npcCombatTapeText() {
  return "wait\n";
}

bool makeGeneratedAsciiPackage(const std::filesystem::path& root,
                               std::string_view sourceText,
                               std::string_view scenarioText,
                               std::string_view tapeText,
                               std::filesystem::path& packagePath,
                               std::filesystem::path& tapePath) {
  std::filesystem::create_directories(root / "assets" / "rooms");
  std::filesystem::create_directories(root / "assets" / "meshes");
  std::filesystem::create_directories(root / "assets" / "materials");

  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceName = "generated/ascii_gameplay_loop.iggyroom.txt";
  request.roomId = "ascii_gameplay_loop";
  request.centerOnOrigin = false;
  request.emitAssetText = true;
  request.sourceText = std::string(sourceText);

  const iggy3d::ProductAsciiRoomAuthoringResult authored =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  if (!authored.ok || !authored.assetText.ok) {
    return false;
  }

  packagePath = root / "package.iggy3d.toml";
  tapePath = root / "ascii_gameplay_loop.iggy3d.tape";
  return iggy3d::smoke::writeTextFile(packagePath, generatedPackageText()) &&
         iggy3d::smoke::writeTextFile(root / "scenario.iggy3d.toml",
                                      std::string(scenarioText)) &&
         iggy3d::smoke::writeTextFile(
             root / "assets" / "rooms" / "ascii_gameplay_loop.room.iggy3d.toml",
             authored.assetText.text) &&
         iggy3d::smoke::writeTextFile(
             root / "assets" / "meshes" /
                 "ascii_room_primitives.meshes.iggy3d.toml",
             generatedMeshText()) &&
         iggy3d::smoke::writeTextFile(
             root / "assets" / "materials" / "ascii_room.materials.iggy3d.toml",
             generatedMaterialText()) &&
         iggy3d::smoke::writeTextFile(tapePath, tapeText);
}

bool makeGeneratedAsciiPackage(const std::filesystem::path& root,
                               std::string_view sourceText,
                               std::string_view tapeText,
                               std::filesystem::path& packagePath,
                               std::filesystem::path& tapePath) {
  return makeGeneratedAsciiPackage(root,
                                   sourceText,
                                   generatedScenarioText(),
                                   tapeText,
                                   packagePath,
                                   tapePath);
}

}  // namespace

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("package"));
  const std::filesystem::path wallRoot =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("wall_package"));
  const std::filesystem::path npcRoot =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("npc_package"));
  const std::filesystem::path passiveNpcRoot =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("passive_npc_package"));
  const std::filesystem::path ghostNpcRoot =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("ghost_npc_package"));
  const std::filesystem::path physicsControlPath =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("physics_movement_control.txt"));
  const std::filesystem::path physicsTapePath =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("physics_movement_tape.txt"));
  const std::filesystem::path physicsFlatControlPath =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("physics_flat_control.txt"));
  const std::filesystem::path physicsFlatTapePath =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("physics_flat_tape.txt"));
  const std::filesystem::path physicsCorridorControlPath =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("physics_corridor_control.txt"));
  const std::filesystem::path physicsCorridorTapePath =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("physics_corridor_tape.txt"));
  const std::filesystem::path physicsCornerControlPath =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("physics_corner_control.txt"));
  const std::filesystem::path physicsCornerTapePath =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_gameplay_tape_" +
       iggy3d::smoke::uniqueCaseToken("physics_corner_tape.txt"));
  const std::filesystem::path saveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape");
  const std::filesystem::path physicsSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape_physics_movement");
  const std::filesystem::path physicsFlatSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape_physics_flat_room");
  const std::filesystem::path physicsCorridorSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape_physics_wall_corridor");
  const std::filesystem::path physicsCornerSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape_physics_corner_slide");
  const std::filesystem::path wallSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape_wall_collision");
  const std::filesystem::path npcSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape_npc_combat");
  const std::filesystem::path passiveNpcSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape_passive_npc");
  const std::filesystem::path ghostNpcSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape_ghost_npc");

  std::filesystem::path packagePath;
  std::filesystem::path tapePath;
  std::filesystem::path wallPackagePath;
  std::filesystem::path wallTapePath;
  std::filesystem::path npcPackagePath;
  std::filesystem::path npcTapePath;
  std::filesystem::path passiveNpcPackagePath;
  std::filesystem::path passiveNpcTapePath;
  std::filesystem::path ghostNpcPackagePath;
  std::filesystem::path ghostNpcTapePath;
  const bool packageGenerated =
      makeGeneratedAsciiPackage(root,
                                "#######\n"
                                "#PKs$E#\n"
                                "#######\n",
                                gameplayTapeText(),
                                packagePath,
                                tapePath);
  const bool wallPackageGenerated =
      makeGeneratedAsciiPackage(wallRoot,
                                "#####\n"
                                "#P#K#\n"
                                "#####\n",
                                wallCollisionTapeText(),
                                wallPackagePath,
                                wallTapePath);
  const bool npcPackageGenerated =
      makeGeneratedAsciiPackage(npcRoot,
                                "######\n"
                                "#PN$E#\n"
                                "######\n",
                                npcCombatTapeText(),
                                npcPackagePath,
                                npcTapePath);
  const bool passiveNpcPackageGenerated =
      makeGeneratedAsciiPackage(passiveNpcRoot,
                                "######\n"
                                "#PN$E#\n"
                                "######\n",
                                generatedPassiveNpcScenarioText(),
                                npcCombatTapeText(),
                                passiveNpcPackagePath,
                                passiveNpcTapePath);
  const bool ghostNpcPackageGenerated =
      makeGeneratedAsciiPackage(ghostNpcRoot,
                                "######\n"
                                "#PN$E#\n"
                                "######\n",
                                generatedGhostNpcScenarioText(),
                                npcCombatTapeText(),
                                ghostNpcPackagePath,
                                ghostNpcTapePath);
  const bool physicsControlGenerated =
      iggy3d::smoke::writeTextFile(
          physicsControlPath, "gameplay.physics_movement=true\n");
  const bool physicsTapeGenerated =
      iggy3d::smoke::writeTextFile(physicsTapePath,
                                   "move marker_key_r1_c3\n");
  const bool physicsFlatControlGenerated = iggy3d::smoke::writeTextFile(
      physicsFlatControlPath,
      "frontend.select=new_world\n"
      "frontend.execute=true\n"
      "world.dungeon_id=physics_flat_room\n"
      "gameplay.physics_movement=true\n"
      "world.create=true\n");
  const bool physicsFlatTapeGenerated =
      iggy3d::smoke::writeTextFile(physicsFlatTapePath,
                                   "move marker_key_r1_c3\n");
  const bool physicsCorridorControlGenerated = iggy3d::smoke::writeTextFile(
      physicsCorridorControlPath,
      "frontend.select=new_world\n"
      "frontend.execute=true\n"
      "world.dungeon_id=physics_wall_corridor\n"
      "gameplay.physics_movement=true\n"
      "world.create=true\n");
  const bool physicsCorridorTapeGenerated =
      iggy3d::smoke::writeTextFile(physicsCorridorTapePath,
                                   "move marker_key_r3_c1\n");
  const bool physicsCornerControlGenerated = iggy3d::smoke::writeTextFile(
      physicsCornerControlPath,
      "frontend.select=new_world\n"
      "frontend.execute=true\n"
      "world.dungeon_id=physics_corner_slide\n"
      "gameplay.physics_movement=true\n"
      "world.create=true\n");
  const bool physicsCornerTapeGenerated =
      iggy3d::smoke::writeTextFile(physicsCornerTapePath,
                                   "move marker_key_r3_c3\n");

  int exitCode = 77;
  iggy3d::smoke::ReceiptFields fields;
  const bool receiptValid =
      appAvailable && packageGenerated &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "product_gameplay_tape",
          std::string{"--package "} + iggy3d::smoke::shellQuote(packagePath) +
              " --auto-new-world --gameplay-tape " +
              iggy3d::smoke::shellQuote(tapePath) + " " +
              iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode);

  int physicsExitCode = 77;
  iggy3d::smoke::ReceiptFields physicsFields;
  const bool physicsReceiptValid =
      appAvailable && wallPackageGenerated && physicsControlGenerated &&
      physicsTapeGenerated &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "product_gameplay_tape_physics_movement",
          std::string{"--package "} + iggy3d::smoke::shellQuote(wallPackagePath) +
              " --auto-new-world --automation-control " +
              iggy3d::smoke::shellQuote(physicsControlPath) +
              " --debug-overlay --gameplay-tape " +
              iggy3d::smoke::shellQuote(physicsTapePath) + " " +
              iggy3d::smoke::saveRootArg(physicsSaveRoot),
          physicsFields,
          physicsExitCode);

  int physicsFlatExitCode = 77;
  iggy3d::smoke::ReceiptFields physicsFlatFields;
  const bool physicsFlatReceiptValid =
      appAvailable && physicsFlatControlGenerated && physicsFlatTapeGenerated &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "product_gameplay_tape_physics_flat_room",
          std::string{"--automation-control "} +
              iggy3d::smoke::shellQuote(physicsFlatControlPath) +
              " --debug-overlay --gameplay-tape " +
              iggy3d::smoke::shellQuote(physicsFlatTapePath) + " " +
              iggy3d::smoke::saveRootArg(physicsFlatSaveRoot),
          physicsFlatFields,
          physicsFlatExitCode);

  int physicsCorridorExitCode = 77;
  iggy3d::smoke::ReceiptFields physicsCorridorFields;
  const bool physicsCorridorReceiptValid =
      appAvailable && physicsCorridorControlGenerated &&
      physicsCorridorTapeGenerated &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "product_gameplay_tape_physics_wall_corridor",
          std::string{"--automation-control "} +
              iggy3d::smoke::shellQuote(physicsCorridorControlPath) +
              " --debug-overlay --gameplay-tape " +
              iggy3d::smoke::shellQuote(physicsCorridorTapePath) + " " +
              iggy3d::smoke::saveRootArg(physicsCorridorSaveRoot),
          physicsCorridorFields,
          physicsCorridorExitCode);

  int physicsCornerExitCode = 77;
  iggy3d::smoke::ReceiptFields physicsCornerFields;
  const bool physicsCornerReceiptValid =
      appAvailable && physicsCornerControlGenerated &&
      physicsCornerTapeGenerated &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "product_gameplay_tape_physics_corner_slide",
          std::string{"--automation-control "} +
              iggy3d::smoke::shellQuote(physicsCornerControlPath) +
              " --debug-overlay --gameplay-tape " +
              iggy3d::smoke::shellQuote(physicsCornerTapePath) + " " +
              iggy3d::smoke::saveRootArg(physicsCornerSaveRoot),
          physicsCornerFields,
          physicsCornerExitCode);

  int wallExitCode = 77;
  iggy3d::smoke::ReceiptFields wallFields;
  const bool wallReceiptValid =
      appAvailable && wallPackageGenerated &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "product_gameplay_tape_wall_collision",
          std::string{"--package "} +
              iggy3d::smoke::shellQuote(wallPackagePath) +
              " --auto-new-world --gameplay-tape " +
              iggy3d::smoke::shellQuote(wallTapePath) + " " +
              iggy3d::smoke::saveRootArg(wallSaveRoot),
          wallFields,
          wallExitCode);

  int npcExitCode = 77;
  iggy3d::smoke::ReceiptFields npcFields;
  const bool npcReceiptValid =
      appAvailable && npcPackageGenerated &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "product_gameplay_tape_npc_combat",
          std::string{"--package "} +
              iggy3d::smoke::shellQuote(npcPackagePath) +
              " --auto-new-world --gameplay-tape " +
              iggy3d::smoke::shellQuote(npcTapePath) + " " +
              iggy3d::smoke::saveRootArg(npcSaveRoot),
          npcFields,
          npcExitCode);

  int passiveNpcExitCode = 77;
  iggy3d::smoke::ReceiptFields passiveNpcFields;
  const bool passiveNpcReceiptValid =
      appAvailable && passiveNpcPackageGenerated &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "product_gameplay_tape_passive_npc",
          std::string{"--package "} +
              iggy3d::smoke::shellQuote(passiveNpcPackagePath) +
              " --auto-new-world --gameplay-tape " +
              iggy3d::smoke::shellQuote(passiveNpcTapePath) +
              " --debug-overlay " +
              iggy3d::smoke::saveRootArg(passiveNpcSaveRoot),
          passiveNpcFields,
          passiveNpcExitCode);

  int ghostNpcExitCode = 77;
  iggy3d::smoke::ReceiptFields ghostNpcFields;
  const bool ghostNpcReceiptValid =
      appAvailable && ghostNpcPackageGenerated &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "product_gameplay_tape_ghost_npc",
          std::string{"--package "} +
              iggy3d::smoke::shellQuote(ghostNpcPackagePath) +
              " --auto-new-world --gameplay-tape " +
              iggy3d::smoke::shellQuote(ghostNpcTapePath) +
              " --debug-overlay " +
              iggy3d::smoke::saveRootArg(ghostNpcSaveRoot),
          ghostNpcFields,
          ghostNpcExitCode);

  const bool passed =
      exitCode == 0 && receiptValid && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields,
                              "selected_package_id",
                              "iggy3d.ascii_gameplay_loop") &&
      iggy3d::smoke::hasField(fields,
                              "selected_scenario_id",
                              "ascii_gameplay_loop.runtime_loop") &&
      iggy3d::smoke::hasField(fields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(fields, "active_room_source", "package_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id", "ascii_gameplay_loop") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_spatial_surface_count",
                              "38") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_walkable_surface_count",
                              "5") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_actor_blocker_count",
                              "17") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_query_surface_count",
                              "37") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_runtime_owned_surface_count",
                              "1") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_runtime_filtered_surface_count",
                              "1") &&
      iggy3d::smoke::hasField(fields,
                              "active_room_collision_active_door_blocker_count",
                              "0") &&
      iggy3d::smoke::hasField(fields, "gameplay_tape_requested", "true") &&
      iggy3d::smoke::hasField(fields, "gameplay_tape_loaded", "true") &&
      iggy3d::smoke::hasField(fields,
                              "gameplay_tape_status",
                              "gameplay_tape_completed") &&
      iggy3d::smoke::hasField(fields,
                              "gameplay_tape_reason_code",
                              "gameplay_tape_completed") &&
      iggy3d::smoke::hasField(fields, "gameplay_tape_line_count", "8") &&
      iggy3d::smoke::hasField(fields, "gameplay_tape_step_count", "8") &&
      iggy3d::smoke::hasField(fields,
                              "gameplay_tape_executed_step_count",
                              "6") &&
      iggy3d::smoke::hasField(fields,
                              "gameplay_tape_expected_rejected_step_count",
                              "2") &&
      iggy3d::smoke::hasField(fields, "gameplay_tape_failed_step", "none") &&
      iggy3d::smoke::hasField(fields, "gameplay_tape_last_action", "interact") &&
      iggy3d::smoke::hasField(fields,
                              "gameplay_tape_last_target",
                              "marker_exit_r1_c5") &&
      iggy3d::smoke::hasField(fields,
                              "gameplay_tape_key_collected",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "gameplay_tape_secret_door_opened",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "gameplay_tape_treasure_collected",
                              "true") &&
      iggy3d::smoke::hasField(fields,
                              "gameplay_tape_exit_objective_complete",
                              "true") &&
      iggy3d::smoke::hasField(fields, "gameplay_tape_loop_complete", "true") &&
      iggy3d::smoke::hasField(fields,
                              "physics_movement_planner_enabled",
                              "false") &&
      iggy3d::smoke::hasField(fields,
                              "physics_movement_planner_requested",
                              "false") &&
      iggy3d::smoke::hasField(fields,
                              "physics_movement_planner_used",
                              "false") &&
      iggy3d::smoke::hasField(fields,
                              "physics_movement_planner_status",
                              "physics_movement_planner_disabled") &&
      iggy3d::smoke::hasField(fields, "session_outcome", "Victory");

  const bool physicsPassed =
      physicsExitCode == 0 && physicsReceiptValid &&
      iggy3d::smoke::productReceipt(physicsFields) &&
      iggy3d::smoke::hasField(physicsFields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(physicsFields, "window_created", "false") &&
      iggy3d::smoke::hasField(physicsFields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(physicsFields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(physicsFields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(physicsFields, "active_room_source", "package_room") &&
      iggy3d::smoke::hasField(physicsFields,
                              "active_room_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(physicsFields,
                              "active_room_collision_query_surface_count",
                              "28") &&
      iggy3d::smoke::hasField(physicsFields,
                              "product_vulkan_room_mesh_cpu_ready",
                              "true") &&
      iggy3d::smoke::hasField(physicsFields,
                              "automation_control_requested",
                              "true") &&
      iggy3d::smoke::hasField(physicsFields,
                              "automation_control_loaded",
                              "true") &&
      iggy3d::smoke::hasField(physicsFields,
                              "automation_control_last_key",
                              "gameplay.physics_movement") &&
      iggy3d::smoke::hasField(physicsFields,
                              "automation_control_last_result",
                              "applied") &&
      iggy3d::smoke::hasField(physicsFields,
                              "gameplay_tape_status",
                              "gameplay_tape_completed") &&
      iggy3d::smoke::hasField(physicsFields,
                              "gameplay_tape_step_count",
                              "1") &&
      iggy3d::smoke::hasField(physicsFields,
                              "gameplay_tape_executed_step_count",
                              "1") &&
      iggy3d::smoke::hasField(physicsFields,
                              "gameplay_tape_last_action",
                              "move") &&
      iggy3d::smoke::hasField(physicsFields,
                              "gameplay_tape_last_target",
                              "marker_key_r1_c3") &&
      iggy3d::smoke::hasField(physicsFields,
                              "physics_movement_planner_enabled",
                              "true") &&
      iggy3d::smoke::hasField(physicsFields,
                              "physics_movement_planner_requested",
                              "true") &&
      iggy3d::smoke::hasField(physicsFields,
                              "physics_movement_planner_used",
                              "true") &&
      iggy3d::smoke::hasField(physicsFields,
                              "physics_movement_planner_status",
                              "physics_movement_planner_used") &&
      iggy3d::smoke::hasField(physicsFields,
                              "physics_movement_planner_reason_code",
                              "physics_movement_planner_used") &&
      iggy3d::smoke::hasField(physicsFields,
                              "physics_debug_hud_visible",
                              "true") &&
      iggy3d::smoke::hasField(physicsFields,
                              "physics_debug_hud_line_count",
                              "4") &&
      iggy3d::smoke::hasField(physicsFields,
                              "physics_debug_hud_status",
                              "physics_debug_ready") &&
      iggy3d::smoke::hasField(physicsFields,
                              "product_draw_physics_debug_visible",
                              "true") &&
      iggy3d::smoke::positiveIntegerField(
          physicsFields, "product_draw_physics_debug_item_count") &&
      iggy3d::smoke::positiveIntegerField(
          physicsFields, "product_draw_physics_aabb_debug_count") &&
      iggy3d::smoke::positiveIntegerField(
          physicsFields, "product_draw_physics_contact_normal_debug_count") &&
      iggy3d::smoke::hasField(physicsFields,
                              "product_render_bridge_physics_debug_visible",
                              "true") &&
      iggy3d::smoke::positiveIntegerField(
          physicsFields, "product_render_bridge_physics_debug_item_count") &&
      iggy3d::smoke::positiveIntegerField(
          physicsFields, "product_render_bridge_physics_aabb_debug_count") &&
      iggy3d::smoke::positiveIntegerField(
          physicsFields,
          "product_render_bridge_physics_contact_normal_debug_count") &&
      iggy3d::smoke::hasField(physicsFields, "session_outcome", "None");

  const bool physicsFlatRoomPassed =
      physicsFlatExitCode == 0 && physicsFlatReceiptValid &&
      selectedPhysicsRoomTapePassed(physicsFlatFields,
                                    "physics_flat_room",
                                    "55",
                                    "marker_key_r1_c3") &&
      iggy3d::smoke::hasField(physicsFlatFields,
                              "gameplay_movement_status",
                              "moved") &&
      iggy3d::smoke::hasField(physicsFlatFields,
                              "gameplay_movement_blocked",
                              "false") &&
      iggy3d::smoke::hasField(physicsFlatFields,
                              "gameplay_movement_clamped",
                              "false") &&
      iggy3d::smoke::hasField(physicsFlatFields,
                              "gameplay_movement_slid",
                              "false") &&
      iggy3d::smoke::hasField(physicsFlatFields,
                              "gameplay_movement_hit_surface_id",
                              "none") &&
      iggy3d::smoke::hasField(
          physicsFlatFields, "product_draw_physics_contact_normal_debug_count", "0") &&
      iggy3d::smoke::hasField(
          physicsFlatFields,
          "product_render_bridge_physics_contact_normal_debug_count",
          "0") &&
      iggy3d::smoke::hasField(physicsFlatFields, "session_outcome", "None");

  const bool physicsWallCorridorPassed =
      physicsCorridorExitCode == 0 && physicsCorridorReceiptValid &&
      selectedPhysicsRoomTapePassed(physicsCorridorFields,
                                    "physics_wall_corridor",
                                    "76",
                                    "marker_key_r3_c1") &&
      iggy3d::smoke::hasField(physicsCorridorFields,
                              "gameplay_movement_status",
                              "moved") &&
      iggy3d::smoke::hasField(physicsCorridorFields,
                              "gameplay_movement_blocked",
                              "false") &&
      iggy3d::smoke::hasField(physicsCorridorFields,
                              "gameplay_movement_clamped",
                              "true") &&
      nonNoneField(physicsCorridorFields, "gameplay_movement_hit_surface_id") &&
      iggy3d::smoke::positiveIntegerField(
          physicsCorridorFields,
          "product_draw_physics_contact_normal_debug_count") &&
      iggy3d::smoke::positiveIntegerField(
          physicsCorridorFields,
          "product_render_bridge_physics_contact_normal_debug_count") &&
      iggy3d::smoke::hasField(physicsCorridorFields, "session_outcome", "None");

  const bool physicsCornerSlidePassed =
      physicsCornerExitCode == 0 && physicsCornerReceiptValid &&
      selectedPhysicsRoomTapePassed(physicsCornerFields,
                                    "physics_corner_slide",
                                    "66",
                                    "marker_key_r3_c3") &&
      iggy3d::smoke::hasField(physicsCornerFields,
                              "gameplay_movement_status",
                              "moved") &&
      iggy3d::smoke::hasField(physicsCornerFields,
                              "gameplay_movement_blocked",
                              "false") &&
      iggy3d::smoke::hasField(physicsCornerFields,
                              "gameplay_movement_clamped",
                              "true") &&
      iggy3d::smoke::hasField(physicsCornerFields,
                              "gameplay_movement_slid",
                              "true") &&
      nonNoneField(physicsCornerFields, "gameplay_movement_hit_surface_id") &&
      iggy3d::smoke::positiveIntegerField(
          physicsCornerFields,
          "product_draw_physics_contact_normal_debug_count") &&
      iggy3d::smoke::positiveIntegerField(
          physicsCornerFields,
          "product_render_bridge_physics_contact_normal_debug_count") &&
      iggy3d::smoke::hasField(physicsCornerFields, "session_outcome", "None");

  const bool wallPassed =
      wallExitCode == 0 && wallReceiptValid &&
      iggy3d::smoke::productReceipt(wallFields) &&
      iggy3d::smoke::hasField(wallFields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(wallFields, "window_created", "false") &&
      iggy3d::smoke::hasField(wallFields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(wallFields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(wallFields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(wallFields, "active_room_source", "package_room") &&
      iggy3d::smoke::hasField(wallFields,
                              "active_room_collision_ready",
                              "true") &&
      iggy3d::smoke::hasField(wallFields,
                              "active_room_collision_query_surface_count",
                              "28") &&
      iggy3d::smoke::hasField(wallFields,
                              "active_room_collision_active_door_blocker_count",
                              "0") &&
      iggy3d::smoke::hasField(wallFields,
                              "gameplay_tape_status",
                              "gameplay_tape_completed") &&
      iggy3d::smoke::hasField(wallFields,
                              "gameplay_tape_step_count",
                              "1") &&
      iggy3d::smoke::hasField(wallFields,
                              "gameplay_tape_executed_step_count",
                              "0") &&
      iggy3d::smoke::hasField(wallFields,
                              "gameplay_tape_expected_blocked_step_count",
                              "1") &&
      iggy3d::smoke::hasField(wallFields,
                              "gameplay_tape_last_action",
                              "move") &&
      iggy3d::smoke::hasField(wallFields,
                              "gameplay_tape_last_target",
                              "marker_key_r1_c3") &&
      iggy3d::smoke::hasField(wallFields,
                              "gameplay_tape_last_movement_block",
                              "blocked_by_collision") &&
      iggy3d::smoke::hasField(wallFields,
                              "gameplay_tape_failed_step",
                              "none") &&
      iggy3d::smoke::hasField(wallFields,
                              "gameplay_tape_loop_complete",
                              "false") &&
      iggy3d::smoke::hasField(wallFields,
                              "session_outcome",
                              "None");

  const bool npcPassed =
      npcExitCode == 0 && npcReceiptValid &&
      iggy3d::smoke::productReceipt(npcFields) &&
      iggy3d::smoke::hasField(npcFields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(npcFields, "window_created", "false") &&
      iggy3d::smoke::hasField(npcFields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(npcFields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(npcFields, "active_room_loaded", "true") &&
      iggy3d::smoke::hasField(npcFields, "active_room_source", "package_room") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_status",
                              "gameplay_tape_completed") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_step_count",
                              "1") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_executed_step_count",
                              "1") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_last_action",
                              "wait") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_last_target",
                              "none") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_npc_targetable",
                              "true") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_npc_defeated",
                              "false") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_ai_command_logged",
                              "true") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_ai_attack_logged",
                              "true") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_ai_wait_logged",
                              "false") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_ai_player_damaged",
                              "true") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_ai_player_hp_before",
                              "10") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_ai_player_hp_after",
                              "9") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_ai_actor_id",
                              "2") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_ai_target_id",
                              "1") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_ai_behavior",
                              "attacking") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_ai_intent",
                              "attack_target") &&
      iggy3d::smoke::hasField(npcFields,
                              "gameplay_tape_loop_complete",
                              "false") &&
      iggy3d::smoke::hasField(npcFields,
                              "session_outcome",
                              "None");

  const bool passiveNpcPassed =
      passiveNpcExitCode == 0 && passiveNpcReceiptValid &&
      iggy3d::smoke::productReceipt(passiveNpcFields) &&
      iggy3d::smoke::hasField(passiveNpcFields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(passiveNpcFields, "window_created", "false") &&
      iggy3d::smoke::hasField(passiveNpcFields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(passiveNpcFields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "selected_package_id",
                              "iggy3d.ascii_gameplay_loop") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "selected_scenario_id",
                              "ascii_gameplay_loop.runtime_loop") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "active_room_loaded",
                              "true") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "active_room_source",
                              "package_room") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_status",
                              "gameplay_tape_completed") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_step_count",
                              "1") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_executed_step_count",
                              "1") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_last_action",
                              "wait") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_ai_command_logged",
                              "true") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_ai_attack_logged",
                              "false") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_ai_wait_logged",
                              "true") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_ai_player_damaged",
                              "false") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_ai_player_hp_before",
                              "10") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_ai_player_hp_after",
                              "10") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_ai_actor_id",
                              "2") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_ai_target_id",
                              "1") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_ai_behavior",
                              "alert") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_ai_intent",
                              "wait") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "npc_behavior_debug_hud_visible",
                              "true") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "npc_behavior_debug_hud_line_count",
                              "2") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "npc_behavior_debug_hud_dev_tools_enabled",
                              "true") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "npc_behavior_debug_hud_debug_overlay_enabled",
                              "true") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "npc_behavior_debug_hud_debug_available",
                              "true") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "npc_behavior_debug_hud_status",
                              "npc_debug_ready") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "npc_behavior_debug_hud_reason_code",
                              "npc_debug_ready") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "npc_behavior_debug_hud_has_unresolved_profile",
                              "false") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "gameplay_tape_loop_complete",
                              "false") &&
      iggy3d::smoke::hasField(passiveNpcFields,
                              "session_outcome",
                              "None");

  const bool ghostNpcPassed =
      ghostNpcExitCode == 0 && ghostNpcReceiptValid &&
      iggy3d::smoke::productReceipt(ghostNpcFields) &&
      iggy3d::smoke::hasField(ghostNpcFields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(ghostNpcFields, "window_created", "false") &&
      iggy3d::smoke::hasField(ghostNpcFields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(ghostNpcFields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_requested",
                              "true") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_loaded",
                              "true") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_status",
                              "gameplay_tape_completed") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_step_count",
                              "1") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_executed_step_count",
                              "1") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_last_action",
                              "wait") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_ai_command_logged",
                              "false") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_ai_attack_logged",
                              "false") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_ai_wait_logged",
                              "false") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_ai_player_damaged",
                              "false") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_ai_player_hp_before",
                              "10") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_ai_player_hp_after",
                              "10") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_ai_behavior",
                              "none") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "gameplay_tape_ai_intent",
                              "none") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "npc_behavior_debug_hud_visible",
                              "true") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "npc_behavior_debug_hud_line_count",
                              "2") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "npc_behavior_debug_hud_debug_available",
                              "true") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "npc_behavior_debug_hud_status",
                              "npc_debug_ready") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "npc_behavior_debug_hud_reason_code",
                              "npc_debug_ready") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "npc_behavior_debug_hud_has_unresolved_profile",
                              "true") &&
      iggy3d::smoke::hasField(ghostNpcFields,
                              "session_outcome",
                              "None");

  const bool ok = expect(appAvailable, "app binary exists") &&
                  expect(packageGenerated, "generated ascii package") &&
                  expect(wallPackageGenerated, "generated wall ascii package") &&
                  expect(npcPackageGenerated, "generated npc ascii package") &&
                  expect(passiveNpcPackageGenerated,
                         "generated passive npc ascii package") &&
                  expect(ghostNpcPackageGenerated,
                         "generated ghost npc ascii package") &&
                  expect(receiptValid, "receipt valid") &&
                  expect(passed, "product gameplay tape pass") &&
                  expect(physicsControlGenerated,
                         "physics movement automation control generated") &&
                  expect(physicsTapeGenerated,
                         "physics movement tape generated") &&
                  expect(physicsReceiptValid,
                         "physics movement receipt valid") &&
                  expect(physicsPassed,
                         "product gameplay tape physics movement pass") &&
                  expect(physicsFlatControlGenerated,
                         "physics flat room control generated") &&
                  expect(physicsFlatTapeGenerated,
                         "physics flat room tape generated") &&
                  expect(physicsFlatReceiptValid,
                         "physics flat room receipt valid") &&
                  expect(physicsFlatRoomPassed,
                         "product selectable physics flat room movement pass") &&
                  expect(physicsCorridorControlGenerated,
                         "physics wall corridor control generated") &&
                  expect(physicsCorridorTapeGenerated,
                         "physics wall corridor tape generated") &&
                  expect(physicsCorridorReceiptValid,
                         "physics wall corridor receipt valid") &&
                  expect(physicsWallCorridorPassed,
                         "product selectable physics wall corridor movement pass") &&
                  expect(physicsCornerControlGenerated,
                         "physics corner slide control generated") &&
                  expect(physicsCornerTapeGenerated,
                         "physics corner slide tape generated") &&
                  expect(physicsCornerReceiptValid,
                         "physics corner slide receipt valid") &&
                  expect(physicsCornerSlidePassed,
                         "product selectable physics corner slide movement pass") &&
                  expect(wallReceiptValid, "wall receipt valid") &&
                  expect(wallPassed, "product package wall collision pass") &&
                  expect(npcReceiptValid, "npc receipt valid") &&
                  expect(npcPassed, "product package npc combat pass") &&
                  expect(passiveNpcReceiptValid, "passive npc receipt valid") &&
                  expect(passiveNpcPassed,
                         "product package passive npc profile pass") &&
                  expect(ghostNpcReceiptValid, "ghost npc receipt valid") &&
                  expect(ghostNpcPassed,
                         "product package ghost npc profile diagnostic pass");
  std::cout << "smoke=product_gameplay_tape\n";
  std::cout << "package_generated=" << (packageGenerated ? "true" : "false") << "\n";
  std::cout << "receipt_valid=" << (receiptValid ? "true" : "false") << "\n";
  std::cout << "physics_receipt_valid="
            << (physicsReceiptValid ? "true" : "false") << "\n";
  std::cout << "physics_flat_room_receipt_valid="
            << (physicsFlatReceiptValid ? "true" : "false") << "\n";
  std::cout << "physics_wall_corridor_receipt_valid="
            << (physicsCorridorReceiptValid ? "true" : "false") << "\n";
  std::cout << "physics_corner_slide_receipt_valid="
            << (physicsCornerReceiptValid ? "true" : "false") << "\n";
  std::cout << "wall_receipt_valid=" << (wallReceiptValid ? "true" : "false") << "\n";
  std::cout << "npc_receipt_valid=" << (npcReceiptValid ? "true" : "false") << "\n";
  std::cout << "passive_npc_receipt_valid="
            << (passiveNpcReceiptValid ? "true" : "false") << "\n";
  std::cout << "ghost_npc_receipt_valid="
            << (ghostNpcReceiptValid ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (ok ? "pass" : (appAvailable ? "fail" : "skip")) << "\n";
  std::cout << "reason_code="
            << (ok ? "product_gameplay_tape_pass"
                   : (appAvailable ? "product_gameplay_tape_failed"
                                   : "product_app_unavailable"))
            << "\n";
  if (ok) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}

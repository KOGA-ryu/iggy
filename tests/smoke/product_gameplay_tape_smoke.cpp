#include "ProductAutomationSmokeSupport.hpp"

#include "app/iggy3d/ProductAsciiRoomAuthoring.hpp"

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
                                      generatedScenarioText()) &&
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
  const std::filesystem::path saveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape");
  const std::filesystem::path wallSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape_wall_collision");
  const std::filesystem::path npcSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_tape_npc_combat");

  std::filesystem::path packagePath;
  std::filesystem::path tapePath;
  std::filesystem::path wallPackagePath;
  std::filesystem::path wallTapePath;
  std::filesystem::path npcPackagePath;
  std::filesystem::path npcTapePath;
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
      iggy3d::smoke::hasField(fields, "session_outcome", "Victory");

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

  const bool ok = expect(appAvailable, "app binary exists") &&
                  expect(packageGenerated, "generated ascii package") &&
                  expect(wallPackageGenerated, "generated wall ascii package") &&
                  expect(npcPackageGenerated, "generated npc ascii package") &&
                  expect(receiptValid, "receipt valid") &&
                  expect(passed, "product gameplay tape pass") &&
                  expect(wallReceiptValid, "wall receipt valid") &&
                  expect(wallPassed, "product package wall collision pass") &&
                  expect(npcReceiptValid, "npc receipt valid") &&
                  expect(npcPassed, "product package npc combat pass");
  std::cout << "smoke=product_gameplay_tape\n";
  std::cout << "package_generated=" << (packageGenerated ? "true" : "false") << "\n";
  std::cout << "receipt_valid=" << (receiptValid ? "true" : "false") << "\n";
  std::cout << "wall_receipt_valid=" << (wallReceiptValid ? "true" : "false") << "\n";
  std::cout << "npc_receipt_valid=" << (npcReceiptValid ? "true" : "false") << "\n";
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

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

bool asciiPackageSelectionFields(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::hasField(fields,
                                 "selected_package_id",
                                 "iggy3d.ascii_training_room") &&
         iggy3d::smoke::hasField(fields,
                                 "selected_scenario_id",
                                 "ascii_training_room.runtime_loop");
}

bool asciiPackageLoadedFields(const iggy3d::smoke::ReceiptFields& fields) {
  return iggy3d::smoke::hasField(fields, "package_load_status", "ok") &&
         asciiPackageSelectionFields(fields);
}

}  // namespace

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);

  const std::filesystem::path packagePath =
      "fixtures/demos/ascii_training_room/package.iggy3d.toml";
  const bool packageAvailable = std::filesystem::exists(packagePath);
  const std::filesystem::path saveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_package");
  const std::filesystem::path scriptedSaveRoot =
      iggy3d::smoke::cleanSaveRoot("ascii_package_scripted");

  const std::string packageArg =
      std::string{"--package "} + iggy3d::smoke::shellQuote(packagePath);

  int newWorldExitCode = 77;
  iggy3d::smoke::ReceiptFields newWorldFields;
  const bool newWorldReceiptValid =
      appAvailable && packageAvailable &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "ascii_package_new_world",
          packageArg + " --auto-new-world " + iggy3d::smoke::saveRootArg(saveRoot),
          newWorldFields,
          newWorldExitCode);

  int starterExitCode = 77;
  iggy3d::smoke::ReceiptFields starterFields;
  const bool starterReceiptValid =
      appAvailable && packageAvailable &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "ascii_package_starter",
          packageArg + " " + iggy3d::smoke::saveRootArg(saveRoot),
          starterFields,
          starterExitCode);

  int continueExitCode = 77;
  iggy3d::smoke::ReceiptFields continueFields;
  const bool continueReceiptValid =
      appAvailable && packageAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "ascii_package_continue",
          "frontend.select=continue\nfrontend.execute=true\n",
          packageArg + " " + iggy3d::smoke::saveRootArg(saveRoot),
          continueFields,
          continueExitCode);

  int scriptedExitCode = 77;
  iggy3d::smoke::ReceiptFields scriptedFields;
  const bool scriptedReceiptValid =
      appAvailable && packageAvailable &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "ascii_package_scripted",
          packageArg + " --scripted-gameplay-smoke " +
              iggy3d::smoke::saveRootArg(scriptedSaveRoot),
          scriptedFields,
          scriptedExitCode);

  const std::filesystem::path saveFile = saveRoot / "save_001.iggy3d.save";
  const bool newWorldPassed =
      newWorldExitCode == 0 && newWorldReceiptValid &&
      iggy3d::smoke::productReceipt(newWorldFields) &&
      iggy3d::smoke::hasField(newWorldFields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(newWorldFields, "window_created", "false") &&
      asciiPackageLoadedFields(newWorldFields) &&
      iggy3d::smoke::hasField(newWorldFields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(newWorldFields, "runtime_session_created", "true") &&
      iggy3d::smoke::hasField(newWorldFields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(newWorldFields,
                              "world_creation_status",
                              "world_creation_initial_save_written") &&
      iggy3d::smoke::hasField(newWorldFields,
                              "world_creation_initial_save_written",
                              "true") &&
      iggy3d::smoke::hasField(newWorldFields,
                              "product_save_status",
                              "product_save_written") &&
      iggy3d::smoke::hasField(newWorldFields,
                              "active_product_save_id",
                              "save_001") &&
      std::filesystem::exists(saveFile);

  const bool starterPassed =
      starterExitCode == 0 && starterReceiptValid &&
      iggy3d::smoke::productReceipt(starterFields) &&
      iggy3d::smoke::hasField(starterFields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(starterFields, "save_count", "1") &&
      iggy3d::smoke::hasField(starterFields, "compatible_save_count", "1") &&
      asciiPackageSelectionFields(starterFields);

  const bool continuePassed =
      continueExitCode == 0 && continueReceiptValid &&
      iggy3d::smoke::productReceipt(continueFields) &&
      iggy3d::smoke::automationApplied(continueFields) &&
      iggy3d::smoke::hasField(continueFields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(continueFields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(continueFields,
                              "product_save_load_status",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(continueFields,
                              "product_save_load_source",
                              "continue") &&
      iggy3d::smoke::hasField(continueFields,
                              "product_save_load_session_loaded",
                              "true") &&
      iggy3d::smoke::hasField(continueFields,
                              "active_product_save_id",
                              "save_001") &&
      asciiPackageLoadedFields(continueFields);

  const bool scriptedPassed =
      scriptedExitCode == 0 && scriptedReceiptValid &&
      iggy3d::smoke::productReceipt(scriptedFields) &&
      iggy3d::smoke::hasField(scriptedFields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(scriptedFields, "window_created", "false") &&
      asciiPackageLoadedFields(scriptedFields) &&
      iggy3d::smoke::hasField(scriptedFields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(scriptedFields, "runtime_session_created", "true") &&
      iggy3d::smoke::hasField(scriptedFields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "scripted_gameplay_smoke",
                              "true") &&
      iggy3d::smoke::hasField(scriptedFields, "scene_item_count", "5") &&
      iggy3d::smoke::hasField(scriptedFields, "player_visible", "true") &&
      iggy3d::smoke::hasField(scriptedFields, "objective_visible", "true") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_draw_item_count",
                              "41") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_render_bridge_ready",
                              "true") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_vulkan_room_mesh_cpu_ready",
                              "true") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_vulkan_room_mesh_backend_presented",
                              "false") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_vulkan_room_mesh_source",
                              "scene_room_projection") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_vulkan_room_asset_id",
                              "training_room_ascii") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_vulkan_room_floor_visible",
                              "true") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_vulkan_room_wall_visible",
                              "true") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_vulkan_room_source_mesh_count",
                              "35") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_vulkan_room_vertex_count",
                              "280") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_vulkan_room_index_count",
                              "2520") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_vulkan_room_draw_count",
                              "35") &&
      iggy3d::smoke::positiveIntegerField(
          scriptedFields, "product_vulkan_room_geometry_signature") &&
      iggy3d::smoke::hasField(scriptedFields, "target_discovered", "true") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "gameplay_command_kind",
                              "attack") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "gameplay_command_status",
                              "accepted") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "gameplay_command_accepted",
                              "true") &&
      iggy3d::smoke::hasField(scriptedFields, "gameplay_reach_gate", "pass") &&
      iggy3d::smoke::hasField(scriptedFields, "gameplay_last_rejection", "none") &&
      iggy3d::smoke::hasField(scriptedFields, "attack_executed", "false") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_feedback_visible",
                              "true") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_feedback_command_kind",
                              "attack") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_feedback_command_status",
                              "accepted") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_feedback_rejection_reason",
                              "none") &&
      iggy3d::smoke::hasField(scriptedFields,
                              "product_feedback_attack_visible",
                              "true");

  const bool ok = expect(appAvailable, "app binary exists") &&
                  expect(packageAvailable, "ascii package exists") &&
                  expect(newWorldReceiptValid, "new world receipt valid") &&
                  expect(newWorldPassed, "new world ascii package pass") &&
                  expect(starterReceiptValid, "starter receipt valid") &&
                  expect(starterPassed, "starter scan ascii package pass") &&
                  expect(continueReceiptValid, "continue receipt valid") &&
                  expect(continuePassed, "continue ascii package pass") &&
                  expect(scriptedReceiptValid, "scripted receipt valid") &&
                  expect(scriptedPassed, "scripted ascii package pass");

  std::cout << "smoke=product_ascii_package\n";
  std::cout << "new_world=" << (newWorldPassed ? "true" : "false") << "\n";
  std::cout << "starter=" << (starterPassed ? "true" : "false") << "\n";
  std::cout << "continue_load=" << (continuePassed ? "true" : "false") << "\n";
  std::cout << "scripted_attack=" << (scriptedPassed ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (ok ? "pass" : (appAvailable ? "fail" : "skip")) << "\n";
  std::cout << "reason_code="
            << (ok ? "product_ascii_package_pass"
                   : (appAvailable ? "product_ascii_package_failed"
                                   : "product_app_unavailable"))
            << "\n";
  if (ok) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}

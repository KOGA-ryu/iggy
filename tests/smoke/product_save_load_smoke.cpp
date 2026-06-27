#include "AutomationSmokeSupport.hpp"

#include <filesystem>
#include <iostream>
#include <system_error>

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);

  int exitCode = 77;
  iggy3d::smoke::ReceiptFields fields;
  const std::filesystem::path saveRoot =
      iggy3d::smoke::cleanSaveRoot("save_load");
  const bool seed =
      appAvailable && iggy3d::smoke::seedWorldSave(binary, "save_load_seed",
                                                   saveRoot, "Save Load World");

  const bool continueLoad =
      appAvailable && seed &&
      iggy3d::smoke::runProductCase(
          binary,
          "continue_load",
          "frontend.select=continue\nfrontend.execute=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "continue") &&
      iggy3d::smoke::hasField(fields, "frontend_launch_requested", "true") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_reason_code",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_load_source", "continue") &&
      iggy3d::smoke::hasField(fields, "product_save_load_selected_id",
                              "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_load_selected_enabled",
                              "true") &&
      iggy3d::smoke::hasField(fields, "product_save_load_session_loaded",
                              "true") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_save_load_loaded_hash") &&
      iggy3d::smoke::hasField(fields, "world_creation_status", "not_requested") &&
      iggy3d::smoke::hasField(fields, "product_save_status", "not_requested") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_transition_last_action",
                              "launch_gameplay") &&
      iggy3d::smoke::hasField(fields, "product_transition_status",
                              "gameplay_active");

  fields.clear();
  const bool loadSaveSelector =
      appAvailable && seed &&
      iggy3d::smoke::runProductCase(
          binary,
          "load_save_selector",
          "frontend.select=load_save\nfrontend.execute=true\nmenu.confirm=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "load") &&
      iggy3d::smoke::hasField(fields, "frontend_launch_requested", "true") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_reason_code",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_load_source",
                              "load_save_selector") &&
      iggy3d::smoke::hasField(fields, "product_save_load_selected_id",
                              "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_load_selected_enabled",
                              "true") &&
      iggy3d::smoke::hasField(fields, "product_save_load_session_loaded",
                              "true") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_save_load_loaded_hash") &&
      iggy3d::smoke::hasField(fields, "world_creation_status", "not_requested") &&
      iggy3d::smoke::hasField(fields, "product_save_status", "not_requested") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_transition_last_action",
                              "launch_gameplay") &&
      iggy3d::smoke::hasField(fields, "product_transition_status",
                              "gameplay_active");

  std::error_code copyError;
  const bool secondSaveReady =
      seed && std::filesystem::copy_file(
                  saveRoot / "save_001.iggy3d.save",
                  saveRoot / "save_002.iggy3d.save",
                  std::filesystem::copy_options::overwrite_existing,
                  copyError) &&
      !copyError;

  fields.clear();
  const bool loadSaveSelectorSelected =
      appAvailable && secondSaveReady &&
      iggy3d::smoke::runProductCase(
          binary,
          "load_save_selector_selected",
          "frontend.select=load_save\nfrontend.execute=true\n"
          "save.select=save_002\nmenu.confirm=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "load") &&
      iggy3d::smoke::hasField(fields, "frontend_launch_requested", "true") &&
      iggy3d::smoke::hasField(fields, "save_count", "2") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "2") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "selected_save_id", "save_002") &&
      iggy3d::smoke::hasField(fields, "selected_save_enabled", "true") &&
      iggy3d::smoke::hasField(fields, "selected_save_status", "selected") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_reason_code",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_save_id", "save_002") &&
      iggy3d::smoke::hasField(fields, "product_save_load_source",
                              "load_save_selector") &&
      iggy3d::smoke::hasField(fields, "product_save_load_selected_id",
                              "save_002") &&
      iggy3d::smoke::hasField(fields, "product_save_load_selected_enabled",
                              "true") &&
      iggy3d::smoke::hasField(fields, "product_save_load_session_loaded",
                              "true") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_save_load_loaded_hash") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_002") &&
      iggy3d::smoke::hasField(fields, "product_transition_status",
                              "gameplay_active");

  const bool corruptSaveReady =
      iggy3d::smoke::writeTextFile(saveRoot / "corrupt.iggy3d.save",
                                   "not_an_iggy3d_save_envelope\n");

  fields.clear();
  const bool loadSaveSelectorCorruptRejected =
      appAvailable && corruptSaveReady &&
      iggy3d::smoke::runProductCase(
          binary,
          "load_save_selector_corrupt_rejected",
          "frontend.select=load_save\nfrontend.execute=true\n"
          "save.select=corrupt\nmenu.confirm=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "load_save") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "load") &&
      iggy3d::smoke::hasField(fields, "frontend_launch_requested", "false") &&
      iggy3d::smoke::hasField(fields, "save_count", "3") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "2") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "selected_save_id", "corrupt") &&
      iggy3d::smoke::hasField(fields, "selected_save_enabled", "false") &&
      iggy3d::smoke::hasField(fields, "selected_save_status", "disabled") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "save_file_decode_failed") &&
      iggy3d::smoke::hasField(fields, "product_save_load_reason_code",
                              "save_file_decode_failed") &&
      iggy3d::smoke::hasField(fields, "product_save_load_source",
                              "load_save_selector") &&
      iggy3d::smoke::hasField(fields, "product_save_load_selected_id",
                              "corrupt") &&
      iggy3d::smoke::hasField(fields, "product_save_load_selected_enabled",
                              "false") &&
      iggy3d::smoke::hasField(fields, "product_save_load_session_loaded",
                              "false") &&
      iggy3d::smoke::hasField(fields, "product_save_load_loaded_hash", "0") &&
      iggy3d::smoke::hasField(fields, "world_creation_status", "not_requested") &&
      iggy3d::smoke::hasField(fields, "product_save_status", "not_requested");

  const bool passed = seed && continueLoad && loadSaveSelector &&
                      loadSaveSelectorSelected && loadSaveSelectorCorruptRejected;
  std::cout << "smoke=product_save_load\n";
  std::cout << "seed=" << (seed ? "true" : "false") << "\n";
  std::cout << "continue_load=" << (continueLoad ? "true" : "false") << "\n";
  std::cout << "load_save_selector="
            << (loadSaveSelector ? "true" : "false") << "\n";
  std::cout << "load_save_selector_selected="
            << (loadSaveSelectorSelected ? "true" : "false") << "\n";
  std::cout << "load_save_selector_corrupt_rejected="
            << (loadSaveSelectorCorruptRejected ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (appAvailable ? "fail" : "skip"))
            << "\n";
  std::cout << "reason_code="
            << (passed ? "product_save_load_pass"
                       : (appAvailable ? "product_save_load_failed"
                                       : "product_app_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}

#include "ProductAutomationSmokeSupport.hpp"

#include <filesystem>
#include <iostream>

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

  std::cout << "smoke=product_world_setup\n";
  std::cout << "new_world=" << (newWorld ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result="
            << (newWorld ? "pass" : (appAvailable ? "fail" : "skip")) << "\n";
  std::cout << "reason_code="
            << (newWorld ? "product_world_setup_pass"
                         : (appAvailable ? "product_world_setup_failed"
                                         : "product_app_unavailable"))
            << "\n";
  if (newWorld) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}

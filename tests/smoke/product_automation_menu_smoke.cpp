#include "ProductAutomationSmokeSupport.hpp"

#include <filesystem>
#include <iostream>

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);

  int exitCode = 77;
  iggy3d::smoke::ReceiptFields fields;

  const bool starterSettings =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "starter_settings",
          "frontend.select=settings\nfrontend.execute=true\nsettings.tab=audio\n",
          "",
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "settings") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "settings") &&
      iggy3d::smoke::hasField(fields, "settings_selected_tab", "audio") &&
      iggy3d::smoke::hasField(fields, "input_owner", "settings") &&
      iggy3d::smoke::hasField(fields, "gameplay_input_suppressed", "true");

  fields.clear();
  const bool starterDevTools =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "starter_dev_tools",
          "frontend.select=dev_tools\nfrontend.execute=true\n"
          "dev_tools.category=input\n",
          "",
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen",
                              "starter_dev_tools") &&
      iggy3d::smoke::hasField(fields, "dev_tools_open", "true") &&
      iggy3d::smoke::hasField(fields, "dev_tools_category", "input") &&
      iggy3d::smoke::hasField(fields, "input_owner", "dev_tools") &&
      iggy3d::smoke::hasField(fields, "gameplay_input_suppressed", "true");

  fields.clear();
  const bool invalidValue =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary, "invalid_value", "menu.input=teleport\n", "", fields, exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::hasField(fields, "automation_control_requested", "true") &&
      iggy3d::smoke::hasField(fields, "automation_control_loaded", "false") &&
      iggy3d::smoke::hasField(fields, "automation_control_status",
                              "invalid_value") &&
      iggy3d::smoke::hasField(fields, "automation_control_scope",
                              "frontend_menu");

  const bool passed = starterSettings && starterDevTools && invalidValue;
  std::cout << "smoke=product_automation_menu\n";
  std::cout << "starter_settings=" << (starterSettings ? "true" : "false")
            << "\n";
  std::cout << "starter_dev_tools=" << (starterDevTools ? "true" : "false")
            << "\n";
  std::cout << "invalid_value=" << (invalidValue ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (appAvailable ? "fail" : "skip"))
            << "\n";
  std::cout << "reason_code="
            << (passed ? "product_automation_menu_pass"
                       : (appAvailable ? "product_automation_menu_failed"
                                       : "product_app_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}

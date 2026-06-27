#include "ProductAutomationSmokeSupport.hpp"

#include <filesystem>
#include <iostream>

namespace {

bool controllerMappedTo(const iggy3d::smoke::ReceiptFields& fields,
                        std::string_view control,
                        std::string_view mode,
                        std::string_view surface,
                        std::string_view action) {
  return iggy3d::smoke::hasField(fields, "controller_action_mapped", "true") &&
         iggy3d::smoke::hasField(fields, "controller_action_status",
                                 "controller_action_mapped") &&
         iggy3d::smoke::hasField(fields, "controller_action_reason_code",
                                 "controller_action_mapped") &&
         iggy3d::smoke::hasField(fields, "controller_action_control", control) &&
         iggy3d::smoke::hasField(fields, "controller_action_mode", mode) &&
         iggy3d::smoke::hasField(fields, "controller_action_surface", surface) &&
         iggy3d::smoke::hasField(fields, "controller_action_input_action",
                                 action);
}

bool controllerChordConsumed(const iggy3d::smoke::ReceiptFields& fields,
                             std::string_view mode,
                             std::string_view surface) {
  return iggy3d::smoke::hasField(fields, "controller_action_mapped", "false") &&
         iggy3d::smoke::hasField(fields, "controller_action_status",
                                 "controller_action_chord_consumed") &&
         iggy3d::smoke::hasField(fields, "controller_action_reason_code",
                                 "controller_action_chord_consumed") &&
         iggy3d::smoke::hasField(fields, "controller_action_control", "none") &&
         iggy3d::smoke::hasField(fields, "controller_action_mode", mode) &&
         iggy3d::smoke::hasField(fields, "controller_action_surface", surface) &&
         iggy3d::smoke::hasField(fields, "controller_action_input_action",
                                 "none");
}

bool toggleState(const iggy3d::smoke::ReceiptFields& fields,
                 std::string_view mode,
                 std::string_view requested,
                 std::string_view accepted,
                 std::string_view status,
                 std::string_view surface) {
  return iggy3d::smoke::hasField(fields, "interaction_mode", mode) &&
         iggy3d::smoke::hasField(fields, "controller_mode_toggle_requested",
                                 requested) &&
         iggy3d::smoke::hasField(fields, "controller_mode_toggle_accepted",
                                 accepted) &&
         iggy3d::smoke::hasField(fields, "controller_mode_toggle_status",
                                 status) &&
         iggy3d::smoke::hasField(fields, "controller_mode_toggle_reason_code",
                                 status) &&
         iggy3d::smoke::hasField(fields, "controller_mode_toggle_surface",
                                 surface);
}

}  // namespace

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);
  int exitCode = 77;
  iggy3d::smoke::ReceiptFields fields;

  const std::filesystem::path playerSaveRoot =
      iggy3d::smoke::cleanSaveRoot("controller_input_player_gameplay");
  const bool playerGameplay =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "controller_input_player_gameplay",
          "frontend.select=new_world\nfrontend.execute=true\nworld.create=true\n"
          "controller.input=left_stick_up\n",
          iggy3d::smoke::saveRootArg(playerSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      toggleState(fields, "player", "false", "false",
                  "interaction_mode_chord_partial", "gameplay") &&
      controllerMappedTo(fields, "left_stick_up", "player", "gameplay",
                         "game.move_y") &&
      iggy3d::smoke::hasField(fields, "gameplay_input_source", "controller") &&
      iggy3d::smoke::hasField(fields, "gameplay_command_submitted", "true") &&
      iggy3d::smoke::hasField(fields, "gameplay_command_kind", "move") &&
      iggy3d::smoke::hasField(fields, "room_editor_cursor_ready", "false");

  fields.clear();
  const std::filesystem::path chordSaveRoot =
      iggy3d::smoke::cleanSaveRoot("controller_input_mode_chord");
  const bool chordToggle =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "controller_input_mode_chord",
          "frontend.select=new_world\nfrontend.execute=true\nworld.create=true\n"
          "controller.input=mode_chord\n",
          iggy3d::smoke::saveRootArg(chordSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      toggleState(fields, "creative", "true", "true",
                  "interaction_mode_toggled", "gameplay") &&
      controllerChordConsumed(fields, "creative", "gameplay");

  fields.clear();
  const std::filesystem::path chordBackSaveRoot =
      iggy3d::smoke::cleanSaveRoot("controller_input_mode_chord_back");
  const bool chordToggleBack =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "controller_input_mode_chord_back",
          "frontend.select=new_world\nfrontend.execute=true\nworld.create=true\n"
          "controller.input=mode_chord,release,mode_chord\n",
          iggy3d::smoke::saveRootArg(chordBackSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      toggleState(fields, "player", "true", "true",
                  "interaction_mode_toggled", "gameplay") &&
      controllerChordConsumed(fields, "player", "gameplay");

  fields.clear();
  const std::filesystem::path creativeSaveRoot =
      iggy3d::smoke::cleanSaveRoot("controller_input_creative_editor");
  const bool creativeEditor =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "controller_input_creative_editor",
          "frontend.select=new_world\nfrontend.execute=true\nworld.create=true\n"
          "room_edit.start_active=true\n"
          "controller.input=mode_chord,release,dpad_right\n",
          iggy3d::smoke::saveRootArg(creativeSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "interaction_mode", "creative") &&
      controllerMappedTo(fields, "dpad_right", "creative", "room_editor",
                         "editor.nudge_x") &&
      iggy3d::smoke::hasField(fields, "input_owner", "editor") &&
      iggy3d::smoke::hasField(fields, "gameplay_input_suppressed", "true") &&
      iggy3d::smoke::hasField(fields, "room_editor_cursor_ready", "true") &&
      iggy3d::smoke::hasField(fields, "room_editor_grid_x", "1") &&
      iggy3d::smoke::hasField(fields, "room_editor_status",
                              "room_editor_cursor_moved");

  fields.clear();
  const bool blockedStarter =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "controller_input_blocked_starter",
          "controller.input=mode_chord\n",
          "",
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      toggleState(fields, "player", "true", "false",
                  "interaction_mode_surface_blocked", "starter") &&
      controllerChordConsumed(fields, "player", "starter");

  const bool passed = playerGameplay && chordToggle && chordToggleBack &&
                      creativeEditor && blockedStarter;
  std::cout << "smoke=product_controller_input\n";
  std::cout << "player_gameplay=" << (playerGameplay ? "true" : "false")
            << "\n";
  std::cout << "chord_toggle=" << (chordToggle ? "true" : "false") << "\n";
  std::cout << "chord_toggle_back=" << (chordToggleBack ? "true" : "false")
            << "\n";
  std::cout << "creative_editor=" << (creativeEditor ? "true" : "false")
            << "\n";
  std::cout << "blocked_starter=" << (blockedStarter ? "true" : "false")
            << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (appAvailable ? "fail" : "skip"))
            << "\n";
  std::cout << "reason_code="
            << (passed ? "product_controller_input_pass"
                       : (appAvailable ? "product_controller_input_failed"
                                       : "product_app_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}

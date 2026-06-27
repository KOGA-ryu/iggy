#include "app/iggy3d/input/ControllerActionMap.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool expectMapped(iggy3d::ProductInputSurface surface,
                  iggy3d::ProductInteractionMode mode,
                  iggy3d::ProductControllerControl control,
                  iggy3d::InputAction action,
                  float value,
                  std::string_view message) {
  const iggy3d::ProductControllerActionMapResult result =
      iggy3d::mapProductControllerAction({surface, mode, control});
  return expect(result.mapped, message) &&
         expect(result.action == action, message) &&
         expect(std::fabs(result.actionValue - value) < 0.001F, message) &&
         expect(result.status == "controller_action_mapped", message) &&
         expect(result.reasonCode == "controller_action_mapped", message);
}

bool expectUnmapped(iggy3d::ProductInputSurface surface,
                    iggy3d::ProductInteractionMode mode,
                    iggy3d::ProductControllerControl control,
                    std::string_view status,
                    std::string_view message) {
  const iggy3d::ProductControllerActionMapResult result =
      iggy3d::mapProductControllerAction({surface, mode, control});
  return expect(!result.mapped, message) &&
         expect(result.action == iggy3d::InputAction::None, message) &&
         expect(result.actionValue == 0.0F, message) &&
         expect(result.status == status, message) &&
         expect(result.reasonCode == status, message);
}

bool namesAreStable() {
  struct ControlNameCase {
    iggy3d::ProductControllerControl control;
    std::string_view name;
  };
  constexpr std::array cases{
      ControlNameCase{iggy3d::ProductControllerControl::None, "none"},
      ControlNameCase{iggy3d::ProductControllerControl::LeftStickUp,
                      "left_stick_up"},
      ControlNameCase{iggy3d::ProductControllerControl::LeftStickDown,
                      "left_stick_down"},
      ControlNameCase{iggy3d::ProductControllerControl::LeftStickLeft,
                      "left_stick_left"},
      ControlNameCase{iggy3d::ProductControllerControl::LeftStickRight,
                      "left_stick_right"},
      ControlNameCase{iggy3d::ProductControllerControl::RightStickUp,
                      "right_stick_up"},
      ControlNameCase{iggy3d::ProductControllerControl::RightStickDown,
                      "right_stick_down"},
      ControlNameCase{iggy3d::ProductControllerControl::RightStickLeft,
                      "right_stick_left"},
      ControlNameCase{iggy3d::ProductControllerControl::RightStickRight,
                      "right_stick_right"},
      ControlNameCase{iggy3d::ProductControllerControl::DpadUp, "dpad_up"},
      ControlNameCase{iggy3d::ProductControllerControl::DpadDown,
                      "dpad_down"},
      ControlNameCase{iggy3d::ProductControllerControl::DpadLeft,
                      "dpad_left"},
      ControlNameCase{iggy3d::ProductControllerControl::DpadRight,
                      "dpad_right"},
      ControlNameCase{iggy3d::ProductControllerControl::SouthButton,
                      "south_button"},
      ControlNameCase{iggy3d::ProductControllerControl::EastButton,
                      "east_button"},
      ControlNameCase{iggy3d::ProductControllerControl::WestButton,
                      "west_button"},
      ControlNameCase{iggy3d::ProductControllerControl::NorthButton,
                      "north_button"},
      ControlNameCase{iggy3d::ProductControllerControl::LeftShoulder,
                      "left_shoulder"},
      ControlNameCase{iggy3d::ProductControllerControl::RightShoulder,
                      "right_shoulder"},
      ControlNameCase{iggy3d::ProductControllerControl::LeftTrigger,
                      "left_trigger"},
      ControlNameCase{iggy3d::ProductControllerControl::RightTrigger,
                      "right_trigger"},
      ControlNameCase{iggy3d::ProductControllerControl::LeftStickPress,
                      "left_stick_press"},
      ControlNameCase{iggy3d::ProductControllerControl::RightStickPress,
                      "right_stick_press"},
  };

  bool ok = true;
  for (const ControlNameCase row : cases) {
    ok = expect(iggy3d::productControllerControlName(row.control) == row.name,
                "controller control name is stable") &&
         ok;
  }
  return ok;
}

bool playerGameplayMappingsAreStable() {
  return expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Player,
                      iggy3d::ProductControllerControl::LeftStickUp,
                      iggy3d::InputAction::PlayerMoveY,
                      1.0F,
                      "player left stick up maps to forward") &&
         expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Player,
                      iggy3d::ProductControllerControl::LeftStickLeft,
                      iggy3d::InputAction::PlayerMoveX,
                      -1.0F,
                      "player left stick left maps to strafe left") &&
         expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Player,
                      iggy3d::ProductControllerControl::RightStickUp,
                      iggy3d::InputAction::PlayerLookY,
                      -1.0F,
                      "player right stick up maps to look up") &&
         expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Player,
                      iggy3d::ProductControllerControl::RightStickRight,
                      iggy3d::InputAction::PlayerLookX,
                      1.0F,
                      "player right stick right maps to look right") &&
         expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Player,
                      iggy3d::ProductControllerControl::SouthButton,
                      iggy3d::InputAction::PlayerInteract,
                      1.0F,
                      "player south button maps to interact");
}

bool creativeGameplayMappingsAreStable() {
  return expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Creative,
                      iggy3d::ProductControllerControl::LeftStickUp,
                      iggy3d::InputAction::EditorNudgeZ,
                      -1.0F,
                      "creative left stick up maps to editor nudge") &&
         expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Creative,
                      iggy3d::ProductControllerControl::DpadRight,
                      iggy3d::InputAction::EditorNudgeX,
                      1.0F,
                      "creative dpad right maps to editor nudge") &&
         expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Creative,
                      iggy3d::ProductControllerControl::WestButton,
                      iggy3d::InputAction::EditorPreviewPlacement,
                      1.0F,
                      "creative west button maps to preview") &&
         expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Creative,
                      iggy3d::ProductControllerControl::SouthButton,
                      iggy3d::InputAction::EditorConfirmPreview,
                      1.0F,
                      "creative south button maps to confirm preview") &&
         expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Creative,
                      iggy3d::ProductControllerControl::EastButton,
                      iggy3d::InputAction::EditorCancelPreview,
                      1.0F,
                      "creative east button maps to cancel preview") &&
         expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Creative,
                      iggy3d::ProductControllerControl::NorthButton,
                      iggy3d::InputAction::EditorRotateWallDirection,
                      1.0F,
                      "creative north button maps to wall direction") &&
         expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Creative,
                      iggy3d::ProductControllerControl::LeftShoulder,
                      iggy3d::InputAction::EditorPreviousTool,
                      1.0F,
                      "creative left shoulder maps to previous tool") &&
         expectMapped(iggy3d::ProductInputSurface::Gameplay,
                      iggy3d::ProductInteractionMode::Creative,
                      iggy3d::ProductControllerControl::RightShoulder,
                      iggy3d::InputAction::EditorNextTool,
                      1.0F,
                      "creative right shoulder maps to next tool");
}

bool roomEditorSurfaceUsesEditorMappings() {
  return expectMapped(iggy3d::ProductInputSurface::RoomEditor,
                      iggy3d::ProductInteractionMode::Player,
                      iggy3d::ProductControllerControl::LeftStickRight,
                      iggy3d::InputAction::EditorNudgeX,
                      1.0F,
                      "room editor player mode uses editor nudge") &&
         expectMapped(iggy3d::ProductInputSurface::RoomEditor,
                      iggy3d::ProductInteractionMode::Creative,
                      iggy3d::ProductControllerControl::SouthButton,
                      iggy3d::InputAction::EditorConfirmPreview,
                      1.0F,
                      "room editor creative mode uses editor confirm");
}

bool blockedSurfacesStayUnmapped() {
  constexpr std::array surfaces{
      iggy3d::ProductInputSurface::None,
      iggy3d::ProductInputSurface::Starter,
      iggy3d::ProductInputSurface::Pause,
      iggy3d::ProductInputSurface::Settings,
      iggy3d::ProductInputSurface::DevTools,
      iggy3d::ProductInputSurface::SaveBrowser,
      iggy3d::ProductInputSurface::WorldSetup,
  };

  bool ok = true;
  for (const iggy3d::ProductInputSurface surface : surfaces) {
    ok = expectUnmapped(surface,
                        iggy3d::ProductInteractionMode::Player,
                        iggy3d::ProductControllerControl::SouthButton,
                        "controller_action_unmapped",
                        "blocked surface leaves south button unmapped") &&
         ok;
  }
  return ok;
}

bool playerAndCreativeDifferForSameControl() {
  const iggy3d::ProductControllerActionMapResult player =
      iggy3d::mapProductControllerAction(
          {iggy3d::ProductInputSurface::Gameplay,
           iggy3d::ProductInteractionMode::Player,
           iggy3d::ProductControllerControl::SouthButton});
  const iggy3d::ProductControllerActionMapResult creative =
      iggy3d::mapProductControllerAction(
          {iggy3d::ProductInputSurface::Gameplay,
           iggy3d::ProductInteractionMode::Creative,
           iggy3d::ProductControllerControl::SouthButton});

  return expect(player.mapped, "player south button mapped") &&
         expect(creative.mapped, "creative south button mapped") &&
         expect(player.action == iggy3d::InputAction::PlayerInteract,
                "player south action") &&
         expect(creative.action == iggy3d::InputAction::EditorConfirmPreview,
                "creative south action");
}

bool chordComponentControlsAreReserved() {
  constexpr std::array controls{
      iggy3d::ProductControllerControl::LeftTrigger,
      iggy3d::ProductControllerControl::RightTrigger,
      iggy3d::ProductControllerControl::LeftStickPress,
      iggy3d::ProductControllerControl::RightStickPress,
  };

  bool ok = true;
  for (const iggy3d::ProductControllerControl control : controls) {
    ok = expect(iggy3d::productControllerControlIsModeChordComponent(control),
                "chord component marked reserved") &&
         expectUnmapped(iggy3d::ProductInputSurface::Gameplay,
                        iggy3d::ProductInteractionMode::Player,
                        control,
                        "controller_action_chord_reserved",
                        "chord component is reserved") &&
         ok;
  }
  return ok;
}

bool missingRowsAreStable() {
  return expectUnmapped(iggy3d::ProductInputSurface::Gameplay,
                        iggy3d::ProductInteractionMode::Player,
                        iggy3d::ProductControllerControl::DpadUp,
                        "controller_action_unmapped",
                        "player gameplay dpad is unmapped") &&
         expectUnmapped(iggy3d::ProductInputSurface::Gameplay,
                        iggy3d::ProductInteractionMode::Creative,
                        iggy3d::ProductControllerControl::RightStickUp,
                        "controller_action_unmapped",
                        "creative gameplay right stick is unmapped");
}

}  // namespace

int main() {
  const bool ok = namesAreStable() && playerGameplayMappingsAreStable() &&
                  creativeGameplayMappingsAreStable() &&
                  roomEditorSurfaceUsesEditorMappings() &&
                  blockedSurfacesStayUnmapped() &&
                  playerAndCreativeDifferForSameControl() &&
                  chordComponentControlsAreReserved() && missingRowsAreStable();
  return ok ? 0 : 1;
}

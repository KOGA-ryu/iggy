#include "app/iggy3d/input/ControllerActionRouting.hpp"

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

bool expectSingleAction(const iggy3d::ActionState& actions,
                        iggy3d::InputAction action,
                        bool pressed,
                        float value,
                        std::string_view message) {
  return expect(actions.entries.size() == 1U, message) &&
         expect(actions.entries.front().action == action, message) &&
         expect(actions.entries.front().down, message) &&
         expect(actions.entries.front().pressed == pressed, message) &&
         expect(std::fabs(actions.entries.front().value - value) < 0.001F,
                message);
}

bool playerGameplayMapsToGameplayActions() {
  iggy3d::ProductControllerActionRoutingState state;
  iggy3d::ActionState actions;
  iggy3d::GamepadControllerActionSample sample;
  sample.leftStickY = 0.75F;

  const iggy3d::ProductControllerActionRoutingResult result =
      iggy3d::recordProductControllerMappedActions({
          iggy3d::ProductInputSurface::Gameplay,
          iggy3d::ProductInteractionMode::Player,
          sample,
          state,
          actions,
      });

  return expect(result.mapped, "player gameplay mapped") &&
         expect(result.mappedCount == 1U, "player gameplay mapped count") &&
         expect(result.control == iggy3d::ProductControllerControl::LeftStickUp,
                "player gameplay control") &&
         expect(result.action == iggy3d::InputAction::PlayerMoveY,
                "player gameplay action") &&
         expect(result.status == "controller_action_mapped",
                "player gameplay status") &&
         expectSingleAction(actions,
                            iggy3d::InputAction::PlayerMoveY,
                            false,
                            0.75F,
                            "player gameplay records movement") &&
         expect(!iggy3d::actionWasPressed(actions,
                                          iggy3d::InputAction::EditorNudgeZ),
                "player gameplay does not emit editor nudge");
}

bool creativeGameplayMapsToEditorActions() {
  iggy3d::ProductControllerActionRoutingState state;
  iggy3d::ActionState actions;
  iggy3d::GamepadControllerActionSample sample;
  sample.leftStickY = 1.0F;

  const iggy3d::ProductControllerActionRoutingResult result =
      iggy3d::recordProductControllerMappedActions({
          iggy3d::ProductInputSurface::Gameplay,
          iggy3d::ProductInteractionMode::Creative,
          sample,
          state,
          actions,
      });

  return expect(result.mapped, "creative gameplay mapped") &&
         expect(result.action == iggy3d::InputAction::EditorNudgeZ,
                "creative gameplay action") &&
         expectSingleAction(actions,
                            iggy3d::InputAction::EditorNudgeZ,
                            true,
                            -1.0F,
                            "creative gameplay records editor nudge") &&
         expect(iggy3d::actionAxisValue(actions,
                                        iggy3d::InputAction::PlayerMoveY) ==
                    0.0F,
                "creative gameplay does not emit player movement");
}

bool roomEditorSurfaceMapsToEditorRegardlessOfMode() {
  iggy3d::ProductControllerActionRoutingState state;
  iggy3d::ActionState actions;
  iggy3d::GamepadControllerActionSample sample;
  sample.dpadRightDown = true;

  const iggy3d::ProductControllerActionRoutingResult result =
      iggy3d::recordProductControllerMappedActions({
          iggy3d::ProductInputSurface::RoomEditor,
          iggy3d::ProductInteractionMode::Player,
          sample,
          state,
          actions,
      });

  return expect(result.mapped, "room editor player mode mapped") &&
         expect(result.action == iggy3d::InputAction::EditorNudgeX,
                "room editor action") &&
         expectSingleAction(actions,
                            iggy3d::InputAction::EditorNudgeX,
                            true,
                            1.0F,
                            "room editor records editor nudge");
}

bool heldEditorControlDoesNotRepeat() {
  iggy3d::ProductControllerActionRoutingState state;
  iggy3d::GamepadControllerActionSample sample;
  sample.southButtonDown = true;

  iggy3d::ActionState firstActions;
  const iggy3d::ProductControllerActionRoutingResult first =
      iggy3d::recordProductControllerMappedActions({
          iggy3d::ProductInputSurface::Gameplay,
          iggy3d::ProductInteractionMode::Creative,
          sample,
          state,
          firstActions,
      });
  iggy3d::ActionState heldActions;
  const iggy3d::ProductControllerActionRoutingResult held =
      iggy3d::recordProductControllerMappedActions({
          iggy3d::ProductInputSurface::Gameplay,
          iggy3d::ProductInteractionMode::Creative,
          sample,
          state,
          heldActions,
      });

  return expect(first.mapped, "first editor button mapped") &&
         expectSingleAction(firstActions,
                            iggy3d::InputAction::EditorConfirmPreview,
                            true,
                            1.0F,
                            "first editor button records confirm") &&
         expect(!held.mapped, "held editor button not remapped") &&
         expect(held.status == "controller_action_held",
                "held editor button status") &&
         expect(heldActions.entries.empty(), "held editor button emits nothing");
}

bool chordControlsRemainReserved() {
  iggy3d::ProductControllerActionRoutingState state;
  iggy3d::ActionState actions;
  iggy3d::GamepadControllerActionSample sample;
  sample.leftTriggerDown = true;
  sample.rightTriggerDown = true;
  sample.leftStickPressDown = true;
  sample.rightStickPressDown = true;

  const iggy3d::ProductControllerActionRoutingResult result =
      iggy3d::recordProductControllerMappedActions({
          iggy3d::ProductInputSurface::Gameplay,
          iggy3d::ProductInteractionMode::Player,
          sample,
          state,
          actions,
      });

  return expect(!result.mapped, "chord controls not mapped") &&
         expect(result.status == "controller_action_chord_reserved",
                "chord controls reserved status") &&
         expect(actions.entries.empty(), "chord controls emit no actions");
}

bool blockedSurfaceDoesNotEmitMappedAction() {
  iggy3d::ProductControllerActionRoutingState state;
  iggy3d::ActionState actions;
  iggy3d::GamepadControllerActionSample sample;
  sample.southButtonDown = true;

  const iggy3d::ProductControllerActionRoutingResult result =
      iggy3d::recordProductControllerMappedActions({
          iggy3d::ProductInputSurface::Pause,
          iggy3d::ProductInteractionMode::Creative,
          sample,
          state,
          actions,
      });

  return expect(!result.mapped, "blocked surface not mapped") &&
         expect(result.status == "controller_action_unmapped",
                "blocked surface status") &&
         expect(actions.entries.empty(), "blocked surface emits no actions");
}

bool skippedRoutingRecordsChordConsumption() {
  iggy3d::ProductAppWindowState window;
  const iggy3d::ProductControllerActionRoutingResult skipped =
      iggy3d::productControllerActionRoutingSkipped(
          iggy3d::ProductInputSurface::Gameplay,
          iggy3d::ProductInteractionMode::Creative,
          "controller_action_chord_consumed");
  iggy3d::recordProductControllerActionRoutingResult(window, skipped);

  return expect(!window.controllerActionMapped, "skipped routing not mapped") &&
         expect(window.controllerActionStatus == "controller_action_chord_consumed",
                "skipped routing status") &&
         expect(window.controllerActionControl == "none",
                "skipped routing control") &&
         expect(window.controllerActionMode == "creative",
                "skipped routing mode") &&
         expect(window.controllerActionSurface == "gameplay",
                "skipped routing surface") &&
         expect(window.controllerActionInputAction == "none",
                "skipped routing action");
}

}  // namespace

int main() {
  const bool ok = playerGameplayMapsToGameplayActions() &&
                  creativeGameplayMapsToEditorActions() &&
                  roomEditorSurfaceMapsToEditorRegardlessOfMode() &&
                  heldEditorControlDoesNotRepeat() &&
                  chordControlsRemainReserved() &&
                  blockedSurfaceDoesNotEmitMappedAction() &&
                  skippedRoutingRecordsChordConsumption();
  return ok ? 0 : 1;
}

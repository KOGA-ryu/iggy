#include "app/input/InputBindings.hpp"

namespace iggy3d {

const std::vector<InputBinding>& defaultInputBindings() {
  static const std::vector<InputBinding> bindings = {
      {NeutralInput::KeyW, InputAction::PlayerMoveY, 1.0F},
      {NeutralInput::KeyS, InputAction::PlayerMoveY, -1.0F},
      {NeutralInput::KeyA, InputAction::PlayerMoveX, -1.0F},
      {NeutralInput::KeyD, InputAction::PlayerMoveX, 1.0F},
      {NeutralInput::KeyUp, InputAction::MenuUp, 1.0F},
      {NeutralInput::KeyDown, InputAction::MenuDown, 1.0F},
      {NeutralInput::KeyLeft, InputAction::MenuLeft, 1.0F},
      {NeutralInput::KeyRight, InputAction::MenuRight, 1.0F},
      {NeutralInput::KeyEnter, InputAction::MenuConfirm, 1.0F},
      {NeutralInput::KeySpace, InputAction::MenuConfirm, 1.0F},
      {NeutralInput::KeySpace, InputAction::EditorApply, 1.0F},
      {NeutralInput::KeyEscape, InputAction::MenuBack, 1.0F},
      {NeutralInput::KeyTab, InputAction::MenuNextTab, 1.0F},
      {NeutralInput::KeyShiftTab, InputAction::MenuPreviousTab, 1.0F},
      {NeutralInput::KeyF1, InputAction::DevToggle, 1.0F},
      {NeutralInput::KeyF2, InputAction::DevCollisionOverlay, 1.0F},
      {NeutralInput::KeyF3, InputAction::DevDebugOverlay, 1.0F},
      {NeutralInput::KeyF4, InputAction::MovementTuningToggle, 1.0F},
      {NeutralInput::KeyM, InputAction::MapMakerToggle, 1.0F},
      {NeutralInput::KeyE, InputAction::PlayerInteract, 1.0F},
      {NeutralInput::KeyR, InputAction::PlayerRetryOrReset, 1.0F},
      {NeutralInput::MouseLeft, InputAction::MenuConfirm, 1.0F},
      {NeutralInput::MouseDeltaX, InputAction::PlayerLookX, 1.0F},
      {NeutralInput::MouseDeltaY, InputAction::PlayerLookY, 1.0F},
      {NeutralInput::DpadUp, InputAction::MenuUp, 1.0F},
      {NeutralInput::DpadDown, InputAction::MenuDown, 1.0F},
      {NeutralInput::DpadLeft, InputAction::MenuLeft, 1.0F},
      {NeutralInput::DpadRight, InputAction::MenuRight, 1.0F},
      {NeutralInput::ButtonSouth, InputAction::MenuConfirm, 1.0F},
      {NeutralInput::ButtonSouth, InputAction::EditorApply, 1.0F},
      {NeutralInput::ButtonEast, InputAction::MenuBack, 1.0F},
      {NeutralInput::Start, InputAction::SystemPause, 1.0F},
      {NeutralInput::StartBackChord, InputAction::SystemHardQuit, 1.0F},
      {NeutralInput::LeftStickX, InputAction::PlayerMoveX, 1.0F},
      {NeutralInput::LeftStickY, InputAction::PlayerMoveY, 1.0F},
      {NeutralInput::RightStickX, InputAction::PlayerLookX, 1.0F},
      {NeutralInput::RightStickY, InputAction::PlayerLookY, 1.0F},
      {NeutralInput::RightTrigger, InputAction::PlayerAttack, 1.0F},
      {NeutralInput::AutomationMenuUp, InputAction::MenuUp, 1.0F},
      {NeutralInput::AutomationMenuDown, InputAction::MenuDown, 1.0F},
      {NeutralInput::AutomationMenuConfirm, InputAction::MenuConfirm, 1.0F},
      {NeutralInput::AutomationMenuBack, InputAction::MenuBack, 1.0F},
      {NeutralInput::AutomationPause, InputAction::SystemPause, 1.0F},
      {NeutralInput::AutomationDevTools, InputAction::DevToggle, 1.0F},
      {NeutralInput::AutomationHardQuit, InputAction::SystemHardQuit, 1.0F},
      {NeutralInput::AutomationInteract, InputAction::PlayerInteract, 1.0F},
      {NeutralInput::AutomationAttack, InputAction::PlayerAttack, 1.0F},
      {NeutralInput::AutomationRetryOrReset, InputAction::PlayerRetryOrReset, 1.0F},
  };
  return bindings;
}

InputAction actionForInput(NeutralInput input) {
  for (const InputBinding& binding : defaultInputBindings()) {
    if (binding.input == input) {
      return binding.action;
    }
  }
  return InputAction::None;
}

}  // namespace iggy3d

#include "app/input/InputActions.hpp"

#include <array>
#include <cstdint>

namespace iggy3d {
namespace {

constexpr std::size_t kInputActionDescriptorTableSize = 256U;

constexpr std::size_t actionIndex(InputAction action) {
  return static_cast<std::size_t>(static_cast<std::uint8_t>(action));
}

constexpr std::array<InputActionDescriptor, kInputActionDescriptorTableSize>
buildInputActionDescriptors() {
  std::array<InputActionDescriptor, kInputActionDescriptorTableSize> descriptors{};

  descriptors[actionIndex(InputAction::None)] =
      {InputAction::None, "none", InputActionGroup::None, "none", "none"};

  descriptors[actionIndex(InputAction::MenuUp)] =
      {InputAction::MenuUp, "menu.up", InputActionGroup::Menu, "menu", "menu"};
  descriptors[actionIndex(InputAction::MenuDown)] =
      {InputAction::MenuDown, "menu.down", InputActionGroup::Menu, "menu", "menu"};
  descriptors[actionIndex(InputAction::MenuLeft)] =
      {InputAction::MenuLeft, "menu.left", InputActionGroup::Menu, "menu", "menu"};
  descriptors[actionIndex(InputAction::MenuRight)] =
      {InputAction::MenuRight, "menu.right", InputActionGroup::Menu, "menu", "menu"};
  descriptors[actionIndex(InputAction::MenuConfirm)] =
      {InputAction::MenuConfirm, "menu.confirm", InputActionGroup::Menu, "menu", "menu"};
  descriptors[actionIndex(InputAction::MenuBack)] =
      {InputAction::MenuBack, "menu.back", InputActionGroup::Menu, "menu", "menu"};
  descriptors[actionIndex(InputAction::MenuNextTab)] =
      {InputAction::MenuNextTab, "menu.next_tab", InputActionGroup::Menu, "menu", "menu"};
  descriptors[actionIndex(InputAction::MenuPreviousTab)] =
      {InputAction::MenuPreviousTab, "menu.previous_tab", InputActionGroup::Menu, "menu", "menu"};

  descriptors[actionIndex(InputAction::SystemPause)] =
      {InputAction::SystemPause, "system.pause", InputActionGroup::System, "system", "system"};
  descriptors[actionIndex(InputAction::SystemDevTools)] =
      {InputAction::SystemDevTools, "dev.toggle", InputActionGroup::System, "dev_tools", "overlay"};
  descriptors[actionIndex(InputAction::SystemHardQuit)] =
      {InputAction::SystemHardQuit, "system.quit_chord", InputActionGroup::System, "system", "system"};
  descriptors[actionIndex(InputAction::MapMakerToggle)] =
      {InputAction::MapMakerToggle,
       "map_maker.toggle",
       InputActionGroup::Player,
       "map_maker",
       "gameplay",
       true,
       true};
  descriptors[actionIndex(InputAction::MovementTuningToggle)] =
      {InputAction::MovementTuningToggle,
       "movement_tuning.toggle",
       InputActionGroup::System,
       "movement_tuning",
       "gameplay",
       true,
       true};

  descriptors[actionIndex(InputAction::PlayerMoveX)] =
      {InputAction::PlayerMoveX, "game.move_x", InputActionGroup::Player, "player", "gameplay"};
  descriptors[actionIndex(InputAction::PlayerMoveY)] =
      {InputAction::PlayerMoveY, "game.move_y", InputActionGroup::Player, "player", "gameplay"};
  descriptors[actionIndex(InputAction::PlayerLookX)] =
      {InputAction::PlayerLookX, "game.look_x", InputActionGroup::Player, "player", "gameplay"};
  descriptors[actionIndex(InputAction::PlayerLookY)] =
      {InputAction::PlayerLookY, "game.look_y", InputActionGroup::Player, "player", "gameplay"};
  descriptors[actionIndex(InputAction::PlayerJump)] =
      {InputAction::PlayerJump, "game.jump", InputActionGroup::Player, "player", "gameplay"};
  descriptors[actionIndex(InputAction::PlayerCrouch)] =
      {InputAction::PlayerCrouch, "game.crouch", InputActionGroup::Player, "player", "gameplay"};
  descriptors[actionIndex(InputAction::PlayerSprint)] =
      {InputAction::PlayerSprint, "game.sprint", InputActionGroup::Player, "player", "gameplay"};
  descriptors[actionIndex(InputAction::PlayerDash)] =
      {InputAction::PlayerDash, "game.dash", InputActionGroup::Player, "player", "gameplay"};
  descriptors[actionIndex(InputAction::PlayerInteract)] =
      {InputAction::PlayerInteract, "game.interact", InputActionGroup::Player, "player", "gameplay"};
  descriptors[actionIndex(InputAction::PlayerAttack)] =
      {InputAction::PlayerAttack, "game.attack", InputActionGroup::Player, "player", "gameplay"};
  descriptors[actionIndex(InputAction::PlayerCast)] =
      {InputAction::PlayerCast, "game.cast", InputActionGroup::Player, "player", "gameplay"};
  descriptors[actionIndex(InputAction::PlayerRetryOrReset)] =
      {InputAction::PlayerRetryOrReset,
       "game.retry_or_reset",
       InputActionGroup::Player,
       "player",
       "gameplay"};

  descriptors[actionIndex(InputAction::EditorToggle)] =
      {InputAction::EditorToggle, "editor.toggle", InputActionGroup::Editor, "room_editor", "editor"};
  descriptors[actionIndex(InputAction::EditorSelect)] =
      {InputAction::EditorSelect, "editor.select", InputActionGroup::Editor, "room_editor", "editor"};
  descriptors[actionIndex(InputAction::EditorPlace)] =
      {InputAction::EditorPlace, "editor.place", InputActionGroup::Editor, "room_editor", "editor"};
  descriptors[actionIndex(InputAction::EditorApply)] =
      {InputAction::EditorApply, "editor.apply", InputActionGroup::Editor, "room_editor", "editor"};
  descriptors[actionIndex(InputAction::EditorDelete)] =
      {InputAction::EditorDelete, "editor.delete", InputActionGroup::Editor, "room_editor", "editor"};
  descriptors[actionIndex(InputAction::EditorUndo)] =
      {InputAction::EditorUndo, "editor.undo", InputActionGroup::Editor, "room_editor", "editor"};
  descriptors[actionIndex(InputAction::EditorRedo)] =
      {InputAction::EditorRedo, "editor.redo", InputActionGroup::Editor, "room_editor", "editor"};
  descriptors[actionIndex(InputAction::EditorNextTool)] =
      {InputAction::EditorNextTool, "editor.next_tool", InputActionGroup::Editor, "room_editor", "editor"};
  descriptors[actionIndex(InputAction::EditorPreviousTool)] =
      {InputAction::EditorPreviousTool,
       "editor.previous_tool",
       InputActionGroup::Editor,
       "room_editor",
       "editor"};
  descriptors[actionIndex(InputAction::EditorSelectFloorTool)] =
      {InputAction::EditorSelectFloorTool,
       "editor.select_floor_tool",
       InputActionGroup::Editor,
       "room_editor",
       "editor"};
  descriptors[actionIndex(InputAction::EditorSelectWallTool)] =
      {InputAction::EditorSelectWallTool,
       "editor.select_wall_tool",
       InputActionGroup::Editor,
       "room_editor",
       "editor"};
  descriptors[actionIndex(InputAction::EditorRotateWallDirection)] =
      {InputAction::EditorRotateWallDirection,
       "editor.rotate_wall_direction",
       InputActionGroup::Editor,
       "room_editor",
       "editor"};
  descriptors[actionIndex(InputAction::EditorPreviewPlacement)] =
      {InputAction::EditorPreviewPlacement,
       "editor.preview",
       InputActionGroup::Editor,
       "room_editor",
       "editor"};
  descriptors[actionIndex(InputAction::EditorConfirmPreview)] =
      {InputAction::EditorConfirmPreview,
       "editor.preview_confirm",
       InputActionGroup::Editor,
       "room_editor",
       "editor"};
  descriptors[actionIndex(InputAction::EditorCancelPreview)] =
      {InputAction::EditorCancelPreview,
       "editor.preview_cancel",
       InputActionGroup::Editor,
       "room_editor",
       "editor"};
  descriptors[actionIndex(InputAction::EditorNudgeX)] =
      {InputAction::EditorNudgeX, "editor.nudge_x", InputActionGroup::Editor, "room_editor", "editor"};
  descriptors[actionIndex(InputAction::EditorNudgeZ)] =
      {InputAction::EditorNudgeZ, "editor.nudge_z", InputActionGroup::Editor, "room_editor", "editor"};
  descriptors[actionIndex(InputAction::EditorResizeX)] =
      {InputAction::EditorResizeX, "editor.resize_x", InputActionGroup::Editor, "room_editor", "editor"};
  descriptors[actionIndex(InputAction::EditorResizeZ)] =
      {InputAction::EditorResizeZ, "editor.resize_z", InputActionGroup::Editor, "room_editor", "editor"};

  descriptors[actionIndex(InputAction::DevToggle)] =
      {InputAction::DevToggle, "dev.toggle", InputActionGroup::System, "dev_tools", "overlay"};
  descriptors[actionIndex(InputAction::DevDebugOverlay)] =
      {InputAction::DevDebugOverlay,
       "dev.debug_overlay",
       InputActionGroup::System,
       "debug_overlay",
       "overlay"};
  descriptors[actionIndex(InputAction::DevCollisionOverlay)] =
      {InputAction::DevCollisionOverlay,
       "dev.collision_overlay",
       InputActionGroup::System,
       "debug_overlay",
       "overlay"};
  descriptors[actionIndex(InputAction::DevReceiptDump)] =
      {InputAction::DevReceiptDump,
       "dev.receipt_dump",
       InputActionGroup::System,
       "debug_overlay",
       "overlay"};

  return descriptors;
}

constexpr auto kInputActionDescriptors = buildInputActionDescriptors();

}  // namespace

std::string_view inputActionName(InputAction action) {
  return inputActionDescriptor(action).name;
}

InputActionGroup inputActionGroup(InputAction action) {
  return inputActionDescriptor(action).group;
}

std::string_view inputActionGroupName(InputActionGroup group) {
  // branch-gate: BG-1225
  switch (group) {
    case InputActionGroup::None:
      return "none";
    case InputActionGroup::Menu:
      return "menu";
    case InputActionGroup::System:
      return "system";
    case InputActionGroup::Player:
      return "player";
    case InputActionGroup::Editor:
      return "editor";
  }
  return "none";
}

const InputActionDescriptor& inputActionDescriptor(InputAction action) {
  return kInputActionDescriptors[actionIndex(action)];
}

std::string_view inputActionFeatureName(InputAction action) {
  return inputActionDescriptor(action).feature;
}

std::string_view inputActionOwnerName(InputAction action) {
  return inputActionDescriptor(action).owner;
}

bool inputActionHandledBeforeMenu(InputAction action) {
  return inputActionDescriptor(action).handledBeforeMenu;
}

bool inputActionKeepsGameplayActive(InputAction action) {
  return inputActionDescriptor(action).keepsGameplayActive;
}

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
    // branch-gate: BG-1226
    if (binding.input == input) {
      return binding.action;
    }
  }
  return InputAction::None;
}

}  // namespace iggy3d

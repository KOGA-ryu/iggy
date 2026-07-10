#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "app/input/InputDeviceEvent.hpp"

namespace iggy3d {

enum class InputAction : std::uint8_t {
  None,

  MenuUp,
  MenuDown,
  MenuLeft,
  MenuRight,
  MenuConfirm,
  MenuBack,
  MenuNextTab,
  MenuPreviousTab,

  SystemPause,
  SystemDevTools,
  SystemHardQuit,
  MapMakerToggle,
  MovementTuningToggle,

  PlayerMoveX,
  PlayerMoveY,
  PlayerLookX,
  PlayerLookY,
  PlayerJump,
  PlayerCrouch,
  PlayerSprint,
  PlayerDash,
  PlayerInteract,
  PlayerAttack,
  PlayerCast,
  PlayerRetryOrReset,

  EditorToggle,
  EditorSelect,
  EditorPlace,
  EditorApply,
  EditorDelete,
  EditorUndo,
  EditorRedo,
  EditorNextTool,
  EditorPreviousTool,
  EditorSelectFloorTool,
  EditorSelectWallTool,
  EditorRotateWallDirection,
  EditorPreviewPlacement,
  EditorConfirmPreview,
  EditorCancelPreview,
  EditorNudgeX,
  EditorNudgeZ,
  EditorResizeX,
  EditorResizeZ,

  DevToggle,
  DevDebugOverlay,
  DevCollisionOverlay,
  DevReceiptDump,
};

enum class InputActionGroup : std::uint8_t {
  None,
  Menu,
  System,
  Player,
  Editor,
};

struct InputActionDescriptor {
  InputAction action = InputAction::None;
  std::string_view name = "none";
  InputActionGroup group = InputActionGroup::None;
  std::string_view feature = "none";
  std::string_view owner = "none";
  bool handledBeforeMenu = false;
  bool keepsGameplayActive = false;
};

enum class InputDeviceKind {
  Keyboard,
  Mouse,
  Gamepad,
  Automation,
};

struct InputBinding {
  NeutralInput input = NeutralInput::None;
  InputAction action = InputAction::None;
  float scale = 1.0F;
};

std::string_view inputActionName(InputAction action);
InputActionGroup inputActionGroup(InputAction action);
std::string_view inputActionGroupName(InputActionGroup group);
const InputActionDescriptor& inputActionDescriptor(InputAction action);
std::string_view inputActionFeatureName(InputAction action);
std::string_view inputActionOwnerName(InputAction action);
bool inputActionHandledBeforeMenu(InputAction action);
bool inputActionKeepsGameplayActive(InputAction action);
const std::vector<InputBinding>& defaultInputBindings();
InputAction actionForInput(NeutralInput input);

}  // namespace iggy3d

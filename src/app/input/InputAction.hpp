#pragma once

#include <cstdint>
#include <string_view>

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

  PlayerMoveX,
  PlayerMoveY,
  PlayerLookX,
  PlayerLookY,
  PlayerJump,
  PlayerCrouch,
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

std::string_view inputActionName(InputAction action);
InputActionGroup inputActionGroup(InputAction action);
std::string_view inputActionGroupName(InputActionGroup group);

}  // namespace iggy3d

#include "app/input/InputAction.hpp"

namespace iggy3d {

std::string_view inputActionName(InputAction action) {
  switch (action) {
    case InputAction::None:
      return "none";
    case InputAction::MenuUp:
      return "menu.up";
    case InputAction::MenuDown:
      return "menu.down";
    case InputAction::MenuLeft:
      return "menu.left";
    case InputAction::MenuRight:
      return "menu.right";
    case InputAction::MenuConfirm:
      return "menu.confirm";
    case InputAction::MenuBack:
      return "menu.back";
    case InputAction::MenuNextTab:
      return "menu.next_tab";
    case InputAction::MenuPreviousTab:
      return "menu.previous_tab";
    case InputAction::SystemPause:
      return "system.pause";
    case InputAction::SystemDevTools:
      return "dev.toggle";
    case InputAction::SystemHardQuit:
      return "system.quit_chord";
    case InputAction::PlayerMoveX:
      return "game.move_x";
    case InputAction::PlayerMoveY:
      return "game.move_y";
    case InputAction::PlayerLookX:
      return "game.look_x";
    case InputAction::PlayerLookY:
      return "game.look_y";
    case InputAction::PlayerJump:
      return "game.jump";
    case InputAction::PlayerCrouch:
      return "game.crouch";
    case InputAction::PlayerSprint:
      return "game.sprint";
    case InputAction::PlayerDash:
      return "game.dash";
    case InputAction::PlayerInteract:
      return "game.interact";
    case InputAction::PlayerAttack:
      return "game.attack";
    case InputAction::PlayerCast:
      return "game.cast";
    case InputAction::PlayerRetryOrReset:
      return "game.retry_or_reset";
    case InputAction::EditorToggle:
      return "editor.toggle";
    case InputAction::EditorSelect:
      return "editor.select";
    case InputAction::EditorPlace:
      return "editor.place";
    case InputAction::EditorApply:
      return "editor.apply";
    case InputAction::EditorDelete:
      return "editor.delete";
    case InputAction::EditorUndo:
      return "editor.undo";
    case InputAction::EditorRedo:
      return "editor.redo";
    case InputAction::EditorNextTool:
      return "editor.next_tool";
    case InputAction::EditorPreviousTool:
      return "editor.previous_tool";
    case InputAction::EditorSelectFloorTool:
      return "editor.select_floor_tool";
    case InputAction::EditorSelectWallTool:
      return "editor.select_wall_tool";
    case InputAction::EditorRotateWallDirection:
      return "editor.rotate_wall_direction";
    case InputAction::EditorPreviewPlacement:
      return "editor.preview";
    case InputAction::EditorConfirmPreview:
      return "editor.preview_confirm";
    case InputAction::EditorCancelPreview:
      return "editor.preview_cancel";
    case InputAction::EditorNudgeX:
      return "editor.nudge_x";
    case InputAction::EditorNudgeZ:
      return "editor.nudge_z";
    case InputAction::EditorResizeX:
      return "editor.resize_x";
    case InputAction::EditorResizeZ:
      return "editor.resize_z";
    case InputAction::DevToggle:
      return "dev.toggle";
    case InputAction::DevDebugOverlay:
      return "dev.debug_overlay";
    case InputAction::DevCollisionOverlay:
      return "dev.collision_overlay";
    case InputAction::DevReceiptDump:
      return "dev.receipt_dump";
  }
  return "none";
}

InputActionGroup inputActionGroup(InputAction action) {
  switch (action) {
    case InputAction::MenuUp:
    case InputAction::MenuDown:
    case InputAction::MenuLeft:
    case InputAction::MenuRight:
    case InputAction::MenuConfirm:
    case InputAction::MenuBack:
    case InputAction::MenuNextTab:
    case InputAction::MenuPreviousTab:
      return InputActionGroup::Menu;
    case InputAction::SystemPause:
    case InputAction::SystemDevTools:
    case InputAction::SystemHardQuit:
      return InputActionGroup::System;
    case InputAction::PlayerMoveX:
    case InputAction::PlayerMoveY:
    case InputAction::PlayerLookX:
    case InputAction::PlayerLookY:
    case InputAction::PlayerJump:
    case InputAction::PlayerCrouch:
    case InputAction::PlayerSprint:
    case InputAction::PlayerDash:
    case InputAction::PlayerInteract:
    case InputAction::PlayerAttack:
    case InputAction::PlayerCast:
    case InputAction::PlayerRetryOrReset:
      return InputActionGroup::Player;
    case InputAction::EditorToggle:
    case InputAction::EditorSelect:
    case InputAction::EditorPlace:
    case InputAction::EditorApply:
    case InputAction::EditorDelete:
    case InputAction::EditorUndo:
    case InputAction::EditorRedo:
    case InputAction::EditorNextTool:
    case InputAction::EditorPreviousTool:
    case InputAction::EditorSelectFloorTool:
    case InputAction::EditorSelectWallTool:
    case InputAction::EditorRotateWallDirection:
    case InputAction::EditorPreviewPlacement:
    case InputAction::EditorConfirmPreview:
    case InputAction::EditorCancelPreview:
    case InputAction::EditorNudgeX:
    case InputAction::EditorNudgeZ:
    case InputAction::EditorResizeX:
    case InputAction::EditorResizeZ:
      return InputActionGroup::Editor;
    case InputAction::DevToggle:
    case InputAction::DevDebugOverlay:
    case InputAction::DevCollisionOverlay:
    case InputAction::DevReceiptDump:
      return InputActionGroup::System;
    case InputAction::None:
      break;
  }
  return InputActionGroup::None;
}

std::string_view inputActionGroupName(InputActionGroup group) {
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

}  // namespace iggy3d

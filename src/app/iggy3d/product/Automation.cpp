#include "app/iggy3d/product/Automation.hpp"

#include <algorithm>
#include <array>
#include <iterator>

#include "app/iggy3d/ProductRoomAuthoringController.hpp"
#include "app/iggy3d/ProductRoomEditorActionController.hpp"
#include "app/iggy3d/ProductRoomEditorCursor.hpp"

namespace iggy3d {

std::string_view productAutomationCommandCategoryName(
    ProductAutomationCommandCategory category) {
  static constexpr std::array names{
      std::string_view{"owner"},
      std::string_view{"menu_input"},
      std::string_view{"menu_shortcut"},
      std::string_view{"frontend_select"},
      std::string_view{"frontend_execute"},
      std::string_view{"world_setup"},
      std::string_view{"dungeon_draft"},
      std::string_view{"ascii_room"},
      std::string_view{"save_browser"},
      std::string_view{"deleted_save_browser"},
      std::string_view{"room_edit"},
      std::string_view{"room_editor"},
      std::string_view{"gameplay_input"},
      std::string_view{"gameplay_tape"},
      std::string_view{"settings"},
      std::string_view{"dev_tools"},
      std::string_view{"system"},
      std::string_view{"unknown"},
  };
  return names[static_cast<std::size_t>(category)];
}

std::string_view productAutomationValueKindName(
    ProductAutomationValueKind valueKind) {
  static constexpr std::array names{
      std::string_view{"bool"},
      std::string_view{"string"},
      std::string_view{"float"},
      std::string_view{"size"},
      std::string_view{"csv"},
      std::string_view{"action"},
      std::string_view{"direction"},
      std::string_view{"tool"},
      std::string_view{"raw"},
      std::string_view{"unknown"},
  };
  return names[static_cast<std::size_t>(valueKind)];
}

std::string_view productAutomationCommandIdName(
    ProductAutomationCommandId commandId) {
  static constexpr std::array names{
      std::string_view{"menu_shortcut"},
      std::string_view{"room_edit.start"},
      std::string_view{"room_edit.start_active"},
      std::string_view{"room_edit.add_floor"},
      std::string_view{"room_edit.add_wall"},
      std::string_view{"room_edit.delete_floor"},
      std::string_view{"room_edit.delete_wall"},
      std::string_view{"room_edit.undo"},
      std::string_view{"room_edit.redo"},
      std::string_view{"room_editor.move"},
      std::string_view{"room_editor.tool"},
      std::string_view{"room_editor.cycle_tool"},
      std::string_view{"room_editor.wall_direction"},
      std::string_view{"room_editor.place"},
      std::string_view{"unknown"},
  };
  return names[static_cast<std::size_t>(commandId)];
}

ProductAutomationCommandRegistry makeProductAutomationCommandRegistry() {
  using Category = ProductAutomationCommandCategory;
  using Value = ProductAutomationValueKind;

  // Registry rows define every known automation key, alias, and metadata field.
  ProductAutomationCommandRegistry registry;
  registry.specs = {
      {"automation.owner", {}, Category::Owner, Value::Raw, true, false, "any"},

      {"menu.input", {}, Category::MenuInput, Value::Action, true, false, "menu"},
      {"menu.up", {}, Category::MenuShortcut, Value::Bool, true, true, "menu"},
      {"menu.down", {}, Category::MenuShortcut, Value::Bool, true, true, "menu"},
      {"menu.left", {}, Category::MenuShortcut, Value::Bool, true, true, "menu"},
      {"menu.right", {}, Category::MenuShortcut, Value::Bool, true, true, "menu"},
      {"menu.confirm", {}, Category::MenuShortcut, Value::Bool, true, true,
       "menu"},
      {"menu.back", {}, Category::MenuShortcut, Value::Bool, true, true, "menu"},
      {"menu.next_tab", {}, Category::MenuShortcut, Value::Bool, true, true,
       "menu"},
      {"menu.previous_tab", {}, Category::MenuShortcut, Value::Bool, true, true,
       "menu"},

      {"frontend.select", {"pause.select"}, Category::FrontendSelect,
       Value::Action, true, false, "frontend"},
      {"frontend.execute", {"pause.execute", "dev_tools.execute"},
       Category::FrontendExecute, Value::Bool, true, true, "frontend"},

      {"world.title", {"world_setup.title"}, Category::WorldSetup,
       Value::String, true, false, "new_world"},
      {"world.dungeon_id",
       {"world.map_id", "world_setup.dungeon_id", "world_setup.map_id"},
       Category::WorldSetup, Value::String, true, false, "new_world"},
      {"world.create", {"world_setup.create"}, Category::WorldSetup,
       Value::Bool, true, true, "new_world"},
      {"world.ascii_room_text", {"world_setup.ascii_room_text"},
       Category::WorldSetup, Value::Raw, true, false, "new_world"},
      {"world.ascii_room_id", {"world_setup.ascii_room_id"},
       Category::WorldSetup, Value::String, true, false, "new_world"},
      {"world.ascii_room_source_name", {"world_setup.ascii_room_source_name"},
       Category::WorldSetup, Value::String, true, false, "new_world"},

      {"world.draft_edit_mode", {"world_setup.draft_edit_mode"},
       Category::DungeonDraft, Value::Bool, true, false, "new_world"},
      {"world.draft_move", {"world_setup.draft_move", "frontend.draft_move"},
       Category::DungeonDraft, Value::Direction, true, false, "new_world"},
      {"world.draft_paint", {"world_setup.draft_paint", "frontend.draft_paint"},
       Category::DungeonDraft, Value::String, true, false, "new_world"},
      {"world.draft_cell", {"world_setup.draft_cell"}, Category::DungeonDraft,
       Value::Csv, true, false, "new_world"},

      {"ascii_room.text", {"frontend.ascii_room_text"}, Category::AsciiRoom,
       Value::Raw, true, false, "frontend"},
      {"ascii_room.room_id", {"frontend.ascii_room_id"}, Category::AsciiRoom,
       Value::String, true, false, "frontend"},
      {"ascii_room.source_name", {"frontend.ascii_room_source_name"},
       Category::AsciiRoom, Value::String, true, false, "frontend"},
      {"ascii_room.build", {"frontend.ascii_room_build"}, Category::AsciiRoom,
       Value::Bool, true, true, "frontend"},
      {"ascii_room.activate", {"frontend.ascii_room_activate"},
       Category::AsciiRoom, Value::Bool, true, true, "frontend"},

      {"save.select", {"frontend.save_select"}, Category::SaveBrowser,
       Value::String, true, false, "load_save"},
      {"save.delete", {"frontend.save_delete"}, Category::SaveBrowser,
       Value::Bool, true, true, "load_save"},
      {"save.show_deleted", {"frontend.show_deleted_saves"},
       Category::SaveBrowser, Value::Bool, true, true, "load_save"},
      {"save.deleted_select", {"frontend.deleted_save_select"},
       Category::DeletedSaveBrowser, Value::String, true, false, "load_save"},
      {"save.recover", {"frontend.save_recover"}, Category::DeletedSaveBrowser,
       Value::Bool, true, true, "load_save"},

      {"room_edit.start", {"frontend.room_edit_start"}, Category::RoomEdit,
       Value::Bool, true, true, "room_edit"},
      {"room_edit.start_active", {"frontend.room_edit_start_active"},
       Category::RoomEdit, Value::Bool, true, true, "room_edit"},
      {"room_edit.add_floor", {"frontend.room_edit_add_floor"},
       Category::RoomEdit, Value::Csv, true, false, "room_edit"},
      {"room_edit.add_wall", {"frontend.room_edit_add_wall"},
       Category::RoomEdit, Value::Csv, true, false, "room_edit"},
      {"room_edit.delete_floor", {"frontend.room_edit_delete_floor"},
       Category::RoomEdit, Value::String, true, false, "room_edit"},
      {"room_edit.delete_wall", {"frontend.room_edit_delete_wall"},
       Category::RoomEdit, Value::String, true, false, "room_edit"},
      {"room_edit.undo", {"frontend.room_edit_undo"}, Category::RoomEdit,
       Value::Bool, true, true, "room_edit"},
      {"room_edit.redo", {"frontend.room_edit_redo"}, Category::RoomEdit,
       Value::Bool, true, true, "room_edit"},

      {"room_editor.move", {"frontend.room_editor_move"}, Category::RoomEditor,
       Value::Direction, true, false, "room_editor"},
      {"room_editor.tool", {"frontend.room_editor_tool"}, Category::RoomEditor,
       Value::Tool, true, false, "room_editor"},
      {"room_editor.cycle_tool", {"frontend.room_editor_cycle_tool"},
       Category::RoomEditor, Value::Bool, true, true, "room_editor"},
      {"room_editor.wall_direction", {"frontend.room_editor_wall_direction"},
       Category::RoomEditor, Value::Direction, true, false, "room_editor"},
      {"room_editor.place", {"frontend.room_editor_place"},
       Category::RoomEditor, Value::Bool, true, true, "room_editor"},
      {"editor.input", {}, Category::RoomEditor, Value::Action, true, false,
       "editor"},

      {"game.move_x", {"frontend.game_move_x"}, Category::GameplayInput,
       Value::Float, true, false, "gameplay"},
      {"game.move_y", {"frontend.game_move_y"}, Category::GameplayInput,
       Value::Float, true, false, "gameplay"},
      {"game.attack", {"frontend.game_attack"}, Category::GameplayInput,
       Value::Bool, true, true, "gameplay"},
      {"game.interact", {"frontend.game_interact"}, Category::GameplayInput,
       Value::Bool, true, true, "gameplay"},

      {"settings.tab", {}, Category::Settings, Value::String, true, false,
       "settings"},
      {"settings.apply", {}, Category::Settings, Value::Bool, true, true,
       "settings"},
      {"settings.restore_defaults", {}, Category::Settings, Value::Bool, true,
       true, "settings"},
      {"settings.back", {"system.back"}, Category::Settings, Value::Bool, true,
       true, "settings"},
      {"dev_tools.category", {}, Category::DevTools, Value::String, true, false,
       "dev_tools"},

      {"frontend.return_to_title", {}, Category::System, Value::Bool, true,
       false, "frontend"},
      {"system.pause", {}, Category::System, Value::Bool, true, true, "system"},
      {"system.quit", {}, Category::System, Value::Bool, true, false, "system"},

      {"unknown", {}, Category::Unknown, Value::Unknown, false, false, "none"},
  };
  return registry;
}

const ProductAutomationCommandSpec& findProductAutomationCommandSpec(
    const ProductAutomationCommandRegistry& registry,
    std::string_view key) {
  const ProductAutomationCommandSpec& fallback = registry.specs.back();
  const auto searchEnd = std::prev(registry.specs.end());
  const auto row = std::find_if(
      registry.specs.begin(), searchEnd,
      [key](const ProductAutomationCommandSpec& candidate) {
        return candidate.canonicalKey == key ||
               std::find(candidate.aliases.begin(), candidate.aliases.end(),
                         key) != candidate.aliases.end();
      });
  const std::array<const ProductAutomationCommandSpec*, 2U> selected{
      &fallback, &*row};
  return *selected[row != searchEnd];
}

std::string_view productAutomationCanonicalKey(
    const ProductAutomationCommandRegistry& registry,
    std::string_view key) {
  return findProductAutomationCommandSpec(registry, key).canonicalKey;
}

const ProductAutomationCommandDispatchSpec& findProductAutomationCommandDispatchSpec(
    std::string_view canonicalKey) {
  using Category = ProductAutomationCommandCategory;
  using Id = ProductAutomationCommandId;
  using Value = ProductAutomationValueKind;
  // Dispatch rows define only the commands this runtime path handles directly.
  static constexpr std::array rows{
      ProductAutomationCommandDispatchSpec{
          true, "menu.up", Id::MenuShortcut, Category::MenuShortcut, Value::Bool,
          InputAction::MenuUp},
      ProductAutomationCommandDispatchSpec{
          true, "menu.down", Id::MenuShortcut, Category::MenuShortcut, Value::Bool,
          InputAction::MenuDown},
      ProductAutomationCommandDispatchSpec{
          true, "menu.left", Id::MenuShortcut, Category::MenuShortcut, Value::Bool,
          InputAction::MenuLeft},
      ProductAutomationCommandDispatchSpec{
          true, "menu.right", Id::MenuShortcut, Category::MenuShortcut, Value::Bool,
          InputAction::MenuRight},
      ProductAutomationCommandDispatchSpec{
          true, "menu.confirm", Id::MenuShortcut, Category::MenuShortcut, Value::Bool,
          InputAction::MenuConfirm},
      ProductAutomationCommandDispatchSpec{
          true, "menu.back", Id::MenuShortcut, Category::MenuShortcut, Value::Bool,
          InputAction::MenuBack},
      ProductAutomationCommandDispatchSpec{
          true, "menu.next_tab", Id::MenuShortcut, Category::MenuShortcut, Value::Bool,
          InputAction::MenuNextTab},
      ProductAutomationCommandDispatchSpec{
          true, "menu.previous_tab", Id::MenuShortcut, Category::MenuShortcut,
          Value::Bool, InputAction::MenuPreviousTab},
      ProductAutomationCommandDispatchSpec{
          true, "room_edit.start", Id::RoomEditStart, Category::RoomEdit, Value::Bool,
          InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_edit.start_active", Id::RoomEditStartActive, Category::RoomEdit,
          Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_edit.add_floor", Id::RoomEditAddFloor, Category::RoomEdit,
          Value::Csv, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_edit.add_wall", Id::RoomEditAddWall, Category::RoomEdit,
          Value::Csv, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_edit.delete_floor", Id::RoomEditDeleteFloor, Category::RoomEdit,
          Value::String, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_edit.delete_wall", Id::RoomEditDeleteWall, Category::RoomEdit,
          Value::String, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_edit.undo", Id::RoomEditUndo, Category::RoomEdit, Value::Bool,
          InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_edit.redo", Id::RoomEditRedo, Category::RoomEdit, Value::Bool,
          InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_editor.move", Id::RoomEditorMove, Category::RoomEditor,
          Value::Direction, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_editor.tool", Id::RoomEditorTool, Category::RoomEditor,
          Value::Tool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_editor.cycle_tool", Id::RoomEditorCycleTool,
          Category::RoomEditor, Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_editor.wall_direction", Id::RoomEditorWallDirection,
          Category::RoomEditor, Value::Direction, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_editor.place", Id::RoomEditorPlace, Category::RoomEditor,
          Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          false, "unknown", Id::Unknown, Category::Unknown, Value::Unknown,
          InputAction::None},
  };
  const ProductAutomationCommandDispatchSpec& fallback = rows.back();
  const auto searchEnd = std::prev(rows.end());
  const auto row = std::find_if(
      rows.begin(), searchEnd,
      [canonicalKey](const ProductAutomationCommandDispatchSpec& candidate) {
        return candidate.canonicalKey == canonicalKey;
      });
  const std::array<const ProductAutomationCommandDispatchSpec*, 2U> selected{
      &fallback, &*row};
  return *selected[row != searchEnd];
}

ProductAutomationCommandDispatchResult resolveProductAutomationCommandDispatch(
    ProductAutomationCommandDispatchRequest request) {
  const std::string_view canonicalKey =
      productAutomationCanonicalKey(*request.registry, request.key);
  const ProductAutomationCommandDispatchSpec& spec =
      findProductAutomationCommandDispatchSpec(canonicalKey);
  const std::array<std::string_view, 2U> statuses{
      std::string_view{"unknown"},
      std::string_view{"automation_command_resolved"},
  };
  ProductAutomationCommandDispatchResult result;
  result.handled = spec.handled;
  result.status = statuses[spec.handled];
  result.reasonCode = statuses[spec.handled];
  result.canonicalActionLabel = spec.canonicalKey;
  result.spec = spec;
  return result;
}

ProductRoomEditorCursorResult applyProductRoomEditorMoveAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction) {
  return moveProductRoomEditorCursor(state, direction);
}

ProductRoomEditorCursorResult applyProductRoomEditorToolAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorTool tool) {
  return setProductRoomEditorTool(state, tool);
}

ProductRoomEditorCursorResult applyProductRoomEditorCycleToolAutomation(
    ProductRoomEditorCursorState state) {
  return cycleProductRoomEditorTool(state);
}

ProductRoomEditorCursorResult applyProductRoomEditorWallDirectionAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction) {
  return setProductRoomEditorWallDirection(state, direction);
}

ProductRoomEditorActionResult applyProductRoomEditorPlaceAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    ProductRoomAuthoringInputSource inputSource) {
  return applyProductRoomAuthoringCursorPlace(
      {editing, cursor, inputSource});
}

ProductRoomEditorActionResult applyProductEditorInputAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    const ActionState& actions,
    ProductRoomAuthoringInputSource inputSource) {
  return applyProductRoomEditorActions(editing, cursor, actions, inputSource);
}

ProductRoomEditingStartResult startProductRoomEditAutomationFromAsciiDraft(
    ProductRoomAuthoringStartFromAsciiRequest request) {
  return startProductRoomAuthoringFromAsciiDraft(request);
}

ProductRoomEditingStartResult startProductRoomEditAutomationFromActiveRoom(
    ProductRoomAuthoringStartFromActiveRoomRequest request) {
  return startProductRoomAuthoringFromActiveRoom(request);
}

ProductRoomEditingOperationResult applyProductRoomEditAutomation(
    ProductRoomAuthoringEditCommandRequest request) {
  return applyProductRoomAuthoringEditCommand(request);
}

ProductRoomEditingOperationResult undoProductRoomEditAutomation(
    ProductRoomAuthoringUndoRedoRequest request) {
  return undoProductRoomAuthoringEdit(request);
}

ProductRoomEditingOperationResult redoProductRoomEditAutomation(
    ProductRoomAuthoringUndoRedoRequest request) {
  return redoProductRoomAuthoringEdit(request);
}

}  // namespace iggy3d

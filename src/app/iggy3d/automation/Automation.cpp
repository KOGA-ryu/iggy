#include "app/iggy3d/automation/Automation.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/MenuInput.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/ascii_room/Preview.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/world/DungeonDraft.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/room_editor/AuthoringController.hpp"
#include "app/iggy3d/room_editor/ActionController.hpp"
#include "app/iggy3d/room_editor/Cursor.hpp"

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
      std::string_view{"controller_input"},
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
      std::string_view{"room_editor.mouse_pick"},
      std::string_view{"room_editor.preview"},
      std::string_view{"room_editor.preview_confirm"},
      std::string_view{"room_editor.preview_cancel"},
      std::string_view{"room_editor.place"},
      std::string_view{"save.select"},
      std::string_view{"save.delete"},
      std::string_view{"save.show_deleted"},
      std::string_view{"save.deleted_select"},
      std::string_view{"save.recover"},
      std::string_view{"game.move_x"},
      std::string_view{"game.move_y"},
      std::string_view{"game.attack"},
      std::string_view{"game.interact"},
      std::string_view{"gameplay.jump"},
      std::string_view{"gameplay.crouch"},
      std::string_view{"gameplay.player_position"},
      std::string_view{"gameplay.physics_movement"},
      std::string_view{"controller.input"},
      std::string_view{"frontend.execute"},
      std::string_view{"settings.apply"},
      std::string_view{"settings.restore_defaults"},
      std::string_view{"settings.back"},
      std::string_view{"frontend.return_to_title"},
      std::string_view{"system.pause"},
      std::string_view{"system.quit"},
      std::string_view{"world.create"},
      std::string_view{"unknown"},
  };
  return names[static_cast<std::size_t>(commandId)];
}

bool parseProductAutomationBool(std::string_view value, bool& out) {
  // branch-gate: BG-1001
  if (value == "true" || value == "1" || value == "yes") {
    out = true;
    return true;
  }
  // branch-gate: BG-1001
  if (value == "false" || value == "0" || value == "no") {
    out = false;
    return true;
  }
  return false;
}

bool parseProductAutomationFloat(std::string_view value, float& out) {
  // branch-gate: BG-1001
  if (value.empty()) {
    return false;
  }
  const auto [ptr, error] =
      std::from_chars(value.data(), value.data() + value.size(), out);
  return error == std::errc{} && ptr == value.data() + value.size() &&
         std::isfinite(out);
}

std::vector<std::string_view> splitProductAutomationCsv(
    std::string_view value) {
  std::vector<std::string_view> fields;
  std::size_t start = 0;
  while (start <= value.size()) {
    // branch-gate: BG-1001
    const std::size_t comma = value.find(',', start);
    // branch-gate: BG-1001
    if (comma == std::string_view::npos) {
      fields.push_back(value.substr(start));
      break;
    }
    fields.push_back(value.substr(start, comma - start));
    start = comma + 1U;
  }
  return fields;
}

bool parseProductAutomationCsvFloat(std::string_view value, float& out) {
  return parseProductAutomationFloat(value, out);
}

bool parseProductAutomationFloorCommand(std::string_view value,
                                        RoomEditCommand& command) {
  const std::vector<std::string_view> fields = splitProductAutomationCsv(value);
  // branch-gate: BG-1001
  if (fields.size() != 7U && fields.size() != 8U) {
    return false;
  }

  EditableRoomFloor floor;
  floor.id = std::string(fields[0]);
  // branch-gate: BG-1001
  if (floor.id.empty() ||
      !parseProductAutomationCsvFloat(fields[1], floor.centerMeters.x) ||
      !parseProductAutomationCsvFloat(fields[2], floor.centerMeters.y) ||
      !parseProductAutomationCsvFloat(fields[3], floor.centerMeters.z) ||
      !parseProductAutomationCsvFloat(fields[4], floor.sizeMeters.x) ||
      !parseProductAutomationCsvFloat(fields[5], floor.sizeMeters.y) ||
      !parseProductAutomationCsvFloat(fields[6], floor.sizeMeters.z)) {
    return false;
  }
  // branch-gate: BG-1001
  floor.semantics = defaultFloorSemantics(
      fields.size() == 8U && !fields[7].empty()
          ? std::string(fields[7])
          : std::string{"debug_floor"});
  command = addFloorCommand(std::move(floor));
  return true;
}

bool parseProductAutomationWallCommand(std::string_view value,
                                       RoomEditCommand& command) {
  const std::vector<std::string_view> fields = splitProductAutomationCsv(value);
  // branch-gate: BG-1001
  if (fields.size() != 10U && fields.size() != 11U) {
    return false;
  }

  EditableRoomWall wall;
  wall.id = std::string(fields[0]);
  // branch-gate: BG-1001
  if (wall.id.empty() ||
      !parseProductAutomationCsvFloat(fields[1], wall.startMeters.x) ||
      !parseProductAutomationCsvFloat(fields[2], wall.startMeters.y) ||
      !parseProductAutomationCsvFloat(fields[3], wall.startMeters.z) ||
      !parseProductAutomationCsvFloat(fields[4], wall.endMeters.x) ||
      !parseProductAutomationCsvFloat(fields[5], wall.endMeters.y) ||
      !parseProductAutomationCsvFloat(fields[6], wall.endMeters.z) ||
      !parseProductAutomationCsvFloat(fields[7], wall.bottomY) ||
      !parseProductAutomationCsvFloat(fields[8], wall.heightMeters) ||
      !parseProductAutomationCsvFloat(fields[9], wall.thicknessMeters)) {
    return false;
  }
  // branch-gate: BG-1001
  wall.semantics = defaultWallSemantics(
      fields.size() == 11U && !fields[10].empty()
          ? std::string(fields[10])
          : std::string{"debug_wall"});
  command = addWallCommand(std::move(wall));
  return true;
}

template <typename Value>
struct AutomationParserRow {
  std::string_view name;
  Value value;
};

struct RoomEditorInputActionRow {
  std::string_view name;
  InputAction action;
  float value;
};

template <typename Value, std::size_t Count>
bool parseAutomationTableValue(
    std::string_view value,
    const std::array<AutomationParserRow<Value>, Count>& rows,
    Value& out) {
  const auto row = std::find_if(
      rows.begin(), rows.end(),
      [value](const AutomationParserRow<Value>& candidate) {
        return candidate.name == value;
      });
  // branch-gate: BG-1001
  if (row == rows.end()) {
    return false;
  }
  out = row->value;
  return true;
}

bool parseProductRoomEditorDirection(std::string_view value,
                                     ProductRoomEditorDirection& out) {
  static constexpr std::array rows{
      AutomationParserRow<ProductRoomEditorDirection>{
          "up", ProductRoomEditorDirection::Up},
      AutomationParserRow<ProductRoomEditorDirection>{
          "down", ProductRoomEditorDirection::Down},
      AutomationParserRow<ProductRoomEditorDirection>{
          "left", ProductRoomEditorDirection::Left},
      AutomationParserRow<ProductRoomEditorDirection>{
          "right", ProductRoomEditorDirection::Right},
  };
  return parseAutomationTableValue(value, rows, out);
}

bool parseProductRoomEditorTool(std::string_view value,
                                ProductRoomEditorTool& out) {
  static constexpr std::array rows{
      AutomationParserRow<ProductRoomEditorTool>{
          "floor", ProductRoomEditorTool::Floor},
      AutomationParserRow<ProductRoomEditorTool>{
          "wall", ProductRoomEditorTool::Wall},
      AutomationParserRow<ProductRoomEditorTool>{
          "object", ProductRoomEditorTool::Object},
  };
  return parseAutomationTableValue(value, rows, out);
}

bool parseProductRoomEditorInputAction(std::string_view value,
                                       InputAction& out,
                                       float& actionValue) {
  static constexpr std::array rows{
      RoomEditorInputActionRow{"editor.nudge_x_pos",
                               InputAction::EditorNudgeX, 1.0F},
      RoomEditorInputActionRow{"editor.nudge_x_neg",
                               InputAction::EditorNudgeX, -1.0F},
      RoomEditorInputActionRow{"editor.nudge_z_pos",
                               InputAction::EditorNudgeZ, 1.0F},
      RoomEditorInputActionRow{"editor.nudge_z_neg",
                               InputAction::EditorNudgeZ, -1.0F},
      RoomEditorInputActionRow{"editor.next_tool",
                               InputAction::EditorNextTool, 1.0F},
      RoomEditorInputActionRow{"editor.previous_tool",
                               InputAction::EditorPreviousTool, 1.0F},
      RoomEditorInputActionRow{"editor.select_floor_tool",
                               InputAction::EditorSelectFloorTool, 1.0F},
      RoomEditorInputActionRow{"editor.select_wall_tool",
                               InputAction::EditorSelectWallTool, 1.0F},
      RoomEditorInputActionRow{"editor.rotate_wall_direction",
                               InputAction::EditorRotateWallDirection, 1.0F},
      RoomEditorInputActionRow{"editor.preview",
                               InputAction::EditorPreviewPlacement, 1.0F},
      RoomEditorInputActionRow{"editor.preview_confirm",
                               InputAction::EditorConfirmPreview, 1.0F},
      RoomEditorInputActionRow{"editor.preview_cancel",
                               InputAction::EditorCancelPreview, 1.0F},
      RoomEditorInputActionRow{"editor.place", InputAction::EditorPlace, 1.0F},
      RoomEditorInputActionRow{"editor.apply", InputAction::EditorApply, 1.0F},
      RoomEditorInputActionRow{"editor.delete", InputAction::EditorDelete, 1.0F},
      RoomEditorInputActionRow{"editor.undo", InputAction::EditorUndo, 1.0F},
      RoomEditorInputActionRow{"editor.redo", InputAction::EditorRedo, 1.0F},
  };
  actionValue = 1.0F;
  const auto row = std::find_if(
      rows.begin(), rows.end(),
      [value](const RoomEditorInputActionRow& candidate) {
        return candidate.name == value;
      });
  // branch-gate: BG-1001
  if (row == rows.end()) {
    return false;
  }
  out = row->action;
  actionValue = row->value;
  return true;
}

bool parseProductAutomationOwner(std::string_view value, MenuOwner& out) {
  static constexpr std::array rows{
      AutomationParserRow<MenuOwner>{"starter", MenuOwner::Starter},
      AutomationParserRow<MenuOwner>{"pause", MenuOwner::Pause},
      AutomationParserRow<MenuOwner>{"settings", MenuOwner::Settings},
      AutomationParserRow<MenuOwner>{"dev_tools", MenuOwner::DevTools},
  };
  return parseAutomationTableValue(value, rows, out);
}

bool hasProductAutomationKey(const std::vector<ProductAutomationCommand>& commands,
                             const std::string& key) {
  for (const ProductAutomationCommand& command : commands) {
    // branch-gate: BG-1001
    if (command.key == key) {
      return true;
    }
  }
  return false;
}

bool readProductAutomationCommands(const std::filesystem::path& path,
                                   ProductAppWindowState& window,
                                   std::vector<ProductAutomationCommand>& commands) {
  window.automationControlRequested = !path.empty();
  // branch-gate: BG-1001
  window.automationControlPath = path.empty() ? "" : path.generic_string();
  // branch-gate: BG-1001
  if (path.empty()) {
    return false;
  }
  window.automationControlScope = "frontend_menu";
  std::ifstream input(path);
  // branch-gate: BG-1001
  if (!input) {
    window.automationControlStatus = "read_failed";
    window.automationControlLastResult = "failed";
    return false;
  }

  std::string line;
  while (std::getline(input, line)) {
    // branch-gate: BG-1001
    if (line.empty()) {
      continue;
    }
    ++window.automationControlLineCount;
    const std::size_t equals = line.find('=');
    // branch-gate: BG-1001
    if (equals == std::string::npos || equals == 0U) {
      window.automationControlStatus = "parse_error";
      window.automationControlLastKey = "none";
      window.automationControlLastResult = "failed";
      return false;
    }
    ProductAutomationCommand command{line.substr(0, equals), line.substr(equals + 1U)};
    // branch-gate: BG-1001
    if (hasProductAutomationKey(commands, command.key)) {
      window.automationControlStatus = "duplicate_key";
      window.automationControlLastKey = command.key;
      window.automationControlLastResult = "failed";
      return false;
    }
    commands.push_back(std::move(command));
  }
  window.automationControlLoaded = true;
  window.automationControlStatus = "loaded";
  return true;
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
      {"world.ascii_room_file", {"world_setup.ascii_room_file"},
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
      {"room_editor.mouse_pick", {"frontend.room_editor_mouse_pick"},
       Category::RoomEditor, Value::Csv, true, false, "room_editor"},
      {"room_editor.preview",
       {"frontend.room_editor_preview", "room_editor.preview_place",
        "frontend.room_editor_preview_place"},
       Category::RoomEditor, Value::Bool, true, true, "room_editor"},
      {"room_editor.preview_confirm",
       {"frontend.room_editor_preview_confirm", "room_editor.confirm_place",
        "frontend.room_editor_confirm_place"},
       Category::RoomEditor, Value::Bool, true, true, "room_editor"},
      {"room_editor.preview_cancel",
       {"frontend.room_editor_preview_cancel", "room_editor.cancel_preview",
        "frontend.room_editor_cancel_preview"},
       Category::RoomEditor, Value::Bool, true, true, "room_editor"},
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
      {"gameplay.jump", {"game.jump", "frontend.game_jump"},
       Category::GameplayInput, Value::Bool, true, true, "gameplay"},
      {"gameplay.crouch", {"game.crouch"},
       Category::GameplayInput, Value::Bool, true, false, "gameplay"},
      {"gameplay.player_position", {"game.player_position"},
       Category::GameplayInput, Value::Csv, true, false, "gameplay"},
      {"gameplay.physics_movement", {"frontend.physics_movement"},
       Category::GameplayInput, Value::Bool, true, false, "gameplay"},
      {"controller.input", {"controller.sample"}, Category::ControllerInput,
       Value::Action, true, false, "controller"},

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
          true, "save.select", Id::SaveSelect, Category::SaveBrowser, Value::String,
          InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "save.delete", Id::SaveDelete, Category::SaveBrowser, Value::Bool,
          InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "save.show_deleted", Id::SaveShowDeleted, Category::SaveBrowser,
          Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "save.deleted_select", Id::SaveDeletedSelect,
          Category::DeletedSaveBrowser, Value::String, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "save.recover", Id::SaveRecover, Category::DeletedSaveBrowser,
          Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "game.move_x", Id::GameplayMoveX, Category::GameplayInput,
          Value::Float, InputAction::PlayerMoveX},
      ProductAutomationCommandDispatchSpec{
          true, "game.move_y", Id::GameplayMoveY, Category::GameplayInput,
          Value::Float, InputAction::PlayerMoveY},
      ProductAutomationCommandDispatchSpec{
          true, "game.attack", Id::GameplayAttack, Category::GameplayInput,
          Value::Bool, InputAction::PlayerAttack},
      ProductAutomationCommandDispatchSpec{
          true, "game.interact", Id::GameplayInteract, Category::GameplayInput,
          Value::Bool, InputAction::PlayerInteract},
      ProductAutomationCommandDispatchSpec{
          true, "gameplay.jump", Id::GameplayJump, Category::GameplayInput,
          Value::Bool, InputAction::PlayerJump},
      ProductAutomationCommandDispatchSpec{
          true, "gameplay.crouch", Id::GameplayCrouch, Category::GameplayInput,
          Value::Bool, InputAction::PlayerCrouch},
      ProductAutomationCommandDispatchSpec{
          true, "gameplay.player_position", Id::GameplayPlayerPosition,
          Category::GameplayInput, Value::Csv, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "gameplay.physics_movement", Id::GameplayPhysicsMovement,
          Category::GameplayInput, Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "controller.input", Id::ControllerInput,
          Category::ControllerInput, Value::Action, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "frontend.execute", Id::FrontendExecute,
          Category::FrontendExecute, Value::Bool, InputAction::MenuConfirm},
      ProductAutomationCommandDispatchSpec{
          true, "settings.apply", Id::SettingsApply, Category::Settings,
          Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "settings.restore_defaults", Id::SettingsRestoreDefaults,
          Category::Settings, Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "settings.back", Id::SettingsBack, Category::Settings,
          Value::Bool, InputAction::MenuBack},
      ProductAutomationCommandDispatchSpec{
          true, "frontend.return_to_title", Id::FrontendReturnToTitle,
          Category::System, Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "system.pause", Id::SystemPause, Category::System,
          Value::Bool, InputAction::SystemPause},
      ProductAutomationCommandDispatchSpec{
          true, "system.quit", Id::SystemQuit, Category::System,
          Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "world.create", Id::WorldCreate, Category::WorldSetup,
          Value::Bool, InputAction::MenuConfirm},
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
          true, "room_editor.mouse_pick", Id::RoomEditorMousePick,
          Category::RoomEditor, Value::Csv, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_editor.preview", Id::RoomEditorPreview,
          Category::RoomEditor, Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_editor.preview_confirm", Id::RoomEditorPreviewConfirm,
          Category::RoomEditor, Value::Bool, InputAction::None},
      ProductAutomationCommandDispatchSpec{
          true, "room_editor.preview_cancel", Id::RoomEditorPreviewCancel,
          Category::RoomEditor, Value::Bool, InputAction::None},
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

ProductMenuShortcutAutomationResult resolveProductMenuShortcutAutomation(
    const ProductAutomationCommandDispatchSpec& spec,
    std::string_view value) {
  struct MenuShortcutBoolRow {
    std::string_view name;
    bool value;
  };
  static constexpr std::array rows{
      MenuShortcutBoolRow{"true", true},
      MenuShortcutBoolRow{"1", true},
      MenuShortcutBoolRow{"yes", true},
      MenuShortcutBoolRow{"false", false},
      MenuShortcutBoolRow{"0", false},
      MenuShortcutBoolRow{"no", false},
  };
  static constexpr std::array lookup{
      MenuShortcutBoolRow{"true", true},
      MenuShortcutBoolRow{"1", true},
      MenuShortcutBoolRow{"yes", true},
      MenuShortcutBoolRow{"false", false},
      MenuShortcutBoolRow{"0", false},
      MenuShortcutBoolRow{"no", false},
      MenuShortcutBoolRow{"unknown", false},
  };
  const auto row = std::find_if(
      rows.begin(), rows.end(), [value](const MenuShortcutBoolRow& candidate) {
        return candidate.name == value;
      });
  const std::size_t rowIndex =
      static_cast<std::size_t>(std::distance(rows.begin(), row));
  const std::size_t selectedIndex =
      std::min(rowIndex, lookup.size() - 1U);
  const MenuShortcutBoolRow& parsed = lookup[selectedIndex];

  ProductMenuShortcutAutomationResult result;
  result.valid = rowIndex != rows.size();
  result.routeRequested = result.valid && parsed.value;
  result.inputAction = spec.inputAction;
  return result;
}

ProductMenuInputAutomationResult resolveProductMenuInputAutomation(
    std::string_view value) {
  struct MenuInputRow {
    std::string_view name;
    InputAction action;
  };
  static constexpr std::array rows{
      MenuInputRow{"up", InputAction::MenuUp},
      MenuInputRow{"down", InputAction::MenuDown},
      MenuInputRow{"left", InputAction::MenuLeft},
      MenuInputRow{"right", InputAction::MenuRight},
      MenuInputRow{"confirm", InputAction::MenuConfirm},
      MenuInputRow{"back", InputAction::MenuBack},
      MenuInputRow{"next_tab", InputAction::MenuNextTab},
      MenuInputRow{"previous_tab", InputAction::MenuPreviousTab},
      MenuInputRow{"none", InputAction::None},
  };
  static constexpr std::array lookup{
      MenuInputRow{"up", InputAction::MenuUp},
      MenuInputRow{"down", InputAction::MenuDown},
      MenuInputRow{"left", InputAction::MenuLeft},
      MenuInputRow{"right", InputAction::MenuRight},
      MenuInputRow{"confirm", InputAction::MenuConfirm},
      MenuInputRow{"back", InputAction::MenuBack},
      MenuInputRow{"next_tab", InputAction::MenuNextTab},
      MenuInputRow{"previous_tab", InputAction::MenuPreviousTab},
      MenuInputRow{"none", InputAction::None},
      MenuInputRow{"unknown", InputAction::None},
  };
  const auto row = std::find_if(
      rows.begin(), rows.end(), [value](const MenuInputRow& candidate) {
        return candidate.name == value;
      });
  const std::size_t rowIndex =
      static_cast<std::size_t>(std::distance(rows.begin(), row));
  const std::size_t selectedIndex =
      std::min(rowIndex, lookup.size() - 1U);

  ProductMenuInputAutomationResult result;
  result.valid = rowIndex != rows.size();
  result.inputAction = lookup[selectedIndex].action;
  return result;
}

ProductFrontendSelectAutomationResult resolveProductFrontendSelectAutomation(
    std::string_view value) {
  struct FrontendSelectRow {
    std::string_view name;
    FrontendAction action;
  };
  static constexpr std::array rows{
      FrontendSelectRow{"continue", FrontendAction::Continue},
      FrontendSelectRow{"new_world", FrontendAction::NewWorld},
      FrontendSelectRow{"load_save", FrontendAction::LoadSave},
      FrontendSelectRow{"settings", FrontendAction::Settings},
      FrontendSelectRow{"dev_tools", FrontendAction::DevTools},
      FrontendSelectRow{"exit", FrontendAction::Exit},
      FrontendSelectRow{"resume", FrontendAction::Resume},
      FrontendSelectRow{"edit_room", FrontendAction::EditRoom},
      FrontendSelectRow{"leave_editor", FrontendAction::LeaveEditor},
      FrontendSelectRow{"save", FrontendAction::Save},
      FrontendSelectRow{"save_and_exit", FrontendAction::SaveAndExit},
      FrontendSelectRow{"return_to_title", FrontendAction::ReturnToTitle},
      FrontendSelectRow{"exit_game", FrontendAction::ExitGame},
  };
  static constexpr std::array lookup{
      FrontendSelectRow{"continue", FrontendAction::Continue},
      FrontendSelectRow{"new_world", FrontendAction::NewWorld},
      FrontendSelectRow{"load_save", FrontendAction::LoadSave},
      FrontendSelectRow{"settings", FrontendAction::Settings},
      FrontendSelectRow{"dev_tools", FrontendAction::DevTools},
      FrontendSelectRow{"exit", FrontendAction::Exit},
      FrontendSelectRow{"resume", FrontendAction::Resume},
      FrontendSelectRow{"edit_room", FrontendAction::EditRoom},
      FrontendSelectRow{"leave_editor", FrontendAction::LeaveEditor},
      FrontendSelectRow{"save", FrontendAction::Save},
      FrontendSelectRow{"save_and_exit", FrontendAction::SaveAndExit},
      FrontendSelectRow{"return_to_title", FrontendAction::ReturnToTitle},
      FrontendSelectRow{"exit_game", FrontendAction::ExitGame},
      FrontendSelectRow{"unknown", FrontendAction::None},
  };
  const auto row = std::find_if(
      rows.begin(), rows.end(), [value](const FrontendSelectRow& candidate) {
        return candidate.name == value;
      });
  const std::size_t rowIndex =
      static_cast<std::size_t>(std::distance(rows.begin(), row));
  const std::size_t selectedIndex =
      std::min(rowIndex, lookup.size() - 1U);

  ProductFrontendSelectAutomationResult result;
  result.valid = rowIndex != rows.size();
  result.action = lookup[selectedIndex].action;
  return result;
}

ProductSettingsTabAutomationResult resolveProductSettingsTabAutomation(
    std::string_view value) {
  struct SettingsTabRow {
    std::string_view name;
    FrontendSettingsTab settingsTab;
  };
  static constexpr std::array rows{
      SettingsTabRow{"input", FrontendSettingsTab::Input},
      SettingsTabRow{"controls", FrontendSettingsTab::Controls},
      SettingsTabRow{"camera", FrontendSettingsTab::Camera},
      SettingsTabRow{"gameplay", FrontendSettingsTab::Gameplay},
      SettingsTabRow{"video_display", FrontendSettingsTab::VideoDisplay},
      SettingsTabRow{"audio", FrontendSettingsTab::Audio},
      SettingsTabRow{"accessibility", FrontendSettingsTab::Accessibility},
      SettingsTabRow{"developer", FrontendSettingsTab::Developer},
  };
  static constexpr std::array lookup{
      SettingsTabRow{"input", FrontendSettingsTab::Input},
      SettingsTabRow{"controls", FrontendSettingsTab::Controls},
      SettingsTabRow{"camera", FrontendSettingsTab::Camera},
      SettingsTabRow{"gameplay", FrontendSettingsTab::Gameplay},
      SettingsTabRow{"video_display", FrontendSettingsTab::VideoDisplay},
      SettingsTabRow{"audio", FrontendSettingsTab::Audio},
      SettingsTabRow{"accessibility", FrontendSettingsTab::Accessibility},
      SettingsTabRow{"developer", FrontendSettingsTab::Developer},
      SettingsTabRow{"unknown", FrontendSettingsTab::None},
  };
  ProductSettingsTabAutomationResult result;
  const auto row = std::find_if(
      rows.begin(), rows.end(), [value](const SettingsTabRow& candidate) {
        return candidate.name == value;
      });
  const std::size_t rowIndex =
      static_cast<std::size_t>(std::distance(rows.begin(), row));
  const std::size_t selectedIndex =
      std::min(rowIndex, lookup.size() - 1U);
  result.valid = row != rows.end();
  result.settingsTab = lookup[selectedIndex].settingsTab;
  return result;
}

ProductDevToolsCategoryAutomationResult resolveProductDevToolsCategoryAutomation(
    std::string_view value) {
  struct DevToolsCategoryRow {
    std::string_view name;
    FrontendDevToolsCategory category;
  };
  static constexpr std::array rows{
      DevToolsCategoryRow{"session", FrontendDevToolsCategory::Session},
      DevToolsCategoryRow{"input", FrontendDevToolsCategory::Input},
      DevToolsCategoryRow{"player", FrontendDevToolsCategory::Player},
      DevToolsCategoryRow{"movement", FrontendDevToolsCategory::Movement},
      DevToolsCategoryRow{"world_editor", FrontendDevToolsCategory::WorldEditor},
      DevToolsCategoryRow{"collision", FrontendDevToolsCategory::Collision},
      DevToolsCategoryRow{"spells", FrontendDevToolsCategory::Spells},
      DevToolsCategoryRow{"camera", FrontendDevToolsCategory::Camera},
      DevToolsCategoryRow{"renderer", FrontendDevToolsCategory::Renderer},
      DevToolsCategoryRow{"performance", FrontendDevToolsCategory::Performance},
  };
  static constexpr std::array lookup{
      DevToolsCategoryRow{"session", FrontendDevToolsCategory::Session},
      DevToolsCategoryRow{"input", FrontendDevToolsCategory::Input},
      DevToolsCategoryRow{"player", FrontendDevToolsCategory::Player},
      DevToolsCategoryRow{"movement", FrontendDevToolsCategory::Movement},
      DevToolsCategoryRow{"world_editor", FrontendDevToolsCategory::WorldEditor},
      DevToolsCategoryRow{"collision", FrontendDevToolsCategory::Collision},
      DevToolsCategoryRow{"spells", FrontendDevToolsCategory::Spells},
      DevToolsCategoryRow{"camera", FrontendDevToolsCategory::Camera},
      DevToolsCategoryRow{"renderer", FrontendDevToolsCategory::Renderer},
      DevToolsCategoryRow{"performance", FrontendDevToolsCategory::Performance},
      DevToolsCategoryRow{"unknown", FrontendDevToolsCategory::None},
  };
  ProductDevToolsCategoryAutomationResult result;
  const auto row = std::find_if(
      rows.begin(), rows.end(), [value](const DevToolsCategoryRow& candidate) {
        return candidate.name == value;
      });
  const std::size_t rowIndex =
      static_cast<std::size_t>(std::distance(rows.begin(), row));
  const std::size_t selectedIndex =
      std::min(rowIndex, lookup.size() - 1U);
  result.valid = row != rows.end();
  result.category = lookup[selectedIndex].category;
  return result;
}

ProductSaveSelectionAutomationResult resolveProductSaveSelectionAutomation(
    std::string_view value) {
  ProductSaveSelectionAutomationResult result;
  result.valid = !value.empty();
  result.saveId = value;
  return result;
}

ProductBoolAutomationResult resolveProductAutomationBool(
    std::string_view value) {
  struct BoolRow {
    std::string_view name;
    bool requested;
  };
  static constexpr std::array rows{
      BoolRow{"true", true},
      BoolRow{"1", true},
      BoolRow{"yes", true},
      BoolRow{"false", false},
      BoolRow{"0", false},
      BoolRow{"no", false},
  };
  static constexpr std::array lookup{
      BoolRow{"true", true},
      BoolRow{"1", true},
      BoolRow{"yes", true},
      BoolRow{"false", false},
      BoolRow{"0", false},
      BoolRow{"no", false},
      BoolRow{"unknown", false},
  };
  const auto row = std::find_if(
      rows.begin(), rows.end(), [value](const BoolRow& candidate) {
        return candidate.name == value;
      });
  const std::size_t rowIndex =
      static_cast<std::size_t>(std::distance(rows.begin(), row));
  const std::size_t selectedIndex =
      std::min(rowIndex, lookup.size() - 1U);

  ProductBoolAutomationResult result;
  result.valid = row != rows.end();
  result.requested = lookup[selectedIndex].requested;
  return result;
}

bool resolveProductAutomationBool(std::string_view value, bool& out) {
  const ProductBoolAutomationResult result = resolveProductAutomationBool(value);
  out = result.requested;
  return result.valid;
}

ProductGameplayAxisAutomationResult resolveProductGameplayAxisAutomation(
    std::string_view value) {
  ProductGameplayAxisAutomationResult result;
  float parsedValue = 0.0F;
  const auto [ptr, error] =
      std::from_chars(value.data(), value.data() + value.size(), parsedValue);
  result.valid = error == std::errc{} && ptr == value.data() + value.size() &&
                 std::isfinite(parsedValue);
  result.value = parsedValue;
  return result;
}

ProductNonEmptyStringAutomationResult resolveProductNonEmptyStringAutomation(
    std::string_view value) {
  ProductNonEmptyStringAutomationResult result;
  result.valid = !value.empty();
  result.value = value;
  return result;
}

ProductDungeonDraftDirectionAutomationResult resolveProductDungeonDraftDirectionAutomation(
    std::string_view value) {
  struct DirectionRow {
    std::string_view name;
    ProductDungeonDraftDirection direction;
  };
  static constexpr std::array rows{
      DirectionRow{"up", ProductDungeonDraftDirection::Up},
      DirectionRow{"down", ProductDungeonDraftDirection::Down},
      DirectionRow{"left", ProductDungeonDraftDirection::Left},
      DirectionRow{"right", ProductDungeonDraftDirection::Right},
  };
  static constexpr std::array lookup{
      DirectionRow{"up", ProductDungeonDraftDirection::Up},
      DirectionRow{"down", ProductDungeonDraftDirection::Down},
      DirectionRow{"left", ProductDungeonDraftDirection::Left},
      DirectionRow{"right", ProductDungeonDraftDirection::Right},
      DirectionRow{"unknown", ProductDungeonDraftDirection::Up},
  };
  const auto row = std::find_if(
      rows.begin(), rows.end(), [value](const DirectionRow& candidate) {
        return candidate.name == value;
      });
  const std::size_t rowIndex =
      static_cast<std::size_t>(std::distance(rows.begin(), row));
  const std::size_t selectedIndex =
      std::min(rowIndex, lookup.size() - 1U);
  ProductDungeonDraftDirectionAutomationResult result;
  result.valid = row != rows.end();
  result.direction = lookup[selectedIndex].direction;
  return result;
}

ProductDungeonDraftPaintAutomationResult resolveProductDungeonDraftPaintAutomation(
    std::string_view value) {
  ProductDungeonDraftPaintAutomationResult result;
  result.valid = value.size() == 1U;
  result.glyph = value;
  return result;
}

bool parseProductAutomationSize(std::string_view value, std::size_t& out) {
  const auto [ptr, error] =
      std::from_chars(value.data(), value.data() + value.size(), out);
  return error == std::errc{} && ptr == value.data() + value.size();
}

ProductDungeonDraftCellAutomationResult resolveProductDungeonDraftCellAutomation(
    std::string_view rowValue,
    std::string_view columnValue,
    std::string_view glyphValue) {
  ProductDungeonDraftCellAutomationResult result;
  std::size_t row = 0;
  std::size_t column = 0;
  result.valid = glyphValue.size() == 1U &&
                 parseProductAutomationSize(rowValue, row) &&
                 parseProductAutomationSize(columnValue, column);
  result.row = row;
  result.column = column;
  return result;
}

bool resolveProductSaveBrowserBoolAutomation(std::string_view value,
                                             bool& out) {
  struct BoolRow {
    std::string_view name;
    bool value;
  };
  static constexpr std::array rows{
      BoolRow{"true", true},
      BoolRow{"1", true},
      BoolRow{"yes", true},
      BoolRow{"false", false},
      BoolRow{"0", false},
      BoolRow{"no", false},
  };
  static constexpr std::array lookup{
      BoolRow{"true", true},
      BoolRow{"1", true},
      BoolRow{"yes", true},
      BoolRow{"false", false},
      BoolRow{"0", false},
      BoolRow{"no", false},
      BoolRow{"unknown", false},
  };
  const auto row = std::find_if(
      rows.begin(), rows.end(), [value](const BoolRow& candidate) {
        return candidate.name == value;
      });
  const std::size_t rowIndex =
      static_cast<std::size_t>(std::distance(rows.begin(), row));
  const std::size_t selectedIndex =
      std::min(rowIndex, lookup.size() - 1U);
  out = lookup[selectedIndex].value;
  return row != rows.end();
}

void recordWorldSetupDraftState(const WorldSetupDraft& draft,
                                ProductAppWindowState& window) {
  window.worldSetupTitle = draft.worldName;
  window.worldSetupDungeonTitle = draft.worldName;
  window.worldSetupDungeonCount = productBuiltinDungeonCatalog().size();
  const std::size_t dungeonIndex =
      productBuiltinDungeonIndexForRoomId(draft.asciiRoomId);
  window.worldSetupDungeonIndex = 0;
  // branch-gate: BG-1136
  if (dungeonIndex < productBuiltinDungeonCatalog().size()) {
    window.worldSetupDungeonIndex =
        static_cast<std::uint64_t>(dungeonIndex + 1U);
  }
  window.worldSetupAsciiRoomEnabled = draft.asciiRoomEnabled;
  window.worldSetupAsciiRoomTextPresent = !draft.asciiRoomText.empty();
  // branch-gate: BG-1004
  window.worldSetupAsciiRoomId =
      draft.asciiRoomId.empty() ? "none" : draft.asciiRoomId;
  // branch-gate: BG-1004
  window.worldSetupAsciiRoomSourceName =
      draft.asciiRoomSourceName.empty() ? "none" : draft.asciiRoomSourceName;
  window.asciiRoomDraftText = draft.asciiRoomText;
  // branch-gate: BG-1004
  window.asciiRoomDraftRoomId =
      draft.asciiRoomId.empty() ? "ascii_preview" : draft.asciiRoomId;
  // branch-gate: BG-1004
  window.asciiRoomDraftSourceName =
      draft.asciiRoomSourceName.empty() ? "world_setup_ascii_room" :
                                          draft.asciiRoomSourceName;
  // branch-gate: BG-1004
  if (draft.asciiRoomEnabled && !draft.asciiRoomText.empty()) {
    buildProductAsciiRoomPreviewResult(window);
    return;
  }
  window.asciiRoomPreviewStatus = "not_requested";
  window.asciiRoomPreviewReasonCode = "not_requested";
  window.asciiRoomPreviewFailedStage = "not_started";
  window.asciiRoomPreviewRoomId = "none";
  window.asciiRoomPreviewSourceName = "none";
  window.asciiRoomPreviewReady = false;
  window.asciiRoomPreviewWidth = 0;
  window.asciiRoomPreviewHeight = 0;
  window.asciiRoomPreviewFloorCount = 0;
  window.asciiRoomPreviewWallCount = 0;
  window.asciiRoomPreviewMarkerCount = 0;
  window.asciiRoomPreviewElevatedFloorCount = 0;
  window.asciiRoomPreviewRampCount = 0;
  window.asciiRoomPreviewBlockedSlopeCount = 0;
  window.asciiRoomPreviewStaticMeshCount = 0;
  window.asciiRoomPreviewAnchorCount = 0;
  window.asciiRoomPreviewSpatialSurfaceCount = 0;
  window.asciiRoomPreviewAssetTextWritten = false;
  window.asciiRoomPreviewAssetTextBytes = 0;
}

ProductDungeonDraftCursor dungeonDraftCursorFromWindow(
    const ProductAppWindowState& window) {
  return ProductDungeonDraftCursor{
      static_cast<std::size_t>(window.worldSetupDungeonDraftCursorRow),
      static_cast<std::size_t>(window.worldSetupDungeonDraftCursorColumn),
  };
}

void recordDungeonDraftOperation(ProductAppWindowState& window,
                                 const ProductDungeonDraftOperationResult& result) {
  window.worldSetupDungeonDraftStatus = std::string(result.status);
  window.worldSetupDungeonDraftReasonCode = std::string(result.reasonCode);
  window.worldSetupDungeonDraftCursorRow =
      static_cast<std::uint64_t>(result.cursor.row);
  window.worldSetupDungeonDraftCursorColumn =
      static_cast<std::uint64_t>(result.cursor.column);
  // branch-gate: BG-1004
  window.worldSetupDungeonDraftLastGlyph =
      result.glyph == '\0' ? std::string{"none"} : std::string(1U, result.glyph);
  // branch-gate: BG-1004
  if (result.modified) {
    window.worldSetupDungeonDraftModified = true;
  }
}

void resetDungeonDraftWindowCursor(const WorldSetupDraft& draft,
                                   ProductAppWindowState& window) {
  const ProductDungeonDraftCursor cursor =
      clampProductDungeonDraftCursor(draft, ProductDungeonDraftCursor{});
  window.worldSetupDungeonDraftCursorRow = static_cast<std::uint64_t>(cursor.row);
  window.worldSetupDungeonDraftCursorColumn =
      static_cast<std::uint64_t>(cursor.column);
  window.worldSetupDungeonDraftLastGlyph = "none";
}

bool applyDungeonDraftPaintGlyph(WorldSetupDraft& worldSetupDraft,
                                 ProductAppWindowState& window,
                                 char glyph) {
  // branch-gate: BG-1004
  if (!window.worldSetupDungeonDraftEditMode) {
    window.worldSetupDungeonDraftStatus = "dungeon_draft_edit_mode_off";
    window.worldSetupDungeonDraftReasonCode = "dungeon_draft_edit_mode_off";
    return false;
  }
  ProductDungeonDraftOperationResult painted = paintProductDungeonDraftCell(
      worldSetupDraft, dungeonDraftCursorFromWindow(window), glyph);
  recordDungeonDraftOperation(window, painted);
  recordWorldSetupDraftState(worldSetupDraft, window);
  return painted.ok;
}

bool selectDungeonDraftPaintGlyph(ProductAppWindowState& window, char glyph) {
  // branch-gate: BG-1146
  if (!window.worldSetupDungeonDraftEditMode) {
    window.worldSetupDungeonDraftStatus = "dungeon_draft_edit_mode_off";
    window.worldSetupDungeonDraftReasonCode = "dungeon_draft_edit_mode_off";
    return false;
  }
  // branch-gate: BG-1146
  if (!isProductDungeonDraftGlyph(glyph)) {
    window.worldSetupDungeonDraftStatus = "dungeon_draft_invalid_glyph";
    window.worldSetupDungeonDraftReasonCode = "dungeon_draft_invalid_glyph";
    return false;
  }
  window.worldSetupDungeonDraftSelectedGlyph = std::string(1U, glyph);
  window.worldSetupDungeonDraftStatus = "dungeon_draft_paint_tool_selected";
  window.worldSetupDungeonDraftReasonCode = "dungeon_draft_paint_tool_selected";
  return true;
}

std::string readAutomationTextFile(std::string_view path) {
  std::ifstream input{std::string(path)};
  // branch-gate: BG-1165
  if (!input) {
    return {};
  }
  return std::string((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
}

std::string titleFromAutomationAsciiRoomPath(std::string_view path) {
  std::string title = std::filesystem::path(std::string(path)).stem().string();
  // branch-gate: BG-1165
  if (title.empty()) {
    return std::string{"Custom Draft"};
  }
  for (char& ch : title) {
    // branch-gate: BG-1165
    if (ch == '_' || ch == '-') {
      ch = ' ';
    }
  }
  return title;
}

void markAutomationApplied(ProductAppWindowState& window,
                           const ProductAutomationCommand& command,
                           std::string_view action,
                           MenuOwner owner,
                           std::string_view result) {
  window.automationControlLastKey = command.key;
  window.automationControlLastAction = std::string(action);
  window.automationControlLastOwner = owner;
  window.automationControlLastResult = std::string(result);
  // branch-gate: BG-1002
  if (result == "applied") {
    ++window.automationControlAppliedCount;
    window.automationControlStatus = "applied";
  }
}

ProductAutomationExecutionResult applyProductCommonAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationExecutionContext& context) {
  const std::string_view key{command.key};
  const std::string_view value{command.value};

  // branch-gate: BG-1002
  if (key == "automation.owner") {
    MenuOwner expectedOwner = MenuOwner::None;
    // branch-gate: BG-1002
    if (!parseProductAutomationOwner(value, expectedOwner)) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    const MenuOwner currentOwner = context.currentOwner();
    // branch-gate: BG-1002
    if (currentOwner != expectedOwner) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, menuOwnerName(expectedOwner),
                            currentOwner, "failed");
      return {true, false};
    }
    markAutomationApplied(context.window, command, menuOwnerName(expectedOwner),
                          currentOwner, "applied");
    return {true, true};
  }

  // branch-gate: BG-1002
  if (key == "menu.input") {
    const ProductMenuInputAutomationResult menuInput =
        resolveProductMenuInputAutomation(value);
    // branch-gate: BG-1002
    if (!menuInput.valid) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    const bool routed = context.routeInput(menuInput.inputAction);
    // branch-gate: BG-1002
    markAutomationApplied(context.window, command, inputActionName(menuInput.inputAction),
                          context.window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return {true, routed};
  }

  // branch-gate: BG-1002
  if (automationSpec.commandId == ProductAutomationCommandId::MenuShortcut) {
    const ProductMenuShortcutAutomationResult shortcut =
        resolveProductMenuShortcutAutomation(automationSpec, value);
    // branch-gate: BG-1002
    if (!shortcut.valid) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    // branch-gate: BG-1002
    if (!shortcut.routeRequested) {
      markAutomationApplied(context.window, command, "none", context.currentOwner(),
                            "ignored");
      return {true, true};
    }
    const bool routed = context.routeInput(shortcut.inputAction);
    // branch-gate: BG-1002
    markAutomationApplied(context.window, command, inputActionName(shortcut.inputAction),
                          context.window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return {true, routed};
  }

  // branch-gate: BG-1002
  if (key == "frontend.select" || key == "pause.select") {
    const ProductFrontendSelectAutomationResult select =
        resolveProductFrontendSelectAutomation(value);
    // branch-gate: BG-1002
    if (!select.valid) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    context.frontend.selectedAction = select.action;
    markAutomationApplied(context.window, command, frontendActionName(select.action),
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1002
  if (key == "settings.tab") {
    const ProductSettingsTabAutomationResult settingsResult =
        resolveProductSettingsTabAutomation(value);
    // branch-gate: BG-1002
    if (!settingsResult.valid) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    context.settingsTab = settingsResult.settingsTab;
    context.window.selectedSettingsTab = context.settingsTab;
    markAutomationApplied(context.window, command,
                          frontendSettingsTabName(context.settingsTab),
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1002
  if (key == "dev_tools.category") {
    const ProductDevToolsCategoryAutomationResult devToolsResult =
        resolveProductDevToolsCategoryAutomation(value);
    // branch-gate: BG-1002
    if (!devToolsResult.valid) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    const FrontendDevToolsCategory category = devToolsResult.category;
    context.frontend.devToolsCategory = category;
    markAutomationApplied(context.window, command,
                          frontendDevToolsCategoryName(category),
                          context.currentOwner(), "applied");
    return {true, true};
  }

  return {};
}

ProductAutomationExecutionResult applyProductWorldSetupAutomationCommand(
    const ProductAutomationCommand& command,
    std::string_view canonicalKey,
    ProductAutomationWorldSetupContext& context) {
  const std::string_view value{command.value};

  // branch-gate: BG-1004
  if (canonicalKey == "world.create") {
    bool boolValue = false;
    // branch-gate: BG-1004
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    // branch-gate: BG-1004
    if (!boolValue) {
      markAutomationApplied(context.window, command, "world.create",
                            context.currentOwner(), "ignored");
      return {true, true};
    }
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.create",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const bool editMode = context.window.worldSetupDungeonDraftEditMode;
    context.window.worldSetupDungeonDraftEditMode = false;
    const bool routed = context.routeInput(InputAction::MenuConfirm);
    context.window.worldSetupDungeonDraftEditMode = editMode;
    markAutomationApplied(context.window, command,
                          inputActionName(InputAction::MenuConfirm),
                          context.window.automationControlLastOwner,
                          // branch-gate: BG-1004
                          routed ? "applied" : "failed");
    return {true, routed};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.title") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.title",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    context.worldSetupDraft.worldName = std::string(value);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    context.window.worldSetupStatus = "world_setup_title_updated";
    markAutomationApplied(context.window, command, "world.title",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.dungeon_id") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.dungeon_id",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductNonEmptyStringAutomationResult dungeonId =
        resolveProductNonEmptyStringAutomation(value);
    const std::size_t index = productBuiltinDungeonIndexForRoomId(dungeonId.value);
    const auto fail = [&]() -> ProductAutomationExecutionResult {
      context.window.automationControlStatus = "invalid_value";
      markAutomationApplied(context.window, command, "world.dungeon_id",
                            context.currentOwner(), "failed");
      return {true, false};
    };
    const auto apply = [&]() -> ProductAutomationExecutionResult {
      const bool applied =
          applyProductBuiltinDungeonToDraft(index, context.worldSetupDraft);
      // branch-gate: BG-1004
      if (!applied) {
        return fail();
      }
      recordWorldSetupDraftState(context.worldSetupDraft, context.window);
      context.window.worldSetupDungeonDraftModified = false;
      context.window.worldSetupDungeonDraftEditMode = false;
      resetDungeonDraftWindowCursor(context.worldSetupDraft, context.window);
      context.window.worldSetupStatus = "world_setup_dungeon_selected";
      markAutomationApplied(context.window, command, "world.dungeon_id",
                            context.currentOwner(), "applied");
      return {true, true};
    };
    // branch-gate: BG-1004
    return dungeonId.valid ? apply() : fail();
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.draft_edit_mode") {
    bool boolValue = false;
    // branch-gate: BG-1004
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.draft_edit_mode",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    context.window.worldSetupDungeonDraftEditMode = boolValue;
    // branch-gate: BG-1004
    context.worldSetupDraft.selectedField =
        boolValue ? WorldSetupField::AsciiRoom : WorldSetupField::Create;
    // branch-gate: BG-1004
    context.window.worldSetupDungeonDraftStatus =
        boolValue ? "dungeon_draft_edit_mode_on" : "dungeon_draft_edit_mode_off";
    context.window.worldSetupDungeonDraftReasonCode =
        context.window.worldSetupDungeonDraftStatus;
    resetDungeonDraftWindowCursor(context.worldSetupDraft, context.window);
    markAutomationApplied(context.window, command, "world.draft_edit_mode",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.draft_move") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.draft_move",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    // branch-gate: BG-1004
    if (!context.window.worldSetupDungeonDraftEditMode) {
      context.window.worldSetupDungeonDraftStatus = "dungeon_draft_edit_mode_off";
      context.window.worldSetupDungeonDraftReasonCode =
          "dungeon_draft_edit_mode_off";
      context.window.automationControlStatus = "command_failed";
      markAutomationApplied(context.window, command, "world.draft_move",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductDungeonDraftDirectionAutomationResult direction =
        resolveProductDungeonDraftDirectionAutomation(value);
    // branch-gate: BG-1004
    if (!direction.valid) {
      context.window.automationControlStatus = "invalid_value";
      markAutomationApplied(context.window, command, "world.draft_move",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductDungeonDraftOperationResult moved =
        moveProductDungeonDraftCursor(context.worldSetupDraft,
                                      dungeonDraftCursorFromWindow(context.window),
                                      direction.direction);
    recordDungeonDraftOperation(context.window, moved);
    // branch-gate: BG-1004
    markAutomationApplied(context.window, command, "world.draft_move",
                          context.currentOwner(),
                          moved.ok ? "applied" : "failed");
    // branch-gate: BG-1004
    if (!moved.ok) {
      context.window.automationControlStatus = "command_failed";
    }
    return {true, moved.ok};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.draft_paint") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.draft_paint",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductDungeonDraftPaintAutomationResult paint =
        resolveProductDungeonDraftPaintAutomation(value);
    // branch-gate: BG-1004
    if (!paint.valid) {
      context.window.automationControlStatus = "invalid_value";
      markAutomationApplied(context.window, command, "world.draft_paint",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const bool painted =
        applyDungeonDraftPaintGlyph(context.worldSetupDraft, context.window,
                                    paint.glyph.front());
    // branch-gate: BG-1004
    markAutomationApplied(context.window, command, "world.draft_paint",
                          context.currentOwner(),
                          painted ? "applied" : "failed");
    // branch-gate: BG-1004
    if (!painted) {
      context.window.automationControlStatus = "command_failed";
    }
    return {true, painted};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.draft_cell") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.draft_cell",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const std::vector<std::string_view> fields = splitProductAutomationCsv(value);
    const ProductDungeonDraftCellAutomationResult cell =
        fields.size() == 3U
            ? resolveProductDungeonDraftCellAutomation(fields[0], fields[1],
                                                      fields[2])
            : ProductDungeonDraftCellAutomationResult{};
    // branch-gate: BG-1004
    if (!cell.valid) {
      context.window.automationControlStatus = "invalid_value";
      markAutomationApplied(context.window, command, "world.draft_cell",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    ProductDungeonDraftOperationResult painted =
        setProductDungeonDraftCell(context.worldSetupDraft, cell.row, cell.column,
                                   fields[2].front());
    recordDungeonDraftOperation(context.window, painted);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    // branch-gate: BG-1004
    markAutomationApplied(context.window, command, "world.draft_cell",
                          context.currentOwner(),
                          painted.ok ? "applied" : "failed");
    // branch-gate: BG-1004
    if (!painted.ok) {
      context.window.automationControlStatus = "command_failed";
    }
    return {true, painted.ok};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.ascii_room_text") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.ascii_room_text",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    context.worldSetupDraft.asciiRoomEnabled = true;
    context.worldSetupDraft.asciiRoomText =
        decodeProductAsciiRoomAutomationText(value);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    context.window.worldSetupStatus = "world_setup_ascii_room_text_updated";
    markAutomationApplied(context.window, command, "world.ascii_room_text",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.ascii_room_id") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.ascii_room_id",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductNonEmptyStringAutomationResult asciiRoomId =
        resolveProductNonEmptyStringAutomation(value);
    // branch-gate: BG-1004
    if (!asciiRoomId.valid) {
      context.window.automationControlStatus = "invalid_value";
      markAutomationApplied(context.window, command, "world.ascii_room_id",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    context.worldSetupDraft.asciiRoomEnabled = true;
    context.worldSetupDraft.asciiRoomId = std::string(asciiRoomId.value);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    context.window.worldSetupStatus = "world_setup_ascii_room_id_updated";
    markAutomationApplied(context.window, command, "world.ascii_room_id",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "world.ascii_room_source_name") {
    // branch-gate: BG-1004
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command,
                            "world.ascii_room_source_name",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductNonEmptyStringAutomationResult asciiRoomSourceName =
        resolveProductNonEmptyStringAutomation(value);
    // branch-gate: BG-1004
    if (!asciiRoomSourceName.valid) {
      context.window.automationControlStatus = "invalid_value";
      markAutomationApplied(context.window, command,
                            "world.ascii_room_source_name",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    context.worldSetupDraft.asciiRoomEnabled = true;
    context.worldSetupDraft.asciiRoomSourceName =
        std::string(asciiRoomSourceName.value);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    context.window.worldSetupStatus =
        "world_setup_ascii_room_source_name_updated";
    markAutomationApplied(context.window, command, "world.ascii_room_source_name",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1165
  if (canonicalKey == "world.ascii_room_file") {
    // branch-gate: BG-1165
    if (context.frontend.childScreen != FrontendScreen::NewWorld) {
      context.window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(context.window, command, "world.ascii_room_file",
                            context.currentOwner(), "failed");
      return {true, false};
    }
    const ProductNonEmptyStringAutomationResult asciiRoomFile =
        resolveProductNonEmptyStringAutomation(value);
    const auto fail = [&]() -> ProductAutomationExecutionResult {
      context.window.automationControlStatus = "invalid_value";
      markAutomationApplied(context.window, command, "world.ascii_room_file",
                            context.currentOwner(), "failed");
      return {true, false};
    };
    // branch-gate: BG-1165
    if (!asciiRoomFile.valid) {
      return fail();
    }
    const std::string asciiRoomText =
        readAutomationTextFile(asciiRoomFile.value);
    // branch-gate: BG-1165
    if (asciiRoomText.empty()) {
      return fail();
    }
    context.worldSetupDraft.worldName =
        titleFromAutomationAsciiRoomPath(asciiRoomFile.value);
    context.worldSetupDraft.asciiRoomEnabled = true;
    context.worldSetupDraft.asciiRoomId =
        std::string(productCustomDungeonRoomId());
    context.worldSetupDraft.asciiRoomSourceName = std::string(asciiRoomFile.value);
    context.worldSetupDraft.asciiRoomText = asciiRoomText;
    context.window.worldSetupDungeonDraftModified = false;
    context.window.worldSetupDungeonDraftEditMode = false;
    resetDungeonDraftWindowCursor(context.worldSetupDraft, context.window);
    recordWorldSetupDraftState(context.worldSetupDraft, context.window);
    context.window.worldSetupStatus = "world_setup_ascii_room_file_loaded";
    markAutomationApplied(context.window, command, "world.ascii_room_file",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "ascii_room.text") {
    context.window.asciiRoomDraftText =
        decodeProductAsciiRoomAutomationText(value);
    context.window.asciiRoomPreviewStatus = "ascii_room_text_updated";
    context.window.asciiRoomPreviewReasonCode = "ascii_room_text_updated";
    context.window.asciiRoomPreviewFailedStage = "not_started";
    context.window.asciiRoomPreviewReady = false;
    markAutomationApplied(context.window, command, "ascii_room.text",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "ascii_room.room_id") {
    const ProductNonEmptyStringAutomationResult roomId =
        resolveProductNonEmptyStringAutomation(value);
    // branch-gate: BG-1004
    if (!roomId.valid) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    context.window.asciiRoomDraftRoomId = std::string(roomId.value);
    markAutomationApplied(context.window, command, "ascii_room.room_id",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "ascii_room.source_name") {
    const ProductNonEmptyStringAutomationResult sourceName =
        resolveProductNonEmptyStringAutomation(value);
    // branch-gate: BG-1004
    if (!sourceName.valid) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    context.window.asciiRoomDraftSourceName = std::string(sourceName.value);
    markAutomationApplied(context.window, command, "ascii_room.source_name",
                          context.currentOwner(), "applied");
    return {true, true};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "ascii_room.build") {
    bool boolValue = false;
    // branch-gate: BG-1004
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    // branch-gate: BG-1004
    if (!boolValue) {
      markAutomationApplied(context.window, command, "ascii_room.build",
                            context.currentOwner(), "ignored");
      return {true, true};
    }
    const bool previewBuilt = buildProductAsciiRoomPreview(context.window);
    // branch-gate: BG-1004
    markAutomationApplied(context.window, command, "ascii_room.build",
                          context.currentOwner(),
                          previewBuilt ? "applied" : "failed");
    return {true, previewBuilt};
  }

  // branch-gate: BG-1004
  if (canonicalKey == "ascii_room.activate") {
    bool boolValue = false;
    // branch-gate: BG-1004
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControlStatus = "invalid_value";
      return {true, false};
    }
    // branch-gate: BG-1004
    if (!boolValue) {
      markAutomationApplied(context.window, command, "ascii_room.activate",
                            context.currentOwner(), "ignored");
      return {true, true};
    }
    const bool activated = context.activateAsciiRoom();
    // branch-gate: BG-1004
    markAutomationApplied(context.window, command, "ascii_room.activate",
                          context.currentOwner(),
                          activated ? "applied" : "failed");
    return {true, activated};
  }

  return {};
}

}  // namespace iggy3d

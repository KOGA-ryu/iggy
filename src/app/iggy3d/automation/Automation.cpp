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
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/room_editor/AuthoringController.hpp"
#include "app/iggy3d/room_editor/ActionController.hpp"
#include "app/iggy3d/room_editor/Cursor.hpp"

namespace iggy3d {







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
  window.automationControl.requested = !path.empty();
  // branch-gate: BG-1001
  window.automationControl.path = path.empty() ? "" : path.generic_string();
  // branch-gate: BG-1001
  if (path.empty()) {
    return false;
  }
  window.automationControl.scope = "frontend_menu";
  std::ifstream input(path);
  // branch-gate: BG-1001
  if (!input) {
    window.automationControl.status = "read_failed";
    window.automationControl.lastResult = "failed";
    return false;
  }

  std::string line;
  while (std::getline(input, line)) {
    // branch-gate: BG-1001
    if (line.empty()) {
      continue;
    }
    ++window.automationControl.lineCount;
    const std::size_t equals = line.find('=');
    // branch-gate: BG-1001
    if (equals == std::string::npos || equals == 0U) {
      window.automationControl.status = "parse_error";
      window.automationControl.lastKey = "none";
      window.automationControl.lastResult = "failed";
      return false;
    }
    ProductAutomationCommand command{line.substr(0, equals), line.substr(equals + 1U)};
    // branch-gate: BG-1001
    if (hasProductAutomationKey(commands, command.key)) {
      window.automationControl.status = "duplicate_key";
      window.automationControl.lastKey = command.key;
      window.automationControl.lastResult = "failed";
      return false;
    }
    commands.push_back(std::move(command));
  }
  window.automationControl.loaded = true;
  window.automationControl.status = "loaded";
  return true;
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



















void markAutomationApplied(ProductAppWindowState& window,
                           const ProductAutomationCommand& command,
                           std::string_view action,
                           MenuOwner owner,
                           std::string_view result) {
  window.automationControl.lastKey = command.key;
  window.automationControl.lastAction = std::string(action);
  window.automationControl.lastOwner = owner;
  window.automationControl.lastResult = std::string(result);
  // branch-gate: BG-1002
  if (result == "applied") {
    ++window.automationControl.appliedCount;
    window.automationControl.status = "applied";
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
      context.window.automationControl.status = "invalid_value";
      return {true, false};
    }
    const MenuOwner currentOwner = context.currentOwner();
    // branch-gate: BG-1002
    if (currentOwner != expectedOwner) {
      context.window.automationControl.status = "owner_unavailable";
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
      context.window.automationControl.status = "invalid_value";
      return {true, false};
    }
    const bool routed = context.routeInput(menuInput.inputAction);
    // branch-gate: BG-1002
    markAutomationApplied(context.window, command, inputActionName(menuInput.inputAction),
                          context.window.automationControl.lastOwner,
                          routed ? "applied" : "ignored");
    return {true, routed};
  }

  // branch-gate: BG-1002
  if (automationSpec.commandId == ProductAutomationCommandId::MenuShortcut) {
    const ProductMenuShortcutAutomationResult shortcut =
        resolveProductMenuShortcutAutomation(automationSpec, value);
    // branch-gate: BG-1002
    if (!shortcut.valid) {
      context.window.automationControl.status = "invalid_value";
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
                          context.window.automationControl.lastOwner,
                          routed ? "applied" : "ignored");
    return {true, routed};
  }

  // branch-gate: BG-1002
  if (key == "frontend.select" || key == "pause.select") {
    const ProductFrontendSelectAutomationResult select =
        resolveProductFrontendSelectAutomation(value);
    // branch-gate: BG-1002
    if (!select.valid) {
      context.window.automationControl.status = "invalid_value";
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
      context.window.automationControl.status = "invalid_value";
      return {true, false};
    }
    context.settingsTab = settingsResult.settingsTab;
    context.window.frontendShell.selectedSettingsTab = context.settingsTab;
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
      context.window.automationControl.status = "invalid_value";
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



}  // namespace iggy3d

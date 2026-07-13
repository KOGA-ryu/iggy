#include "EditorToolOptions.hpp"

#include <algorithm>
#include <string>
#include <string_view>

#include "EditorGroup.hpp"
#include "EditorInteraction.hpp"
#include "EditorObjectActions.hpp"
#include "EditorState.hpp"
#include "EditorToolCapabilities.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/platform/SdlWindow.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr cr::CreativeWheelProfile kToolOptionsWheelProfile{
    cr::CreativeWheelPolarity::Reversed,
    cr::CreativeWheelStepMode::Unit,
    1.0e-4F};

struct ToolOptionsLayout {
  std::int32_t panelX = 0;
  std::int32_t panelY = 0;
  std::uint32_t panelWidth = 0;
  std::uint32_t panelHeight = 0;
  std::int32_t rowsY = 0;
  std::uint32_t rowHeight = 42;
  std::int32_t footerY = 0;
};

[[nodiscard]] ToolOptionsLayout toolOptionsLayout(
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::size_t optionCount) noexcept {
  ToolOptionsLayout layout;
  const std::int32_t width = static_cast<std::int32_t>(drawableWidth);
  const std::int32_t height = static_cast<std::int32_t>(drawableHeight);
  const std::int32_t availableWidth = std::max(1, width - 16);
  const std::int32_t availableHeight = std::max(1, height - 16);
  if (optionCount > 0U) {
    const std::int32_t rowBudget = std::max(1, availableHeight - 104);
    const std::int32_t fittedRowHeight =
        rowBudget / static_cast<std::int32_t>(optionCount);
    layout.rowHeight = static_cast<std::uint32_t>(
        std::clamp(fittedRowHeight, 24, 42));
  }
  const std::int32_t desiredHeight =
      104 + static_cast<std::int32_t>(optionCount * layout.rowHeight);
  layout.panelWidth =
      static_cast<std::uint32_t>(std::min(560, availableWidth));
  layout.panelHeight =
      static_cast<std::uint32_t>(std::min(desiredHeight, availableHeight));
  layout.panelX = std::max(
      0, (width - static_cast<std::int32_t>(layout.panelWidth)) / 2);
  layout.panelY = std::max(
      0, (height - static_cast<std::int32_t>(layout.panelHeight)) / 2);
  layout.rowsY = layout.panelY + 52;
  layout.footerY = std::max(
      layout.panelY,
      layout.panelY + static_cast<std::int32_t>(layout.panelHeight) - 44);
  return layout;
}

void appendText(std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
                std::string_view text,
                std::int32_t x,
                std::int32_t y,
                std::uint32_t drawableWidth,
                std::uint32_t drawableHeight,
                float r,
                float g,
                float b) {
  iggy3d::DebugHudLayoutResult layout = iggy3d::layoutDebugHudTextAt(
      text, x, y, drawableWidth, drawableHeight);
  for (iggy3d::DebugHudGlyphQuad& quad : layout.quads) {
    quad.r = r;
    quad.g = g;
    quad.b = b;
    quad.a = 1.0F;
  }
  glyphs.insert(glyphs.end(), layout.quads.begin(), layout.quads.end());
}

[[nodiscard]] std::string toolOptionsTargetLabel(
    const CreativeEditorToolOptionsState& state) {
  switch (describeCreativeEditorToolCapability(state.targetEntry.kind)
              .displayProfile) {
    case CreativeEditorToolDisplayProfile::Selection: {
      if (state.contextSelectionCount == 0U) {
        return "NO SELECTION";
      }
      std::string label(cr::toString(state.contextPrimaryObjectKind));
      label.append(" | ");
      label.append(std::to_string(state.contextSelectionCount));
      label.append(" SELECTED");
      return label;
    }
    case CreativeEditorToolDisplayProfile::Material: {
      std::string label(cr::toString(state.targetEntry.kind));
      if (state.targetEntry.objectKind != cr::CreativeObjectKind::Unknown) {
        label.append(" | ");
        label.append(cr::toString(state.targetEntry.objectKind));
      }
      return label;
    }
    case CreativeEditorToolDisplayProfile::Standard:
    case CreativeEditorToolDisplayProfile::Count:
      return std::string(cr::toString(state.targetEntry.kind));
  }
  return {};
}

void moveSelection(CreativeEditorToolOptionsState& state,
                   std::int32_t direction) noexcept {
  const std::size_t rowCount = creativeEditorToolOptionsRowCount(state);
  if (direction == 0 || rowCount == 0U) {
    return;
  }
  const cr::CreativeWrappedIndexResult next = cr::stepCreativeWrappedIndex(
      state.selectedIndex, rowCount, direction);
  if (next.valid) {
    state.selectedIndex = next.index;
  }
}

void adjustSelection(CreativeEditorState& editor,
                     std::int32_t direction) {
  CreativeEditorToolOptionsState& state = editor.toolOptions;
  if (state.selectedIndex >= state.options.count) {
    return;
  }
  const cr::CreativeToolOptionId option =
      state.options.ids[state.selectedIndex];
  const cr::CreativeToolOptionAdjustReceipt receipt =
      cr::adjustCreativeToolOption(state.draft, option, direction,
                                   editor.brushPalette);
  const bool optionSetChanged =
      option == cr::CreativeToolOptionId::ArrayMode ||
      option == cr::CreativeToolOptionId::MaterialBrushShape ||
      option == cr::CreativeToolOptionId::MaterialBrushMask ||
      option == cr::CreativeToolOptionId::TerrainRodStampMode ||
      option == cr::CreativeToolOptionId::TerrainProfileKind ||
      option == cr::CreativeToolOptionId::TerrainProfileRodPolicy ||
      option == cr::CreativeToolOptionId::TerrainRegionOperation;
  if (receipt.changed && optionSetChanged) {
    state.options =
        creativeEditorToolOptionsForEntry(state.targetEntry, state.draft);
    state.selectedIndex = 0U;
  }
}

[[nodiscard]] bool commitOptions(CreativeEditorState& editor) {
  CreativeEditorToolOptionsState& state = editor.toolOptions;
  if (!cr::isValidCreativeToolSettings(state.draft)) {
    return false;
  }
  const bool enteringTerrainSeed =
      editor.toolSettings.terrainRodStampMode !=
          cr::CreativeTerrainRodStampMode::Seed &&
      state.draft.terrainRodStampMode ==
          cr::CreativeTerrainRodStampMode::Seed;
  if (enteringTerrainSeed && editor.terrain.selectionValid) {
    editor.terrain.heightCells = editor.terrain.selectedOriginal.heightCells;
    editor.terrain.radiusCells = editor.terrain.selectedOriginal.radiusCells;
    editor.terrain.selectionValid = false;
  }
  editor.toolSettings = state.draft;
  static_cast<void>(storeSelectedCreativeMaterialBrushPreset(
      editor.interaction.materialBrushPresets,
      editor.interaction.hotbar, editor.toolSettings));
  editor.placeCellSize =
      cr::creativeSnapIncrementMeters(editor.toolSettings.snapIncrement);
  syncCreativeEditorQuickEdit(editor);
  state.open = false;
  return true;
}

[[nodiscard]] bool commandEnabled(
    const CreativeEditorState& editor,
    const CreativeEditorToolOptionsState& state,
    CreativeEditorToolOptionsCommandId command) noexcept {
  if (creativeEditorCommandIsObjectAction(command)) {
    return creativeEditorObjectActionEnabled(editor, state, command);
  }
  const CreativeMaterialBrushPivotState& pivot =
      editor.interaction.materialBrushPivot;
  switch (command) {
    case CreativeEditorToolOptionsCommandId::SetMaterialBrushSymmetryPivot:
      return pivot.aimAvailable &&
             pivot.documentId != cr::kInvalidDocumentId;
    case CreativeEditorToolOptionsCommandId::ClearMaterialBrushSymmetryPivot:
      return pivot.locked;
    case CreativeEditorToolOptionsCommandId::EditGroupContents:
      return state.contextGroupId != cr::kInvalidObjectId &&
             editor.groupFocus.depth < editor.groupFocus.groupIds.size();
    case CreativeEditorToolOptionsCommandId::TransformSelection:
    case CreativeEditorToolOptionsCommandId::ResetSelectionTransform:
    case CreativeEditorToolOptionsCommandId::DuplicateSelection:
    case CreativeEditorToolOptionsCommandId::DeleteSelection:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionLocked:
    case CreativeEditorToolOptionsCommandId::GroupSelection:
    case CreativeEditorToolOptionsCommandId::UngroupSelection:
    case CreativeEditorToolOptionsCommandId::Count:
      return false;
  }
  return false;
}

[[nodiscard]] std::string pivotCellLabel(
    std::string_view prefix,
    cr::CreativeGridCoord3 cell) {
  std::string label(prefix);
  label.append(" ");
  label.append(std::to_string(cell.x));
  label.append(" ");
  label.append(std::to_string(cell.y));
  label.append(" ");
  label.append(std::to_string(cell.z));
  return label;
}

[[nodiscard]] std::string_view commandLabel(
    CreativeEditorToolOptionsCommandId command) noexcept {
  if (creativeEditorCommandIsObjectAction(command)) {
    return creativeEditorObjectActionLabel(command);
  }
  switch (command) {
    case CreativeEditorToolOptionsCommandId::SetMaterialBrushSymmetryPivot:
      return "SET SYMMETRY PIVOT";
    case CreativeEditorToolOptionsCommandId::ClearMaterialBrushSymmetryPivot:
      return "CLEAR SYMMETRY PIVOT";
    case CreativeEditorToolOptionsCommandId::EditGroupContents:
      return "EDIT GROUP CONTENTS";
    case CreativeEditorToolOptionsCommandId::TransformSelection:
    case CreativeEditorToolOptionsCommandId::ResetSelectionTransform:
    case CreativeEditorToolOptionsCommandId::DuplicateSelection:
    case CreativeEditorToolOptionsCommandId::DeleteSelection:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionLocked:
    case CreativeEditorToolOptionsCommandId::GroupSelection:
    case CreativeEditorToolOptionsCommandId::UngroupSelection:
    case CreativeEditorToolOptionsCommandId::Count:
      return "INVALID COMMAND";
  }
  return "INVALID COMMAND";
}

[[nodiscard]] std::string commandValueLabel(
    const CreativeEditorState& editor,
    CreativeEditorToolOptionsCommandId command) {
  const CreativeEditorToolOptionsState& state = editor.toolOptions;
  if (creativeEditorCommandIsObjectAction(command)) {
    return creativeEditorObjectActionValueLabel(state, command);
  }
  const CreativeMaterialBrushPivotState& pivot =
      editor.interaction.materialBrushPivot;
  switch (command) {
    case CreativeEditorToolOptionsCommandId::SetMaterialBrushSymmetryPivot:
      return pivot.aimAvailable ? pivotCellLabel("AIM", pivot.aimCell)
                                : "NO AIM";
    case CreativeEditorToolOptionsCommandId::ClearMaterialBrushSymmetryPivot:
      return pivot.locked ? pivotCellLabel("LOCKED", pivot.lockedCell)
                          : "NOT SET";
    case CreativeEditorToolOptionsCommandId::EditGroupContents:
      return editor.toolOptions.contextGroupId != cr::kInvalidObjectId
                 ? "ENTER"
                 : "SELECT GROUP";
    case CreativeEditorToolOptionsCommandId::TransformSelection:
    case CreativeEditorToolOptionsCommandId::ResetSelectionTransform:
    case CreativeEditorToolOptionsCommandId::DuplicateSelection:
    case CreativeEditorToolOptionsCommandId::DeleteSelection:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionLocked:
    case CreativeEditorToolOptionsCommandId::GroupSelection:
    case CreativeEditorToolOptionsCommandId::UngroupSelection:
    case CreativeEditorToolOptionsCommandId::Count:
      return "INVALID";
  }
  return "INVALID";
}

void processPointerInput(const CreativeEditorToolOptionsFrameRequest& request,
                         CreativeEditorToolOptionsFrameResult& result) {
  CreativeEditorToolOptionsState& state = request.editor.toolOptions;
  const iggy3d::SdlWindowEventState& events = request.window.eventState();
  if (!events.primaryPointerPressed) {
    return;
  }
  const ToolOptionsLayout layout = toolOptionsLayout(
      request.drawableWidth, request.drawableHeight,
      creativeEditorToolOptionsRowCount(state));
  const cr::CreativeDrawablePointer pointer =
      cr::resolveCreativeDrawablePointer(
          {events.pointerX, events.pointerY, events.windowWidth,
           events.windowHeight, request.drawableWidth, request.drawableHeight,
           events.pointerMoved, events.primaryPointerPressed});
  if (!pointer.valid) {
    return;
  }
  const std::int32_t x = static_cast<std::int32_t>(pointer.x);
  const std::int32_t y = static_cast<std::int32_t>(pointer.y);
  const std::int32_t panelRight =
      layout.panelX + static_cast<std::int32_t>(layout.panelWidth);

  if (y >= layout.rowsY && y < layout.footerY &&
      x >= layout.panelX && x < panelRight) {
    const std::size_t row = static_cast<std::size_t>(
        (y - layout.rowsY) / static_cast<std::int32_t>(layout.rowHeight));
    if (row < creativeEditorToolOptionsRowCount(state)) {
      state.selectedIndex = row;
      if (row >= state.options.count) {
        result.committed =
            activateCreativeEditorToolOptionsSelection(
                request.appState, request.editor);
      } else if (layout.panelWidth >= 112U) {
        const std::int32_t decreaseX = panelRight - 88;
        const std::int32_t increaseX = panelRight - 48;
        if (x >= increaseX) {
          adjustSelection(request.editor, 1);
        } else if (x >= decreaseX) {
          adjustSelection(request.editor, -1);
        }
      }
    }
    return;
  }

  if (y >= layout.footerY && y < layout.footerY + 36) {
    const std::int32_t midpoint = layout.panelX +
                                  static_cast<std::int32_t>(layout.panelWidth) /
                                      2;
    if (x >= layout.panelX && x < midpoint) {
      state.open = false;
    } else if (x >= midpoint && x < panelRight) {
      const bool commandOnly = state.options.count == 0U &&
                               state.commands.count > 0U;
      result.committed =
          commandOnly
              ? activateCreativeEditorToolOptionsSelection(
                    request.appState, request.editor)
              : commitOptions(request.editor);
    }
  }
}

}  // namespace

bool activateCreativeEditorToolOptionsSelection(
    CreativeEditorState& editor) {
  CreativeEditorToolOptionsState& state = editor.toolOptions;
  if (!state.open) {
    return false;
  }
  if (state.selectedIndex < state.options.count) {
    return commitOptions(editor);
  }
  const std::size_t commandIndex =
      state.selectedIndex - state.options.count;
  if (commandIndex >= state.commands.count ||
      describeCreativeEditorToolCapability(state.targetEntry.kind)
              .commandProfile !=
          CreativeEditorToolCommandProfile::MaterialBrush ||
      !cr::isValidCreativeToolSettings(state.draft)) {
    return false;
  }
  const CreativeEditorToolOptionsCommandId command =
      state.commands.ids[commandIndex];
  if (creativeEditorCommandIsObjectAction(command)) {
    return false;
  }
  if (!commandEnabled(editor, state, command)) {
    return false;
  }

  bool accepted = false;
  switch (command) {
    case CreativeEditorToolOptionsCommandId::SetMaterialBrushSymmetryPivot:
      accepted = lockCreativeMaterialBrushPivotFromAim(
          editor.interaction.materialBrushPivot);
      break;
    case CreativeEditorToolOptionsCommandId::ClearMaterialBrushSymmetryPivot:
      accepted = clearCreativeMaterialBrushPivot(
          editor.interaction.materialBrushPivot);
      break;
    case CreativeEditorToolOptionsCommandId::EditGroupContents:
    case CreativeEditorToolOptionsCommandId::TransformSelection:
    case CreativeEditorToolOptionsCommandId::ResetSelectionTransform:
    case CreativeEditorToolOptionsCommandId::DuplicateSelection:
    case CreativeEditorToolOptionsCommandId::DeleteSelection:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionLocked:
    case CreativeEditorToolOptionsCommandId::GroupSelection:
    case CreativeEditorToolOptionsCommandId::UngroupSelection:
      break;
    case CreativeEditorToolOptionsCommandId::Count:
      break;
  }
  return accepted && commitOptions(editor);
}

bool activateCreativeEditorToolOptionsSelection(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor) {
  CreativeEditorToolOptionsState& state = editor.toolOptions;
  if (!state.open || state.selectedIndex < state.options.count) {
    return activateCreativeEditorToolOptionsSelection(editor);
  }
  const std::size_t commandIndex =
      state.selectedIndex - state.options.count;
  if (commandIndex >= state.commands.count ||
      !cr::isValidCreativeToolSettings(state.draft)) {
    return false;
  }
  const CreativeEditorToolOptionsCommandId command =
      state.commands.ids[commandIndex];
  if (!commandEnabled(editor, state, command)) {
    return false;
  }
  if (creativeEditorCommandIsObjectAction(command)) {
    return activateCreativeEditorObjectAction(appState, editor, command);
  }

  bool accepted = false;
  switch (command) {
    case CreativeEditorToolOptionsCommandId::SetMaterialBrushSymmetryPivot:
    case CreativeEditorToolOptionsCommandId::ClearMaterialBrushSymmetryPivot:
      return activateCreativeEditorToolOptionsSelection(editor);
    case CreativeEditorToolOptionsCommandId::EditGroupContents:
      accepted = enterCreativeEditorGroupFocus(
                     appState, editor.groupFocus, state.contextGroupId)
                     .accepted;
      break;
    case CreativeEditorToolOptionsCommandId::TransformSelection:
    case CreativeEditorToolOptionsCommandId::ResetSelectionTransform:
    case CreativeEditorToolOptionsCommandId::DuplicateSelection:
    case CreativeEditorToolOptionsCommandId::DeleteSelection:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionVisibility:
    case CreativeEditorToolOptionsCommandId::ToggleSelectionLocked:
    case CreativeEditorToolOptionsCommandId::GroupSelection:
    case CreativeEditorToolOptionsCommandId::UngroupSelection:
    case CreativeEditorToolOptionsCommandId::Count:
      break;
  }
  if (accepted) {
    state.open = false;
  }
  return accepted;
}

CreativeEditorToolOptionsFrameResult processCreativeEditorToolOptionsFrame(
    const CreativeEditorToolOptionsFrameRequest& request) {
  CreativeEditorToolOptionsFrameResult result;
  CreativeEditorToolOptionsState& state = request.editor.toolOptions;
  const bool wasOpen = state.open;

  if (request.openRequested && !state.open) {
    const cr::CreativeToolOptionList options =
        creativeEditorToolOptionsForEntry(request.requestedEntry,
                                          request.editor.toolSettings);
    const CreativeEditorToolOptionsCommandList commands =
        creativeEditorToolOptionCommandsForEntry(request.requestedEntry);
    if (options.count + commands.count > 0U &&
        !options.capacityExceeded) {
      state.open = true;
      state.targetEntry = request.requestedEntry;
      state.draft = request.editor.toolSettings;
      state.options = options;
      state.commands = commands;
      state.selectedIndex = 0U;
      refreshCreativeEditorObjectActionContext(request.appState, state);
    }
  }

  if (state.open &&
      request.routedInput.context == cr::CreativeInputContext::ToolOptions) {
    for (const cr::CreativeInputActionEvent& event :
         request.routedInput.actionEvents()) {
      if (!state.open) {
        break;
      }
      switch (event.action) {
        case cr::CreativeInputActionId::ToolOptionsPrevious:
          moveSelection(state, -1);
          break;
        case cr::CreativeInputActionId::ToolOptionsNext:
          moveSelection(state, 1);
          break;
        case cr::CreativeInputActionId::ToolOptionsDecrease:
          adjustSelection(request.editor, -1);
          break;
        case cr::CreativeInputActionId::ToolOptionsIncrease:
          adjustSelection(request.editor, 1);
          break;
        case cr::CreativeInputActionId::ToolOptionsConfirm:
          result.committed =
              activateCreativeEditorToolOptionsSelection(
                  request.appState, request.editor);
          break;
        case cr::CreativeInputActionId::ToolOptionsClose:
          state.open = false;
          break;
        default:
          break;
      }
    }
    if (state.open) {
      const std::int32_t wheelSteps = cr::quantizeCreativeWheelSteps(
          request.window.eventState().mouseWheelY,
          kToolOptionsWheelProfile);
      if (wheelSteps != 0) {
        moveSelection(state, wheelSteps);
      }
      processPointerInput(request, result);
    }
  }

  result.openChanged = wasOpen != state.open;
  if (result.openChanged) {
    static_cast<void>(request.window.setTextInputActive(false));
    static_cast<void>(request.window.setRelativeMouseMode(!state.open));
  }
  result.blockWorldActions =
      request.routedInput.context == cr::CreativeInputContext::ToolOptions ||
      state.open || result.openChanged;
  if (result.blockWorldActions) {
    request.editor.interaction.target = {};
    request.editor.volume.cursorValid = false;
  }
  return result;
}

void appendCreativeEditorToolOptionsOverlay(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs) {
  const CreativeEditorToolOptionsState& state = editor.toolOptions;
  if (!state.open || drawableWidth == 0U || drawableHeight == 0U) {
    return;
  }
  const ToolOptionsLayout layout =
      toolOptionsLayout(drawableWidth, drawableHeight,
                        creativeEditorToolOptionsRowCount(state));
  uiRects.push_back(
      {0, 0, drawableWidth, drawableHeight, 0.01F, 0.015F, 0.02F, 0.66F});
  uiRects.push_back({layout.panelX, layout.panelY, layout.panelWidth,
                     layout.panelHeight, 0.05F, 0.06F, 0.07F, 0.98F});
  const bool objectActions =
      describeCreativeEditorToolCapability(state.targetEntry.kind)
          .displayProfile == CreativeEditorToolDisplayProfile::Selection;
  appendText(glyphs, objectActions ? "OBJECT ACTIONS" : "TOOL OPTIONS",
             layout.panelX + 18,
             layout.panelY + 16, drawableWidth, drawableHeight,
             0.91F, 0.94F, 0.96F);
  const std::string targetLabel = toolOptionsTargetLabel(state);
  appendText(glyphs, targetLabel, layout.panelX + 180, layout.panelY + 16,
             drawableWidth, drawableHeight, 0.64F, 0.72F, 0.76F);

  const std::int32_t panelWidth =
      static_cast<std::int32_t>(layout.panelWidth);
  const std::int32_t panelRight = layout.panelX + panelWidth;
  const std::uint32_t rowInset =
      std::min(12U, layout.panelWidth / 2U);
  const std::uint32_t rowWidth =
      std::max(1U, layout.panelWidth - rowInset * 2U);
  const std::int32_t maxTextInset = std::max(0, panelWidth - 1);
  const std::int32_t labelInset = std::min(24, maxTextInset);
  const std::int32_t valueInset = std::clamp(
      std::min(252, panelWidth / 2), labelInset, maxTextInset);
  const bool showAdjustButtons = layout.panelWidth >= 112U;
  const std::uint32_t adjustButtonHeight =
      std::min(30U, std::max(1U, layout.rowHeight - 8U));

  for (std::size_t row = 0; row < state.options.count; ++row) {
    const cr::CreativeToolOptionDescriptor* descriptor =
        cr::creativeToolOptionDescriptor(state.options.ids[row]);
    if (descriptor == nullptr) {
      continue;
    }
    const std::int32_t y =
        layout.rowsY + static_cast<std::int32_t>(row * layout.rowHeight);
    if (y + static_cast<std::int32_t>(layout.rowHeight) > layout.footerY) {
      break;
    }
    const bool selected = row == state.selectedIndex;
    uiRects.push_back({layout.panelX + static_cast<std::int32_t>(rowInset), y,
                       rowWidth,
                       layout.rowHeight - 4U,
                       selected ? 0.13F : 0.07F,
                       selected ? 0.15F : 0.08F,
                       selected ? 0.16F : 0.09F, 0.98F});
    appendText(glyphs, descriptor->label, layout.panelX + labelInset, y + 12,
               drawableWidth, drawableHeight, 0.82F, 0.86F, 0.88F);
    appendText(glyphs,
               cr::creativeToolOptionValueLabel(state.draft, descriptor->id),
               layout.panelX + valueInset, y + 12,
               drawableWidth, drawableHeight, 0.92F, 0.78F, 0.31F);
    if (showAdjustButtons) {
      uiRects.push_back({panelRight - 88, y + 4, 32U, adjustButtonHeight,
                         0.10F, 0.11F, 0.12F, 1.0F});
      uiRects.push_back({panelRight - 48, y + 4, 32U, adjustButtonHeight,
                         0.10F, 0.11F, 0.12F, 1.0F});
      appendText(glyphs, "-", panelRight - 76, y + 11,
                 drawableWidth, drawableHeight, 0.88F, 0.90F, 0.92F);
      appendText(glyphs, "+", panelRight - 37, y + 11,
                 drawableWidth, drawableHeight, 0.88F, 0.90F, 0.92F);
    }
  }

  for (std::size_t commandIndex = 0U;
       commandIndex < state.commands.count; ++commandIndex) {
    const std::size_t row = state.options.count + commandIndex;
    const std::int32_t y =
        layout.rowsY + static_cast<std::int32_t>(row * layout.rowHeight);
    if (y + static_cast<std::int32_t>(layout.rowHeight) > layout.footerY) {
      break;
    }
    const CreativeEditorToolOptionsCommandId command =
        state.commands.ids[commandIndex];
    const bool enabled = commandEnabled(editor, state, command);
    const bool selected = row == state.selectedIndex;
    uiRects.push_back({layout.panelX + static_cast<std::int32_t>(rowInset), y,
                       rowWidth, layout.rowHeight - 4U,
                       selected && enabled ? 0.10F : 0.06F,
                       selected && enabled ? 0.18F : 0.07F,
                       selected && enabled ? 0.12F : 0.08F, 0.98F});
    appendText(glyphs, commandLabel(command), layout.panelX + labelInset,
               y + 12, drawableWidth, drawableHeight,
               enabled ? 0.78F : 0.42F, enabled ? 0.92F : 0.46F,
               enabled ? 0.80F : 0.48F);
    const std::string value = commandValueLabel(editor, command);
    appendText(glyphs, value, layout.panelX + valueInset, y + 12,
               drawableWidth, drawableHeight,
               enabled ? 0.32F : 0.42F, enabled ? 1.0F : 0.46F,
               enabled ? 0.48F : 0.48F);
  }

  const std::uint32_t halfWidth = layout.panelWidth / 2U;
  if (halfWidth > 0U) {
    uiRects.push_back({layout.panelX, layout.footerY, halfWidth, 36U,
                       0.09F, 0.10F, 0.11F, 1.0F});
    uiRects.push_back({layout.panelX + static_cast<std::int32_t>(halfWidth),
                       layout.footerY, layout.panelWidth - halfWidth, 36U,
                       0.82F, 0.72F, 0.26F, 1.0F});
    appendText(glyphs, "CANCEL", layout.panelX + labelInset,
               layout.footerY + 10, drawableWidth, drawableHeight,
               0.78F, 0.82F, 0.84F);
    const bool commandOnly = state.options.count == 0U &&
                             state.commands.count > 0U;
    appendText(glyphs, commandOnly ? "ACTIVATE" : "APPLY",
               layout.panelX + static_cast<std::int32_t>(halfWidth) +
                   labelInset,
               layout.footerY + 10, drawableWidth, drawableHeight,
               0.06F, 0.065F, 0.07F);
  }
}

}  // namespace iggy3d_creative_app

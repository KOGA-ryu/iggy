#include "EditorCatalog.hpp"
#include "EditorCatalogInternal.hpp"
#include "EditorCatalogLayout.hpp"

#include "EditorToolWheelPreferences.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/platform/SdlWindow.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr cr::CreativeWheelProfile kCatalogWheelProfile{
    cr::CreativeWheelPolarity::Reversed,
    cr::CreativeWheelStepMode::RoundedMagnitude,
    1.0e-4F};

[[nodiscard]] bool contains(CatalogRect rect,
                            std::int32_t x,
                            std::int32_t y) noexcept {
  return x >= rect.x && y >= rect.y &&
         x < rect.x + static_cast<std::int32_t>(rect.width) &&
         y < rect.y + static_cast<std::int32_t>(rect.height);
}

struct CatalogPointer {
  std::int32_t x = 0;
  std::int32_t y = 0;
  bool pressed = false;
};

[[nodiscard]] CatalogPointer catalogPointer(
    const iggy3d::SdlWindowEventState& events,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight) noexcept {
  const cr::CreativeDrawablePointer pointer =
      cr::resolveCreativeDrawablePointer(
          {events.pointerX, events.pointerY, events.windowWidth,
           events.windowHeight, drawableWidth, drawableHeight,
           events.pointerMoved, events.primaryPointerPressed});
  return {static_cast<std::int32_t>(pointer.x),
          static_cast<std::int32_t>(pointer.y),
          pointer.valid && pointer.primaryPressed};
}

[[nodiscard]] std::optional<cr::CreativeCatalogPage> catalogPageAtPointer(
    const CatalogLayout& layout,
    const CatalogPointer& pointer) noexcept {
  if (pointer.y < layout.tabsY ||
      pointer.y >= layout.tabsY + static_cast<std::int32_t>(layout.tabHeight) ||
      pointer.x < layout.tabsX ||
      pointer.x >= layout.tabsX +
                       static_cast<std::int32_t>(layout.tabWidth * 2U)) {
    return std::nullopt;
  }
  const std::int32_t relativeX = pointer.x - layout.tabsX;
  const std::size_t pageIndex = static_cast<std::size_t>(
      relativeX / static_cast<std::int32_t>(layout.tabWidth));
  return static_cast<cr::CreativeCatalogPage>(pageIndex);
}

[[nodiscard]] std::optional<std::size_t> hotbarSlotForAction(
    cr::CreativeInputActionId action) noexcept {
  static_assert(
      static_cast<std::size_t>(cr::CreativeInputActionId::HotbarSlot9) -
              static_cast<std::size_t>(
                  cr::CreativeInputActionId::HotbarSlot1) +
          1U ==
      cr::kCreativeHotbarSlotCount);
  if (action < cr::CreativeInputActionId::HotbarSlot1 ||
      action > cr::CreativeInputActionId::HotbarSlot9) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(action) -
         static_cast<std::size_t>(cr::CreativeInputActionId::HotbarSlot1);
}

void ensureSelectionVisible(CreativeEditorCatalogState& catalog,
                            std::size_t visibleRows) noexcept {
  const std::size_t resultCount = catalog.model.filteredEntryIndices.size();
  if (resultCount == 0U) {
    catalog.scrollOffset = 0U;
    return;
  }
  if (catalog.model.selectedFilteredIndex < catalog.scrollOffset) {
    catalog.scrollOffset = catalog.model.selectedFilteredIndex;
  } else if (catalog.model.selectedFilteredIndex >=
             catalog.scrollOffset + visibleRows) {
    catalog.scrollOffset =
        catalog.model.selectedFilteredIndex - visibleRows + 1U;
  }
  const std::size_t maxOffset =
      resultCount > visibleRows ? resultCount - visibleRows : 0U;
  catalog.scrollOffset = std::min(catalog.scrollOffset, maxOffset);
}

void ensureActionSelectionVisible(CreativeEditorCatalogState& catalog,
                                  std::size_t visibleRows) noexcept {
  const std::size_t actionCount = cr::creativeCatalogActionEntries().size();
  if (actionCount == 0U) {
    catalog.actionScrollOffset = 0U;
    return;
  }
  if (catalog.model.selectedActionIndex < catalog.actionScrollOffset) {
    catalog.actionScrollOffset = catalog.model.selectedActionIndex;
  } else if (catalog.model.selectedActionIndex >=
             catalog.actionScrollOffset + visibleRows) {
    catalog.actionScrollOffset =
        catalog.model.selectedActionIndex - visibleRows + 1U;
  }
  const std::size_t maxOffset =
      actionCount > visibleRows ? actionCount - visibleRows : 0U;
  catalog.actionScrollOffset =
      std::min(catalog.actionScrollOffset, maxOffset);
}

[[nodiscard]] bool assignCatalogSelection(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::optional<std::size_t> slot) {
  const cr::CreativeCatalogEntry* selected =
      cr::selectedCreativeCatalogEntry(editor.catalog.model);
  if (selected == nullptr) {
    return false;
  }
  const std::size_t targetSlot = slot.value_or(
      static_cast<std::size_t>(editor.interaction.hotbar.selectedSlot));
  if (targetSlot >= cr::kCreativeHotbarSlotCount) {
    return false;
  }
  static_cast<void>(storeSelectedCreativeMaterialBrushPreset(
      editor.interaction.materialBrushPresets,
      editor.interaction.hotbar, editor.toolSettings));
  const cr::CreativeHeldItemKind previousKind =
      editor.interaction.hotbar.entries[targetSlot].kind;
  static_cast<void>(cr::assignSelectedCreativeCatalogEntry(
      editor.catalog.model, editor.interaction.hotbar, slot));
  cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  held = cr::resolveCreativeCatalogHotbarEntry(*selected, editor.placeBrush);
  if (previousKind != held.kind) {
    clearCreativeMaterialBrushPresetSlot(
        editor.interaction.materialBrushPresets, targetSlot);
  }
  static_cast<void>(activateSelectedCreativeMaterialBrushPreset(
      editor.interaction.materialBrushPresets,
      editor.interaction.hotbar, editor.toolSettings));
  if (cr::creativeCatalogEntryUsesShapeSelection(*selected)) {
    editor.toolSettings.shapeBrushKind = editor.catalog.shapeSelection.kind;
    editor.toolSettings.shapeBrushAxis = editor.catalog.shapeSelection.axis;
  }
  syncCreativeEditorHeldItem(appState, editor);
  if (cr::creativeCatalogEntryUsesShapeSelection(*selected)) {
    cr::clearCreativeVolumeSelection(editor.volume.selection);
    editor.volume.lastReceipt = {};
  }
  return true;
}

[[nodiscard]] bool assignToolWheelSelection(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor) {
  const cr::CreativeCatalogEntry* selected =
      cr::selectedCreativeToolWheelEntry(editor.catalog.toolWheel,
                                         editor.catalog.model);
  if (selected == nullptr) {
    return false;
  }
  const std::size_t targetSlot =
      std::min<std::size_t>(editor.interaction.hotbar.selectedSlot,
                            cr::kCreativeHotbarSlotCount - 1U);
  static_cast<void>(storeSelectedCreativeMaterialBrushPreset(
      editor.interaction.materialBrushPresets,
      editor.interaction.hotbar, editor.toolSettings));
  const cr::CreativeHeldItemKind previousKind =
      editor.interaction.hotbar.entries[targetSlot].kind;
  static_cast<void>(cr::assignSelectedCreativeToolWheelEntry(
      editor.catalog.toolWheel, editor.catalog.model,
      editor.interaction.hotbar));
  cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  held = cr::resolveCreativeCatalogHotbarEntry(*selected, editor.placeBrush);
  if (previousKind != held.kind) {
    clearCreativeMaterialBrushPresetSlot(
        editor.interaction.materialBrushPresets, targetSlot);
  }
  static_cast<void>(activateSelectedCreativeMaterialBrushPreset(
      editor.interaction.materialBrushPresets,
      editor.interaction.hotbar, editor.toolSettings));
  syncCreativeEditorHeldItem(appState, editor);
  return true;
}

[[nodiscard]] bool beginToolWheelAssignment(
    CreativeEditorCatalogState& state) noexcept {
  const std::optional<std::size_t> catalogIndex =
      cr::selectedCreativeCatalogEntryIndex(state.model);
  if (!catalogIndex.has_value() ||
      state.model.entries[*catalogIndex].category !=
          cr::CreativeCatalogEntryCategory::Tool ||
      state.toolWheel.entryCount == 0U) {
    return false;
  }
  state.toolWheelAssignmentCatalogEntryIndex = *catalogIndex;
  if (const std::optional<std::size_t> existing =
          cr::creativeToolWheelSectorForCatalogEntry(state.toolWheel,
                                                     *catalogIndex);
      existing.has_value()) {
    state.toolWheel.selectedIndex = *existing;
  }
  state.statusLabel.clear();
  static_cast<void>(cr::setCreativeCatalogOpen(state.model, false));
  static_cast<void>(cr::setCreativeToolWheelOpen(state.toolWheel, true));
  return true;
}

void finishToolWheelAssignment(CreativeEditorCatalogState& state,
                               bool reopenCatalog) noexcept {
  state.toolWheelAssignmentCatalogEntryIndex.reset();
  static_cast<void>(cr::setCreativeToolWheelOpen(state.toolWheel, false));
  if (reopenCatalog) {
    static_cast<void>(cr::setCreativeCatalogOpen(state.model, true));
  }
}

[[nodiscard]] bool commitToolWheelAssignment(
    const CreativeEditorCatalogFrameRequest& request,
    CreativeEditorCatalogFrameResult& result) {
  CreativeEditorCatalogState& state = request.editor.catalog;
  if (!state.toolWheelAssignmentCatalogEntryIndex.has_value()) {
    return false;
  }
  const std::size_t sector = state.toolWheel.selectedIndex;
  const bool changed = cr::assignCreativeToolWheelCatalogEntry(
      state.toolWheel, state.model, sector,
      *state.toolWheelAssignmentCatalogEntryIndex);
  result.toolWheelChanged = changed;
  if (changed) {
    const CreativeEditorToolWheelPersistenceReceipt saved =
        saveCreativeEditorToolWheel(state.toolWheel, state.model,
                                    request.toolWheelSettingsPath);
    result.toolWheelSaved =
        saved.status == CreativeEditorToolWheelPersistenceStatus::Saved;
    state.statusLabel = result.toolWheelSaved ? "TOOL WHEEL UPDATED"
                                              : "TOOL WHEEL SAVE FAILED";
  } else {
    state.statusLabel = "TOOL WHEEL UNCHANGED";
  }
  finishToolWheelAssignment(state, true);
  return true;
}

[[nodiscard]] bool actionPresent(
    const cr::CreativeInputRouteResult& routedInput,
    cr::CreativeInputActionId action) noexcept {
  return std::any_of(
      routedInput.actionEvents().begin(), routedInput.actionEvents().end(),
      [action](const cr::CreativeInputActionEvent& event) {
        return event.action == action;
      });
}

void applyInventoryModeActions(
    const CreativeEditorCatalogFrameRequest& request) {
  CreativeEditorCatalogState& state = request.editor.catalog;
  switch (request.routedInput.context) {
    case cr::CreativeInputContext::EditorViewport:
      if (actionPresent(request.routedInput,
                        cr::CreativeInputActionId::ToggleCatalog)) {
        state.toolWheelAssignmentCatalogEntryIndex.reset();
        state.statusLabel.clear();
        state.shapeSelection = cr::normalizeCreativeCatalogShapeSelection(
            request.editor.toolSettings.shapeBrushKind,
            request.editor.toolSettings.shapeBrushAxis);
        static_cast<void>(cr::setCreativeCatalogOpen(state.model, true));
        static_cast<void>(cr::setCreativeToolWheelOpen(state.toolWheel, false));
        return;
      }
      if (actionPresent(request.routedInput,
                        cr::CreativeInputActionId::ToggleToolWheel) &&
          state.toolWheel.entryCount > 0U) {
        state.toolWheelAssignmentCatalogEntryIndex.reset();
        static_cast<void>(cr::selectCreativeToolWheelForHotbarEntry(
            state.toolWheel, state.model,
            cr::selectedCreativeHotbarEntry(request.editor.interaction.hotbar)));
        static_cast<void>(cr::setCreativeCatalogOpen(state.model, false));
        static_cast<void>(cr::setCreativeToolWheelOpen(state.toolWheel, true));
      }
      return;
    case cr::CreativeInputContext::Catalog:
      if (actionPresent(request.routedInput,
                        cr::CreativeInputActionId::ToggleCatalog) ||
          actionPresent(request.routedInput,
                        cr::CreativeInputActionId::CatalogClose)) {
        static_cast<void>(cr::setCreativeCatalogOpen(state.model, false));
      }
      return;
    case cr::CreativeInputContext::ToolWheel:
      if (actionPresent(request.routedInput,
                        cr::CreativeInputActionId::ToggleToolWheel) ||
          actionPresent(request.routedInput,
                        cr::CreativeInputActionId::ToolWheelClose)) {
        if (state.toolWheelAssignmentCatalogEntryIndex.has_value()) {
          state.statusLabel = "TOOL WHEEL ASSIGNMENT CANCELLED";
          finishToolWheelAssignment(state, true);
        } else {
          static_cast<void>(
              cr::setCreativeToolWheelOpen(state.toolWheel, false));
        }
      }
      return;
    case cr::CreativeInputContext::ToolOptions:
    case cr::CreativeInputContext::TransformPreview:
    case cr::CreativeInputContext::TransformControls:
    case cr::CreativeInputContext::Controls:
    case cr::CreativeInputContext::TextEntry:
    case cr::CreativeInputContext::Modal:
    case cr::CreativeInputContext::Capture:
    case cr::CreativeInputContext::Count:
      return;
  }
}

void applyInventoryWindowMode(iggy3d::SdlWindow& window,
                              CreativeEditorState& editor,
                              bool centerToolWheelPointer,
                              bool keepPointerAvailable) {
  const bool inventoryOpen = editor.catalog.model.open ||
                             editor.catalog.toolWheel.open ||
                             keepPointerAvailable;
  static_cast<void>(window.setTextInputActive(
      editor.catalog.model.open &&
      editor.catalog.model.page == cr::CreativeCatalogPage::Build));
  static_cast<void>(window.setRelativeMouseMode(!inventoryOpen));
  if (centerToolWheelPointer) {
    window.centerPointer();
  }
  editor.interaction.target = {};
  editor.volume.cursorValid = false;
}

[[nodiscard]] cr::CreativeCatalogActionAvailability catalogActionAvailability(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor) noexcept {
  cr::CreativeCatalogActionAvailability availability;
  availability.undoAvailable = cr::creativeUndoAvailable(appState.history);
  availability.redoAvailable = cr::creativeRedoAvailable(appState.history);
  const bool objectSelection =
      cr::selectedTargetCount(appState.facade.selectionState()) > 0U;
  const cr::CreativeHeldItemKind held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar).kind;
  const bool terrainRegion = held == cr::CreativeHeldItemKind::TerrainRegion;
  const bool terrainSelection =
      terrainRegion &&
      cr::creativeVolumeSelectionComplete(editor.volume.selection);
  availability.copyAvailable = terrainRegion ? terrainSelection : objectSelection;
  availability.cutAvailable = !terrainRegion && objectSelection;
  availability.duplicateAvailable =
      terrainRegion ? terrainSelection : objectSelection;
  availability.clipboardAvailable =
      terrainRegion ? cr::isValidCreativeTerrainStamp(appState.terrainStamp)
                    : !cr::creativeClipboardEmpty(appState.clipboard);
  return availability;
}

[[nodiscard]] bool catalogActionAvailable(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    cr::CreativeInputActionId action) noexcept {
  return cr::creativeCatalogActionAvailable(
      action, catalogActionAvailability(appState, editor));
}

void requestSelectedCatalogAction(
    const CreativeEditorCatalogFrameRequest& request,
    cr::CreativeInputKey trigger,
    CreativeEditorCatalogFrameResult& result) {
  CreativeEditorCatalogState& catalog = request.editor.catalog;
  if (!request.window.eventState().focused) {
    return;
  }
  const cr::CreativeCatalogActionEntry* selected =
      cr::selectedCreativeCatalogAction(catalog.model);
  if (selected == nullptr ||
      !catalogActionAvailable(request.appState, request.editor,
                              selected->action)) {
    return;
  }
  const cr::CreativeCatalogActionActivation activation =
      cr::activateSelectedCreativeCatalogAction(catalog.model);
  if (!activation.requested) {
    return;
  }
  result.deferredCommandInput.context = cr::CreativeInputContext::EditorViewport;
  result.deferredCommandInput.actions[0] = {activation.action, trigger};
  result.deferredCommandInput.actionCount = 1U;
  static_cast<void>(cr::setCreativeCatalogOpen(catalog.model, false));
}

void processCatalogInput(const CreativeEditorCatalogFrameRequest& request,
                         CreativeEditorCatalogFrameResult& result) {
  CreativeEditorCatalogState& catalog = request.editor.catalog;
  const iggy3d::SdlWindowEventState& events = request.window.eventState();
  if (catalog.model.page == cr::CreativeCatalogPage::Build) {
    static_cast<void>(cr::appendCreativeCatalogQueryText(
        catalog.model, events.textInput));
    static_cast<void>(cr::eraseCreativeCatalogQuery(
        catalog.model, events.backspacePressCount));
  }

  for (const cr::CreativeInputActionEvent& event :
       request.routedInput.actionEvents()) {
    if (!catalog.model.open) {
      break;
    }
    switch (event.action) {
      case cr::CreativeInputActionId::CatalogPreviousPage:
        result.pageChanged =
            cr::moveCreativeCatalogPage(catalog.model, -1) ||
            result.pageChanged;
        break;
      case cr::CreativeInputActionId::CatalogNextPage:
        result.pageChanged =
            cr::moveCreativeCatalogPage(catalog.model, 1) ||
            result.pageChanged;
        break;
      case cr::CreativeInputActionId::CatalogPrevious:
        if (catalog.model.page == cr::CreativeCatalogPage::Build) {
          static_cast<void>(
              cr::moveCreativeCatalogSelection(catalog.model, -1));
        } else {
          static_cast<void>(
              cr::moveCreativeCatalogActionSelection(catalog.model, -1));
        }
        break;
      case cr::CreativeInputActionId::CatalogNext:
        if (catalog.model.page == cr::CreativeCatalogPage::Build) {
          static_cast<void>(
              cr::moveCreativeCatalogSelection(catalog.model, 1));
        } else {
          static_cast<void>(
              cr::moveCreativeCatalogActionSelection(catalog.model, 1));
        }
        break;
      case cr::CreativeInputActionId::CatalogPreviousVariant:
      case cr::CreativeInputActionId::CatalogNextVariant: {
        const cr::CreativeCatalogEntry* selected =
            cr::selectedCreativeCatalogEntry(catalog.model);
        if (selected != nullptr &&
            cr::creativeCatalogEntryUsesShapeSelection(*selected)) {
          const std::int32_t direction =
              event.action == cr::CreativeInputActionId::CatalogPreviousVariant
                  ? -1
                  : 1;
          static_cast<void>(cr::moveCreativeCatalogShapeSelection(
              catalog.shapeSelection, direction));
        }
        break;
      }
      case cr::CreativeInputActionId::CatalogAssignToolWheel:
        if (catalog.model.page == cr::CreativeCatalogPage::Build) {
          static_cast<void>(beginToolWheelAssignment(catalog));
        }
        break;
      case cr::CreativeInputActionId::CatalogConfirm:
        if (catalog.model.page == cr::CreativeCatalogPage::Build) {
          result.assigned =
              assignCatalogSelection(request.appState, request.editor,
                                     std::nullopt) ||
              result.assigned;
          if (result.assigned) {
            static_cast<void>(cr::setCreativeCatalogOpen(catalog.model, false));
          }
        } else {
          requestSelectedCatalogAction(request, event.trigger, result);
        }
        break;
      default:
        if (catalog.model.page == cr::CreativeCatalogPage::Build) {
          const std::optional<std::size_t> slot =
              hotbarSlotForAction(event.action);
          if (!slot.has_value()) {
            break;
          }
          result.assigned =
              assignCatalogSelection(request.appState, request.editor, slot) ||
              result.assigned;
        }
        break;
    }
  }

  if (!catalog.model.open) {
    return;
  }
  const std::int32_t wheelSteps = cr::quantizeCreativeWheelSteps(
      events.mouseWheelY, kCatalogWheelProfile);
  if (wheelSteps != 0) {
    if (catalog.model.page == cr::CreativeCatalogPage::Build) {
      static_cast<void>(
          cr::moveCreativeCatalogSelection(catalog.model, wheelSteps));
    } else {
      static_cast<void>(
          cr::moveCreativeCatalogActionSelection(catalog.model, wheelSteps));
    }
  }

  const CatalogLayout layout =
      catalogLayout(request.drawableWidth, request.drawableHeight);
  if (catalog.model.page == cr::CreativeCatalogPage::Build) {
    ensureSelectionVisible(catalog, layout.visibleRows);
  } else {
    ensureActionSelectionVisible(catalog, layout.visibleRows);
  }
  const CatalogPointer pointer = catalogPointer(
      events, request.drawableWidth, request.drawableHeight);
  if (!pointer.pressed) {
    return;
  }
  if (const std::optional<cr::CreativeCatalogPage> page =
          catalogPageAtPointer(layout, pointer);
      page.has_value()) {
    result.pageChanged = cr::setCreativeCatalogPage(catalog.model, *page) ||
                         result.pageChanged;
    return;
  }
  if (catalog.model.page == cr::CreativeCatalogPage::Build) {
    const cr::CreativeCatalogEntry* selected =
        cr::selectedCreativeCatalogEntry(catalog.model);
    if (selected != nullptr &&
        selected->category == cr::CreativeCatalogEntryCategory::Tool &&
        layout.assignWheelX >= layout.contentX &&
        contains(assignWheelButton(layout), pointer.x, pointer.y)) {
      static_cast<void>(beginToolWheelAssignment(catalog));
      return;
    }
    if (selected != nullptr &&
        contains(equipButton(layout), pointer.x, pointer.y)) {
      result.assigned =
          assignCatalogSelection(request.appState, request.editor,
                                 std::nullopt) ||
          result.assigned;
      if (result.assigned) {
        static_cast<void>(cr::setCreativeCatalogOpen(catalog.model, false));
      }
      return;
    }
    if (layout.showDetails && selected != nullptr &&
        cr::creativeCatalogEntryUsesShapeSelection(*selected)) {
      if (contains(previousShapeButton(layout), pointer.x, pointer.y)) {
        static_cast<void>(cr::moveCreativeCatalogShapeSelection(
            catalog.shapeSelection, -1));
        return;
      }
      if (contains(nextShapeButton(layout), pointer.x, pointer.y)) {
        static_cast<void>(cr::moveCreativeCatalogShapeSelection(
            catalog.shapeSelection, 1));
        return;
      }
    }
  }
  const std::uint32_t pointerRowsWidth =
      catalog.model.page == cr::CreativeCatalogPage::Build
          ? layout.listWidth
          : layout.contentWidth;
  const bool insideRows =
      pointer.x >= layout.contentX &&
      pointer.x < layout.contentX +
                      static_cast<std::int32_t>(pointerRowsWidth) &&
      pointer.y >= layout.rowsY && pointer.y < layout.footerY;
  if (!insideRows) {
    return;
  }
  const std::size_t row = static_cast<std::size_t>(
      (pointer.y - layout.rowsY) /
      static_cast<std::int32_t>(layout.rowHeight));
  if (catalog.model.page == cr::CreativeCatalogPage::Actions) {
    const std::size_t actionIndex = catalog.actionScrollOffset + row;
    if (actionIndex >= cr::creativeCatalogActionEntries().size()) {
      return;
    }
    static_cast<void>(
        cr::selectCreativeCatalogActionIndex(catalog.model, actionIndex));
    requestSelectedCatalogAction(request, cr::CreativeInputKey::Enter, result);
    return;
  }
  const std::size_t filteredIndex = catalog.scrollOffset + row;
  if (filteredIndex >= catalog.model.filteredEntryIndices.size()) {
    return;
  }
  static_cast<void>(cr::selectCreativeCatalogFilteredIndex(
      catalog.model, filteredIndex));
}

void processToolWheelInput(const CreativeEditorCatalogFrameRequest& request,
                           CreativeEditorCatalogFrameResult& result) {
  CreativeEditorCatalogState& state = request.editor.catalog;
  for (const cr::CreativeInputActionEvent& event :
       request.routedInput.actionEvents()) {
    if (!state.toolWheel.open) {
      break;
    }
    switch (event.action) {
      case cr::CreativeInputActionId::ToolWheelPrevious:
        static_cast<void>(
            cr::moveCreativeToolWheelSelection(state.toolWheel, -1));
        break;
      case cr::CreativeInputActionId::ToolWheelNext:
        static_cast<void>(
            cr::moveCreativeToolWheelSelection(state.toolWheel, 1));
        break;
      case cr::CreativeInputActionId::ToolWheelConfirm:
        if (state.toolWheelAssignmentCatalogEntryIndex.has_value()) {
          static_cast<void>(commitToolWheelAssignment(request, result));
          break;
        }
        result.assigned =
            assignToolWheelSelection(request.appState, request.editor) ||
            result.assigned;
        if (result.assigned) {
          static_cast<void>(
              cr::setCreativeToolWheelOpen(state.toolWheel, false));
        }
        break;
      case cr::CreativeInputActionId::ToolWheelOptions: {
        if (state.toolWheelAssignmentCatalogEntryIndex.has_value()) {
          break;
        }
        const cr::CreativeCatalogEntry* selected =
            cr::selectedCreativeToolWheelEntry(state.toolWheel, state.model);
        if (selected == nullptr) {
          break;
        }
        const cr::CreativeToolOptionList options =
            creativeEditorToolOptionsForEntry(selected->hotbarEntry,
                                              request.editor.toolSettings);
        const CreativeEditorToolOptionsCommandList commands =
            creativeEditorToolOptionCommandsForEntry(selected->hotbarEntry);
        if ((options.count == 0U && commands.count == 0U) ||
            options.capacityExceeded) {
          break;
        }
        result.openToolOptionsRequested = true;
        result.toolOptionsEntry = selected->hotbarEntry;
        static_cast<void>(cr::setCreativeToolWheelOpen(state.toolWheel, false));
        return;
      }
      default:
        break;
    }
  }

  if (!state.toolWheel.open) {
    return;
  }
  const iggy3d::SdlWindowEventState& events = request.window.eventState();
  const std::int32_t wheelSteps = cr::quantizeCreativeWheelSteps(
      events.mouseWheelY, kCatalogWheelProfile);
  if (wheelSteps != 0) {
    static_cast<void>(
        cr::moveCreativeToolWheelSelection(state.toolWheel, wheelSteps));
  }

  const bool gamepadDirected =
      std::isfinite(request.toolWheelDirectionX) &&
      std::isfinite(request.toolWheelDirectionY) &&
      std::hypot(request.toolWheelDirectionX,
                 request.toolWheelDirectionY) > 0.35F;
  if (gamepadDirected) {
    static_cast<void>(cr::selectCreativeToolWheelDirection(
        state.toolWheel, request.toolWheelDirectionX,
        request.toolWheelDirectionY));
  } else {
    const cr::CreativeDrawablePointer pointer =
        cr::resolveCreativeDrawablePointer(
            {events.pointerX, events.pointerY, events.windowWidth,
             events.windowHeight, request.drawableWidth,
             request.drawableHeight, events.pointerMoved,
             events.primaryPointerPressed});
    if (pointer.valid && pointer.moved) {
      static_cast<void>(cr::selectCreativeToolWheelDirection(
          state.toolWheel, pointer.centeredX, pointer.centeredY, 24.0F));
    }
  }

  if (events.primaryPointerPressed) {
    if (state.toolWheelAssignmentCatalogEntryIndex.has_value()) {
      static_cast<void>(commitToolWheelAssignment(request, result));
      return;
    }
    result.assigned =
        assignToolWheelSelection(request.appState, request.editor) ||
        result.assigned;
    if (result.assigned) {
      static_cast<void>(
          cr::setCreativeToolWheelOpen(state.toolWheel, false));
    }
  }
}

}  // namespace

bool creativeEditorCatalogActionAvailable(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    cr::CreativeInputActionId action) noexcept {
  return catalogActionAvailable(appState, editor, action);
}

CreativeEditorCatalogFrameResult processCreativeEditorCatalogFrame(
    const CreativeEditorCatalogFrameRequest& request) {
  CreativeEditorCatalogFrameResult result;
  CreativeEditorCatalogState& catalog = request.editor.catalog;
  const bool wasCatalogOpen = catalog.model.open;
  const bool wasToolWheelOpen = catalog.toolWheel.open;
  applyInventoryModeActions(request);
  if (catalog.model.open &&
      request.routedInput.context == cr::CreativeInputContext::Catalog) {
    processCatalogInput(request, result);
  }
  if (catalog.toolWheel.open &&
      request.routedInput.context == cr::CreativeInputContext::ToolWheel) {
    processToolWheelInput(request, result);
  }

  result.openChanged = wasCatalogOpen != catalog.model.open ||
                       wasToolWheelOpen != catalog.toolWheel.open;
  if (wasToolWheelOpen && !catalog.toolWheel.open) {
    request.editor.rightStickLookRearmRequired = true;
  }
  if (result.openChanged || result.pageChanged) {
    applyInventoryWindowMode(request.window, request.editor,
                             !wasToolWheelOpen && catalog.toolWheel.open,
                             result.openToolOptionsRequested);
  }
  const bool routedThroughInventory =
      request.routedInput.context == cr::CreativeInputContext::Catalog ||
      request.routedInput.context == cr::CreativeInputContext::ToolWheel;
  result.blockWorldActions =
      routedThroughInventory || catalog.model.open || catalog.toolWheel.open ||
      result.openChanged;
  if (result.blockWorldActions) {
    request.editor.interaction.target = {};
    request.editor.volume.cursorValid = false;
  }
  return result;
}

}  // namespace iggy3d_creative_app

#include "EditorCatalog.hpp"
#include "EditorCatalogInternal.hpp"
#include "EditorCatalogLayout.hpp"

#include <algorithm>
#include <optional>

#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"
#include "app/platform/SdlWindow.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] bool catalogShowsEntries(
    cr::CreativeCatalogPage page) noexcept {
  return page != cr::CreativeCatalogPage::Actions &&
         page != cr::CreativeCatalogPage::Count;
}

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
                       static_cast<std::int32_t>(
                           layout.tabWidth * static_cast<std::uint32_t>(
                               cr::CreativeCatalogPage::Count))) {
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

void resetAssetAction(CreativeEditorCatalogState& catalog) noexcept {
  catalog.assetAction = CreativeEditorCatalogAssetAction::Equip;
  catalog.assetMaterialVariantIndex = 0U;
  catalog.statusLabel.clear();
}

void moveAssetAction(CreativeEditorCatalogState& catalog,
                     const cr::CreativeCatalogEntry& selected,
                     std::int32_t direction) noexcept {
  if (direction == 0) {
    return;
  }
  if (selected.authoredComposite) {
    catalog.assetAction =
        catalog.assetAction == CreativeEditorCatalogAssetAction::ManageAsset
            ? CreativeEditorCatalogAssetAction::Equip
            : CreativeEditorCatalogAssetAction::ManageAsset;
  } else {
    catalog.assetAction = moveCreativeEditorCatalogAssetAction(
        catalog.assetAction, direction);
  }
  catalog.statusLabel.clear();
}

[[nodiscard]] bool assignCatalogSelection(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::optional<std::size_t> slot) {
  const cr::CreativeCatalogEntry* selected =
      cr::selectedCreativeCatalogEntry(editor.catalog.model);
  if (selected == nullptr || !cr::creativeCatalogEntryAssignable(*selected)) {
    return false;
  }
  const std::size_t targetSlot = slot.value_or(
      static_cast<std::size_t>(editor.interaction.hotbar.selectedSlot));
  if (targetSlot >= cr::kCreativeHotbarSlotCount) {
    return false;
  }
  if (selected->category == cr::CreativeCatalogEntryCategory::Asset &&
      editor.catalog.assetMaterialVariantIndex >
          selected->assetMaterialVariants.size()) {
    return false;
  }
  const cr::CreativeHotbarEntry resolved =
      cr::resolveCreativeCatalogHotbarEntry(
          *selected, editor.placeBrush,
          editor.catalog.assetMaterialVariantIndex);
  static_cast<void>(storeSelectedCreativeMaterialBrushPreset(
      editor.interaction.materialBrushPresets,
      editor.interaction.hotbar, editor.toolSettings));
  const cr::CreativeHeldItemKind previousKind =
      editor.interaction.hotbar.entries[targetSlot].kind;
  editor.interaction.hotbar.entries[targetSlot] = resolved;
  static_cast<void>(
      cr::selectCreativeHotbarSlot(editor.interaction.hotbar, targetSlot));
  if (previousKind != resolved.kind) {
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

void activateSelectedCatalogEntry(
    const CreativeEditorCatalogFrameRequest& request,
    std::optional<std::size_t> slot,
    CreativeEditorCatalogFrameResult& result) {
  CreativeEditorCatalogState& catalog = request.editor.catalog;
  const cr::CreativeCatalogEntry* selected =
      cr::selectedCreativeCatalogEntry(catalog.model);
  if (selected == nullptr) {
    return;
  }
  if (cr::creativeCatalogEntryRequestsAssetReload(*selected)) {
    result.assetReloadRequested = true;
    catalog.statusLabel = "RELOADING ASSETS...";
    return;
  }
  if (selected->category == cr::CreativeCatalogEntryCategory::AssetFailure) {
    catalog.statusLabel = "ASSET ERROR: " + selected->detail;
    return;
  }
  result.assigned =
      assignCatalogSelection(request.appState, request.editor, slot) ||
      result.assigned;
  if (result.assigned) {
    static_cast<void>(cr::setCreativeCatalogOpen(catalog.model, false));
  }
}

void requestSelectedAssetReplacement(
    const CreativeEditorCatalogFrameRequest& request,
    CreativeEditorCatalogFrameResult& result) {
  CreativeEditorCatalogState& catalog = request.editor.catalog;
  const cr::CreativeCatalogEntry* selected =
      cr::selectedCreativeCatalogEntry(catalog.model);
  if (selected == nullptr ||
      selected->category != cr::CreativeCatalogEntryCategory::Asset ||
      selected->authoredComposite) {
    return;
  }
  if (cr::selectedTargetCount(request.appState.facade.selectionState()) == 0U) {
    catalog.statusLabel = "REPLACE REQUIRES AN OBJECT SELECTION";
    return;
  }
  const std::string_view assetId =
      cr::creativeHotbarAssetId(selected->hotbarEntry);
  if (assetId.empty()) {
    catalog.statusLabel = "REPLACE TARGET IS INVALID";
    return;
  }
  result.assetReplacementRequested = true;
  result.replacementObjectKind = selected->hotbarEntry.objectKind;
  result.replacementAssetId = assetId;
  catalog.statusLabel.clear();
}

void requestSelectedAuthoredAssetLibrary(
    const CreativeEditorCatalogFrameRequest& request,
    CreativeEditorCatalogFrameResult& result) {
  const cr::CreativeCatalogEntry* selected =
      cr::selectedCreativeCatalogEntry(request.editor.catalog.model);
  if (selected == nullptr ||
      selected->category != cr::CreativeCatalogEntryCategory::Asset ||
      !selected->authoredComposite) {
    return;
  }
  const std::string_view assetId =
      cr::creativeHotbarAssetId(selected->hotbarEntry);
  if (assetId.empty()) {
    request.editor.catalog.statusLabel = "AUTHORED ASSET IS INVALID";
    return;
  }
  result.authoredAssetLibraryRequested = true;
  result.authoredAssetId = assetId;
  request.editor.catalog.statusLabel.clear();
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
        resetAssetAction(state);
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
    case cr::CreativeInputContext::RuntimePlay:
    case cr::CreativeInputContext::AssetReplacementPreview:
    case cr::CreativeInputContext::AssetLibrary:
    case cr::CreativeInputContext::AuthoredAssetEditMenu:
    case cr::CreativeInputContext::TransformPreview:
    case cr::CreativeInputContext::TransformControls:
    case cr::CreativeInputContext::Controls:
    case cr::CreativeInputContext::TextEntry:
    case cr::CreativeInputContext::Modal:
    case cr::CreativeInputContext::Capture:
    case cr::CreativeInputContext::DesktopUi:
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
      catalogShowsEntries(editor.catalog.model.page)));
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
  availability.saveAvailable = !editor.assetEdit.active;
  availability.newAvailable = !editor.assetEdit.active;
  availability.loadAvailable = !editor.assetEdit.active;
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
  if (catalogShowsEntries(catalog.model.page)) {
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
        if (cr::moveCreativeCatalogPage(catalog.model, -1)) {
          result.pageChanged = true;
          resetAssetAction(catalog);
        }
        break;
      case cr::CreativeInputActionId::CatalogNextPage:
        if (cr::moveCreativeCatalogPage(catalog.model, 1)) {
          result.pageChanged = true;
          resetAssetAction(catalog);
        }
        break;
      case cr::CreativeInputActionId::CatalogPrevious:
        if (catalogShowsEntries(catalog.model.page)) {
          static_cast<void>(
              cr::moveCreativeCatalogSelection(catalog.model, -1));
          resetAssetAction(catalog);
        } else {
          static_cast<void>(
              cr::moveCreativeCatalogActionSelection(catalog.model, -1));
        }
        break;
      case cr::CreativeInputActionId::CatalogNext:
        if (catalogShowsEntries(catalog.model.page)) {
          static_cast<void>(
              cr::moveCreativeCatalogSelection(catalog.model, 1));
          resetAssetAction(catalog);
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
            selected->category == cr::CreativeCatalogEntryCategory::Asset) {
          static_cast<void>(moveCreativeEditorCatalogAssetMaterialVariant(
              catalog, *selected,
              event.action == cr::CreativeInputActionId::CatalogPreviousVariant
                  ? -1
                  : 1));
        } else if (selected != nullptr &&
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
      case cr::CreativeInputActionId::CatalogContextAction: {
        const cr::CreativeCatalogEntry* selected =
            cr::selectedCreativeCatalogEntry(catalog.model);
        if (selected != nullptr &&
            selected->category == cr::CreativeCatalogEntryCategory::Tool) {
          static_cast<void>(beginToolWheelAssignment(catalog));
        } else if (selected != nullptr &&
                   selected->category ==
                       cr::CreativeCatalogEntryCategory::Asset) {
          moveAssetAction(catalog, *selected, 1);
        }
        break;
      }
      case cr::CreativeInputActionId::CatalogConfirm:
        if (catalogShowsEntries(catalog.model.page)) {
          const cr::CreativeCatalogEntry* selected =
              cr::selectedCreativeCatalogEntry(catalog.model);
          if (selected != nullptr &&
              selected->category == cr::CreativeCatalogEntryCategory::Asset &&
              !selected->authoredComposite &&
              catalog.assetAction ==
                  CreativeEditorCatalogAssetAction::ReplaceSelection) {
            requestSelectedAssetReplacement(request, result);
          } else if (selected != nullptr &&
                     selected->category ==
                         cr::CreativeCatalogEntryCategory::Asset &&
                     selected->authoredComposite &&
                     catalog.assetAction ==
                         CreativeEditorCatalogAssetAction::ManageAsset) {
            requestSelectedAuthoredAssetLibrary(request, result);
          } else {
            activateSelectedCatalogEntry(request, std::nullopt, result);
          }
        } else {
          requestSelectedCatalogAction(request, event.trigger, result);
        }
        break;
      default:
        if (catalogShowsEntries(catalog.model.page)) {
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
    if (catalogShowsEntries(catalog.model.page)) {
      static_cast<void>(
          cr::moveCreativeCatalogSelection(catalog.model, wheelSteps));
      resetAssetAction(catalog);
    } else {
      static_cast<void>(
          cr::moveCreativeCatalogActionSelection(catalog.model, wheelSteps));
    }
  }

  const CatalogLayout layout =
      catalogLayout(request.drawableWidth, request.drawableHeight);
  if (catalogShowsEntries(catalog.model.page)) {
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
    resetAssetAction(catalog);
    return;
  }
  if (catalogShowsEntries(catalog.model.page)) {
    const cr::CreativeCatalogEntry* selected =
        cr::selectedCreativeCatalogEntry(catalog.model);
    if (layout.showDetails && selected != nullptr &&
        selected->category == cr::CreativeCatalogEntryCategory::Asset) {
      if (contains(previousAssetMaterialVariantButton(layout), pointer.x,
                   pointer.y)) {
        static_cast<void>(moveCreativeEditorCatalogAssetMaterialVariant(
            catalog, *selected, -1));
        return;
      }
      if (contains(nextAssetMaterialVariantButton(layout), pointer.x,
                   pointer.y)) {
        static_cast<void>(moveCreativeEditorCatalogAssetMaterialVariant(
            catalog, *selected, 1));
        return;
      }
    }
    if (selected != nullptr &&
        selected->category == cr::CreativeCatalogEntryCategory::Tool &&
        layout.assignWheelX >= layout.contentX &&
        contains(assignWheelButton(layout), pointer.x, pointer.y)) {
      static_cast<void>(beginToolWheelAssignment(catalog));
      return;
    }
    if (selected != nullptr &&
        selected->category == cr::CreativeCatalogEntryCategory::Asset &&
        layout.replaceX >= layout.contentX &&
        contains(replaceSelectionButton(layout), pointer.x, pointer.y)) {
      if (selected->authoredComposite) {
        requestSelectedAuthoredAssetLibrary(request, result);
      } else {
        requestSelectedAssetReplacement(request, result);
      }
      return;
    }
    if (selected != nullptr &&
        contains(equipButton(layout), pointer.x, pointer.y)) {
      activateSelectedCatalogEntry(request, std::nullopt, result);
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
      catalogShowsEntries(catalog.model.page)
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
  resetAssetAction(catalog);
}


}  // namespace

CreativeEditorCatalogAssetAction moveCreativeEditorCatalogAssetAction(
    CreativeEditorCatalogAssetAction action,
    std::int32_t direction) noexcept {
  if (direction == 0) {
    return action;
  }
  switch (action) {
    case CreativeEditorCatalogAssetAction::Equip:
      return CreativeEditorCatalogAssetAction::ReplaceSelection;
    case CreativeEditorCatalogAssetAction::ReplaceSelection:
    case CreativeEditorCatalogAssetAction::ManageAsset:
    case CreativeEditorCatalogAssetAction::Count:
      return CreativeEditorCatalogAssetAction::Equip;
  }
  return CreativeEditorCatalogAssetAction::Equip;
}

bool moveCreativeEditorCatalogAssetMaterialVariant(
    CreativeEditorCatalogState& state,
    const cr::CreativeCatalogEntry& entry,
    std::int32_t direction) noexcept {
  if (direction == 0 ||
      entry.category != cr::CreativeCatalogEntryCategory::Asset) {
    return false;
  }
  const std::size_t optionCount = entry.assetMaterialVariants.size() + 1U;
  const cr::CreativeWrappedIndexResult next = cr::stepCreativeWrappedIndex(
      state.assetMaterialVariantIndex, optionCount, direction);
  if (!next.valid) {
    return false;
  }
  state.assetMaterialVariantIndex = next.index;
  state.statusLabel.clear();
  return next.changed;
}

std::string_view creativeEditorCatalogAssetMaterialVariantLabel(
    const CreativeEditorCatalogState& state,
    const cr::CreativeCatalogEntry& entry) noexcept {
  if (entry.category != cr::CreativeCatalogEntryCategory::Asset ||
      state.assetMaterialVariantIndex == 0U) {
    return "DEFAULT";
  }
  const std::size_t variantIndex = state.assetMaterialVariantIndex - 1U;
  return variantIndex < entry.assetMaterialVariants.size()
             ? std::string_view{entry.assetMaterialVariants[variantIndex].name}
             : std::string_view{"DEFAULT"};
}

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

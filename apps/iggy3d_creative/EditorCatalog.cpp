#include "EditorCatalog.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>

#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/platform/SdlWindow.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

struct CatalogLayout {
  std::int32_t panelX = 0;
  std::int32_t panelY = 0;
  std::uint32_t panelWidth = 0;
  std::uint32_t panelHeight = 0;
  std::int32_t searchY = 0;
  std::int32_t rowsY = 0;
  std::int32_t footerY = 0;
  std::int32_t tabsX = 0;
  std::int32_t tabsY = 0;
  std::uint32_t tabWidth = 92;
  std::uint32_t tabHeight = 28;
  std::uint32_t rowHeight = 30;
  std::size_t visibleRows = 1;
  std::int32_t contentX = 0;
  std::uint32_t contentWidth = 0;
  std::uint32_t listWidth = 0;
  std::int32_t detailX = 0;
  std::uint32_t detailWidth = 0;
  bool showDetails = false;
  std::int32_t equipX = 0;
  std::int32_t equipY = 0;
  std::uint32_t equipWidth = 88;
  std::uint32_t equipHeight = 28;
};

[[nodiscard]] CatalogLayout catalogLayout(std::uint32_t drawableWidth,
                                          std::uint32_t drawableHeight) {
  CatalogLayout layout;
  const std::int32_t width = static_cast<std::int32_t>(drawableWidth);
  const std::int32_t height = static_cast<std::int32_t>(drawableHeight);
  layout.panelWidth = static_cast<std::uint32_t>(
      std::max(1, std::min(760, width - 16)));
  layout.panelHeight = static_cast<std::uint32_t>(
      std::max(120, std::min(560, height - 96)));
  layout.panelX = std::max(8, (width - static_cast<std::int32_t>(
                                         layout.panelWidth)) /
                                  2);
  layout.panelY = std::max(8, (height - static_cast<std::int32_t>(
                                          layout.panelHeight) -
                              48) /
                                  2);
  layout.searchY = layout.panelY + 48;
  layout.rowsY = layout.searchY + 48;
  layout.footerY = layout.panelY +
                   static_cast<std::int32_t>(layout.panelHeight) - 38;
  const std::uint32_t tabAreaWidth =
      layout.panelWidth > 16U ? layout.panelWidth - 16U : layout.panelWidth;
  layout.tabWidth = std::min(92U, std::max(1U, tabAreaWidth / 2U));
  const std::int32_t tabsWidth =
      static_cast<std::int32_t>(layout.tabWidth * 2U);
  layout.tabsX =
      layout.panelX +
      std::max(0, static_cast<std::int32_t>(layout.panelWidth) - tabsWidth - 8);
  layout.tabsY = layout.panelY + 9;
  const std::int32_t rowsHeight = std::max(0, layout.footerY - layout.rowsY);
  layout.visibleRows = static_cast<std::size_t>(
      std::max(1, rowsHeight / static_cast<std::int32_t>(layout.rowHeight)));
  layout.contentX = layout.panelX + 16;
  layout.contentWidth =
      layout.panelWidth > 32U ? layout.panelWidth - 32U : 1U;
  layout.showDetails = layout.panelWidth >= 600U;
  layout.listWidth = layout.showDetails
                         ? std::max(280U, layout.contentWidth * 58U / 100U)
                         : layout.contentWidth;
  const std::uint32_t detailGap = layout.showDetails ? 16U : 0U;
  layout.detailX = layout.contentX +
                   static_cast<std::int32_t>(layout.listWidth + detailGap);
  layout.detailWidth =
      layout.showDetails && layout.contentWidth > layout.listWidth + detailGap
          ? layout.contentWidth - layout.listWidth - detailGap
          : 0U;
  layout.equipX = layout.panelX + static_cast<std::int32_t>(layout.panelWidth) -
                  16 - static_cast<std::int32_t>(layout.equipWidth);
  layout.equipY = layout.footerY + 5;
  return layout;
}

struct CatalogRect {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
};

[[nodiscard]] bool contains(CatalogRect rect,
                            std::int32_t x,
                            std::int32_t y) noexcept {
  return x >= rect.x && y >= rect.y &&
         x < rect.x + static_cast<std::int32_t>(rect.width) &&
         y < rect.y + static_cast<std::int32_t>(rect.height);
}

[[nodiscard]] CatalogRect previousShapeButton(
    const CatalogLayout& layout) noexcept {
  return {layout.detailX + 12, layout.rowsY + 32, 30U, 28U};
}

[[nodiscard]] CatalogRect nextShapeButton(
    const CatalogLayout& layout) noexcept {
  return {layout.detailX + static_cast<std::int32_t>(layout.detailWidth) - 42,
          layout.rowsY + 32, 30U, 28U};
}

[[nodiscard]] CatalogRect equipButton(const CatalogLayout& layout) noexcept {
  return {layout.equipX, layout.equipY, layout.equipWidth,
          layout.equipHeight};
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
  const float scaleX = events.windowWidth > 0U
                           ? static_cast<float>(drawableWidth) /
                                 static_cast<float>(events.windowWidth)
                           : 1.0F;
  const float scaleY = events.windowHeight > 0U
                           ? static_cast<float>(drawableHeight) /
                                 static_cast<float>(events.windowHeight)
                           : 1.0F;
  return {static_cast<std::int32_t>(events.pointerX * scaleX),
          static_cast<std::int32_t>(events.pointerY * scaleY),
          events.primaryPointerPressed};
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

[[nodiscard]] std::uint32_t catalogInnerWidth(
    const CatalogLayout& layout) noexcept {
  return layout.contentWidth;
}

[[nodiscard]] std::string fitCatalogText(std::string_view text,
                                         std::uint32_t widthPixels) {
  const std::size_t maxCharacters =
      std::max<std::size_t>(4U, widthPixels / 8U);
  if (text.size() <= maxCharacters) {
    return std::string(text);
  }
  std::string output(text.substr(0, maxCharacters - 3U));
  output.append("...");
  return output;
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
  static_cast<void>(cr::assignSelectedCreativeCatalogEntry(
      editor.catalog.model, editor.interaction.hotbar, slot));
  cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  held = cr::resolveCreativeCatalogHotbarEntry(*selected, editor.placeBrush);
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
  static_cast<void>(cr::assignSelectedCreativeToolWheelEntry(
      editor.catalog.toolWheel, editor.catalog.model,
      editor.interaction.hotbar));
  cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  held = cr::resolveCreativeCatalogHotbarEntry(*selected, editor.placeBrush);
  syncCreativeEditorHeldItem(appState, editor);
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
        static_cast<void>(cr::setCreativeToolWheelOpen(state.toolWheel, false));
      }
      return;
    case cr::CreativeInputContext::ToolOptions:
    case cr::CreativeInputContext::ClipboardPreview:
    case cr::CreativeInputContext::TextEntry:
    case cr::CreativeInputContext::Modal:
    case cr::CreativeInputContext::Capture:
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

[[nodiscard]] std::int32_t wheelSelectionSteps(float wheelY) noexcept {
  if (!std::isfinite(wheelY) || std::fabs(wheelY) <= 1.0e-4F) {
    return 0;
  }
  const float magnitude = std::max(1.0F, std::round(std::fabs(wheelY)));
  return wheelY > 0.0F ? -static_cast<std::int32_t>(magnitude)
                       : static_cast<std::int32_t>(magnitude);
}

[[nodiscard]] cr::CreativeCatalogActionAvailability catalogActionAvailability(
    const cr::CreativeAppState& appState) noexcept {
  cr::CreativeCatalogActionAvailability availability;
  availability.undoAvailable = cr::creativeUndoAvailable(appState.history);
  availability.redoAvailable = cr::creativeRedoAvailable(appState.history);
  availability.selectionAvailable =
      cr::selectedTargetCount(appState.facade.selectionState()) > 0U;
  availability.clipboardAvailable =
      !cr::creativeClipboardEmpty(appState.clipboard);
  return availability;
}

[[nodiscard]] bool catalogActionAvailable(
    const cr::CreativeAppState& appState,
    cr::CreativeInputActionId action) noexcept {
  return cr::creativeCatalogActionAvailable(
      action, catalogActionAvailability(appState));
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
      !catalogActionAvailable(request.appState, selected->action)) {
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
  const std::int32_t wheelSteps = wheelSelectionSteps(events.mouseWheelY);
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
  if (cr::creativeWorldActionPressed(
          request.worldActions, cr::CreativeWorldActionId::Secondary)) {
    const cr::CreativeCatalogEntry* selected =
        cr::selectedCreativeToolWheelEntry(state.toolWheel, state.model);
    if (selected != nullptr) {
      const cr::CreativeToolOptionList options =
          cr::creativeToolOptionsForHeldItem(selected->hotbarEntry.kind);
      if (options.count > 0U && !options.capacityExceeded) {
        result.openToolOptionsRequested = true;
        result.toolOptionsHeldItem = selected->hotbarEntry.kind;
        static_cast<void>(
            cr::setCreativeToolWheelOpen(state.toolWheel, false));
        return;
      }
    }
  }
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
        result.assigned =
            assignToolWheelSelection(request.appState, request.editor) ||
            result.assigned;
        if (result.assigned) {
          static_cast<void>(
              cr::setCreativeToolWheelOpen(state.toolWheel, false));
        }
        break;
      default:
        break;
    }
  }

  if (!state.toolWheel.open) {
    return;
  }
  const iggy3d::SdlWindowEventState& events = request.window.eventState();
  const std::int32_t wheelSteps = wheelSelectionSteps(events.mouseWheelY);
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
  } else if (events.pointerMoved) {
    const float scaleX = events.windowWidth > 0U
                             ? static_cast<float>(request.drawableWidth) /
                                   static_cast<float>(events.windowWidth)
                             : 1.0F;
    const float scaleY = events.windowHeight > 0U
                             ? static_cast<float>(request.drawableHeight) /
                                   static_cast<float>(events.windowHeight)
                             : 1.0F;
    const float x = events.pointerX * scaleX -
                    static_cast<float>(request.drawableWidth) * 0.5F;
    const float y = static_cast<float>(request.drawableHeight) * 0.5F -
                    events.pointerY * scaleY;
    static_cast<void>(cr::selectCreativeToolWheelDirection(
        state.toolWheel, x, y, 24.0F));
  }

  if (events.primaryPointerPressed) {
    result.assigned =
        assignToolWheelSelection(request.appState, request.editor) ||
        result.assigned;
    if (result.assigned) {
      static_cast<void>(
          cr::setCreativeToolWheelOpen(state.toolWheel, false));
    }
  }
}

void appendToolWheelOverlay(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs) {
  const CreativeEditorCatalogState& state = editor.catalog;
  if (state.toolWheel.entryCount == 0U) {
    return;
  }
  constexpr float kTau = 6.28318530717958647692F;
  const std::int32_t width = static_cast<std::int32_t>(drawableWidth);
  const std::int32_t height = static_cast<std::int32_t>(drawableHeight);
  const std::int32_t centerX = width / 2;
  const std::int32_t centerY = height / 2;
  const std::uint32_t availableTileWidth =
      drawableWidth > 16U ? drawableWidth - 16U : drawableWidth;
  const std::uint32_t tileWidth = std::min(132U, availableTileWidth);
  const std::uint32_t tileHeight = std::min(38U, drawableHeight);
  const std::int32_t radiusX =
      std::min(280, std::max(0, (width -
                                static_cast<std::int32_t>(tileWidth) - 32) /
                                   2));
  const std::int32_t radiusY =
      std::min(190, std::max(0, (height -
                                static_cast<std::int32_t>(tileHeight) - 100) /
                                   2));

  uiRects.push_back(
      {0, 0, drawableWidth, drawableHeight, 0.01F, 0.015F, 0.02F, 0.66F});
  for (std::size_t index = 0; index < state.toolWheel.entryCount; ++index) {
    const std::size_t catalogIndex =
        state.toolWheel.catalogEntryIndices[index];
    if (catalogIndex >= state.model.entries.size()) {
      continue;
    }
    const cr::CreativeCatalogEntry& entry = state.model.entries[catalogIndex];
    const float angle =
        kTau * static_cast<float>(index) /
        static_cast<float>(state.toolWheel.entryCount);
    const std::int32_t tileX = std::clamp(
        centerX + static_cast<std::int32_t>(
                      std::round(std::sin(angle) *
                                 static_cast<float>(radiusX))) -
            static_cast<std::int32_t>(tileWidth / 2U),
        0, std::max(0, width - static_cast<std::int32_t>(tileWidth)));
    const std::int32_t tileY = std::clamp(
        centerY - static_cast<std::int32_t>(
                      std::round(std::cos(angle) *
                                 static_cast<float>(radiusY))) -
            static_cast<std::int32_t>(tileHeight / 2U),
        0, std::max(0, height - static_cast<std::int32_t>(tileHeight)));
    const bool selected = index == state.toolWheel.selectedIndex;
    uiRects.push_back({tileX, tileY, tileWidth, tileHeight,
                       selected ? 0.86F : 0.07F,
                       selected ? 0.76F : 0.08F,
                       selected ? 0.28F : 0.09F,
                       selected ? 0.98F : 0.94F});
    appendText(glyphs, entry.label, tileX + 8, tileY + 11,
               drawableWidth, drawableHeight,
               selected ? 0.06F : 0.88F,
               selected ? 0.065F : 0.91F,
               selected ? 0.07F : 0.94F);
  }

  const cr::CreativeCatalogEntry* selected =
      cr::selectedCreativeToolWheelEntry(state.toolWheel, state.model);
  const std::uint32_t centerWidth =
      std::min(196U, availableTileWidth);
  const std::uint32_t centerHeight = std::min(62U, drawableHeight);
  const std::int32_t centerPanelX =
      std::max(0, centerX - static_cast<std::int32_t>(centerWidth / 2U));
  const std::int32_t centerPanelY =
      std::max(0, centerY - static_cast<std::int32_t>(centerHeight / 2U));
  uiRects.push_back({centerPanelX, centerPanelY, centerWidth, centerHeight,
                     0.045F, 0.052F, 0.058F, 0.98F});
  appendText(glyphs, selected != nullptr ? selected->label : "TOOL WHEEL",
             centerPanelX + 12, centerPanelY + 13,
             drawableWidth, drawableHeight, 0.92F, 0.94F, 0.96F);
  char slotLabel[32];
  std::snprintf(slotLabel, sizeof(slotLabel), "SLOT %u",
                static_cast<unsigned>(editor.interaction.hotbar.selectedSlot) +
                    1U);
  appendText(glyphs, slotLabel, centerPanelX + 12, centerPanelY + 36,
             drawableWidth, drawableHeight, 0.65F, 0.71F, 0.75F);
}

void appendCatalogBuildDetails(
    const CreativeEditorState& editor,
    const CatalogLayout& layout,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs) {
  if (!layout.showDetails) {
    return;
  }
  const cr::CreativeCatalogEntry* entry =
      cr::selectedCreativeCatalogEntry(editor.catalog.model);
  if (entry == nullptr) {
    return;
  }

  uiRects.push_back({layout.detailX - 8, layout.searchY, 1U,
                     static_cast<std::uint32_t>(
                         std::max(1, layout.footerY - layout.searchY)),
                     0.24F, 0.28F, 0.31F, 0.9F});
  const bool isMaterial =
      entry->category == cr::CreativeCatalogEntryCategory::Material;
  appendText(glyphs, isMaterial ? "MATERIAL" : "TOOL",
             layout.detailX + 4, layout.searchY + 8, drawableWidth,
             drawableHeight, 0.58F, 0.66F, 0.71F);
  appendText(glyphs,
             fitCatalogText(entry->label, layout.detailWidth - 8U),
             layout.detailX + 4, layout.searchY + 29, drawableWidth,
             drawableHeight, 0.94F, 0.96F, 0.98F);

  const bool shapeTool = cr::creativeCatalogEntryUsesShapeSelection(*entry);
  std::int32_t detailY = layout.rowsY + 12;
  if (shapeTool) {
    appendText(glyphs, "SHAPE", layout.detailX + 4, detailY, drawableWidth,
               drawableHeight, 0.58F, 0.66F, 0.71F);
    const CatalogRect previous = previousShapeButton(layout);
    const CatalogRect next = nextShapeButton(layout);
    uiRects.push_back({previous.x, previous.y, previous.width, previous.height,
                       0.12F, 0.14F, 0.16F, 1.0F});
    uiRects.push_back({next.x, next.y, next.width, next.height,
                       0.12F, 0.14F, 0.16F, 1.0F});
    appendText(glyphs, "<", previous.x + 11, previous.y + 7, drawableWidth,
               drawableHeight, 0.90F, 0.93F, 0.95F);
    appendText(glyphs, ">", next.x + 11, next.y + 7, drawableWidth,
               drawableHeight, 0.90F, 0.93F, 0.95F);
    const std::uint32_t shapeLabelWidth =
        layout.detailWidth > 100U ? layout.detailWidth - 100U : 1U;
    appendText(
        glyphs,
        fitCatalogText(cr::creativeCatalogShapeSelectionLabel(
                           editor.catalog.shapeSelection),
                       shapeLabelWidth),
        previous.x + 40, previous.y + 7, drawableWidth, drawableHeight, 0.88F,
        0.92F, 0.94F);

    detailY += 75;
    appendText(glyphs, "AXIS", layout.detailX + 4, detailY, drawableWidth,
               drawableHeight, 0.58F, 0.66F, 0.71F);
    const std::string axisLabel =
        editor.catalog.shapeSelection.kind ==
                cr::CreativeShapeBrushKind::Cylinder
            ? std::string(cr::toString(editor.catalog.shapeSelection.axis))
            : std::string{"--"};
    appendText(glyphs, axisLabel, layout.detailX + 4, detailY + 21,
               drawableWidth, drawableHeight, 0.90F, 0.93F, 0.95F);
    detailY += 58;
  }

  const cr::CreativeObjectKind material =
      isMaterial ? entry->hotbarEntry.objectKind
                 : cr::creativeHeldItemUsesMaterial(entry->hotbarEntry.kind)
                       ? editor.placeBrush
                       : cr::CreativeObjectKind::Unknown;
  if (material != cr::CreativeObjectKind::Unknown) {
    appendText(glyphs, "MATERIAL", layout.detailX + 4, detailY, drawableWidth,
               drawableHeight, 0.58F, 0.66F, 0.71F);
    appendText(glyphs,
               fitCatalogText(cr::toString(material), layout.detailWidth - 8U),
               layout.detailX + 4, detailY + 21, drawableWidth, drawableHeight,
               0.90F, 0.93F, 0.95F);
  }
}

}  // namespace

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

void appendCreativeEditorCatalogOverlay(
    const cr::CreativeAppState& appState,
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs) {
  const CreativeEditorCatalogState& catalog = editor.catalog;
  if (drawableWidth == 0U || drawableHeight == 0U) {
    return;
  }
  if (catalog.toolWheel.open) {
    appendToolWheelOverlay(editor, drawableWidth, drawableHeight, uiRects,
                           glyphs);
    return;
  }
  if (!catalog.model.open) {
    return;
  }
  const CatalogLayout layout = catalogLayout(drawableWidth, drawableHeight);
  uiRects.push_back(
      {0, 0, drawableWidth, drawableHeight, 0.01F, 0.015F, 0.02F, 0.74F});
  uiRects.push_back({layout.panelX, layout.panelY, layout.panelWidth,
                     layout.panelHeight, 0.055F, 0.065F, 0.075F, 0.98F});
  uiRects.push_back({layout.panelX + 16, layout.searchY,
                     catalog.model.page == cr::CreativeCatalogPage::Build
                         ? layout.listWidth
                         : catalogInnerWidth(layout),
                     34U,
                     0.10F, 0.115F, 0.125F, 1.0F});

  if (layout.tabsX - layout.panelX >= 80) {
    appendText(glyphs,
               layout.panelWidth >= 440U ? "CREATIVE CATALOG" : "CATALOG",
               layout.panelX + 18, layout.panelY + 16, drawableWidth,
               drawableHeight, 0.90F, 0.94F, 0.96F);
  }
  constexpr std::array pages{cr::CreativeCatalogPage::Build,
                             cr::CreativeCatalogPage::Actions};
  for (std::size_t index = 0; index < pages.size(); ++index) {
    const bool selected = catalog.model.page == pages[index];
    const std::int32_t x =
        layout.tabsX + static_cast<std::int32_t>(index * layout.tabWidth);
    uiRects.push_back({x, layout.tabsY, layout.tabWidth, layout.tabHeight,
                       selected ? 0.86F : 0.075F,
                       selected ? 0.76F : 0.085F,
                       selected ? 0.28F : 0.095F,
                       selected ? 0.98F : 0.92F});
    const bool compactTab = layout.tabWidth < 70U;
    appendText(glyphs,
               index == 0U ? (compactTab ? "BLD" : "BUILD")
                           : (compactTab ? "ACT" : "ACTIONS"),
               x + (compactTab ? 6 : 10),
               layout.tabsY + 7, drawableWidth, drawableHeight,
               selected ? 0.06F : 0.82F, selected ? 0.065F : 0.86F,
               selected ? 0.07F : 0.90F);
  }

  char footer[128];
  if (catalog.model.page == cr::CreativeCatalogPage::Build) {
    const std::string queryText = fitCatalogText(
        "Search: " + catalog.model.query + "_", layout.listWidth - 16U);
    appendText(glyphs, queryText, layout.panelX + 24, layout.searchY + 9,
               drawableWidth, drawableHeight, 0.88F, 0.92F, 0.94F);

    const std::size_t resultCount = catalog.model.filteredEntryIndices.size();
    const std::size_t end =
        std::min(resultCount, catalog.scrollOffset + layout.visibleRows);
    for (std::size_t filteredIndex = catalog.scrollOffset;
         filteredIndex < end; ++filteredIndex) {
      const cr::CreativeCatalogEntry* entry =
          cr::creativeCatalogEntryAtFilteredIndex(catalog.model, filteredIndex);
      if (entry == nullptr) {
        continue;
      }
      const std::size_t row = filteredIndex - catalog.scrollOffset;
      const std::int32_t y =
          layout.rowsY + static_cast<std::int32_t>(row * layout.rowHeight);
      const bool selected =
          filteredIndex == catalog.model.selectedFilteredIndex;
      uiRects.push_back({layout.contentX, y, layout.listWidth,
                         layout.rowHeight - 2U,
                         selected ? 0.86F : 0.075F,
                         selected ? 0.76F : 0.085F,
                         selected ? 0.28F : 0.095F,
                         selected ? 0.98F : 0.92F});
      const std::string rowText =
          std::string(entry->category ==
                              cr::CreativeCatalogEntryCategory::Material
                          ? "MAT  "
                          : "TOOL ") +
          entry->label;
      appendText(glyphs,
                 fitCatalogText(rowText, layout.listWidth - 16U),
                 layout.panelX + 24, y + 8, drawableWidth,
                 drawableHeight, selected ? 0.06F : 0.88F,
                 selected ? 0.065F : 0.91F,
                 selected ? 0.07F : 0.94F);
    }

    if (resultCount == 0U) {
      appendText(glyphs, "No matching materials or tools", layout.panelX + 24,
                 layout.rowsY + 8, drawableWidth, drawableHeight, 0.82F, 0.54F,
                 0.48F);
    }
    appendCatalogBuildDetails(editor, layout, drawableWidth, drawableHeight,
                              uiRects, glyphs);
    const cr::CreativeCatalogEntry* selectedEntry =
        cr::selectedCreativeCatalogEntry(catalog.model);
    if (selectedEntry != nullptr) {
      const CatalogRect button = equipButton(layout);
      uiRects.push_back({button.x, button.y, button.width, button.height,
                         0.86F, 0.76F, 0.28F, 0.98F});
      appendText(glyphs, "EQUIP", button.x + 20, button.y + 7, drawableWidth,
                 drawableHeight, 0.06F, 0.065F, 0.07F);
    }
    std::snprintf(footer, sizeof(footer), "%zu results | slot %u",
                  resultCount,
                  static_cast<unsigned>(
                      editor.interaction.hotbar.selectedSlot) +
                      1U);
  } else {
    const std::span<const cr::CreativeCatalogActionEntry> actions =
        cr::creativeCatalogActionEntries();
    const cr::CreativeCatalogActionEntry* selectedAction =
        cr::selectedCreativeCatalogAction(catalog.model);
    const bool confirming =
        selectedAction != nullptr &&
        catalog.model.pendingActionConfirmation == selectedAction->action;
    const std::string statusText =
        confirming ? "CONFIRM  " + std::string(selectedAction->label)
                   : "EDITOR ACTIONS";
    appendText(glyphs, statusText, layout.panelX + 24, layout.searchY + 9,
               drawableWidth, drawableHeight, confirming ? 0.96F : 0.88F,
               confirming ? 0.42F : 0.92F,
               confirming ? 0.32F : 0.94F);

    const std::size_t end = std::min(
        actions.size(), catalog.actionScrollOffset + layout.visibleRows);
    for (std::size_t actionIndex = catalog.actionScrollOffset;
         actionIndex < end; ++actionIndex) {
      const cr::CreativeCatalogActionEntry& action = actions[actionIndex];
      const std::size_t row = actionIndex - catalog.actionScrollOffset;
      const std::int32_t y =
          layout.rowsY + static_cast<std::int32_t>(row * layout.rowHeight);
      const bool selected = actionIndex == catalog.model.selectedActionIndex;
      const bool available = catalogActionAvailable(appState, action.action);
      const bool pending =
          catalog.model.pendingActionConfirmation == action.action;
      uiRects.push_back({layout.panelX + 16, y, catalogInnerWidth(layout),
                         layout.rowHeight - 2U,
                         selected && available ? 0.86F : 0.075F,
                         selected && available ? 0.76F : 0.085F,
                         selected && available ? 0.28F : 0.095F,
                         available ? 0.98F : 0.64F});
      std::string rowText(pending ? "CONFIRM  " : "ACTION   ");
      rowText.append(action.label);
      appendText(glyphs, rowText, layout.panelX + 24, y + 8, drawableWidth,
                 drawableHeight,
                 available ? (selected ? 0.06F : 0.88F) : 0.42F,
                 available ? (selected ? 0.065F : 0.91F) : 0.45F,
                 available ? (selected ? 0.07F : 0.94F) : 0.48F);
    }
    std::snprintf(
        footer, sizeof(footer), "%zu actions | undo %llu | redo %llu",
        actions.size(),
        static_cast<unsigned long long>(cr::creativeUndoDepth(appState.history)),
        static_cast<unsigned long long>(cr::creativeRedoDepth(appState.history)));
  }
  appendText(glyphs,
             fitCatalogText(footer, layout.panelWidth > 152U
                                         ? layout.panelWidth - 152U
                                         : layout.panelWidth),
             layout.panelX + 20, layout.footerY + 10,
             drawableWidth, drawableHeight, 0.68F, 0.74F, 0.78F);
}

}  // namespace iggy3d_creative_app

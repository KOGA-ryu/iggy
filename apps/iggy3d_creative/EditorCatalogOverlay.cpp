#include "EditorCatalog.hpp"
#include "EditorCatalogInternal.hpp"
#include "EditorCatalogLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] const cr::CreativeCatalogEntry* toolWheelAssignmentEntry(
    const CreativeEditorCatalogState& state) noexcept {
  if (!state.toolWheelAssignmentCatalogEntryIndex.has_value() ||
      *state.toolWheelAssignmentCatalogEntryIndex >=
          state.model.entries.size()) {
    return nullptr;
  }
  const cr::CreativeCatalogEntry& entry =
      state.model.entries[*state.toolWheelAssignmentCatalogEntryIndex];
  return entry.category == cr::CreativeCatalogEntryCategory::Tool ? &entry
                                                                  : nullptr;
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

[[nodiscard]] std::string_view catalogTabLabel(
    cr::CreativeCatalogPage page,
    bool compact) noexcept {
  switch (page) {
    case cr::CreativeCatalogPage::Structure:
      return compact ? "BLD" : "STRUCT";
    case cr::CreativeCatalogPage::Terrain:
      return compact ? "TER" : "TERR";
    case cr::CreativeCatalogPage::Movement:
      return compact ? "MOV" : "MOVE";
    case cr::CreativeCatalogPage::Logic:
      return compact ? "LOG" : "LOGIC";
    case cr::CreativeCatalogPage::Dressing:
      return compact ? "DEC" : "DRESS";
    case cr::CreativeCatalogPage::Media:
      return compact ? "MED" : "MEDIA";
    case cr::CreativeCatalogPage::Gameplay:
      return compact ? "GME" : "GAME";
    case cr::CreativeCatalogPage::Testing:
      return compact ? "TST" : "TEST";
    case cr::CreativeCatalogPage::Helpers:
      return compact ? "HLP" : "HELP";
    case cr::CreativeCatalogPage::Tools:
      return compact ? "TLS" : "TOOLS";
    case cr::CreativeCatalogPage::Experimental:
      return compact ? "LAB" : "EXPER";
    case cr::CreativeCatalogPage::Assets:
      return compact ? "AST" : "ASSET";
    case cr::CreativeCatalogPage::Actions:
      return compact ? "ACT" : "ACTION";
    case cr::CreativeCatalogPage::Count:
      return "";
  }
  return "";
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
  const cr::CreativeCatalogEntry* assignment =
      toolWheelAssignmentEntry(state);
  const bool assigning = assignment != nullptr;

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
    const float selectedR = assigning ? 0.20F : 0.86F;
    const float selectedG = assigning ? 0.72F : 0.76F;
    const float selectedB = assigning ? 0.42F : 0.28F;
    uiRects.push_back({tileX, tileY, tileWidth, tileHeight,
                       selected ? selectedR : 0.07F,
                       selected ? selectedG : 0.08F,
                       selected ? selectedB : 0.09F,
                       selected ? 0.98F : 0.94F});
    char sectorLabel[160];
    std::snprintf(sectorLabel, sizeof(sectorLabel), "%zu  %s", index + 1U,
                  entry.label.c_str());
    appendText(glyphs, sectorLabel, tileX + 8, tileY + 11,
               drawableWidth, drawableHeight,
               selected ? 0.06F : 0.88F,
               selected ? 0.065F : 0.91F,
               selected ? 0.07F : 0.94F);
  }

  const cr::CreativeCatalogEntry* selected =
      cr::selectedCreativeToolWheelEntry(state.toolWheel, state.model);
  const std::uint32_t centerWidth =
      std::min(assigning ? 300U : 196U, availableTileWidth);
  const std::uint32_t centerHeight =
      std::min(assigning ? 86U : 62U, drawableHeight);
  const std::int32_t centerPanelX =
      std::max(0, centerX - static_cast<std::int32_t>(centerWidth / 2U));
  const std::int32_t centerPanelY =
      std::max(0, centerY - static_cast<std::int32_t>(centerHeight / 2U));
  uiRects.push_back({centerPanelX, centerPanelY, centerWidth, centerHeight,
                     0.045F, 0.052F, 0.058F, 0.98F});
  if (assigning) {
    appendText(glyphs,
               fitCatalogText("ASSIGN " + assignment->label,
                              centerWidth > 24U ? centerWidth - 24U : 1U),
               centerPanelX + 12, centerPanelY + 10, drawableWidth,
               drawableHeight, 0.92F, 0.96F, 0.94F);
    appendText(glyphs,
               fitCatalogText(
                   "REPLACE " +
                       std::string(selected != nullptr ? selected->label
                                                       : "EMPTY"),
                   centerWidth > 24U ? centerWidth - 24U : 1U),
               centerPanelX + 12, centerPanelY + 34, drawableWidth,
               drawableHeight, 0.72F, 0.78F, 0.81F);
    appendText(glyphs, "X ASSIGN  CIRCLE BACK", centerPanelX + 12,
               centerPanelY + 59, drawableWidth, drawableHeight, 0.58F, 0.82F,
               0.66F);
    return;
  }
  appendText(glyphs, selected != nullptr ? selected->label : "TOOL WHEEL",
             centerPanelX + 12, centerPanelY + 13, drawableWidth,
             drawableHeight, 0.92F, 0.94F, 0.96F);
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
  const bool isAsset =
      entry->category == cr::CreativeCatalogEntryCategory::Asset;
  appendText(glyphs, cr::toString(entry->category),
             layout.detailX + 4, layout.searchY + 8, drawableWidth,
             drawableHeight, 0.58F, 0.66F, 0.71F);
  appendText(glyphs,
             fitCatalogText(entry->label, layout.detailWidth - 8U),
             layout.detailX + 4, layout.searchY + 29, drawableWidth,
             drawableHeight, 0.94F, 0.96F, 0.98F);
  if (isAsset) {
    appendCatalogAssetDetails(editor, *entry, layout, drawableWidth,
                              drawableHeight, uiRects, glyphs);
    return;
  }

  const bool shapeTool = cr::creativeCatalogEntryUsesShapeSelection(*entry);
  std::int32_t detailY = layout.rowsY + 12;
  if (entry->category == cr::CreativeCatalogEntryCategory::AssetFailure ||
      entry->category == cr::CreativeCatalogEntryCategory::Command) {
    appendText(glyphs, "DETAIL", layout.detailX + 4, detailY,
               drawableWidth, drawableHeight, 0.58F, 0.66F, 0.71F);
    appendText(glyphs,
               fitCatalogText(entry->detail, layout.detailWidth - 8U),
               layout.detailX + 4, detailY + 21, drawableWidth,
               drawableHeight,
               entry->category == cr::CreativeCatalogEntryCategory::AssetFailure
                   ? 0.94F
                   : 0.90F,
               entry->category == cr::CreativeCatalogEntryCategory::AssetFailure
                   ? 0.42F
                   : 0.93F,
               entry->category == cr::CreativeCatalogEntryCategory::AssetFailure
                   ? 0.32F
                   : 0.95F);
    detailY += 58;
  }
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
      (isMaterial || isAsset) ? entry->hotbarEntry.objectKind
                 : cr::creativeHeldItemUsesMaterial(entry->hotbarEntry.kind)
                       ? editor.placeBrush
                       : cr::CreativeObjectKind::Unknown;
  if (material != cr::CreativeObjectKind::Unknown) {
    appendText(glyphs, isAsset ? "OBJECT KIND" : "MATERIAL",
               layout.detailX + 4, detailY, drawableWidth,
               drawableHeight, 0.58F, 0.66F, 0.71F);
    appendText(glyphs,
               fitCatalogText(cr::toString(material), layout.detailWidth - 8U),
               layout.detailX + 4, detailY + 21, drawableWidth, drawableHeight,
               0.90F, 0.93F, 0.95F);
  }
}

}  // namespace

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
  const bool entryPage =
      catalog.model.page != cr::CreativeCatalogPage::Actions;
  uiRects.push_back({layout.panelX + 16, layout.searchY,
                     entryPage
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
  for (std::size_t index = 0;
       index < static_cast<std::size_t>(cr::CreativeCatalogPage::Count);
       ++index) {
    const cr::CreativeCatalogPage page =
        static_cast<cr::CreativeCatalogPage>(index);
    const bool selected = catalog.model.page == page;
    const std::int32_t x =
        layout.tabsX + static_cast<std::int32_t>(index * layout.tabWidth);
    uiRects.push_back({x, layout.tabsY, layout.tabWidth, layout.tabHeight,
                       selected ? 0.86F : 0.075F,
                       selected ? 0.76F : 0.085F,
                       selected ? 0.28F : 0.095F,
                       selected ? 0.98F : 0.92F});
    const bool compactTab = layout.tabWidth < 50U;
    const std::string_view tabLabel = catalogTabLabel(page, compactTab);
    appendText(glyphs, tabLabel,
               x + (compactTab ? 4 : 7),
               layout.tabsY + 7, drawableWidth, drawableHeight,
               selected ? 0.06F : 0.82F, selected ? 0.065F : 0.86F,
               selected ? 0.07F : 0.90F);
  }

  char footer[128];
  if (entryPage) {
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
      std::string_view prefix = "TOOL ";
      switch (entry->category) {
        case cr::CreativeCatalogEntryCategory::Material: prefix = "MAT  "; break;
        case cr::CreativeCatalogEntryCategory::Asset: prefix = "ASSET "; break;
        case cr::CreativeCatalogEntryCategory::AssetFailure:
          prefix = "ERROR ";
          break;
        case cr::CreativeCatalogEntryCategory::Command: prefix = "ACTION "; break;
        case cr::CreativeCatalogEntryCategory::Tool: break;
      }
      std::string rowText(prefix);
      if (entry->category == cr::CreativeCatalogEntryCategory::Asset) {
        rowText.push_back('[');
        rowText.append(cr::creativeCatalogAssetPhysicsLabel(
            entry->assetAuthoringMetadata));
        rowText.append("] ");
      }
      rowText.append(entry->label);
      appendText(glyphs,
                 fitCatalogText(rowText, layout.listWidth - 16U),
                 layout.panelX + 24, y + 8, drawableWidth,
                 drawableHeight, selected ? 0.06F : 0.88F,
                 selected ? 0.065F : 0.91F,
                 selected ? 0.07F : 0.94F);
    }

    if (resultCount == 0U) {
      appendText(glyphs,
                 catalog.model.page == cr::CreativeCatalogPage::Assets
                     ? "No matching assets"
                     : "No matching entries",
                 layout.panelX + 24,
                 layout.rowsY + 8, drawableWidth, drawableHeight, 0.82F, 0.54F,
                 0.48F);
    }
    appendCatalogBuildDetails(editor, layout, drawableWidth, drawableHeight,
                              uiRects, glyphs);
    const cr::CreativeCatalogEntry* selectedEntry =
        cr::selectedCreativeCatalogEntry(catalog.model);
    if (selectedEntry != nullptr) {
      if (selectedEntry->category == cr::CreativeCatalogEntryCategory::Tool &&
          layout.assignWheelX >= layout.contentX) {
        const CatalogRect wheelButton = assignWheelButton(layout);
        uiRects.push_back({wheelButton.x, wheelButton.y, wheelButton.width,
                           wheelButton.height, 0.14F, 0.44F, 0.28F, 0.98F});
        appendText(glyphs, "ASSIGN WHEEL", wheelButton.x + 14,
                   wheelButton.y + 7, drawableWidth, drawableHeight, 0.90F,
                   0.96F, 0.92F);
      }
      if (cr::creativeCatalogEntryAssignable(*selectedEntry) ||
          cr::creativeCatalogEntryRequestsAssetReload(*selectedEntry)) {
        const CatalogRect button = equipButton(layout);
        const bool assetEntry =
            selectedEntry->category == cr::CreativeCatalogEntryCategory::Asset;
        const bool equipFocused =
            !assetEntry ||
            catalog.assetAction == CreativeEditorCatalogAssetAction::Equip;
        uiRects.push_back({button.x, button.y, button.width, button.height,
                           equipFocused ? 0.86F : 0.12F,
                           equipFocused ? 0.76F : 0.14F,
                           equipFocused ? 0.28F : 0.16F, 0.98F});
        appendText(glyphs,
                   cr::creativeCatalogEntryRequestsAssetReload(*selectedEntry)
                       ? "RELOAD"
                       : "EQUIP",
                   button.x + 20, button.y + 7, drawableWidth,
                   drawableHeight, equipFocused ? 0.06F : 0.82F,
                   equipFocused ? 0.065F : 0.86F,
                   equipFocused ? 0.07F : 0.88F);
        if (assetEntry && layout.replaceX >= layout.contentX) {
          const CatalogRect replace = replaceSelectionButton(layout);
          const bool manage = selectedEntry->authoredComposite;
          const bool replaceFocused =
              catalog.assetAction ==
              (manage ? CreativeEditorCatalogAssetAction::ManageAsset
                       : CreativeEditorCatalogAssetAction::ReplaceSelection);
          const bool replaceAvailable =
              manage ||
              cr::selectedTargetCount(appState.facade.selectionState()) > 0U;
          uiRects.push_back(
              {replace.x, replace.y, replace.width, replace.height,
               replaceFocused && replaceAvailable ? 0.16F : 0.085F,
               replaceFocused && replaceAvailable ? 0.72F : 0.10F,
               replaceFocused && replaceAvailable ? 0.32F : 0.11F,
               replaceAvailable ? 0.98F : 0.68F});
          appendText(glyphs, manage ? "MANAGE ASSET" : "REPLACE SELECTION",
                     replace.x + 14, replace.y + 7, drawableWidth,
                     drawableHeight, replaceAvailable ? 0.88F : 0.42F,
                     replaceAvailable ? 0.96F : 0.45F,
                     replaceAvailable ? 0.90F : 0.48F);
        }
      }
    }
    if (!catalog.statusLabel.empty()) {
      std::snprintf(footer, sizeof(footer), "%s", catalog.statusLabel.c_str());
    } else if (catalog.model.page == cr::CreativeCatalogPage::Assets) {
      const std::size_t assetCount = static_cast<std::size_t>(std::count_if(
          catalog.model.entries.begin(), catalog.model.entries.end(),
          [](const cr::CreativeCatalogEntry& entry) {
            return entry.category == cr::CreativeCatalogEntryCategory::Asset;
          }));
      std::snprintf(footer, sizeof(footer), "%zu assets | %zu rejected | slot %u",
                    assetCount, catalog.model.rejectedAssetCount,
                    static_cast<unsigned>(
                        editor.interaction.hotbar.selectedSlot) +
                        1U);
    } else {
      const std::string_view categoryLabel =
          catalog.model.query.empty()
              ? cr::toString(catalog.model.page)
              : std::string_view{"Search all"};
      std::snprintf(footer, sizeof(footer), "%.*s | %zu results | slot %u",
                    static_cast<int>(categoryLabel.size()),
                    categoryLabel.data(), resultCount,
                    static_cast<unsigned>(
                        editor.interaction.hotbar.selectedSlot) +
                        1U);
    }
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
      const bool available =
          creativeEditorCatalogActionAvailable(appState, editor,
                                                action.action);
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

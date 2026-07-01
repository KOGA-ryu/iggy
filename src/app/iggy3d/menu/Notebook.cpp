#include "app/iggy3d/menu/Notebook.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

#include "app/iggy3d/ui/Widget.hpp"

namespace iggy3d {
namespace {

// Book shell (virtual 1280x720). Two facing pages over a dark cover, spine gutter
// between them, fore-edge tabs on the right.
constexpr float kBookX = 120.0F;
constexpr float kBookY = 48.0F;
constexpr float kBookW = 1040.0F;
constexpr float kBookH = 624.0F;
constexpr float kLeftPageX = 150.0F;
constexpr float kRightPageX = 660.0F;
constexpr float kPageY = 78.0F;
constexpr float kPageW = 470.0F;
constexpr float kPageH = 564.0F;

// Left page sketch area (the blocky floor plan lives here).
constexpr float kSketchX = 180.0F;
constexpr float kSketchY = 178.0F;
constexpr float kSketchW = 410.0F;
constexpr float kSketchH = 424.0F;

// Right page notes column.
constexpr float kNotesX = 728.0F;
constexpr float kRuleX = 690.0F;
constexpr float kRuleW = 420.0F;

std::string notebookId(std::string_view suffix) {
  std::string id = "notebook.";
  id.append(suffix);
  return id;
}

// A right-page notes section: a heading + its entries, laid out down the page.
// Returns the y just below the section.
float emitNotesSection(WidgetOutput& out,
                       std::string_view idStem,
                       std::string_view heading,
                       const std::vector<std::string>& entries,
                       float y) {
  emit(UiText{.rect = {kNotesX, y, kRuleW - 40.0F, 28.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = notebookId(std::string(idStem) + ".heading"),
              .text = std::string(heading)},
       out);
  emit(UiPanel{.rect = {kNotesX, y + 24.0F, 96.0F, 2.0F},
               .kind = ProductUiPrimitiveKind::Border,
               .tone = ProductUiTone::Border,
               .semanticId = notebookId(std::string(idStem) + ".rule")},
       out);
  float rowY = y + 34.0F;
  for (std::size_t i = 0; i < entries.size(); ++i) {
    emit(UiText{.rect = {kNotesX + 8.0F, rowY, kRuleW - 48.0F, 26.0F},
                .tone = ProductUiTone::TextMuted,
                .semanticId = notebookId(std::string(idStem) + ".entry_" +
                                         std::to_string(i)),
                .text = entries[i]},
         out);
    rowY += 30.0F;
  }
  return rowY + 14.0F;
}

// Render the ASCII floor plan as blocky cells in the sketch area. Walls fill a
// cell; door/window/vent/guard become an accented marker cell + its glyph.
void emitFloorPlan(WidgetOutput& out, const std::vector<std::string>& grid) {
  const std::size_t rows = grid.size();
  std::size_t cols = 0;
  for (const std::string& row : grid) {
    cols = std::max(cols, row.size());
  }
  // branch-gate: BG-1073
  if (rows == 0 || cols == 0) {
    return;
  }
  const float cell = std::min(kSketchW / static_cast<float>(cols),
                              kSketchH / static_cast<float>(rows));
  const float originX =
      kSketchX + (kSketchW - cell * static_cast<float>(cols)) * 0.5F;
  const float originY =
      kSketchY + (kSketchH - cell * static_cast<float>(rows)) * 0.5F;
  for (std::size_t r = 0; r < rows; ++r) {
    for (std::size_t c = 0; c < cols; ++c) {
      const char ch = c < grid[r].size() ? grid[r][c] : ' ';
      const float x = originX + static_cast<float>(c) * cell;
      const float y = originY + static_cast<float>(r) * cell;
      const std::string cellSuffix =
          "_" + std::to_string(r) + "_" + std::to_string(c);
      // branch-gate: BG-1073
      if (ch == '#') {
        emit(UiPanel{.rect = {x, y, cell, cell},
                     .kind = ProductUiPrimitiveKind::Rect,
                     .tone = ProductUiTone::TextPrimary,
                     .semanticId = notebookId("plan.wall" + cellSuffix)},
             out);
      } else if (ch == 'D' || ch == 'W' || ch == 'V' || ch == 'G') {
        emit(UiPanel{.rect = {x + cell * 0.12F, y + cell * 0.12F, cell * 0.76F,
                              cell * 0.76F},
                     .kind = ProductUiPrimitiveKind::Rect,
                     .tone = ProductUiTone::Accent,
                     .semanticId = notebookId("plan.entry" + cellSuffix)},
             out);
        emit(UiText{.rect = {x, y + cell * 0.72F, cell, cell * 0.6F},
                    .tone = ProductUiTone::Surface,
                    .semanticId = notebookId("plan.entry" + cellSuffix + ".glyph"),
                    .text = std::string(1, ch)},
             out);
      }
    }
  }
}

constexpr std::array<ProductNotebookTab, 4> kTabOrder{
    ProductNotebookTab::Maps,
    ProductNotebookTab::Bestiary,
    ProductNotebookTab::Jobs,
    ProductNotebookTab::Spells,
};

constexpr std::array<std::string_view, 4> kTabGlyphs{"M", "B", "J", "S"};

void emitTabs(WidgetOutput& out, ProductNotebookTab active) {
  for (std::size_t i = 0; i < kTabOrder.size(); ++i) {
    const ProductNotebookTab tab = kTabOrder[i];
    const bool selected = tab == active;
    const float y = 150.0F + static_cast<float>(i) * 74.0F;
    const std::string idStem =
        std::string("tab.") + std::string(productNotebookTabName(tab));
    emit(UiPanel{.rect = {1116.0F, y, 40.0F, 60.0F},
                 .kind = ProductUiPrimitiveKind::Panel,
                 // branch-gate: BG-1073
                 .tone = selected ? ProductUiTone::Selected
                                  : ProductUiTone::SurfaceRaised,
                 .semanticId = notebookId(idStem),
                 .selected = selected},
         out);
    emit(UiText{.rect = {1130.0F, y + 38.0F, 24.0F, 28.0F},
                // branch-gate: BG-1073
                .tone = selected ? ProductUiTone::TextPrimary
                                 : ProductUiTone::TextMuted,
                .semanticId = notebookId(idStem + ".glyph"),
                .text = std::string(kTabGlyphs[i]),
                .selected = selected},
         out);
  }
}

ProductUiDrawList rejectedNotebook(const ProductNotebookUiRequest& request) {
  ProductUiDrawList list;
  list.ready = false;
  list.status = "product_notebook_ui_not_ready";
  list.reasonCode = "product_notebook_ui_missing_page";
  list.virtualWidth = request.virtualWidth;
  list.virtualHeight = request.virtualHeight;
  return list;
}

}  // namespace

std::string_view productNotebookTabName(ProductNotebookTab tab) {
  // branch-gate: BG-1217
  switch (tab) {
    case ProductNotebookTab::Maps:
      return "maps";
    case ProductNotebookTab::Bestiary:
      return "bestiary";
    case ProductNotebookTab::Jobs:
      return "jobs";
    case ProductNotebookTab::Spells:
      return "spells";
  }
  return "maps";
}

ProductUiDrawList buildProductNotebookUiDrawList(
    const ProductNotebookUiRequest& request) {
  // branch-gate: BG-1073
  if (request.page == nullptr) {
    return rejectedNotebook(request);
  }
  const ProductNotebookReconPage& page = *request.page;

  ProductUiDrawList list;
  list.ready = true;
  list.status = "product_notebook_ui_ready";
  list.reasonCode = "product_notebook_ui_ready";
  list.virtualWidth = request.virtualWidth;
  list.virtualHeight = request.virtualHeight;
  list.selectedAction = std::string(productNotebookTabName(request.tab));

  WidgetOutput out;

  // Book shell: cover + two facing pages + ribbon over the spine.
  emit(UiPanel{.rect = {kBookX, kBookY, kBookW, kBookH},
               .kind = ProductUiPrimitiveKind::Panel,
               .tone = ProductUiTone::Surface,
               .semanticId = notebookId("book")},
       out);
  emit(UiPanel{.rect = {kLeftPageX, kPageY, kPageW, kPageH},
               .kind = ProductUiPrimitiveKind::Panel,
               .tone = ProductUiTone::SurfaceRaised,
               .semanticId = notebookId("page.left")},
       out);
  emit(UiPanel{.rect = {kRightPageX, kPageY, kPageW, kPageH},
               .kind = ProductUiPrimitiveKind::Panel,
               .tone = ProductUiTone::SurfaceRaised,
               .semanticId = notebookId("page.right")},
       out);
  emit(UiPanel{.rect = {634.0F, 60.0F, 10.0F, 236.0F},
               .kind = ProductUiPrimitiveKind::Rect,
               .tone = ProductUiTone::Accent,
               .semanticId = notebookId("ribbon")},
       out);

  // Left page: title + the blocky floor plan.
  emit(UiText{.rect = {kLeftPageX + 22.0F, 120.0F, kPageW - 44.0F, 40.0F},
              .tone = ProductUiTone::TextPrimary,
              .semanticId = notebookId("sketch.title"),
              .text = page.title},
       out);
  emit(UiPanel{.rect = {kLeftPageX + 22.0F, 152.0F, kPageW - 44.0F, 3.0F},
               .kind = ProductUiPrimitiveKind::Border,
               .tone = ProductUiTone::Border,
               .semanticId = notebookId("sketch.underline")},
       out);
  emitFloorPlan(out, page.floorPlan);

  // Right page: ruled lines + margin, then the notes sections.
  for (int i = 0; i < 9; ++i) {
    const float y = 178.0F + static_cast<float>(i) * 44.0F;
    emit(UiPanel{.rect = {kRuleX, y, kRuleW, 2.0F},
                 .kind = ProductUiPrimitiveKind::Border,
                 .tone = ProductUiTone::Border,
                 .semanticId = notebookId("rule_" + std::to_string(i))},
         out);
  }
  emit(UiPanel{.rect = {712.0F, 160.0F, 3.0F, 452.0F},
               .kind = ProductUiPrimitiveKind::Rect,
               .tone = ProductUiTone::Accent,
               .semanticId = notebookId("margin")},
       out);
  float y = 196.0F;
  y = emitNotesSection(out, "garrison", "GARRISON", page.garrison, y);
  y = emitNotesSection(out, "patrol", "PATROL", page.patrols, y);
  y = emitNotesSection(out, "hazards", "HAZARDS", page.hazards, y);

  emitTabs(out, request.tab);

  emit(UiText{.rect = {860.0F, 616.0F, 120.0F, 26.0F},
              .tone = ProductUiTone::TextMuted,
              .semanticId = notebookId("page_number"),
              .text = "- " + std::to_string(page.pageNumber) + " -"},
       out);

  appendWidgetOutput(list, out);
  list.primitiveCount = list.primitives.size();
  return list;
}

}  // namespace iggy3d

#include "app/iggy3d/menu/Notebook.hpp"

#include <cstddef>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

const iggy3d::ProductUiPrimitive* findPrimitive(
    const iggy3d::ProductUiDrawList& list,
    std::string_view semanticId) {
  for (const iggy3d::ProductUiPrimitive& primitive : list.primitives) {
    if (primitive.semanticId == semanticId) {
      return &primitive;
    }
  }
  return nullptr;
}

std::size_t countWithPrefix(const iggy3d::ProductUiDrawList& list,
                            std::string_view prefix) {
  std::size_t count = 0;
  for (const iggy3d::ProductUiPrimitive& primitive : list.primitives) {
    if (primitive.semanticId.rfind(prefix, 0) == 0) {
      ++count;
    }
  }
  return count;
}

iggy3d::ProductNotebookReconPage sampleReconPage() {
  iggy3d::ProductNotebookReconPage page;
  page.title = "WARDEN'S KEEP - GROUND";
  page.floorPlan = {
      "#####",
      "#..D#",
      "#.#.#",
      "W...#",
      "##V##",
  };
  page.garrison = {"2 x watchman - shortblade", "1 x crossbowman - arbalest"};
  page.patrols = {"loop: hall -> gallery -> yard", "~40s, 12s gap at cellar"};
  page.hazards = {"tripwire - east corridor"};
  page.pageNumber = 7;
  return page;
}

bool notebookBuildsBookShellSketchAndNotes() {
  const iggy3d::ProductNotebookReconPage page = sampleReconPage();
  iggy3d::ProductNotebookUiRequest request;
  request.tab = iggy3d::ProductNotebookTab::Maps;
  request.page = &page;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductNotebookUiDrawList(request);

  bool ok = true;
  ok &= expect(list.ready, "notebook draw list ready");
  ok &= expect(list.status == "product_notebook_ui_ready", "ready status");
  ok &= expect(list.virtualWidth == 1280U, "virtual width");
  ok &= expect(list.selectedAction == "maps", "active tab recorded");
  ok &= expect(list.theme == iggy3d::ProductUiThemeId::Journal,
               "notebook wears the journal theme");
  ok &= expect(list.primitiveCount == list.primitives.size(), "primitive count");

  ok &= expect(findPrimitive(list, "notebook.book") != nullptr, "book shell");
  ok &= expect(findPrimitive(list, "notebook.page.left") != nullptr, "left page");
  ok &= expect(findPrimitive(list, "notebook.page.right") != nullptr, "right page");

  const iggy3d::ProductUiPrimitive* title =
      findPrimitive(list, "notebook.sketch.title");
  ok &= expect(title != nullptr && title->text == "WARDEN'S KEEP - GROUND",
               "sketch title text");
  return ok;
}

bool notebookRendersBlockyFloorPlanFromAscii() {
  const iggy3d::ProductNotebookReconPage page = sampleReconPage();
  iggy3d::ProductNotebookUiRequest request;
  request.page = &page;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductNotebookUiDrawList(request);

  bool ok = true;
  // The sample grid has 15 '#' cells and 3 entry glyphs (D, W, V).
  ok &= expect(countWithPrefix(list, "notebook.plan.wall_") == 15U,
               "wall cell count from ascii");
  const iggy3d::ProductUiPrimitive* door =
      findPrimitive(list, "notebook.plan.entry_1_3.glyph");
  const iggy3d::ProductUiPrimitive* window =
      findPrimitive(list, "notebook.plan.entry_3_0.glyph");
  const iggy3d::ProductUiPrimitive* vent =
      findPrimitive(list, "notebook.plan.entry_4_2.glyph");
  ok &= expect(door != nullptr && door->text == "D", "door marker glyph");
  ok &= expect(window != nullptr && window->text == "W", "window marker glyph");
  ok &= expect(vent != nullptr && vent->text == "V", "vent marker glyph");
  return ok;
}

bool notebookRendersNotesSections() {
  const iggy3d::ProductNotebookReconPage page = sampleReconPage();
  iggy3d::ProductNotebookUiRequest request;
  request.page = &page;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductNotebookUiDrawList(request);

  const iggy3d::ProductUiPrimitive* garrison =
      findPrimitive(list, "notebook.garrison.heading");
  const iggy3d::ProductUiPrimitive* garrisonEntry =
      findPrimitive(list, "notebook.garrison.entry_0");
  const iggy3d::ProductUiPrimitive* patrol =
      findPrimitive(list, "notebook.patrol.heading");
  const iggy3d::ProductUiPrimitive* hazards =
      findPrimitive(list, "notebook.hazards.heading");
  const iggy3d::ProductUiPrimitive* hazardEntry =
      findPrimitive(list, "notebook.hazards.entry_0");
  const iggy3d::ProductUiPrimitive* pageNumber =
      findPrimitive(list, "notebook.page_number");

  bool ok = true;
  ok &= expect(garrison != nullptr && garrison->text == "GARRISON",
               "garrison heading");
  ok &= expect(garrisonEntry != nullptr &&
                   garrisonEntry->text == "2 x watchman - shortblade",
               "garrison first entry");
  ok &= expect(patrol != nullptr && patrol->text == "PATROL", "patrol heading");
  ok &= expect(hazards != nullptr && hazards->text == "HAZARDS",
               "hazards heading");
  ok &= expect(hazardEntry != nullptr &&
                   hazardEntry->text == "tripwire - east corridor",
               "hazards first entry");
  ok &= expect(pageNumber != nullptr && pageNumber->text == "- 7 -",
               "page number");
  return ok;
}

bool notebookTabsHighlightTheActiveSection() {
  const iggy3d::ProductNotebookReconPage page = sampleReconPage();
  iggy3d::ProductNotebookUiRequest request;
  request.tab = iggy3d::ProductNotebookTab::Jobs;
  request.page = &page;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductNotebookUiDrawList(request);

  const iggy3d::ProductUiPrimitive* jobs = findPrimitive(list, "notebook.tab.jobs");
  const iggy3d::ProductUiPrimitive* jobsGlyph =
      findPrimitive(list, "notebook.tab.jobs.glyph");
  const iggy3d::ProductUiPrimitive* maps = findPrimitive(list, "notebook.tab.maps");

  bool ok = true;
  ok &= expect(list.selectedAction == "jobs", "active tab is jobs");
  ok &= expect(jobs != nullptr && jobs->selected, "jobs tab selected");
  ok &= expect(jobsGlyph != nullptr && jobsGlyph->text == "J", "jobs tab glyph");
  ok &= expect(maps != nullptr && !maps->selected, "maps tab not selected");
  ok &= expect(findPrimitive(list, "notebook.tab.spells") != nullptr,
               "spells tab present");
  return ok;
}

bool notebookWithoutPageIsNotReady() {
  iggy3d::ProductNotebookUiRequest request;
  request.page = nullptr;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductNotebookUiDrawList(request);
  bool ok = true;
  ok &= expect(!list.ready, "missing page rejected");
  ok &= expect(list.reasonCode == "product_notebook_ui_missing_page",
               "missing page reason");
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok &= notebookBuildsBookShellSketchAndNotes();
  ok &= notebookRendersBlockyFloorPlanFromAscii();
  ok &= notebookRendersNotesSections();
  ok &= notebookTabsHighlightTheActiveSection();
  ok &= notebookWithoutPageIsNotReady();
  if (!ok) {
    return 1;
  }
  std::cout << "product_notebook_ui_draw_list_tests=pass\n";
  return 0;
}

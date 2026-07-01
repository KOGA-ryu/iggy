#include "app/iggy3d/menu/PauseUi.hpp"

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

const iggy3d::UiHitRegion* findHitRegion(const iggy3d::ProductUiDrawList& list,
                                         std::string_view semanticId) {
  for (const iggy3d::UiHitRegion& region : list.hitRegions) {
    if (region.semanticId == semanticId) {
      return &region;
    }
  }
  return nullptr;
}

iggy3d::PauseMenuModel samplePauseModel() {
  iggy3d::PauseMenuContext context;
  context.pauseOpen = true;
  context.runtimeSessionAvailable = true;
  context.saveRootWritable = true;
  context.compatibleSaveCount = 1U;
  context.developerToolsEnabled = true;
  return iggy3d::buildPauseMenuModel(context, iggy3d::FrontendAction::Resume);
}

bool pauseMenuBuildsJournalThemedPage() {
  const iggy3d::PauseMenuModel model = samplePauseModel();
  iggy3d::ProductPauseUiRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductPauseUiDrawList(request);

  const iggy3d::ProductUiPrimitive* title = findPrimitive(list, "pause.title");
  const iggy3d::ProductUiPrimitive* resume =
      findPrimitive(list, "pause.row.resume.label");
  const iggy3d::ProductUiPrimitive* resumeHighlight =
      findPrimitive(list, "pause.row.resume.highlight");

  bool ok = true;
  ok &= expect(list.ready, "pause draw list ready");
  ok &= expect(list.status == "product_pause_ui_ready", "ready status");
  ok &= expect(list.theme == iggy3d::ProductUiThemeId::Journal,
               "pause menu wears the journal theme");
  ok &= expect(list.selectedAction == "resume", "selected action recorded");
  ok &= expect(findPrimitive(list, "pause.page") != nullptr, "journal page");
  ok &= expect(title != nullptr && title->text == "PAUSED", "pause title");
  ok &= expect(resume != nullptr && resume->text == "Resume", "resume row label");
  ok &= expect(resume != nullptr && resume->selected, "resume row selected");
  ok &= expect(resumeHighlight != nullptr,
               "selected row has a highlight background");
  return ok;
}

bool pauseRowsEmitHitRegionsForTheirActions() {
  const iggy3d::PauseMenuModel model = samplePauseModel();
  iggy3d::ProductPauseUiRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductPauseUiDrawList(request);

  const iggy3d::UiHitRegion* resumeHit =
      findHitRegion(list, "pause.row.resume.label");
  bool ok = true;
  ok &= expect(resumeHit != nullptr &&
                   resumeHit->action == iggy3d::FrontendAction::Resume,
               "resume row emits a hit region for its action");
  ok &= expect(resumeHit != nullptr && resumeHit->kind == iggy3d::UiHitKind::Button,
               "pause row hit region is a button");
  return ok;
}

bool pauseActionLabelsAreHumanReadable() {
  bool ok = true;
  ok &= expect(iggy3d::productPauseActionLabel(iggy3d::FrontendAction::Resume) ==
                   "Resume",
               "resume label");
  ok &= expect(iggy3d::productPauseActionLabel(iggy3d::FrontendAction::ExitGame) ==
                   "Quit Game",
               "exit game label");
  ok &= expect(iggy3d::productPauseActionLabel(
                   iggy3d::FrontendAction::SaveAndExit) == "Save & Exit",
               "save and exit label");
  return ok;
}

bool pauseWithoutModelIsNotReady() {
  iggy3d::ProductPauseUiRequest request;
  request.model = nullptr;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductPauseUiDrawList(request);
  bool ok = true;
  ok &= expect(!list.ready, "missing model rejected");
  ok &= expect(list.reasonCode == "product_pause_ui_missing_model",
               "missing model reason");
  ok &= expect(list.theme == iggy3d::ProductUiThemeId::Journal,
               "even the rejected pause list is journal-themed");
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok &= pauseMenuBuildsJournalThemedPage();
  ok &= pauseRowsEmitHitRegionsForTheirActions();
  ok &= pauseActionLabelsAreHumanReadable();
  ok &= pauseWithoutModelIsNotReady();
  if (!ok) {
    return 1;
  }
  std::cout << "product_pause_ui_draw_list_tests=pass\n";
  return 0;
}

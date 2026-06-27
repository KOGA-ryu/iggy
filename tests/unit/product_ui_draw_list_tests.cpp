#include "app/iggy3d/menu/DrawList.hpp"

#include <iostream>
#include <string_view>

#include "app/frontend/StarterScreen.hpp"

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

iggy3d::FrontendState starterFrontend(iggy3d::FrontendAction selected) {
  iggy3d::FrontendState frontend;
  frontend.screen = iggy3d::FrontendScreen::Starter;
  frontend.childScreen = iggy3d::FrontendScreen::Gameplay;
  frontend.selectedAction = selected;
  frontend.status = "starter_screen_ready";
  return frontend;
}

bool starterRootDrawListContainsHeaderAndRowsInOrder() {
  const iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::Continue);
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList({&frontend, 1U, 1280U, 720U});
  bool ok = true;
  ok &= expect(list.ready, "starter draw list ready");
  ok &= expect(!list.partial, "starter root not partial");
  ok &= expect(list.status == "product_ui_draw_list_ready", "ready status");
  ok &= expect(list.reasonCode == "product_ui_draw_list_ready", "ready reason");
  ok &= expect(list.virtualWidth == 1280U, "virtual width");
  ok &= expect(list.virtualHeight == 720U, "virtual height");
  ok &= expect(list.rowCount == iggy3d::starterActionOrder().size(), "row count");
  ok &= expect(list.selectedAction == "continue", "selected action copied");
  ok &= expect(list.textCount == 11U, "text count");
  ok &= expect(list.rectCount == 11U, "rect count");
  ok &= expect(list.primitiveCount == list.primitives.size(), "primitive count");

  const char* rowIds[] = {
      "starter.row.continue.label",
      "starter.row.new_world.label",
      "starter.row.load_save.label",
      "starter.row.delete.label",
      "starter.row.settings.label",
      "starter.row.dev_tools.label",
      "starter.row.exit.label",
  };
  for (const char* rowId : rowIds) {
    ok &= expect(findPrimitive(list, rowId) != nullptr, rowId);
  }

  const iggy3d::ProductUiPrimitive* header =
      findPrimitive(list, "starter.header.title");
  ok &= expect(header != nullptr, "header primitive exists");
  ok &= expect(header != nullptr && header->text == "IGGY3D", "header text");
  return ok;
}

bool selectedNewWorldGetsHighlightAndStableCoordinates() {
  const iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::NewWorld);
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList({&frontend, 0U, 1280U, 720U});
  const iggy3d::ProductUiPrimitive* highlight =
      findPrimitive(list, "starter.row.new_world.background");
  const iggy3d::ProductUiPrimitive* label =
      findPrimitive(list, "starter.row.new_world.label");
  bool ok = true;
  ok &= expect(highlight != nullptr, "new world highlight exists");
  ok &= expect(highlight != nullptr &&
                   highlight->kind == iggy3d::ProductUiPrimitiveKind::Highlight,
               "new world highlight kind");
  ok &= expect(highlight != nullptr &&
                   highlight->tone == iggy3d::ProductUiTone::Selected,
               "new world selected tone");
  ok &= expect(highlight != nullptr && highlight->selected,
               "new world selected flag");
  ok &= expect(highlight != nullptr && highlight->enabled,
               "new world enabled");
  ok &= expect(highlight != nullptr && highlight->rect.x == 50.0F,
               "new world x coordinate");
  ok &= expect(highlight != nullptr && highlight->rect.y == 194.0F,
               "new world y coordinate");
  ok &= expect(label != nullptr && label->text == "New World",
               "new world label text");
  ok &= expect(list.selectedAction == "new_world", "selected new world code");
  return ok;
}

bool disabledSaveRowsAreRepresentedWithoutCompatibleSaves() {
  const iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::Continue);
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList({&frontend, 0U, 1280U, 720U});
  const iggy3d::ProductUiPrimitive* continueLabel =
      findPrimitive(list, "starter.row.continue.label");
  const iggy3d::ProductUiPrimitive* loadLabel =
      findPrimitive(list, "starter.row.load_save.label");
  const iggy3d::ProductUiPrimitive* deleteLabel =
      findPrimitive(list, "starter.row.delete.label");
  bool ok = true;
  ok &= expect(list.disabledRowCount == 3U, "three disabled save rows");
  ok &= expect(continueLabel != nullptr && !continueLabel->enabled,
               "continue disabled");
  ok &= expect(continueLabel != nullptr &&
                   continueLabel->tone == iggy3d::ProductUiTone::Disabled,
               "continue disabled tone");
  ok &= expect(loadLabel != nullptr && !loadLabel->enabled,
               "load save disabled");
  ok &= expect(deleteLabel != nullptr && !deleteLabel->enabled,
               "delete disabled");
  return ok;
}

bool childScreenIsRepresentedAsPartialStarterSurface() {
  iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::NewWorld);
  frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList({&frontend, 0U, 1280U, 720U});
  const iggy3d::ProductUiPrimitive* child =
      findPrimitive(list, "starter.content.child_screen");
  return expect(list.ready, "child partial is ready") &&
         expect(list.partial, "child partial flag") &&
         expect(list.status == "product_ui_draw_list_partial",
                "child partial status") &&
         expect(list.reasonCode == "starter_child_panel_not_modeled",
                "child partial reason") &&
         expect(child != nullptr && child->text == "new_world",
                "child screen text");
}

bool invalidContextRejectsWithoutRendererTypes() {
  const iggy3d::ProductUiDrawList missing =
      iggy3d::buildProductStarterUiDrawList({nullptr, 0U, 1280U, 720U});
  iggy3d::FrontendState gameplay;
  gameplay.screen = iggy3d::FrontendScreen::Gameplay;
  gameplay.childScreen = iggy3d::FrontendScreen::Gameplay;
  const iggy3d::ProductUiDrawList unsupported =
      iggy3d::buildProductStarterUiDrawList({&gameplay, 0U, 1280U, 720U});
  return expect(!missing.ready, "missing rejected") &&
         expect(missing.reasonCode == "product_ui_draw_list_missing_frontend",
                "missing reason") &&
         expect(!unsupported.ready, "unsupported rejected") &&
         expect(unsupported.status == "product_ui_draw_list_unsupported_screen",
                "unsupported status") &&
         expect(iggy3d::productUiPrimitiveKindName(
                    iggy3d::ProductUiPrimitiveKind::Text) == "text",
                "primitive kind name") &&
         expect(iggy3d::productUiToneName(iggy3d::ProductUiTone::Accent) ==
                    "accent",
                "tone name") &&
         expect(iggy3d::productUiToneColor(iggy3d::ProductUiTone::Accent).a ==
                    1.0F,
                "tone color alpha");
}

}  // namespace

int main() {
  bool ok = true;
  ok &= starterRootDrawListContainsHeaderAndRowsInOrder();
  ok &= selectedNewWorldGetsHighlightAndStableCoordinates();
  ok &= disabledSaveRowsAreRepresentedWithoutCompatibleSaves();
  ok &= childScreenIsRepresentedAsPartialStarterSurface();
  ok &= invalidContextRejectsWithoutRendererTypes();
  if (!ok) {
    return 1;
  }
  std::cout << "product_ui_draw_list_tests=pass\n";
  return 0;
}

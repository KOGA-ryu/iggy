#include "app/iggy3d/menu/DrawList.hpp"

#include <iostream>
#include <string>
#include <string_view>

#include "app/frontend/StarterScreen.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"

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

bool newWorldChildScreenBuildsReadySelectorSurface() {
  iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::NewWorld);
  frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_new_world_ui");
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList({&frontend, 0U, 1280U, 720U, &draft});
  const iggy3d::ProductUiPrimitive* title =
      findPrimitive(list, "starter.content.new_world.title");
  const iggy3d::ProductUiPrimitive* dungeon =
      findPrimitive(list, "starter.content.new_world.dungeon_value");
  const iggy3d::ProductUiPrimitive* room =
      findPrimitive(list, "starter.content.new_world.room_id_value");
  const iggy3d::ProductUiPrimitive* source =
      findPrimitive(list, "starter.content.new_world.source_value");
  const iggy3d::ProductUiPrimitive* create =
      findPrimitive(list, "starter.content.new_world.create");
  const iggy3d::ProductUiPrimitive* asciiRow =
      findPrimitive(list, "starter.content.new_world.ascii_row_0");
  bool ok = true;
  ok &= expect(list.ready, "new world draw-list ready");
  ok &= expect(!list.partial, "new world not partial");
  ok &= expect(list.status == "product_ui_draw_list_ready",
               "new world ready status");
  ok &= expect(list.reasonCode == "product_ui_draw_list_ready",
               "new world ready reason");
  ok &= expect(title != nullptr && title->text == "NEW WORLD",
               "new world title");
  ok &= expect(dungeon != nullptr && dungeon->text == "Loop Keep",
               "new world dungeon value");
  ok &= expect(room != nullptr && room->text == "loop_keep_ascii",
               "new world room id value");
  ok &= expect(source != nullptr &&
                   source->text == "fixtures/rooms/ascii/loop_keep.iggyroom.txt",
               "new world source value");
  ok &= expect(create != nullptr &&
                   create->action == iggy3d::FrontendAction::CreateAndEnter,
               "new world create action");
  ok &= expect(asciiRow != nullptr && asciiRow->text == "#################",
               "new world ascii preview row");
  return ok;
}

bool newWorldEditModeShowsCursorPaletteAndLastGlyph() {
  iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::NewWorld);
  frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_new_world_edit_ui");
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList({&frontend,
                                             0U,
                                             1280U,
                                             720U,
                                             &draft,
                                             true,
                                             true,
                                             1U,
                                             2U,
                                             "C"});
  const iggy3d::ProductUiPrimitive* instructions =
      findPrimitive(list, "starter.content.new_world.instructions");
  const iggy3d::ProductUiPrimitive* draftValue =
      findPrimitive(list, "starter.content.new_world.draft_value");
  const iggy3d::ProductUiPrimitive* cursorValue =
      findPrimitive(list, "starter.content.new_world.cursor_value");
  const iggy3d::ProductUiPrimitive* lastGlyph =
      findPrimitive(list, "starter.content.new_world.last_glyph");
  const iggy3d::ProductUiPrimitive* asciiRow =
      findPrimitive(list, "starter.content.new_world.ascii_row_1");
  const iggy3d::ProductUiPrimitive* create =
      findPrimitive(list, "starter.content.new_world.create");
  bool ok = true;
  ok &= expect(instructions != nullptr &&
                   instructions->text ==
                       "EDIT MODE   ARROWS MOVE   PAINT 1# 2. 3P 4K 5$ 6E 7+ 8C",
               "new world edit instructions show paint keys");
  ok &= expect(draftValue != nullptr && draftValue->text == "CUSTOM",
               "new world edit draft value");
  ok &= expect(cursorValue != nullptr && cursorValue->text == "1,2",
               "new world edit cursor value");
  ok &= expect(lastGlyph != nullptr && lastGlyph->text == "LAST C",
               "new world edit last glyph");
  ok &= expect(asciiRow != nullptr && asciiRow->text.find("[") != std::string::npos,
               "new world edit ascii row marks cursor");
  ok &= expect(create != nullptr &&
                   create->text == "TAB EXIT EDIT   CONFIRM CREATE",
               "new world edit create text");
  return ok;
}

bool nonWorldChildScreenIsRepresentedAsPartialStarterSurface() {
  iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::Settings);
  frontend.childScreen = iggy3d::FrontendScreen::Settings;
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
         expect(child != nullptr && child->text == "settings",
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
  ok &= newWorldChildScreenBuildsReadySelectorSurface();
  ok &= newWorldEditModeShowsCursorPaletteAndLastGlyph();
  ok &= nonWorldChildScreenIsRepresentedAsPartialStarterSurface();
  ok &= invalidContextRejectsWithoutRendererTypes();
  if (!ok) {
    return 1;
  }
  std::cout << "product_ui_draw_list_tests=pass\n";
  return 0;
}

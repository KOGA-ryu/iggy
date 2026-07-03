#include "app/iggy3d/menu/DrawList.hpp"

#include <iostream>
#include <string>
#include <string_view>

#include "app/frontend/SaveSlotModel.hpp"
#include "app/frontend/StarterScreen.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
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

const iggy3d::UiHitRegion* findHitRegion(const iggy3d::ProductUiDrawList& list,
                                         std::string_view semanticId) {
  for (const iggy3d::UiHitRegion& region : list.hitRegions) {
    if (region.semanticId == semanticId) {
      return &region;
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

iggy3d::ProductSaveBridgeResult oneSlotSaves() {
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::SaveSlotPreview slot;
  slot.id = "movement_gym";
  slot.displayTitle = "Movement Gym";
  slot.enabled = true;
  slot.reason = "compatible";
  saves.slots.slots.push_back(slot);
  saves.slots.compatibleCount = 1U;
  return saves;
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
  ok &= expect(list.textCount == 12U, "text count");
  ok &= expect(list.rectCount == 12U, "rect count");
  ok &= expect(list.primitiveCount == list.primitives.size(), "primitive count");

  const char* rowIds[] = {
      "starter.row.continue.label",
      "starter.row.new_world.label",
      "starter.row.creative_new_world.label",
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
  ok &= expect(label != nullptr && label->text == "Build Map",
               "build map label text");
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
  ok &= expect(title != nullptr && title->text == "MAP BUILDER",
               "map builder title");
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
                                             ">",
                                             ">"});
  const iggy3d::ProductUiPrimitive* instructions =
      findPrimitive(list, "starter.content.new_world.instructions");
  const iggy3d::ProductUiPrimitive* draftValue =
      findPrimitive(list, "starter.content.new_world.draft_value");
  const iggy3d::ProductUiPrimitive* cursorValue =
      findPrimitive(list, "starter.content.new_world.cursor_value");
  const iggy3d::ProductUiPrimitive* selectedGlyph =
      findPrimitive(list, "starter.content.new_world.selected_glyph");
  const iggy3d::ProductUiPrimitive* lastGlyph =
      findPrimitive(list, "starter.content.new_world.last_glyph");
  const iggy3d::ProductUiPrimitive* asciiRow =
      findPrimitive(list, "starter.content.new_world.ascii_row_1");
  const iggy3d::ProductUiPrimitive* create =
      findPrimitive(list, "starter.content.new_world.create");
  bool ok = true;
  ok &= expect(instructions != nullptr &&
                   instructions->text ==
                       "EDIT MODE   ARROWS MOVE   1# 2. 3P 4K 5$ 6E 7+ 8C R=RESET 9^ 0v -< => SELECT",
               "new world edit instructions show paint keys");
  ok &= expect(draftValue != nullptr && draftValue->text == "CUSTOM",
               "new world edit draft value");
  ok &= expect(cursorValue != nullptr && cursorValue->text == "1,2",
               "new world edit cursor value");
  ok &= expect(selectedGlyph != nullptr && selectedGlyph->text == "TOOL > RAMP E",
               "new world edit selected glyph");
  ok &= expect(lastGlyph != nullptr && lastGlyph->text == "LAST >",
               "new world edit last glyph");
  ok &= expect(asciiRow != nullptr && asciiRow->text.find("[") != std::string::npos,
               "new world edit ascii row marks cursor");
  ok &= expect(create != nullptr &&
                   create->text == "TAB EXIT EDIT   CONFIRM BUILD",
               "new world edit create text");
  return ok;
}

bool loadSaveChildScreenBuildsSharedSelectorSurface() {
  iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::LoadSave);
  frontend.childScreen = iggy3d::FrontendScreen::LoadSave;
  frontend.saveBrowserMode = iggy3d::FrontendSaveBrowserMode::Load;
  iggy3d::ProductSaveBridgeResult saves = oneSlotSaves();
  iggy3d::ProductUiDrawListRequest request;
  request.frontend = &frontend;
  request.compatibleSaveCount = saves.slots.compatibleCount;
  request.saves = &saves;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList(request);
  const iggy3d::ProductUiPrimitive* title =
      findPrimitive(list, "starter.content.load_save.title");
  const iggy3d::ProductUiPrimitive* slot =
      findPrimitive(list, "starter.content.load_save.slot_0.title");
  const iggy3d::ProductUiPrimitive* load =
      findPrimitive(list, "starter.content.load_save.action.load");
  const iggy3d::ProductUiPrimitive* del =
      findPrimitive(list, "starter.content.load_save.action.delete");
  const iggy3d::ProductUiPrimitive* back =
      findPrimitive(list, "starter.content.load_save.action.back");
  bool ok = true;
  ok &= expect(list.ready, "load save ready");
  ok &= expect(!list.partial, "load save not partial");
  ok &= expect(list.status == "product_ui_draw_list_ready",
               "load save ready status");
  ok &= expect(list.reasonCode == "product_ui_draw_list_ready",
               "load save ready reason");
  ok &= expect(title != nullptr && title->text == "LOAD MAP",
               "load save title");
  ok &= expect(slot != nullptr && slot->text == "Movement Gym",
               "load save slot title");
  ok &= expect(load != nullptr && load->action == iggy3d::FrontendAction::Load,
               "load action primitive");
  ok &= expect(del != nullptr && del->action == iggy3d::FrontendAction::Delete,
               "delete action primitive");
  ok &= expect(back != nullptr && back->action == iggy3d::FrontendAction::Back,
               "back action primitive");
  return ok;
}

bool deleteModeChildScreenBuildsDeleteWorldSurface() {
  iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::Delete);
  frontend.childScreen = iggy3d::FrontendScreen::LoadSave;
  frontend.saveBrowserMode = iggy3d::FrontendSaveBrowserMode::Delete;
  iggy3d::ProductSaveBridgeResult saves = oneSlotSaves();
  iggy3d::ProductUiDrawListRequest request;
  request.frontend = &frontend;
  request.compatibleSaveCount = saves.slots.compatibleCount;
  request.saves = &saves;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList(request);
  const iggy3d::ProductUiPrimitive* title =
      findPrimitive(list, "starter.content.load_save.title");
  const iggy3d::ProductUiPrimitive* del =
      findPrimitive(list, "starter.content.load_save.action.delete");
  const iggy3d::ProductUiPrimitive* back =
      findPrimitive(list, "starter.content.load_save.action.back");
  bool ok = true;
  ok &= expect(list.ready, "delete selector ready");
  ok &= expect(!list.partial, "delete selector not partial");
  ok &= expect(title != nullptr && title->text == "DELETE WORLD",
               "delete selector title");
  ok &= expect(del != nullptr && del->text == "DELETE SELECTED",
               "delete selected action");
  ok &= expect(back != nullptr && back->text == "BACK",
               "delete selector back action");
  return ok;
}

bool deleteConfirmChildScreenBuildsSharedConfirmSurface() {
  iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::Delete);
  frontend.childScreen = iggy3d::FrontendScreen::DeleteConfirm;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList({&frontend, 1U, 1280U, 720U});
  const iggy3d::ProductUiPrimitive* title =
      findPrimitive(list, "starter.content.delete_confirm.title");
  const iggy3d::ProductUiPrimitive* confirm =
      findPrimitive(list, "starter.content.delete_confirm.confirm");
  bool ok = true;
  ok &= expect(list.ready, "delete confirm ready");
  ok &= expect(!list.partial, "delete confirm not partial");
  ok &= expect(title != nullptr && title->text == "DELETE MAP",
               "delete confirm title");
  ok &= expect(confirm != nullptr &&
                   confirm->action == iggy3d::FrontendAction::Delete,
               "delete confirm action");
  return ok;
}

// sd2: the delete-confirm panel names the world it will delete. The map value renders the
// candidate's real displayTitle and the status is truthful — resolved through the one shared
// resolveProductDeleteConfirmModel that the SDL lane also consumes.
bool deleteConfirmPanelNamesTheCandidateWorld() {
  iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::Delete);
  frontend.childScreen = iggy3d::FrontendScreen::DeleteConfirm;

  iggy3d::SaveSlotPreview slot;
  slot.id = "save_42";
  slot.displayTitle = "DUNGEON ALPHA";
  iggy3d::ProductSaveBridgeResult saves;
  saves.slots.slots.push_back(slot);
  saves.slots.compatibleCount = 1U;

  iggy3d::ProductUiDrawListRequest request;
  request.frontend = &frontend;
  request.compatibleSaveCount = 1U;
  request.saves = &saves;
  request.deleteCandidateId = "save_42";

  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList(request);
  const iggy3d::ProductUiPrimitive* mapValue =
      findPrimitive(list, "starter.content.delete_confirm.map_value");
  const iggy3d::ProductUiPrimitive* statusValue =
      findPrimitive(list, "starter.content.delete_confirm.status_value");

  bool ok = true;
  ok &= expect(mapValue != nullptr && mapValue->text == "DUNGEON ALPHA",
               "delete confirm names the real candidate title");
  ok &= expect(statusValue != nullptr && statusValue->text == "READY TO DELETE",
               "delete confirm shows a truthful status");
  // The shared resolver produces the same model the SDL lane draws (compile-enforced there).
  const iggy3d::ProductDeleteConfirmModel model =
      iggy3d::resolveProductDeleteConfirmModel("save_42", saves);
  ok &= expect(model.mapTitle == "DUNGEON ALPHA" && model.statusText == "READY TO DELETE",
               "shared resolver returns the candidate model");
  return ok;
}

bool deleteConfirmPanelFallsBackWhenNoCandidate() {
  iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::Delete);
  frontend.childScreen = iggy3d::FrontendScreen::DeleteConfirm;

  // No candidate id set (and no catalog): the panel must show the deterministic fallback,
  // never a blank or stale string.
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList({&frontend, 0U, 1280U, 720U});
  const iggy3d::ProductUiPrimitive* mapValue =
      findPrimitive(list, "starter.content.delete_confirm.map_value");
  const iggy3d::ProductUiPrimitive* statusValue =
      findPrimitive(list, "starter.content.delete_confirm.status_value");

  bool ok = true;
  ok &= expect(mapValue != nullptr && mapValue->text == "NO MAP SELECTED",
               "delete confirm falls back with no candidate (map)");
  ok &= expect(statusValue != nullptr && statusValue->text == "NO MAP SELECTED",
               "delete confirm falls back with no candidate (status)");
  // A candidate id that is not in the catalog also falls back (no garbage).
  const iggy3d::ProductSaveBridgeResult empty;
  const iggy3d::ProductDeleteConfirmModel missing =
      iggy3d::resolveProductDeleteConfirmModel("ghost_save", empty);
  ok &= expect(missing.mapTitle == "NO MAP SELECTED", "unknown candidate falls back");
  return ok;
}

bool widgetBuiltScreensCaptureActionHitRegions() {
  // The widget layer emits a hit region for every interactive primitive, onto the
  // draw list's hit-region lane. This guards that lane against bit-rot: if a
  // rebuilt screen stops emitting an action's hit region, or emits it from a
  // different rect than it draws, this fails.
  iggy3d::FrontendState confirmFrontend =
      starterFrontend(iggy3d::FrontendAction::Delete);
  confirmFrontend.childScreen = iggy3d::FrontendScreen::DeleteConfirm;
  const iggy3d::ProductUiDrawList confirmList =
      iggy3d::buildProductStarterUiDrawList({&confirmFrontend, 1U, 1280U, 720U});
  const iggy3d::UiHitRegion* confirmHit =
      findHitRegion(confirmList, "starter.content.delete_confirm.confirm");
  const iggy3d::UiHitRegion* backHit =
      findHitRegion(confirmList, "starter.content.delete_confirm.back");
  const iggy3d::ProductUiPrimitive* confirmDraw =
      findPrimitive(confirmList, "starter.content.delete_confirm.confirm");
  bool ok = true;
  ok &= expect(confirmList.hitRegionCount == confirmList.hitRegions.size(),
               "hit region count matches vector");
  ok &= expect(confirmHit != nullptr &&
                   confirmHit->action == iggy3d::FrontendAction::Delete,
               "delete-confirm confirm hit region");
  ok &= expect(confirmHit != nullptr &&
                   confirmHit->kind == iggy3d::UiHitKind::Button,
               "confirm hit region is a button");
  ok &= expect(backHit != nullptr && backHit->action == iggy3d::FrontendAction::Back,
               "delete-confirm back hit region");
  // The hit region shares the drawn primitive's rect — draw and hit cannot drift.
  ok &= expect(confirmHit != nullptr && confirmDraw != nullptr &&
                   confirmHit->rect.x == confirmDraw->rect.x &&
                   confirmHit->rect.y == confirmDraw->rect.y,
               "hit region rect equals draw rect");

  iggy3d::FrontendState loadFrontend =
      starterFrontend(iggy3d::FrontendAction::LoadSave);
  loadFrontend.childScreen = iggy3d::FrontendScreen::LoadSave;
  loadFrontend.saveBrowserMode = iggy3d::FrontendSaveBrowserMode::Load;
  iggy3d::ProductSaveBridgeResult saves = oneSlotSaves();
  iggy3d::ProductUiDrawListRequest request;
  request.frontend = &loadFrontend;
  request.compatibleSaveCount = saves.slots.compatibleCount;
  request.saves = &saves;
  const iggy3d::ProductUiDrawList loadList =
      iggy3d::buildProductStarterUiDrawList(request);
  ok &= expect(
      findHitRegion(loadList, "starter.content.load_save.action.load") != nullptr,
      "load-save load action hit region");
  ok &= expect(
      findHitRegion(loadList, "starter.content.load_save.action.delete") != nullptr,
      "load-save delete action hit region");
  ok &= expect(
      findHitRegion(loadList, "starter.content.load_save.action.back") != nullptr,
      "load-save back action hit region");
  return ok;
}

bool settingsAndDevToolsChildScreensAreModeled() {
  iggy3d::FrontendState settings =
      starterFrontend(iggy3d::FrontendAction::Settings);
  settings.childScreen = iggy3d::FrontendScreen::Settings;
  iggy3d::ProductUiDrawListRequest settingsRequest;
  settingsRequest.frontend = &settings;
  settingsRequest.settingsTab = iggy3d::FrontendSettingsTab::Gameplay;
  const iggy3d::ProductUiDrawList settingsList =
      iggy3d::buildProductStarterUiDrawList(settingsRequest);

  iggy3d::FrontendState devTools =
      starterFrontend(iggy3d::FrontendAction::DevTools);
  devTools.childScreen = iggy3d::FrontendScreen::StarterDevTools;
  devTools.devToolsCategory = iggy3d::FrontendDevToolsCategory::Movement;
  const iggy3d::ProductUiDrawList devToolsList =
      iggy3d::buildProductStarterUiDrawList({&devTools, 0U, 1280U, 720U});

  const iggy3d::ProductUiPrimitive* settingsTitle =
      findPrimitive(settingsList, "starter.content.settings.title");
  const iggy3d::ProductUiPrimitive* gameplayTab =
      findPrimitive(settingsList, "starter.content.settings.tab.gameplay");
  const iggy3d::ProductUiPrimitive* devToolsTitle =
      findPrimitive(devToolsList, "starter.content.dev_tools.title");
  const iggy3d::ProductUiPrimitive* movementCategory =
      findPrimitive(devToolsList, "starter.content.dev_tools.category.movement");
  bool ok = true;
  ok &= expect(settingsList.ready, "settings draw list ready");
  ok &= expect(!settingsList.partial, "settings draw list not partial");
  ok &= expect(settingsTitle != nullptr && settingsTitle->text == "SETTINGS",
               "settings title");
  ok &= expect(gameplayTab != nullptr && gameplayTab->selected,
               "settings selected tab");
  ok &= expect(devToolsList.ready, "dev tools draw list ready");
  ok &= expect(!devToolsList.partial, "dev tools draw list not partial");
  ok &= expect(devToolsTitle != nullptr && devToolsTitle->text == "DEV TOOLS",
               "dev tools title");
  ok &= expect(movementCategory != nullptr && movementCategory->selected,
               "dev tools selected category");
  return ok;
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

bool loadSaveDrawListHighlightsSelectedSlot() {
  iggy3d::FrontendState frontend =
      starterFrontend(iggy3d::FrontendAction::LoadSave);
  frontend.childScreen = iggy3d::FrontendScreen::LoadSave;
  frontend.saveBrowserMode = iggy3d::FrontendSaveBrowserMode::Load;

  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::SaveSlotPreview first;
  first.id = "save_001";
  first.displayTitle = "First Map";
  first.enabled = true;
  first.reason = "compatible";
  iggy3d::SaveSlotPreview second;
  second.id = "save_002";
  second.displayTitle = "Second Map";
  second.enabled = true;
  second.reason = "compatible";
  saves.slots.slots.push_back(first);
  saves.slots.slots.push_back(second);
  saves.slots.compatibleCount = 2U;

  iggy3d::ProductUiDrawListRequest request;
  request.frontend = &frontend;
  request.compatibleSaveCount = saves.slots.compatibleCount;
  request.saves = &saves;
  // The input layer has the player on the SECOND map; the drawn highlight must
  // follow it instead of defaulting to slot 0.
  request.selectedSaveId = "save_002";

  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductStarterUiDrawList(request);
  const iggy3d::ProductUiPrimitive* slot0 =
      findPrimitive(list, "starter.content.load_save.slot_0.title");
  const iggy3d::ProductUiPrimitive* slot1 =
      findPrimitive(list, "starter.content.load_save.slot_1.title");
  bool ok = true;
  ok &= expect(slot0 != nullptr && slot0->text == "First Map", "first slot title");
  ok &= expect(slot1 != nullptr && slot1->text == "Second Map", "second slot title");
  ok &= expect(slot0 != nullptr && !slot0->selected,
               "unselected slot is not highlighted");
  ok &= expect(slot1 != nullptr && slot1->selected,
               "selected save id highlights its row");
  return ok;
}

int main() {
  bool ok = true;
  ok &= starterRootDrawListContainsHeaderAndRowsInOrder();
  ok &= loadSaveDrawListHighlightsSelectedSlot();
  ok &= selectedNewWorldGetsHighlightAndStableCoordinates();
  ok &= disabledSaveRowsAreRepresentedWithoutCompatibleSaves();
  ok &= newWorldChildScreenBuildsReadySelectorSurface();
  ok &= newWorldEditModeShowsCursorPaletteAndLastGlyph();
  ok &= loadSaveChildScreenBuildsSharedSelectorSurface();
  ok &= deleteModeChildScreenBuildsDeleteWorldSurface();
  ok &= deleteConfirmChildScreenBuildsSharedConfirmSurface();
  ok &= deleteConfirmPanelNamesTheCandidateWorld();
  ok &= deleteConfirmPanelFallsBackWhenNoCandidate();
  ok &= widgetBuiltScreensCaptureActionHitRegions();
  ok &= settingsAndDevToolsChildScreensAreModeled();
  ok &= invalidContextRejectsWithoutRendererTypes();
  if (!ok) {
    return 1;
  }
  std::cout << "product_ui_draw_list_tests=pass\n";
  return 0;
}

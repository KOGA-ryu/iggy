#include "app/iggy3d/menu/ActionHandlers.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/world/DungeonDraft.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/automation/Automation.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/input/InputAction.hpp"
#include "runtime/session/Session.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductAppOptions makeTestOptions(std::string_view name) {
  iggy3d::ProductAppOptions options;
  options.saveRoot = std::filesystem::temp_directory_path() /
                     std::string{"iggy3d_"} / std::string{name};
  std::filesystem::remove_all(options.saveRoot);
  return options;
}

iggy3d::ProductMenuActionResult applyNewWorldAction(
    iggy3d::InputAction action,
    iggy3d::FrontendState& frontend,
    const iggy3d::ProductAppOptions& options,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::WorldSetupDraft& draft,
    iggy3d::ProductAppWindowState& window) {
  // sd1: the context gained a save-catalog reference (the in-window new-world refresh target).
  // These tests assert draft/window behavior, not the catalog, so a local scratch is enough.
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductNewWorldMenuActionContext context{
      frontend, options, saves, activeSession, draft, window};
  return iggy3d::applyProductNewWorldMenuAction(action, context);
}

bool newWorldDraftHotkeysDriveDraftState() {
  iggy3d::FrontendState frontend;
  frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
  iggy3d::ProductAppOptions options =
      makeTestOptions("new_world_hotkeys_tests");
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_new_world_hotkeys");
  iggy3d::ProductAppWindowState window;

  const iggy3d::ProductMenuActionResult tab = applyNewWorldAction(
      iggy3d::InputAction::MenuNextTab, frontend, options, activeSession, draft,
      window);
  bool ok = expect(tab.handled && tab.accepted, "tab toggles new world draft mode");
  ok = expect(window.worldSetupDungeonDraftEditMode,
              "draft edit mode enabled after menu next tab") &&
       ok;
  ok = expect(draft.selectedField == iggy3d::WorldSetupField::AsciiRoom,
              "draft edit mode selects ascii room field") &&
       ok;
  ok = expect(frontend.status == "dungeon_draft_edit_mode_on",
              "tab records edit mode on status") &&
       ok;

  const iggy3d::ProductMenuActionResult down = applyNewWorldAction(
      iggy3d::InputAction::MenuDown, frontend, options, activeSession, draft,
      window);
  const iggy3d::ProductMenuActionResult rightOne = applyNewWorldAction(
      iggy3d::InputAction::MenuRight, frontend, options, activeSession, draft,
      window);
  const iggy3d::ProductMenuActionResult rightTwo = applyNewWorldAction(
      iggy3d::InputAction::MenuRight, frontend, options, activeSession, draft,
      window);
  ok = expect(down.handled && down.accepted && rightOne.handled &&
                  rightOne.accepted && rightTwo.handled && rightTwo.accepted,
              "menu movement is accepted in draft edit mode") &&
       ok;
  ok = expect(window.worldSetupDungeonDraftCursorRow == 1U,
              "draft cursor row reaches one") &&
       ok;
  ok = expect(window.worldSetupDungeonDraftCursorColumn == 2U,
              "draft cursor column reaches two") &&
       ok;

  const bool selected = iggy3d::selectDungeonDraftPaintGlyph(window, '>');
  ok = expect(selected, "ramp glyph selects paint tool") && ok;
  ok = expect(window.worldSetupDungeonDraftSelectedGlyph == ">",
              "painting records selected glyph") &&
       ok;
  ok = expect(!window.worldSetupDungeonDraftModified,
              "selecting paint tool does not mutate draft") &&
       ok;

  const iggy3d::ProductMenuActionResult paint = applyNewWorldAction(
      iggy3d::InputAction::MenuConfirm, frontend, options, activeSession, draft,
      window);
  ok = expect(paint.handled && paint.accepted,
              "confirm paints selected glyph in edit mode") &&
       ok;
  ok = expect(window.worldSetupDungeonDraftModified,
              "painting marks dungeon draft modified") &&
       ok;
  ok = expect(window.worldSetupDungeonDraftStatus == "dungeon_draft_cell_painted",
              "painting records stable draft paint status") &&
       ok;
  ok = expect(window.worldSetupDungeonDraftReasonCode ==
                  "dungeon_draft_cell_painted",
              "painting records stable draft paint reason") &&
       ok;
  ok = expect(window.worldSetupDungeonDraftLastGlyph == ">",
              "painting records last glyph") &&
       ok;
  ok = expect(window.worldSetupAsciiRoomId ==
                  iggy3d::productCustomDungeonRoomId(),
              "painting switches to custom dungeon room id") &&
       ok;
  return ok;
}

bool manualDungeonSelectorCanCreateObjectCrateRoom() {
  iggy3d::FrontendState frontend;
  frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
  iggy3d::ProductAppOptions options =
      makeTestOptions("new_world_crate_selector_tests");
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_new_world_crate");
  iggy3d::ProductAppWindowState window;
  iggy3d::recordWorldSetupDraftState(draft, window);

  bool ok = expect(window.worldSetupDungeonTitle == "Loop Keep",
                   "selector starts on loop keep") &&
            expect(window.worldSetupDungeonIndex == 1U,
                   "selector starts on index one") &&
            expect(window.worldSetupDungeonCount ==
                       iggy3d::productBuiltinDungeonCatalog().size(),
                   "selector count mirrors catalog");

  while (draft.asciiRoomId != "object_crate_room") {
    const iggy3d::ProductMenuActionResult right = applyNewWorldAction(
        iggy3d::InputAction::MenuRight, frontend, options, activeSession, draft,
        window);
    ok = expect(right.handled && right.accepted, "selector right accepted") && ok;
  }

  ok = expect(draft.worldName == "Object Crate Room",
              "selector reaches object crate title") &&
       ok;
  ok = expect(draft.asciiRoomId == "object_crate_room",
              "selector reaches object crate room id") &&
       ok;
  ok = expect(window.worldSetupStatus == "world_setup_dungeon_selected",
              "selector records selected status") &&
       ok;
  ok = expect(window.worldSetupDungeonTitle == "Object Crate Room",
              "window mirrors selected dungeon title") &&
       ok;
  ok = expect(window.worldSetupDungeonIndex == 8U,
              "window mirrors selected dungeon index") &&
       ok;
  ok = expect(window.worldSetupDungeonCount ==
                  iggy3d::productBuiltinDungeonCatalog().size(),
              "window mirrors selected dungeon count") &&
       ok;
  ok = expect(window.worldSetupAsciiRoomId == "object_crate_room",
              "window mirrors selected room id") &&
       ok;

  const iggy3d::ProductMenuActionResult confirm = applyNewWorldAction(
      iggy3d::InputAction::MenuConfirm, frontend, options, activeSession, draft,
      window);

  ok = expect(confirm.handled && confirm.accepted, "confirm creates selected room") &&
       ok;
  ok = expect(activeSession.has_value(), "selected room session created") && ok;
  ok = expect(frontend.childScreen == iggy3d::FrontendScreen::Gameplay,
              "confirm enters gameplay") &&
       ok;
  ok = expect(window.worldSetupStatus == "world_setup_create_requested",
              "confirm records create requested") &&
       ok;
  ok = expect(window.worldSetupDungeonTitle == "Object Crate Room",
              "created receipt keeps dungeon title") &&
       ok;
  ok = expect(window.worldSetupDungeonIndex == 8U,
              "created receipt keeps dungeon index") &&
       ok;
  ok = expect(window.worldSetupDungeonCount ==
                  iggy3d::productBuiltinDungeonCatalog().size(),
              "created receipt keeps dungeon count") &&
       ok;
  ok = expect(window.worldCreationAsciiRoomId == "object_crate_room",
              "created room id") &&
       ok;
  ok = expect(window.activeRoom.loaded, "created active room loaded") && ok;
  ok = expect(window.activeRoom.roomId == "object_crate_room",
              "active room id") &&
       ok;
  ok = expect(window.activeRoom.authoredObjectCount == 1U,
              "active room authored object count") &&
       ok;
  ok = expect(window.activeRoom.staticMeshCount == 36U,
              "active room static mesh count") &&
       ok;
  ok = expect(window.activeRoomCollision.ready,
              "active room collision ready") &&
       ok;
  ok = expect(window.activeRoomCollision.querySurfaceCount == 57U,
              "active room collision surface count") &&
       ok;
  return ok;
}

bool manualDungeonSelectorCanCreateMovementGym() {
  iggy3d::FrontendState frontend;
  frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
  iggy3d::ProductAppOptions options =
      makeTestOptions("new_world_movement_gym_selector_tests");
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeProductDefaultWorldSetupDraft("seed_new_world_movement_gym");
  iggy3d::ProductAppWindowState window;
  iggy3d::recordWorldSetupDraftState(draft, window);

  bool selected = false;
  bool ok = true;
  for (std::size_t index = 0;
       index < iggy3d::productBuiltinDungeonCatalog().size();
       ++index) {
    if (draft.asciiRoomId == "movement_gym") {
      selected = true;
      break;
    }
    const iggy3d::ProductMenuActionResult next = applyNewWorldAction(
        iggy3d::InputAction::MenuRight, frontend, options, activeSession, draft,
        window);
    ok = expect(next.handled && next.accepted,
                "selector advances toward movement gym") &&
         ok;
  }
  ok = expect(selected, "selector reaches movement gym") && ok;
  ok = expect(draft.worldName == "Movement Gym",
              "selector reaches movement gym title") &&
       ok;
  ok = expect(draft.asciiRoomId == "movement_gym",
              "selector reaches movement gym room id") &&
       ok;
  ok = expect(window.worldSetupDungeonTitle == "Movement Gym",
              "window mirrors movement gym title") &&
       ok;
  ok = expect(window.worldSetupDungeonIndex ==
                  iggy3d::productBuiltinDungeonIndexForRoomId("movement_gym") + 1U,
              "window mirrors movement gym index") &&
       ok;
  ok = expect(window.worldSetupDungeonCount ==
                  iggy3d::productBuiltinDungeonCatalog().size(),
              "window mirrors movement gym count") &&
       ok;

  const iggy3d::ProductMenuActionResult confirm = applyNewWorldAction(
      iggy3d::InputAction::MenuConfirm, frontend, options, activeSession, draft,
      window);

  ok = expect(confirm.handled && confirm.accepted,
              "confirm creates movement gym") &&
       ok;
  ok = expect(activeSession.has_value(), "movement gym session created") && ok;
  ok = expect(frontend.childScreen == iggy3d::FrontendScreen::Gameplay,
              "movement gym enters gameplay") &&
       ok;
  ok = expect(window.worldCreationAsciiRoomId == "movement_gym",
              "movement gym created room id") &&
       ok;
  ok = expect(window.activeRoom.loaded, "movement gym active room loaded") && ok;
  ok = expect(window.activeRoom.roomId == "movement_gym",
              "movement gym active room id") &&
       ok;
  ok = expect(window.activeRoom.authoredObjectCount == 10U,
              "movement gym authored object count") &&
       ok;
  ok = expect(window.activeRoom.staticMeshCount == 1045U,
              "movement gym static mesh count") &&
       ok;
  ok = expect(window.activeRoomCollision.ready,
              "movement gym collision ready") &&
       ok;
  ok = expect(window.activeRoomCollision.walkableSurfaceCount == 780U,
              "movement gym ledge top walkable surface count") &&
       ok;
  return ok;
}

}  // namespace

int main() {
  const bool ok = newWorldDraftHotkeysDriveDraftState() &&
                  manualDungeonSelectorCanCreateObjectCrateRoom() &&
                  manualDungeonSelectorCanCreateMovementGym();
  return ok ? 0 : 1;
}

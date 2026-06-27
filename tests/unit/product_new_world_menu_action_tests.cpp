#include "app/iggy3d/menu/ActionHandlers.hpp"

#include <iostream>
#include <optional>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/world/DungeonDraft.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/automation/Automation.hpp"
#include "app/input/InputAction.hpp"
#include "runtime/session/Session.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductMenuActionResult applyNewWorldAction(
    iggy3d::InputAction action,
    iggy3d::FrontendState& frontend,
    const iggy3d::ProductAppOptions& options,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::WorldSetupDraft& draft,
    iggy3d::ProductAppWindowState& window) {
  iggy3d::ProductNewWorldMenuActionContext context{
      frontend, options, activeSession, draft, window};
  return iggy3d::applyProductNewWorldMenuAction(action, context);
}

bool newWorldDraftHotkeysDriveDraftState() {
  iggy3d::FrontendState frontend;
  frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
  iggy3d::ProductAppOptions options;
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

  const bool painted = iggy3d::applyDungeonDraftPaintGlyph(draft, window, '#');
  ok = expect(painted, "number-key wall glyph paints draft cell") && ok;
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
  ok = expect(window.worldSetupDungeonDraftLastGlyph == "#",
              "painting records last glyph") &&
       ok;
  ok = expect(window.worldSetupAsciiRoomId ==
                  iggy3d::productCustomDungeonRoomId(),
              "painting switches to custom dungeon room id") &&
       ok;
  return ok;
}

}  // namespace

int main() {
  const bool ok = newWorldDraftHotkeysDriveDraftState();
  return ok ? 0 : 1;
}

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/product/Automation.hpp"

#include <iostream>
#include <set>
#include <string>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

const iggy3d::ProductAutomationCommandSpec& spec(
    const iggy3d::ProductAutomationCommandRegistry& registry,
    std::string_view key) {
  return iggy3d::findProductAutomationCommandSpec(registry, key);
}

void expectCanonical(const iggy3d::ProductAutomationCommandRegistry& registry,
                     std::string_view key,
                     std::string_view canonical) {
  expect(iggy3d::productAutomationCanonicalKey(registry, key) == canonical,
         std::string{"canonical key mismatch for "} + std::string{key});
}

}  // namespace

int main() {
  using Category = iggy3d::ProductAutomationCommandCategory;
  using Value = iggy3d::ProductAutomationValueKind;

  const iggy3d::ProductAutomationCommandRegistry registry =
      iggy3d::makeProductAutomationCommandRegistry();

  expect(iggy3d::productAutomationCommandCategoryName(Category::RoomEditor) ==
             "room_editor",
         "room editor category name is stable");
  expect(iggy3d::productAutomationCommandCategoryName(Category::DeletedSaveBrowser) ==
             "deleted_save_browser",
         "deleted save browser category name is stable");
  expect(iggy3d::productAutomationValueKindName(Value::Bool) == "bool",
         "bool value kind name is stable");
  expect(iggy3d::productAutomationValueKindName(Value::Direction) == "direction",
         "direction value kind name is stable");
  expect(iggy3d::productAutomationCommandIdName(
             iggy3d::ProductAutomationCommandId::RoomEditorPlace) ==
             "room_editor.place",
         "room editor command id name is stable");

  expect(spec(registry, "automation.owner").category == Category::Owner,
         "owner command is registered");
  expect(spec(registry, "menu.input").category == Category::MenuInput,
         "menu input command is registered");
  expect(spec(registry, "menu.confirm").category == Category::MenuShortcut,
         "menu shortcut command is registered");
  expect(spec(registry, "frontend.select").category == Category::FrontendSelect,
         "frontend select command is registered");
  expect(spec(registry, "frontend.execute").category ==
             Category::FrontendExecute,
         "frontend execute command is registered");
  expect(spec(registry, "world.title").category == Category::WorldSetup,
         "world setup command is registered");
  expect(spec(registry, "world.draft_cell").category == Category::DungeonDraft,
         "dungeon draft command is registered");
  expect(spec(registry, "ascii_room.build").category == Category::AsciiRoom,
         "ascii room command is registered");
  expect(spec(registry, "save.select").category == Category::SaveBrowser,
         "save browser command is registered");
  expect(spec(registry, "save.deleted_select").category ==
             Category::DeletedSaveBrowser,
         "deleted save browser command is registered");
  expect(spec(registry, "room_edit.add_wall").category == Category::RoomEdit,
         "room edit command is registered");
  expect(spec(registry, "room_editor.place").category == Category::RoomEditor,
         "room editor command is registered");
  expect(spec(registry, "game.move_x").category == Category::GameplayInput,
         "gameplay input command is registered");
  expect(spec(registry, "settings.tab").category == Category::Settings,
         "settings command is registered");
  expect(spec(registry, "dev_tools.category").category == Category::DevTools,
         "dev tools command is registered");
  expect(spec(registry, "system.pause").category == Category::System,
         "system command is registered");

  expectCanonical(registry, "frontend.room_editor_place", "room_editor.place");
  expectCanonical(registry, "frontend.room_edit_start_active",
                  "room_edit.start_active");
  expectCanonical(registry, "frontend.save_select", "save.select");
  expectCanonical(registry, "frontend.show_deleted_saves", "save.show_deleted");
  expectCanonical(registry, "frontend.deleted_save_select",
                  "save.deleted_select");
  expectCanonical(registry, "frontend.save_recover", "save.recover");
  expectCanonical(registry, "pause.select", "frontend.select");
  expectCanonical(registry, "pause.execute", "frontend.execute");
  expectCanonical(registry, "world_setup.draft_paint", "world.draft_paint");
  expectCanonical(registry, "frontend.draft_move", "world.draft_move");
  expectCanonical(registry, "frontend.game_attack", "game.attack");

  const iggy3d::ProductAutomationCommandSpec& unknown =
      spec(registry, "not.a.real.command");
  expect(unknown.category == Category::Unknown, "unknown key returns unknown spec");
  expect(unknown.canonicalKey == "unknown", "unknown key canonical is unknown");

  expect(spec(registry, "room_editor.cycle_tool").ignoresFalseBool,
         "cycle tool ignores false bool");
  expect(!spec(registry, "room_editor.tool").ignoresFalseBool,
         "room editor tool does not ignore false bool");
  expect(spec(registry, "game.move_x").valueKind == Value::Float,
         "game move x is a float value");
  expect(spec(registry, "room_edit.add_floor").valueKind == Value::Csv,
         "add floor is a csv value");

  const iggy3d::ProductMenuInputAutomationResult menuUp =
      iggy3d::resolveProductMenuInputAutomation("up");
  expect(menuUp.valid, "menu input up is valid");
  expect(menuUp.inputAction == iggy3d::InputAction::MenuUp,
         "menu input up resolves to menu up");

  const iggy3d::ProductMenuInputAutomationResult menuNone =
      iggy3d::resolveProductMenuInputAutomation("none");
  expect(menuNone.valid, "menu input none is valid");
  expect(menuNone.inputAction == iggy3d::InputAction::None,
         "menu input none resolves to none");

  const iggy3d::ProductMenuInputAutomationResult menuInvalid =
      iggy3d::resolveProductMenuInputAutomation("teleport");
  expect(!menuInvalid.valid, "menu input teleport is invalid");

  const iggy3d::ProductFrontendSelectAutomationResult selectSettings =
      iggy3d::resolveProductFrontendSelectAutomation("settings");
  expect(selectSettings.valid, "frontend select settings is valid");
  expect(selectSettings.action == iggy3d::FrontendAction::Settings,
         "frontend select settings resolves to settings");

  const iggy3d::ProductFrontendSelectAutomationResult selectPauseAlias =
      iggy3d::resolveProductFrontendSelectAutomation("pause.select");
  expect(!selectPauseAlias.valid, "frontend select alias is not resolved by parser");

  const iggy3d::ProductFrontendSelectAutomationResult selectInvalid =
      iggy3d::resolveProductFrontendSelectAutomation("teleport");
  expect(!selectInvalid.valid, "frontend select teleport is invalid");

  const iggy3d::ProductSettingsTabAutomationResult settingsInput =
      iggy3d::resolveProductSettingsTabAutomation("input");
  expect(settingsInput.valid, "settings tab input is valid");
  expect(settingsInput.settingsTab == iggy3d::FrontendSettingsTab::Input,
         "settings tab input resolves to input");

  const iggy3d::ProductSettingsTabAutomationResult settingsDeveloper =
      iggy3d::resolveProductSettingsTabAutomation("developer");
  expect(settingsDeveloper.valid, "settings tab developer is valid");
  expect(settingsDeveloper.settingsTab == iggy3d::FrontendSettingsTab::Developer,
         "settings tab developer resolves to developer");

  const iggy3d::ProductSettingsTabAutomationResult settingsInvalid =
      iggy3d::resolveProductSettingsTabAutomation("teleport");
  expect(!settingsInvalid.valid, "settings tab teleport is invalid");

  const iggy3d::ProductDevToolsCategoryAutomationResult devToolsSession =
      iggy3d::resolveProductDevToolsCategoryAutomation("session");
  expect(devToolsSession.valid, "dev tools category session is valid");
  expect(devToolsSession.category == iggy3d::FrontendDevToolsCategory::Session,
         "dev tools category session resolves to session");

  const iggy3d::ProductDevToolsCategoryAutomationResult devToolsRenderer =
      iggy3d::resolveProductDevToolsCategoryAutomation("renderer");
  expect(devToolsRenderer.valid, "dev tools category renderer is valid");
  expect(devToolsRenderer.category == iggy3d::FrontendDevToolsCategory::Renderer,
         "dev tools category renderer resolves to renderer");

  const iggy3d::ProductDevToolsCategoryAutomationResult devToolsInvalid =
      iggy3d::resolveProductDevToolsCategoryAutomation("teleport");
  expect(!devToolsInvalid.valid, "dev tools category teleport is invalid");

  const iggy3d::ProductAutomationCommandDispatchResult placeDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.room_editor_place"});
  expect(placeDispatch.handled, "frontend room editor place is dispatch-handled");
  expect(placeDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::RoomEditorPlace,
         "frontend room editor place dispatch id is stable");
  expect(placeDispatch.canonicalActionLabel == "room_editor.place",
         "frontend room editor place dispatch label is canonical");

  const iggy3d::ProductAutomationCommandDispatchResult addWallDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.room_edit_add_wall"});
  expect(addWallDispatch.handled, "frontend room edit add wall is dispatch-handled");
  expect(addWallDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::RoomEditAddWall,
         "frontend room edit add wall dispatch id is stable");

  const iggy3d::ProductAutomationCommandDispatchResult menuDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{&registry, "menu.up"});
  expect(menuDispatch.handled, "menu up is dispatch-handled");
  expect(menuDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::MenuShortcut,
         "menu up dispatch id is menu shortcut");
  expect(menuDispatch.spec.inputAction == iggy3d::InputAction::MenuUp,
         "menu up dispatch action is stable");

  const iggy3d::ProductAutomationCommandDispatchResult frontendDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.select"});
  expect(!frontendDispatch.handled,
         "frontend select is not dispatch-handled by this slice");

  const iggy3d::ProductAutomationCommandDispatchResult unknownDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "not.a.real.command"});
  expect(!unknownDispatch.handled, "unknown dispatch is not handled");

  std::set<std::string> canonicalKeys;
  std::set<std::string> ownedKeys;
  for (const iggy3d::ProductAutomationCommandSpec& commandSpec :
       registry.specs) {
    if (commandSpec.category == Category::Unknown) {
      continue;
    }
    expect(canonicalKeys.insert(commandSpec.canonicalKey).second,
           std::string{"duplicate canonical key "} + commandSpec.canonicalKey);
    expect(ownedKeys.insert(commandSpec.canonicalKey).second,
           std::string{"duplicate owned key "} + commandSpec.canonicalKey);
    for (const std::string& alias : commandSpec.aliases) {
      expect(ownedKeys.insert(alias).second,
             std::string{"duplicate alias ownership "} + alias);
    }
  }

  return failures == 0 ? 0 : 1;
}

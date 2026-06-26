#include "app/iggy3d/ProductAutomationCommandRegistry.hpp"

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

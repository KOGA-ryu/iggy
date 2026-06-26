#include "app/iggy3d/ProductAutomationCommandRegistry.hpp"

#include <algorithm>
#include <array>
#include <iterator>

namespace iggy3d {

std::string_view productAutomationCommandCategoryName(
    ProductAutomationCommandCategory category) {
  static constexpr std::array names{
      std::string_view{"owner"},
      std::string_view{"menu_input"},
      std::string_view{"menu_shortcut"},
      std::string_view{"frontend_select"},
      std::string_view{"frontend_execute"},
      std::string_view{"world_setup"},
      std::string_view{"dungeon_draft"},
      std::string_view{"ascii_room"},
      std::string_view{"save_browser"},
      std::string_view{"deleted_save_browser"},
      std::string_view{"room_edit"},
      std::string_view{"room_editor"},
      std::string_view{"gameplay_input"},
      std::string_view{"gameplay_tape"},
      std::string_view{"settings"},
      std::string_view{"dev_tools"},
      std::string_view{"system"},
      std::string_view{"unknown"},
  };
  return names[static_cast<std::size_t>(category)];
}

std::string_view productAutomationValueKindName(
    ProductAutomationValueKind valueKind) {
  static constexpr std::array names{
      std::string_view{"bool"},
      std::string_view{"string"},
      std::string_view{"float"},
      std::string_view{"size"},
      std::string_view{"csv"},
      std::string_view{"action"},
      std::string_view{"direction"},
      std::string_view{"tool"},
      std::string_view{"raw"},
      std::string_view{"unknown"},
  };
  return names[static_cast<std::size_t>(valueKind)];
}

ProductAutomationCommandRegistry makeProductAutomationCommandRegistry() {
  using Category = ProductAutomationCommandCategory;
  using Value = ProductAutomationValueKind;

  ProductAutomationCommandRegistry registry;
  registry.specs = {
      {"automation.owner", {}, Category::Owner, Value::Raw, true, false, "any"},

      {"menu.input", {}, Category::MenuInput, Value::Action, true, false, "menu"},
      {"menu.up", {}, Category::MenuShortcut, Value::Bool, true, true, "menu"},
      {"menu.down", {}, Category::MenuShortcut, Value::Bool, true, true, "menu"},
      {"menu.left", {}, Category::MenuShortcut, Value::Bool, true, true, "menu"},
      {"menu.right", {}, Category::MenuShortcut, Value::Bool, true, true, "menu"},
      {"menu.confirm", {}, Category::MenuShortcut, Value::Bool, true, true,
       "menu"},
      {"menu.back", {}, Category::MenuShortcut, Value::Bool, true, true, "menu"},
      {"menu.next_tab", {}, Category::MenuShortcut, Value::Bool, true, true,
       "menu"},
      {"menu.previous_tab", {}, Category::MenuShortcut, Value::Bool, true, true,
       "menu"},

      {"frontend.select", {"pause.select"}, Category::FrontendSelect,
       Value::Action, true, false, "frontend"},
      {"frontend.execute", {"pause.execute", "dev_tools.execute"},
       Category::FrontendExecute, Value::Bool, true, true, "frontend"},

      {"world.title", {"world_setup.title"}, Category::WorldSetup,
       Value::String, true, false, "new_world"},
      {"world.dungeon_id",
       {"world.map_id", "world_setup.dungeon_id", "world_setup.map_id"},
       Category::WorldSetup, Value::String, true, false, "new_world"},
      {"world.create", {"world_setup.create"}, Category::WorldSetup,
       Value::Bool, true, true, "new_world"},
      {"world.ascii_room_text", {"world_setup.ascii_room_text"},
       Category::WorldSetup, Value::Raw, true, false, "new_world"},
      {"world.ascii_room_id", {"world_setup.ascii_room_id"},
       Category::WorldSetup, Value::String, true, false, "new_world"},
      {"world.ascii_room_source_name", {"world_setup.ascii_room_source_name"},
       Category::WorldSetup, Value::String, true, false, "new_world"},

      {"world.draft_edit_mode", {"world_setup.draft_edit_mode"},
       Category::DungeonDraft, Value::Bool, true, false, "new_world"},
      {"world.draft_move", {"world_setup.draft_move", "frontend.draft_move"},
       Category::DungeonDraft, Value::Direction, true, false, "new_world"},
      {"world.draft_paint", {"world_setup.draft_paint", "frontend.draft_paint"},
       Category::DungeonDraft, Value::String, true, false, "new_world"},
      {"world.draft_cell", {"world_setup.draft_cell"}, Category::DungeonDraft,
       Value::Csv, true, false, "new_world"},

      {"ascii_room.text", {"frontend.ascii_room_text"}, Category::AsciiRoom,
       Value::Raw, true, false, "frontend"},
      {"ascii_room.room_id", {"frontend.ascii_room_id"}, Category::AsciiRoom,
       Value::String, true, false, "frontend"},
      {"ascii_room.source_name", {"frontend.ascii_room_source_name"},
       Category::AsciiRoom, Value::String, true, false, "frontend"},
      {"ascii_room.build", {"frontend.ascii_room_build"}, Category::AsciiRoom,
       Value::Bool, true, true, "frontend"},
      {"ascii_room.activate", {"frontend.ascii_room_activate"},
       Category::AsciiRoom, Value::Bool, true, true, "frontend"},

      {"save.select", {"frontend.save_select"}, Category::SaveBrowser,
       Value::String, true, false, "load_save"},
      {"save.delete", {"frontend.save_delete"}, Category::SaveBrowser,
       Value::Bool, true, true, "load_save"},
      {"save.show_deleted", {"frontend.show_deleted_saves"},
       Category::SaveBrowser, Value::Bool, true, true, "load_save"},
      {"save.deleted_select", {"frontend.deleted_save_select"},
       Category::DeletedSaveBrowser, Value::String, true, false, "load_save"},
      {"save.recover", {"frontend.save_recover"}, Category::DeletedSaveBrowser,
       Value::Bool, true, true, "load_save"},

      {"room_edit.start", {"frontend.room_edit_start"}, Category::RoomEdit,
       Value::Bool, true, true, "room_edit"},
      {"room_edit.start_active", {"frontend.room_edit_start_active"},
       Category::RoomEdit, Value::Bool, true, true, "room_edit"},
      {"room_edit.add_floor", {"frontend.room_edit_add_floor"},
       Category::RoomEdit, Value::Csv, true, false, "room_edit"},
      {"room_edit.add_wall", {"frontend.room_edit_add_wall"},
       Category::RoomEdit, Value::Csv, true, false, "room_edit"},
      {"room_edit.delete_floor", {"frontend.room_edit_delete_floor"},
       Category::RoomEdit, Value::String, true, false, "room_edit"},
      {"room_edit.delete_wall", {"frontend.room_edit_delete_wall"},
       Category::RoomEdit, Value::String, true, false, "room_edit"},
      {"room_edit.undo", {"frontend.room_edit_undo"}, Category::RoomEdit,
       Value::Bool, true, true, "room_edit"},
      {"room_edit.redo", {"frontend.room_edit_redo"}, Category::RoomEdit,
       Value::Bool, true, true, "room_edit"},

      {"room_editor.move", {"frontend.room_editor_move"}, Category::RoomEditor,
       Value::Direction, true, false, "room_editor"},
      {"room_editor.tool", {"frontend.room_editor_tool"}, Category::RoomEditor,
       Value::Tool, true, false, "room_editor"},
      {"room_editor.cycle_tool", {"frontend.room_editor_cycle_tool"},
       Category::RoomEditor, Value::Bool, true, true, "room_editor"},
      {"room_editor.wall_direction", {"frontend.room_editor_wall_direction"},
       Category::RoomEditor, Value::Direction, true, false, "room_editor"},
      {"room_editor.place", {"frontend.room_editor_place"},
       Category::RoomEditor, Value::Bool, true, true, "room_editor"},
      {"editor.input", {}, Category::RoomEditor, Value::Action, true, false,
       "editor"},

      {"game.move_x", {"frontend.game_move_x"}, Category::GameplayInput,
       Value::Float, true, false, "gameplay"},
      {"game.move_y", {"frontend.game_move_y"}, Category::GameplayInput,
       Value::Float, true, false, "gameplay"},
      {"game.attack", {"frontend.game_attack"}, Category::GameplayInput,
       Value::Bool, true, true, "gameplay"},
      {"game.interact", {"frontend.game_interact"}, Category::GameplayInput,
       Value::Bool, true, true, "gameplay"},

      {"settings.tab", {}, Category::Settings, Value::String, true, false,
       "settings"},
      {"settings.apply", {}, Category::Settings, Value::Bool, true, true,
       "settings"},
      {"settings.restore_defaults", {}, Category::Settings, Value::Bool, true,
       true, "settings"},
      {"settings.back", {"system.back"}, Category::Settings, Value::Bool, true,
       true, "settings"},
      {"dev_tools.category", {}, Category::DevTools, Value::String, true, false,
       "dev_tools"},

      {"frontend.return_to_title", {}, Category::System, Value::Bool, true,
       false, "frontend"},
      {"system.pause", {}, Category::System, Value::Bool, true, true, "system"},
      {"system.quit", {}, Category::System, Value::Bool, true, false, "system"},

      {"unknown", {}, Category::Unknown, Value::Unknown, false, false, "none"},
  };
  return registry;
}

const ProductAutomationCommandSpec& findProductAutomationCommandSpec(
    const ProductAutomationCommandRegistry& registry,
    std::string_view key) {
  const auto searchEnd = std::prev(registry.specs.end());
  const auto row = std::find_if(
      registry.specs.begin(), searchEnd,
      [key](const ProductAutomationCommandSpec& candidate) {
        return candidate.canonicalKey == key ||
               std::find(candidate.aliases.begin(), candidate.aliases.end(),
                         key) != candidate.aliases.end();
      });
  return *row;
}

std::string_view productAutomationCanonicalKey(
    const ProductAutomationCommandRegistry& registry,
    std::string_view key) {
  return findProductAutomationCommandSpec(registry, key).canonicalKey;
}

}  // namespace iggy3d

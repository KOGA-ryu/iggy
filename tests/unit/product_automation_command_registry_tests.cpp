#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/world/DungeonDraft.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/automation/Automation.hpp"

#include <filesystem>
#include <fstream>
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

std::filesystem::path tempAutomationPath() {
  return std::filesystem::temp_directory_path() /
         "iggy3d_product_automation_commands.txt";
}

void writeAutomationFile(const std::filesystem::path& path,
                         std::string_view content) {
  std::ofstream output(path);
  output << content;
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
  expect(iggy3d::productAutomationCommandIdName(
             iggy3d::ProductAutomationCommandId::RoomEditorMousePick) ==
             "room_editor.mouse_pick",
         "room editor mouse pick command id name is stable");
  expect(iggy3d::productAutomationCommandIdName(
             iggy3d::ProductAutomationCommandId::RoomEditorPreview) ==
             "room_editor.preview",
         "room editor preview command id name is stable");
  expect(iggy3d::productAutomationCommandIdName(
             iggy3d::ProductAutomationCommandId::RoomEditorPreviewConfirm) ==
             "room_editor.preview_confirm",
         "room editor preview confirm command id name is stable");
  expect(iggy3d::productAutomationCommandIdName(
             iggy3d::ProductAutomationCommandId::RoomEditorPreviewCancel) ==
             "room_editor.preview_cancel",
         "room editor preview cancel command id name is stable");
  expect(iggy3d::productAutomationCommandIdName(
             iggy3d::ProductAutomationCommandId::ControllerInput) ==
             "controller.input",
         "controller input command id name is stable");
  expect(iggy3d::productAutomationCommandIdName(
             iggy3d::ProductAutomationCommandId::GameplayJump) ==
             "gameplay.jump",
         "gameplay jump command id name is stable");
  expect(iggy3d::productAutomationCommandIdName(
             iggy3d::ProductAutomationCommandId::GameplayPlayerPosition) ==
             "gameplay.player_position",
         "gameplay player position command id name is stable");
  expect(iggy3d::productAutomationCommandIdName(
             iggy3d::ProductAutomationCommandId::GameplayPhysicsMovement) ==
             "gameplay.physics_movement",
         "gameplay physics movement command id name is stable");

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
  expect(spec(registry, "room_editor.mouse_pick").category == Category::RoomEditor,
         "room editor mouse pick command is registered");
  expect(spec(registry, "room_editor.preview").category == Category::RoomEditor,
         "room editor preview command is registered");
  expect(spec(registry, "room_editor.preview_confirm").category ==
             Category::RoomEditor,
         "room editor preview confirm command is registered");
  expect(spec(registry, "room_editor.preview_cancel").category ==
             Category::RoomEditor,
         "room editor preview cancel command is registered");
  expect(spec(registry, "game.move_x").category == Category::GameplayInput,
         "gameplay input command is registered");
  expect(spec(registry, "gameplay.jump").category == Category::GameplayInput,
         "gameplay jump command is registered");
  expect(spec(registry, "gameplay.player_position").category ==
             Category::GameplayInput,
         "gameplay player position command is registered");
  expect(spec(registry, "gameplay.physics_movement").category ==
             Category::GameplayInput,
         "gameplay physics movement command is registered");
  expect(spec(registry, "controller.input").category == Category::ControllerInput,
         "controller input command is registered");
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
  expectCanonical(registry, "frontend.game_jump", "gameplay.jump");
  expectCanonical(registry, "game.player_position", "gameplay.player_position");
  expectCanonical(registry, "frontend.physics_movement",
                  "gameplay.physics_movement");
  expectCanonical(registry, "controller.sample", "controller.input");

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
  expect(spec(registry, "room_editor.mouse_pick").valueKind == Value::Csv,
         "mouse pick is a csv value");
  expect(spec(registry, "room_editor.preview").valueKind == Value::Bool,
         "room editor preview is a bool value");
  expect(spec(registry, "room_editor.preview").ignoresFalseBool,
         "room editor preview ignores false bool");
  expect(spec(registry, "room_editor.preview_confirm").valueKind == Value::Bool,
         "room editor preview confirm is a bool value");
  expect(spec(registry, "room_editor.preview_confirm").ignoresFalseBool,
         "room editor preview confirm ignores false bool");
  expect(spec(registry, "room_editor.preview_cancel").valueKind == Value::Bool,
         "room editor preview cancel is a bool value");
  expect(spec(registry, "room_editor.preview_cancel").ignoresFalseBool,
         "room editor preview cancel ignores false bool");
  expect(spec(registry, "controller.input").valueKind == Value::Action,
         "controller input is an action sequence value");
  expect(spec(registry, "gameplay.jump").valueKind == Value::Bool,
         "gameplay jump is a bool value");
  expect(spec(registry, "gameplay.jump").ignoresFalseBool,
         "gameplay jump ignores false bool");
  expect(spec(registry, "gameplay.player_position").valueKind == Value::Csv,
         "gameplay player position is a csv value");
  expect(spec(registry, "gameplay.physics_movement").valueKind == Value::Bool,
         "gameplay physics movement is a bool value");

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

  const iggy3d::ProductSaveSelectionAutomationResult saveSelection =
      iggy3d::resolveProductSaveSelectionAutomation("slot_1");
  expect(saveSelection.valid, "save selection slot_1 is valid");
  expect(saveSelection.saveId == "slot_1",
         "save selection slot_1 preserves the id");

  const iggy3d::ProductSaveSelectionAutomationResult saveSelectionInvalid =
      iggy3d::resolveProductSaveSelectionAutomation("");
  expect(!saveSelectionInvalid.valid, "save selection empty is invalid");

  const iggy3d::ProductBoolAutomationResult boolTrue =
      iggy3d::resolveProductAutomationBool("yes");
  expect(boolTrue.valid, "generic bool yes is valid");
  expect(boolTrue.requested, "generic bool yes resolves to true");

  const iggy3d::ProductBoolAutomationResult boolFalse =
      iggy3d::resolveProductAutomationBool("no");
  expect(boolFalse.valid, "generic bool no is valid");
  expect(!boolFalse.requested, "generic bool no resolves to false");

  const iggy3d::ProductBoolAutomationResult boolInvalid =
      iggy3d::resolveProductAutomationBool("teleport");
  expect(!boolInvalid.valid, "generic bool teleport is invalid");

  const iggy3d::ProductGameplayAxisAutomationResult gameplayAxis =
      iggy3d::resolveProductGameplayAxisAutomation("1.25");
  expect(gameplayAxis.valid, "gameplay axis 1.25 is valid");
  expect(gameplayAxis.value == 1.25F, "gameplay axis 1.25 preserves value");

  const iggy3d::ProductGameplayAxisAutomationResult gameplayAxisInvalid =
      iggy3d::resolveProductGameplayAxisAutomation("teleport");
  expect(!gameplayAxisInvalid.valid, "gameplay axis teleport is invalid");

  const iggy3d::ProductNonEmptyStringAutomationResult nonEmptyString =
      iggy3d::resolveProductNonEmptyStringAutomation("room_1");
  expect(nonEmptyString.valid, "non-empty string room_1 is valid");
  expect(nonEmptyString.value == "room_1",
         "non-empty string room_1 preserves the value");

  const iggy3d::ProductNonEmptyStringAutomationResult emptyString =
      iggy3d::resolveProductNonEmptyStringAutomation("");
  expect(!emptyString.valid, "empty string is invalid");

  const iggy3d::ProductDungeonDraftDirectionAutomationResult draftUp =
      iggy3d::resolveProductDungeonDraftDirectionAutomation("up");
  expect(draftUp.valid, "dungeon draft direction up is valid");
  expect(draftUp.direction == iggy3d::ProductDungeonDraftDirection::Up,
         "dungeon draft direction up resolves to up");

  const iggy3d::ProductDungeonDraftDirectionAutomationResult draftInvalid =
      iggy3d::resolveProductDungeonDraftDirectionAutomation("teleport");
  expect(!draftInvalid.valid, "dungeon draft direction teleport is invalid");

  const iggy3d::ProductDungeonDraftPaintAutomationResult draftPaint =
      iggy3d::resolveProductDungeonDraftPaintAutomation("#");
  expect(draftPaint.valid, "dungeon draft paint # is valid");
  expect(draftPaint.glyph == "#", "dungeon draft paint preserves glyph");

  const iggy3d::ProductDungeonDraftPaintAutomationResult draftPaintInvalid =
      iggy3d::resolveProductDungeonDraftPaintAutomation("##");
  expect(!draftPaintInvalid.valid, "dungeon draft paint ## is invalid");

  const iggy3d::ProductDungeonDraftCellAutomationResult draftCell =
      iggy3d::resolveProductDungeonDraftCellAutomation("4", "7", "@");
  expect(draftCell.valid, "dungeon draft cell 4,7,@ is valid");
  expect(draftCell.row == 4U, "dungeon draft cell preserves row");
  expect(draftCell.column == 7U, "dungeon draft cell preserves column");

  const iggy3d::ProductDungeonDraftCellAutomationResult draftCellInvalid =
      iggy3d::resolveProductDungeonDraftCellAutomation("4", "7", "@@");
  expect(!draftCellInvalid.valid, "dungeon draft cell invalid glyph is rejected");

  const std::filesystem::path automationPath = tempAutomationPath();
  {
    writeAutomationFile(automationPath,
                        "menu.input=up\n"
                        "world.draft_move=left\n");
    iggy3d::ProductAppWindowState window;
    std::vector<iggy3d::ProductAutomationCommand> commands;
    expect(iggy3d::readProductAutomationCommands(automationPath, window, commands),
           "automation control file loads");
    expect(window.automationControlRequested,
           "automation control requested is recorded");
    expect(window.automationControlLoaded, "automation control loaded is recorded");
    expect(window.automationControlStatus == "loaded",
           "automation control status is loaded");
    expect(window.automationControlLineCount == 2U,
           "automation control line count tracks parsed lines");
    expect(commands.size() == 2U, "automation control command count is stable");
    expect(commands[0].key == "menu.input" && commands[0].value == "up",
           "automation control first command preserves key/value");
    expect(commands[1].key == "world.draft_move" && commands[1].value == "left",
           "automation control second command preserves key/value");
  }
  {
    writeAutomationFile(automationPath,
                        "menu.input=up\n"
                        "menu.input=down\n");
    iggy3d::ProductAppWindowState window;
    std::vector<iggy3d::ProductAutomationCommand> commands;
    expect(!iggy3d::readProductAutomationCommands(automationPath, window, commands),
           "duplicate automation control keys are rejected");
    expect(window.automationControlStatus == "duplicate_key",
           "duplicate automation control status is stable");
    expect(window.automationControlLastKey == "menu.input",
           "duplicate automation control last key is stable");
    expect(commands.size() == 1U, "duplicate automation control preserves prior rows");
  }

  bool saveDeleteBool = false;
  expect(iggy3d::resolveProductSaveBrowserBoolAutomation("yes", saveDeleteBool),
         "save browser bool yes is valid");
  expect(saveDeleteBool, "save browser bool yes resolves to true");

  expect(iggy3d::resolveProductSaveBrowserBoolAutomation("no", saveDeleteBool),
         "save browser bool no is valid");
  expect(!saveDeleteBool, "save browser bool no resolves to false");

  expect(!iggy3d::resolveProductSaveBrowserBoolAutomation("teleport", saveDeleteBool),
         "save browser bool teleport is invalid");

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

  const iggy3d::ProductAutomationCommandDispatchResult mousePickDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.room_editor_mouse_pick"});
  expect(mousePickDispatch.handled, "frontend room editor mouse pick is dispatch-handled");
  expect(mousePickDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::RoomEditorMousePick,
         "frontend room editor mouse pick dispatch id is stable");
  expect(mousePickDispatch.canonicalActionLabel == "room_editor.mouse_pick",
         "frontend room editor mouse pick dispatch label is canonical");

  const iggy3d::ProductAutomationCommandDispatchResult previewDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.room_editor_preview"});
  expect(previewDispatch.handled, "frontend room editor preview is dispatch-handled");
  expect(previewDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::RoomEditorPreview,
         "frontend room editor preview dispatch id is stable");
  expect(previewDispatch.canonicalActionLabel == "room_editor.preview",
         "frontend room editor preview dispatch label is canonical");

  const iggy3d::ProductAutomationCommandDispatchResult previewPlaceDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.room_editor_preview_place"});
  expect(previewPlaceDispatch.handled,
         "frontend room editor preview place is dispatch-handled");
  expect(previewPlaceDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::RoomEditorPreview,
         "frontend room editor preview place dispatch id is stable");
  expect(previewPlaceDispatch.canonicalActionLabel == "room_editor.preview",
         "frontend room editor preview place dispatch label is canonical");

  const iggy3d::ProductAutomationCommandDispatchResult confirmDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.room_editor_preview_confirm"});
  expect(confirmDispatch.handled,
         "frontend room editor preview confirm is dispatch-handled");
  expect(confirmDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::RoomEditorPreviewConfirm,
         "frontend room editor preview confirm dispatch id is stable");
  expect(confirmDispatch.canonicalActionLabel == "room_editor.preview_confirm",
         "frontend room editor preview confirm dispatch label is canonical");

  const iggy3d::ProductAutomationCommandDispatchResult confirmPlaceDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "room_editor.confirm_place"});
  expect(confirmPlaceDispatch.handled,
         "room editor confirm place is dispatch-handled");
  expect(confirmPlaceDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::RoomEditorPreviewConfirm,
         "room editor confirm place dispatch id is stable");
  expect(confirmPlaceDispatch.canonicalActionLabel == "room_editor.preview_confirm",
         "room editor confirm place dispatch label is canonical");

  const iggy3d::ProductAutomationCommandDispatchResult cancelDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.room_editor_preview_cancel"});
  expect(cancelDispatch.handled,
         "frontend room editor preview cancel is dispatch-handled");
  expect(cancelDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::RoomEditorPreviewCancel,
         "frontend room editor preview cancel dispatch id is stable");
  expect(cancelDispatch.canonicalActionLabel == "room_editor.preview_cancel",
         "frontend room editor preview cancel dispatch label is canonical");

  const iggy3d::ProductAutomationCommandDispatchResult cancelPreviewDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.room_editor_cancel_preview"});
  expect(cancelPreviewDispatch.handled,
         "frontend room editor cancel preview is dispatch-handled");
  expect(cancelPreviewDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::RoomEditorPreviewCancel,
         "frontend room editor cancel preview dispatch id is stable");
  expect(cancelPreviewDispatch.canonicalActionLabel == "room_editor.preview_cancel",
         "frontend room editor cancel preview dispatch label is canonical");

  const iggy3d::ProductAutomationCommandDispatchResult addWallDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.room_edit_add_wall"});
  expect(addWallDispatch.handled, "frontend room edit add wall is dispatch-handled");
  expect(addWallDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::RoomEditAddWall,
         "frontend room edit add wall dispatch id is stable");

  const iggy3d::ProductAutomationCommandDispatchResult controllerDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "controller.sample"});
  expect(controllerDispatch.handled, "controller sample alias is dispatch-handled");
  expect(controllerDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::ControllerInput,
         "controller input dispatch id is stable");
  expect(controllerDispatch.canonicalActionLabel == "controller.input",
         "controller input dispatch label is canonical");

  const iggy3d::ProductAutomationCommandDispatchResult physicsMovementDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.physics_movement"});
  expect(physicsMovementDispatch.handled,
         "frontend physics movement alias is dispatch-handled");
  expect(physicsMovementDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::GameplayPhysicsMovement,
         "physics movement dispatch id is stable");
  expect(physicsMovementDispatch.canonicalActionLabel ==
             "gameplay.physics_movement",
         "physics movement dispatch label is canonical");

  const iggy3d::ProductAutomationCommandDispatchResult gameplayJumpDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "frontend.game_jump"});
  expect(gameplayJumpDispatch.handled,
         "frontend gameplay jump alias is dispatch-handled");
  expect(gameplayJumpDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::GameplayJump,
         "gameplay jump dispatch id is stable");
  expect(gameplayJumpDispatch.canonicalActionLabel == "gameplay.jump",
         "gameplay jump dispatch label is canonical");

  const iggy3d::ProductAutomationCommandDispatchResult playerPositionDispatch =
      iggy3d::resolveProductAutomationCommandDispatch(
          iggy3d::ProductAutomationCommandDispatchRequest{
              &registry, "game.player_position"});
  expect(playerPositionDispatch.handled,
         "game player position alias is dispatch-handled");
  expect(playerPositionDispatch.spec.commandId ==
             iggy3d::ProductAutomationCommandId::GameplayPlayerPosition,
         "game player position dispatch id is stable");
  expect(playerPositionDispatch.canonicalActionLabel ==
             "gameplay.player_position",
         "game player position dispatch label is canonical");

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

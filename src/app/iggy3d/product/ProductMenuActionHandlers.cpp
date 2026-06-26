#include "app/iggy3d/product/ProductMenuActionHandlers.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/ProductBuiltinDungeon.hpp"
#include "app/iggy3d/ProductDungeonDraft.hpp"
#include "app/iggy3d/ProductAppOperations.hpp"
#include "app/iggy3d/ProductMenuTransitions.hpp"
#include "app/iggy3d/ProductRoomAuthoringController.hpp"
#include "app/iggy3d/ProductSaveFlow.hpp"
#include "app/iggy3d/product/Automation.hpp"
#include "app/iggy3d/product/AutomationRoomEditing.hpp"

namespace iggy3d {
namespace {

using DevToolsBackHandler = void (*)(FrontendState&, ProductAppWindowState&);

struct DevToolsMenuActionConfig {
  std::string_view selectionChangedStatus;
  std::string_view confirmStatus;
  DevToolsBackHandler backHandler = nullptr;
};

FrontendAction nextPauseSelection(FrontendAction current, InputAction action) {
  const auto& actions = pauseActionOrder();
  std::size_t index = 0;
  // branch-gate: BG-1017
  for (std::size_t i = 0; i < actions.size(); ++i) {
    // branch-gate: BG-1017
    if (actions[i] == current) {
      index = i;
      break;
    }
  }
  // branch-gate: BG-1017
  if (action == InputAction::MenuUp) {
    index = index == 0 ? actions.size() - 1 : index - 1;  // branch-gate: BG-1017
  } else if (action == InputAction::MenuDown) {  // branch-gate: BG-1017
    index = (index + 1) % actions.size();
  }
  return actions[index];
}

FrontendDevToolsCategory nextDevToolsSelection(FrontendDevToolsCategory current,
                                               InputAction action) {
  const auto& categories = devToolsCategoryOrder();
  std::size_t index = 0;
  // branch-gate: BG-1018
  for (std::size_t i = 0; i < categories.size(); ++i) {
    // branch-gate: BG-1018
    if (categories[i] == current) {
      index = i;
      break;
    }
  }
  // branch-gate: BG-1018
  if (action == InputAction::MenuUp) {
    index = index == 0 ? categories.size() - 1 : index - 1;  // branch-gate: BG-1018
  } else if (action == InputAction::MenuDown) {  // branch-gate: BG-1018
    index = (index + 1) % categories.size();
  }
  return categories[index];
}

FrontendSettingsTab nextSettingsSelection(FrontendSettingsTab current,
                                          InputAction action) {
  const auto& tabs = settingsTabOrder();
  std::size_t index = 0;
  // branch-gate: BG-1019
  for (std::size_t i = 0; i < tabs.size(); ++i) {
    // branch-gate: BG-1019
    if (tabs[i] == current) {
      index = i;
      break;
    }
  }
  // branch-gate: BG-1019
  if (action == InputAction::MenuUp) {
    index = index == 0 ? tabs.size() - 1 : index - 1;  // branch-gate: BG-1019
  } else if (action == InputAction::MenuDown) {  // branch-gate: BG-1019
    index = (index + 1) % tabs.size();
  }
  return tabs[index];
}

void closeDevOverlayToGameplay(FrontendState& frontend,
                               ProductAppWindowState& window) {
  closeProductOverlayToGameplayTransition(frontend, window);
}

void closeStarterDevTools(FrontendState& frontend, ProductAppWindowState&) {
  frontend.childScreen = FrontendScreen::Gameplay;
  frontend.devToolsOpen = false;
  frontend.status = "dev_tools_closed";
}

ProductMenuActionResult applyDevToolsMenuAction(
    InputAction action,
    ProductDevToolsMenuActionContext& context,
    const DevToolsMenuActionConfig& config) {
  FrontendState& frontend = context.frontend;
  // branch-gate: BG-1018
  if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
    frontend.devToolsCategory =
        nextDevToolsSelection(frontend.devToolsCategory, action);
    frontend.status = config.selectionChangedStatus;
    return {true, true};
  }
  // branch-gate: BG-1018
  if (action == InputAction::MenuBack) {
    config.backHandler(frontend, context.window);
    return {true, true};
  }
  // branch-gate: BG-1018
  if (action == InputAction::MenuConfirm) {
    frontend.status = config.confirmStatus;
    return {true, true};
  }
  return {true, false};
}

std::optional<ProductDungeonDraftDirection> dungeonDraftDirectionForAction(
    InputAction action) {
  // branch-gate: BG-1021
  if (action == InputAction::MenuUp) {
    return ProductDungeonDraftDirection::Up;
  }
  // branch-gate: BG-1021
  if (action == InputAction::MenuDown) {
    return ProductDungeonDraftDirection::Down;
  }
  // branch-gate: BG-1021
  if (action == InputAction::MenuLeft) {
    return ProductDungeonDraftDirection::Left;
  }
  // branch-gate: BG-1021
  if (action == InputAction::MenuRight) {
    return ProductDungeonDraftDirection::Right;
  }
  return std::nullopt;
}

bool isPreviousBuiltinDungeonAction(InputAction action) {
  return action == InputAction::MenuUp || action == InputAction::MenuLeft;
}

ProductMenuActionResult handlePauseConfirm(ProductPauseMenuActionContext& context) {
  FrontendState& frontend = context.frontend;
  ProductAppWindowState& window = context.window;
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::Resume) {
    closeProductOverlayToGameplayTransition(frontend, window);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::EditRoom) {
    const ProductRoomEditingStartResult started =
        startProductRoomAuthoringFromActiveRoom({window.activeRoom});
    recordProductRoomEditingStart(window, started, "pause_edit_room");
    frontend.status =
        started.ok ? "pause_edit_room_requested" : "pause_edit_room_failed";  // branch-gate: BG-1017
    // branch-gate: BG-1017
    if (started.ok) {
      closeProductOverlayToGameplayTransition(frontend, window);
    }
    return {true, started.ok};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::Settings) {
    openProductPauseSettingsTransition(frontend, window, context.settingsTab);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::DevTools) {
    openProductPauseDevToolsTransition(frontend, window,
                                       FrontendDevToolsCategory::Session);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::Save) {
    executeProductPauseSaveFlow(ProductPauseSaveFlowKind::Save, context.options,
                                frontend, context.activeSession, window);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::SaveAndExit) {
    executeProductPauseSaveFlow(ProductPauseSaveFlowKind::SaveAndExit,
                                context.options, frontend,
                                context.activeSession, window);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::ReturnToTitle) {
    returnProductToTitleTransition(frontend, window);
    context.activeSession.reset();
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::ExitGame) {
    frontend.status = "pause_exit_game_requested";
    context.closeRequested = true;
    return {true, true};
  }
  frontend.status = "pause_action_selected";
  return {true, true};
}

}  // namespace

ProductMenuActionResult applyProductPauseMenuAction(
    InputAction action,
    ProductPauseMenuActionContext context) {
  FrontendState& frontend = context.frontend;
  ProductAppWindowState& window = context.window;
  // branch-gate: BG-1017
  if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
    frontend.selectedAction = nextPauseSelection(frontend.selectedAction, action);
    frontend.status = "pause_menu_selection_changed";
    return {true, true};
  }
  // branch-gate: BG-1017
  if (action == InputAction::MenuBack) {
    closeProductOverlayToGameplayTransition(frontend, window);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (action != InputAction::MenuConfirm) {
    return {true, false};
  }
  return handlePauseConfirm(context);
}

ProductMenuActionResult applyProductDevOverlayMenuAction(
    InputAction action,
    ProductDevToolsMenuActionContext context) {
  static constexpr DevToolsMenuActionConfig config{
      "dev_overlay_selection_changed",
      "dev_overlay_category_selected",
      closeDevOverlayToGameplay,
  };
  return applyDevToolsMenuAction(action, context, config);
}

ProductMenuActionResult applyProductStarterDevToolsMenuAction(
    InputAction action,
    ProductDevToolsMenuActionContext context) {
  static constexpr DevToolsMenuActionConfig config{
      "dev_tools_selection_changed",
      "dev_tools_category_selected",
      closeStarterDevTools,
  };
  return applyDevToolsMenuAction(action, context, config);
}

ProductMenuActionResult applyProductSettingsMenuAction(
    InputAction action,
    ProductSettingsMenuActionContext context) {
  FrontendState& frontend = context.frontend;
  // branch-gate: BG-1019
  if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
    context.settingsTab = nextSettingsSelection(context.settingsTab, action);
    frontend.status = "settings_selection_changed";
    return {true, true};
  }
  // branch-gate: BG-1019
  if (action == InputAction::MenuBack) {
    // branch-gate: BG-1019
    if (frontend.screen == FrontendScreen::Settings &&
        frontend.childScreen == FrontendScreen::Pause) {
      openProductPauseTransition(frontend, context.window,
                                 FrontendAction::Settings);
    } else {
      frontend.childScreen = FrontendScreen::Gameplay;
    }
    frontend.status = "settings_closed";
    return {true, true};
  }
  // branch-gate: BG-1019
  if (action == InputAction::MenuConfirm) {
    frontend.status = "settings_tab_selected";
    return {true, true};
  }
  return {true, false};
}

ProductMenuActionResult applyProductDeleteConfirmMenuAction(
    InputAction action,
    ProductDeleteConfirmMenuActionContext context) {
  // branch-gate: BG-1020
  if (action == InputAction::MenuBack) {
    cancelProductSaveDeleteConfirmation(context.window, context.frontend);
    return {true, true};
  }
  // branch-gate: BG-1020
  if (action == InputAction::MenuConfirm) {
    executeProductSaveSoftDelete(context.options, context.window,
                                 context.frontend);
    return {true, true};
  }
  return {true, false};
}

ProductMenuActionResult applyProductLoadSaveMenuAction(
    InputAction action,
    ProductLoadSaveMenuActionContext context) {
  FrontendState& frontend = context.frontend;
  // branch-gate: BG-1020
  if (action == InputAction::MenuBack) {
    frontend.childScreen = FrontendScreen::Gameplay;
    frontend.status = "load_save_closed";
    return {true, true};
  }
  // branch-gate: BG-1020
  if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
    moveSelectedProductSaveSlot(context.saves.slots, action, context.window);
    frontend.status = "load_save_selection_changed";
    return {true, true};
  }
  // branch-gate: BG-1020
  if (action == InputAction::MenuConfirm) {
    // branch-gate: BG-1020
    if (frontend.selectedAction == FrontendAction::Delete) {
      openProductSaveDeleteConfirmation(context.saves.slots, context.window,
                                        frontend);
    } else {
      launchProductLoadSaveSelection(
          context.options, productWorldTemplateFromOptions(context.options),
          context.saves, frontend, context.activeSession, context.window);
    }
    return {true, true};
  }
  return {true, false};
}

ProductMenuActionResult applyProductNewWorldMenuAction(
    InputAction action,
    ProductNewWorldMenuActionContext context) {
  FrontendState& frontend = context.frontend;
  ProductAppWindowState& window = context.window;
  WorldSetupDraft& worldSetupDraft = context.worldSetupDraft;
  recordWorldSetupDraftState(worldSetupDraft, window);
  // branch-gate: BG-1021
  if (action == InputAction::MenuNextTab) {
    window.worldSetupDungeonDraftEditMode =
        !window.worldSetupDungeonDraftEditMode;
    worldSetupDraft.selectedField =
        window.worldSetupDungeonDraftEditMode ? WorldSetupField::AsciiRoom  // branch-gate: BG-1021
                                              : WorldSetupField::Create;
    window.worldSetupDungeonDraftStatus =
        window.worldSetupDungeonDraftEditMode ? "dungeon_draft_edit_mode_on"
                                              : "dungeon_draft_edit_mode_off";  // branch-gate: BG-1021
    window.worldSetupDungeonDraftReasonCode =
        window.worldSetupDungeonDraftStatus;
    resetDungeonDraftWindowCursor(worldSetupDraft, window);
    frontend.status = window.worldSetupDungeonDraftStatus;
    return {true, true};
  }

  const std::optional<ProductDungeonDraftDirection> direction =
      dungeonDraftDirectionForAction(action);
  // branch-gate: BG-1021
  if (window.worldSetupDungeonDraftEditMode && direction.has_value()) {
    const ProductDungeonDraftOperationResult moved =
        moveProductDungeonDraftCursor(worldSetupDraft,
                                      dungeonDraftCursorFromWindow(window),
                                      *direction);
    recordDungeonDraftOperation(window, moved);
    frontend.status = moved.status;
    return {true, true};
  }
  // branch-gate: BG-1021
  if (!window.worldSetupDungeonDraftEditMode && direction.has_value()) {
    const bool previous = isPreviousBuiltinDungeonAction(action);
    const bool changed = previous
                             ? selectPreviousProductBuiltinDungeon(worldSetupDraft)
                             : selectNextProductBuiltinDungeon(worldSetupDraft);  // branch-gate: BG-1021
    window.worldSetupDungeonDraftModified = false;
    window.worldSetupDungeonDraftEditMode = false;
    resetDungeonDraftWindowCursor(worldSetupDraft, window);
    recordWorldSetupDraftState(worldSetupDraft, window);
    frontend.status =
        changed ? "new_world_dungeon_selection_changed" : "new_world_input_ignored";  // branch-gate: BG-1021
    window.worldSetupStatus =
        changed ? "world_setup_dungeon_selected" : "world_setup_dungeon_unavailable";  // branch-gate: BG-1021
    return {true, true};
  }
  // branch-gate: BG-1021
  if (action == InputAction::MenuBack) {
    frontend.childScreen = FrontendScreen::Gameplay;
    frontend.selectedAction = FrontendAction::NewWorld;
    frontend.status = "new_world_closed";
    window.worldSetupStatus = "world_setup_back";
    return {true, true};
  }
  // branch-gate: BG-1021
  if (action == InputAction::MenuConfirm) {
    launchProductNewWorld(context.options, worldSetupDraft, frontend,
                          context.activeSession, window);
    return {true, true};
  }
  frontend.status = "new_world_input_ignored";
  return {true, false};
}

}  // namespace iggy3d

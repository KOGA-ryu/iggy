#include "app/iggy3d/menu/ActionHandlers.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/world/DungeonDraft.hpp"
#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/room_editor/AuthoringController.hpp"
#include "app/iggy3d/save/Flow.hpp"
#include "app/iggy3d/automation/Automation.hpp"
#include "app/iggy3d/automation/AutomationRoomEditing.hpp"

namespace iggy3d {
namespace {

using DevToolsBackHandler = void (*)(FrontendState&, ProductAppWindowState&);
using StarterConfirmHandler = ProductMenuActionResult (*)(
    ProductStarterMenuActionContext);

struct DevToolsMenuActionConfig {
  std::string_view selectionChangedStatus;
  std::string_view confirmStatus;
  DevToolsBackHandler backHandler = nullptr;
};

struct StarterConfirmActionRow {
  FrontendAction action = FrontendAction::None;
  StarterConfirmHandler handler = nullptr;
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

FrontendAction nextStarterSelection(FrontendAction current, InputAction action) {
  const auto& actions = starterActionOrder();

  std::size_t index = 0;
  // branch-gate: BG-1022
  for (std::size_t i = 0; i < actions.size(); ++i) {
    // branch-gate: BG-1022
    if (actions[i] == current) {
      index = i;
      break;
    }
  }

  // branch-gate: BG-1022
  if (action == InputAction::MenuUp) {
    index = index == 0 ? actions.size() - 1 : index - 1;  // branch-gate: BG-1022
  } else if (action == InputAction::MenuDown) {  // branch-gate: BG-1022
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

ProductMenuActionResult applyGameplayMovementTuningAction(
    InputAction action,
    ProductAppWindowState& window,
    FrontendState& frontend) {
  // branch-gate: BG-1206
  if (action == InputAction::MenuConfirm) {
    window.gameplay.gameplayMovement.tuningSelectedField =
        nextProductGameplayMovementTuningField(
            window.gameplay.gameplayMovement.tuningSelectedField);
    window.gameplay.gameplayMovement.tuningStatus = "movement_tuning_field_selected";
    window.gameplay.gameplayMovement.tuningReasonCode = window.gameplay.gameplayMovement.tuningStatus;
    frontend.status = window.gameplay.gameplayMovement.tuningStatus;
    return {true, true};
  }
  // branch-gate: BG-1206
  if (action == InputAction::MenuLeft || action == InputAction::MenuRight) {
    // branch-gate: BG-1209
    const int direction = action == InputAction::MenuLeft ? -1 : 1;
    (void)adjustProductGameplayMovementTuning(
        window.gameplay.gameplayMovement.tuning,
        window.gameplay.gameplayMovement.tuningSelectedField,
        direction);
    window.gameplay.gameplayMovement.tuningStatus = "movement_tuning_adjusted";
    window.gameplay.gameplayMovement.tuningReasonCode = window.gameplay.gameplayMovement.tuningStatus;
    frontend.status = window.gameplay.gameplayMovement.tuningStatus;
    return {true, true};
  }
  return {false, false};
}

void closeDevOverlayToGameplay(FrontendState& frontend,
                               ProductAppWindowState& window) {
  closeProductOverlayToGameplayTransition(frontend, window);
}

void closeStarterDevTools(FrontendState& frontend, ProductAppWindowState&) {
  frontend.childScreen = FrontendScreen::Gameplay;
  frontend.status = "dev_tools_closed";
}

void openStarterDevTools(FrontendState& frontend, ProductAppWindowState& window) {
  frontend.childScreen = FrontendScreen::StarterDevTools;
  frontend.devToolsCategory = FrontendDevToolsCategory::Session;
  frontend.status = "opening_menu_dev_tools_selected";
  syncProductWindowInputOwnerFromActiveSurface(frontend, window);
}

ProductMenuActionResult applyProductDevToggleMenuAction(
    FrontendState& frontend,
    ProductAppWindowState& window) {
  // branch-gate: BG-1124
  if (frontend.screen == FrontendScreen::DevOverlay) {
    closeProductOverlayToGameplayTransition(frontend, window);
    return {true, true};
  }
  // branch-gate: BG-1124
  if (frontend.screen == FrontendScreen::Gameplay && window.gameplay.gameplayActive) {
    openProductPauseDevToolsTransition(frontend, window,
                                       FrontendDevToolsCategory::Session);
    return {true, true};
  }
  // branch-gate: BG-1124
  if (frontend.screen == FrontendScreen::Starter &&
      frontend.childScreen == FrontendScreen::StarterDevTools) {
    closeStarterDevTools(frontend, window);
    return {true, true};
  }
  // branch-gate: BG-1124
  if (frontend.screen == FrontendScreen::Starter) {
    openStarterDevTools(frontend, window);
    return {true, true};
  }
  return {true, false};
}

bool resolvedSurfaceAcceptsGameplayInput(const FrontendState& frontend,
                                         const ProductAppWindowState& window) {
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  return window.gameplay.gameplayActive &&
         surface.activeSurface == ProductFrontendSurface::Gameplay &&
         surface.inputOwner == MenuOwner::Gameplay &&
         !surface.gameplayInputSuppressed;
}

bool canToggleDevDebugOverlay(const FrontendState& frontend,
                              const ProductAppWindowState& window) {
  return resolvedSurfaceAcceptsGameplayInput(frontend, window);
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

char selectedDungeonDraftGlyph(const ProductAppWindowState& window) {
  // branch-gate: BG-1147
  if (window.worldSetup.dungeonDraftSelectedGlyph.empty()) {
    return '.';
  }
  return window.worldSetup.dungeonDraftSelectedGlyph.front();
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
        startProductRoomAuthoringFromActiveRoom({activeRoom(window)});
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
  if (frontend.selectedAction == FrontendAction::LeaveEditor) {  // branch-gate: BG-1067
    const bool left =
        recordProductRoomEditingLeave(frontend, window, "pause_leave_editor");
    frontend.status =
        left ? "pause_leave_editor_requested" : "pause_leave_editor_failed";  // branch-gate: BG-1067
    // branch-gate: BG-1017
    if (left) {
      closeProductOverlayToGameplayTransition(frontend, window);
    }
    return {true, left};
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
                                frontend, context.activeSession, window,
                                context.settings, context.creativeApp);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::SaveAndExit) {
    executeProductPauseSaveFlow(ProductPauseSaveFlowKind::SaveAndExit,
                                context.options, frontend,
                                context.activeSession, window,
                                context.settings, context.creativeApp);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::ReturnToTitle) {
    returnProductToTitleTransition(frontend, window, context.settings);
    if (context.creativeApp != nullptr) {
      creative::clearCreativeUndoStack(context.creativeApp->undoStack);
      context.window.creativeUndo.available = false;
      context.window.creativeUndo.depth = 0;
      context.creativeApp->identity.clear();
    }
    context.activeSession.reset();
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::ExitGame) {
    frontend.status = "pause_exit_game_requested";
    context.closeRequested = true;
    return {true, true};
  }
  // sd3: complete the pause Load row. Open the save browser as a Pause-OWNED child overlay:
  // set only childScreen (screen stays Pause), so pauseChildDecision routes it to the
  // Pause-owned SaveSelector; MenuBack (childScreen->Gameplay) then returns to the pause menu.
  // Set the mode enum AND its string mirror in lockstep so no stale Delete residue leaks in.
  // Matches confirmStarterLoadSave's idiom (which keeps screen=Starter).
  if (frontend.selectedAction == FrontendAction::LoadSave) {
    frontend.childScreen = FrontendScreen::LoadSave;
    frontend.saveBrowserMode = FrontendSaveBrowserMode::Load;
    window.saveSession.saveSlotBrowserMode =
        std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
    initializeSelectedProductSaveSlot(context.saves.slots, window);
    frontend.status = "pause_load_save_opened";
    return {true, true};
  }
  frontend.status = "pause_action_selected";
  return {true, true};
}

ProductMenuActionResult confirmStarterContinue(ProductStarterMenuActionContext context) {
  FrontendState& frontend = context.frontend;
  ProductAppWindowState& window = context.window;

  if (context.saves.slots.compatibleCount == 0) {
    frontend.status = "opening_menu_action_disabled";
    return {true, true};
  }
  clearProductGameplayMovementTuning(window);
  launchProductContinueSave(
      context.options, productWorldTemplateFromOptions(context.options),
      context.saves, frontend, context.activeSession, window);
  return {true, true};
}

ProductMenuActionResult confirmStarterExit(ProductStarterMenuActionContext context) {
  clearProductGameplayMovementTuning(context.window);
  context.frontend.status = "opening_menu_exit_requested";
  context.closeRequested = true;
  return {true, true};
}

ProductMenuActionResult confirmStarterNewWorld(
    ProductStarterMenuActionContext context) {
  clearProductGameplayMovementTuning(context.window);
  context.frontend.childScreen = FrontendScreen::NewWorld;
  context.frontend.selectedAction = FrontendAction::CreateAndEnter;
  context.frontend.status = "opening_menu_new_world_selected";
  recordWorldSetupDraftState(context.worldSetupDraft, context.window);
  context.window.worldSetup.status = "world_setup_open";
  return {true, true};
}

ProductMenuActionResult confirmStarterCreativeNewWorld(
    ProductStarterMenuActionContext context) {
  clearProductGameplayMovementTuning(context.window);
  if (context.creativeApp == nullptr) {
    context.window.launchStatus = "product_creative_world_facade_missing";
    context.frontend.status = "product_creative_world_facade_missing";
    return {true, true};
  }

  ProductCreativeNewWorldLaunchRequest request;
  request.title = context.worldSetupDraft.worldName;
  request.templateId = "empty";
  request.requestedAtUtc = productSaveTimestampNowUtc();

  const ProductCreativeNewWorldLaunchResult launched =
      launchProductCreativeNewWorld(context.options,
                                    request,
                                    context.frontend,
                                    context.activeSession,
                                    context.window,
                                    *context.creativeApp);
  if (!launched.accepted) {
    context.frontend.status = "opening_menu_creative_new_world_failed";
    return {true, true};
  }

  const ProductWorldTemplate productWorld =
      productWorldTemplateFromOptions(context.options);
  context.saves = scanProductSaves(context.options.saveRoot,
                                   productWorld.packageId,
                                   productWorld.scenarioId);
  return {true, true};
}

ProductMenuActionResult confirmStarterCreativeOpenWorld(
    ProductStarterMenuActionContext context) {
  clearProductGameplayMovementTuning(context.window);
  if (context.creativeApp == nullptr) {
    context.window.launchStatus = "product_creative_world_facade_missing";
    context.frontend.status = "product_creative_world_facade_missing";
    return {true, true};
  }

  const ProductSaveBridgeResult creativeSaves =
      scanProductSaves(context.options.saveRoot,
                       "iggy3d.creative",
                       "creative.document");
  const ProductCreativeWorldSelectionResult selection =
      selectCreativeWorldContinueSave(creativeSaves.catalog.catalog);
  if (!selection.selected) {
    context.window.launchStatus = selection.reasonCode;
    context.frontend.status = "opening_menu_creative_open_world_unavailable";
    return {true, true};
  }

  ProductCreativeOpenWorldLaunchRequest request;
  request.saveId = selection.selectedSaveId;
  const ProductCreativeOpenWorldLaunchResult opened =
      launchProductCreativeOpenWorld(context.options,
                                     request,
                                     context.frontend,
                                     context.activeSession,
                                     context.window,
                                     *context.creativeApp);
  if (!opened.accepted) {
    context.frontend.status = "opening_menu_creative_open_world_failed";
    return {true, true};
  }
  return {true, true};
}

ProductMenuActionResult confirmStarterLoadSave(
    ProductStarterMenuActionContext context) {
  clearProductGameplayMovementTuning(context.window);
  context.frontend.childScreen = FrontendScreen::LoadSave;
  context.frontend.saveBrowserMode = FrontendSaveBrowserMode::Load;
  context.window.saveSession.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(context.frontend.saveBrowserMode));
  initializeSelectedProductSaveSlot(context.saves.slots, context.window);
  context.frontend.status = "opening_menu_load_save_selected";
  return {true, true};
}

ProductMenuActionResult confirmStarterDelete(ProductStarterMenuActionContext context) {
  clearProductGameplayMovementTuning(context.window);
  context.frontend.childScreen = FrontendScreen::LoadSave;
  context.frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
  context.frontend.selectedAction = FrontendAction::Delete;
  context.window.saveSession.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(context.frontend.saveBrowserMode));
  initializeSelectedProductSaveSlot(context.saves.slots, context.window);
  context.frontend.status = "delete_world_browser_open";
  return {true, true};
}

ProductMenuActionResult confirmStarterSettings(
    ProductStarterMenuActionContext context) {
  clearProductGameplayMovementTuning(context.window);
  context.frontend.childScreen = FrontendScreen::Settings;
  context.settingsTab = FrontendSettingsTab::Input;
  context.frontend.status = "opening_menu_settings_selected";
  return {true, true};
}

ProductMenuActionResult confirmStarterDevTools(
    ProductStarterMenuActionContext context) {
  clearProductGameplayMovementTuning(context.window);
  context.frontend.childScreen = FrontendScreen::StarterDevTools;
  context.frontend.devToolsCategory = FrontendDevToolsCategory::Session;
  context.frontend.status = "opening_menu_dev_tools_selected";
  return {true, true};
}

ProductMenuActionResult handleStarterConfirm(ProductStarterMenuActionContext context) {
  static constexpr std::array kStarterConfirmActions{
      StarterConfirmActionRow{FrontendAction::Continue, confirmStarterContinue},
      StarterConfirmActionRow{FrontendAction::Exit, confirmStarterExit},
      StarterConfirmActionRow{FrontendAction::NewWorld, confirmStarterNewWorld},
      StarterConfirmActionRow{FrontendAction::CreativeNewWorld,
                              confirmStarterCreativeNewWorld},
      StarterConfirmActionRow{FrontendAction::CreativeOpenWorld,
                              confirmStarterCreativeOpenWorld},
      StarterConfirmActionRow{FrontendAction::LoadSave, confirmStarterLoadSave},
      StarterConfirmActionRow{FrontendAction::Delete, confirmStarterDelete},
      StarterConfirmActionRow{FrontendAction::Settings, confirmStarterSettings},
      StarterConfirmActionRow{FrontendAction::DevTools, confirmStarterDevTools},
  };

  for (const StarterConfirmActionRow& row : kStarterConfirmActions) {
    if (context.frontend.selectedAction == row.action) {
      return row.handler(context);
    }
  }
  context.frontend.status = "opening_menu_action_selected";
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
  // branch-gate: BG-1206
  if (context.settingsTab == FrontendSettingsTab::Gameplay) {
    const ProductMenuActionResult tuning =
        applyGameplayMovementTuningAction(action, context.window, frontend);
    // branch-gate: BG-1206
    if (tuning.handled) {
      return tuning;
    }
  }
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
    executeProductSaveSoftDelete(context.options, context.saves, context.window,
                                 context.frontend);
    return {true, true};
  }
  return {true, false};
}

ProductMenuActionResult applyProductLoadSaveMenuAction(
    InputAction action,
    ProductLoadSaveMenuActionContext context) {
  FrontendState& frontend = context.frontend;
  context.window.saveSession.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  // branch-gate: BG-1020
  if (action == InputAction::MenuBack) {
    frontend.childScreen = FrontendScreen::Gameplay;
    frontend.status = frontend.saveBrowserMode == FrontendSaveBrowserMode::Delete
                          ? "delete_world_browser_closed"
                          : "load_save_closed";
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
    if (frontend.saveBrowserMode == FrontendSaveBrowserMode::Delete) {
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
    window.worldSetup.dungeonDraftEditMode =
        !window.worldSetup.dungeonDraftEditMode;
    worldSetupDraft.selectedField =
        window.worldSetup.dungeonDraftEditMode ? WorldSetupField::AsciiRoom  // branch-gate: BG-1021
                                              : WorldSetupField::Create;
    window.worldSetup.dungeonDraftStatus =
        window.worldSetup.dungeonDraftEditMode ? "dungeon_draft_edit_mode_on"
                                              : "dungeon_draft_edit_mode_off";  // branch-gate: BG-1021
    window.worldSetup.dungeonDraftReasonCode =
        window.worldSetup.dungeonDraftStatus;
    resetDungeonDraftWindowCursor(worldSetupDraft, window);
    frontend.status = window.worldSetup.dungeonDraftStatus;
    return {true, true};
  }

  const std::optional<ProductDungeonDraftDirection> direction =
      dungeonDraftDirectionForAction(action);
  // branch-gate: BG-1021
  if (window.worldSetup.dungeonDraftEditMode && direction.has_value()) {
    const ProductDungeonDraftOperationResult moved =
        moveProductDungeonDraftCursor(worldSetupDraft,
                                      dungeonDraftCursorFromWindow(window),
                                      *direction);
    recordDungeonDraftOperation(window, moved);
    frontend.status = moved.status;
    return {true, true};
  }
  // branch-gate: BG-1021
  if (!window.worldSetup.dungeonDraftEditMode && direction.has_value()) {
    const bool previous = isPreviousBuiltinDungeonAction(action);
    const bool changed = previous
                             ? selectPreviousProductBuiltinDungeon(worldSetupDraft)
                             : selectNextProductBuiltinDungeon(worldSetupDraft);  // branch-gate: BG-1021
    window.worldSetup.dungeonDraftModified = false;
    window.worldSetup.dungeonDraftEditMode = false;
    resetDungeonDraftWindowCursor(worldSetupDraft, window);
    recordWorldSetupDraftState(worldSetupDraft, window);
    frontend.status =
        changed ? "new_world_dungeon_selection_changed" : "new_world_input_ignored";  // branch-gate: BG-1021
    window.worldSetup.status =
        changed ? "world_setup_dungeon_selected" : "world_setup_dungeon_unavailable";  // branch-gate: BG-1021
    return {true, true};
  }
  // branch-gate: BG-1021
  if (action == InputAction::MenuBack) {
    frontend.childScreen = FrontendScreen::Gameplay;
    frontend.selectedAction = FrontendAction::NewWorld;
    frontend.status = "new_world_closed";
    window.worldSetup.status = "world_setup_back";
    return {true, true};
  }
  // branch-gate: BG-1021
  if (action == InputAction::MenuConfirm) {
    // branch-gate: BG-1147
    if (window.worldSetup.dungeonDraftEditMode) {
      const bool painted = applyDungeonDraftPaintGlyph(
          worldSetupDraft, window, selectedDungeonDraftGlyph(window));
      frontend.status = window.worldSetup.dungeonDraftStatus;
      return {true, painted};
    }
    launchProductNewWorld(context.options, worldSetupDraft, frontend,
                          context.activeSession, window);
    // Close the in-window new-world hole (sd1): the durable initial save just written is
    // invisible to the loop's pre-window scan, so refresh the loop-local catalog here the same
    // way soft-delete does. Unconditional is truthful — on a failed launch the durable state is
    // unchanged so the scan equals the prior catalog.
    const ProductWorldTemplate newWorld =
        productWorldTemplateFromOptions(context.options);
    context.saves = scanProductSaves(context.options.saveRoot, newWorld.packageId,
                                     newWorld.scenarioId);
    return {true, true};
  }
  frontend.status = "new_world_input_ignored";
  return {true, false};
}

ProductMenuActionResult applyProductStarterMenuAction(
    InputAction action,
    ProductStarterMenuActionContext context) {
  FrontendState& frontend = context.frontend;
  // branch-gate: BG-1022
  if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
    frontend.selectedAction = nextStarterSelection(frontend.selectedAction, action);
    frontend.status = "opening_menu_selection_changed";
    return {true, true};
  }
  // branch-gate: BG-1022
  if (action == InputAction::MenuBack) {
    frontend.status = "opening_menu_back_requested";
    context.closeRequested = true;
    return {true, true};
  }
  // branch-gate: BG-1022
  if (action != InputAction::MenuConfirm) {
    return {true, false};
  }
  return handleStarterConfirm(context);
}

ProductMenuActionResult applyProductGameplayMapMakerToggleAction(
    InputAction action,
    ProductGameplayMapMakerToggleActionContext context) {
  // branch-gate: BG-1205
  if (action == InputAction::MapMakerToggle) {
    if (productCreativeWorldActiveForSource(context.window,
                                            context.creativeApp)) {
      context.frontend.status = "map_maker_toggle_creative_world_active";
      context.window.viewport.mapMakerStatus =
          "map_maker_creative_world_active";
      context.window.viewport.mapMakerReasonCode =
          context.window.viewport.mapMakerStatus;
      context.window.viewport.creativeFlyActive = false;
      context.window.viewport.creativeFlyStatus = "creative_fly_not_requested";
      context.window.viewport.creativeFlyReasonCode =
          context.window.viewport.creativeFlyStatus;
      context.window.viewport.creativeFlySpeedMetersPerSecond = 0.0F;
      return {true, false};
    }
    // branch-gate: BG-1205
    if (!resolvedSurfaceAcceptsGameplayInput(context.frontend, context.window)) {
      clearProductMapMakerMode(context.window);
      context.frontend.status = "map_maker_toggle_ignored";
      context.window.viewport.mapMakerStatus = "map_maker_gameplay_inactive";
      context.window.viewport.mapMakerReasonCode =
          context.window.viewport.mapMakerStatus;
      return {true, false};
    }
    const bool enable =
        !productMapMakerLiveForSource(context.frontend,
                                      context.window,
                                      context.creativeApp);
    // branch-gate: BG-1205
    context.window.inputDevice.interactionMode =
        enable ? ProductInteractionMode::Creative : ProductInteractionMode::Player;
    syncProductWindowInputOwnerFromActiveSurface(context.frontend, context.window);
    // branch-gate: BG-1205
    context.window.viewport.mapMakerStatus =
        enable ? "map_maker_enabled" : "map_maker_disabled";
    context.window.viewport.mapMakerReasonCode =
        context.window.viewport.mapMakerStatus;
    context.window.viewport.creativeFlyActive = enable;
    // branch-gate: BG-1205
    if (!enable) {
      context.window.viewport.creativeFlyStatus = "creative_fly_not_requested";
      context.window.viewport.creativeFlyReasonCode =
          context.window.viewport.creativeFlyStatus;
      context.window.viewport.creativeFlySpeedMetersPerSecond = 0.0F;
    }
    context.frontend.status = context.window.viewport.mapMakerStatus;
    return {true, true};
  }

  return {false, false};
}

ProductMenuActionResult applyProductSystemPauseMenuAction(
    InputAction action,
    ProductSystemPauseMenuActionContext context) {
  if (action == InputAction::DevDebugOverlay) {  // branch-gate: BG-1124
    // branch-gate: BG-1124
    if (!canToggleDevDebugOverlay(context.frontend, context.window)) {
      context.frontend.status = "debug_overlay_gameplay_inactive";
      return {true, false};
    }
    // branch-gate: BG-1124
    if (context.settings == nullptr) {
      return {true, false};
    }
    context.settings->debugOverlayEnabled = !context.settings->debugOverlayEnabled;
    context.frontend.status = context.settings->debugOverlayEnabled
                                  ? "debug_overlay_enabled"
                                  : "debug_overlay_disabled";
    return {true, true};
  }
  if (action == InputAction::DevCollisionOverlay) {  // branch-gate: BG-1124
    context.window.debugHud.devCollisionOverlay.visible =
        !context.window.debugHud.devCollisionOverlay.visible;
    context.window.debugHud.devCollisionOverlay.status =
        context.window.debugHud.devCollisionOverlay.visible
            ? "dev_collision_overlay_enabled"
            : "dev_collision_overlay_hidden";
    context.window.debugHud.devCollisionOverlay.reasonCode =
        context.window.debugHud.devCollisionOverlay.status;
    context.frontend.status =
        context.window.debugHud.devCollisionOverlay.status;
    return {true, true};
  }
  // branch-gate: BG-1124
  if (action == InputAction::DevToggle || action == InputAction::SystemDevTools) {
    return applyProductDevToggleMenuAction(context.frontend, context.window);
  }
  // branch-gate: BG-1023
  if (action != InputAction::SystemPause) {
    return {false, false};
  }
  FrontendState& frontend = context.frontend;
  ProductAppWindowState& window = context.window;
  // branch-gate: BG-1023
  if (frontend.screen == FrontendScreen::Gameplay && window.gameplay.gameplayActive) {
    const bool preserveCreativeMode =
        productCreativeDocumentEditorActiveForSource(window,
                                                     context.creativeApp);
    openProductPauseTransition(frontend, window, FrontendAction::Resume);
    if (preserveCreativeMode) {
      window.inputDevice.interactionMode = ProductInteractionMode::Creative;
    }
    frontend.status = "pause_opened_from_gameplay";
    return {true, true};
  }
  // branch-gate: BG-1023
  if (frontend.screen == FrontendScreen::Pause) {
    closeProductOverlayToGameplayTransition(frontend, window);
    return {true, true};
  }
  // branch-gate: BG-1023
  if (frontend.screen == FrontendScreen::DevOverlay) {
    closeProductOverlayToGameplayTransition(frontend, window);
    return {true, true};
  }
  // branch-gate: BG-1023
  if (frontend.screen == FrontendScreen::Settings &&
      frontend.childScreen == FrontendScreen::Pause) {
    openProductPauseTransition(frontend, window, FrontendAction::Settings);
    return {true, true};
  }
  frontend.status = "opening_menu_pause_back_requested";
  context.closeRequested = true;
  return {true, true};
}

}  // namespace iggy3d

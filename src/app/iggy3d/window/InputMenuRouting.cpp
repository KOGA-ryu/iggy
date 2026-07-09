#include "app/iggy3d/window/InputFrame.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/save/SaveSlotOperations.hpp"
#include "app/iggy3d/view/OpeningMenuHitTest.hpp"
#include "app/iggy3d/window/MouseCapturePolicy.hpp"
#include "app/input/ActionState.hpp"
#include "app/platform/SdlWindow.hpp"
#include "app/iggy3d/window/InputFrameStages.hpp"

#include <array>

namespace iggy3d {
namespace {

struct ProductWindowFunctionKeyBinding {
  bool SdlWindowEventState::* pressed = nullptr;
  InputAction action = InputAction::None;
  bool KeyboardInputState::* wasDown = nullptr;
};

using ProductWindowTopLevelToggleHandler = ProductWindowTopLevelToggleResult (*)(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp);

struct ProductWindowTopLevelToggleRow {
  InputAction action = InputAction::None;
  ProductWindowTopLevelToggleHandler handler = nullptr;
  const char* eligibility = "";
};

ProductWindowTopLevelToggleResult dispatchProductWindowSystemToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp);

static constexpr std::array kProductWindowFunctionKeyBindings{
    ProductWindowFunctionKeyBinding{&SdlWindowEventState::f3Pressed,
                                    InputAction::DevDebugOverlay,
                                    &KeyboardInputState::debugOverlayWasDown},
    ProductWindowFunctionKeyBinding{
        &SdlWindowEventState::f4Pressed,
        InputAction::MovementTuningToggle,
        &KeyboardInputState::movementTuningToggleWasDown},
    ProductWindowFunctionKeyBinding{&SdlWindowEventState::f1Pressed,
                                    InputAction::DevToggle,
                                    &KeyboardInputState::devToggleWasDown},
    ProductWindowFunctionKeyBinding{
        &SdlWindowEventState::f2Pressed,
        InputAction::DevCollisionOverlay,
        &KeyboardInputState::devCollisionOverlayWasDown},
    ProductWindowFunctionKeyBinding{&SdlWindowEventState::mPressed,
                                    InputAction::MapMakerToggle,
                                    &KeyboardInputState::mapMakerToggleWasDown},
};

static constexpr std::array kProductWindowTopLevelToggleRows{
    ProductWindowTopLevelToggleRow{InputAction::DevDebugOverlay,
                                   dispatchProductWindowSystemToggleAction,
                                   "gameplay_owned"},
    ProductWindowTopLevelToggleRow{InputAction::MovementTuningToggle,
                                   dispatchProductWindowMovementTuningToggleAction,
                                   "gameplay_owned"},
    ProductWindowTopLevelToggleRow{InputAction::DevToggle,
                                   dispatchProductWindowSystemToggleAction,
                                   "screen_aware_overlay"},
    ProductWindowTopLevelToggleRow{InputAction::DevCollisionOverlay,
                                   dispatchProductWindowSystemToggleAction,
                                   "screen_aware_overlay"},
    ProductWindowTopLevelToggleRow{InputAction::MapMakerToggle,
                                   dispatchProductWindowMapMakerToggleAction,
                                   "gameplay_owned"},
};

bool productWindowFocused(const SdlWindow* sdlWindow) {
  return sdlWindow == nullptr || sdlWindow->eventState().focused;
}

void recordProductMouseCaptureResult(ProductAppWindowState& window,
                                     const ProductMouseCapturePolicy& policy,
                                     const SdlMouseCaptureResult* platform) {
  window.inputDevice.mouseCapture.requested = policy.requested;
  window.inputDevice.mouseCapture.active = false;
  window.inputDevice.mouseCapture.status = policy.status;
  window.inputDevice.mouseCapture.reasonCode = policy.reasonCode;
  window.inputDevice.mouseCapture.mode = policy.mode;
  window.inputDevice.mouseCapture.inputOwner = policy.inputOwner;
  // branch-gate: BG-1076
  if (platform != nullptr) {
    window.inputDevice.mouseCapture.requested = platform->requested;
    window.inputDevice.mouseCapture.active = platform->active;
    window.inputDevice.mouseCapture.status = platform->status;
    window.inputDevice.mouseCapture.reasonCode = platform->reasonCode;
  }
}

}  // namespace

void updateProductWindowMouseCapture(const FrontendState& frontend,
                                     ProductAppWindowState& window,
                                     SdlWindow* sdlWindow,
                                     const creative::CreativeAppState*
                                         creativeApp) {
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  const ProductMouseCapturePolicy policy = buildProductMouseCapturePolicy({
      window.gameplay.gameplayActive,
      window.inputDevice.interactionMode,
      surface.inputOwner,
      surface.gameplayInputSuppressed,
      productWindowFocused(sdlWindow),
      sdlWindow != nullptr,
      productCreativeDocumentEditorActiveForSource(window, creativeApp),
      window.creativeAuthoring.creativeNavigateActive,
  });
  // branch-gate: BG-1076
  if (sdlWindow == nullptr) {
    recordProductMouseCaptureResult(window, policy, nullptr);
    return;
  }
  const SdlMouseCaptureResult platform =
      sdlWindow->setRelativeMouseMode(policy.requested);
  recordProductMouseCaptureResult(window, policy, &platform);
}

namespace {

ProductWindowTopLevelToggleResult dispatchProductWindowSystemToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp) {
  bool ignoredCloseRequested = false;
  bool& closeTarget =
      closeRequested == nullptr ? ignoredCloseRequested : *closeRequested;  // branch-gate: BG-1194
  const ProductMenuActionResult menuResult = applyProductSystemPauseMenuAction(
      action, {frontend, window, closeTarget, settings, creativeApp});
  return {menuResult.handled, menuResult.accepted, action};
}

struct OpeningMenuNavigationHitRow {
  OpeningMenuHitArea area = OpeningMenuHitArea::None;
  InputAction action = InputAction::None;
};

static constexpr std::array kOpeningMenuNavigationHitRows{
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::DevToolsBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::SettingsBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldCreate,
                                InputAction::MenuConfirm},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldPreviousDungeon,
                                InputAction::MenuLeft},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldNextDungeon,
                                InputAction::MenuRight},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::LoadSaveBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::DeleteConfirmConfirm,
                                InputAction::MenuConfirm},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::DeleteConfirmBack,
                                InputAction::MenuBack},
};

InputAction openingMenuNavigationActionFor(OpeningMenuHitArea area) {
  for (const OpeningMenuNavigationHitRow& row : kOpeningMenuNavigationHitRows) {
    // branch-gate: BG-1029
    if (row.area == area) {
      return row.action;
    }
  }
  return InputAction::None;
}

MouseClick productWindowClickForHitTest(MouseClick click,
                                        const SdlWindow* sdlWindow,
                                        std::uint32_t virtualWidth,
                                        std::uint32_t virtualHeight) {
  // branch-gate: BG-1123
  if (sdlWindow == nullptr) {
    return click;
  }
  const SdlWindowEventState& eventState = sdlWindow->eventState();
  return normalizeProductWindowMenuClick(click,
                                         eventState.windowWidth,
                                         eventState.windowHeight,
                                         virtualWidth,
                                         virtualHeight);
}

}  // namespace

MouseClick productWindowMenuClickForHitTest(MouseClick click,
                                            const SdlWindow* sdlWindow,
                                            std::uint32_t virtualWidth,
                                            std::uint32_t virtualHeight) {
  return productWindowClickForHitTest(click,
                                      sdlWindow,
                                      virtualWidth,
                                      virtualHeight);
}

void routeProductWindowMenuInput(InputAction inputAction,
                                 ActionState& actionState,
                                 ProductOpeningMenuInputContext context) {
  // branch-gate: BG-1029
  if (inputAction == InputAction::MenuBack &&
      cancelProductRoomEditorPendingPreviewFromBack(context.frontend,
                                                    context.window)) {
    recordAction(actionState,
                 InputAction::EditorCancelPreview,
                 true,
                 true,
                 false,
                 1.0F);
    return;
  }
  routeProductOpeningMenuInput(inputAction, actionState, context);
}

void initializeProductWindowInputFrameState(ProductWindowInputFrameState& state,
                                            ProductAppWindowState& window) {
  initializeGamepadMenuState(state.gamepad);
  window.inputDevice.gamepadAvailable = state.gamepad.gamepadAvailable;
  window.inputDevice.gamepadName = state.gamepad.gamepadName;
  // branch-gate: BG-1029
  window.inputDevice.gamepadMapping =
      state.gamepad.gamepadAvailable ? "sdl_gamepad" : "unavailable";
}

void shutdownProductWindowInputFrameState(ProductWindowInputFrameState& state,
                                          SdlWindow* sdlWindow,
                                          ProductAppWindowState* window) {
  // branch-gate: BG-1076
  if (sdlWindow != nullptr) {
    const SdlMouseCaptureResult platform =
        sdlWindow->setRelativeMouseMode(false);
    // branch-gate: BG-1076
    if (window != nullptr) {
      ProductMouseCapturePolicy policy;
      policy.reasonCode = "mouse_capture_shutdown";
      recordProductMouseCaptureResult(*window, policy, &platform);
    }
  }
  shutdownGamepadMenuState(state.gamepad);
}

ProductWindowTopLevelToggleResult dispatchProductWindowTopLevelToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp) {
  for (const ProductWindowTopLevelToggleRow& row : kProductWindowTopLevelToggleRows) {
    // branch-gate: BG-1194
    if (row.action == action) {
      return row.handler(frontend, window, action, settings, closeRequested,
                         creativeApp);
    }
  }
  return {};
}

void dispatchProductOpeningMenuMouseHit(
    const OpeningMenuHitTestResult& hit,
    const MouseClick& click,
    ActionState& actionState,
    ProductOpeningMenuInputContext context) {
  const InputAction navigationAction = openingMenuNavigationActionFor(hit.area);
  // branch-gate: BG-1029
  if (navigationAction != InputAction::None) {
    routeProductOpeningMenuInput(navigationAction, actionState, context);
    return;
  }

  // branch-gate: BG-1029
  switch (hit.area) {
    case OpeningMenuHitArea::StarterAction:
      context.frontend.selectedAction = hit.action;
      recordAction(actionState,
                   mouseClickAction(click),
                   true,
                   true,
                   false,
                   1.0F);
      routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState,
                                   context);
      return;
    case OpeningMenuHitArea::DevToolsCategory:
      context.frontend.devToolsCategory = hit.devToolsCategory;
      context.frontend.status = "dev_tools_category_selected";
      return;
    case OpeningMenuHitArea::SettingsTab:
      context.settingsTab = hit.settingsTab;
      context.frontend.status = "settings_tab_selected";
      return;
    case OpeningMenuHitArea::LoadSaveSlot:
      // branch-gate: BG-1122
      if (hit.saveSlotIndex < context.saves.slots.slots.size()) {
        (void)selectProductSaveSlotById(
            context.saves.slots,
            context.saves.slots.slots[hit.saveSlotIndex].id,
            context.window);
        context.frontend.status = "load_save_selection_changed";
      }
      return;
    case OpeningMenuHitArea::LoadSaveLoad:
      context.frontend.saveBrowserMode = FrontendSaveBrowserMode::Load;
      context.window.saveSession.saveSlotBrowserMode =
          std::string(frontendSaveBrowserModeName(context.frontend.saveBrowserMode));
      routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState,
                                   context);
      return;
    case OpeningMenuHitArea::LoadSaveDelete:
      context.frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
      context.window.saveSession.saveSlotBrowserMode =
          std::string(frontendSaveBrowserModeName(context.frontend.saveBrowserMode));
      routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState,
                                   context);
      return;
    default:
      context.frontend.status = "opening_menu_hit_area_unhandled";
      return;
  }
}

MouseClick normalizeProductWindowMenuClick(MouseClick click,
                                           std::uint32_t windowWidth,
                                           std::uint32_t windowHeight,
                                           std::uint32_t virtualWidth,
                                           std::uint32_t virtualHeight) {
  // branch-gate: BG-1123
  if (!click.clicked || windowWidth == 0U || windowHeight == 0U ||
      virtualWidth == 0U || virtualHeight == 0U) {
    return click;
  }
  click.x = click.x * static_cast<float>(virtualWidth) /
            static_cast<float>(windowWidth);
  click.y = click.y * static_cast<float>(virtualHeight) /
            static_cast<float>(windowHeight);
  return click;
}

MouseClick resolveProductWindowInputMouseClick(
    ProductWindowInputClickOverride clickOverride,
    MouseInputState& mouse) {
  if (clickOverride.enabled) {
    return clickOverride.click;
  }
  return pollMouseClick(mouse);
}

InputAction productWindowFunctionKeyAction(const SdlWindowEventState& eventState) {
  for (const ProductWindowFunctionKeyBinding& binding :
       kProductWindowFunctionKeyBindings) {
    // branch-gate: BG-1194
    if (eventState.*(binding.pressed)) {
      return binding.action;
    }
  }
  return InputAction::None;
}

void recordProductWindowFunctionKeyKeyboardState(
    KeyboardInputState& keyboard,
    const SdlWindowEventState& eventState) {
  for (const ProductWindowFunctionKeyBinding& binding :
       kProductWindowFunctionKeyBindings) {
    // branch-gate: BG-1194
    if (eventState.*(binding.pressed)) {
      keyboard.*(binding.wasDown) = true;
    }
  }
}

}  // namespace iggy3d

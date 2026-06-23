#include "app/iggy3d/AppShell.hpp"

#include <iostream>
#include <array>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/DefaultWorldTemplate.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/SaveBridge.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "app/input/GamepadInput.hpp"
#include "app/input/KeyboardInput.hpp"
#include "app/input/MouseInput.hpp"
#include "render/RenderDiagnostics.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <chrono>
#include <thread>

#include <SDL3/SDL.h>

#include "app/iggy3d/OpeningMenuView.hpp"
#include "app/platform/SdlWindow.hpp"
#endif

namespace iggy3d {

namespace {

FrontendInputBackend settingsInputBackendFromOptions(ProductInputBackend backend) {
  switch (backend) {
    case ProductInputBackend::Keyboard:
      return FrontendInputBackend::Keyboard;
    case ProductInputBackend::Gamepad:
      return FrontendInputBackend::Gamepad;
    case ProductInputBackend::Auto:
      return FrontendInputBackend::Auto;
  }
  return FrontendInputBackend::Auto;
}

FrontendRendererChoice settingsRendererFromOptions(ProductRendererRequest renderer) {
  switch (renderer) {
    case ProductRendererRequest::Null:
      return FrontendRendererChoice::Null;
    case ProductRendererRequest::Vulkan:
      return FrontendRendererChoice::Vulkan;
  }
  return FrontendRendererChoice::Null;
}

FrontendWindowMode settingsWindowModeFromOptions(ProductWindowMode mode) {
  switch (mode) {
    case ProductWindowMode::NoWindow:
      return FrontendWindowMode::NoWindow;
    case ProductWindowMode::Window:
      return FrontendWindowMode::Window;
  }
  return FrontendWindowMode::NoWindow;
}

FrontendSettings productFrontendSettingsFromOptions(const ProductAppOptions& options) {
  FrontendSettings settings = defaultFrontendSettings();
  settings.inputBackend = settingsInputBackendFromOptions(options.inputBackend);
  settings.renderer = settingsRendererFromOptions(options.renderer);
  settings.windowMode = settingsWindowModeFromOptions(options.windowMode);
  settings.cameraMode = FrontendCameraMode::FirstPerson;
  settings.devToolsEnabled = true;
  settings.debugOverlayEnabled = true;
  return settings;
}

FrontendAction nextStarterSelection(FrontendAction current, InputAction action) {
  const std::array<FrontendAction, 6> actions = {
      FrontendAction::Continue,
      FrontendAction::NewWorld,
      FrontendAction::LoadSave,
      FrontendAction::Settings,
      FrontendAction::DevTools,
      FrontendAction::Exit,
  };

  std::size_t index = 0;
  for (std::size_t i = 0; i < actions.size(); ++i) {
    if (actions[i] == current) {
      index = i;
      break;
    }
  }

  if (action == InputAction::MenuUp) {
    index = index == 0 ? actions.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % actions.size();
  }
  return actions[index];
}

FrontendDevToolsCategory nextDevToolsSelection(FrontendDevToolsCategory current,
                                               InputAction action) {
  const auto& categories = devToolsCategoryOrder();
  std::size_t index = 0;
  for (std::size_t i = 0; i < categories.size(); ++i) {
    if (categories[i] == current) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp) {
    index = index == 0 ? categories.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % categories.size();
  }
  return categories[index];
}

FrontendSettingsTab nextSettingsSelection(FrontendSettingsTab current, InputAction action) {
  const auto& tabs = settingsTabOrder();
  std::size_t index = 0;
  for (std::size_t i = 0; i < tabs.size(); ++i) {
    if (tabs[i] == current) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp) {
    index = index == 0 ? tabs.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % tabs.size();
  }
  return tabs[index];
}

void applyOpeningMenuAction(FrontendState& frontend,
                            const ProductSaveBridgeResult& saves,
                            FrontendSettingsTab& settingsTab,
                            InputAction action,
                            bool& closeRequested) {
  if (action == InputAction::SystemPause) {
    frontend.status = "opening_menu_pause_back_requested";
    closeRequested = true;
    return;
  }

  if (frontend.childScreen == FrontendScreen::StarterDevTools) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      frontend.devToolsCategory = nextDevToolsSelection(frontend.devToolsCategory, action);
      frontend.status = "dev_tools_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.devToolsOpen = false;
      frontend.status = "dev_tools_closed";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      frontend.status = "dev_tools_category_selected";
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::Settings) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      settingsTab = nextSettingsSelection(settingsTab, action);
      frontend.status = "settings_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.status = "settings_closed";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      frontend.status = "settings_tab_selected";
      return;
    }
  }

  if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
    frontend.selectedAction = nextStarterSelection(frontend.selectedAction, action);
    frontend.status = "opening_menu_selection_changed";
    return;
  }
  if (action == InputAction::MenuBack) {
    frontend.status = "opening_menu_back_requested";
    closeRequested = true;
    return;
  }
  if (action != InputAction::MenuConfirm) {
    return;
  }

  if (frontend.selectedAction == FrontendAction::Continue && saves.slots.compatibleCount == 0) {
    frontend.status = "opening_menu_action_disabled";
    return;
  }
  if (frontend.selectedAction == FrontendAction::Exit) {
    frontend.status = "opening_menu_exit_requested";
    closeRequested = true;
    return;
  }
  if (frontend.selectedAction == FrontendAction::NewWorld) {
    frontend.status = "opening_menu_new_world_selected";
    return;
  }
  if (frontend.selectedAction == FrontendAction::LoadSave) {
    frontend.childScreen = FrontendScreen::LoadSave;
    frontend.status = "opening_menu_load_save_selected";
    return;
  }
  if (frontend.selectedAction == FrontendAction::Settings) {
    frontend.childScreen = FrontendScreen::Settings;
    settingsTab = FrontendSettingsTab::Input;
    frontend.status = "opening_menu_settings_selected";
    return;
  }
  if (frontend.selectedAction == FrontendAction::DevTools) {
    frontend.childScreen = FrontendScreen::StarterDevTools;
    frontend.devToolsOpen = true;
    frontend.devToolsCategory = FrontendDevToolsCategory::Session;
    frontend.status = "opening_menu_dev_tools_selected";
    return;
  }
  frontend.status = "opening_menu_action_selected";
}

void routeOpeningMenuInput(FrontendState& frontend,
                           const ProductSaveBridgeResult& saves,
                           FrontendSettingsTab& settingsTab,
                           ActionState& actionState,
                           InputAction inputAction,
                           ProductAppWindowState& window,
                           bool& closeRequested) {
  if (inputAction == InputAction::None) {
    return;
  }

  recordAction(actionState, inputAction, true, true, false, 1.0F);

  InputRoutingContext routingContext;
  routingContext.owners.starter = frontend.screen == FrontendScreen::Starter;
  const InputRoutingResult routed = routeInputAction(routingContext, inputAction);
  window.inputOwner = routed.owner;
  window.lastInputAction = routed.action;
  window.lastInputAccepted = routed.accepted;
  window.gameplayInputSuppressed = routed.gameplaySuppressed;
  if (routed.accepted) {
    applyOpeningMenuAction(frontend, saves, settingsTab, routed.action, closeRequested);
  }
}

ProductAppWindowState runOpeningMenuWindow(const ProductAppOptions& options,
                                           const ProductWorldTemplate& world,
                                           FrontendState& frontend,
                                           const ProductSaveBridgeResult& saves) {
  ProductAppWindowState window;
  window.requested = options.windowMode == ProductWindowMode::Window;
  window.inputOwner =
      frontend.screen == FrontendScreen::Starter ? MenuOwner::Starter : MenuOwner::None;
  window.gameplayInputSuppressed = frontend.screen == FrontendScreen::Starter;
  if (!window.requested) {
    return window;
  }

#if defined(IGGY3D_HAS_SDL3)
  window.sdlAvailable = true;

  SdlWindowCreateInfo createInfo;
  createInfo.title = "iggy3d - Opening Menu";
  createInfo.width = 1280;
  createInfo.height = 720;
  createInfo.resizable = true;
  createInfo.highDpi = true;
  createInfo.vulkan = false;

  SdlWindow sdlWindow(createInfo);
  window.created = sdlWindow.nativeWindow() != nullptr;
  window.drawable = sdlWindow.isDrawable();
  window.openingMenuVisible = window.created && frontend.screen == FrontendScreen::Starter;
  if (!window.created) {
    window.status = "window_create_failed";
    return window;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(sdlWindow.nativeWindow(), nullptr);
  if (renderer == nullptr) {
    window.status = "renderer_create_failed";
    return window;
  }

  sdlWindow.setTitle("iggy3d - Opening Menu");
  const auto start = std::chrono::steady_clock::now();
  KeyboardInputState keyboard;
  MouseInputState mouse;
  GamepadMenuState gamepad;
  initializeGamepadMenuState(gamepad);
  window.gamepadAvailable = gamepad.gamepadAvailable;
  window.gamepadName = gamepad.gamepadName;
  window.gamepadMapping = gamepad.gamepadAvailable ? "sdl_gamepad" : "unavailable";
  bool closeRequested = false;
  FrontendSettingsTab settingsTab = FrontendSettingsTab::Input;
  while (sdlWindow.isOpen()) {
    ActionState actionState;
    sdlWindow.pollEvents();
    ++window.eventPollCount;
    window.drawable = sdlWindow.isDrawable();

    routeOpeningMenuInput(frontend, saves, settingsTab, actionState,
                          pollKeyboardMenuAction(keyboard), window, closeRequested);

    const InputAction gamepadAction = pollGamepadMenuAction(gamepad);
    if (gamepadAction != InputAction::None) {
      window.gamepadMenuSelectUsed = true;
      routeOpeningMenuInput(frontend, saves, settingsTab, actionState, gamepadAction, window,
                            closeRequested);
    }

    const MouseClick click = pollMouseClick(mouse);
    if (click.clicked) {
      const OpeningMenuHitTestResult hit = openingMenuActionAt(frontend, click.x, click.y);
      if (hit.hit) {
        window.mouseMenuSelectUsed = true;
        if (hit.area == OpeningMenuHitArea::StarterAction) {
        frontend.selectedAction = hit.action;
          routeOpeningMenuInput(frontend, saves, settingsTab, actionState, mouseClickAction(click),
                                window, closeRequested);
        } else if (hit.area == OpeningMenuHitArea::DevToolsCategory) {
          frontend.devToolsCategory = hit.devToolsCategory;
          frontend.status = "dev_tools_category_selected";
        } else if (hit.area == OpeningMenuHitArea::SettingsTab) {
          settingsTab = hit.settingsTab;
          frontend.status = "settings_tab_selected";
        }
      }
    }

    if (window.drawable) {
      const OpeningMenuViewState view =
          drawOpeningMenuView(*renderer, options, world, frontend, settingsTab, saves);
      window.menuTextDrawn = window.menuTextDrawn || view.textDrawn;
      window.selectedRowDrawn = window.selectedRowDrawn || view.selectedRowDrawn;
      window.menuRowCount = view.rowCount;
    }
    ++window.framesPresented;

    if (options.frames > 0 && window.framesPresented >= options.frames) {
      break;
    }
    if (closeRequested) {
      break;
    }
    if (options.holdSeconds > 0) {
      const auto elapsed = std::chrono::steady_clock::now() - start;
      if (elapsed >= std::chrono::seconds(options.holdSeconds)) {
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  shutdownGamepadMenuState(gamepad);
  SDL_DestroyRenderer(renderer);
  window.selectedSettingsTab = settingsTab;
  window.status = window.menuTextDrawn ? "opening_menu_text_ready"
                                       : "opening_menu_window_ready";
  return window;
#else
  (void)world;
  (void)frontend;
  (void)saves;
  window.sdlAvailable = false;
  window.created = false;
  window.drawable = false;
  window.openingMenuVisible = false;
  window.status = "sdl3_unavailable";
  return window;
#endif
}

}  // namespace

int runProductApp(int argc, char** argv) {
  const ProductAppOptionsParseResult parsed = parseProductAppOptions(argc, argv);
  if (parsed.status == ProductAppOptionStatus::Help) {
    std::cout << productAppHelpText();
    return 0;
  }
  if (parsed.status != ProductAppOptionStatus::Ok) {
    RenderReceipt receipt;
    appendReceiptField(receipt, "app", "iggy3d");
    appendReceiptField(receipt, "result", "fail");
    appendReceiptField(receipt, "reason_code", productAppOptionStatusReason(parsed.status));
    appendReceiptField(receipt, "option", parsed.option);
    std::cout << formatRenderReceipt(receipt);
    return 2;
  }

  const ProductAppOptions& options = parsed.options;
  const ProductWorldTemplate world =
      options.devPackageOverride.empty()
          ? defaultProductWorldTemplate()
          : devOverrideProductWorldTemplate(options.devPackageOverride.generic_string(),
                                            options.devScenario);
  const ProductSaveBridgeResult saves =
      scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  const FrontendSettings settings = productFrontendSettingsFromOptions(options);

  FrontendState frontend;
  completeFrontendBoot(frontend, true, true);
  frontend.screen = FrontendScreen::Starter;
  frontend.selectedAction = saves.slots.compatibleCount > 0 ? FrontendAction::Continue
                                                            : FrontendAction::NewWorld;
  frontend.status = "opening_menu_ready";
  frontend.inputOwned = true;

  const ProductAppWindowState window = runOpeningMenuWindow(options, world, frontend, saves);

  if (options.printRenderReceipt) {
    std::cout << formatRenderReceipt(
        buildProductAppReceipt(options, world, frontend, settings, window, saves));
  }

  return window.requested && !window.created ? 77 : 0;
}

}  // namespace iggy3d

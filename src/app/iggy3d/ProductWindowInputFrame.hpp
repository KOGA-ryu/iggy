#pragma once

#include <optional>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/input/GamepadInput.hpp"
#include "app/input/KeyboardInput.hpp"
#include "app/input/MouseInput.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductSaveBridgeResult;
struct WorldSetupDraft;

struct ProductWindowInputFrameState {
  KeyboardInputState keyboard;
  MouseInputState mouse;
  GamepadMenuState gamepad;
};

struct ProductWindowInputFrameContext {
  FrontendState& frontend;
  const ProductSaveBridgeResult& saves;
  const ProductAppOptions& options;
  FrontendSettingsTab& settingsTab;
  std::optional<Session>& activeSession;
  WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState& window;
  const FrontendSettings& settings;
  ProductWindowInputFrameState& inputFrame;
  bool& closeRequested;
};

void initializeProductWindowInputFrameState(ProductWindowInputFrameState& state,
                                            ProductAppWindowState& window);
void shutdownProductWindowInputFrameState(ProductWindowInputFrameState& state);
void processProductWindowInputFrame(ProductWindowInputFrameContext context);

}  // namespace iggy3d

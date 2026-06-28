#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/input/ControllerActionRouting.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/view/ViewportFraming.hpp"
#include "app/input/GamepadInput.hpp"
#include "app/input/KeyboardInput.hpp"
#include "app/input/MouseInput.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

class SdlWindow;
struct SdlWindowEventState;
struct ProductSaveBridgeResult;
struct WorldSetupDraft;

struct ProductWindowInputFrameState {
  KeyboardInputState keyboard;
  MouseInputState mouse;
  GamepadMenuState gamepad;
  ProductControllerModeChordState controllerModeChord;
  ProductControllerActionRoutingState controllerAction;
};

struct ProductWindowInputFrameContext {
  FrontendState& frontend;
  const ProductSaveBridgeResult& saves;
  const ProductAppOptions& options;
  FrontendSettingsTab& settingsTab;
  std::optional<Session>& activeSession;
  WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState& window;
  FrontendSettings& settings;
  ProductWindowInputFrameState& inputFrame;
  bool& closeRequested;
  SdlWindow* sdlWindow = nullptr;
};

struct ProductControllerSampleInputContext {
  const FrontendState& frontend;
  ProductAppWindowState& window;
  Session* activeSession = nullptr;
  ProductControllerModeChordState& controllerModeChord;
  ProductControllerActionRoutingState& controllerAction;
  const FrontendSettings* settings = nullptr;
  std::string_view inputSource = "controller";
};

struct ProductControllerSampleInputResult {
  bool processed = false;
  bool actionApplied = false;
  bool actionAccepted = false;
  std::string_view status = "controller_sample_not_processed";
  std::string_view reasonCode = "controller_sample_not_processed";
};

struct ProductWindowEditorMousePickPreviewContext {
  const FrontendState& frontend;
  ProductAppWindowState& window;
  MouseClick click;
  ProductViewportFrameConfig viewportConfig;
  Vec3 anchorWorld;
};

struct ProductWindowEditorMousePickPreviewResult {
  bool handled = false;
  bool picked = false;
  bool previewBuilt = false;
  bool accepted = false;
  std::string status = "room_editor_mouse_pick_preview_not_requested";
  std::string reasonCode = "room_editor_mouse_pick_preview_not_requested";
};

void initializeProductWindowInputFrameState(ProductWindowInputFrameState& state,
                                            ProductAppWindowState& window);
MouseClick normalizeProductWindowMenuClick(MouseClick click,
                                           std::uint32_t windowWidth,
                                           std::uint32_t windowHeight,
                                           std::uint32_t virtualWidth = 1280U,
                                           std::uint32_t virtualHeight = 720U);
void shutdownProductWindowInputFrameState(ProductWindowInputFrameState& state,
                                          SdlWindow* sdlWindow = nullptr,
                                          ProductAppWindowState* window = nullptr);
void processProductWindowInputFrame(ProductWindowInputFrameContext context);
InputAction productWindowFunctionKeyAction(const SdlWindowEventState& eventState);
bool cancelProductRoomEditorPendingPreviewFromBack(ProductAppWindowState& window);
ProductControllerSampleInputResult processProductControllerActionSample(
    ProductControllerSampleInputContext context,
    GamepadControllerActionSample sample);
ProductWindowEditorMousePickPreviewResult processProductWindowEditorMousePickPreview(
    ProductWindowEditorMousePickPreviewContext context);

}  // namespace iggy3d

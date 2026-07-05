#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/iggy3d/creative/spatial/ViewportPick.hpp"
#include "app/iggy3d/input/ControllerActionRouting.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"
#include "app/iggy3d/creative/bridge/InputFrame.hpp"
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
struct OpeningMenuHitTestResult;
struct ProductOpeningMenuInputContext;
struct ProductUiDrawList;

namespace creative {
struct CreativeAppState;
}  // namespace creative

struct ProductMovementTuningRepeatPolicy {
  std::uint32_t initialDelayFrames = 12U;
  std::uint32_t repeatIntervalFrames = 4U;
};

struct ProductMovementTuningRepeatState {
  int heldDirection = 0;
  std::uint32_t heldFrames = 0U;
};

struct ProductWindowInputFrameState {
  KeyboardInputState keyboard;
  MouseInputState mouse;
  GamepadMenuState gamepad;
  ProductControllerModeChordState controllerModeChord;
  ProductControllerActionRoutingState controllerAction;
  ProductMovementTuningRepeatState movementTuningRepeat;
  // TL-3 held-button pointer gesture lifecycle (creative document mode only).
  ProductCreativePointerLifecycleState creativePointerLifecycle;
};

// Scripted pointer-lifecycle sample carried by an override (test/automation).
// When `enabled`, it drives the creative pointer-lifecycle resolver in place of
// the (absent-headless / real-under-override) SDL mouse, so an injected
// press-hold-release drag can flow through processProductWindowInputFrame.
struct ProductWindowInputPointerLifecycleOverride {
  bool enabled = false;
  bool primaryButtonDown = false;
  float x = 0.0F;
  float y = 0.0F;
};

struct ProductWindowInputClickOverride {
  bool enabled = false;
  MouseClick click;
  ProductWindowInputPointerLifecycleOverride pointerLifecycle;
};

struct ProductWindowInputFrameContext {
  FrontendState& frontend;
  ProductSaveBridgeResult& saves;
  const ProductAppOptions& options;
  FrontendSettingsTab& settingsTab;
  std::optional<Session>& activeSession;
  WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState& window;
  FrontendSettings& settings;
  ProductWindowInputFrameState& inputFrame;
  bool& closeRequested;
  SdlWindow* sdlWindow = nullptr;
  creative::CreativeAppState* creativeApp = nullptr;
  const ProductUiDrawList* creativeUiDrawList = nullptr;
  creative::CreativeViewportPickViewport creativeViewportPickViewport = {};
  creative::CreativeSpatialProjectionRequest creativeViewportPickProjectionRequest = {};
  std::int32_t creativeViewportPickZ = 0;
  creative::CreativeViewportPickDepthMode creativeViewportPickDepthMode =
      creative::CreativeViewportPickDepthMode::FixedZ;
  ProductWindowInputClickOverride clickOverride;
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

struct ProductMovementTuningInputResult {
  bool handled = false;
  bool accepted = false;
  std::string status = "movement_tuning_input_not_handled";
  std::string reasonCode = "movement_tuning_input_not_handled";
};

struct ProductWindowTopLevelToggleResult {
  bool handled = false;
  bool accepted = false;
  InputAction action = InputAction::None;
};

void initializeProductWindowInputFrameState(ProductWindowInputFrameState& state,
                                            ProductAppWindowState& window);
MouseClick normalizeProductWindowMenuClick(MouseClick click,
                                           std::uint32_t windowWidth,
                                           std::uint32_t windowHeight,
                                           std::uint32_t virtualWidth = 1280U,
                                           std::uint32_t virtualHeight = 720U);
MouseClick resolveProductWindowInputMouseClick(
    ProductWindowInputClickOverride clickOverride,
    MouseInputState& mouse);
void shutdownProductWindowInputFrameState(ProductWindowInputFrameState& state,
                                          SdlWindow* sdlWindow = nullptr,
                                          ProductAppWindowState* window = nullptr);
void dispatchProductOpeningMenuMouseHit(
    const OpeningMenuHitTestResult& hit,
    const MouseClick& click,
    ActionState& actionState,
    ProductOpeningMenuInputContext context);
void processProductWindowInputFrame(ProductWindowInputFrameContext context);
InputAction productWindowFunctionKeyAction(const SdlWindowEventState& eventState);
void recordProductWindowFunctionKeyKeyboardState(
    KeyboardInputState& keyboard,
    const SdlWindowEventState& eventState);
bool cancelProductRoomEditorPendingPreviewFromBack(
    const FrontendState& frontend,
    ProductAppWindowState& window);
ProductMovementTuningInputResult applyProductWindowMovementTuningInput(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action);
ProductWindowTopLevelToggleResult dispatchProductWindowTopLevelToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings = nullptr,
    bool* closeRequested = nullptr);
ProductMovementTuningInputResult applyProductWindowMovementTuningHeldInput(
    FrontendState& frontend,
    ProductAppWindowState& window,
    ProductMovementTuningRepeatState& repeat,
    bool leftDown,
    bool rightDown,
    ProductMovementTuningRepeatPolicy policy = {});
ProductControllerSampleInputResult processProductControllerActionSample(
    ProductControllerSampleInputContext context,
    GamepadControllerActionSample sample);
ProductControllerSampleInputResult applyProductWindowInputActions(
    const FrontendState& frontend,
    ProductAppWindowState& window,
    Session* activeSession,
    const FrontendSettings* settings,
    const ActionState& gameplayActions,
    std::string_view inputSource);
ProductWindowEditorMousePickPreviewResult processProductWindowEditorMousePickPreview(
    ProductWindowEditorMousePickPreviewContext context);

}  // namespace iggy3d

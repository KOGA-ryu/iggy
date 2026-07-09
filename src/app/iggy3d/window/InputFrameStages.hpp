#pragma once

#include <cstdint>

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/input/ControllerActionRouting.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/MouseInput.hpp"

namespace iggy3d {

class SdlWindow;
struct ProductWindowInputFrameContext;
struct ProductWindowTopLevelToggleResult;

struct ProductCreativeDocumentInputOrchestrationRequest {
  ProductWindowInputFrameContext& context;
  MouseClick click;
  bool higherPriorityMouseConsumed = false;
  bool frontendMouseOwnsInput = false;
};

struct ProductCreativeDocumentInputOrchestrationResult {
  MouseClick downstreamClick;
  creative::TargetRef pointerTarget;
  bool creativeDocumentActive = false;
  bool creativeDocumentInputHandled = false;
};

MouseClick productWindowMenuClickForHitTest(
    MouseClick click,
    const SdlWindow* sdlWindow,
    std::uint32_t virtualWidth = 1280U,
    std::uint32_t virtualHeight = 720U);

void routeProductWindowMenuInput(InputAction inputAction,
                                 ActionState& actionState,
                                 ProductOpeningMenuInputContext context);

ProductCreativeFlyResult applyProductWindowCreativeFlyActions(
    ProductAppWindowState& window,
    const Session* activeSession,
    const ActionState& actions);

ProductCreativeDocumentInputOrchestrationResult
processProductCreativeDocumentInputOrchestration(
    ProductCreativeDocumentInputOrchestrationRequest request);

ProductWindowTopLevelToggleResult dispatchProductWindowMapMakerToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp);

ProductWindowTopLevelToggleResult dispatchProductWindowMovementTuningToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp);

void applyProductWindowRoomEditorActions(ProductAppWindowState& window,
                                         const ActionState& actions);

ActionState gameplayActionsAfterMapMakerConsumesMovement(
    const ActionState& actions);

InputAction movementTuningHeldAdjustmentAction(bool leftDown, bool rightDown);

void recordProductWindowControllerActions(
    const FrontendState& frontend,
    ProductAppWindowState& window,
    ProductControllerActionRoutingState& controllerAction,
    const GamepadControllerActionSample& sample,
    bool controllerModeChordRequested,
    ActionState& actions,
    const creative::CreativeAppState* creativeApp);

void updateProductWindowMouseCapture(
    const FrontendState& frontend,
    ProductAppWindowState& window,
    SdlWindow* sdlWindow,
    const creative::CreativeAppState* creativeApp);

}  // namespace iggy3d

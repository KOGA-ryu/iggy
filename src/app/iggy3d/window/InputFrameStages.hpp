#pragma once

#include <cstdint>

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/MouseInput.hpp"

namespace iggy3d {

class SdlWindow;
struct ProductWindowInputFrameContext;

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

}  // namespace iggy3d

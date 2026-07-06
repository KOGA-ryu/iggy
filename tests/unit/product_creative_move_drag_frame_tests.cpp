// TV1-G window-plumbing coverage (TV1-F entry requirement (i)): drive a Move
// drag press -> hold -> release THROUGH processProductWindowInputFrame and
// assert the object actually moved to the snapped destination. The Press is
// injected via the click-override socket (headless has no real SDL mouse, so
// pollMouseClick can never synthesize a press); the Move/Release lifecycle is
// then driven through the RAW held-button path (override disabled) so the raw
// mouse -> resolver -> dispatch -> facade-commit seam is exercised end to end.

#include "app/iggy3d/window/InputFrame.hpp"

#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

iggy3d::FrontendState gameplayFrontend() {
  iggy3d::FrontendState frontend;
  iggy3d::enterFrontendGameplay(frontend, iggy3d::FrontendAction::NewWorld);
  return frontend;
}

void markCreativeDocumentWindow(iggy3d::ProductAppWindowState& window) {
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.activeCreative.saveId = "creative_save";
  window.activeCreative.worldId = "world_001";
  window.activeCreative.documentId = 42U;
  window.gameplayActive = true;
}

iggy3d::MouseClick clickAt(float x, float y) {
  iggy3d::MouseClick click;
  click.clicked = true;
  click.x = x;
  click.y = y;
  return click;
}

cr::CreativeViewportPickViewport viewport() {
  return {0.0F, 0.0F, 400.0F, 400.0F};
}

cr::CreativeSpatialProjectionRequest projectionRequest() {
  cr::CreativeSpatialProjectionRequest request;
  request.gridSize = {4, 4, 2};
  request.cellSize = 1.0;
  request.clampToGrid = true;
  request.includeAuthoringOnly = false;
  return request;
}

cr::CreativeObjectId createRoom(cr::Facade& facade) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Room";
  request.bounds.min = {1.0, 2.0, 1.0};
  request.bounds.max = {2.0, 3.0, 2.0};
  request.hasBoundsOverride = true;
  return facade.createDocumentObject(request).objectId;
}

struct WindowInputHarness {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppOptions options;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::Input;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::ProductAppWindowState window;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductWindowInputFrameState inputFrame;
  bool closeRequested = false;

  WindowInputHarness() {
    activeSession.emplace();
    markCreativeDocumentWindow(window);
  }

  void run(iggy3d::creative::CreativeAppState& app,
           iggy3d::ProductWindowInputClickOverride clickOverride) {
    iggy3d::processProductWindowInputFrame(iggy3d::ProductWindowInputFrameContext{
        frontend,
        saves,
        options,
        settingsTab,
        activeSession,
        worldSetupDraft,
        window,
        settings,
        inputFrame,
        closeRequested,
        nullptr,
        &app,
        nullptr,
        viewport(),
        projectionRequest(),
        1,
        iggy3d::creative::CreativeViewportPickDepthMode::FixedZ,
        clickOverride,
    });
  }
};

iggy3d::ProductWindowInputClickOverride lifecycleOverride(bool down,
                                                          float x,
                                                          float y) {
  iggy3d::ProductWindowInputClickOverride override;
  override.pointerLifecycle.enabled = true;
  override.pointerLifecycle.primaryButtonDown = down;
  override.pointerLifecycle.x = x;
  override.pointerLifecycle.y = y;
  return override;
}

// The payoff: a full press -> hold -> release drag through the window frame
// moves the room to the release destination, snapped, exactly once. The Press
// is an injected click (selects + begins drag); the hold/release are injected
// pointer-lifecycle samples driving the raw resolver -> dispatch -> commit.
bool windowFrameDragMovesRoomToReleaseDestination() {
  WindowInputHarness harness;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));

  const std::uint64_t revisionBefore = facade.document().revision();

  // Frame 1: inject the Press on the room's viewport cell (150,250 -> grid
  // (1,2)). The pick selects the room and the Move tool begins the drag.
  iggy3d::ProductWindowInputClickOverride press;
  press.enabled = true;
  press.click = clickAt(150.0F, 250.0F);
  harness.run(app, press);

  const bool beganOk =
      expect(facade.selectionState().selectedTarget.value == roomId,
             "window press selects room") &&
      expect(facade.toolState().moveDragActive,
             "window press begins drag");

  // Frame 2: injected held-button down at the destination pointer (250,150 ->
  // grid (2,1)). The resolver records the gesture in flight (Press edge).
  harness.run(app, lifecycleOverride(/*down=*/true, 250.0F, 150.0F));

  // Frame 3: injected button release at the same pointer. The resolver emits
  // Release; the window resolves the destination world XY from grid cell (2,1)
  // and the facade commits one snapped Move.
  harness.run(app, lifecycleOverride(/*down=*/false, 250.0F, 150.0F));

  const cr::CreativeObject* room = facade.findObject(roomId);
  return beganOk &&
         expect(room != nullptr, "window drag room exists") &&
         // TD-7 screen=XY drag: the object tracks the cursor. The press cell
         // was screen (1,2); the release cell is screen (2,1). World X follows
         // screen-horizontal (1 -> 2). World Y follows screen-vertical: the
         // pick maps a LARGER pointer-pixel-Y to a LARGER world Y (grid Y grows
         // downward with pixel Y — NOT inverted), and the cursor moved UP the
         // screen (pixel Y 250 -> 150), so world Y DECREASES 2 -> 1. The DEPTH
         // axis (world Z) holds the start anchor Z == 1.0.
         expect(room->bounds.min.x == 2.0, "window drag corner x tracks cursor") &&
         expect(room->bounds.min.y == 1.0, "window drag corner y tracks cursor") &&
         expect(room->bounds.min.z == 1.0, "window drag corner z unchanged") &&
         expect(!facade.toolState().moveDragActive,
                "window drag cleared after release") &&
         expect(facade.moveDragReceipt().outcome ==
                    cr::CreativeFacadeMoveDragOutcome::Applied,
                "window drag applied receipt") &&
         expect(facade.moveDragReceipt().committed,
                "window drag committed receipt") &&
         expect(facade.document().revision() == revisionBefore + 1U,
                "window drag bumps revision exactly once");
}

// TV1-F entry requirement (ii): a Release with no preceding Press (a drag
// interrupted by pause then resumed with the button still held) is a harmless
// no-op through the window frame — no crash, no spurious move.
bool windowFrameReleaseWithoutPressIsNoOp() {
  WindowInputHarness harness;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  const std::uint64_t revisionBefore = facade.document().revision();

  // The button is reported held (as if resumed mid-gesture)...
  harness.run(app, lifecycleOverride(/*down=*/true, 250.0F, 150.0F));
  // ...then released. No drag ever began, so nothing commits.
  harness.run(app, lifecycleOverride(/*down=*/false, 250.0F, 150.0F));

  const cr::CreativeObject* room = facade.findObject(roomId);
  return expect(room != nullptr && room->bounds.min.x == 1.0,
                "orphan window release room unmoved") &&
         expect(!facade.toolState().moveDragActive,
                "orphan window release no drag") &&
         expect(facade.document().revision() == revisionBefore,
                "orphan window release no revision bump");
}

}  // namespace

int main() {
  const bool ok = windowFrameDragMovesRoomToReleaseDestination() &&
                  windowFrameReleaseWithoutPressIsNoOp();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

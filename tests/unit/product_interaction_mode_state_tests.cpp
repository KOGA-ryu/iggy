#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/input/InteractionModeState.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/room_editor/EditingState.hpp"
#include "app/iggy3d/automation/AutomationRoomEditing.hpp"

#include <array>
#include <iostream>
#include <string_view>

#include "render/RenderDiagnostics.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductControllerModeChordSample fullChord() {
  return {
      true,
      true,
      true,
      true,
  };
}

iggy3d::ProductAsciiRoomAuthoringRequest smallRoomRequest() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "###\n"
      "#P#\n"
      "###\n";
  request.roomId = "interaction_mode_room";
  request.sourceName = "unit/interaction_mode_room.iggyroom.txt";
  request.emitAssetText = false;
  return request;
}

bool defaultStateIsPlayerMode() {
  const iggy3d::ProductAppWindowState window;
  return expect(window.inputDevice.interactionMode ==
                    iggy3d::ProductInteractionMode::Player,
                "default interaction mode is player") &&
         expect(!window.inputDevice.controllerModeToggle.requested,
                "default toggle not requested") &&
         expect(!window.inputDevice.controllerModeToggle.accepted,
                "default toggle not accepted") &&
         expect(window.inputDevice.controllerModeToggle.status ==
                    "interaction_mode_toggle_not_requested",
                "default toggle status") &&
         expect(window.inputDevice.controllerModeToggle.surface == "none",
                "default toggle surface");
}

bool roomEditingStartSetsCreativeMode() {
  iggy3d::ProductAppWindowState window;
  const iggy3d::ProductRoomEditingStartResult started =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest());
  iggy3d::recordProductRoomEditingStart(window, started, "unit_edit_room");

  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window,
                                     saves);

  return expect(started.ok, "room editing start accepted") &&
         expect(window.creativeAuthoring.roomEditing.ready, "room editing ready") &&
         expect(window.inputDevice.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "room editing start sets creative mode") &&
         expect(iggy3d::hasReceiptField(receipt, "interaction_mode", "creative"),
                "room editing start receipt interaction mode");
}

bool roomEditingLeaveReturnsPlayerModeAndPreservesActiveRoom() {
  iggy3d::FrontendState frontend;
  iggy3d::enterFrontendGameplay(frontend, iggy3d::FrontendAction::NewWorld);
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  const iggy3d::ProductRoomEditingStartResult started =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest());
  iggy3d::recordProductRoomEditingStart(window, started, "unit_edit_room");
  window.creativeAuthoring.roomEditorPreview.active = true;
  window.creativeAuthoring.roomEditorPreview.visible = true;
  window.creativeAuthoring.roomEditorOverlay.visible = true;
  window.creativeAuthoring.roomEditorHud.visible = true;
  window.viewport.productDrawRoomEditorCursorVisible = true;
  window.viewport.productDrawRoomEditorCursorCount = 1;
  const std::string activeRoomId = iggy3d::activeRoom(window).roomId;
  const std::uint64_t activeFloorCount = iggy3d::activeRoom(window).authoredFloorCount;
  const std::uint64_t activeWallCount = iggy3d::activeRoom(window).authoredWallCount;
  const std::uint64_t activeCollisionCount =
      iggy3d::activeRoomCollision(window).querySurfaceCount;

  const bool left =
      iggy3d::recordProductRoomEditingLeave(frontend, window, "unit_leave_editor");
  const iggy3d::ProductActiveSurfaceFrame surfaceAfterLeave =
      iggy3d::resolveProductActiveSurface(
          iggy3d::productActiveSurfaceContextForWindow(frontend, window));

  return expect(started.ok, "room editing start accepted for leave") &&
         expect(left, "room editing leave accepted") &&
         expect(!window.creativeAuthoring.roomEditing.ready, "room editing no longer ready") &&
         expect(window.creativeAuthoring.roomEditing.status == "product_room_editing_left",
                "room editing leave status") &&
         expect(window.inputDevice.interactionMode ==
                    iggy3d::ProductInteractionMode::Player,
                "room editing leave returns player mode") &&
         expect(surfaceAfterLeave.inputOwner == iggy3d::MenuOwner::Gameplay,
                "room editing leave returns gameplay owner") &&
         expect(!surfaceAfterLeave.gameplayInputSuppressed,
                "room editing leave unsuppresses gameplay") &&
         expect(!window.creativeAuthoring.roomEditorCursorReady, "room editor cursor no longer ready") &&
         expect(window.creativeAuthoring.roomEditorStatus == "room_editor_not_ready",
                "room editor leave status") &&
         expect(!window.creativeAuthoring.roomEditorOverlay.visible, "room editor overlay hidden") &&
         expect(!window.creativeAuthoring.roomEditorPreview.visible, "room editor preview hidden") &&
         expect(!window.creativeAuthoring.roomEditorPreview.active, "room editor preview inactive") &&
         expect(!window.creativeAuthoring.roomEditorHud.visible, "room editor hud hidden") &&
         expect(!window.viewport.productDrawRoomEditorCursorVisible,
                "room editor cursor draw hidden") &&
         expect(iggy3d::activeRoom(window).loaded, "active room still loaded") &&
         expect(iggy3d::activeRoom(window).roomId == activeRoomId, "active room id preserved") &&
         expect(iggy3d::activeRoom(window).authoredFloorCount == activeFloorCount,
                "active floor count preserved") &&
         expect(iggy3d::activeRoom(window).authoredWallCount == activeWallCount,
                "active wall count preserved") &&
         expect(iggy3d::activeRoomCollision(window).querySurfaceCount == activeCollisionCount,
                "active collision count preserved");
}

bool roomEditingLeaveRejectsWhenNotReady() {
  iggy3d::FrontendState frontend;
  iggy3d::enterFrontendGameplay(frontend, iggy3d::FrontendAction::NewWorld);
  iggy3d::ProductAppWindowState window;
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Player;

  const bool left =
      iggy3d::recordProductRoomEditingLeave(frontend, window, "unit_leave_editor");

  return expect(!left, "not-ready leave rejected") &&
         expect(!window.creativeAuthoring.roomEditing.ready, "not-ready leave keeps editing off") &&
         expect(window.creativeAuthoring.roomEditingLastOperationStatus == "room_editor_not_ready",
                "not-ready leave status") &&
         expect(window.creativeAuthoring.roomEditorStatus == "room_editor_not_ready",
                "not-ready editor status") &&
         expect(window.inputDevice.interactionMode ==
                    iggy3d::ProductInteractionMode::Player,
                "not-ready leave keeps player mode");
}

bool returnToTitleResetsCreativeModeToPlayer() {
  iggy3d::ProductAppWindowState window;
  const iggy3d::ProductRoomEditingStartResult started =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest());
  iggy3d::recordProductRoomEditingStart(window, started, "unit_edit_room");

  iggy3d::FrontendState frontend;
  frontend.screen = iggy3d::FrontendScreen::Gameplay;
  window.gameplay.gameplayActive = true;
  window.gameplay.runtimeSessionCreated = true;
  iggy3d::returnProductToTitleTransition(frontend, window);

  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window,
                                     saves);

  return expect(started.ok, "room editing start accepted for title reset") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Starter,
                "return to title opens starter") &&
         expect(window.inputDevice.interactionMode ==
                    iggy3d::ProductInteractionMode::Player,
                "return to title resets player mode") &&
         expect(iggy3d::hasReceiptField(receipt, "interaction_mode", "player"),
                "return to title receipt interaction mode");
}

bool gamepadSampleConversionIsStable() {
  const iggy3d::ProductControllerModeChordSample sample =
      iggy3d::productControllerModeChordSampleFromGamepad(
          iggy3d::GamepadControllerModeChordSample{true, false, true, false});
  return expect(sample.leftTriggerPressed, "left trigger converted") &&
         expect(!sample.rightTriggerPressed, "right trigger converted") &&
         expect(sample.leftStickPressed, "left stick press converted") &&
         expect(!sample.rightStickPressed, "right stick press converted");
}

bool surfaceDerivationIsConservative() {
  struct SurfaceCase {
    iggy3d::FrontendScreen screen;
    iggy3d::FrontendScreen childScreen;
    bool gameplayActive;
    bool roomEditingReady;
    iggy3d::ProductInputSurface expected;
  };
  constexpr std::array cases{
      SurfaceCase{iggy3d::FrontendScreen::Starter,
                  iggy3d::FrontendScreen::Gameplay,
                  false,
                  false,
                  iggy3d::ProductInputSurface::Starter},
      SurfaceCase{iggy3d::FrontendScreen::Starter,
                  iggy3d::FrontendScreen::NewWorld,
                  false,
                  false,
                  iggy3d::ProductInputSurface::WorldSetup},
      SurfaceCase{iggy3d::FrontendScreen::Starter,
                  iggy3d::FrontendScreen::LoadSave,
                  false,
                  false,
                  iggy3d::ProductInputSurface::SaveBrowser},
      SurfaceCase{iggy3d::FrontendScreen::Starter,
                  iggy3d::FrontendScreen::Settings,
                  false,
                  false,
                  iggy3d::ProductInputSurface::Settings},
      SurfaceCase{iggy3d::FrontendScreen::Starter,
                  iggy3d::FrontendScreen::StarterDevTools,
                  false,
                  false,
                  iggy3d::ProductInputSurface::DevTools},
      SurfaceCase{iggy3d::FrontendScreen::Pause,
                  iggy3d::FrontendScreen::Gameplay,
                  true,
                  false,
                  iggy3d::ProductInputSurface::Pause},
      SurfaceCase{iggy3d::FrontendScreen::Gameplay,
                  iggy3d::FrontendScreen::Gameplay,
                  true,
                  false,
                  iggy3d::ProductInputSurface::Gameplay},
      SurfaceCase{iggy3d::FrontendScreen::Gameplay,
                  iggy3d::FrontendScreen::Gameplay,
                  true,
                  true,
                  iggy3d::ProductInputSurface::RoomEditor},
      SurfaceCase{iggy3d::FrontendScreen::Gameplay,
                  iggy3d::FrontendScreen::Gameplay,
                  false,
                  false,
                  iggy3d::ProductInputSurface::None},
  };

  bool ok = true;
  for (const SurfaceCase row : cases) {
    iggy3d::FrontendState frontend;
    frontend.screen = row.screen;
    frontend.childScreen = row.childScreen;
    iggy3d::ProductAppWindowState window;
    window.gameplay.gameplayActive = row.gameplayActive;
    window.creativeAuthoring.roomEditing.ready = row.roomEditingReady;
    ok = expect(iggy3d::productInputSurfaceFor(frontend, window) ==
                    row.expected,
                "surface derivation case") &&
         ok;
  }
  return ok;
}

bool gameplayChordTogglesAndLatches() {
  iggy3d::FrontendState frontend;
  frontend.screen = iggy3d::FrontendScreen::Gameplay;
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  iggy3d::ProductControllerModeChordState chordState;

  const iggy3d::ProductInteractionModeToggleResult first =
      iggy3d::applyProductInteractionModeFrameToggle(
          {frontend, window, chordState, fullChord()});
  const iggy3d::ProductInteractionModeToggleResult held =
      iggy3d::applyProductInteractionModeFrameToggle(
          {frontend, window, chordState, fullChord()});
  const iggy3d::ProductInteractionModeToggleResult released =
      iggy3d::applyProductInteractionModeFrameToggle(
          {frontend, window, chordState, {}});
  const iggy3d::ProductInteractionModeToggleResult pressedAgain =
      iggy3d::applyProductInteractionModeFrameToggle(
          {frontend, window, chordState, fullChord()});

  return expect(first.toggleAccepted, "first gameplay chord accepted") &&
         expect(first.mode == iggy3d::ProductInteractionMode::Creative,
                "first gameplay chord sets creative") &&
         expect(window.inputDevice.interactionMode ==
                    iggy3d::ProductInteractionMode::Player,
                "re-pressed gameplay chord returns player") &&
         expect(held.toggleRequested, "held chord requested") &&
         expect(!held.toggleAccepted, "held chord not accepted") &&
         expect(held.status == "interaction_mode_chord_held",
                "held chord status") &&
         expect(!released.toggleRequested, "released chord not requested") &&
         expect(released.status == "interaction_mode_chord_partial",
                "released chord status") &&
         expect(pressedAgain.toggleAccepted, "re-pressed chord accepted") &&
         expect(pressedAgain.mode == iggy3d::ProductInteractionMode::Player,
                "re-pressed chord toggles player") &&
         expect(window.inputDevice.controllerModeToggle.surface == "gameplay",
                "gameplay toggle surface") &&
         expect(window.inputDevice.controllerModeToggle.status ==
                    "interaction_mode_toggled",
                "final toggle status");
}

bool roomEditorSurfaceAllowsToggle() {
  iggy3d::FrontendState frontend;
  frontend.screen = iggy3d::FrontendScreen::Gameplay;
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  window.creativeAuthoring.roomEditing.ready = true;
  iggy3d::ProductControllerModeChordState chordState;

  const iggy3d::ProductInteractionModeToggleResult result =
      iggy3d::applyProductInteractionModeFrameToggle(
          {frontend, window, chordState, fullChord()});
  return expect(result.toggleAccepted, "room editor chord accepted") &&
         expect(window.inputDevice.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "room editor toggles mode") &&
         expect(window.inputDevice.controllerModeToggle.surface == "room_editor",
                "room editor surface recorded");
}

bool blockedSurfaceDoesNotToggle() {
  iggy3d::FrontendState frontend;
  frontend.screen = iggy3d::FrontendScreen::Starter;
  iggy3d::ProductAppWindowState window;
  iggy3d::ProductControllerModeChordState chordState;

  const iggy3d::ProductInteractionModeToggleResult result =
      iggy3d::applyProductInteractionModeFrameToggle(
          {frontend, window, chordState, fullChord()});
  return expect(result.toggleRequested, "starter chord requested") &&
         expect(!result.toggleAccepted, "starter chord rejected") &&
         expect(window.inputDevice.interactionMode ==
                    iggy3d::ProductInteractionMode::Player,
                "starter preserves player mode") &&
         expect(window.inputDevice.controllerModeToggle.status ==
                    "interaction_mode_surface_blocked",
                "starter blocked status") &&
         expect(window.inputDevice.controllerModeToggle.reasonCode ==
                    "interaction_mode_surface_blocked",
                "starter blocked reason") &&
         expect(window.inputDevice.controllerModeToggle.surface == "starter",
                "starter surface recorded");
}

bool receiptFieldsExposeInteractionModeProof() {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductAppWindowState window;
  iggy3d::ProductSaveBridgeResult saves;

  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.inputDevice.controllerModeToggle.requested = true;
  window.inputDevice.controllerModeToggle.accepted = true;
  window.inputDevice.controllerModeToggle.status = "interaction_mode_toggled";
  window.inputDevice.controllerModeToggle.reasonCode = "interaction_mode_toggled";
  window.inputDevice.controllerModeToggle.surface = "gameplay";

  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window,
                                     saves);
  return expect(iggy3d::hasReceiptField(receipt, "interaction_mode", "creative"),
                "receipt interaction mode") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "controller_mode_toggle_requested",
                                        "true"),
                "receipt toggle requested") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "controller_mode_toggle_accepted",
                                        "true"),
                "receipt toggle accepted") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "controller_mode_toggle_status",
                                        "interaction_mode_toggled"),
                "receipt toggle status") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "controller_mode_toggle_reason_code",
                                        "interaction_mode_toggled"),
                "receipt toggle reason") &&
         expect(iggy3d::hasReceiptField(receipt,
                                        "controller_mode_toggle_surface",
                                        "gameplay"),
                "receipt toggle surface");
}

}  // namespace

int main() {
  const bool ok =
      defaultStateIsPlayerMode() && roomEditingStartSetsCreativeMode() &&
      roomEditingLeaveReturnsPlayerModeAndPreservesActiveRoom() &&
      roomEditingLeaveRejectsWhenNotReady() &&
      returnToTitleResetsCreativeModeToPlayer() &&
      gamepadSampleConversionIsStable() &&
      surfaceDerivationIsConservative() && gameplayChordTogglesAndLatches() &&
      roomEditorSurfaceAllowsToggle() && blockedSurfaceDoesNotToggle() &&
      receiptFieldsExposeInteractionModeProof();
  return ok ? 0 : 1;
}

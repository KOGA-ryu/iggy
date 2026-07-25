#include "EditorAppFramePhases.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include "EditorAssetReplacement.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"
#include "render/vulkan/VulkanBackend.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

CreativeEditorAppInputPhaseReceipt runCreativeEditorAppInputPhase(
    const CreativeEditorAppInputPhaseRequest& request) {
  CreativeEditorAppInputPhaseReceipt receipt;
  const PlaytestProcessOwner::PollResult playtestPoll =
      request.playtestOwner.poll();
  request.editor.desktopUi.playtestRunning = playtestPoll.running;
  if (playtestPoll.exitObserved) {
    request.editor.desktopUi.statusMessage =
        request.playtestOwner.monitor().lastExitMessage;
  } else if (playtestPoll.stalled) {
    request.editor.desktopUi.statusMessage =
        playtestStalledStatusMessage(playtestPoll.stallAgeMs);
  } else if (playtestPoll.stallRecovered) {
    request.editor.desktopUi.statusMessage = "playtest recovered";
  }
  {
    const PlaytestMonitorState& playtestMonitor =
        request.playtestOwner.monitor();
    static std::uint64_t lastSurfacedAckSeq = 0U;
    if (playtestMonitor.lastAckSeq != 0U &&
        playtestMonitor.lastAckSeq != lastSurfacedAckSeq &&
        playtestMonitor.lastAckStatus != "applied") {
      lastSurfacedAckSeq = playtestMonitor.lastAckSeq;
      request.editor.desktopUi.statusMessage =
          "playtest " + playtestMonitor.lastAckVerb + ": " +
          playtestMonitor.lastAckStatus +
          (playtestMonitor.lastAckReason.empty()
               ? std::string{}
               : " (" + playtestMonitor.lastAckReason + ")");
    }
  }

  receipt.frameInput = beginCreativeEditorFrameInput(
      request.window, request.backend, request.gamepad, request.editor,
      request.captureMode, /*applyEditorNavigation=*/true);
  if (!receipt.frameInput.keepRunning) {
    receipt.disposition = CreativeEditorAppFrameDisposition::Stop;
    return receipt;
  }
  if (receipt.frameInput.skipFrame) {
    if (!receipt.frameInput.windowFocused) {
      creative::CreativeAppState& activeAppState =
          activeCreativeEditorAppState(request.editor, request.appState);
      finalizeCreativeEditorContinuousGestures(
          activeAppState, request.editor,
          "creative_continuous_gesture_focus_lost");
      if (cancelCreativeEditorSelectionTransformPreview(
              request.editor.transform, "selection_transform_focus_lost")) {
        static_cast<void>(request.window.setRelativeMouseMode(true));
      }
      static_cast<void>(cancelCreativeEditorAssetReplacement(
          request.editor.assetReplacement,
          "creative_asset_replace_focus_lost"));
    }
    receipt.disposition = CreativeEditorAppFrameDisposition::Skip;
    return receipt;
  }

  if (!receipt.frameInput.windowFocused &&
      cancelCreativeEditorSelectionTransformPreview(
          request.editor.transform, "selection_transform_focus_lost")) {
    static_cast<void>(request.window.setRelativeMouseMode(true));
  }
  if (!receipt.frameInput.windowFocused) {
    static_cast<void>(cancelCreativeEditorAssetReplacement(
        request.editor.assetReplacement,
        "creative_asset_replace_focus_lost"));
  }

  layoutCreativeEditorDesktopDockspace(request.editor.desktopUi);
  const bool capturedEscRelease =
      creativeDesktopPointerCaptured(
          request.editor.desktopUi.viewportPointerCaptureMode) &&
      creative::creativeInputRouteContains(
          receipt.frameInput.routedInput,
          creative::CreativeInputActionId::ToggleControls);
  if (capturedEscRelease) {
    request.editor.desktopUi.viewportPointerCaptureMode =
        CreativeDesktopPointerCaptureMode::None;
    static_cast<void>(request.window.setViewportPointerCapture(false));
    creative::creativeInputRouteRemove(
        receipt.frameInput.routedInput,
        creative::CreativeInputActionId::ToggleControls);
    return receipt;
  }

  const bool primaryOverViewport =
      request.window.eventState().primaryPointerPressed &&
      !request.backend.externalUiWantsMouse();
  const bool viewportContext =
      receipt.frameInput.routedInput.context ==
          creative::CreativeInputContext::EditorViewport ||
      receipt.frameInput.routedInput.context ==
          creative::CreativeInputContext::RuntimePlay;
  const creative::CreativeInputModifierMask viewportGestureModifiers =
      receipt.frameInput.routedInput.context ==
              creative::CreativeInputContext::EditorViewport
          ? receipt.frameInput.modifiers
          : creative::kCreativeInputModifierNone;
  const CreativeDesktopPointerDecision pointerDecision =
      decideCreativeDesktopPointerCapture(
          request.editor.desktopUi.shellEnabled,
          request.editor.desktopUi.viewportPointerCaptureMode,
          primaryOverViewport,
          creative::creativeInputKeyDown(
              receipt.frameInput.inputFrame,
              creative::CreativeInputKey::MousePrimary),
          viewportGestureModifiers, viewportContext,
          receipt.frameInput.windowFocused);
  const bool gestureOwnsPrimary =
      creativeDesktopPointerModeOwnsPrimaryAction(
          request.editor.desktopUi.viewportPointerCaptureMode) ||
      creativeDesktopPointerModeOwnsPrimaryAction(pointerDecision.mode);
  if (pointerDecision.consumePrimaryPress || gestureOwnsPrimary) {
    creative::setCreativeInputKey(receipt.frameInput.inputFrame,
                                  creative::CreativeInputKey::MousePrimary,
                                  false);
    creative::creativeInputRouteRemove(
        receipt.frameInput.routedInput,
        creative::CreativeInputActionId::PrimaryAction);
    creative::creativeInputRouteRemove(
        receipt.frameInput.routedInput,
        creative::CreativeInputActionId::RuntimeAttack);
    constexpr std::size_t primaryActionIndex =
        static_cast<std::size_t>(creative::CreativeWorldActionId::Primary);
    receipt.frameInput.worldActions.down[primaryActionIndex] = false;
    receipt.frameInput.worldActions.pressed[primaryActionIndex] = false;
    receipt.frameInput.worldActions.released[primaryActionIndex] = false;
  }
  if (pointerDecision.changed) {
    const bool wasCaptured = creativeDesktopPointerCaptured(
        request.editor.desktopUi.viewportPointerCaptureMode);
    const iggy3d::SdlMouseCaptureResult captureResult =
        request.window.setViewportPointerCapture(pointerDecision.captured);
    const bool acquired = !wasCaptured && captureResult.active;
    request.editor.desktopUi.viewportPointerCaptureMode =
        captureResult.active ? pointerDecision.mode
                             : CreativeDesktopPointerCaptureMode::None;
    request.editor.desktopUi.discardNextViewportMouseDelta =
        acquired && pointerDecision.discardNextMouseDelta;
  }
  return receipt;
}

}  // namespace iggy3d_creative_app

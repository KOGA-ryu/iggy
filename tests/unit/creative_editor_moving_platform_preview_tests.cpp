#include "EditorMovingPlatformPreview.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs, float epsilon = 1.0e-4F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

cr::CreativeDocument makeDocument(cr::CreativeObjectId& platformId) {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Moving Platform Preview");
  static_cast<void>(document.assignId(880U));
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::MovingPlatform;
  request.name = "Preview Platform";
  request.transform.position = {0.0, 0.5, 0.0};
  request.hasTransformOverride = true;
  request.bounds = {{-0.5, 0.25, -0.5}, {0.5, 0.75, 0.5}};
  request.hasBoundsOverride = true;
  request.pathPoints = {{{0.0, 0.5, 0.0}}, {{2.0, 0.5, 0.0}}};
  request.hasPathOverride = true;
  request.movingPlatform.speedMetersPerSecond = 1.0;
  request.hasMovingPlatformSettingsOverride = true;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(request);
  platformId = created.objectId;
  return document;
}

bool playbackIsTransientDeterministicAndBounded() {
  cr::CreativeObjectId platformId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocument(platformId);
  const cr::CreativeObject* platform = document.findObject(platformId);
  if (platform == nullptr) {
    return expect(false, "preview platform exists");
  }
  const std::uint64_t revisionBefore = document.revision();
  app::CreativeMovingPlatformPreviewState state;
  const app::CreativeMovingPlatformPreviewReceipt synced =
      app::syncCreativeMovingPlatformPreview(
          state, document.id(), platform);
  const auto wrongTarget = app::applyCreativeMovingPlatformPreviewCommand(
      state, app::CreativeMovingPlatformPreviewCommand::TogglePlayback,
      platformId + 1U);
  const auto playing = app::applyCreativeMovingPlatformPreviewCommand(
      state, app::CreativeMovingPlatformPreviewCommand::TogglePlayback,
      platformId);
  const auto firstTick =
      app::advanceCreativeMovingPlatformPreview(state, 1.0 / 60.0);
  const auto boundedCatchUp =
      app::advanceCreativeMovingPlatformPreview(state, 1.0);
  const auto seek = app::applyCreativeMovingPlatformPreviewCommand(
      state, app::CreativeMovingPlatformPreviewCommand::Seek, platformId,
      0.5);
  const bool seekedToMidpoint =
      near(state.runtimeState.positionMeters.x, 1.0F) && !state.playing;
  const auto restart = app::applyCreativeMovingPlatformPreviewCommand(
      state, app::CreativeMovingPlatformPreviewCommand::Restart, platformId);

  return expect(synced.accepted && synced.reset && state.available &&
                    state.visible,
                "selection builds a visible transient preview") &&
         expect(!wrongTarget.accepted,
                "preview commands reject a stale object target") &&
         expect(playing.accepted && firstTick.accepted &&
                    firstTick.advancedTickCount == 1U,
                "play uses one deterministic fixed tick") &&
         expect(boundedCatchUp.accepted &&
                    boundedCatchUp.advancedTickCount == 15U,
                "editor catch-up is bounded to fifteen route ticks") &&
         expect(seek.accepted && seekedToMidpoint,
                "scrubbing samples the route midpoint and pauses") &&
         expect(restart.accepted && restart.reset &&
                    near(state.runtimeState.positionMeters.x, 0.0F) &&
                    state.normalizedProgress == 0.0,
                "restart returns to authored route origin") &&
         expect(document.revision() == revisionBefore,
                "preview playback never mutates document revision");
}

bool routeEditsResetAndSelectionLossClears() {
  cr::CreativeObjectId platformId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocument(platformId);
  const cr::CreativeObject* platform = document.findObject(platformId);
  if (platform == nullptr) {
    return expect(false, "reset platform exists");
  }
  app::CreativeMovingPlatformPreviewState state;
  static_cast<void>(app::syncCreativeMovingPlatformPreview(
      state, document.id(), platform));
  static_cast<void>(app::applyCreativeMovingPlatformPreviewCommand(
      state, app::CreativeMovingPlatformPreviewCommand::TogglePlayback,
      platformId));
  static_cast<void>(app::advanceCreativeMovingPlatformPreview(state, 0.1));

  cr::CreativeObject edited = *platform;
  edited.pathPoints[1].position.x = 4.0;
  const app::CreativeMovingPlatformPreviewReceipt reset =
      app::syncCreativeMovingPlatformPreview(state, document.id(), &edited);
  const bool resumedAtOrigin =
      reset.accepted && reset.reset && state.playing &&
      state.normalizedProgress == 0.0 &&
      near(state.runtimeState.positionMeters.x, 0.0F);
  static_cast<void>(app::advanceCreativeMovingPlatformPreview(state, 0.1));
  edited.transform.rotationEulerRadians.y = 0.5;
  edited.transform.scale = {1.5, 0.75, 1.25};
  const app::CreativeMovingPlatformPreviewReceipt transformReset =
      app::syncCreativeMovingPlatformPreview(state, document.id(), &edited);
  const bool transformResetAtOrigin =
      transformReset.accepted && transformReset.reset && state.playing &&
      state.normalizedProgress == 0.0 &&
      near(state.runtimeState.positionMeters.x, 0.0F);
  static_cast<void>(app::advanceCreativeMovingPlatformPreview(state, 0.1));
  edited.pathPoints[1].dwellSeconds = 0.5;
  const app::CreativeMovingPlatformPreviewReceipt dwellReset =
      app::syncCreativeMovingPlatformPreview(state, document.id(), &edited);
  const bool dwellResetAtOrigin =
      dwellReset.accepted && dwellReset.reset && state.playing &&
      state.normalizedProgress == 0.0 &&
      near(state.runtimeState.positionMeters.x, 0.0F);
  static_cast<void>(app::advanceCreativeMovingPlatformPreview(state, 0.1));
  edited.pathPoints[0].outgoingSpeedMultiplier = 2.0;
  const app::CreativeMovingPlatformPreviewReceipt speedReset =
      app::syncCreativeMovingPlatformPreview(state, document.id(), &edited);
  const bool speedResetAtOrigin =
      speedReset.accepted && speedReset.reset && state.playing &&
      state.normalizedProgress == 0.0 &&
      near(state.runtimeState.positionMeters.x, 0.0F);
  const app::CreativeMovingPlatformPreviewReceipt cleared =
      app::syncCreativeMovingPlatformPreview(state, document.id(), nullptr);
  return expect(resumedAtOrigin,
                "same-object route edits reset and preserve play intent") &&
         expect(transformResetAtOrigin,
                "rotation and scale edits reset route playback") &&
         expect(dwellResetAtOrigin,
                "waypoint dwell edits reset route playback") &&
         expect(speedResetAtOrigin,
                "segment speed edits reset route playback") &&
         expect(cleared.accepted && cleared.changed && !state.available &&
                    !state.visible && !state.playing,
                "leaving moving-platform selection clears preview state");
}

bool previewRendersThroughItsOwnBoundedRole() {
  cr::CreativeObjectId platformId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocument(platformId);
  const cr::CreativeObject* platform = document.findObject(platformId);
  if (platform == nullptr) {
    return expect(false, "render preview platform exists");
  }
  app::CreativeEditorState editor;
  static_cast<void>(app::syncCreativeMovingPlatformPreview(
      editor.movingPlatformPreview, document.id(), platform));
  static_cast<void>(app::applyCreativeMovingPlatformPreviewCommand(
      editor.movingPlatformPreview,
      app::CreativeMovingPlatformPreviewCommand::Seek, platformId, 0.5));
  iggy3d::FrameInput frame;
  frame.camera.clipFromWorld = iggy3d::identityMat4();
  app::attachCreativeEditorPlacementPreviews(editor, false, frame, &document);
  const std::uint64_t revisionAfterPreview = document.revision();

  iggy3d::FrameInput captureFrame;
  captureFrame.camera.clipFromWorld = iggy3d::identityMat4();
  app::attachCreativeEditorPlacementPreviews(editor, true, captureFrame,
                                             &document);
  return expect(frame.creativePreview.itemCount == 1U &&
                    frame.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::MovingPlatformRoute,
                "moving platform uses one dedicated preview item") &&
         expect(near(frame.creativePreview.items[0].clipFromModel.m[3], 1.0F),
                "preview matrix follows sampled world position") &&
         expect(captureFrame.creativePreview.itemCount == 0U,
                "scripted capture remains preview-inert") &&
         expect(document.revision() == revisionAfterPreview,
                "render attachment does not mutate authored state");
}

bool controllerToolOptionsExposeContextualPlaybackCommands() {
  const cr::CreativeHotbarEntry move{cr::CreativeHeldItemKind::ObjectMove,
                                     cr::CreativeObjectKind::Unknown};
  const app::CreativeEditorToolOptionsCommandList ordinary =
      app::creativeEditorToolOptionCommandsForEntry(move);
  const app::CreativeEditorToolOptionsCommandList movingPlatform =
      app::creativeEditorToolOptionCommandsForEntry(
          move, cr::CreativeObjectKind::MovingPlatform);
  return expect(ordinary.count + 4U == movingPlatform.count,
                "route controls are contextual rather than global") &&
         expect(movingPlatform.ids[movingPlatform.count - 4U] ==
                    app::CreativeEditorToolOptionsCommandId::
                        SetMovingPlatformWaypointDwell &&
                    movingPlatform.ids[movingPlatform.count - 3U] ==
                        app::CreativeEditorToolOptionsCommandId::
                            SetMovingPlatformSegmentSpeed &&
                    movingPlatform.ids[movingPlatform.count - 2U] ==
                    app::CreativeEditorToolOptionsCommandId::
                        ToggleMovingPlatformPreview &&
                    movingPlatform.ids[movingPlatform.count - 1U] ==
                        app::CreativeEditorToolOptionsCommandId::
                            RestartMovingPlatformPreview,
                "tool options expose dwell, segment speed, playback, and restart");
}

bool controllerToolOptionsCommitWaypointDwellOnce() {
  cr::CreativeObjectId platformId = cr::kInvalidObjectId;
  cr::CreativeAppState appState;
  static_cast<void>(
      appState.facade.installDocument(makeDocument(platformId)));
  app::CreativeEditorState editor;
  app::CreativeEditorToolOptionsState& options = editor.toolOptions;
  options.open = true;
  options.targetEntry = {cr::CreativeHeldItemKind::ObjectMove,
                         cr::CreativeObjectKind::Unknown};
  options.commands = app::creativeEditorToolOptionCommandsForEntry(
      options.targetEntry, cr::CreativeObjectKind::MovingPlatform);
  options.contextPrimaryObjectId = platformId;
  options.contextPrimaryObjectKind =
      cr::CreativeObjectKind::MovingPlatform;
  options.contextSelectionCount = 1U;
  options.contextMovingPlatformPointSelected = true;
  options.contextMovingPlatformPointIndex = 1U;
  options.movingPlatformWaypointDwellDraft = 0.75;
  std::size_t commandIndex = options.commands.count;
  for (std::size_t index = 0U; index < options.commands.count; ++index) {
    if (options.commands.ids[index] == app::CreativeEditorToolOptionsCommandId::
                                           SetMovingPlatformWaypointDwell) {
      commandIndex = index;
      break;
    }
  }
  if (commandIndex == options.commands.count) {
    return expect(false, "controller dwell command exists");
  }
  options.selectedIndex = options.options.count + commandIndex;

  const bool accepted =
      app::activateCreativeEditorToolOptionsSelection(appState, editor);
  const cr::CreativeObject* platform =
      appState.facade.findObject(platformId);
  return expect(accepted && !options.open && platform != nullptr &&
                    platform->pathPoints[1].dwellSeconds == 0.75,
                "controller confirm authors the selected waypoint wait") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U,
                "controller waypoint wait records one undo entry");
}

bool previewReportsWaitingAtAuthoredDwell() {
  cr::CreativeObjectId platformId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocument(platformId);
  cr::CreativeObject edited = *document.findObject(platformId);
  edited.movingPlatform.speedMetersPerSecond = 60.0;
  edited.pathPoints[1].dwellSeconds = 0.25;
  app::CreativeMovingPlatformPreviewState state;
  const auto sync = app::syncCreativeMovingPlatformPreview(
      state, document.id(), &edited);
  static_cast<void>(app::applyCreativeMovingPlatformPreviewCommand(
      state, app::CreativeMovingPlatformPreviewCommand::TogglePlayback,
      platformId));
  static_cast<void>(app::advanceCreativeMovingPlatformPreview(
      state, 2.0 / 60.0));

  return expect(sync.accepted && state.runtimeState.dwellTicksRemaining == 15U,
                "preview uses runtime dwell tick conversion") &&
         expect(app::creativeMovingPlatformPreviewStatusLabel(state) ==
                    "Waiting",
                "preview reports waiting while held at waypoint") &&
         expect(document.revision() == 1U,
                "dwell preview remains document-inert");
}

bool controllerToolOptionsCommitSegmentSpeedOnce() {
  cr::CreativeObjectId platformId = cr::kInvalidObjectId;
  cr::CreativeAppState appState;
  static_cast<void>(
      appState.facade.installDocument(makeDocument(platformId)));
  app::CreativeEditorState editor;
  app::CreativeEditorToolOptionsState& options = editor.toolOptions;
  options.open = true;
  options.targetEntry = {cr::CreativeHeldItemKind::ObjectMove,
                         cr::CreativeObjectKind::Unknown};
  options.commands = app::creativeEditorToolOptionCommandsForEntry(
      options.targetEntry, cr::CreativeObjectKind::MovingPlatform);
  options.contextPrimaryObjectId = platformId;
  options.contextPrimaryObjectKind =
      cr::CreativeObjectKind::MovingPlatform;
  options.contextSelectionCount = 1U;
  options.contextMovingPlatformPointSelected = true;
  options.contextMovingPlatformPointHasOutgoingSegment = true;
  options.contextMovingPlatformPointIndex = 0U;
  options.movingPlatformSegmentSpeedDraft = 2.25;
  std::size_t commandIndex = options.commands.count;
  for (std::size_t index = 0U; index < options.commands.count; ++index) {
    if (options.commands.ids[index] ==
        app::CreativeEditorToolOptionsCommandId::
            SetMovingPlatformSegmentSpeed) {
      commandIndex = index;
      break;
    }
  }
  if (commandIndex == options.commands.count) {
    return expect(false, "controller segment speed command exists");
  }
  options.selectedIndex = options.options.count + commandIndex;

  const bool accepted =
      app::activateCreativeEditorToolOptionsSelection(appState, editor);
  const cr::CreativeObject* platform = appState.facade.findObject(platformId);
  const bool speedStored =
      platform != nullptr &&
      platform->pathPoints[0].outgoingSpeedMultiplier == 2.25;
  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeObject* undone = appState.facade.findObject(platformId);
  return expect(accepted && !options.open && speedStored,
                "controller confirm authors the selected route segment speed") &&
         expect(cr::creativeUndoDepth(appState.history) == 0U &&
                    undo.status == cr::CreativeHistoryStatus::Applied &&
                    undone != nullptr &&
                    undone->pathPoints[0].outgoingSpeedMultiplier == 1.0,
                "segment speed authors one reversible history entry");
}

}  // namespace

int main() {
  const bool ok = playbackIsTransientDeterministicAndBounded() &&
                  routeEditsResetAndSelectionLossClears() &&
                  previewRendersThroughItsOwnBoundedRole() &&
                  controllerToolOptionsExposeContextualPlaybackCommands() &&
                  controllerToolOptionsCommitWaypointDwellOnce() &&
                  controllerToolOptionsCommitSegmentSpeedOnce() &&
                  previewReportsWaitingAtAuthoredDwell();
  return ok ? 0 : 1;
}

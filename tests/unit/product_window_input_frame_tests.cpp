#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/room_editor/EditingState.hpp"
#include "app/iggy3d/window/InputFrame.hpp"
#include "app/iggy3d/automation/AutomationRoomEditing.hpp"

#include <iostream>
#include <limits>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductAsciiRoomAuthoringRequest smallRoomRequest() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "###\n"
      "#P#\n"
      "###\n";
  request.roomId = "input_frame_room";
  request.sourceName = "unit/input_frame_room.iggyroom.txt";
  request.emitAssetText = false;
  return request;
}

iggy3d::FrontendState gameplayFrontend() {
  iggy3d::FrontendState frontend;
  iggy3d::enterFrontendGameplay(frontend, iggy3d::FrontendAction::NewWorld);
  return frontend;
}

iggy3d::ProductViewportFrameConfig simpleViewportConfig() {
  iggy3d::ProductViewportFrameConfig config;
  config.centerX = 100.0F;
  config.centerY = 100.0F;
  config.pixelsPerMeter = 100.0F;
  return config;
}

iggy3d::ProductAppWindowState editingWindow() {
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;

  const iggy3d::ProductRoomEditingStartResult started =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest());
  iggy3d::recordProductRoomEditingStart(window, started, "unit_edit_room");
  window.roomEditorCursor = {};
  window.roomEditorCursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  return window;
}

iggy3d::ProductWindowEditorMousePickPreviewContext pickContext(
    const iggy3d::FrontendState& frontend,
    iggy3d::ProductAppWindowState& window,
    iggy3d::MouseClick click) {
  return {
      frontend,
      window,
      click,
      simpleViewportConfig(),
      {},
  };
}

iggy3d::MouseClick clickAt(float x, float y) {
  iggy3d::MouseClick click;
  click.clicked = true;
  click.x = x;
  click.y = y;
  return click;
}

bool creativeClickPicksCursorAndBuildsPreviewWithoutMutation() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialFloors = window.roomEditing.documentFloorCount;
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;
  const std::uint64_t initialWalkable =
      window.roomEditing.collisionWalkableSurfaceCount;
  const std::uint64_t initialBlockers =
      window.roomEditing.collisionActorBlockerSurfaceCount;

  const iggy3d::ProductWindowEditorMousePickPreviewResult result =
      iggy3d::processProductWindowEditorMousePickPreview(
          pickContext(frontend, window, clickAt(300.0F, 100.0F)));

  return expect(result.handled, "creative mouse click handled") &&
         expect(window.interactionMode == iggy3d::ProductInteractionMode::Creative,
                "edit room entry keeps creative mode") &&
         expect(result.picked, "creative mouse click picked") &&
         expect(result.previewBuilt, "creative mouse click built preview") &&
         expect(result.accepted, "creative mouse click preview accepted") &&
         expect(result.status == "room_editor_preview_ready",
                "creative mouse preview status") &&
         expect(window.inputOwner == iggy3d::MenuOwner::Editor,
                "creative mouse input owner editor") &&
         expect(window.lastInputAction ==
                    iggy3d::InputAction::EditorPreviewPlacement,
                "creative mouse semantic action") &&
         expect(window.lastInputAccepted, "creative mouse input accepted") &&
         expect(window.gameplayInputSuppressed,
                "creative mouse suppresses gameplay input") &&
         expect(window.roomEditorStatus == "room_editor_mouse_pick_mapped",
                "creative mouse pick status") &&
         expect(window.roomEditorLastOperation == "room_editor.mouse_pick",
                "creative mouse pick operation") &&
         expect(window.roomEditorCursor.gridX == 2,
                "creative mouse pick cursor x") &&
         expect(window.roomEditorCursor.gridZ == 0,
                "creative mouse pick cursor z") &&
         expect(window.roomEditorCursor.selectedTool ==
                    iggy3d::ProductRoomEditorTool::Wall,
                "creative mouse pick preserves tool") &&
         expect(window.roomEditorPreviewVisible,
                "creative mouse preview visible") &&
         expect(window.roomEditorPreviewStatus == "room_editor_preview_ready",
                "creative mouse preview receipt status") &&
         expect(window.roomEditorPreviewCandidateId == "edit_wall_1",
                "creative mouse preview candidate") &&
         expect(window.roomEditorPreviewTool == "wall",
                "creative mouse preview tool") &&
         expect(window.roomEditorPreviewGridX == 2,
                "creative mouse preview grid x") &&
         expect(window.roomEditorPreviewGridZ == 0,
                "creative mouse preview grid z") &&
         expect(window.roomEditing.documentFloorCount == initialFloors,
                "creative mouse leaves floor count") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "creative mouse leaves wall count") &&
         expect(window.roomEditing.collisionWalkableSurfaceCount == initialWalkable,
                "creative mouse leaves walkable collision") &&
         expect(window.roomEditing.collisionActorBlockerSurfaceCount ==
                    initialBlockers,
                "creative mouse leaves blocker collision");
}

bool playerClickDoesNotRunEditorPickPreview() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  window.interactionMode = iggy3d::ProductInteractionMode::Player;
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

  const iggy3d::ProductWindowEditorMousePickPreviewResult result =
      iggy3d::processProductWindowEditorMousePickPreview(
          pickContext(frontend, window, clickAt(300.0F, 100.0F)));

  return expect(!result.handled, "player mouse click not editor handled") &&
         expect(!result.picked, "player mouse click not picked") &&
         expect(!result.previewBuilt, "player mouse click no preview") &&
         expect(!result.accepted, "player mouse click not accepted") &&
         expect(result.status == "room_editor_mouse_pick_preview_mode_blocked",
                "player mouse mode-blocked status") &&
         expect(window.roomEditorCursor.gridX == 0,
                "player mouse leaves cursor x") &&
         expect(!window.roomEditorPreviewVisible,
                "player mouse leaves preview hidden") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "player mouse leaves wall count");
}

bool notReadyClickDoesNotMutateEditorState() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;

  const iggy3d::ProductWindowEditorMousePickPreviewResult result =
      iggy3d::processProductWindowEditorMousePickPreview(
          pickContext(frontend, window, clickAt(300.0F, 100.0F)));

  return expect(!result.handled, "not-ready mouse click not handled") &&
         expect(result.status == "room_editor_not_ready",
                "not-ready mouse click status") &&
         expect(!window.roomEditing.ready, "not-ready leaves editing off") &&
         expect(!window.roomEditorPreviewVisible,
                "not-ready leaves preview hidden");
}

bool invalidClickPropagatesMousePickRejectionWithoutMutation() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

  const iggy3d::ProductWindowEditorMousePickPreviewResult result =
      iggy3d::processProductWindowEditorMousePickPreview(
          pickContext(frontend,
                      window,
                      clickAt(std::numeric_limits<float>::infinity(), 100.0F)));

  return expect(result.handled, "invalid mouse click handled by editor") &&
         expect(!result.picked, "invalid mouse click not picked") &&
         expect(!result.previewBuilt, "invalid mouse click no preview") &&
         expect(!result.accepted, "invalid mouse click not accepted") &&
         expect(result.status == "room_editor_mouse_pick_invalid_input",
                "invalid mouse pick status") &&
         expect(window.roomEditorStatus ==
                    "room_editor_mouse_pick_invalid_input",
                "invalid mouse pick receipt status") &&
         expect(!window.lastInputAccepted, "invalid mouse input not accepted") &&
         expect(!window.roomEditorPreviewVisible,
                "invalid mouse leaves preview hidden") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "invalid mouse leaves wall count");
}

bool controllerSouthStagesThenConfirmsPlacement() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  window.roomEditorCursor.gridX = 1;
  iggy3d::Session session;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

  const iggy3d::GamepadControllerActionSample south =
      iggy3d::productControllerActionSampleForControl(
          iggy3d::ProductControllerControl::SouthButton);
  const iggy3d::ProductControllerSampleInputResult staged =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &session, chord, routing, nullptr, "unit"}, south);

  const bool firstOk =
      expect(staged.actionApplied, "first south action applied") &&
      expect(staged.actionAccepted, "first south stages accepted preview") &&
      expect(window.lastInputAction == iggy3d::InputAction::EditorPlace,
             "first south routes to editor place") &&
      expect(window.roomEditorStatus == "room_editor_preview_ready",
             "first south stages preview") &&
      expect(window.roomEditorLastOperation == "editor.place",
             "first south operation is editor place") &&
      expect(window.roomEditorPreviewActive, "first south preview pending") &&
      expect(window.roomEditorPreviewVisible, "first south preview visible") &&
      expect(window.roomEditorPreviewCandidateId == "edit_wall_1",
             "first south candidate id") &&
      expect(window.roomEditing.documentWallCount == initialWalls,
             "first south does not mutate walls");

  (void)iggy3d::processProductControllerActionSample(
      {frontend, window, &session, chord, routing, nullptr, "unit"},
      iggy3d::GamepadControllerActionSample{});
  const iggy3d::ProductControllerSampleInputResult confirmed =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &session, chord, routing, nullptr, "unit"}, south);

  return firstOk &&
         expect(confirmed.actionApplied, "second south action applied") &&
         expect(confirmed.actionAccepted, "second south confirms preview") &&
         expect(window.lastInputAction == iggy3d::InputAction::EditorPlace,
                "second south routes to editor place") &&
         expect(window.roomEditorStatus == "room_editor_preview_confirmed",
                "second south confirm status") &&
         expect(window.roomEditorLastOperation == "editor.place",
                "second south operation is editor place") &&
         expect(!window.roomEditorPreviewActive,
                "second south clears pending preview") &&
         expect(!window.roomEditorPreviewVisible,
                "second south hides preview") &&
         expect(window.roomEditorLastPrimitiveId == "edit_wall_1",
                "second south primitive id") &&
         expect(window.roomEditing.documentWallCount == initialWalls + 1U,
                "second south mutates walls") &&
         expect(window.roomEditingLastOperation == "editor.place",
                "second south editing operation") &&
         expect(window.roomEditingLastOperationAccepted,
                "second south editing accepted");
}

bool controllerMoveInvalidatesPendingPreview() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  iggy3d::Session session;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

  (void)iggy3d::processProductControllerActionSample(
      {frontend, window, &session, chord, routing, nullptr, "unit"},
      iggy3d::productControllerActionSampleForControl(
          iggy3d::ProductControllerControl::SouthButton));
  (void)iggy3d::processProductControllerActionSample(
      {frontend, window, &session, chord, routing, nullptr, "unit"},
      iggy3d::GamepadControllerActionSample{});
  const iggy3d::ProductControllerSampleInputResult moved =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &session, chord, routing, nullptr, "unit"},
          iggy3d::productControllerActionSampleForControl(
              iggy3d::ProductControllerControl::DpadRight));

  return expect(moved.actionApplied, "dpad move action applied") &&
         expect(moved.actionAccepted, "dpad move accepted") &&
         expect(window.lastInputAction == iggy3d::InputAction::EditorNudgeX,
                "dpad right routes to nudge x") &&
         expect(window.roomEditorCursor.gridX == 1,
                "dpad right moves cursor") &&
         expect(window.roomEditorStatus == "room_editor_cursor_moved",
                "dpad move cursor status") &&
         expect(!window.roomEditorPreviewActive,
                "dpad move clears pending preview") &&
         expect(!window.roomEditorPreviewVisible,
                "dpad move hides preview") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "dpad move does not mutate walls");
}

bool backCancelsPendingPreviewBeforePauseRoute() {
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;
  window.roomEditorCursor.gridX = 1;

  const iggy3d::ProductRoomEditorPreviewInputResult staged =
      iggy3d::applyProductRoomEditorPreviewInputAction(
          window, iggy3d::InputAction::EditorPlace);
  const bool stageOk =
      expect(staged.ok, "back test stages preview") &&
      expect(window.roomEditorPreviewActive, "back test preview active") &&
      expect(window.roomEditing.documentWallCount == initialWalls,
             "back test stage leaves walls");

  const bool cancelled =
      iggy3d::cancelProductRoomEditorPendingPreviewFromBack(window);

  return stageOk && expect(cancelled, "back cancels pending preview") &&
         expect(window.inputOwner == iggy3d::MenuOwner::Editor,
                "back cancel owner editor") &&
         expect(window.lastInputAction == iggy3d::InputAction::EditorCancelPreview,
                "back cancel records editor cancel action") &&
         expect(window.lastInputAccepted, "back cancel input accepted") &&
         expect(window.gameplayInputSuppressed,
                "back cancel suppresses gameplay input") &&
         expect(window.roomEditorStatus == "room_editor_preview_cancelled",
                "back cancel status") &&
         expect(window.roomEditorLastOperation == "editor.preview_cancel",
                "back cancel operation") &&
         expect(!window.roomEditorPreviewActive,
                "back cancel clears pending preview") &&
         expect(!window.roomEditorPreviewVisible, "back cancel hides preview") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "back cancel leaves walls");
}

bool backWithoutPreviewFallsThroughPolicy() {
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

  const bool cancelled =
      iggy3d::cancelProductRoomEditorPendingPreviewFromBack(window);

  return expect(!cancelled, "back without preview not consumed") &&
         expect(window.roomEditing.ready, "back without preview keeps editor ready") &&
         expect(!window.roomEditorPreviewActive,
                "back without preview leaves preview inactive") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "back without preview leaves walls");
}

bool controllerEastCancelsPendingPreviewWithoutMutation() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  window.roomEditorCursor.gridX = 1;
  iggy3d::Session session;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

  const iggy3d::ProductControllerSampleInputResult staged =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &session, chord, routing, nullptr, "unit"},
          iggy3d::productControllerActionSampleForControl(
              iggy3d::ProductControllerControl::SouthButton));
  (void)iggy3d::processProductControllerActionSample(
      {frontend, window, &session, chord, routing, nullptr, "unit"},
      iggy3d::GamepadControllerActionSample{});
  const iggy3d::ProductControllerSampleInputResult cancelled =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &session, chord, routing, nullptr, "unit"},
          iggy3d::productControllerActionSampleForControl(
              iggy3d::ProductControllerControl::EastButton));

  return expect(staged.actionApplied, "east cancel test stages action") &&
         expect(staged.actionAccepted, "east cancel test stages preview") &&
         expect(cancelled.actionApplied, "east cancel action applied") &&
         expect(cancelled.actionAccepted, "east cancel action accepted") &&
         expect(window.lastInputAction == iggy3d::InputAction::EditorCancelPreview,
                "east cancel routes to editor cancel preview") &&
         expect(window.roomEditorStatus == "room_editor_preview_cancelled",
                "east cancel status") &&
         expect(window.roomEditorLastOperation == "editor.preview_cancel",
                "east cancel operation") &&
         expect(!window.roomEditorPreviewActive,
                "east cancel clears pending preview") &&
         expect(!window.roomEditorPreviewVisible, "east cancel hides preview") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "east cancel leaves walls");
}

}  // namespace

int main() {
  const bool passed =
      creativeClickPicksCursorAndBuildsPreviewWithoutMutation() &&
      playerClickDoesNotRunEditorPickPreview() &&
      notReadyClickDoesNotMutateEditorState() &&
      invalidClickPropagatesMousePickRejectionWithoutMutation() &&
      controllerSouthStagesThenConfirmsPlacement() &&
      controllerMoveInvalidatesPendingPreview() &&
      backCancelsPendingPreviewBeforePauseRoute() &&
      backWithoutPreviewFallsThroughPolicy() &&
      controllerEastCancelsPendingPreviewWithoutMutation();
  std::cout << "product_window_input_frame_tests="
            << (passed ? "pass" : "fail") << '\n';
  return passed ? 0 : 1;
}

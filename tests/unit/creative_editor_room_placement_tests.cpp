#include "EditorInteraction.hpp"
#include "EditorEdits.hpp"
#include "EditorRoomPlacement.hpp"
#include "EditorState.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"

namespace {
namespace cr = iggy3d::creative;
namespace app = iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) <= 1.0e-9;
}

bool sameBounds(cr::CreativeBounds lhs, cr::CreativeBounds rhs) {
  return near(lhs.min.x, rhs.min.x) && near(lhs.min.y, rhs.min.y) &&
         near(lhs.min.z, rhs.min.z) && near(lhs.max.x, rhs.max.x) &&
         near(lhs.max.y, rhs.max.y) && near(lhs.max.z, rhs.max.z);
}

bool installDocument(cr::CreativeAppState& appState,
                     cr::CreativeDocumentId documentId) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Room Tool");
  static_cast<void>(document.assignId(documentId));
  return appState.facade.installDocument(std::move(document)).accepted;
}

app::CreativeEditorState roomEditor() {
  app::CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::BuildingRoom,
      cr::CreativeObjectKind::Unknown};
  editor.interaction.synchronizedHeldItemKind =
      cr::CreativeHeldItemKind::BuildingRoom;
  editor.toolSettings.roomWallHeight = cr::CreativeRoomWallHeight::ThreeMeters;
  editor.toolSettings.roomWallThickness =
      cr::CreativeRoomWallThickness::QuarterMeter;
  editor.toolSettings.roomFloorThickness =
      cr::CreativeRoomFloorThickness::QuarterMeter;
  editor.frameIndex = 12U;
  return editor;
}

void setRoomTarget(app::CreativeEditorState& editor,
                   std::int32_t x,
                   std::int32_t z,
                   double floorTop = 0.0) {
  editor.interaction.target = {};
  editor.interaction.target.valid = true;
  editor.interaction.target.grid.valid = true;
  editor.interaction.target.grid.adjacentCell = {x, 0, z};
  editor.interaction.target.grid.adjacentCellBounds = {
      {static_cast<double>(x), floorTop, static_cast<double>(z)},
      {static_cast<double>(x + 1), floorTop + 1.0,
       static_cast<double>(z + 1)}};
}

bool twoCornersApplyOneUndoableShell() {
  cr::CreativeAppState appState;
  app::CreativeEditorState editor = roomEditor();
  if (!expect(installDocument(appState, 301U),
              "room tool document installed")) {
    return false;
  }

  setRoomTarget(editor, 0, 0);
  const app::CreativeEditorRoomPlacementReceipt first =
      app::advanceCreativeEditorRoomPlacement(appState, editor, "room-first");
  bool ok = expect(first.accepted && !first.changed &&
                       first.status ==
                           app::CreativeEditorRoomPlacementStatus::
                               FirstCornerSet &&
                       editor.interaction.roomPlacement.active,
                   "first room corner starts a transient draft") &&
            expect(appState.facade.document().objectCount() == 0U &&
                       cr::creativeUndoDepth(appState.history) == 0U,
                   "first room corner does not mutate the document");

  setRoomTarget(editor, 2, 1);
  const app::CreativeEditorRoomPlacementReceipt applied =
      app::advanceCreativeEditorRoomPlacement(appState, editor, "room-apply");
  const cr::CreativeObject* floor = appState.facade.document().findObject(1U);
  const auto northFound = std::find_if(
      appState.facade.document().objects().begin(),
      appState.facade.document().objects().end(),
      [](const cr::CreativeObject& object) {
        return object.kind == cr::CreativeObjectKind::Wall;
      });
  const cr::CreativeObject* north =
      northFound == appState.facade.document().objects().end()
          ? nullptr
          : &*northFound;
  const bool linkedFloorSelection =
      floor != nullptr && app::selectCreativeEditorWorldLayoutObjectSource(
                              editor.worldLayout, *floor);
  const bool linkedWallSelection =
      north != nullptr && app::selectCreativeEditorWorldLayoutObjectSource(
                              editor.worldLayout, *north);
  ok = expect(applied.accepted && applied.changed &&
                  applied.status ==
                      app::CreativeEditorRoomPlacementStatus::Applied &&
                  applied.generatedObjectCount == 6U,
              "second room corner applies the complete shell") &&
       expect(!editor.interaction.roomPlacement.active &&
                  appState.facade.document().objectCount() == 6U &&
                  cr::creativeUndoDepth(appState.history) == 1U &&
                  editor.worldLayout.source.rooms.size() == 1U &&
                  editor.worldLayout.generatedRevision ==
                      editor.worldLayout.revision,
              "room shell is one completed history transaction") &&
       expect(floor != nullptr && floor->kind == cr::CreativeObjectKind::Floor &&
                  sameBounds(floor->bounds,
                             {{0.0, -0.25, 0.0}, {3.0, 0.0, 2.0}}),
              "room floor ends exactly on the chosen plane") &&
       expect(north != nullptr && north->kind == cr::CreativeObjectKind::Wall &&
                  sameBounds(north->bounds,
                             {{0.0, 0.0, -0.125}, {3.0, 3.0, 0.125}}),
              "room wall starts exactly on the chosen plane") &&
       expect(linkedFloorSelection && linkedWallSelection &&
                  editor.worldLayout.selection.kind ==
                      app::CreativeEditorWorldLayoutSelectionKind::Room &&
                  editor.worldLayout.selection.index == 0U,
              "generated floor and wall select their semantic room") &&
       ok;

  const bool undo =
      app::undoLastEdit(appState, "room-undo", &editor.worldLayout);
  const bool undoRestored =
      undo && appState.facade.document().objectCount() == 0U &&
      editor.worldLayout.source.rooms.empty();
  const bool redo =
      app::redoLastEdit(appState, "room-redo", &editor.worldLayout);
  return expect(undoRestored,
                "one undo removes semantic source and generated shell") &&
         expect(redo && appState.facade.document().objectCount() == 6U &&
                    editor.worldLayout.source.rooms.size() == 1U,
                "one redo restores semantic source and generated shell") &&
         ok;
}

bool previewIsExactAndDoesNotMutate() {
  cr::CreativeAppState appState;
  app::CreativeEditorState editor = roomEditor();
  if (!expect(installDocument(appState, 302U),
              "room preview document installed")) {
    return false;
  }
  setRoomTarget(editor, -1, 4);
  static_cast<void>(app::advanceCreativeEditorRoomPlacement(
      appState, editor, "preview-first"));
  setRoomTarget(editor, 1, 6);
  const cr::CreativeRectangularRoomGeometryPlan preview =
      app::creativeEditorRoomPlacementPreview(appState, editor);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  const std::size_t appended =
      app::appendCreativeEditorRoomPlacementWireframe(
          appState, editor, 0.04F, lines);
  return expect(preview.accepted &&
                    sameBounds(preview.floorBounds,
                               {{-1.0, -0.25, 4.0}, {2.0, 0.0, 7.0}}),
                "room preview uses the same semantic geometry") &&
         expect(appended == 60U && lines.size() == 60U &&
                    lines.front().color.g == 1.0F,
                "room preview draws one floor and four green wall boxes") &&
         expect(appState.facade.document().objectCount() == 0U &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "room preview remains transient");
}

bool invalidSecondCornerPreservesDraftUntilCancel() {
  cr::CreativeAppState appState;
  app::CreativeEditorState editor = roomEditor();
  if (!expect(installDocument(appState, 303U),
              "invalid room document installed")) {
    return false;
  }
  setRoomTarget(editor, 0, 0, 0.0);
  static_cast<void>(app::advanceCreativeEditorRoomPlacement(
      appState, editor, "invalid-first"));
  setRoomTarget(editor, 3, 3, 1.0);
  const app::CreativeEditorRoomPlacementReceipt rejected =
      app::advanceCreativeEditorRoomPlacement(appState, editor,
                                              "invalid-second");
  return expect(!rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        app::CreativeEditorRoomPlacementStatus::
                            InvalidGeometry &&
                    editor.interaction.roomPlacement.active,
                "uneven second corner rejects without losing first corner") &&
         expect(appState.facade.document().objectCount() == 0U &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "invalid room geometry cannot partially mutate") &&
         expect(app::cancelCreativeEditorRoomPlacement(editor) &&
                    !editor.interaction.roomPlacement.active,
                "room draft cancels explicitly");
}

bool interruptionCancelsEmptyDraftWithoutHistory() {
  cr::CreativeAppState appState;
  app::CreativeEditorState editor = roomEditor();
  if (!expect(installDocument(appState, 304U),
              "interrupted room document installed")) {
    return false;
  }
  setRoomTarget(editor, 2, -3);
  static_cast<void>(app::advanceCreativeEditorRoomPlacement(
      appState, editor, "interrupted-first"));
  app::finalizeCreativeEditorContinuousGestures(
      appState, editor, "room_draft_interrupted");
  return expect(!editor.interaction.roomPlacement.active,
                "common interaction finalizer cancels the room draft") &&
         expect(appState.facade.document().objectCount() == 0U &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "interrupted room draft creates no object or history entry");
}

bool authoredAssetWorkspaceCannotConsumeMapLayoutState() {
  cr::CreativeAppState appState;
  app::CreativeEditorState editor = roomEditor();
  if (!expect(installDocument(appState, 305U),
              "asset-workspace room document installed")) {
    return false;
  }
  editor.assetEdit.active = true;
  setRoomTarget(editor, 0, 0);
  const app::CreativeEditorRoomPlacementReceipt rejected =
      app::advanceCreativeEditorRoomPlacement(appState, editor,
                                              "asset-room-rejected");
  return expect(!rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        app::CreativeEditorRoomPlacementStatus::InvalidTarget,
                "room tool rejects the authored-asset workspace") &&
         expect(editor.worldLayout.source.rooms.empty() &&
                    appState.facade.document().objectCount() == 0U &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "asset workspace cannot mutate main-map semantic state");
}

}  // namespace

int main() {
  const bool ok = twoCornersApplyOneUndoableShell() &&
                  previewIsExactAndDoesNotMutate() &&
                  invalidSecondCornerPreservesDraftUntilCancel() &&
                  interruptionCancelsEmptyDraftWithoutHistory() &&
                  authoredAssetWorkspaceCannotConsumeMapLayoutState();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

#include "creative_desktop_command_test_runners.hpp"
#include "creative_desktop_command_test_support.hpp"

namespace {

bool worldLayoutLevelCommandsRouteThroughDispatcher() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Building Levels");
  static_cast<void>(document.assignId(425U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {6, 4}}, 0.0, 3U, 0.25, 1U});
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const std::uint64_t revisionBeforeAdd = editor.worldLayout.revision;
  const auto added = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutLevelOperation, context,
      app::CreativeDesktopWorldLayoutLevelOperationPayload{
          app::CreativeEditorWorldLayoutLevelOperation::Add, 0U, 0U});
  const std::uint64_t revisionAfterAdd = editor.worldLayout.revision;
  const auto selected = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutLevelOperation, context,
      app::CreativeDesktopWorldLayoutLevelOperationPayload{
          app::CreativeEditorWorldLayoutLevelOperation::Select, 0U, 0U});

  return expect(shell.accepted && shell.changed,
                "level command test creates a building shell") &&
         expect(added.accepted && added.changed &&
                    added.worldLayoutChanged && !added.sceneChanged &&
                    editor.worldLayout.source.levels.size() == 2U &&
                    editor.worldLayout.source.rooms.size() == 1U &&
                    revisionAfterAdd == revisionBeforeAdd + 1U,
                "add level routes as one semantic source mutation") &&
         expect(selected.accepted && selected.changed &&
                    !selected.worldLayoutChanged && !selected.sceneChanged &&
                    editor.worldLayout.activeLevelIndex == 0U &&
                    editor.worldLayout.revision == revisionAfterAdd,
                "select level changes only the active editor view");
}

bool worldLayoutSourceScopeSelectionDoesNotMoveTheCanvas() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Source Scope");
  static_cast<void>(document.assignId(440U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {6, 4}}, 0.0, 3U, 0.25, 1U});
  if (!shell.accepted || editor.worldLayout.source.rooms.empty()) {
    return expect(false, "source scope fixture creates a room");
  }
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Building, 0U};
  editor.worldLayout.canvasPanX = 17.0F;
  editor.worldLayout.canvasPanZ = -9.0F;
  const std::string roomKey = editor.worldLayout.source.rooms[0].stableKey;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult previewed = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutPreview, context);
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!previewed.accepted || !generated.accepted) {
    return expect(false, "source scope fixture generates 3D output");
  }
  const app::CreativeDesktopCommandResult selected = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope, context,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Room, 0U, roomKey, 0U});
  const cr::CreativeSelectionState& objectSelection =
      appState.facade.selectionState();
  const std::span<const cr::TargetRef> selectedTargets =
      cr::selectedTargetList(objectSelection);
  const bool generatedMembersSelected =
      !selectedTargets.empty() &&
      selectedTargets.size() ==
          editor.generatedSourceScopeCache.objectIds.size() &&
      std::all_of(
          selectedTargets.begin(), selectedTargets.end(),
          [&](cr::TargetRef target) {
            const cr::CreativeObject* object =
                appState.facade.document().findObject(target.value);
            return object != nullptr &&
                   app::creativeDesktopGeneratedObjectBelongsToSourceScope(
                       editor.worldLayout.source, *object,
                       cr::CreativeWorldLayoutTable::Room, 0U);
          });
  const bool selectedWithoutPan =
      selected.accepted && selected.changed &&
      editor.worldLayout.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Room &&
      editor.worldLayout.selection.index == 0U &&
      editor.worldLayout.activeLevelIndex == 0U &&
      editor.worldLayout.canvasPanX == 17.0F &&
      editor.worldLayout.canvasPanZ == -9.0F && generatedMembersSelected &&
      objectSelection.selectedTarget.value ==
          editor.generatedSourceScopeCache.objectIds.front();
  const app::CreativeDesktopCommandResult wrongFloor = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope, context,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Room, 0U, roomKey, 99U});
  const app::CreativeDesktopCommandResult stale = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope, context,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Level, 0U, "stale_level"});
  const app::CreativeDesktopCommandResult focused = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutFocusSource, context,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Room, 0U, roomKey});
  const bool staleAndFloorMismatchPreservedSelection =
      !stale.accepted && !stale.changed && !wrongFloor.accepted &&
      !wrongFloor.changed && editor.worldLayout.selection.kind ==
                                 app::CreativeEditorWorldLayoutSelectionKind::Room &&
      editor.worldLayout.activeLevelIndex == 0U &&
      cr::selectedTargetCount(appState.facade.selectionState()) ==
          selectedTargets.size();
  const bool focusMovedCanvas =
      focused.accepted && editor.worldLayout.canvasPanX != 17.0F;
  const app::CreativeDesktopCommandResult cleared = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutClearSelection, context);

  return expect(selectedWithoutPan,
                "scope selection synchronizes generated objects without moving "
                "the canvas") &&
         expect(staleAndFloorMismatchPreservedSelection,
                "invalid scope commands preserve 2D and 3D selection") &&
         expect(focusMovedCanvas,
                "explicit focus remains the only scope action that pans") &&
         expect(cleared.accepted && cleared.changed &&
                    editor.worldLayout.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::None &&
                    cr::selectedTargetCount(appState.facade.selectionState()) ==
                        0U,
                "clearing the plan selection clears generated object selection");
}

bool objectSelectionSynchronizesGeneratedSourcesAcrossViews() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Generated Selection Sync");
  static_cast<void>(document.assignId(443U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {6, 4}}, 0.0, 3U, 0.25, 1U});
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const auto previewed = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutPreview, context);
  const auto generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!shell.accepted || !previewed.accepted || !generated.accepted ||
      editor.worldLayout.source.rooms.empty()) {
    return expect(false, "generated selection fixture builds one room");
  }

  std::vector<cr::CreativeObjectId> roomObjectIds;
  cr::CreativeObjectId directRoomObjectId = cr::kInvalidObjectId;
  for (const cr::CreativeObject& object : appState.facade.document().objects()) {
    if (!cr::creativeWorldLayoutObjectBelongsToSource(
            editor.worldLayout.source, object,
            cr::CreativeWorldLayoutTable::Room, 0U)) {
      continue;
    }
    roomObjectIds.push_back(object.id);
    const cr::CreativeWorldLayoutObjectProvenance provenance =
        cr::resolveCreativeWorldLayoutObjectProvenance(
            editor.worldLayout.source, object);
    if (provenance.table == cr::CreativeWorldLayoutTable::Room) {
      directRoomObjectId = object.id;
    }
  }
  if (roomObjectIds.empty() || directRoomObjectId == cr::kInvalidObjectId) {
    return expect(false,
                  "generated selection fixture exposes direct room output");
  }

  const auto roomSelected = dispatchPayload(
      app::CreativeDesktopCommandId::SelectObjects, context,
      app::CreativeDesktopSelectPayload{roomObjectIds, roomObjectIds.front()});
  const bool roomSynchronized =
      roomSelected.accepted &&
      cr::selectedTargetCount(appState.facade.selectionState()) ==
          roomObjectIds.size() &&
      editor.worldLayout.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Room &&
      editor.worldLayout.selection.index == 0U;

  const cr::CreativeObjectId authoredObjectId =
      createCrate(appState.facade, 20.0);
  const auto mixed = dispatchPayload(
      app::CreativeDesktopCommandId::SelectObjects, context,
      app::CreativeDesktopSelectPayload{
          {directRoomObjectId, authoredObjectId}, authoredObjectId});
  const bool mixedClearsSource =
      mixed.accepted &&
      cr::selectedTargetCount(appState.facade.selectionState()) == 2U &&
      editor.worldLayout.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::None;

  const auto hidden = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectsVisible, context,
      app::CreativeDesktopObjectFlagPayload{{directRoomObjectId}, false});
  const auto locked = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectsLocked, context,
      app::CreativeDesktopObjectFlagPayload{{directRoomObjectId}, true});
  const cr::CreativeDocumentMutationReceipt fixtureHidden =
      appState.facade.mutateObject(
          directRoomObjectId, cr::CreativeMutationKind::SetVisible,
          cr::makeVisibilityPayload(false));
  const cr::CreativeDocumentMutationReceipt fixtureLocked =
      appState.facade.mutateObject(
          directRoomObjectId, cr::CreativeMutationKind::SetLocked,
          cr::makeLockPayload(true));
  const auto inspected = dispatchPayload(
      app::CreativeDesktopCommandId::SelectObjects, context,
      app::CreativeDesktopSelectPayload{{directRoomObjectId},
                                        directRoomObjectId});
  const std::uint64_t documentRevisionBeforeRejectedDelete =
      appState.facade.document().revision();
  const std::uint64_t worldLayoutRevisionBeforeRejectedDelete =
      editor.worldLayout.revision;
  const std::size_t roomCountBeforeRejectedDelete =
      editor.worldLayout.source.rooms.size();
  const auto rejectedDelete =
      dispatchOne(app::CreativeDesktopCommandId::DeleteSelection, context);
  const cr::CreativeObject* directRoomObject =
      appState.facade.findObject(directRoomObjectId);
  const bool hiddenLockedRemainsInspectable =
      !hidden.accepted && !locked.accepted &&
      cr::documentMutationSucceeded(fixtureHidden.status) &&
      fixtureHidden.changed &&
      cr::documentMutationSucceeded(fixtureLocked.status) &&
      fixtureLocked.changed && inspected.accepted &&
      directRoomObject != nullptr && !directRoomObject->visible &&
      directRoomObject->locked &&
      appState.facade.selectionState().selectedTarget.value ==
          static_cast<cr::Id>(directRoomObjectId) &&
      editor.worldLayout.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Room &&
      editor.worldLayout.selection.index == 0U;
  const bool lockedGeneratedDeleteFailsBeforeMutation =
      !rejectedDelete.accepted && !rejectedDelete.changed &&
      rejectedDelete.message == "selection is locked" &&
      rejectedDelete.objectAction.action ==
          cr::CreativeSemanticObjectAction::Delete &&
      rejectedDelete.objectAction.status ==
          app::CreativeEditorObjectActionOutcomeStatus::Rejected &&
      rejectedDelete.objectAction.reasonCode ==
          "creative_editor_object_action_selection_locked" &&
      app::creativeEditorObjectActionOutcomeValid(
          rejectedDelete.objectAction) &&
      appState.facade.document().revision() ==
          documentRevisionBeforeRejectedDelete &&
      editor.worldLayout.revision ==
          worldLayoutRevisionBeforeRejectedDelete &&
      editor.worldLayout.source.rooms.size() ==
          roomCountBeforeRejectedDelete &&
      appState.facade.findObject(directRoomObjectId) != nullptr;

  return expect(roomSynchronized,
                "Outliner object selection synchronizes its common 2D room") &&
         expect(mixedClearsSource,
                "mixed authored and generated selection clears 2D source") &&
         expect(hiddenLockedRemainsInspectable,
                "hidden locked generated output remains Outliner-selectable") &&
         expect(lockedGeneratedDeleteFailsBeforeMutation,
                "locked generated output rejects deletion before either owner "
                "mutates");
}

bool desktopActionContextCachesAndTracksSelectionIdentity() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Desktop Action Context");
  static_cast<void>(document.assignId(444U));
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = "Unlocked";
  const cr::CreativeObjectId unlocked = document.createObject(request).objectId;
  request.name = "Locked";
  request.locked = true;
  request.hasLockedOverride = true;
  const cr::CreativeObjectId locked = document.createObject(request).objectId;

  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  const std::array firstSelection{unlocked};
  static_cast<void>(appState.facade.selectTargets(firstSelection, unlocked));
  const app::CreativeDesktopObjectActionContext& first =
      app::refreshCreativeEditorDesktopObjectActionContext(
          editor.desktopUi, appState, &editor.worldLayout);
  const bool firstAllowsDelete =
      cr::creativeSemanticObjectActionAdmission(
          first.admissions, cr::CreativeSemanticObjectAction::Delete)
          .allowed;
  const std::uint64_t firstBuildCount = first.rebuildCount;
  const app::CreativeDesktopObjectActionContext& reused =
      app::refreshCreativeEditorDesktopObjectActionContext(
          editor.desktopUi, appState, &editor.worldLayout);
  const std::uint64_t reusedBuildCount = reused.rebuildCount;

  const std::array replacementSelection{locked};
  static_cast<void>(
      appState.facade.selectTargets(replacementSelection, locked));
  const app::CreativeDesktopObjectActionContext& replaced =
      app::refreshCreativeEditorDesktopObjectActionContext(
          editor.desktopUi, appState, &editor.worldLayout);
  const cr::CreativeSemanticObjectActionAdmission lockedDelete =
      cr::creativeSemanticObjectActionAdmission(
          replaced.admissions, cr::CreativeSemanticObjectAction::Delete);
  const std::uint64_t replacementBuildCount = replaced.rebuildCount;

  ++editor.worldLayout.sourceEpoch;
  const app::CreativeDesktopObjectActionContext& sourceMoved =
      app::refreshCreativeEditorDesktopObjectActionContext(
          editor.desktopUi, appState, &editor.worldLayout);

  return expect(firstAllowsDelete && firstBuildCount == 1U &&
                    reusedBuildCount == firstBuildCount,
                "unchanged desktop action context reuses one semantic snapshot") &&
         expect(!lockedDelete.allowed &&
                    lockedDelete.status ==
                        cr::CreativeSemanticObjectActionAdmissionStatus::
                            SelectionLocked &&
                    replacementBuildCount == firstBuildCount + 1U,
                "same-count selection identity change rebuilds lock admission") &&
         expect(sourceMoved.rebuildCount == replacementBuildCount + 1U &&
                    sourceMoved.worldLayoutSourceEpoch ==
                        editor.worldLayout.sourceEpoch,
                "World Layout source epoch invalidates desktop admission");
}

bool worldLayoutSourceScopeFramesThe3dCameraWithoutMutatingSource() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Source Scope Frame");
  static_cast<void>(document.assignId(441U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {12, 8}}, 0.0, 4U, 0.25, 1U});
  if (!shell.accepted || editor.worldLayout.source.rooms.empty()) {
    return expect(false, "source scope frame fixture creates a room");
  }

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult previewed = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutPreview, context);
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!previewed.accepted || !generated.accepted) {
    return expect(false, "source scope frame fixture generates 3D output");
  }

  const std::string roomKey = editor.worldLayout.source.rooms[0].stableKey;
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Room, 0U};
  editor.worldLayout.canvasPanX = 13.0F;
  editor.worldLayout.canvasPanZ = -7.0F;
  editor.flyPos = {50.0F, 30.0F, 50.0F};
  editor.yawDegrees = 28.0F;
  editor.pitchDegrees = -18.0F;
  editor.desktopUi.contentViewport = {0U, 0U, 1200U, 720U};
  const iggy3d::Vec3 cameraBefore = editor.flyPos;
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const app::CreativeDesktopCommandResult framed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutFrameSourceScope3D, context,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Room, 0U, roomKey});
  const iggy3d::Vec3 cameraAfterFrame = editor.flyPos;
  const bool framingPreservedSource =
      appState.facade.document().revision() == documentRevisionBefore &&
      editor.worldLayout.revision == sourceRevisionBefore;

  const app::CreativeDesktopCommandResult stale = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutFrameSourceScope3D, context,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Room, 0U, "stale_room"});
  const bool stalePreservedCamera =
      editor.flyPos.x == cameraAfterFrame.x &&
      editor.flyPos.y == cameraAfterFrame.y &&
      editor.flyPos.z == cameraAfterFrame.z;

  ++editor.worldLayout.revision;
  const app::CreativeDesktopCommandResult pending = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutFrameSourceScope3D, context,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Room, 0U, roomKey});
  const app::CreativeDesktopCommandResult mismatch = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutFrameSourceScope3D, context,
      std::monostate{});
  const bool rejectedRequestsPreservedCamera =
      editor.flyPos.x == cameraAfterFrame.x &&
      editor.flyPos.y == cameraAfterFrame.y &&
      editor.flyPos.z == cameraAfterFrame.z;

  return expect(framed.accepted && framed.changed &&
                    framed.affectedObjectCount > 0U &&
                    editor.generatedSourceScopeCache.summary.valid &&
                    editor.generatedSourceScopeCache.summary.hasBounds,
                "source scope command frames its generated member bounds") &&
         expect(cameraAfterFrame.x != cameraBefore.x ||
                    cameraAfterFrame.y != cameraBefore.y ||
                    cameraAfterFrame.z != cameraBefore.z,
                "source scope framing moves the 3D camera anchor") &&
         expect(editor.yawDegrees == 28.0F && editor.pitchDegrees == -18.0F &&
                    editor.worldLayout.canvasPanX == 13.0F &&
                    editor.worldLayout.canvasPanZ == -7.0F &&
                    editor.worldLayout.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::Room &&
                    editor.worldLayout.selection.index == 0U &&
                    framingPreservedSource &&
                    appState.facade.document().revision() ==
                        documentRevisionBefore &&
                    sourceRevisionBefore + 1U == editor.worldLayout.revision,
                "3D framing preserves view direction, 2D focus, and source data") &&
         expect(!stale.accepted && stalePreservedCamera &&
                    rejectedRequestsPreservedCamera &&
                    stale.message == "layout source frame: stale target" &&
                    !pending.accepted &&
                    pending.message ==
                        "layout source frame: generate pending edits" &&
                    !mismatch.accepted &&
                    mismatch.message ==
                        "layout source frame: payload mismatch",
                "scope framing rejects stale, pending, and mistyped requests");
}

bool worldLayoutStructuralCommandsRouteThroughDispatcher() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Structural Layout");
  static_cast<void>(document.assignId(423U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const auto setFloorTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Floor});
  const auto floorBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {0.0, 0.0}});
  const auto floorCreate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {4.0, 3.0}});
  const std::uint64_t revisionBeforeFloorSettings = editor.worldLayout.revision;
  const auto floorSettings = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetBoxSettings, context,
      app::CreativeDesktopWorldLayoutBoxSettingsPayload{
          0U, {{{0, 0}, {4, 3}}, 1, 2U}});
  const bool floorSettingsCommittedOnce =
      editor.worldLayout.revision == revisionBeforeFloorSettings + 1U;
  static_cast<void>(dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Select}));
  const std::uint64_t revisionBeforeFloorMove = editor.worldLayout.revision;
  const auto floorMoveBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBox, context,
      app::CreativeDesktopWorldLayoutBoxManipulationPayload{
          app::CreativeEditorWorldLayoutBoxManipulationPhase::Begin,
          {2.0, 1.5}, 0.2});
  const auto floorMoveUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBox, context,
      app::CreativeDesktopWorldLayoutBoxManipulationPayload{
          app::CreativeEditorWorldLayoutBoxManipulationPhase::Update,
          {4.2, 2.6}, 0.2});
  const bool floorMovePreviewOnly =
      editor.worldLayout.revision == revisionBeforeFloorMove &&
      editor.worldLayout.source.boxes[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0} &&
      editor.worldLayout.boxManipulation.previewFootprint.minimum ==
          cr::CreativeTerrainCoord2{2, 1};
  const auto floorMoveCommit = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBox, context,
      app::CreativeDesktopWorldLayoutBoxManipulationPayload{
          app::CreativeEditorWorldLayoutBoxManipulationPhase::Commit,
          {4.2, 2.6}, 0.2});
  const bool floorMoveCommittedOnce =
      editor.worldLayout.revision == revisionBeforeFloorMove + 1U;

  const auto setWallTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Wall});
  const auto wallBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {0.0, 0.0}});
  const auto wallCreate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {8.0, 0.0}});
  const std::uint64_t revisionBeforeWallSettings = editor.worldLayout.revision;
  const auto wallSettings = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetWallSettings, context,
      app::CreativeDesktopWorldLayoutWallSettingsPayload{
          0U, {{0, 0}, {8, 0}, 0.5, 4U, 0.5}});
  const bool wallSettingsCommittedOnce =
      editor.worldLayout.revision == revisionBeforeWallSettings + 1U;
  static_cast<void>(dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Select}));
  const std::uint64_t revisionBeforeWallMove = editor.worldLayout.revision;
  const auto wallMoveBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateWall, context,
      app::CreativeDesktopWorldLayoutWallManipulationPayload{
          app::CreativeEditorWorldLayoutWallManipulationPhase::Begin,
          {4.0, 0.0}, 0.2});
  const auto wallMoveUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateWall, context,
      app::CreativeDesktopWorldLayoutWallManipulationPayload{
          app::CreativeEditorWorldLayoutWallManipulationPhase::Update,
          {5.2, 2.1}, 0.2});
  const bool wallMovePreviewOnly =
      editor.worldLayout.revision == revisionBeforeWallMove &&
      editor.worldLayout.source.walls[0].start ==
          cr::CreativeTerrainCoord2{0, 0} &&
      editor.worldLayout.wallManipulation.previewStart ==
          cr::CreativeTerrainCoord2{1, 2};
  const auto wallMoveCommit = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateWall, context,
      app::CreativeDesktopWorldLayoutWallManipulationPayload{
          app::CreativeEditorWorldLayoutWallManipulationPhase::Commit,
          {5.2, 2.1}, 0.2});
  const cr::CreativeWorldLayoutRect floorAfterMove =
      editor.worldLayout.source.boxes[0].footprint;
  const cr::CreativeWorldLayoutWall wallAfterMove =
      editor.worldLayout.source.walls[0];
  const std::uint64_t revisionAfterWallMove = editor.worldLayout.revision;

  const auto buildingSelected = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSelectBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingSelectionPayload{0U});
  const std::uint64_t revisionBeforeBuildingMove =
      editor.worldLayout.revision;
  const auto buildingMoveBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingManipulationPayload{
          app::CreativeEditorWorldLayoutBuildingManipulationPhase::Begin,
          {4.0, 2.0}, 0.2});
  const auto buildingMoveUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingManipulationPayload{
          app::CreativeEditorWorldLayoutBuildingManipulationPhase::Update,
          {6.2, 3.1}, 0.2});
  const bool buildingMovePreviewOnly =
      editor.worldLayout.revision == revisionBeforeBuildingMove &&
      editor.worldLayout.source.walls[0].start ==
          cr::CreativeTerrainCoord2{1, 2} &&
      editor.worldLayout.buildingManipulation.previewDeltaXCells == 2 &&
      editor.worldLayout.buildingManipulation.previewDeltaZCells == 1;
  const auto buildingMoveCommit = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingManipulationPayload{
          app::CreativeEditorWorldLayoutBuildingManipulationPhase::Commit,
          {6.2, 3.1}, 0.2});
  const bool buildingMoveCommittedOnce =
      editor.worldLayout.revision == revisionBeforeBuildingMove + 1U;
  std::int64_t duplicateDeltaX = 0;
  std::int64_t duplicateDeltaZ = 0;
  const bool hasDuplicateOffset =
      app::defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
          editor.worldLayout, 0U, duplicateDeltaX, duplicateDeltaZ);
  const std::uint64_t revisionBeforeBuildingDuplicate =
      editor.worldLayout.revision;
  const auto buildingDuplicated = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutDuplicateBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingDuplicatePayload{
          0U, duplicateDeltaX, duplicateDeltaZ});
  const bool buildingDuplicatedOnce =
      editor.worldLayout.revision == revisionBeforeBuildingDuplicate + 1U;
  const std::uint64_t revisionBeforeBuildingTransform =
      editor.worldLayout.revision;
  const auto buildingTransformPreview = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutTransformBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingTransformPayload{
          app::CreativeEditorWorldLayoutBuildingTransformPhase::Preview,
          cr::CreativeWorldLayoutBuildingTransformOperation::MirrorZ});
  const bool buildingTransformPreviewOnly =
      buildingTransformPreview.accepted && buildingTransformPreview.changed &&
      !buildingTransformPreview.worldLayoutChanged &&
      editor.worldLayout.revision == revisionBeforeBuildingTransform &&
      editor.worldLayout.source.walls[1].start ==
          cr::CreativeTerrainCoord2{13, 3} &&
      app::creativeEditorWorldLayoutDisplaySource(editor.worldLayout)
              .walls[1]
              .start == cr::CreativeTerrainCoord2{13, 4};
  const auto buildingTransformCommit = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutTransformBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingTransformPayload{
          app::CreativeEditorWorldLayoutBuildingTransformPhase::Commit,
          cr::CreativeWorldLayoutBuildingTransformOperation::MirrorZ});
  const bool buildingTransformCommittedOnce =
      buildingTransformCommit.accepted && buildingTransformCommit.changed &&
      buildingTransformCommit.worldLayoutChanged &&
      editor.worldLayout.revision == revisionBeforeBuildingTransform + 1U &&
      editor.worldLayout.source.walls[1].start ==
          cr::CreativeTerrainCoord2{13, 4};
  const auto groupModeCleared = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutClearSelection, context);

  return expect(setFloorTool.accepted && floorBegin.accepted &&
                    floorCreate.accepted && floorCreate.worldLayoutChanged &&
                    floorSettings.accepted && floorSettings.changed &&
                    floorSettings.worldLayoutChanged &&
                    floorSettingsCommittedOnce && floorMoveBegin.accepted &&
                    floorMoveBegin.changed &&
                    !floorMoveBegin.worldLayoutChanged &&
                    floorMoveUpdate.accepted && floorMoveUpdate.changed &&
                    !floorMoveUpdate.worldLayoutChanged &&
                    floorMovePreviewOnly && floorMoveCommit.accepted &&
                    floorMoveCommit.changed &&
                    floorMoveCommit.worldLayoutChanged &&
                    floorMoveCommittedOnce &&
                    floorAfterMove.minimum ==
                        cr::CreativeTerrainCoord2{2, 1} &&
                    editor.worldLayout.source.boxes[0].anchorLayer == 1.0 &&
                    editor.worldLayout.source.boxes[0].layerCount == 2U,
                "floor settings and manipulation use typed dispatcher commands") &&
         expect(setWallTool.accepted && wallBegin.accepted &&
                    wallCreate.accepted && wallCreate.worldLayoutChanged &&
                    wallSettings.accepted && wallSettings.changed &&
                    wallSettings.worldLayoutChanged &&
                    wallSettingsCommittedOnce && wallMoveBegin.accepted &&
                    wallMoveBegin.changed &&
                    !wallMoveBegin.worldLayoutChanged &&
                    wallMoveUpdate.accepted && wallMoveUpdate.changed &&
                    !wallMoveUpdate.worldLayoutChanged &&
                    wallMovePreviewOnly && wallMoveCommit.accepted &&
                    wallMoveCommit.changed &&
                    wallMoveCommit.worldLayoutChanged &&
                    revisionAfterWallMove == revisionBeforeWallMove + 1U &&
                    wallAfterMove.start ==
                        cr::CreativeTerrainCoord2{1, 2} &&
                    wallAfterMove.end ==
                        cr::CreativeTerrainCoord2{9, 2} &&
                    wallAfterMove.baseLayer == 0.5 &&
                    wallAfterMove.heightCells == 4U &&
                    wallAfterMove.thicknessCells == 0.5,
                "partition settings and manipulation use typed dispatcher commands") &&
         expect(buildingSelected.accepted && buildingSelected.changed &&
                    buildingMoveBegin.accepted && buildingMoveBegin.changed &&
                    !buildingMoveBegin.worldLayoutChanged &&
                    buildingMoveUpdate.accepted &&
                    buildingMoveUpdate.changed &&
                    !buildingMoveUpdate.worldLayoutChanged &&
                    buildingMovePreviewOnly && buildingMoveCommit.accepted &&
                    buildingMoveCommit.changed &&
                    buildingMoveCommit.worldLayoutChanged &&
                    buildingMoveCommittedOnce && hasDuplicateOffset &&
                    duplicateDeltaX == 10 && duplicateDeltaZ == 0 &&
                    buildingDuplicated.accepted &&
                    buildingDuplicated.changed &&
                    buildingDuplicated.worldLayoutChanged &&
                    buildingDuplicatedOnce &&
                    editor.worldLayout.source.buildings.size() == 2U &&
                    editor.worldLayout.source.boxes.size() == 2U &&
                    editor.worldLayout.source.walls.size() == 2U &&
                    editor.worldLayout.source.boxes[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{4, 2} &&
                    editor.worldLayout.source.boxes[1].footprint.minimum ==
                        cr::CreativeTerrainCoord2{14, 2} &&
                    editor.worldLayout.source.walls[0].start ==
                        cr::CreativeTerrainCoord2{3, 3} &&
                    editor.worldLayout.source.walls[1].start ==
                        cr::CreativeTerrainCoord2{13, 4} &&
                    buildingTransformPreviewOnly &&
                    buildingTransformCommittedOnce &&
                    groupModeCleared.accepted && groupModeCleared.changed &&
                    editor.worldLayout.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::None,
                "building group commands preview, commit, duplicate, and clear semantically");
}

bool synchronizedStructuralPreviewsShareInspectionSource() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Synchronized Building Transform");
  static_cast<void>(document.assignId(452U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "synchronized_building_transform");
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {12, 8}}, 0.0, 4U, 0.25, 1U});
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const auto previewed = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutPreview, context);
  const auto generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Select));
  const auto selected = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSelectBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingSelectionPayload{0U});
  if (!shell.accepted || !previewed.accepted || !generated.accepted ||
      !selected.accepted) {
    return expect(false, "synchronized building transform fixture is ready");
  }

  const std::uint64_t revisionBefore = editor.worldLayout.revision;
  const auto authoredFingerprint = cr::fingerprintCreativeWorldLayoutBuilding(
      editor.worldLayout.source, 0U);
  const auto transformed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutTransformBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingTransformPayload{
          app::CreativeEditorWorldLayoutBuildingTransformPhase::Preview,
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
  const app::CreativeEditorWorldLayoutInspection inspection =
      app::inspectCreativeEditorWorldLayout(editor.worldLayout);
  const auto candidateFingerprint = cr::fingerprintCreativeWorldLayoutBuilding(
      editor.worldLayout.buildingTransform.candidate, 0U);
  const bool onePreviewSource =
      transformed.accepted && transformed.changed &&
      !transformed.worldLayoutChanged && transformed.sceneChanged &&
      editor.worldLayout.revision == revisionBefore &&
      cr::fingerprintCreativeWorldLayoutBuilding(editor.worldLayout.source,
                                                 0U) ==
          authoredFingerprint &&
      inspection.source == &editor.worldLayout.previewSource &&
      inspection.sourceKind ==
          app::CreativeEditorWorldLayoutInspectionSourceKind::Preview &&
      inspection.previewValidity ==
          app::CreativeEditorWorldLayoutPreviewValidity::Valid &&
      !inspection.volatileSource &&
      &app::creativeEditorWorldLayoutDisplaySource(editor.worldLayout) ==
          inspection.source &&
      cr::fingerprintCreativeWorldLayoutBuilding(*inspection.source, 0U) ==
          candidateFingerprint &&
      &app::creativeEditorWorldLayoutRenderDocument(
           editor.worldLayout, appState.facade.document()) ==
          &editor.worldLayout.preview.document;
  const std::uint64_t previewContentRevisionBeforeRepeat =
      editor.worldLayout.previewContentRevision;
  const auto repeatedTransform = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutTransformBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingTransformPayload{
          app::CreativeEditorWorldLayoutBuildingTransformPhase::Preview,
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
  const bool repeatedTransformReusesPreview =
      repeatedTransform.accepted && !repeatedTransform.changed &&
      !repeatedTransform.worldLayoutChanged && !repeatedTransform.sceneChanged &&
      editor.worldLayout.previewContentRevision ==
          previewContentRevisionBeforeRepeat;

  const auto cancelled = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutTransformBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingTransformPayload{
          app::CreativeEditorWorldLayoutBuildingTransformPhase::Cancel,
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
  const app::CreativeEditorWorldLayoutInspection authoredInspection =
      app::inspectCreativeEditorWorldLayout(editor.worldLayout);
  const bool cancelRestoresAuthoredInspection =
      cancelled.accepted && cancelled.changed && cancelled.sceneChanged &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      !editor.worldLayout.liveEditPreviewVisible &&
      authoredInspection.source == &editor.worldLayout.source &&
      authoredInspection.sourceKind ==
          app::CreativeEditorWorldLayoutInspectionSourceKind::Authored &&
      authoredInspection.previewValidity ==
          app::CreativeEditorWorldLayoutPreviewValidity::None &&
      cr::fingerprintCreativeWorldLayoutBuilding(editor.worldLayout.source,
                                                 0U) ==
          authoredFingerprint;

  const auto roomBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoom, context,
      app::CreativeDesktopWorldLayoutRoomManipulationPayload{
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
          {6.0, 4.0}, 0.25});
  const auto roomUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoom, context,
      app::CreativeDesktopWorldLayoutRoomManipulationPayload{
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Update,
          {8.0, 5.0}, 0.25});
  const app::CreativeEditorWorldLayoutInspection roomInspection =
      app::inspectCreativeEditorWorldLayout(editor.worldLayout);
  const auto roomCandidateFingerprint =
      cr::fingerprintCreativeWorldLayoutBuilding(
          editor.worldLayout.roomManipulation.previewEdit.edited, 0U);
  const bool roomPreviewUsesCompiledSource =
      roomBegin.accepted && roomUpdate.accepted && roomUpdate.changed &&
      roomUpdate.sceneChanged && editor.worldLayout.roomManipulation.active &&
      roomInspection.source == &editor.worldLayout.previewSource &&
      roomInspection.sourceKind ==
          app::CreativeEditorWorldLayoutInspectionSourceKind::Preview &&
      roomInspection.previewValidity ==
          app::CreativeEditorWorldLayoutPreviewValidity::Valid &&
      cr::fingerprintCreativeWorldLayoutBuilding(*roomInspection.source, 0U) ==
          roomCandidateFingerprint &&
      &app::creativeEditorWorldLayoutRenderDocument(
           editor.worldLayout, appState.facade.document()) ==
          &editor.worldLayout.preview.document;
  const auto roomCancelled = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoom, context,
      app::CreativeDesktopWorldLayoutRoomManipulationPayload{
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Cancel,
          {}, 0.25});

  return expect(onePreviewSource,
                "building transform shares one exact source across Plan Elevation and 3D") &&
         expect(repeatedTransformReusesPreview,
                "identical building transform preview reuses the exact source") &&
         expect(cancelRestoresAuthoredInspection,
                "building transform cancel restores the authored inspection source") &&
         expect(roomPreviewUsesCompiledSource,
                "room drag inspection prefers its compiled exact preview") &&
         expect(roomCancelled.accepted && roomCancelled.sceneChanged,
                "room drag cancel closes its exact inspection preview");
}

bool worldLayoutVerticalConnectorCommandsRouteThroughDispatcher() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Vertical Connector Layout");
  static_cast<void>(document.assignId(426U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "connector_commands");
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      editor.worldLayout, {{{0, 0}, {8, 8}}, 0.0, 4U, 0.25, 1U});
  const auto upperLevel = app::applyCreativeEditorWorldLayoutLevelOperation(
      editor.worldLayout,
      app::CreativeEditorWorldLayoutLevelOperation::Add, 0U);
  if (!shell.accepted || !upperLevel.accepted ||
      editor.worldLayout.source.rooms.empty()) {
    return expect(false, "vertical connector command test setup");
  }
  cr::CreativeWorldLayoutRoom upperRoom =
      editor.worldLayout.source.rooms.front();
  upperRoom.levelIndex = 1U;
  upperRoom.stableKey = "command_upper_room";
  upperRoom.name = "Command Upper Room";
  editor.worldLayout.source.rooms.push_back(std::move(upperRoom));
  editor.worldLayout.activeLevelIndex = 0U;
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Stair));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      editor.worldLayout,
      app::CreativeEditorWorldLayoutGesturePhase::Begin, {1.0, 1.0}));
  const auto connector = app::applyCreativeEditorWorldLayoutGesture(
      editor.worldLayout,
      app::CreativeEditorWorldLayoutGesturePhase::Commit, {7.0, 7.0});
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Select));
  const auto preview = app::previewCreativeEditorWorldLayout(
      editor.worldLayout, appState.facade.document());

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  app::CreativeEditorWorldLayoutVerticalConnectorSettings settings;
  const bool settingsRead =
      app::readCreativeEditorWorldLayoutVerticalConnectorSettings(
          editor.worldLayout, 0U, settings);
  settings.kind = cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  const std::uint64_t revisionBeforeSettings =
      editor.worldLayout.revision;
  const auto settingsApplied = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty,
      context,
      app::CreativeDesktopWorldLayoutPropertyEditPayload{
          app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit,
          cr::CreativeWorldLayoutTable::VerticalConnector, 0U,
          editor.worldLayout.source.verticalConnectors[0].stableKey,
          settings});
  const bool settingsCommittedOnce =
      connector.accepted && preview.accepted && settingsRead &&
      settingsApplied.accepted && settingsApplied.changed &&
      settingsApplied.worldLayoutChanged && settingsApplied.sceneChanged &&
      editor.worldLayout.revision == revisionBeforeSettings + 1U &&
      editor.worldLayout.source.verticalConnectors[0].kind ==
          cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;

  app::CreativeEditorWorldLayoutPoint directionHandle;
  const bool handleResolved =
      app::resolveCreativeEditorWorldLayoutVerticalConnectorDirectionHandle(
          editor.worldLayout.source.verticalConnectors[0].footprint,
          editor.worldLayout.source.verticalConnectors[0].direction,
          directionHandle);
  const std::uint64_t revisionBeforeDirection =
      editor.worldLayout.revision;
  const auto directionBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
      context,
      app::CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload{
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Begin,
          directionHandle, 0.2, {}});
  const auto directionUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
      context,
      app::CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload{
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Update,
          {3.0, 0.0}, 0.2, {}});
  const bool directionPreviewOnly =
      handleResolved && directionBegin.accepted && directionBegin.changed &&
      !directionBegin.worldLayoutChanged && directionUpdate.accepted &&
      directionUpdate.changed && !directionUpdate.worldLayoutChanged &&
      editor.worldLayout.revision == revisionBeforeDirection &&
      editor.worldLayout.source.verticalConnectors[0].direction ==
          cr::CreativeWorldLayoutVerticalDirection::PositiveX &&
      editor.worldLayout.verticalConnectorManipulation.previewDirection ==
          cr::CreativeWorldLayoutVerticalDirection::NegativeZ;
  const auto directionCommit = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
      context,
      app::CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload{
          app::CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::
              Commit,
          {3.0, 0.0}, 0.2, {}});
  const bool directionCommittedOnce =
      directionCommit.accepted && directionCommit.changed &&
      directionCommit.worldLayoutChanged && !directionCommit.sceneChanged &&
      editor.worldLayout.revision == revisionBeforeDirection + 1U &&
      editor.worldLayout.source.verticalConnectors[0].direction ==
          cr::CreativeWorldLayoutVerticalDirection::NegativeZ;

  return expect(settingsCommittedOnce,
                "connector settings route through one typed source command") &&
         expect(directionPreviewOnly && directionCommittedOnce,
                "connector direction handle previews then commits through dispatcher");
}

bool worldLayoutBuildingTemplateCommandsRouteThroughDispatcher() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Building Templates");
  static_cast<void>(document.assignId(424U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout, "template_commands");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      editor.worldLayout, app::CreativeEditorWorldLayoutGesturePhase::Begin,
      {0.0, 0.0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      editor.worldLayout, app::CreativeEditorWorldLayoutGesturePhase::Commit,
      {6.0, 4.0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Select));
  static_cast<void>(app::selectCreativeEditorWorldLayoutBuilding(
      editor.worldLayout, 0U));

  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      "iggy3d_desktop_building_template_command_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  const auto libraryLoaded =
      app::loadCreativeEditorWorldLayoutBuildingTemplateLibrary(
          editor.worldLayout.buildingTemplates, root);
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor, root,
                                                    &saveId};

  const auto sourcePreview = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutPreview, context);
  const auto sourceApply = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const std::uint64_t revisionBefore = editor.worldLayout.revision;
  const auto captured = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCaptureBuildingTemplate,
      context, app::CreativeDesktopWorldLayoutBuildingTemplateCapturePayload{
                   0U, "Command House"});
  const auto selected = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSelectBuildingTemplate,
      context,
      app::CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload{0U});
  const auto began = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate, context,
      app::CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin,
          {8.0, 2.0},
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
  const auto moved = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate, context,
      app::CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Update,
          {12.2, 6.2},
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
  const auto rotated = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate, context,
      app::CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::
              Transform,
          {},
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
  const bool previewOnly =
      editor.worldLayout.revision == revisionBefore &&
      editor.worldLayout.source.buildings.size() == 1U &&
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      editor.worldLayout.liveEditPreviewVisible &&
      &app::creativeEditorWorldLayoutRenderDocument(
           editor.worldLayout, appState.facade.document()) ==
          &editor.worldLayout.preview.document &&
      app::creativeEditorWorldLayoutDisplaySource(editor.worldLayout)
              .buildings.size() == 2U;
  const auto committed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate, context,
      app::CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
          app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Commit,
          {},
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});

  const bool ok =
      expect(libraryLoaded.accepted && sourcePreview.accepted &&
                 sourceApply.accepted && captured.accepted && captured.changed &&
                 !captured.worldLayoutChanged &&
                 editor.worldLayout.buildingTemplates.templates.size() == 1U,
             "template capture dispatches without changing layout source") &&
      expect(selected.accepted && began.accepted && began.changed &&
                 !began.worldLayoutChanged && moved.accepted && moved.changed &&
                 !moved.worldLayoutChanged && rotated.accepted &&
                 rotated.changed && !rotated.worldLayoutChanged && previewOnly,
             "template selection and placement preview remain transient") &&
      expect(committed.accepted && committed.changed &&
                 committed.worldLayoutChanged &&
                 editor.worldLayout.revision == revisionBefore + 1U &&
                 editor.worldLayout.source.buildings.size() == 2U &&
                 editor.worldLayout.source.rooms[1].footprint.minimum ==
                     cr::CreativeTerrainCoord2{12, 6} &&
                 editor.worldLayout.source.rooms[1].footprint.maximum ==
                     cr::CreativeTerrainCoord2{16, 12},
             "template commit dispatches one semantic source change");
  std::filesystem::remove_all(root, error);
  return ok;
}

bool worldLayoutBuildingTemplateSyncCommandsRouteThroughDispatcher() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Building Template Sync");
  static_cast<void>(document.assignId(425U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "template_sync_commands");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Room));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      editor.worldLayout, app::CreativeEditorWorldLayoutGesturePhase::Begin,
      {0.0, 0.0}));
  static_cast<void>(app::applyCreativeEditorWorldLayoutGesture(
      editor.worldLayout, app::CreativeEditorWorldLayoutGesturePhase::Commit,
      {6.0, 4.0}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      editor.worldLayout, app::CreativeEditorWorldLayoutTool::Select));

  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      "iggy3d_desktop_building_template_sync_command_tests";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  const auto libraryLoaded =
      app::loadCreativeEditorWorldLayoutBuildingTemplateLibrary(
          editor.worldLayout.buildingTemplates, root);
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor, root,
                                                    &saveId};
  const auto captured = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCaptureBuildingTemplate,
      context, app::CreativeDesktopWorldLayoutBuildingTemplateCapturePayload{
                   0U, "Sync Command House"});

  const auto stampAt = [&](app::CreativeEditorWorldLayoutPoint point) {
    const auto began = dispatchPayload(
        app::CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
        context,
        app::CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
            app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::
                Begin,
            point,
            cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
    const auto committed = dispatchPayload(
        app::CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
        context,
        app::CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload{
            app::CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::
                Commit,
            {},
            cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90});
    return began.accepted && committed.accepted && committed.changed;
  };
  const bool stamped = stampAt({10.0, 0.0}) && stampAt({20.0, 0.0});
  const auto generatedStamps =
      dispatchOne(app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!libraryLoaded.accepted || !captured.accepted || !stamped ||
      !generatedStamps.accepted) {
    std::filesystem::remove_all(root, error);
    return expect(false, "template sync command setup accepted");
  }

  const auto room = std::find_if(
      editor.worldLayout.source.rooms.begin(),
      editor.worldLayout.source.rooms.end(),
      [](const cr::CreativeWorldLayoutRoom& value) {
        return value.buildingIndex == 1U;
      });
  const std::size_t levelIndex = room->levelIndex;
  app::CreativeEditorWorldLayoutLevelSettings settings;
  static_cast<void>(app::readCreativeEditorWorldLayoutLevelSettings(
      editor.worldLayout, levelIndex, settings));
  settings.wallHeightCells += 1U;
  static_cast<void>(app::setCreativeEditorWorldLayoutLevelSettings(
      editor.worldLayout, levelIndex, settings));

  const std::uint64_t revisionBeforeUpdate = editor.worldLayout.revision;
  const auto updated = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutUpdateBuildingTemplate,
      context, app::CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
                   1U,
                   cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                       SelectedInstance});
  const std::uint64_t revisionAfterUpdate = editor.worldLayout.revision;
  const auto siblingOutdated =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(
          editor.worldLayout, 2U);
  const std::uint64_t documentRevisionBeforeBlockedRefresh =
      appState.facade.document().revision();
  const auto blockedRefresh = dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutRefreshBuildingTemplateInstances,
      context, app::CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
                   1U,
                   cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                       SafeInstances});
  const std::uint64_t documentRevisionAfterBlockedRefresh =
      appState.facade.document().revision();
  const auto generatedUpdate =
      dispatchOne(app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const std::uint64_t revisionBeforeRefresh = editor.worldLayout.revision;
  const std::uint64_t documentRevisionBeforeRefresh =
      appState.facade.document().revision();
  const std::uint64_t undoBeforeRefresh =
      cr::creativeUndoDepth(appState.history);
  const auto refreshed = dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutRefreshBuildingTemplateInstances,
      context, app::CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
                   1U,
                   cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                       SafeInstances});
  const auto siblingCurrent =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(
          editor.worldLayout, 2U);
  const std::uint64_t documentRevisionAfterRefresh =
      appState.facade.document().revision();
  const auto undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const std::uint64_t documentRevisionAfterUndo =
      appState.facade.document().revision();
  const auto siblingAfterUndo =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(
          editor.worldLayout, 2U);
  const auto redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const std::uint64_t documentRevisionAfterRedo =
      appState.facade.document().revision();
  const auto siblingAfterRedo =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(
          editor.worldLayout, 2U);
  const std::uint64_t revisionBeforeDetach = editor.worldLayout.revision;
  const cr::CreativeWorldLayoutBuildingTemplateFingerprint beforeDetach =
      cr::fingerprintCreativeWorldLayoutBuilding(editor.worldLayout.source, 2U);
  const std::string detachedStableKey =
      editor.worldLayout.source.buildings[2U].stableKey;
  const auto detached = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutDetachBuildingTemplateInstance,
      context, app::CreativeDesktopWorldLayoutBuildingTemplateSyncPayload{
                   2U,
                   cr::CreativeWorldLayoutBuildingTemplateRefreshMode::
                       SelectedInstance});
  const auto siblingAfterDetach =
      app::inspectCreativeEditorWorldLayoutBuildingTemplateSync(
          editor.worldLayout, 2U);
  const cr::CreativeWorldLayoutBuildingTemplateFingerprint afterDetach =
      cr::fingerprintCreativeWorldLayoutBuilding(editor.worldLayout.source, 2U);

  const bool ok =
      expect(updated.accepted && updated.changed &&
                 updated.worldLayoutChanged &&
                 revisionAfterUpdate == revisionBeforeUpdate + 1U &&
                 revisionBeforeRefresh == revisionAfterUpdate &&
                 siblingOutdated.state ==
                     cr::CreativeWorldLayoutBuildingTemplateSyncState::
                         SourceChanged,
             "template update command publishes one source revision") &&
      expect(!blockedRefresh.accepted && !blockedRefresh.changed &&
                 !blockedRefresh.sceneChanged &&
                 documentRevisionAfterBlockedRefresh ==
                     documentRevisionBeforeBlockedRefresh,
             "template rebuild waits for pending source edits to be generated") &&
      expect(generatedUpdate.accepted && refreshed.accepted &&
                 refreshed.changed && refreshed.worldLayoutChanged &&
                 refreshed.sceneChanged &&
                 documentRevisionAfterRefresh !=
                     documentRevisionBeforeRefresh &&
                 cr::creativeUndoDepth(appState.history) ==
                     undoBeforeRefresh + 1U &&
                 siblingCurrent.state ==
                     cr::CreativeWorldLayoutBuildingTemplateSyncState::Current,
             "safe template refresh rebuilds the stale sibling in 3D once") &&
      expect(undone.accepted && siblingAfterUndo.state ==
                                    cr::CreativeWorldLayoutBuildingTemplateSyncState::
                                        SourceChanged &&
                 redone.accepted && siblingAfterRedo.state ==
                                        cr::CreativeWorldLayoutBuildingTemplateSyncState::
                                            Current &&
                 documentRevisionAfterUndo > documentRevisionAfterRefresh &&
                 documentRevisionAfterRedo > documentRevisionAfterUndo,
             "template rebuild source and geometry undo and redo together") &&
      expect(detached.accepted && detached.changed &&
                 detached.worldLayoutChanged && !detached.sceneChanged &&
                 editor.worldLayout.revision == revisionBeforeDetach + 1U &&
                 editor.worldLayout.source.buildings[2U].stableKey ==
                     detachedStableKey &&
                 beforeDetach == afterDetach &&
                 siblingAfterDetach.state ==
                     cr::CreativeWorldLayoutBuildingTemplateSyncState::Unlinked,
             "template detach removes only provenance through the dispatcher");
  std::filesystem::remove_all(root, error);
  return ok;
}

bool worldLayoutCommandsPreviewAndGenerateThroughDispatcher() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd World Layout");
  static_cast<void>(document.assignId(422U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const app::CreativeDesktopCommandResult tool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Room});
  const app::CreativeDesktopCommandResult anchor = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {0.0, 0.0}});
  const app::CreativeDesktopCommandResult room = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {6.0, 5.0}});
  const std::uint64_t undoDepthAfterRoomCreation =
      cr::creativeUndoDepth(appState.history);
  const std::size_t objectCountAfterRoomCreation =
      appState.facade.document().objectCount();
  const std::size_t levelIndex =
      editor.worldLayout.source.rooms[0U].levelIndex;
  app::CreativeEditorWorldLayoutLevelSettings levelSettings;
  static_cast<void>(app::readCreativeEditorWorldLayoutLevelSettings(
      editor.worldLayout, levelIndex, levelSettings));
  levelSettings.floorTopLayer = 1.0;
  levelSettings.wallHeightCells = 4U;
  const app::CreativeDesktopCommandResult levelResized = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetLevelSettings, context,
      app::CreativeDesktopWorldLayoutLevelSettingsPayload{
          levelIndex,
          editor.worldLayout.source.levels[levelIndex].stableKey,
          levelSettings});
  app::CreativeEditorWorldLayoutRoomSettings roomSettings;
  static_cast<void>(app::readCreativeEditorWorldLayoutRoomSettings(
      editor.worldLayout, 0U, roomSettings));
  roomSettings.footprint = {{0, 0}, {8, 6}};
  roomSettings.wallThicknessCells = 0.5;
  const app::CreativeDesktopCommandResult resized = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      app::CreativeDesktopWorldLayoutPropertyEditPayload{
          app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit,
          cr::CreativeWorldLayoutTable::Room, 0U,
          editor.worldLayout.source.rooms[0].stableKey, roomSettings});
  const app::CreativeDesktopCommandResult selectTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Select});
  const std::uint64_t revisionBeforeMove = editor.worldLayout.revision;
  const app::CreativeDesktopCommandResult moveBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoom, context,
      app::CreativeDesktopWorldLayoutRoomManipulationPayload{
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
          {3.0, 3.0}, 0.3});
  const app::CreativeDesktopCommandResult moveUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoom, context,
      app::CreativeDesktopWorldLayoutRoomManipulationPayload{
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Update,
          {5.0, 4.0}, 0.3});
  const bool moveWasPreviewOnly =
      editor.worldLayout.revision == revisionBeforeMove &&
      editor.worldLayout.source.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0} &&
      editor.worldLayout.source.rooms[0].footprint.maximum ==
          cr::CreativeTerrainCoord2{8, 6} &&
      editor.worldLayout.roomManipulation.previewFootprint.minimum ==
          cr::CreativeTerrainCoord2{2, 1} &&
      editor.worldLayout.roomManipulation.previewFootprint.maximum ==
          cr::CreativeTerrainCoord2{10, 7};
  const app::CreativeDesktopCommandResult moved = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoom, context,
      app::CreativeDesktopWorldLayoutRoomManipulationPayload{
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Commit,
          {5.0, 4.0}, 0.3});
  const bool roomMoveCommittedOnce =
      editor.worldLayout.revision == revisionBeforeMove + 1U;
  const app::CreativeDesktopCommandResult doorTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Door});
  const app::CreativeDesktopCommandResult door = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasPoint, context,
      app::CreativeDesktopWorldLayoutPointPayload{{5.0, 1.0}});
  const std::uint64_t revisionBeforeOpeningSettings =
      editor.worldLayout.revision;
  app::CreativeEditorWorldLayoutOpeningSettings openingSettingsEdit;
  openingSettingsEdit.centerOffsetCells = 3.0;
  openingSettingsEdit.widthCells = 1.5;
  openingSettingsEdit.heightCells = 2.5;
  openingSettingsEdit.door.hingeSide =
      cr::CreativeDoorHingeSide::MinimumEdge;
  openingSettingsEdit.door.swingSide =
      cr::CreativeDoorSwingSide::PositiveNormal;
  openingSettingsEdit.door.initialState =
      cr::CreativeDoorInitialState::Open;
  openingSettingsEdit.facing =
      cr::CreativeBuildingOpeningFacing::NegativeNormal;
  const app::CreativeDesktopCommandResult openingSettings = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetOpeningSettings, context,
      app::CreativeDesktopWorldLayoutOpeningSettingsPayload{
          0U, openingSettingsEdit});
  const bool openingSettingsCommittedOnce =
      editor.worldLayout.revision == revisionBeforeOpeningSettings + 1U;
  const app::CreativeDesktopCommandResult selectOpeningTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Select});
  const std::uint64_t revisionBeforeOpeningMove = editor.worldLayout.revision;
  const app::CreativeDesktopCommandResult openingMoveBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
          {5.0, 1.0}, 0.2});
  const app::CreativeDesktopCommandResult openingMoveUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
          {6.12, 1.0}, 0.2});
  const bool openingMoveWasPreviewOnly =
      editor.worldLayout.revision == revisionBeforeOpeningMove &&
      editor.worldLayout.source.openings[0].centerOffsetCells == 3.0 &&
      editor.worldLayout.openingManipulation.previewCenterOffsetCells == 4.0;
  const app::CreativeDesktopCommandResult openingMoved = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Commit,
          {6.12, 1.0}, 0.2});

  const std::uint64_t liveCountBefore =
      appState.facade.document().objectCount();
  const app::CreativeDesktopCommandResult preview = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutPreview, context);
  const bool exactPreviewVisible =
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      app::creativeEditorWorldLayoutRenderDocument(
          editor.worldLayout, appState.facade.document())
              .objectCount() > liveCountBefore;
  const app::CreativeDesktopCommandResult cancelled = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutCancelPreview, context);
  const bool canvasRestoredAfterCancel =
      editor.desktopUi.showWorldLayout &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
  const app::CreativeDesktopCommandResult previewAgain = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutPreview, context);
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);

  return expect(tool.accepted && anchor.accepted && !anchor.changed &&
                    room.accepted && room.worldLayoutChanged &&
                    room.sceneChanged && undoDepthAfterRoomCreation == 1U &&
                    objectCountAfterRoomCreation > 0U &&
                    levelResized.accepted &&
                    levelResized.worldLayoutChanged &&
                    resized.accepted && resized.worldLayoutChanged &&
                    selectTool.accepted && moveBegin.accepted &&
                    moveBegin.changed && !moveBegin.worldLayoutChanged &&
                    moveUpdate.accepted && moveUpdate.changed &&
                    !moveUpdate.worldLayoutChanged && moveWasPreviewOnly &&
                    moved.accepted && moved.changed &&
                    moved.worldLayoutChanged && roomMoveCommittedOnce &&
                    editor.worldLayout.source.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{2, 1} &&
                    editor.worldLayout.source.rooms[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{10, 7} &&
                    editor.worldLayout.source.levels
                            [editor.worldLayout.source.rooms[0].levelIndex]
                                .floorTopLayer == 1.0 &&
                    editor.worldLayout.source.levels
                            [editor.worldLayout.source.rooms[0].levelIndex]
                                .wallHeightCells == 4U &&
                    editor.worldLayout.source.rooms[0].wallThicknessCells ==
                        0.5,
                "room settings and preview-only manipulation route through typed payloads") &&
         expect(doorTool.accepted && door.accepted && door.worldLayoutChanged &&
                    openingSettings.accepted && openingSettings.changed &&
                    openingSettings.worldLayoutChanged &&
                    openingSettingsCommittedOnce &&
                    selectOpeningTool.accepted && openingMoveBegin.accepted &&
                    openingMoveBegin.changed &&
                    !openingMoveBegin.worldLayoutChanged &&
                    openingMoveUpdate.accepted && openingMoveUpdate.changed &&
                    !openingMoveUpdate.worldLayoutChanged &&
                    openingMoveWasPreviewOnly && openingMoved.accepted &&
                    openingMoved.changed && openingMoved.worldLayoutChanged &&
                    editor.worldLayout.revision ==
                        revisionBeforeOpeningMove + 1U &&
                    editor.worldLayout.source.openings[0].centerOffsetCells ==
                        4.0 &&
                    editor.worldLayout.source.openings[0].widthCells == 1.5 &&
                    editor.worldLayout.source.openings[0].facing ==
                        cr::CreativeBuildingOpeningFacing::NegativeNormal,
                "opening settings and preview-only movement route through typed payloads") &&
         expect(preview.accepted && preview.sceneChanged &&
                    exactPreviewVisible && !editor.desktopUi.showWorldLayout,
                "layout preview is exact, transient, and closes the canvas") &&
         expect(cancelled.accepted && cancelled.sceneChanged &&
                    canvasRestoredAfterCancel,
                "layout preview cancel restores the canvas") &&
         expect(previewAgain.accepted && generated.accepted &&
                    generated.changed && generated.sceneChanged,
                "layout confirm publishes through the semantic dispatcher") &&
         expect(appState.facade.document().objectCount() > liveCountBefore &&
                    cr::creativeUndoDepth(appState.history) ==
                        undoDepthAfterRoomCreation + 1U,
                "creation and later regeneration each install one undo entry");
}

bool worldLayoutConflictResolutionUsesTypedConfirmPayload() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd World Layout Conflict");
  static_cast<void>(document.assignId(433U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "command_reconciliation");
  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Crate;
  object.stableKey = "crate";
  object.name = "Command Crate";
  object.boundsCells = {{1.0, 0.0, 1.0}, {2.0, 1.0, 2.0}};
  editor.worldLayout.source.objects.push_back(object);
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!generated.accepted || appState.facade.document().objects().empty()) {
    return expect(false, "command conflict fixture generated");
  }
  const cr::CreativeObjectId originalId =
      appState.facade.document().objects().front().id;
  const cr::CreativeVec3 generatedPosition =
      appState.facade.document().objects().front().transform.position;
  const cr::CreativeDocumentMutationReceipt refined = appState.facade.mutateObject(
      originalId, cr::CreativeMutationKind::Move,
      cr::makeMovePayload({12.0, 2.0, 8.0}));
  editor.worldLayout.source.objects[0].name = "Command Crate Revised";
  ++editor.worldLayout.revision;

  const app::CreativeDesktopCommandResult blocked = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const bool blockedPreserved =
      appState.facade.document().findObject(originalId) != nullptr;
  const cr::CreativeWorldLayoutCompileResult conflictReport =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       editor.worldLayout.source);
  if (conflictReport.recipeChanges.size() != 1U ||
      conflictReport.recipeChanges[0].memberConflicts.size() != 1U) {
    return expect(false, "command conflict fixture reports one exact member");
  }
  const cr::CreativeWorldLayoutRecipeMemberConflict& conflict =
      conflictReport.recipeChanges[0].memberConflicts[0];
  const cr::CreativeWorldLayoutConflictDecision decision =
      cr::makeCreativeWorldLayoutMemberConflictDecision(
          "command_reconciliation.objects.crate", conflict,
          cr::CreativeWorldLayoutConflictResolution::UseSource);
  const app::CreativeDesktopCommandResult resolved = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context,
      app::CreativeDesktopWorldLayoutConfirmPayload{
          {decision}, {}});
  const cr::CreativeObject* patched =
      appState.facade.document().findObject(originalId);

  return expect(refined.changed && !blocked.accepted && !blocked.changed &&
                    blocked.message ==
                        "creative_world_layout_refinement_conflict" &&
                    blockedPreserved,
                "ordinary confirm blocks conflict before typed resolution") &&
         expect(resolved.accepted && resolved.changed &&
                    resolved.sceneChanged && patched != nullptr &&
                    patched->id == originalId &&
                    patched->name == "Command Crate Revised" &&
                    patched->transform.position.x == generatedPosition.x &&
                    patched->transform.position.y == generatedPosition.y &&
                    patched->transform.position.z == generatedPosition.z,
                "typed exact-member payload resolves through the sole dispatcher");
}

bool worldLayoutTerrainReconciliationUsesTypedConfirmPayload() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd World Layout Terrain Reconciliation");
  static_cast<void>(document.assignId(435U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "command_terrain_reconciliation");
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = "terrain.plateau";
  profile.kind = cr::CreativeTerrainRecipeKind::Plateau;
  profile.center = {2, 3};
  profile.baseHeightCells = 4U;
  profile.radiusCells = 2U;
  profile.spacingCells = 1U;
  profile.blend = cr::CreativeTerrainProfileBlend::Set;
  profile.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Fill;
  editor.worldLayout.source.terrainProfiles.push_back(profile);
  cr::CreativeWorldLayoutObject marker;
  marker.kind = cr::CreativeObjectKind::Crate;
  marker.stableKey = "marker";
  marker.name = "Terrain Marker";
  marker.boundsCells = {{8.0, 0.0, 8.0}, {9.0, 1.0, 9.0}};
  editor.worldLayout.source.objects.push_back(marker);
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const cr::CreativeWorldLayoutTerrainImpactPlan generatedImpact =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(
          appState.facade.document(),
          editor.worldLayout.generatedBaseline.source);
  if (!generated.accepted || generatedImpact.sources.size() != 1U ||
      generatedImpact.sources[0].controls.empty()) {
    return expect(false, "command terrain reconciliation fixture generated");
  }

  cr::CreativeTerrainControlPoint drifted =
      generatedImpact.sources[0].controls.front();
  ++drifted.heightCells;
  const cr::CreativeTerrainControlEdit driftEdit{
      cr::CreativeTerrainEditKind::Upsert, drifted};
  const cr::CreativeTerrainMutationReceipt driftReceipt =
      appState.facade.applyTerrainControlEdits({&driftEdit, 1U});
  const std::uint64_t undoBeforeBlocked =
      cr::creativeUndoDepth(appState.history);
  const std::uint64_t terrainRevisionBeforeBlocked =
      appState.facade.document().terrainField().revision();
  const app::CreativeDesktopCommandResult blocked = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const cr::CreativeTerrainControlPoint* blockedControl =
      appState.facade.document().terrainField().controlAt(drifted.coord);
  const bool blockedWasAtomic =
      cr::creativeUndoDepth(appState.history) == undoBeforeBlocked &&
      appState.facade.document().terrainField().revision() ==
          terrainRevisionBeforeBlocked &&
      blockedControl != nullptr && *blockedControl == drifted;
  const cr::CreativeWorldLayoutTerrainReconciliationResult reconciliation =
      app::reconcileCreativeEditorWorldLayoutTerrain(
          editor.worldLayout, appState.facade.document(),
          editor.worldLayout.source);
  if (!reconciliation.blocked || reconciliation.conflicts.size() != 1U) {
    return expect(false, "command terrain conflict exposes one exact source");
  }
  const cr::CreativeWorldLayoutTerrainConflict& conflict =
      reconciliation.conflicts[0];
  const cr::CreativeWorldLayoutTerrainConflictDecision decision{
      conflict.generatedTable, conflict.generatedIndex, conflict.stableKey,
      cr::CreativeWorldLayoutTerrainConflictResolution::Regenerate};
  const app::CreativeDesktopCommandResult regenerated = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context,
      app::CreativeDesktopWorldLayoutConfirmPayload{{}, {decision}});
  const cr::CreativeWorldLayoutTerrainImpactPlan currentImpact =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(
          appState.facade.document(),
          editor.worldLayout.generatedBaseline.source);
  const std::uint64_t undoAfterRegenerate =
      cr::creativeUndoDepth(appState.history);

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const cr::CreativeWorldLayoutTerrainImpactPlan restoredDrift =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(
          appState.facade.document(),
          editor.worldLayout.generatedBaseline.source);
  const std::uint64_t sourceUndoBeforeDetach =
      app::creativeEditorWorldLayoutSourceUndoDepth(editor.worldLayout);
  const std::uint64_t documentUndoBeforeDetach =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult detached = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutDeleteSource, context,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::TerrainProfile, 0U,
          profile.stableKey});
  const cr::CreativeTerrainControlPoint* keptBeforeConfirm =
      appState.facade.document().terrainField().controlAt(drifted.coord);
  const bool detachedKeptTerrain =
      keptBeforeConfirm != nullptr && *keptBeforeConfirm == drifted;
  const app::CreativeDesktopCommandResult confirmedDetached = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const cr::CreativeTerrainControlPoint* keptAfterConfirm =
      appState.facade.document().terrainField().controlAt(drifted.coord);

  bool ok = true;
  ok = expect(driftReceipt.accepted && driftReceipt.changed &&
                  !blocked.accepted && !blocked.changed &&
                  blocked.message ==
                      "terrain changed in 3D; resolve before generating" &&
                  blockedWasAtomic,
              "ordinary confirm blocks terrain drift without mutation") &&
       ok;
  ok = expect(regenerated.accepted && regenerated.changed &&
                  regenerated.sceneChanged &&
                  undoAfterRegenerate == undoBeforeBlocked + 1U &&
                  currentImpact.sources.size() == 1U &&
                  currentImpact.sources[0].status ==
                      cr::CreativeWorldLayoutTerrainImpactStatus::Current,
              "typed Regenerate restores 2D terrain with one undo entry") &&
       ok;
  ok = expect(undone.accepted && restoredDrift.sources.size() == 1U &&
                  restoredDrift.sources[0].status ==
                      cr::CreativeWorldLayoutTerrainImpactStatus::Drifted,
              "undo restores the refined 3D terrain conflict") &&
       ok;
  ok = expect(detached.accepted && detached.changed &&
                  editor.worldLayout.source.terrainProfiles.empty() &&
                  app::creativeEditorWorldLayoutSourceUndoDepth(
                      editor.worldLayout) == sourceUndoBeforeDetach + 1U &&
                  detachedKeptTerrain,
              "Keep 3D removes only the exact 2D terrain owner") &&
       ok;
  ok = expect(confirmedDetached.accepted,
              confirmedDetached.message.empty()
                  ? "detached PreserveExisting source confirmation rejected"
                  : confirmedDetached.message) &&
       ok;
  ok = expect(!confirmedDetached.changed,
              "detached PreserveExisting source confirms without document mutation") &&
       ok;
  ok = expect(keptAfterConfirm != nullptr && *keptAfterConfirm == drifted,
              "detached PreserveExisting terrain survives generation unchanged") &&
       ok;
  ok = expect(cr::creativeUndoDepth(appState.history) ==
                  documentUndoBeforeDetach,
              "detached source confirmation adds no document history entry") &&
       ok;
  return ok;
}

bool worldLayoutObjectFocusAndAdoptionCloseTheSourceLoop() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd World Layout Adoption");
  static_cast<void>(document.assignId(434U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "command_adoption");
  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Crate;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  object.stableKey = "crate";
  object.name = "Adoption Crate";
  object.pointCells = {1.0, 0.0, 1.0};
  editor.worldLayout.source.objects.push_back(object);
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  if (!generated.accepted || appState.facade.document().objects().empty()) {
    return expect(false, "command adoption fixture generated");
  }
  const cr::CreativeObjectId objectId =
      appState.facade.document().objects().front().id;

  editor.desktopUi.showWorldLayout = false;
  const app::CreativeDesktopCommandResult focused = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutFocusObjectSource, context,
      app::CreativeDesktopWorldLayoutObjectSourcePayload{objectId});
  const bool focusOpenedSource =
      focused.accepted && editor.desktopUi.showWorldLayout &&
      editor.worldLayout.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Object &&
      editor.worldLayout.selection.index == 0U;

  const cr::CreativeObject* live =
      appState.facade.document().findObject(objectId);
  if (live == nullptr) {
    return expect(false, "command adoption object remains live");
  }
  cr::CreativeTransform movedTransform = live->transform;
  movedTransform.position = {4.0, 2.0, 3.0};
  const app::CreativeDesktopCommandResult moved = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      app::CreativeDesktopTransformPayload{objectId, movedTransform, true,
                                           false, false});
  const std::uint64_t historyBeforeAdoption =
      cr::creativeUndoDepth(appState.history);
  editor.desktopUi.showWorldLayout = false;
  const app::CreativeDesktopCommandResult adopted = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutAdoptObjectSource, context,
      app::CreativeDesktopWorldLayoutObjectSourcePayload{objectId});
  const cr::CreativeObject* adoptedObject =
      appState.facade.document().findObject(objectId);
  const bool adoptedState =
      adoptedObject != nullptr && adoptedObject->id == objectId &&
      vecNear(adoptedObject->transform.position, movedTransform.position) &&
      vecNear(editor.worldLayout.source.objects[0].pointCells,
              {4.0, 2.0, 3.0}) &&
      editor.worldLayout.source.objects[0].hasAssetSourceBounds &&
      vecNear(editor.worldLayout.source.objects[0].assetSourceBoundsMeters.min,
              {0.0, 0.0, 0.0}) &&
      vecNear(editor.worldLayout.source.objects[0].assetSourceBoundsMeters.max,
              {1.0, 1.0, 1.0}) &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision;

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const cr::CreativeObject* undoneObject =
      appState.facade.document().findObject(objectId);
  const bool undoRestoredSourceOnly =
      undoneObject != nullptr &&
      vecNear(undoneObject->transform.position, movedTransform.position) &&
      vecNear(editor.worldLayout.source.objects[0].pointCells,
              {1.0, 0.0, 1.0});
  const app::CreativeDesktopCommandResult redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);

  return expect(focusOpenedSource,
                "generated object focuses and opens its exact 2D source") &&
         expect(moved.accepted && moved.changed && adopted.accepted &&
                    adopted.changed && adopted.sceneChanged &&
                    adopted.worldLayoutChanged && adoptedState &&
                    cr::creativeUndoDepth(appState.history) ==
                        historyBeforeAdoption + 1U,
                "representable 3D edit adopts with identity and one history entry") &&
         expect(undone.accepted && undoRestoredSourceOnly && redone.accepted &&
                    vecNear(editor.worldLayout.source.objects[0].pointCells,
                            {4.0, 2.0, 3.0}) &&
                    appState.facade.document().findObject(objectId) != nullptr,
                "adoption undo and redo keep live identity and source parity");
}

bool generatedSettingsCannotBypassTerrainReconciliation() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Generated Settings Terrain Guard");
  static_cast<void>(document.assignId(436U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "generated_settings_guard");
  const app::CreativeEditorWorldLayoutEditReceipt shell =
      app::createCreativeEditorWorldLayoutBuildingShell(
          state, {{{1, 1}, {7, 5}}, 0.0, 4U, 0.25, 1U});
  if (!state.source.buildings.empty()) {
    state.source.buildings[0].groundingMode =
        cr::CreativeWorldLayoutGroundingMode::Absolute;
  }
  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = "terrain.guard";
  profile.kind = cr::CreativeTerrainRecipeKind::Plateau;
  profile.center = {12, 12};
  profile.baseHeightCells = 4U;
  profile.radiusCells = 2U;
  profile.spacingCells = 1U;
  profile.blend = cr::CreativeTerrainProfileBlend::Set;
  profile.rodPolicy = cr::CreativeTerrainProfileRodPolicy::Fill;
  state.source.terrainProfiles.push_back(profile);
  ++state.revision;
  const app::CreativeEditorWorldLayoutApplyReceipt generated =
      app::confirmCreativeEditorWorldLayout(state, appState);
  const cr::CreativeWorldLayoutTerrainImpactPlan impact =
      cr::buildCreativeWorldLayoutTerrainImpactPlan(
          appState.facade.document(), state.generatedBaseline.source);
  if (!shell.accepted || !generated.accepted || impact.sources.size() != 1U ||
      impact.sources[0].controls.empty()) {
    return expect(false, "generated settings terrain guard fixture generated");
  }

  cr::CreativeTerrainControlPoint drifted = impact.sources[0].controls.front();
  ++drifted.heightCells;
  const cr::CreativeTerrainControlEdit edit{
      cr::CreativeTerrainEditKind::Upsert, drifted};
  const cr::CreativeTerrainMutationReceipt driftReceipt =
      appState.facade.applyTerrainControlEdits({&edit, 1U});
  const std::uint64_t sourceRevisionBefore = state.revision;
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t terrainRevisionBefore =
      appState.facade.document().terrainField().revision();
  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);

  const app::CreativeEditorWorldLayoutPreviewReceipt preview =
      app::previewCreativeEditorWorldLayoutGeneratedBuildingOperation(
          state, appState.facade.document(), 0U,
          app::CreativeEditorWorldLayoutGeneratedBuildingOperation::Move, 2,
          0);
  const app::CreativeEditorWorldLayoutApplyReceipt blocked =
      app::applyCreativeEditorWorldLayoutGeneratedBuildingOperationToDocument(
          state, appState, 0U,
          app::CreativeEditorWorldLayoutGeneratedBuildingOperation::Move, 2,
          0);

  return expect(driftReceipt.accepted && driftReceipt.changed &&
                    preview.accepted && preview.changed,
                "generated settings preview remains read-only and available") &&
         expect(!blocked.accepted && !blocked.changed &&
                    blocked.reasonCode ==
                        "creative_world_layout_terrain_refinement_conflict" &&
                    state.revision == sourceRevisionBefore &&
                    appState.facade.document().revision() ==
                        documentRevisionBefore &&
                    appState.facade.document().terrainField().revision() ==
                        terrainRevisionBefore &&
                    cr::creativeUndoDepth(appState.history) == undoBefore,
                "generated settings apply cannot bypass terrain reconciliation");
}

bool generatedBuildingScopeOperationsUseExactPreviewAndOneUndo() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Generated Building Operations");
  static_cast<void>(document.assignId(441U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "generated_building_operations");
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "scope_house";
  building.name = "Scope House";
  editor.worldLayout.source.buildings.push_back(building);
  editor.worldLayout.source.levels.push_back(
      {0U, "ground", "Ground", 0.0, 3U, 1U, 1U, 1U});
  editor.worldLayout.source.rooms.push_back(
      {0U, 0U, "main_room", "Main Room", {{2, 3}, {10, 7}}, 0.25});
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = 0U;
  opening.roomEdge = cr::CreativeWorldLayoutRoomEdge::North;
  opening.kind = cr::CreativeBuildingOpeningKind::Door;
  opening.stableKey = "front_door";
  opening.name = "Front Door";
  opening.centerOffsetCells = 4.0;
  opening.widthCells = 1.0;
  opening.cutoutHeightCells = 2.1;
  editor.worldLayout.source.openings.push_back(opening);
  ++editor.worldLayout.revision;

  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const cr::CreativeObject* roomFloor = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 0U, cr::CreativeObjectKind::Floor);
  if (!generated.accepted || roomFloor == nullptr) {
    return expect(false, "generated building operation fixture generated");
  }
  const cr::CreativeObjectId sourceObjectId = roomFloor->id;
  selectPrimary(appState.facade, sourceObjectId);
  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const std::uint64_t documentRevisionBefore =
      appState.facade.document().revision();
  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);

  const app::CreativeDesktopGeneratedBuildingOperationPayload rotate{
      sourceObjectId,
      0U,
      "scope_house",
      app::CreativeEditorWorldLayoutGeneratedBuildingOperation::RotateRight90,
      0,
      0};
  const app::CreativeDesktopCommandResult previewed = dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutPreviewGeneratedBuildingOperation,
      context, rotate);
  cr::CreativeBounds previewFloor;
  const bool previewBoundsReady = generatedBounds(
      editor.worldLayout.preview.document, editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 0U, cr::CreativeObjectKind::Floor,
      previewFloor);
  const bool previewStayedTransient =
      previewed.accepted && previewed.sceneChanged && previewBoundsReady &&
      near(previewFloor.max.x - previewFloor.min.x, 4.0) &&
      near(previewFloor.max.z - previewFloor.min.z, 8.0) &&
      rectEquals(editor.worldLayout.source.rooms[0].footprint,
                 {{2, 3}, {10, 7}}) &&
      editor.worldLayout.revision == sourceRevisionBefore &&
      appState.facade.document().revision() == documentRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == undoBefore;

  const app::CreativeDesktopCommandResult appliedRotate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedBuildingOperation,
      context, rotate);
  cr::CreativeBounds rotatedFloor;
  const bool rotatedBoundsReady = generatedBounds(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 0U, cr::CreativeObjectKind::Floor,
      rotatedFloor);
  const bool rotateCommitted =
      appliedRotate.accepted && appliedRotate.changed &&
      appliedRotate.worldLayoutChanged && appliedRotate.sceneChanged &&
      rotatedBoundsReady && near(rotatedFloor.max.x - rotatedFloor.min.x, 4.0) &&
      near(rotatedFloor.max.z - rotatedFloor.min.z, 8.0) &&
      editor.worldLayout.source.openings[0].stableKey == "front_door" &&
      editor.worldLayout.revision == sourceRevisionBefore + 1U &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 1U;

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undoRestored =
      undone.accepted &&
      rectEquals(editor.worldLayout.source.rooms[0].footprint,
                 {{2, 3}, {10, 7}});
  const app::CreativeDesktopCommandResult redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const bool redoRestored =
      redone.accepted &&
      rectEquals(editor.worldLayout.source.rooms[0].footprint,
                 {{2, 3}, {6, 11}});

  const cr::CreativeObject* rotatedScopeObject = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 0U, cr::CreativeObjectKind::Floor);
  if (rotatedScopeObject == nullptr) {
    return expect(false, "rotated generated building remains addressable");
  }
  const app::CreativeDesktopGeneratedBuildingOperationPayload move{
      rotatedScopeObject->id,
      0U,
      "scope_house",
      app::CreativeEditorWorldLayoutGeneratedBuildingOperation::Move,
      3,
      -2};
  const app::CreativeDesktopCommandResult moved = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedBuildingOperation,
      context, move);
  const bool moveCommitted =
      moved.accepted && moved.changed &&
      rectEquals(editor.worldLayout.source.rooms[0].footprint,
                 {{5, 1}, {9, 9}}) &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 2U;

  const cr::CreativeObject* movedScopeObject = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 0U, cr::CreativeObjectKind::Floor);
  if (movedScopeObject == nullptr) {
    return expect(false, "moved generated building remains addressable");
  }
  const cr::CreativeObjectId movedScopeObjectId = movedScopeObject->id;
  std::int64_t duplicateX = 0;
  std::int64_t duplicateZ = 0;
  const bool hasDuplicateOffset =
      app::defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
          editor.worldLayout, 0U, duplicateX, duplicateZ);
  const app::CreativeDesktopGeneratedBuildingOperationPayload duplicate{
      movedScopeObjectId,
      0U,
      "scope_house",
      app::CreativeEditorWorldLayoutGeneratedBuildingOperation::Duplicate,
      duplicateX,
      duplicateZ};
  const std::uint64_t liveCountBeforeDuplicate =
      appState.facade.document().objectCount();
  const app::CreativeDesktopCommandResult duplicatePreview = dispatchPayload(
      app::CreativeDesktopCommandId::
          WorldLayoutPreviewGeneratedBuildingOperation,
      context, duplicate);
  const bool duplicateStayedTransient =
      hasDuplicateOffset && duplicatePreview.accepted &&
      editor.worldLayout.source.buildings.size() == 1U &&
      appState.facade.document().objectCount() == liveCountBeforeDuplicate &&
      editor.worldLayout.preview.document.objectCount() >
          liveCountBeforeDuplicate;
  const app::CreativeDesktopCommandResult duplicated = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedBuildingOperation,
      context, duplicate);
  const cr::TargetRef selectedDuplicateTarget =
      appState.facade.selectionState().selectedTarget;
  const cr::CreativeObject* selectedDuplicateObject =
      selectedDuplicateTarget.value == cr::kInvalidId
          ? nullptr
          : appState.facade.document().findObject(
                static_cast<cr::CreativeObjectId>(selectedDuplicateTarget.value));
  const cr::CreativeWorldLayoutObjectProvenance selectedDuplicateProvenance =
      selectedDuplicateObject == nullptr
          ? cr::CreativeWorldLayoutObjectProvenance{}
          : cr::resolveCreativeWorldLayoutObjectProvenance(
                editor.worldLayout.source, *selectedDuplicateObject);
  const app::CreativeDesktopGeneratedSourceScopeModel selectedDuplicateScopes =
      app::buildCreativeDesktopGeneratedSourceScopeModel(
          editor.worldLayout.source, selectedDuplicateProvenance);
  const bool selectedDuplicateBuilding =
      app::findCreativeDesktopGeneratedSourceScope(
          selectedDuplicateScopes, cr::CreativeWorldLayoutTable::Building,
          1U) < selectedDuplicateScopes.count;
  const bool duplicateCommitted =
      duplicated.accepted && duplicated.changed &&
      editor.worldLayout.source.buildings.size() == 2U &&
      editor.worldLayout.source.rooms.size() == 2U &&
      editor.worldLayout.selection.kind ==
          app::CreativeEditorWorldLayoutSelectionKind::Building &&
      editor.worldLayout.selection.index == 1U &&
      selectedDuplicateObject != nullptr &&
      selectedDuplicateObject->kind == cr::CreativeObjectKind::Floor &&
      selectedDuplicateBuilding &&
      cr::creativeUndoDepth(appState.history) == undoBefore + 3U;

  const cr::CreativeObject* duplicateObject = findGeneratedObject(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Room, 1U, cr::CreativeObjectKind::Floor);
  const std::uint64_t rejectRevision = editor.worldLayout.revision;
  const std::uint64_t rejectDocumentRevision =
      appState.facade.document().revision();
  const std::uint64_t rejectUndo = cr::creativeUndoDepth(appState.history);
  app::CreativeDesktopGeneratedBuildingOperationPayload stale = move;
  stale.objectId = movedScopeObjectId;
  stale.stableKey = "stale_house";
  const app::CreativeDesktopCommandResult rejectedStale = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedBuildingOperation,
      context, stale);
  app::CreativeDesktopGeneratedBuildingOperationPayload wrongOwner = move;
  wrongOwner.objectId =
      duplicateObject == nullptr ? cr::kInvalidObjectId : duplicateObject->id;
  const app::CreativeDesktopCommandResult rejectedOwner = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedBuildingOperation,
      context, wrongOwner);
  const bool rejectedAtomically =
      duplicateObject != nullptr && !rejectedStale.accepted &&
      !rejectedStale.changed && !rejectedOwner.accepted &&
      !rejectedOwner.changed && editor.worldLayout.revision == rejectRevision &&
      appState.facade.document().revision() == rejectDocumentRevision &&
      cr::creativeUndoDepth(appState.history) == rejectUndo;

  return expect(previewStayedTransient,
                "building rotation preview is exact and transient") &&
         expect(rotateCommitted,
                "building rotation commits source scene and one undo") &&
         expect(undoRestored && redoRestored,
                "building operation undo and redo restore exact topology") &&
         expect(moveCommitted,
                "building move reuses the bounded grid translation kernel") &&
         expect(duplicateStayedTransient,
                "building duplicate preview stays transient") &&
         expect(duplicateCommitted,
                "building duplicate commits source scene and one undo") &&
         expect(rejectedAtomically,
                "stale and unrelated building targets mutate nothing");
}


}  // namespace

bool runCreativeDesktopWorldLayoutStructureCommandTests() {
  bool ok = true;
  ok = worldLayoutLevelCommandsRouteThroughDispatcher() && ok;
  ok = worldLayoutSourceScopeSelectionDoesNotMoveTheCanvas() && ok;
  ok = objectSelectionSynchronizesGeneratedSourcesAcrossViews() && ok;
  ok = desktopActionContextCachesAndTracksSelectionIdentity() && ok;
  ok = worldLayoutSourceScopeFramesThe3dCameraWithoutMutatingSource() && ok;
  ok = worldLayoutStructuralCommandsRouteThroughDispatcher() && ok;
  ok = synchronizedStructuralPreviewsShareInspectionSource() && ok;
  ok = worldLayoutVerticalConnectorCommandsRouteThroughDispatcher() && ok;
  ok = worldLayoutBuildingTemplateCommandsRouteThroughDispatcher() && ok;
  ok = worldLayoutBuildingTemplateSyncCommandsRouteThroughDispatcher() && ok;
  ok = worldLayoutCommandsPreviewAndGenerateThroughDispatcher() && ok;
  ok = worldLayoutConflictResolutionUsesTypedConfirmPayload() && ok;
  ok = worldLayoutTerrainReconciliationUsesTypedConfirmPayload() && ok;
  ok = worldLayoutObjectFocusAndAdoptionCloseTheSourceLoop() && ok;
  ok = generatedSettingsCannotBypassTerrainReconciliation() && ok;
  ok = generatedBuildingScopeOperationsUseExactPreviewAndOneUndo() && ok;
  return ok;
}

#include "EditorAttachmentPlacement.hpp"
#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorInteraction.hpp"
#include "EditorPlacement.hpp"
#include "EditorPlacementClearance.hpp"
#include "EditorPlacementFeedback.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/spatial/SurfacePose.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;
namespace app = iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeDocument makeDocument(cr::CreativeDocumentId id = 700U,
                                  double extent = 64.0) {
  cr::CreativeDocument document = cr::CreativeDocument::create("clearance");
  static_cast<void>(document.assignId(id));
  static_cast<void>(document.setGridSettings(
      {{0.0, 0.0, 0.0}, 1.0, {128, 64, 128}}));
  static_cast<void>(document.setWorldBounds(
      {{0.0, 0.0, 0.0}, {extent, extent, extent}}));
  return document;
}

app::CreativeBrushPlacementPlan boxPlan(
    cr::CreativeObjectKind kind,
    cr::CreativeVec3 center,
    cr::CreativeVec3 size = {1.0, 1.0, 1.0},
    cr::CreativeVec3 rotation = {}) {
  app::CreativeBrushPlacementPlan plan = app::planBrushPlacement(
      kind, {static_cast<float>(center.x), static_cast<float>(center.y),
             static_cast<float>(center.z)});
  const cr::CreativeVec3 half{size.x * 0.5, size.y * 0.5,
                              size.z * 0.5};
  plan.authoredBounds = {{center.x - half.x, center.y - half.y,
                          center.z - half.z},
                         {center.x + half.x, center.y + half.y,
                          center.z + half.z}};
  plan.previewBounds = plan.authoredBounds;
  plan.transform.position = center;
  plan.transform.rotationEulerRadians = rotation;
  plan.transform.scale = {1.0, 1.0, 1.0};
  plan.hasTransformOverride = true;
  plan.hasBoundsOverride = true;
  plan.hasPathOverride = false;
  plan.pathPointCount = 0U;
  plan.orientationResolved = rotation.x != 0.0 || rotation.y != 0.0 ||
                             rotation.z != 0.0;
  plan.status = app::CreativeBrushPlacementPlanStatus::Ready;
  plan.valid = true;
  return plan;
}

cr::CreativeDocumentCreateReceipt addObject(
    cr::CreativeDocument& document,
    const app::CreativeBrushPlacementPlan& plan,
    std::uint64_t ordinal) {
  return document.createObject(app::buildBrushCreateRequest(plan, ordinal));
}

app::CreativeEditorOverlayFrame buildOverlay(
    cr::CreativeAppState& appState,
    app::CreativeEditorState& editor) {
  app::CreativeEditorSelectionFrame selection;
  app::CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projection;
  iggy3d::FrameInput frame;
  app::CreativeEditorOverlayFrame overlay;
  app::buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projection,
       1280U, 720U, 0.03F, false},
      overlay);
  return overlay;
}

bool authoredContactAndPenetrationAreDistinct() {
  cr::CreativeDocument document = makeDocument();
  const cr::CreativeDocumentCreateReceipt blocker =
      addObject(document, boxPlan(cr::CreativeObjectKind::Crate,
                                  {4.5, 0.5, 4.5}),
                1U);
  const cr::CreativePlacementClearanceResult touching =
      app::evaluateCreativeBrushPlacementClearance(
          document, boxPlan(cr::CreativeObjectKind::Crate,
                            {5.5, 0.5, 4.5}));
  const cr::CreativePlacementClearanceResult penetrating =
      app::evaluateCreativeBrushPlacementClearance(
          document, boxPlan(cr::CreativeObjectKind::Crate,
                            {5.49, 0.5, 4.5}));
  return expect(blocker.accepted, "authored blocker fixture created") &&
         expect(touching.allowed &&
                    touching.status ==
                        cr::CreativePlacementClearanceStatus::Ready,
                "face contact remains placeable") &&
         expect(!penetrating.allowed &&
                    penetrating.status ==
                        cr::CreativePlacementClearanceStatus::
                            AuthoredObjectBlocked &&
                    penetrating.blockingObjectId == blocker.objectId,
                "positive authored penetration is rejected");
}

bool rotatedNarrowPhaseAvoidsAabbFalseBlock() {
  constexpr double kQuarterTurn = 0.78539816339744831;
  cr::CreativeDocument document = makeDocument(701U);
  const app::CreativeBrushPlacementPlan blockerPlan = boxPlan(
      cr::CreativeObjectKind::Crate, {12.0, 2.0, 12.0}, {4.0, 1.0, 1.0},
      {0.0, kQuarterTurn, 0.0});
  const cr::CreativeDocumentCreateReceipt blocker =
      addObject(document, blockerPlan, 1U);
  const cr::CreativeVec3 perpendicular{0.8485281374, 0.0, 0.8485281374};
  const app::CreativeBrushPlacementPlan separated = boxPlan(
      cr::CreativeObjectKind::Crate,
      {12.0 + perpendicular.x, 2.0, 12.0 + perpendicular.z},
      {4.0, 1.0, 1.0}, {0.0, kQuarterTurn, 0.0});
  const app::CreativeBrushPlacementPlan penetrating = boxPlan(
      cr::CreativeObjectKind::Crate,
      {12.0 + perpendicular.x * 0.75, 2.0,
       12.0 + perpendicular.z * 0.75},
      {4.0, 1.0, 1.0}, {0.0, kQuarterTurn, 0.0});
  const cr::CreativePlacementClearanceResult separatedResult =
      app::evaluateCreativeBrushPlacementClearance(document, separated);
  const cr::CreativePlacementClearanceResult penetratingResult =
      app::evaluateCreativeBrushPlacementClearance(document, penetrating);
  return expect(blocker.accepted, "rotated blocker fixture created") &&
         expect(separatedResult.allowed,
                "rotated broadphase false positive passes exact OBB test") &&
         expect(!penetratingResult.allowed &&
                    penetratingResult.blockingObjectId == blocker.objectId,
                "rotated positive-volume overlap is rejected");
}

bool worldVoxelAndTerrainClearanceArePinned() {
  cr::CreativeDocument document = makeDocument(702U, 16.0);
  const cr::CreativePlacementClearanceResult edgeTouch =
      app::evaluateCreativeBrushPlacementClearance(
          document, boxPlan(cr::CreativeObjectKind::Crate,
                            {15.5, 0.5, 15.5}));
  const cr::CreativePlacementClearanceResult outside =
      app::evaluateCreativeBrushPlacementClearance(
          document, boxPlan(cr::CreativeObjectKind::Crate,
                            {15.6, 0.5, 15.5}));

  const cr::CreativeVoxelEdit voxel{{3, 0, 3},
                                     cr::CreativeObjectKind::Wall};
  const cr::CreativeVoxelMutationReceipt voxelReceipt =
      document.applyVoxelEdits(std::span{&voxel, 1U});
  const cr::CreativePlacementClearanceResult voxelOverlap =
      app::evaluateCreativeBrushPlacementClearance(
          document, boxPlan(cr::CreativeObjectKind::Crate,
                            {3.5, 0.5, 3.5}));
  const cr::CreativePlacementClearanceResult voxelTouch =
      app::evaluateCreativeBrushPlacementClearance(
          document, boxPlan(cr::CreativeObjectKind::Crate,
                            {3.5, 1.5, 3.5}));

  const cr::CreativeTerrainControlEdit terrain{
      cr::CreativeTerrainEditKind::Upsert, {{8, 8}, 2U, 4U}};
  const cr::CreativeTerrainMutationReceipt terrainReceipt =
      document.applyTerrainControlEdits(std::span{&terrain, 1U});
  const cr::CreativeTerrainSurfacePose surface =
      cr::sampleCreativeTerrainSurfacePose(
          {&document.terrainField(), {8.5, 0.0, 8.5},
           document.gridSettings().origin,
           document.gridSettings().cellSizeMeters});
  const cr::CreativePlacementClearanceResult terrainTouch =
      app::evaluateCreativeBrushPlacementClearance(
          document, boxPlan(cr::CreativeObjectKind::Crate,
                            {8.5, surface.position.y + 0.5, 8.5}));
  const cr::CreativePlacementClearanceResult terrainOverlap =
      app::evaluateCreativeBrushPlacementClearance(
          document, boxPlan(cr::CreativeObjectKind::Crate,
                            {8.5, surface.position.y + 0.45, 8.5}));

  return expect(edgeTouch.allowed, "world-edge contact remains valid") &&
         expect(!outside.allowed &&
                    outside.status ==
                        cr::CreativePlacementClearanceStatus::
                            OutsideWorldBounds,
                "candidate beyond world edge is rejected") &&
         expect(voxelReceipt.accepted && voxelReceipt.changed,
                "voxel blocker fixture created") &&
         expect(!voxelOverlap.allowed &&
                    voxelOverlap.status ==
                        cr::CreativePlacementClearanceStatus::VoxelBlocked &&
                    voxelOverlap.blockingVoxelCell ==
                        cr::CreativeGridCoord3{3, 0, 3},
                "voxel penetration is rejected") &&
         expect(voxelTouch.allowed,
                "candidate may rest exactly on a voxel") &&
         expect(terrainReceipt.accepted && terrainReceipt.changed &&
                    surface.present,
                "flat terrain fixture resolved") &&
         expect(terrainTouch.allowed,
                "candidate may rest exactly on terrain") &&
         expect(!terrainOverlap.allowed &&
                    terrainOverlap.status ==
                        cr::CreativePlacementClearanceStatus::TerrainBlocked,
                "terrain penetration is rejected");
}

bool attachmentExemptsOnlyItsSocketHost() {
  cr::CreativeDocument document = makeDocument(703U);
  const app::CreativeBrushPlacementPlan blockerPlan = boxPlan(
      cr::CreativeObjectKind::Crate, {6.5, 1.5, 6.5});
  const cr::CreativeDocumentCreateReceipt host =
      addObject(document, blockerPlan, 1U);
  app::CreativeBrushPlacementPlan attached = blockerPlan;
  attached.hasAttachment = true;
  attached.attachmentTargetId = host.objectId;
  attached.attachmentSocket = "fixture_socket";
  const cr::CreativePlacementClearanceResult hostOnly =
      app::evaluateCreativeBrushPlacementClearance(document, attached);

  const cr::CreativeDocumentCreateReceipt unrelated =
      addObject(document, blockerPlan, 2U);
  const cr::CreativePlacementClearanceResult withUnrelated =
      app::evaluateCreativeBrushPlacementClearance(document, attached);
  return expect(host.accepted && hostOnly.allowed,
                "socket placement may overlap its explicit host") &&
         expect(unrelated.accepted && !withUnrelated.allowed &&
                    withUnrelated.blockingObjectId == unrelated.objectId,
                "socket exemption does not hide unrelated blockers");
}

bool roomMetadataAndNonSolidCandidatesDoNotBecomeBlockers() {
  cr::CreativeDocument document = makeDocument(704U);
  cr::CreativeDocumentCreateRequest room;
  room.kind = cr::CreativeObjectKind::Room;
  room.name = "metadata room";
  room.bounds = {{1.0, 0.0, 1.0}, {12.0, 6.0, 12.0}};
  room.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt roomReceipt =
      document.createObject(room);
  const cr::CreativePlacementClearanceResult inside =
      app::evaluateCreativeBrushPlacementClearance(
          document, boxPlan(cr::CreativeObjectKind::Crate,
                            {4.5, 0.5, 4.5}));
  const cr::CreativeDocumentCreateReceipt solid =
      addObject(document, boxPlan(cr::CreativeObjectKind::Crate,
                                  {4.5, 0.5, 4.5}),
                2U);
  const app::CreativeBrushPlacementPlan marker = boxPlan(
      cr::CreativeObjectKind::TestLane, {4.5, 0.5, 4.5});
  const cr::CreativePlacementClearanceResult markerResult =
      app::evaluateCreativeBrushPlacementClearance(document, marker);
  return expect(roomReceipt.accepted && inside.allowed,
                "room container metadata is not solid geometry") &&
         expect(solid.accepted && markerResult.allowed,
                "non-solid candidate may overlap authored geometry");
}

bool cacheIsRevisionKeyedAndMutationRevalidates() {
  cr::CreativeDocument document = makeDocument(705U, 128.0);
  for (std::uint64_t index = 0U; index < 24U; ++index) {
    const double x = 1.5 + static_cast<double>(index) * 2.0;
    if (!addObject(document,
                   boxPlan(cr::CreativeObjectKind::Crate,
                           {x, 0.5, 2.5}),
                   index + 1U)
             .accepted) {
      return expect(false, "cache fixture object created");
    }
  }
  app::CreativePlacementClearanceCache cache;
  const bool firstRefresh =
      app::refreshCreativePlacementClearanceCache(cache, document);
  const bool idleRefresh =
      app::refreshCreativePlacementClearanceCache(cache, document);
  const app::CreativeBrushPlacementPlan cachedCandidate = boxPlan(
      cr::CreativeObjectKind::Crate, {1.6, 0.5, 2.5});
  const cr::CreativePlacementClearanceResult cached =
      app::evaluateCreativeBrushPlacementClearance(document, cachedCandidate,
                                                   &cache);

  const app::CreativeBrushPlacementPlan lateBlocker = boxPlan(
      cr::CreativeObjectKind::Crate, {80.5, 0.5, 2.5});
  const cr::CreativeDocumentCreateReceipt late =
      addObject(document, lateBlocker, 100U);
  const cr::CreativePlacementClearanceResult staleCacheResult =
      app::evaluateCreativeBrushPlacementClearance(document, lateBlocker,
                                                   &cache);
  const bool changedRefresh =
      app::refreshCreativePlacementClearanceCache(cache, document);

  cr::CreativeAppState appState;
  const cr::CreativeFacadeDocumentInstallReceipt install =
      appState.facade.installDocument(std::move(document));
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const app::CreativeBrushPlacementPlan mutationCandidate = boxPlan(
      cr::CreativeObjectKind::Crate, {80.6, 0.5, 2.5});
  const app::CreativeBrushPlacementMutationReceipt rejected =
      app::applyBrushPlacement(appState.facade, mutationCandidate, 101U);

  return expect(firstRefresh && !idleRefresh && cache.rebuildCount == 2U,
                "clearance cache rebuilds once per observed revision") &&
         expect(cached.status ==
                    cr::CreativePlacementClearanceStatus::
                        AuthoredObjectBlocked &&
                    cached.testedAuthoredObjectCount < 24U,
                "cached broadphase narrows authored candidates") &&
         expect(late.accepted && !staleCacheResult.allowed && changedRefresh,
                "stale cache falls back to live document truth") &&
         expect(install.accepted && rejected.requested &&
                    !rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        app::CreativeBrushPlacementMutationStatus::
                            ClearanceRejected &&
                    rejected.clearance.status ==
                        cr::CreativePlacementClearanceStatus::
                            AuthoredObjectBlocked &&
                    appState.facade.document().revision() == revisionBefore,
                "mutation revalidation rejects without revision change");
}

bool resolvedAdmissionCarriesClearanceVerdict() {
  cr::CreativeDocument document = makeDocument(706U);
  cr::CreativeGridTarget target = cr::resolveCreativeGridTargetFromHit(
      {4.25, 0.0, 4.25}, {0.0, 1.0, 0.0}, 1.0);
  target.targetFacts = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::EmptyPlane);
  const app::CreativeBrushPlacementAdmission admitted =
      app::admitBrushPlacement(cr::CreativeObjectKind::Crate, target);
  const cr::CreativeDocumentCreateReceipt blocker =
      addObject(document, admitted.plan, 1U);

  app::CreativeEditorWorldTarget worldTarget;
  worldTarget.valid = true;
  worldTarget.grid = target;
  cr::CreativeHotbarEntry held;
  held.kind = cr::CreativeHeldItemKind::Material;
  held.objectKind = cr::CreativeObjectKind::Crate;
  app::CreativePlacementClearanceCache cache;
  static_cast<void>(
      app::refreshCreativePlacementClearanceCache(cache, document));
  const app::CreativeEditorPlacementResolution resolved =
      app::resolveCreativeEditorPlacement(
          held, worldTarget, cr::CreativePlacementYaw::Degrees0, document,
          nullptr, &cache);
  return expect(admitted.allowed && blocker.accepted,
                "resolved-admission blocker fixture created") &&
         expect(!resolved.admission.allowed &&
                    resolved.admission.status ==
                        app::CreativeBrushPlacementAdmissionStatus::
                            ClearanceBlocked &&
                    resolved.admission.plan.clearance.evaluated &&
                    resolved.admission.plan.clearance.status ==
                        cr::CreativePlacementClearanceStatus::
                            AuthoredObjectBlocked,
                "resolved admission carries the stable clearance verdict");
}

bool blockerFeedbackNamesAndOutlinesTheObstruction() {
  cr::CreativeDocument document = makeDocument(707U);
  cr::CreativeGridTarget target = cr::resolveCreativeGridTargetFromHit(
      {4.25, 0.0, 4.25}, {0.0, 1.0, 0.0}, 1.0);
  target.targetFacts = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::EmptyPlane);
  const app::CreativeBrushPlacementAdmission admitted =
      app::admitBrushPlacement(cr::CreativeObjectKind::Crate, target);
  const cr::CreativeDocumentCreateReceipt blocker =
      addObject(document, admitted.plan, 1U);

  cr::CreativeAppState appState;
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      appState.facade.installDocument(std::move(document));
  app::CreativeEditorState editor;
  editor.frameIndex = 20U;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] =
      {cr::CreativeHeldItemKind::Material, cr::CreativeObjectKind::Crate};
  editor.interaction.target.valid = true;
  editor.interaction.target.grid = target;

  app::CreativeEditorSelectionFrame selection;
  app::CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projection;
  iggy3d::FrameInput frame;
  app::CreativeEditorOverlayFrame overlay;
  app::buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projection,
       1280U, 720U, 0.03F, false},
      overlay);
  std::size_t taggedBlockerEdges = 0U;
  for (const iggy3d::RenderCreativeWireframeDebugLine& line :
       overlay.combinedWireLines) {
    if (line.segmentKind ==
            app::kCreativeEditorPlacementBlockerSegmentKind &&
        line.objectId == blocker.objectId) {
      ++taggedBlockerEdges;
    }
  }

  cr::CreativeWorldActionFrame actions;
  const std::size_t secondary =
      static_cast<std::size_t>(cr::CreativeWorldActionId::Secondary);
  actions.down[secondary] = true;
  actions.pressed[secondary] = true;
  app::processCreativeMaterialStrokeFrame(appState, editor, actions, 0U);
  const app::CreativeEditorPlacementFeedback& feedback =
      editor.interaction.placementFeedback;
  const app::CreativeEditorPlacementFeedbackViewModel feedbackView =
      app::creativeEditorPlacementFeedbackViewModel(
          feedback, editor.frameIndex, &appState.facade.document());

  editor.interaction.target = {};
  iggy3d::FrameInput retainedFrame;
  app::CreativeEditorOverlayFrame retainedOverlay;
  app::buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, retainedFrame, projection,
       1280U, 720U, 0.03F, false},
      retainedOverlay);
  editor.frameIndex += app::kCreativeEditorPlacementFeedbackFrames;
  iggy3d::FrameInput expiredFrame;
  app::CreativeEditorOverlayFrame expiredOverlay;
  app::buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, expiredFrame, projection,
       1280U, 720U, 0.03F, false},
      expiredOverlay);

  return expect(admitted.allowed && blocker.accepted && installed.accepted,
                "blocker-feedback fixture created") &&
         expect(overlay.placementVisualization.targetAvailable &&
                    overlay.placementVisualization.attemptedCornerCount ==
                        8U &&
                    overlay.placementInvalidTargetEdgeCount == 12U &&
                    overlay.placementBlockerEdgeCount == 12U &&
                    taggedBlockerEdges == 12U,
                "live rejection outlines exact target and named blocker") &&
         expect(feedback.status ==
                        app::CreativeEditorPlacementFeedbackStatus::Rejected &&
                    feedback.clearance.status ==
                        cr::CreativePlacementClearanceStatus::
                            AuthoredObjectBlocked &&
                    feedback.clearance.blockingObjectId == blocker.objectId &&
                    feedbackView.visible &&
                    feedbackView.label.view() == "Blocked: Crate",
                "click feedback preserves and labels the clearance receipt") &&
         expect(retainedOverlay.placementInvalidTargetEdgeCount == 0U &&
                    retainedOverlay.placementBlockerEdgeCount == 12U,
                "post-click feedback retains only the blocker outline") &&
         expect(expiredOverlay.placementBlockerEdgeCount == 0U,
                "blocker outline expires with placement feedback");
}

bool voxelTerrainAndWorldBlockersProjectSpecificFeedback() {
  cr::CreativeDocument document = makeDocument(708U, 16.0);
  const cr::CreativeVoxelEdit voxel{{3, 0, 3},
                                     cr::CreativeObjectKind::Wall};
  const cr::CreativeVoxelMutationReceipt voxelReceipt =
      document.applyVoxelEdits(std::span{&voxel, 1U});
  const cr::CreativeTerrainControlEdit terrain{
      cr::CreativeTerrainEditKind::Upsert, {{8, 8}, 2U, 4U}};
  const cr::CreativeTerrainMutationReceipt terrainReceipt =
      document.applyTerrainControlEdits(std::span{&terrain, 1U});
  cr::CreativeAppState appState;
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      appState.facade.installDocument(std::move(document));
  app::CreativeEditorState editor;
  editor.frameIndex = 30U;
  editor.activeControlDevice = cr::CreativeControlDevice::Gamepad;

  cr::CreativePlacementClearanceResult clearance;
  clearance.evaluated = true;
  clearance.status = cr::CreativePlacementClearanceStatus::VoxelBlocked;
  clearance.blockingVoxelCell = voxel.cell;
  app::setCreativeEditorPlacementRejectionFeedback(
      editor.interaction, editor.frameIndex,
      cr::CreativeObjectKind::Crate, clearance);
  const app::CreativeEditorOverlayFrame voxelOverlay =
      buildOverlay(appState, editor);
  std::size_t rejectionGlyphCount = 0U;
  for (const iggy3d::DebugHudGlyphQuad& glyph : voxelOverlay.glyphs) {
    if (std::fabs(glyph.r - 1.0F) < 0.0001F &&
        std::fabs(glyph.g - 0.28F) < 0.0001F &&
        std::fabs(glyph.b - 0.16F) < 0.0001F) {
      ++rejectionGlyphCount;
    }
  }
  const std::string voxelLabel{
      app::creativeEditorPlacementFeedbackViewModel(
          editor.interaction.placementFeedback, editor.frameIndex,
          &appState.facade.document())
          .label.view()};

  clearance = {};
  clearance.evaluated = true;
  clearance.status = cr::CreativePlacementClearanceStatus::TerrainBlocked;
  clearance.blockingTerrainCell = terrain.control.coord;
  app::setCreativeEditorPlacementRejectionFeedback(
      editor.interaction, editor.frameIndex,
      cr::CreativeObjectKind::Crate, clearance);
  const app::CreativeEditorOverlayFrame terrainOverlay =
      buildOverlay(appState, editor);
  const std::string terrainLabel{
      app::creativeEditorPlacementFeedbackViewModel(
          editor.interaction.placementFeedback, editor.frameIndex,
          &appState.facade.document())
          .label.view()};

  clearance = {};
  clearance.evaluated = true;
  clearance.status =
      cr::CreativePlacementClearanceStatus::OutsideWorldBounds;
  app::setCreativeEditorPlacementRejectionFeedback(
      editor.interaction, editor.frameIndex,
      cr::CreativeObjectKind::Crate, clearance);
  const app::CreativeEditorOverlayFrame worldOverlay =
      buildOverlay(appState, editor);
  const std::string worldLabel{
      app::creativeEditorPlacementFeedbackViewModel(
          editor.interaction.placementFeedback, editor.frameIndex,
          &appState.facade.document())
          .label.view()};

  return expect(voxelReceipt.accepted && terrainReceipt.accepted &&
                    installed.accepted,
                "non-authored blocker fixtures created") &&
         expect(voxelOverlay.placementBlockerEdgeCount == 12U &&
                    voxelLabel == "Blocked: Wall" &&
                    rejectionGlyphCount > 0U,
                "voxel blocker has outline, label, and gamepad HUD text") &&
         expect(terrainOverlay.placementBlockerEdgeCount == 12U &&
                    terrainLabel == "Blocked: terrain",
                "terrain blocker has cell outline and semantic label") &&
         expect(worldOverlay.placementBlockerEdgeCount == 12U &&
                    worldLabel == "Outside build bounds",
                "world blocker outlines the configured build bounds");
}

bool genericRejectionUsesTheSharedFeedbackModel() {
  app::CreativeEditorInteractionState interaction;
  app::setCreativeEditorPlacementFeedback(
      interaction, app::CreativeEditorPlacementFeedbackStatus::Rejected,
      50U, cr::CreativeObjectKind::Crate);
  const app::CreativeEditorPlacementFeedbackViewModel rejected =
      app::creativeEditorPlacementFeedbackViewModel(
          interaction.placementFeedback, 50U);
  app::setCreativeEditorPlacementFeedback(
      interaction, app::CreativeEditorPlacementFeedbackStatus::Placed,
      51U, cr::CreativeObjectKind::Crate, 12U);
  const app::CreativeEditorPlacementFeedbackViewModel placed =
      app::creativeEditorPlacementFeedbackViewModel(
          interaction.placementFeedback, 51U);

  return expect(rejected.visible &&
                    rejected.label.view() == "Action rejected" &&
                    rejected.color.r == 1.0F &&
                    rejected.color.g == 0.28F &&
                    rejected.color.b == 0.16F,
                "generic action rejection uses the shared red model") &&
         expect(placed.visible && placed.label.empty() &&
                    placed.color.r == 0.25F &&
                    placed.color.g == 1.0F &&
                    placed.color.b == 0.35F,
                "placement success uses the shared green model");
}

bool structuredRejectionsRemainVisibleToTheCreator() {
  app::CreativeEditorInteractionState interaction;
  app::CreativeBrushPlacementAdmission admission;
  admission.status =
      app::CreativeBrushPlacementAdmissionStatus::UnsupportedBrush;
  admission.plan.brush = cr::CreativeObjectKind::Unknown;
  app::setCreativeEditorPlacementAdmissionRejectionFeedback(
      interaction, 60U, admission);
  const app::CreativeEditorPlacementFeedbackViewModel unsupported =
      app::creativeEditorPlacementFeedbackViewModel(
          interaction.placementFeedback, 60U);

  app::CreativeBrushPlacementMutationReceipt occupiedReceipt;
  occupiedReceipt.requested = true;
  occupiedReceipt.status =
      app::CreativeBrushPlacementMutationStatus::Occupied;
  occupiedReceipt.objectKind = cr::CreativeObjectKind::Wall;
  app::setCreativeEditorPlacementMutationFeedback(
      interaction, 61U, occupiedReceipt);
  const app::CreativeEditorPlacementFeedbackViewModel occupied =
      app::creativeEditorPlacementFeedbackViewModel(
          interaction.placementFeedback, 61U);

  app::setCreativeEditorPlacementRejectionFeedback(
      interaction, 62U, cr::CreativeObjectKind::Crate, {},
      app::CreativeEditorPlacementRejectionReason::SemanticSourceOwned);
  const app::CreativeEditorPlacementFeedbackViewModel generated =
      app::creativeEditorPlacementFeedbackViewModel(
          interaction.placementFeedback, 62U);

  return expect(unsupported.label.view() == "Unsupported shape",
                "unsupported placement reports the missing capability") &&
         expect(occupied.label.view() == "Target occupied",
                "replacement conflict reports occupancy explicitly") &&
         expect(generated.label.view() == "Edit generated source",
                "generated output directs the creator to its source");
}

}  // namespace

int main() {
  const bool ok = authoredContactAndPenetrationAreDistinct() &&
                  rotatedNarrowPhaseAvoidsAabbFalseBlock() &&
                  worldVoxelAndTerrainClearanceArePinned() &&
                  attachmentExemptsOnlyItsSocketHost() &&
                  roomMetadataAndNonSolidCandidatesDoNotBecomeBlockers() &&
                  cacheIsRevisionKeyedAndMutationRevalidates() &&
                  resolvedAdmissionCarriesClearanceVerdict() &&
                  blockerFeedbackNamesAndOutlinesTheObstruction() &&
                  voxelTerrainAndWorldBlockersProjectSpecificFeedback() &&
                  genericRejectionUsesTheSharedFeedbackModel() &&
                  structuredRejectionsRemainVisibleToTheCreator();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

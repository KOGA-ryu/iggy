#include "EditorEdits.hpp"
#include "EditorConnectedFill.hpp"
#include "EditorAttachmentPlacement.hpp"
#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorInteraction.hpp"
#include "EditorInteractionInternal.hpp"
#include "EditorPathEditing.hpp"
#include "EditorPlacement.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorShapePreview.hpp"
#include "EditorState.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "EditorStructuralPlacement.hpp"
#include "EditorToolOptions.hpp"
#include "EditorTransform.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "render/vulkan/BufferImageResources.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace cr = iggy3d::creative;
using namespace iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameCell(cr::CreativeGridCoord3 lhs, cr::CreativeGridCoord3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameBounds(cr::CreativeBounds lhs, cr::CreativeBounds rhs) {
  return sameVec3(lhs.min, rhs.min) && sameVec3(lhs.max, rhs.max);
}

bool sameTransform(cr::CreativeTransform lhs, cr::CreativeTransform rhs) {
  return sameVec3(lhs.position, rhs.position) &&
         sameVec3(lhs.rotationEulerRadians, rhs.rotationEulerRadians) &&
         sameVec3(lhs.scale, rhs.scale);
}

double extentX(cr::CreativeBounds bounds) {
  return bounds.max.x - bounds.min.x;
}

double extentZ(cr::CreativeBounds bounds) {
  return bounds.max.z - bounds.min.z;
}

bool near(float actual, float expected, float epsilon = 1.0e-4F) {
  return std::fabs(actual - expected) <= epsilon;
}

bool nearVec3(cr::CreativeVec3 actual,
              cr::CreativeVec3 expected,
              double epsilon = 1.0e-9) {
  return std::fabs(actual.x - expected.x) <= epsilon &&
         std::fabs(actual.y - expected.y) <= epsilon &&
         std::fabs(actual.z - expected.z) <= epsilon;
}

std::size_t actionIndex(cr::CreativeWorldActionId action) {
  return static_cast<std::size_t>(action);
}

cr::CreativeWorldActionFrame actionFrame(cr::CreativeWorldActionId action,
                                         bool down,
                                         bool pressed,
                                         bool released) {
  cr::CreativeWorldActionFrame frame;
  const std::size_t index = actionIndex(action);
  frame.down[index] = down;
  frame.pressed[index] = pressed;
  frame.released[index] = released;
  return frame;
}

CreativeEditorState materialEditor(cr::CreativeObjectKind kind) {
  CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] =
      {cr::CreativeHeldItemKind::Material, kind};
  editor.placeMode = true;
  editor.placeBrush = kind;
  editor.placeCellSize = 1.0;
  editor.frameIndex = 10U;
  return editor;
}

void installHistoryDocument(cr::CreativeAppState& appState,
                            cr::CreativeDocumentId documentId) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Stroke Test");
  static_cast<void>(document.assignId(documentId));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
}

void setPlaceTarget(CreativeEditorState& editor,
                    std::int32_t x,
                    std::int32_t y = 0,
                    std::int32_t z = 0) {
  CreativeEditorWorldTarget target;
  target.valid = true;
  target.grid.valid = true;
  target.grid.faceNormal = {0.0, 1.0, 0.0};
  target.grid.adjacentCell = {x, y, z};
  target.grid.placementAnchor = {
      static_cast<double>(x) + 0.5, static_cast<double>(y),
      static_cast<double>(z) + 0.5};
  target.grid.adjacentCellBounds = {
      {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z)},
      {static_cast<double>(x + 1), static_cast<double>(y + 1),
       static_cast<double>(z + 1)}};
  target.grid.targetFacts = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::EmptyPlane);
  editor.interaction.target = target;
}

void setVoxelTarget(CreativeEditorState& editor,
                    cr::CreativeGridCoord3 cell,
                    cr::CreativeObjectKind material,
                    cr::CreativeVec3 faceNormal = {0.0, 1.0, 0.0}) {
  CreativeEditorWorldTarget target;
  target.valid = true;
  target.voxelHit = true;
  target.voxelCell = cell;
  target.objectKind = material;
  target.grid.valid = true;
  target.grid.faceNormal = faceNormal;
  target.grid.targetCell = cell;
  target.grid.adjacentCell = {cell.x, cell.y + 1, cell.z};
  target.grid.targetCellBounds = cr::creativeVolumeCellBounds(
      cell, 1.0, {});
  target.grid.adjacentCellBounds = cr::creativeVolumeCellBounds(
      target.grid.adjacentCell, 1.0, {});
  target.grid.targetFacts = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::Voxel, material);
  editor.interaction.target = target;
}

cr::CreativeGridTarget targetWithFacts(
    cr::CreativeGridTarget target,
    cr::CreativePlacementTargetSource source,
    cr::CreativeObjectKind hostKind = cr::CreativeObjectKind::Unknown,
    cr::CreativeObjectId hostObjectId = cr::kInvalidObjectId) {
  target.targetFacts =
      cr::makeCreativePlacementTargetFacts(source, hostKind, hostObjectId);
  return target;
}

bool placementPlanMatchesEveryCreateRequest() {
  constexpr iggy3d::Vec3 anchor{2.5F, 1.0F, -3.5F};
  bool sawPoint = false;
  bool sawLine = false;
  bool sawPath = false;
  bool sawMovingPlatform = false;
  bool ok = true;
  std::size_t supportedCount = 0U;
  for (const cr::CreativeObjectDescriptor& descriptor :
       cr::allObjectDescriptors()) {
    if (!descriptorSupportsBrushPlacement(descriptor)) {
      continue;
    }
    ++supportedCount;
    const CreativeBrushPlacementPlan plan =
        planBrushPlacement(descriptor.kind, anchor);
    const cr::CreativeDocumentCreateRequest request =
        buildBrushCreateRequest(descriptor.kind, anchor, 42U);
    const cr::CreativeDocumentCreateRequest plannedRequest =
        buildBrushCreateRequest(plan, 42U);
    ok = expect(plan.valid &&
                    plan.status == CreativeBrushPlacementPlanStatus::Ready,
                "eligible descriptor has a ready placement plan") &&
         expect(request.kind == plan.brush &&
                    request.hasTransformOverride == plan.hasTransformOverride &&
                    request.hasBoundsOverride == plan.hasBoundsOverride &&
                    request.hasPathOverride == plan.hasPathOverride,
                "create request copies plan ownership flags") &&
         expect(sameTransform(request.transform, plan.transform) &&
                    sameBounds(request.bounds, plan.authoredBounds),
                "create request copies plan geometry") &&
         expect(sameTransform(plannedRequest.transform, request.transform) &&
                    sameBounds(plannedRequest.bounds, request.bounds) &&
                    plannedRequest.pathPoints.size() ==
                        request.pathPoints.size(),
                "plan-consuming request matches compatibility overload") &&
         expect(request.pathPoints.size() == plan.pathPointCount &&
                    std::equal(request.pathPoints.begin(),
                               request.pathPoints.end(),
                               plan.pathPoints.begin(),
                               [](const cr::CreativePathPoint& lhs,
                                  const cr::CreativePathPoint& rhs) {
                                 return sameVec3(lhs.position, rhs.position) &&
                                        lhs.dwellSeconds == rhs.dwellSeconds &&
                                        lhs.outgoingSpeedMultiplier ==
                                            rhs.outgoingSpeedMultiplier;
                               }),
                "create request copies fixed plan path") &&
         ok;
    sawPoint = sawPoint ||
               plan.shapeKind == cr::CreativeObjectShapeKind::Point;
    sawLine = sawLine || plan.shapeKind == cr::CreativeObjectShapeKind::Line;
    sawPath = sawPath || plan.shapeKind == cr::CreativeObjectShapeKind::Path;
    if (descriptor.kind == cr::CreativeObjectKind::MovingPlatform) {
      const cr::CreativeBoundsMetrics movingBounds =
          cr::measureCreativeBounds(plan.authoredBounds);
      sawMovingPlatform =
          movingBounds.valid && plan.pathPointCount == 2U &&
          request.hasPathOverride &&
          sameVec3(plan.pathPoints[0].position, movingBounds.center) &&
          plan.pathPoints[1].position.x == plan.pathPoints[0].position.x &&
          plan.pathPoints[1].position.y ==
              plan.pathPoints[0].position.y + 3.0 &&
          plan.pathPoints[1].position.z == plan.pathPoints[0].position.z;
    }
  }

  const float nan = std::numeric_limits<float>::quiet_NaN();
  const CreativeBrushPlacementPlan nonFinite =
      planBrushPlacement(cr::CreativeObjectKind::Wall, {nan, 0.0F, 0.0F});
  const CreativeBrushPlacementPlan unsupported =
      planBrushPlacement(cr::CreativeObjectKind::Unknown, {});
  const CreativeBrushPlacementPlan overflow = planBrushPlacement(
      cr::CreativeObjectKind::Wall,
      {std::numeric_limits<float>::max(), 0.0F, 0.0F});
  const CreativeBrushPlacementPlan wall =
      planBrushPlacement(cr::CreativeObjectKind::Wall, {});
  const cr::CreativeBoundsMetrics wallBounds =
      cr::measureCreativeBounds(wall.authoredBounds);
  const cr::CreativeDocumentCreateRequest invalidRequest =
      buildBrushCreateRequest(cr::CreativeObjectKind::Unknown, {}, 1U);
  return expect(supportedCount > 0U && sawPoint && sawLine && sawPath,
                "descriptor sweep covers point line and path brushes") &&
         expect(sawMovingPlatform,
                "moving platform placement owns a vertical two-point route") &&
         expect(!nonFinite.valid &&
                    nonFinite.status ==
                        CreativeBrushPlacementPlanStatus::InvalidAnchor,
                "non-finite placement anchor is rejected") &&
         expect(!unsupported.valid &&
                    unsupported.status ==
                        CreativeBrushPlacementPlanStatus::UnsupportedBrush,
                "unsupported brush is rejected") &&
         expect(!overflow.valid &&
                    overflow.status ==
                        CreativeBrushPlacementPlanStatus::InvalidGeometry,
                "precision-overflow placement geometry is rejected") &&
         expect(wall.valid && wallBounds.valid &&
                    sameVec3(wallBounds.size,
                             cr::defaultCreativeObjectSize(
                                 cr::CreativeObjectKind::Wall)),
                "wall placement consumes descriptor geometry without rewrite") &&
         expect(!invalidRequest.hasTransformOverride &&
                    !invalidRequest.hasBoundsOverride &&
                    !invalidRequest.hasPathOverride,
                "invalid plan cannot leak create overrides") &&
         ok;
}

bool standaloneBrushPalettePopulatesEveryCatalogCategory() {
  const std::vector<cr::CreativeObjectKind> palette =
      buildBrushPaletteFromDescriptors();
  cr::CreativeCatalogState catalog = cr::makeCreativeCatalog(palette);
  constexpr std::array pages{
      cr::CreativeCatalogPage::Structure,
      cr::CreativeCatalogPage::Terrain,
      cr::CreativeCatalogPage::Movement,
      cr::CreativeCatalogPage::Logic,
      cr::CreativeCatalogPage::Dressing,
      cr::CreativeCatalogPage::Media,
      cr::CreativeCatalogPage::Gameplay,
      cr::CreativeCatalogPage::Testing,
      cr::CreativeCatalogPage::Helpers,
      cr::CreativeCatalogPage::Tools,
  };
  bool ok = expect(!palette.empty(),
                   "standalone descriptor palette remains populated");
  for (cr::CreativeCatalogPage page : pages) {
    static_cast<void>(cr::setCreativeCatalogPage(catalog, page));
    ok = expect(!catalog.filteredEntryIndices.empty(),
                "standalone catalog category has at least one entry") &&
         ok;
  }
  return ok;
}

bool placementAdmissionOwnsPreviewAndExecutionTruth() {
  cr::CreativeGridTarget target = targetWithFacts(
      cr::resolveCreativeGridTargetFromHit(
          {2.25, 1.0, -3.25}, {0.0, 1.0, 0.0}, 1.0),
      cr::CreativePlacementTargetSource::EmptyPlane);
  const CreativeBrushPlacementAdmission voxelReady =
      admitBrushPlacement(cr::CreativeObjectKind::Wall, target);

  bool ok = expect(voxelReady.allowed &&
                       voxelReady.status ==
                           CreativeBrushPlacementAdmissionStatus::Ready &&
                       voxelReady.plan.valid &&
                       voxelReady.plan.storagePolicy ==
                           cr::CreativePlacementStoragePolicy::VoxelCell &&
                       voxelReady.plan.hasVoxelCell &&
                       voxelReady.plan.voxelCell.x ==
                           target.adjacentCell.x &&
                       voxelReady.plan.voxelCell.y ==
                           target.adjacentCell.y &&
                       voxelReady.plan.voxelCell.z ==
                           target.adjacentCell.z &&
                       sameBounds(voxelReady.plan.previewBounds,
                                  target.adjacentCellBounds),
                   "valid target produces one admitted placement plan") &&
            expect(toString(voxelReady.status) == "creative_placement_ready",
                   "admission status has a stable reason code");

  for (const cr::CreativeVec3 face :
       {cr::CreativeVec3{-1.0, 0.0, 0.0},
        cr::CreativeVec3{1.0, 0.0, 0.0},
        cr::CreativeVec3{0.0, -1.0, 0.0},
        cr::CreativeVec3{0.0, 1.0, 0.0},
        cr::CreativeVec3{0.0, 0.0, -1.0},
        cr::CreativeVec3{0.0, 0.0, 1.0}}) {
    target.faceNormal = face;
    ok = expect(admitBrushPlacement(cr::CreativeObjectKind::Wall, target)
                    .allowed,
                "current wall policy preserves all-face placement") &&
         ok;
  }

  cr::CreativeGridTarget invalidFace = target;
  invalidFace.faceNormal = {};
  const CreativeBrushPlacementAdmission faceRejected =
      admitBrushPlacement(cr::CreativeObjectKind::Wall, invalidFace);
  const CreativeBrushPlacementAdmission targetRejected =
      admitBrushPlacement(cr::CreativeObjectKind::Wall, {});
  const CreativeBrushPlacementAdmission brushRejected =
      admitBrushPlacement(cr::CreativeObjectKind::Unknown, target);
  ok = expect(!faceRejected.allowed &&
                  faceRejected.status ==
                      CreativeBrushPlacementAdmissionStatus::FaceDisallowed,
              "degenerate face is a structured admission rejection") &&
       expect(!targetRejected.allowed &&
                  targetRejected.status ==
                      CreativeBrushPlacementAdmissionStatus::InvalidTarget,
              "missing target is a structured admission rejection") &&
       expect(!brushRejected.allowed &&
                  brushRejected.status ==
                      CreativeBrushPlacementAdmissionStatus::UnsupportedBrush,
              "unsupported brush is a structured admission rejection") &&
       ok;

  cr::CreativeAppState appState;
  installHistoryDocument(appState, 100U);
  const CreativeBrushPlacementMutationReceipt voxelReceipt =
      applyBrushPlacement(appState.facade, voxelReady.plan, 1U);
  const CreativeBrushPlacementMutationReceipt occupiedReceipt =
      applyBrushPlacement(appState.facade, voxelReady.plan, 2U);

  const cr::CreativeGridTarget objectTarget = targetWithFacts(
      cr::resolveCreativeGridTargetFromHit(
          {6.25, 1.0, -3.25}, {0.0, 1.0, 0.0}, 1.0),
      cr::CreativePlacementTargetSource::EmptyPlane);
  const CreativeBrushPlacementAdmission objectReady =
      admitBrushPlacement(cr::CreativeObjectKind::Crate, objectTarget);
  const CreativeBrushPlacementMutationReceipt objectReceipt =
      applyBrushPlacement(appState.facade, objectReady.plan, 3U);
  const cr::CreativeObject* placed =
      appState.facade.findObject(objectReceipt.objectId);
  const std::uint64_t revisionBeforeRejected =
      appState.facade.document().revision();
  const CreativeBrushPlacementPlan invalidPlan = planBrushPlacement(
      cr::CreativeObjectKind::Wall,
      {std::numeric_limits<float>::max(), 0.0F, 0.0F});
  const CreativeBrushPlacementMutationReceipt rejected =
      applyBrushPlacement(appState.facade, invalidPlan, 4U);
  const cr::CreativeGridTarget halfMeterTarget = targetWithFacts(
      cr::resolveCreativeGridTargetFromHit(
          {2.25, 1.0, -3.25}, {0.0, 1.0, 0.0}, 0.5),
      cr::CreativePlacementTargetSource::EmptyPlane);
  const CreativeBrushPlacementAdmission halfMeterAdmission =
      admitBrushPlacement(cr::CreativeObjectKind::Wall, halfMeterTarget);
  const CreativeBrushPlacementMutationReceipt gridMismatch =
      applyBrushPlacement(appState.facade, halfMeterAdmission.plan, 5U);

  CreativeEditorState voxelEditor =
      materialEditor(cr::CreativeObjectKind::Wall);
  voxelEditor.placeCellSize = 0.25;
  CreativeEditorState objectEditor =
      materialEditor(cr::CreativeObjectKind::Crate);
  objectEditor.placeCellSize = 0.25;
  CreativeEditorState volumeEditor =
      materialEditor(cr::CreativeObjectKind::Wall);
  volumeEditor.placeCellSize = 0.25;
  volumeEditor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::VolumeFill;
  syncCreativeEditorHeldItem(appState, volumeEditor);
  return expect(voxelReceipt.accepted && voxelReceipt.changed &&
                    voxelReceipt.voxelCreated && !voxelReceipt.objectCreated &&
                    voxelReceipt.status ==
                        CreativeBrushPlacementMutationStatus::Applied &&
                    appState.facade.document().objectCount() == 1U &&
                    appState.facade.document().voxelField().materialAt(
                        voxelReady.plan.voxelCell) ==
                        cr::CreativeObjectKind::Wall,
                "voxel-backed admission writes one structural cell") &&
         expect(!occupiedReceipt.accepted && !occupiedReceipt.changed &&
                    occupiedReceipt.status ==
                        CreativeBrushPlacementMutationStatus::Occupied &&
                    occupiedReceipt.revisionBefore ==
                        occupiedReceipt.revisionAfter,
                "occupied voxel target rejects without a revision") &&
         expect(objectReceipt.accepted && objectReceipt.objectCreated &&
                    !objectReceipt.voxelCreated && placed != nullptr &&
                    objectReceipt.storagePolicy ==
                        cr::CreativePlacementStoragePolicy::AuthoredObject,
                "authored-object admission remains on the object path") &&
         expect(placed != nullptr &&
                    sameTransform(placed->transform, objectReady.plan.transform) &&
                    sameBounds(placed->bounds, objectReady.plan.authoredBounds),
                "execution consumes the admitted plan without geometry drift") &&
         expect(rejected.requested && !rejected.accepted &&
                    rejected.status ==
                        CreativeBrushPlacementMutationStatus::InvalidPlan &&
                    appState.facade.document().revision() ==
                        revisionBeforeRejected,
                "invalid plan execution fails closed without mutation") &&
         expect(halfMeterAdmission.allowed && !gridMismatch.accepted &&
                    !gridMismatch.changed &&
                    gridMismatch.status ==
                        CreativeBrushPlacementMutationStatus::InvalidPlan &&
                    gridMismatch.reasonCode ==
                        "creative_placement_voxel_grid_mismatch" &&
                    appState.facade.document().revision() ==
                        revisionBeforeRejected,
                "voxel plan must match the document grid exactly") &&
         expect(near(static_cast<float>(creativeEditorTargetCellSize(
                         appState.facade.document(), voxelEditor)),
                     1.0F) &&
                    near(static_cast<float>(creativeEditorTargetCellSize(
                         appState.facade.document(), objectEditor)),
                     0.25F) &&
                    near(static_cast<float>(creativeEditorTargetCellSize(
                         appState.facade.document(), volumeEditor)),
                     1.0F) &&
                    near(static_cast<float>(
                             volumeEditor.volume.selection.cellSize),
                         1.0F),
                "voxel and volume targeting use document cells while objects use snap") &&
         ok;
}

bool semanticCompatibilityOwnsPreviewMutationAndHistory() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 101U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Door);
  setPlaceTarget(editor, 2, 0, 1);

  const CreativeBrushPlacementAdmission rejected = admitBrushPlacement(
      cr::CreativeObjectKind::Door, editor.interaction.target.grid);
  iggy3d::FrameInput rejectedPreview;
  attachCreativeEditorPlacementPreviews(
      editor, false, rejectedPreview, &appState.facade.document());
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const std::size_t undoBefore = cr::creativeUndoDepth(appState.history);
  const cr::CreativeDocumentCreateReceipt guardedObjectMutation =
      placeBrushObject(appState.facade, rejected.plan, 1U);
  const CreativeBrushPlacementMutationReceipt guardedMutation =
      applyBrushPlacement(appState.facade, rejected.plan, 1U);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true), 1U);

  editor.interaction.target.grid.faceNormal = {0.0, 0.0, 1.0};
  editor.interaction.target.grid.targetFacts =
      cr::makeCreativePlacementTargetFacts(
          cr::CreativePlacementTargetSource::Voxel,
          cr::CreativeObjectKind::Wall);
  clearCreativeEditorPlacementFeedback(editor.interaction);
  const CreativeBrushPlacementAdmission accepted = admitBrushPlacement(
      cr::CreativeObjectKind::Door, editor.interaction.target.grid);
  cr::CreativeGridTarget snappedFace = editor.interaction.target.grid;
  snappedFace.faceNormal = {0.0, 1.0, 0.0};
  snappedFace.placementNormal = {1.0, 0.0, 0.0};
  snappedFace.anchorSnapped = true;
  const CreativeBrushPlacementAdmission snapped = admitBrushPlacement(
      cr::CreativeObjectKind::Door, snappedFace);
  iggy3d::FrameInput acceptedPreview;
  attachCreativeEditorPlacementPreviews(
      editor, false, acceptedPreview, &appState.facade.document());

  return expect(!rejected.allowed && rejected.plan.valid &&
                    rejected.status ==
                        CreativeBrushPlacementAdmissionStatus::
                            TargetIncompatible &&
                    rejected.plan.compatibility.status ==
                        cr::CreativePlacementCompatibilityStatus::
                            SourceDisallowed,
                "door rejects an empty construction plane") &&
         expect(rejectedPreview.creativePreview.itemCount == 2U &&
                    rejectedPreview.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::PlacementInvalid,
                "incompatible placement uses the red target preview") &&
         expect(guardedObjectMutation.requested &&
                    !guardedObjectMutation.accepted &&
                    guardedObjectMutation.status ==
                        cr::CreativeDocumentCreateStatus::Rejected &&
                    guardedObjectMutation.reasonCode ==
                        "creative_placement_target_source_disallowed" &&
                    guardedMutation.requested && !guardedMutation.accepted &&
                    !guardedMutation.changed &&
                    guardedMutation.status ==
                        CreativeBrushPlacementMutationStatus::InvalidPlan &&
                    guardedMutation.reasonCode ==
                        "creative_placement_target_source_disallowed" &&
                    appState.facade.document().revision() == revisionBefore &&
                    appState.facade.document().objectCount() == 0U &&
                    cr::creativeUndoDepth(appState.history) == undoBefore,
                "rejected compatibility cannot mutate or create history") &&
         expect(accepted.allowed && accepted.plan.compatibility.allowed &&
                    acceptedPreview.creativePreview.itemCount == 2U &&
                    acceptedPreview.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::PlacementValid,
                "the same door turns green on a structural wall face") &&
         expect(snapped.allowed &&
                    snapped.plan.resolvedFace ==
                        cr::CreativePlacementFace::PositiveX,
                "snapped object face owns admission orientation and preview");
}

bool verticalSurfacePlacementFollowsTheAimedFace() {
  cr::CreativeGridTarget target = targetWithFacts(
      cr::resolveCreativeGridTargetFromHit(
          {2.0, 1.0, 2.0}, {1.0, 0.0, 0.0}, 1.0),
      cr::CreativePlacementTargetSource::AuthoredObject,
      cr::CreativeObjectKind::Wall, 9001U);
  const CreativeBrushPlacementAdmission wallX =
      admitBrushPlacement(cr::CreativeObjectKind::Wall, target);
  target.faceNormal = {0.0, 0.0, 1.0};
  const CreativeBrushPlacementAdmission wallZ =
      admitBrushPlacement(cr::CreativeObjectKind::Wall, target);

  target.faceNormal = {1.0, 0.0, 0.0};
  const CreativeBrushPlacementAdmission doorX =
      admitBrushPlacement(cr::CreativeObjectKind::Door, target);
  target.faceNormal = {0.0, 0.0, 1.0};
  const CreativeBrushPlacementAdmission doorZ =
      admitBrushPlacement(cr::CreativeObjectKind::Door, target);
  target.faceNormal = {0.0, 1.0, 0.0};
  target.placerForward = {1.0, 0.0, 0.0};
  const CreativeBrushPlacementAdmission doorTop =
      admitBrushPlacement(cr::CreativeObjectKind::Door, target);
  target.faceNormal = {1.0, 0.0, 0.0};
  const CreativeBrushPlacementAdmission wallRunX =
      admitBrushPlacement(cr::CreativeObjectKind::WallRunSurface, target);
  target.faceNormal = {0.0, 0.0, 1.0};
  const CreativeBrushPlacementAdmission wallRunZ =
      admitBrushPlacement(cr::CreativeObjectKind::WallRunSurface, target);
  target.faceNormal = {0.0, 1.0, 0.0};
  target.targetFacts = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::EmptyPlane);
  const CreativeBrushPlacementAdmission bridge =
      admitBrushPlacement(cr::CreativeObjectKind::Bridge, target);

  const cr::CreativeTransformedBounds doorXGeometry =
      cr::resolveCreativeTransformedBounds(doorX.plan.authoredBounds,
                                           doorX.plan.transform);
  const cr::CreativeTransformedBounds doorZGeometry =
      cr::resolveCreativeTransformedBounds(doorZ.plan.authoredBounds,
                                           doorZ.plan.transform);
  const cr::CreativeTransformedBounds wallRunXGeometry =
      cr::resolveCreativeTransformedBounds(wallRunX.plan.authoredBounds,
                                           wallRunX.plan.transform);
  const cr::CreativeTransformedBounds wallRunZGeometry =
      cr::resolveCreativeTransformedBounds(wallRunZ.plan.authoredBounds,
                                           wallRunZ.plan.transform);
  constexpr double kHalfPi = 1.57079632679489662;

  bool ok = expect(wallX.allowed && wallZ.allowed &&
                       wallX.plan.storagePolicy ==
                           cr::CreativePlacementStoragePolicy::VoxelCell &&
                       wallZ.plan.storagePolicy ==
                           cr::CreativePlacementStoragePolicy::VoxelCell &&
                       !wallX.plan.orientationResolved &&
                       !wallZ.plan.orientationResolved &&
                       sameBounds(wallX.plan.previewBounds,
                                  wallZ.plan.previewBounds) &&
                       near(static_cast<float>(extentX(
                                wallX.plan.previewBounds)),
                            1.0F) &&
                       near(static_cast<float>(extentZ(
                                wallX.plan.previewBounds)),
                            1.0F),
                   "voxel wall placement remains one cell on every aimed face") &&
            expect(doorX.allowed && doorX.plan.orientationResolved &&
                       doorX.plan.resolvedFace ==
                           cr::CreativePlacementFace::PositiveX &&
                       doorX.plan.resolvedForward ==
                           cr::CreativePlacementFace::PositiveX &&
                       near(static_cast<float>(
                                doorX.plan.transform.rotationEulerRadians.y),
                            static_cast<float>(kHalfPi)) &&
                       doorXGeometry.valid &&
                       near(static_cast<float>(extentX(
                                doorXGeometry.worldBounds)),
                            0.2F) &&
                       near(static_cast<float>(extentZ(
                                doorXGeometry.worldBounds)),
                            1.0F),
                   "authored attachment aimed at X stores +90 yaw") &&
            expect(doorZ.allowed && doorZ.plan.orientationResolved &&
                       doorZ.plan.resolvedForward ==
                           cr::CreativePlacementFace::PositiveZ &&
                       near(static_cast<float>(
                                doorZ.plan.transform.rotationEulerRadians.y),
                            0.0F) &&
                       doorZGeometry.valid &&
                       near(static_cast<float>(extentX(
                                doorZGeometry.worldBounds)),
                            1.0F) &&
                       near(static_cast<float>(extentZ(
                                doorZGeometry.worldBounds)),
                            0.2F),
                   "authored attachment aimed at Z keeps local forward on +Z") &&
            expect(!doorTop.allowed &&
                       doorTop.status ==
                           CreativeBrushPlacementAdmissionStatus::
                               TargetIncompatible &&
                       doorTop.plan.compatibility.status ==
                           cr::CreativePlacementCompatibilityStatus::
                               SurfaceDirectionDisallowed,
                   "door placement rejects a structural top face") &&
            expect(wallRunX.allowed && wallRunX.plan.orientationResolved &&
                       wallRunXGeometry.valid &&
                       near(static_cast<float>(extentX(
                                wallRunXGeometry.worldBounds)),
                            0.25F) &&
                       near(static_cast<float>(extentZ(
                                wallRunXGeometry.worldBounds)),
                            6.0F) &&
                       wallRunZ.allowed && wallRunZ.plan.orientationResolved &&
                       wallRunZGeometry.valid &&
                       near(static_cast<float>(extentX(
                                wallRunZGeometry.worldBounds)),
                            6.0F) &&
                       near(static_cast<float>(extentZ(
                                wallRunZGeometry.worldBounds)),
                            0.25F),
                   "descriptor local-forward data handles alternate thin axes") &&
            expect(bridge.allowed && !bridge.plan.orientationResolved &&
                       near(static_cast<float>(extentX(
                                bridge.plan.authoredBounds)),
                            3.0F) &&
                       near(static_cast<float>(extentZ(
                                bridge.plan.authoredBounds)),
                            8.0F),
                   "non-face-aligned brush retains descriptor dimensions");

  cr::CreativeAppState appState;
  installHistoryDocument(appState, 105U);
  const CreativeBrushPlacementMutationReceipt receipt =
      applyBrushPlacement(appState.facade, doorX.plan, 1U);
  const cr::CreativeObject* placed = appState.facade.findObject(receipt.objectId);
  const ObjectVisualPickBounds pickBounds =
      placed != nullptr
          ? buildObjectVisualPickBounds(*placed, iggy3d::identityMat4(), 800U,
                                        600U)
          : ObjectVisualPickBounds{};
  const cr::CreativeTransformedBounds placedGeometry =
      placed != nullptr ? cr::resolveCreativeObjectBounds(*placed)
                        : cr::CreativeTransformedBounds{};
  const ObjectVisualPickResult picked =
      placedGeometry.valid
          ? pickNearestVisualBoundsObject(
                {pickBounds},
                {true,
                 {static_cast<float>(placedGeometry.center.x - 2.0),
                  static_cast<float>(placedGeometry.center.y),
                  static_cast<float>(placedGeometry.center.z)},
                 {1.0F, 0.0F, 0.0F}})
          : ObjectVisualPickResult{};
  const StandaloneRoomBakePreviewScene baked =
      buildStandaloneRoomBakePreviewScene(appState.facade.document());
  const bool bakeAligned = !baked.roomBake.room.staticMeshes.empty() &&
                           near(baked.roomBake.room.staticMeshes.front()
                                    .sizeMeters.x,
                                1.0F) &&
                           near(baked.roomBake.room.staticMeshes.front()
                                    .sizeMeters.z,
                                0.2F) &&
                           near(baked.roomBake.room.staticMeshes.front()
                                    .rotationEulerRadians.y,
                                static_cast<float>(kHalfPi)) &&
                           !baked.scene.room.meshes.empty() &&
                           near(baked.scene.room.meshes.front()
                                    .rotationEulerRadians.y,
                                static_cast<float>(kHalfPi));
  const bool noGridMeshes = std::none_of(
      baked.scene.room.meshes.begin(), baked.scene.room.meshes.end(),
      [](const iggy3d::SceneRoomMeshItem& mesh) {
        return mesh.role == "grid";
      });
  return expect(receipt.accepted && receipt.objectCreated && placed != nullptr &&
                    sameBounds(placed->bounds, doorX.plan.authoredBounds) &&
                    near(static_cast<float>(
                             placed->transform.rotationEulerRadians.y),
                         static_cast<float>(kHalfPi)),
                "committed attachment keeps authored bounds and admitted yaw") &&
         expect(bakeAligned && noGridMeshes,
                "room bake carries yaw without legacy grid meshes") &&
         expect(pickBounds.orientedBounds.has_value() &&
                    picked.objectId == receipt.objectId,
                "placed cardinal object is picked through its oriented bounds") &&
         ok;
}

bool surfaceFramePlacementFollowsExactNormalsAndKeepsUprightPropsUpright() {
  constexpr double kHalfPi = 1.57079632679489661923;
  constexpr double kQuarterPi = 0.78539816339744830962;
  constexpr double kSqrtHalf = 0.70710678118654752440;

  cr::CreativeGridTarget diagonal;
  diagonal.valid = true;
  diagonal.faceNormal = {kSqrtHalf, 0.0, kSqrtHalf};
  diagonal.placerForward = {0.0, 0.0, -1.0};
  diagonal.placementAnchor = {3.0, 2.0, 4.0};
  diagonal.placementNormal = diagonal.faceNormal;
  diagonal.anchorSnapped = true;
  diagonal.targetFacts = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::AuthoredObject,
      cr::CreativeObjectKind::Wall, 9002U);
  const CreativeBrushPlacementAdmission door = admitBrushPlacement(
      cr::CreativeObjectKind::Door, diagonal);

  cr::CreativeGridTarget slope = diagonal;
  slope.faceNormal = {0.6, 0.8, 0.0};
  slope.placementNormal = slope.faceNormal;
  const CreativeBrushPlacementAdmission rejectedWindow = admitBrushPlacement(
      cr::CreativeObjectKind::Window, slope);
  const CreativeBrushPlacementAdmission surface = admitBrushPlacement(
      cr::CreativeObjectKind::Sign, slope);
  const CreativeBrushPlacementAdmission surfaceRolled = admitBrushPlacement(
      cr::CreativeObjectKind::Sign, slope,
      cr::CreativePlacementYaw::Degrees90);
  const CreativeBrushPlacementAdmission crate = admitBrushPlacement(
      cr::CreativeObjectKind::Crate, slope);
  const cr::CreativeDocumentCreateRequest surfaceRequest =
      buildBrushCreateRequest(surface.plan, 91U);

  const auto rotated = [](cr::CreativeVec3 axis,
                          const CreativeBrushPlacementAdmission& admission) {
    return cr::rotateCreativeVectorEulerXyz(
        axis, admission.plan.transform.rotationEulerRadians);
  };
  CreativeBrushPlacementPlan explicitTransformPlan = surface.plan;
  cr::CreativeTransform explicitTransform;
  explicitTransform.position = {8.0, 2.0, -1.0};
  explicitTransform.rotationEulerRadians = {0.0, kHalfPi, 0.0};
  const bool explicitTransformApplied = applyCreativeAssetPlacementTransform(
      explicitTransformPlan, surface.plan.authoredBounds, explicitTransform);
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 109U);
  const CreativeBrushPlacementMutationReceipt placedSurface =
      applyBrushPlacement(appState.facade, surface.plan, 1U);
  const cr::CreativeObject* placedSurfaceObject =
      appState.facade.findObject(placedSurface.objectId);

  return expect(door.allowed && door.plan.orientationResolved &&
                    !door.plan.surfaceFrame.valid &&
                    near(static_cast<float>(
                             door.plan.transform.rotationEulerRadians.x),
                         0.0F) &&
                    near(static_cast<float>(
                             door.plan.transform.rotationEulerRadians.y),
                         static_cast<float>(kQuarterPi)) &&
                    near(static_cast<float>(
                             door.plan.transform.rotationEulerRadians.z),
                         0.0F) &&
                    nearVec3(rotated({0.0, 0.0, 1.0}, door),
                             diagonal.faceNormal) &&
                    door.plan.contact.valid &&
                    door.plan.contact.sourceFeatureVertexCount == 4U,
                "upright attachments follow exact wall yaw without tilting") &&
         expect(!rejectedWindow.allowed &&
                    rejectedWindow.status ==
                        CreativeBrushPlacementAdmissionStatus::
                            TargetIncompatible &&
                    rejectedWindow.plan.compatibility.status ==
                        cr::CreativePlacementCompatibilityStatus::
                            SurfaceDirectionDisallowed,
                "window placement rejects a sloped structural face") &&
         expect(surface.allowed && surface.plan.orientationResolved &&
                    surface.plan.surfaceFrame.valid &&
                    nearVec3(rotated({0.0, 0.0, 1.0}, surface),
                             surface.plan.surfaceFrame.surfaceNormal) &&
                    nearVec3(rotated({0.0, 1.0, 0.0}, surface),
                             surface.plan.surfaceFrame.surfaceUp) &&
                    surface.plan.contact.valid &&
                    surface.plan.contact.sourceFeatureVertexCount == 4U &&
                    surface.plan.contact.minimumSignedDistanceMeters >=
                        -1.0e-9,
                "surface-bound dressing aligns a full face to a slope") &&
         expect(surfaceRolled.allowed &&
                    surfaceRolled.plan.surfaceFrame.valid &&
                    nearVec3(rotated({0.0, 0.0, 1.0}, surfaceRolled),
                             surface.plan.surfaceFrame.surfaceNormal) &&
                    std::fabs(surfaceRolled.plan.surfaceFrame.surfaceUp.x *
                                  surface.plan.surfaceFrame.surfaceUp.x +
                              surfaceRolled.plan.surfaceFrame.surfaceUp.y *
                                  surface.plan.surfaceFrame.surfaceUp.y +
                              surfaceRolled.plan.surfaceFrame.surfaceUp.z *
                                  surface.plan.surfaceFrame.surfaceUp.z) <=
                        1.0e-9,
                "placement orientation rotates a flush object around its surface normal") &&
         expect(crate.allowed && !crate.plan.orientationResolved &&
                    !crate.plan.surfaceFrame.valid &&
                    sameVec3(crate.plan.transform.rotationEulerRadians, {}),
                "ordinary props keep descriptor-default upright orientation") &&
         expect(sameTransform(surfaceRequest.transform,
                              surface.plan.transform) &&
                    sameBounds(surfaceRequest.bounds,
                               surface.plan.authoredBounds),
                "surface-frame preview and create request share one transform") &&
         expect(placedSurface.accepted && placedSurfaceObject != nullptr &&
                    sameTransform(placedSurfaceObject->transform,
                                  surface.plan.transform) &&
                    sameBounds(placedSurfaceObject->bounds,
                               surface.plan.authoredBounds),
                "document mutation preserves the admitted full surface transform") &&
         expect(explicitTransformApplied &&
                    !explicitTransformPlan.surfaceFrame.valid &&
                    !explicitTransformPlan.contact.valid &&
                    sameTransform(explicitTransformPlan.transform,
                                  explicitTransform),
                "explicit socket-style transforms supersede derived surface frames");
}

bool quickEditOrientationFeedsPreviewAndCreatePlan() {
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Door);
  setPlaceTarget(editor, 2, 0, -1);
  editor.interaction.target.grid.faceNormal = {0.0, 0.0, 1.0};
  editor.interaction.target.grid.targetFacts =
      cr::makeCreativePlacementTargetFacts(
          cr::CreativePlacementTargetSource::AuthoredObject,
          cr::CreativeObjectKind::Wall, 9003U);
  syncCreativeEditorQuickEdit(editor);
  const CreativeBrushPlacementAdmission initial = admitBrushPlacement(
      cr::CreativeObjectKind::Door, editor.interaction.target.grid,
      editor.toolSettings.placementYaw);
  const bool orientationChanged = processCreativeEditorQuickEditAction(
      editor, cr::CreativeInputActionId::QuickEditIncrease);
  const CreativeBrushPlacementAdmission rotated = admitBrushPlacement(
      cr::CreativeObjectKind::Door, editor.interaction.target.grid,
      editor.toolSettings.placementYaw);
  const cr::CreativeDocumentCreateRequest request =
      buildBrushCreateRequest(rotated.plan, 7U);
  const CreativeBrushPlacementAdmission invalid = admitBrushPlacement(
      cr::CreativeObjectKind::Door, editor.interaction.target.grid,
      cr::CreativePlacementYaw::Count);
  CreativeEditorState voxelEditor =
      materialEditor(cr::CreativeObjectKind::Wall);
  setPlaceTarget(voxelEditor, 0);
  voxelEditor.toolSettings.placementYaw =
      cr::CreativePlacementYaw::Degrees90;
  syncCreativeEditorQuickEdit(voxelEditor);
  const CreativeBrushPlacementAdmission voxel = admitBrushPlacement(
      cr::CreativeObjectKind::Wall, voxelEditor.interaction.target.grid,
      voxelEditor.toolSettings.placementYaw);
  cr::CreativeGridTarget pathTarget = editor.interaction.target.grid;
  pathTarget.faceNormal = {0.0, 1.0, 0.0};
  pathTarget.targetFacts = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::EmptyPlane);
  bool sawPath = false;
  bool pathYawIgnored = true;
  for (const cr::CreativeObjectDescriptor& descriptor :
       cr::allObjectDescriptors()) {
    if (descriptor.shapeKind != cr::CreativeObjectShapeKind::Path ||
        !descriptorSupportsBrushPlacement(descriptor)) {
      continue;
    }
    const CreativeBrushPlacementAdmission pathDefault = admitBrushPlacement(
        descriptor.kind, pathTarget,
        cr::CreativePlacementYaw::Degrees0);
    const CreativeBrushPlacementAdmission pathRotated = admitBrushPlacement(
        descriptor.kind, pathTarget,
        cr::CreativePlacementYaw::Degrees90);
    if (pathDefault.allowed) {
      sawPath = true;
      pathYawIgnored = pathRotated.allowed &&
                       sameTransform(pathDefault.plan.transform,
                                     pathRotated.plan.transform);
      break;
    }
  }
  constexpr double kHalfPi = 1.57079632679489662;

  bool ok = expect(initial.allowed && rotated.allowed && orientationChanged &&
                       editor.toolSettings.placementYaw ==
                           cr::CreativePlacementYaw::Degrees90 &&
                       near(static_cast<float>(
                                rotated.plan.transform.rotationEulerRadians.y -
                                initial.plan.transform.rotationEulerRadians.y),
                            static_cast<float>(kHalfPi)),
                   "dpad orientation adds one quarter turn to placement") &&
            expect(sameTransform(request.transform, rotated.plan.transform) &&
                       sameBounds(request.bounds,
                                  rotated.plan.authoredBounds),
                   "rotated preview plan is the create request source") &&
            expect(creativeEditorQuickEditStatusLabel(editor) ==
                       "ORIENTATION 90 DEG" &&
                       creativeEditorHeldItemStatusLabel(editor) ==
                           "Material | Door | [ORIENTATION 90 DEG]",
                   "held HUD exposes the selected dpad channel and value") &&
            expect(!invalid.allowed &&
                       invalid.status ==
                           CreativeBrushPlacementAdmissionStatus::
                               InvalidGeometry,
                   "invalid orientation fails closed") &&
            expect(voxel.allowed &&
                       !voxel.plan.orientationResolved &&
                       sameVec3(voxel.plan.transform.rotationEulerRadians,
                                {}) &&
                       creativeEditorQuickEditStatusLabel(voxelEditor) ==
                           "GRID DOTS OFF",
                   "voxel placement keeps only lattice-wide quick edits") &&
            expect(sawPath && pathYawIgnored,
                   "path brushes ignore global yaw without becoming invalid");

  ok = expect(processCreativeEditorQuickEditAction(
                  editor, cr::CreativeInputActionId::QuickEditNext) &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "GRID SIZE 1 M",
              "dpad down selects the next bounded edit channel") &&
       ok;
  ok = expect(processCreativeEditorQuickEditAction(
                  editor, cr::CreativeInputActionId::QuickEditDecrease) &&
                  editor.toolSettings.snapIncrement ==
                      cr::CreativeSnapIncrement::HalfMeter &&
                  editor.placeCellSize == 0.5,
              "dpad left adjusts the selected channel immediately") &&
       ok;
  ok = expect(processCreativeEditorQuickEditAction(
                  editor, cr::CreativeInputActionId::QuickEditNext) &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "GRID DOTS OFF" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditIncrease) &&
                  editor.toolSettings.placementGridDots ==
                      cr::CreativePlacementGridDots::NearestLayer &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "GRID DOTS NEAREST",
              "square and dpad expose the nearest-layer dot toggle") &&
       ok;
  ok = expect(processCreativeEditorQuickEditAction(
                  editor, cr::CreativeInputActionId::QuickEditNext) &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "PLACEMENT PLANE AUTO" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditIncrease) &&
                  editor.toolSettings.placementPlane ==
                      cr::CreativePlacementPlane::X &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "PLACEMENT PLANE X",
              "square and dpad expose a reusable placement plane lock") &&
       ok;
  ok = expect(processCreativeEditorQuickEditAction(
                  editor, cr::CreativeInputActionId::QuickEditNext) &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "PLACEMENT ANCHOR CENTER" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditIncrease) &&
                  editor.toolSettings.placementAnchor ==
                      cr::CreativePlacementAnchor::Face &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "PLACEMENT ANCHOR FACE",
              "square and dpad expose authored placement anchors") &&
       ok;
  ok = expect(processCreativeEditorQuickEditAction(
                  editor, cr::CreativeInputActionId::QuickEditNext) &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditIncrease) &&
                  editor.toolSettings.placementDepth ==
                      cr::CreativePlacementDepth::OneCell &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "PLACEMENT DEPTH 1 CELL" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditDecrease) &&
                  editor.toolSettings.placementDepth ==
                      cr::CreativePlacementDepth::ZeroCells,
              "dpad right moves placement away and left brings it back") &&
       ok;
  return ok;
}

bool toolOptionsFollowTheRequestedMaterialEntry() {
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  const cr::CreativeHotbarEntry wall{
      cr::CreativeHeldItemKind::Material, cr::CreativeObjectKind::Wall};
  const cr::CreativeHotbarEntry door{
      cr::CreativeHeldItemKind::Material, cr::CreativeObjectKind::Door};
  const cr::CreativeHotbarEntry invalid{
      cr::CreativeHeldItemKind::Material, cr::CreativeObjectKind::Unknown};
  const cr::CreativeHotbarEntry materialBrush{
      cr::CreativeHeldItemKind::MaterialBrush,
      cr::CreativeObjectKind::Wall};

  const cr::CreativeToolOptionList wallOptions =
      creativeEditorToolOptionsForEntry(wall, editor.toolSettings);
  const cr::CreativeToolOptionList doorOptions =
      creativeEditorToolOptionsForEntry(door, editor.toolSettings);
  const cr::CreativeToolOptionList invalidOptions =
      creativeEditorToolOptionsForEntry(invalid, editor.toolSettings);
  const CreativeEditorToolOptionsCommandList brushCommands =
      creativeEditorToolOptionCommandsForEntry(materialBrush);
  const CreativeEditorToolOptionsCommandList doorCommands =
      creativeEditorToolOptionCommandsForEntry(door);

  editor.interaction.hotbar.entries[0] = door;
  syncCreativeEditorQuickEdit(editor);
  const bool doorQuickEditReady =
      editor.quickEdit.targetEntry.objectKind == cr::CreativeObjectKind::Door &&
      creativeEditorQuickEditStatusLabel(editor) == "ORIENTATION 0 DEG";
  editor.interaction.hotbar.entries[0] = wall;
  const bool staleDoorStatusHidden =
      creativeEditorQuickEditStatusLabel(editor).empty();
  syncCreativeEditorQuickEdit(editor);

  return expect(wallOptions.count == 3U &&
                    wallOptions.ids[0] ==
                        cr::CreativeToolOptionId::PlacementGridDots &&
                    wallOptions.ids[1] ==
                        cr::CreativeToolOptionId::PlacementPlane &&
                    wallOptions.ids[2] ==
                        cr::CreativeToolOptionId::PlacementDepth,
                "fixed-grid material exposes only lattice-wide placement options") &&
         expect(doorOptions.count == 6U &&
                    doorOptions.ids[0] ==
                        cr::CreativeToolOptionId::PlacementYaw &&
                    doorOptions.ids[1] ==
                        cr::CreativeToolOptionId::SnapIncrement &&
                    doorOptions.ids[2] ==
                        cr::CreativeToolOptionId::PlacementGridDots &&
                    doorOptions.ids[3] ==
                        cr::CreativeToolOptionId::PlacementPlane &&
                    doorOptions.ids[4] ==
                        cr::CreativeToolOptionId::PlacementAnchor &&
                    doorOptions.ids[5] ==
                        cr::CreativeToolOptionId::PlacementDepth,
                "authored material owns orientation grid dots plane anchor and depth") &&
         expect(invalidOptions.count == 0U,
                "invalid material option targets fail closed") &&
         expect(brushCommands.count == 2U &&
                    brushCommands.ids[0] ==
                        CreativeEditorToolOptionsCommandId::
                            SetMaterialBrushSymmetryPivot &&
                    brushCommands.ids[1] ==
                        CreativeEditorToolOptionsCommandId::
                            ClearMaterialBrushSymmetryPivot &&
                    doorCommands.count == 0U,
                "only material brush options expose bounded pivot commands") &&
         expect(doorQuickEditReady && staleDoorStatusHidden &&
                    editor.quickEdit.targetEntry.objectKind ==
                        cr::CreativeObjectKind::Wall &&
                    editor.quickEdit.options.count == 3U &&
                    creativeEditorQuickEditStatusLabel(editor) ==
                        "GRID DOTS OFF",
                "quick edit tracks the complete material entry without stale labels");
}

bool toolOptionsActivateSymmetryPivotCommands() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 105U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  const cr::CreativeHotbarEntry brush{
      cr::CreativeHeldItemKind::MaterialBrush,
      cr::CreativeObjectKind::Wall};
  editor.interaction.hotbar.entries[0] = brush;
  const cr::CreativeDocumentId documentId = appState.facade.document().id();
  updateCreativeMaterialBrushPivotAim(
      editor.interaction.materialBrushPivot, documentId, true, {4, 2, -3});

  const auto openOptions = [&]() {
    editor.toolOptions.open = true;
    editor.toolOptions.targetEntry = brush;
    editor.toolOptions.draft = editor.toolSettings;
    editor.toolOptions.options =
        creativeEditorToolOptionsForEntry(brush, editor.toolSettings);
    editor.toolOptions.commands =
        creativeEditorToolOptionCommandsForEntry(brush);
  };

  openOptions();
  editor.toolOptions.draft.materialBrushSymmetry =
      cr::CreativeMaterialBrushSymmetry::MirrorXZ;
  editor.toolOptions.selectedIndex = editor.toolOptions.options.count;
  const bool setAccepted =
      activateCreativeEditorToolOptionsSelection(editor);
  bool ok = expect(setAccepted && !editor.toolOptions.open &&
                       editor.toolSettings.materialBrushSymmetry ==
                           cr::CreativeMaterialBrushSymmetry::MirrorXZ &&
                       editor.interaction.materialBrushPresets.initialized[0] !=
                           0U &&
                       editor.interaction.materialBrushPresets.slots[0]
                               .symmetry ==
                           cr::CreativeMaterialBrushSymmetry::MirrorXZ &&
                       editor.interaction.materialBrushPivot.locked &&
                       editor.interaction.materialBrushPivot.lockedCell.x == 4 &&
                       editor.interaction.materialBrushPivot.lockedCell.y == 2 &&
                       editor.interaction.materialBrushPivot.lockedCell.z == -3,
                   "modal confirm commits draft settings and locks aimed pivot");

  openOptions();
  editor.toolOptions.selectedIndex =
      editor.toolOptions.options.count + 1U;
  const bool clearAccepted =
      activateCreativeEditorToolOptionsSelection(editor);
  ok = expect(clearAccepted && !editor.toolOptions.open &&
                  !editor.interaction.materialBrushPivot.locked,
              "clear command removes the pivot through the same confirm path") &&
       ok;

  resetCreativeMaterialBrushPivot(editor.interaction.materialBrushPivot,
                                  documentId);
  openOptions();
  editor.toolOptions.selectedIndex = editor.toolOptions.options.count;
  const bool unavailableSet =
      activateCreativeEditorToolOptionsSelection(editor);
  return expect(!unavailableSet && editor.toolOptions.open &&
                    !editor.interaction.materialBrushPivot.locked &&
                    creativeEditorToolOptionsRowCount(editor.toolOptions) ==
                        editor.toolOptions.options.count + 2U,
                "set command fails closed without a viewport aim snapshot") &&
         ok;
}

bool materialBrushPresetsFollowHotbarSlots() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 106U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::MaterialBrush,
      cr::CreativeObjectKind::Wall};
  editor.interaction.hotbar.entries[1] = {
      cr::CreativeHeldItemKind::MaterialBrush,
      cr::CreativeObjectKind::Floor};
  editor.toolSettings.arrayMode = cr::CreativeArrayMode::Radial;
  syncCreativeEditorHeldItem(appState, editor);
  const bool initializedFirst =
      activateSelectedCreativeMaterialBrushPreset(
          editor.interaction.materialBrushPresets,
          editor.interaction.hotbar, editor.toolSettings);
  const std::uint64_t revisionBefore =
      appState.facade.document().revision();

  bool ok = expect(initializedFirst &&
                       editor.interaction.materialBrushPresets.initialized[0] !=
                           0U &&
                       editor.toolSettings.materialBrushShape ==
                           cr::CreativeMaterialBrushShape::Sphere,
                   "first brush slot captures the current brush configuration");
  ok = expect(selectCreativeEditorHotbarSlot(appState, editor, 1U) &&
                  editor.interaction.materialBrushPresets.initialized[1] != 0U &&
                  editor.toolSettings.materialBrushShape ==
                      cr::CreativeMaterialBrushShape::Sphere,
              "new brush slot clones the outgoing brush configuration") &&
       ok;
  ok = expect(processCreativeEditorQuickEditAction(
                  editor, cr::CreativeInputActionId::QuickEditIncrease) &&
                  editor.toolSettings.materialBrushShape ==
                      cr::CreativeMaterialBrushShape::Cylinder &&
                  editor.interaction.materialBrushPresets.slots[1].shape ==
                      cr::CreativeMaterialBrushShape::Cylinder,
              "D-pad editing writes through to the selected brush slot") &&
       ok;
  editor.toolSettings.materialBrushAxis = cr::CreativeAxis3::X;
  editor.toolSettings.materialBrushFill =
      cr::CreativeMaterialBrushFill::Shell;
  editor.toolSettings.materialBrushGuide =
      cr::CreativeMaterialBrushGuide::LineY;
  editor.toolSettings.materialBrushSymmetry =
      cr::CreativeMaterialBrushSymmetry::MirrorX;
  editor.toolSettings.materialBrushMask =
      cr::CreativeMaterialBrushMask::Replace;
  editor.toolSettings.materialBrushReplaceSourceKind =
      cr::CreativeObjectKind::Wall;
  static_cast<void>(storeSelectedCreativeMaterialBrushPreset(
      editor.interaction.materialBrushPresets,
      editor.interaction.hotbar, editor.toolSettings));

  ok = expect(selectCreativeEditorHotbarSlot(appState, editor, 0U) &&
                  editor.toolSettings.materialBrushShape ==
                      cr::CreativeMaterialBrushShape::Sphere &&
                  editor.toolSettings.materialBrushSize ==
                      cr::CreativeMaterialBrushSize::ThreeCells &&
                  editor.toolSettings.materialBrushFill ==
                      cr::CreativeMaterialBrushFill::Solid &&
                  editor.toolSettings.materialBrushGuide ==
                      cr::CreativeMaterialBrushGuide::Free &&
                  editor.toolSettings.materialBrushSymmetry ==
                      cr::CreativeMaterialBrushSymmetry::Off &&
                  editor.toolSettings.materialBrushMask ==
                      cr::CreativeMaterialBrushMask::Overwrite &&
                  editor.toolSettings.materialBrushReplaceSourceKind ==
                      cr::CreativeObjectKind::Unknown,
              "returning to slot one restores only its brush fields") &&
       ok;
  ok = expect(processCreativeEditorQuickEditAction(
                  editor, cr::CreativeInputActionId::QuickEditNext) &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditIncrease) &&
                  editor.toolSettings.materialBrushSize ==
                      cr::CreativeMaterialBrushSize::FiveCells,
              "slot one keeps an independent D-pad size preset") &&
       ok;
  ok = expect(selectCreativeEditorHotbarSlot(appState, editor, 1U) &&
                  editor.toolSettings.materialBrushShape ==
                      cr::CreativeMaterialBrushShape::Cylinder &&
                  editor.toolSettings.materialBrushAxis == cr::CreativeAxis3::X &&
                  editor.toolSettings.materialBrushSize ==
                      cr::CreativeMaterialBrushSize::ThreeCells &&
                  editor.toolSettings.materialBrushFill ==
                      cr::CreativeMaterialBrushFill::Shell &&
                  editor.toolSettings.materialBrushGuide ==
                      cr::CreativeMaterialBrushGuide::LineY &&
                  editor.toolSettings.materialBrushSymmetry ==
                      cr::CreativeMaterialBrushSymmetry::MirrorX &&
                  editor.toolSettings.materialBrushMask ==
                      cr::CreativeMaterialBrushMask::Replace &&
                  editor.toolSettings.materialBrushReplaceSourceKind ==
                      cr::CreativeObjectKind::Wall &&
                  editor.toolSettings.arrayMode == cr::CreativeArrayMode::Radial,
              "slot two restores its preset without changing array settings") &&
       ok;

  CreativeMaterialBrushGestureConfig firstPreset;
  CreativeMaterialBrushGestureConfig secondPreset;
  const bool foundFirst = creativeMaterialBrushPresetForSlot(
      editor.interaction.materialBrushPresets, 0U, firstPreset);
  const bool foundSecond = creativeMaterialBrushPresetForSlot(
      editor.interaction.materialBrushPresets, 1U, secondPreset);
  ok = expect(foundFirst && foundSecond &&
                  creativeMaterialBrushPresetHotbarLabel(firstPreset) ==
                      "S5" &&
                  creativeMaterialBrushPresetHotbarLabel(secondPreset) ==
                      "CX3HM",
              "compact hotbar labels distinguish independent brush presets") &&
       ok;
  CreativeMaterialBrushGestureConfig invalidPreset = secondPreset;
  invalidPreset.size = cr::CreativeMaterialBrushSize::Count;
  ok = expect(creativeMaterialBrushPresetHotbarLabel(invalidPreset) == "B?",
              "invalid brush presets fail closed in the hotbar label") &&
       ok;

  static_cast<void>(selectCreativeEditorHotbarSlot(appState, editor, 0U));
  clearCreativeMaterialBrushPresetSlot(
      editor.interaction.materialBrushPresets, 1U);
  static_cast<void>(selectCreativeEditorHotbarSlot(appState, editor, 1U));
  return expect(editor.toolSettings.materialBrushShape ==
                    cr::CreativeMaterialBrushShape::Sphere &&
                    editor.toolSettings.materialBrushSize ==
                        cr::CreativeMaterialBrushSize::FiveCells &&
                    editor.toolSettings.materialBrushFill ==
                        cr::CreativeMaterialBrushFill::Solid,
                "cleared slot clones the current brush instead of stale data") &&
         expect(appState.facade.document().revision() == revisionBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "preset switching does not mutate the document or history") &&
         ok;
}

bool heldItemLifecyclePreservesCompatibleDraftsAndCancelsOthers() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 150U);
  CreativeEditorState editor;
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::TerrainPath,
      cr::CreativeObjectKind::Unknown};
  editor.interaction.hotbar.entries[1] = {
      cr::CreativeHeldItemKind::TerrainProfile,
      cr::CreativeObjectKind::Unknown};
  editor.interaction.hotbar.entries[2] = {
      cr::CreativeHeldItemKind::VolumeFill,
      cr::CreativeObjectKind::Floor};
  editor.interaction.hotbar.entries[3] = {
      cr::CreativeHeldItemKind::VolumeHollow,
      cr::CreativeObjectKind::Floor};

  const bool initialSelectionChanged =
      selectCreativeEditorHotbarSlot(appState, editor, 0U);
  editor.terrain.path.pointCount = 2U;
  editor.terrain.path.points[0].coord = {2, 3};
  editor.terrain.path.points[1].coord = {5, 7};
  const bool repeatedSelectionChanged =
      selectCreativeEditorHotbarSlot(appState, editor, 0U);
  bool ok = expect(!initialSelectionChanged && !repeatedSelectionChanged &&
                       editor.terrain.path.pointCount == 2U &&
                       editor.terrain.path.points[0].coord.x == 2 &&
                       editor.terrain.path.points[1].coord.z == 7 &&
                       editor.interaction.synchronizedHeldItemKind ==
                           cr::CreativeHeldItemKind::TerrainPath,
                   "reselecting one draft tool preserves its active draft");

  const bool profileSelected =
      selectCreativeEditorHotbarSlot(appState, editor, 1U);
  ok = expect(profileSelected && editor.terrain.path.pointCount == 0U &&
                  editor.interaction.synchronizedHeldItemKind ==
                      cr::CreativeHeldItemKind::TerrainProfile,
              "switching draft ownership cancels the incompatible draft") &&
       ok;

  const bool fillSelected =
      selectCreativeEditorHotbarSlot(appState, editor, 2U);
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::First, {1, 2, 3}));
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::Second, {4, 5, 6}));
  const bool hollowSelected =
      selectCreativeEditorHotbarSlot(appState, editor, 3U);
  return expect(fillSelected && hollowSelected && editor.volume.active &&
                    cr::creativeVolumeSelectionComplete(
                        editor.volume.selection) &&
                    sameCell(editor.volume.selection.firstCell, {1, 2, 3}) &&
                    sameCell(editor.volume.selection.secondCell, {4, 5, 6}) &&
                    editor.volume.operation ==
                        cr::CreativeVolumeOperationKind::Hollow,
                "compatible volume tools share one selection lifecycle") &&
         ok;
}

bool continuousGestureOwnerIsExclusive() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 151U);
  CreativeEditorState editor;
  cr::CreativeHotbarEntry& held =
      editor.interaction.hotbar.entries[editor.interaction.hotbar.selectedSlot];

  held = {cr::CreativeHeldItemKind::Material, cr::CreativeObjectKind::Wall};
  bool ok = expect(creativeEditorContinuousGestureOwner(editor) ==
                       CreativeEditorContinuousGestureOwner::Material,
                   "ordinary material owns the material stroke");

  cr::CreativeAuthoredAssetDefinition authored;
  authored.assetId = "authored/lifecycle";
  authored.sourceBounds = {{-0.5, 0.0, -0.5}, {0.5, 1.0, 0.5}};
  editor.authoredAssets.definitions.push_back(authored);
  held = {cr::CreativeHeldItemKind::Material,
          cr::CreativeObjectKind::PrefabInstance};
  const bool authoredEquipped = cr::setCreativeHotbarAsset(
      held, authored.assetId, authored.sourceBounds);
  ok = expect(authoredEquipped &&
                  creativeEditorContinuousGestureOwner(editor) ==
                      CreativeEditorContinuousGestureOwner::AuthoredAsset,
              "authored assets own their dedicated stroke") &&
       ok;

  held = {cr::CreativeHeldItemKind::Material, cr::CreativeObjectKind::Wall};
  const bool scatterEquipped = cr::setCreativeHotbarAsset(
      held, "mesh/lifecycle", authored.sourceBounds);
  editor.toolSettings.assetPlacementMode =
      cr::CreativeAssetPlacementMode::Scatter;
  ok = expect(scatterEquipped &&
                  creativeEditorContinuousGestureOwner(editor) ==
                      CreativeEditorContinuousGestureOwner::AssetScatter,
              "scatter assets own their dedicated stroke") &&
       ok;

  const auto expectTerrainOwner = [&](cr::CreativeHeldItemKind kind,
                                      CreativeEditorContinuousGestureOwner owner,
                                      std::string_view message) {
    held = {kind, cr::CreativeObjectKind::Unknown};
    return expect(creativeEditorContinuousGestureOwner(editor) == owner,
                  message);
  };
  ok = expectTerrainOwner(
           cr::CreativeHeldItemKind::TerrainControl,
           CreativeEditorContinuousGestureOwner::TerrainControl,
           "terrain control owns one continuous gesture") &&
       expectTerrainOwner(cr::CreativeHeldItemKind::TerrainPaint,
                          CreativeEditorContinuousGestureOwner::TerrainPaint,
                          "terrain paint owns one continuous gesture") &&
       expectTerrainOwner(cr::CreativeHeldItemKind::TerrainSculpt,
                          CreativeEditorContinuousGestureOwner::TerrainSculpt,
                          "terrain sculpt owns one continuous gesture") &&
       ok;
  held = {cr::CreativeHeldItemKind::ObjectSelect,
          cr::CreativeObjectKind::Unknown};
  ok = expect(creativeEditorContinuousGestureOwner(editor) ==
                  CreativeEditorContinuousGestureOwner::None,
              "discrete tools do not claim a continuous gesture") &&
       ok;

  editor.interaction.materialStroke.repeat.active = true;
  editor.interaction.authoredAssetStroke.repeat.active = true;
  editor.interaction.assetScatter.repeat.active = true;
  editor.terrain.stroke.repeat.active = true;
  editor.terrainPaint.repeat.active = true;
  editor.terrain.sculpt.stroke.repeat.active = true;
  finalizeCreativeEditorContinuousGesturesExcept(
      appState, editor, CreativeEditorContinuousGestureOwner::TerrainSculpt,
      "test_continuous_gesture_owner");
  ok = expect(!editor.interaction.materialStroke.repeat.active &&
                  !editor.interaction.authoredAssetStroke.repeat.active &&
                  !editor.interaction.assetScatter.repeat.active &&
                  !editor.terrain.stroke.repeat.active &&
                  !editor.terrainPaint.repeat.active &&
                  editor.terrain.sculpt.stroke.repeat.active,
              "the active gesture survives while every non-owner finalizes") &&
       ok;

  finalizeCreativeEditorContinuousGestures(
      appState, editor, "test_continuous_gesture_finalize_all");
  return expect(!editor.terrain.sculpt.stroke.repeat.active,
                "an interruption finalizes the remaining gesture") &&
         ok;
}

bool previewFrameUsesWorldTargetAndViewHeldTransforms() {
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  setPlaceTarget(editor, 2, 1, -3);
  iggy3d::FrameInput frame;
  attachCreativeEditorPlacementPreviews(editor, false, frame);

  const CreativeBrushPlacementAdmission admission = admitBrushPlacement(
      cr::CreativeObjectKind::Wall, editor.interaction.target.grid);
  const CreativeBrushPlacementPlan& plan = admission.plan;
  const float targetSizeX =
      static_cast<float>(plan.previewBounds.max.x - plan.previewBounds.min.x);
  const float targetSizeY =
      static_cast<float>(plan.previewBounds.max.y - plan.previewBounds.min.y);
  const float targetSizeZ =
      static_cast<float>(plan.previewBounds.max.z - plan.previewBounds.min.z);
  const iggy3d::Mat4& target = frame.creativePreview.items[0].clipFromModel;
  const iggy3d::Mat4& held = frame.creativePreview.items[1].clipFromModel;
  const auto axisLength = [&](std::uint32_t column) {
    const float x = iggy3d::at(held, 0U, column);
    const float y = iggy3d::at(held, 1U, column);
    const float z = iggy3d::at(held, 2U, column);
    return std::sqrt(x * x + y * y + z * z);
  };
  const float heldX = axisLength(0U);
  const float heldY = axisLength(1U);
  const float heldZ = axisLength(2U);

  bool ok = expect(frame.creativePreview.itemCount == 2U,
                   "valid aim emits target then held preview") &&
            expect(frame.creativePreview.items[0].role ==
                           iggy3d::RenderCreativePreviewRole::PlacementValid &&
                       frame.creativePreview.items[1].role ==
                           iggy3d::RenderCreativePreviewRole::Held,
                   "preview roles retain target-before-held order") &&
            expect(admission.allowed && plan.hasVoxelCell &&
                       near(iggy3d::at(target, 0U, 0U), targetSizeX) &&
                       near(iggy3d::at(target, 1U, 1U), targetSizeY) &&
                       near(iggy3d::at(target, 2U, 2U), targetSizeZ),
                   "voxel target transform preserves the exact aimed cell") &&
            expect(near(iggy3d::at(held, 0U, 3U), 0.42F) &&
                       near(iggy3d::at(held, 1U, 3U), -0.32F) &&
                       near(iggy3d::at(held, 2U, 3U), -0.82F),
                   "held object uses the fixed view-space position") &&
            expect(near(heldX, 0.32F) && near(heldY, 0.32F) &&
                       near(heldZ, 0.32F),
                   "voxel material uses a stable canonical held cube");

  editor.interaction.target.grid.faceNormal = {1.0, 0.0, 0.0};
  attachCreativeEditorPlacementPreviews(editor, false, frame);
  const iggy3d::Mat4& sideTarget =
      frame.creativePreview.items[0].clipFromModel;
  const auto targetAxisLength = [&](std::uint32_t column) {
    const float x = iggy3d::at(sideTarget, 0U, column);
    const float y = iggy3d::at(sideTarget, 1U, column);
    const float z = iggy3d::at(sideTarget, 2U, column);
    return std::sqrt(x * x + y * y + z * z);
  };
  ok = expect(near(targetAxisLength(0U), 1.0F) &&
                  near(targetAxisLength(1U), 1.0F) &&
                  near(targetAxisLength(2U), 1.0F) &&
                  near(iggy3d::at(sideTarget, 2U, 0U), 0.0F) &&
                  near(iggy3d::at(sideTarget, 0U, 2U), 0.0F),
              "voxel target remains an unrotated cell on side faces") &&
       ok;

  editor.interaction.target = {};
  attachCreativeEditorPlacementPreviews(editor, false, frame);
  ok = expect(frame.creativePreview.itemCount == 1U &&
                  frame.creativePreview.items[0].role ==
                      iggy3d::RenderCreativePreviewRole::Held,
              "no target leaves only held object") &&
       ok;

  setPlaceTarget(editor, 2, 1, -3);
  editor.interaction.placementFeedback = {
      CreativeEditorPlacementFeedbackStatus::Rejected, cr::kInvalidObjectId,
      cr::CreativeObjectKind::Wall, editor.frameIndex};
  attachCreativeEditorPlacementPreviews(editor, false, frame);
  ok = expect(frame.creativePreview.itemCount == 2U &&
                  frame.creativePreview.items[0].role ==
                      iggy3d::RenderCreativePreviewRole::PlacementInvalid,
              "current rejected target is solid red") &&
       ok;

  editor.interaction.placementFeedback.status =
      CreativeEditorPlacementFeedbackStatus::Placed;
  attachCreativeEditorPlacementPreviews(editor, false, frame);
  ok = expect(frame.creativePreview.itemCount == 1U &&
                  frame.creativePreview.items[0].role ==
                      iggy3d::RenderCreativePreviewRole::Held,
              "accepted mutation removes target ghost in the same frame") &&
       ok;

  editor.interaction.placementFeedback = {};
  editor.interaction.hotbar.entries[0].objectKind =
      cr::CreativeObjectKind::Crate;
  attachCreativeEditorPlacementPreviews(editor, false, frame);
  return expect(frame.creativePreview.itemCount == 2U &&
                    frame.creativePreview.items[1].role ==
                        iggy3d::RenderCreativePreviewRole::Held,
                "hotbar material changes rebuild held and target previews") &&
         ok;
}

bool generatedTraversalPreviewsUseRoomGeometryProfiles() {
  const auto previewFor = [](cr::CreativeObjectKind kind) {
    CreativeEditorState editor = materialEditor(kind);
    setPlaceTarget(editor, 1, 0, 1);
    iggy3d::FrameInput frame;
    attachCreativeEditorPlacementPreviews(editor, false, frame);
    return frame.creativePreview;
  };

  const iggy3d::RenderCreativePreviewFrame ramp =
      previewFor(cr::CreativeObjectKind::Ramp);
  const iggy3d::RenderCreativePreviewFrame stair =
      previewFor(cr::CreativeObjectKind::Stair);
  const iggy3d::RenderCreativePreviewFrame platform =
      previewFor(cr::CreativeObjectKind::Platform);
  const iggy3d::RenderCreativePreviewFrame bridge =
      previewFor(cr::CreativeObjectKind::Bridge);
  const iggy3d::RenderCreativePreviewFrame column =
      previewFor(cr::CreativeObjectKind::Column);
  const iggy3d::RenderCreativePreviewFrame arch =
      previewFor(cr::CreativeObjectKind::Arch);

  return expect(ramp.itemCount == 2U &&
                    ramp.items[0].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::RampWedge &&
                    ramp.items[1].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::RampWedge &&
                    ramp.items[0].proceduralSegmentCount == 0U &&
                    ramp.items[1].proceduralSegmentCount == 0U,
                "ramp target and held previews use wedge geometry") &&
         expect(stair.itemCount == 2U &&
                    stair.items[0].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::StairSteps &&
                    stair.items[1].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::StairSteps &&
                    stair.items[0].proceduralSegmentCount == 4U &&
                    stair.items[1].proceduralSegmentCount == 4U,
                "stair target and held previews share bounded step count") &&
         expect(platform.itemCount == 2U &&
                    platform.items[0].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::Box &&
                    platform.items[1].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::Box,
                "platform preview remains an exact box") &&
         expect(bridge.itemCount == 2U &&
                    bridge.items[0].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::Box &&
                    bridge.items[1].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::Box &&
                    column.itemCount == 2U &&
                    column.items[0].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::Box &&
                    column.items[1].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::Box,
                "slabs and solid prisms share exact box previews") &&
         expect(arch.itemCount == 2U &&
                    arch.items[0].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::OpenFrame &&
                    arch.items[1].geometryProfile ==
                        iggy3d::RenderCreativePreviewGeometryProfile::OpenFrame &&
                    arch.items[0].proceduralSegmentCount == 0U &&
                    arch.items[1].proceduralSegmentCount == 0U,
                "arch target and held previews preserve the open frame");
}

bool previewHidesForEveryBlockingSurface() {
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  setPlaceTarget(editor, 0);
  iggy3d::FrameInput frame;
  const auto hidden = [&]() {
    attachCreativeEditorPlacementPreviews(editor, false, frame);
    return frame.creativePreview.itemCount == 0U;
  };

  editor.catalog.model.open = true;
  bool ok = expect(hidden(), "catalog hides placement previews");
  editor.catalog.model.open = false;
  editor.catalog.toolWheel.open = true;
  ok = expect(hidden(), "tool wheel hides placement previews") && ok;
  editor.catalog.toolWheel.open = false;
  editor.toolOptions.open = true;
  ok = expect(hidden(), "tool options hide placement previews") && ok;
  editor.toolOptions.open = false;
  editor.assetReplacement.active = true;
  ok = expect(hidden(), "asset replacement hides placement previews") && ok;
  editor.assetReplacement.active = false;
  editor.transform.active = true;
  ok = expect(hidden(), "transform preview hides placement previews") && ok;
  editor.transform.active = false;
  attachCreativeEditorPlacementPreviews(editor, true, frame);
  ok = expect(frame.creativePreview.itemCount == 0U,
              "capture mode hides placement previews") &&
       ok;
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::ObjectSelect;
  return expect(hidden(), "non-material tool hides placement previews") && ok;
}

bool quickEditHudHighlightsTheActiveSetting() {
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  syncCreativeEditorQuickEdit(editor);

  std::vector<iggy3d::RenderUiRect> rects;
  std::vector<iggy3d::DebugHudGlyphQuad> glyphs;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> wireLines;
  appendCreativeEditorInteractionOverlay(editor, 1280U, 720U, 0.04F, rects,
                                         glyphs, wireLines);

  const auto isNeutralBrushGlyph = [](const iggy3d::DebugHudGlyphQuad& quad) {
    return quad.source == 'B' && near(quad.r, 0.88F) &&
           near(quad.g, 0.90F) && near(quad.b, 0.94F);
  };
  const auto isActiveBrushGlyph = [](const iggy3d::DebugHudGlyphQuad& quad) {
    return quad.source == 'B' && near(quad.r, 0.24F) &&
           near(quad.g, 1.0F) && near(quad.b, 0.34F);
  };
  return expect(std::any_of(glyphs.begin(), glyphs.end(),
                            isNeutralBrushGlyph) &&
                    std::any_of(glyphs.begin(), glyphs.end(),
                                isActiveBrushGlyph),
                "held tool stays neutral while active D-pad setting is green");
}

bool previewsDoNotAffectRoomGeometrySignature() {
  iggy3d::SceneRoomProjection room;
  room.loaded = true;
  room.staticMeshCount = 1U;
  iggy3d::SceneRoomMeshItem mesh;
  mesh.id = "floor";
  mesh.role = "floor";
  mesh.position = {};
  mesh.size = {4.0F, 0.25F, 4.0F};
  room.meshes.push_back(mesh);
  const iggy3d::vulkan::RoomMeshCpuGeometry before =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);

  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  setPlaceTarget(editor, 4);
  iggy3d::FrameInput frame;
  attachCreativeEditorPlacementPreviews(editor, false, frame);
  const iggy3d::vulkan::RoomMeshCpuGeometry after =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  return expect(frame.creativePreview.itemCount == 2U,
                "aiming produced transient preview items") &&
         expect(before.sourceRoomGeometrySignature ==
                    after.sourceRoomGeometrySignature &&
                    before.vertices.size() == after.vertices.size() &&
                    before.indices.size() == after.indices.size(),
                "aiming does not enter room geometry or its signature");
}

bool roomGeometryRendersStoredEulerRadians() {
  iggy3d::SceneRoomProjection room;
  room.loaded = true;
  room.staticMeshCount = 1U;
  iggy3d::SceneRoomMeshItem mesh;
  mesh.id = "rotated_prop";
  mesh.role = "prop";
  mesh.position = {};
  mesh.size = {4.0F, 2.0F, 1.0F};
  room.meshes.push_back(mesh);
  const iggy3d::vulkan::RoomMeshCpuGeometry unrotated =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);

  room.meshes[0].rotationEulerRadians.y = 1.57079632679489662F;
  const iggy3d::vulkan::RoomMeshCpuGeometry rotated =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(room);
  float minX = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float minZ = std::numeric_limits<float>::max();
  float maxZ = std::numeric_limits<float>::lowest();
  for (const iggy3d::vulkan::FirstRoomVertex& vertex : rotated.vertices) {
    minX = std::min(minX, vertex.position[0]);
    maxX = std::max(maxX, vertex.position[0]);
    minZ = std::min(minZ, vertex.position[2]);
    maxZ = std::max(maxZ, vertex.position[2]);
  }
  return expect(unrotated.ready && rotated.ready,
                "room geometry accepts finite stored rotation") &&
         expect(unrotated.sourceRoomGeometrySignature !=
                    rotated.sourceRoomGeometrySignature,
                "room geometry signature includes rotation") &&
         expect(near(maxX - minX, 1.0F) && near(maxZ - minZ, 4.0F),
                "room CPU vertices apply Euler yaw to the box") &&
         expect(rotated.roomFloorDrawCount == 0U &&
                    rotated.roomWallDrawCount == 0U,
                "rotated prop remains on the ordinary box path");
}

bool materialAimMovementDoesNotChangeUploadSignature() {
  cr::CreativeAppState appState;
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput frame;

  setPlaceTarget(editor, 0);
  CreativeEditorOverlayFrame firstOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      firstOverlay);
  const iggy3d::Mat4 firstTarget =
      frame.creativePreview.items[0].clipFromModel;
  iggy3d::RenderCreativeWireframeDebugFrame firstWire;
  firstWire.available = true;
  firstWire.visible = true;
  firstWire.lines = firstOverlay.combinedWireLines.data();
  firstWire.lineCount = firstOverlay.combinedWireLines.size();
  const iggy3d::vulkan::CreativeWireframeDebugCpuGeometry firstGeometry =
      iggy3d::vulkan::buildCreativeWireframeDebugCpuGeometry(&firstWire);

  setPlaceTarget(editor, 7);
  CreativeEditorOverlayFrame secondOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      secondOverlay);
  const iggy3d::Mat4 secondTarget =
      frame.creativePreview.items[0].clipFromModel;
  iggy3d::RenderCreativeWireframeDebugFrame secondWire;
  secondWire.available = true;
  secondWire.visible = true;
  secondWire.lines = secondOverlay.combinedWireLines.data();
  secondWire.lineCount = secondOverlay.combinedWireLines.size();
  const iggy3d::vulkan::CreativeWireframeDebugCpuGeometry secondGeometry =
      iggy3d::vulkan::buildCreativeWireframeDebugCpuGeometry(&secondWire);

  bool ok = expect(!near(iggy3d::at(firstTarget, 0U, 3U),
                         iggy3d::at(secondTarget, 0U, 3U)),
                   "aim movement changes only target preview transform") &&
            expect(firstGeometry.geometrySignature ==
                       secondGeometry.geometrySignature &&
                       firstOverlay.combinedWireLines.size() ==
                           secondOverlay.combinedWireLines.size(),
                   "material aim movement does not change upload signature");

  CreativeEditorState pathEditor =
      materialEditor(cr::CreativeObjectKind::PatrolRoute);
  setPlaceTarget(pathEditor, 0);
  CreativeEditorOverlayFrame firstPathOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, pathEditor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      firstPathOverlay);
  iggy3d::RenderCreativeWireframeDebugFrame firstPathWire;
  firstPathWire.available = true;
  firstPathWire.visible = true;
  firstPathWire.lines = firstPathOverlay.combinedWireLines.data();
  firstPathWire.lineCount = firstPathOverlay.combinedWireLines.size();
  const auto firstPathGeometry =
      iggy3d::vulkan::buildCreativeWireframeDebugCpuGeometry(&firstPathWire);
  const bool persistentPath =
      frame.creativePreview.itemCount == 2U &&
      frame.creativePreview.items[0].includePathWireframe;

  setPlaceTarget(pathEditor, 5);
  CreativeEditorOverlayFrame secondPathOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, pathEditor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      secondPathOverlay);
  iggy3d::RenderCreativeWireframeDebugFrame secondPathWire;
  secondPathWire.available = true;
  secondPathWire.visible = true;
  secondPathWire.lines = secondPathOverlay.combinedWireLines.data();
  secondPathWire.lineCount = secondPathOverlay.combinedWireLines.size();
  const auto secondPathGeometry =
      iggy3d::vulkan::buildCreativeWireframeDebugCpuGeometry(&secondPathWire);
  return expect(persistentPath,
                "path target selects persistent path wireframe geometry") &&
         expect(firstPathGeometry.geometrySignature ==
                    secondPathGeometry.geometrySignature,
                "path aim movement is upload-stable") &&
         ok;
}

bool placementGridDotsRenderOnlyTheActiveDepthLayer() {
  cr::CreativeDocument document = cr::CreativeDocument::create("dot layer");
  static_cast<void>(document.assignId(119U));
  static_cast<void>(document.setGridSettings(
      {{0.0, 0.0, 0.0}, 1.0, {12, 12, 12}}));
  static_cast<void>(document.setWorldBounds(
      {{0.0, 0.0, 0.0}, {12.0, 12.0, 12.0}}));
  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Door);
  editor.toolSettings.placementGridDots =
      cr::CreativePlacementGridDots::NearestLayer;
  editor.toolSettings.placementPlane = cr::CreativePlacementPlane::X;
  editor.toolSettings.placementAnchor = cr::CreativePlacementAnchor::Face;
  editor.toolSettings.placementDepth = cr::CreativePlacementDepth::TwoCells;
  const cr::CreativeDocument& installed = appState.facade.document();
  const cr::CreativePlacementGridFrame placementGrid =
      creativeEditorPlacementGridFrame(installed, editor);
  const cr::CreativeGridTarget gridTarget = targetWithFacts(
      cr::resolveCreativeGridTargetFromHit(
          {4.0, 2.5, 5.5}, {1.0, 0.0, 0.0}, placementGrid,
          {1.0, 0.0, 0.0}),
      cr::CreativePlacementTargetSource::AuthoredObject,
      cr::CreativeObjectKind::Wall, 9004U);
  editor.interaction.target.valid = gridTarget.valid;
  editor.interaction.target.grid = gridTarget;

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput frame;
  frame.camera.worldEye = {0.5F, 2.5F, 5.5F};
  CreativeEditorOverlayFrame visible;
  const std::uint64_t revisionBefore = installed.revision();
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      visible);
  const std::size_t renderedDots = static_cast<std::size_t>(std::count_if(
      visible.combinedWireLines.begin(), visible.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.segmentKind == 5U;
      }));
  const std::size_t renderedAnchorCandidates =
      static_cast<std::size_t>(std::count_if(
          visible.combinedWireLines.begin(), visible.combinedWireLines.end(),
          [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
            return line.segmentKind == 9U;
          }));
  const auto guide = std::find_if(
      visible.combinedWireLines.begin(), visible.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.segmentKind == 6U;
      });
  const auto targetMarker = std::find_if(
      visible.combinedWireLines.begin(), visible.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.segmentKind == 7U;
      });
  const auto anchorGuide = std::find_if(
      visible.combinedWireLines.begin(), visible.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.segmentKind == 8U;
      });
  const auto contactGuide = std::find_if(
      visible.combinedWireLines.begin(), visible.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.segmentKind == 10U;
      });
  const bool oneDepthLayer = std::all_of(
      visible.combinedWireLines.begin(), visible.combinedWireLines.end(),
      [&gridTarget](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.segmentKind != 5U ||
               near((line.start.x + line.end.x) * 0.5F,
                    static_cast<float>(gridTarget.basePlacementAnchor.x));
      });

  bool ok = expect(placementGrid.depthOffsetSteps == 2U &&
                       placementGrid.depthAxisLock ==
                           cr::kCreativePlacementGridAxisX &&
                       gridTarget.valid &&
                       gridTarget.adjacentCell ==
                           cr::CreativeGridCoord3{6, 2, 5} &&
                       gridTarget.anchorKind ==
                           cr::CreativePlacementAnchorKind::FaceCenter &&
                       gridTarget.anchorIndex == 0U &&
                       gridTarget.anchorSnapped &&
                       sameVec3(gridTarget.basePlacementAnchor,
                                {6.5, 2.0, 5.5}) &&
                       sameVec3(gridTarget.placementAnchor,
                                {6.0, 2.5, 5.5}) &&
                       visible.placementGridLineCount > 0U &&
                       visible.placementGridDotCount == 144U &&
                       visible.placementGridGuideLineCount == 1U &&
                       visible.placementGridAnchorGuideLineCount == 1U &&
                       visible.placementGridAnchorCandidateLineCount == 15U &&
                       visible.placementGridContactGuideLineCount == 1U &&
                       visible.placementGridTargetMarkerCount == 1U &&
                       renderedDots == visible.placementGridDotCount &&
                       renderedAnchorCandidates ==
                           visible.placementGridAnchorCandidateLineCount &&
                       guide != visible.combinedWireLines.end() &&
                       near(guide->start.x, 4.5F) &&
                       near(guide->end.x, 6.5F) &&
                       anchorGuide != visible.combinedWireLines.end() &&
                       near(anchorGuide->start.x, 6.5F) &&
                       near(anchorGuide->start.y, 2.0F) &&
                       near(anchorGuide->end.x, 6.0F) &&
                       near(anchorGuide->end.y, 2.5F) &&
                       contactGuide != visible.combinedWireLines.end() &&
                       near(contactGuide->start.x, 6.0F) &&
                       contactGuide->end.x > contactGuide->start.x &&
                       near(contactGuide->end.y, contactGuide->start.y) &&
                       targetMarker != visible.combinedWireLines.end() &&
                       near((targetMarker->start.x + targetMarker->end.x) *
                                0.5F,
                            6.0F) &&
                       near((targetMarker->start.y + targetMarker->end.y) *
                                0.5F,
                            2.5F) &&
                       near(targetMarker->color.g, 1.0F) &&
                       near(targetMarker->color.r, 0.18F) &&
                       oneDepthLayer &&
                       appState.facade.document().revision() == revisionBefore,
                   "nearest dots render one shifted layer without mutation");

  editor.toolSettings.placementGridDots = cr::CreativePlacementGridDots::Off;
  CreativeEditorOverlayFrame hiddenByToggle;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      hiddenByToggle);
  ok = expect(hiddenByToggle.placementGridLineCount > 0U &&
                  hiddenByToggle.placementGridDotCount == 0U &&
                  hiddenByToggle.placementGridGuideLineCount == 1U &&
                  hiddenByToggle.placementGridAnchorGuideLineCount == 1U &&
                  hiddenByToggle.placementGridAnchorCandidateLineCount == 15U &&
                  hiddenByToggle.placementGridContactGuideLineCount == 1U &&
                  hiddenByToggle.placementGridTargetMarkerCount == 1U,
              "dot toggle hides dots but retains target anchor cues") &&
       ok;

  editor.toolSettings.placementGridDots =
      cr::CreativePlacementGridDots::NearestLayer;
  editor.toolOptions.open = true;
  CreativeEditorOverlayFrame hiddenByModal;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      hiddenByModal);
  ok = expect(hiddenByModal.placementGridLineCount == 0U &&
                  hiddenByModal.placementGridDotCount == 0U &&
                  hiddenByModal.placementGridGuideLineCount == 0U &&
                  hiddenByModal.placementGridAnchorGuideLineCount == 0U &&
                  hiddenByModal.placementGridAnchorCandidateLineCount == 0U &&
                  hiddenByModal.placementGridContactGuideLineCount == 0U &&
                  hiddenByModal.placementGridTargetMarkerCount == 0U,
              "modal surfaces hide every placement lattice cue") &&
       ok;

  editor.toolOptions.open = false;
  const cr::CreativeGridTarget edgeTarget =
      cr::resolveCreativeGridTargetFromHit(
          {12.0, 2.5, 5.5}, {1.0, 0.0, 0.0}, placementGrid,
          {1.0, 0.0, 0.0});
  editor.interaction.target.valid = edgeTarget.valid;
  editor.interaction.target.grid = edgeTarget;
  CreativeEditorOverlayFrame rejected;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      rejected);
  const auto rejectedMarker = std::find_if(
      rejected.combinedWireLines.begin(), rejected.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.segmentKind == 7U;
      });
  return expect(edgeTarget.valid && !edgeTarget.adjacentInBounds &&
                    rejected.placementGridDotCount == 0U &&
                    rejected.placementGridTargetMarkerCount == 1U &&
                    rejectedMarker != rejected.combinedWireLines.end() &&
                    near(rejectedMarker->color.r, 1.0F) &&
                    near(rejectedMarker->color.g, 0.18F),
                "out-of-bounds placement keeps a red active target marker") &&
         ok;
}

bool lockedPlacementPlaneFeedsPreviewAdmissionAndMutation() {
  cr::CreativeDocument document = cr::CreativeDocument::create("locked plane");
  static_cast<void>(document.assignId(120U));
  static_cast<void>(document.setGridSettings(
      {{0.0, 0.0, 0.0}, 1.0, {8, 8, 8}}));
  static_cast<void>(document.setWorldBounds(
      {{0.0, 0.0, 0.0}, {8.0, 8.0, 8.0}}));
  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  const cr::CreativePlacementGridFrame initialAutoGrid =
      creativeEditorPlacementGridFrame(appState.facade.document(), editor);
  editor.toolSettings.placementPlane = cr::CreativePlacementPlane::Z;
  editor.toolSettings.placementAnchor = cr::CreativePlacementAnchor::Corner;
  editor.toolSettings.placementDepth = cr::CreativePlacementDepth::TwoCells;
  const cr::CreativePlacementGridFrame placementGrid =
      creativeEditorPlacementGridFrame(appState.facade.document(), editor);
  const cr::CreativeGridTarget target = targetWithFacts(
      cr::resolveCreativeGridTargetFromHit(
          {4.0, 2.5, 5.5}, {1.0, 0.0, 0.0}, placementGrid,
          {1.0, 0.0, -0.1}),
      cr::CreativePlacementTargetSource::EmptyPlane);
  editor.interaction.target.valid = target.valid;
  editor.interaction.target.grid = target;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const CreativeBrushPlacementAdmission admission = admitBrushPlacement(
      held, target, editor.toolSettings.placementYaw);

  iggy3d::FrameInput preview;
  attachCreativeEditorPlacementPreviews(
      editor, false, preview, &appState.facade.document());
  const CreativeBrushPlacementMutationReceipt mutation = applyBrushPlacement(
      appState.facade, admission.plan, 1U);

  return expect(sameVec3(initialAutoGrid.previousDepthAxis, {}),
                "unresolved targets do not bias the first automatic plane") &&
         expect(placementGrid.depthAxisLock ==
                    cr::kCreativePlacementGridAxisZ &&
                    placementGrid.anchorKind ==
                        cr::CreativePlacementAnchorKind::BaseCenter &&
                    sameVec3(target.viewDepthAxis, {0.0, 0.0, -1.0}) &&
                    !target.anchorSnapped &&
                    target.surfaceAdjacentCell ==
                        cr::CreativeGridCoord3{4, 2, 5} &&
                    target.adjacentCell == cr::CreativeGridCoord3{4, 2, 3},
                "tool setting resolves one locked placement plane") &&
         expect(admission.allowed && admission.plan.hasVoxelCell &&
                    admission.plan.voxelCell == target.adjacentCell &&
                    sameBounds(admission.plan.previewBounds,
                               target.adjacentCellBounds),
                "admission consumes the locked target without recomputing it") &&
         expect(preview.creativePreview.itemCount == 2U &&
                    preview.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::PlacementValid &&
                    near(preview.creativePreview.items[0].clipFromModel.m[3],
                         4.5F) &&
                    near(preview.creativePreview.items[0].clipFromModel.m[11],
                         3.5F),
                "preview renders the same locked target cell") &&
         expect(mutation.accepted && mutation.voxelCreated &&
                    mutation.voxelCell == target.adjacentCell &&
                    appState.facade.document().voxelField().materialAt(
                        target.adjacentCell) == cr::CreativeObjectKind::Wall,
                "mutation writes the exact locked preview cell");
}

bool authoredAnchorFeedsPreviewAdmissionAndMutation() {
  cr::CreativeDocument document = cr::CreativeDocument::create("anchor parity");
  static_cast<void>(document.assignId(121U));
  static_cast<void>(document.setGridSettings(
      {{0.0, 0.0, 0.0}, 1.0, {8, 8, 8}}));
  static_cast<void>(document.setWorldBounds(
      {{0.0, 0.0, 0.0}, {8.0, 8.0, 8.0}}));
  cr::CreativeAppState appState;
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Door);
  editor.toolSettings.placementAnchor = cr::CreativePlacementAnchor::Corner;
  const cr::CreativePlacementGridFrame placementGrid =
      creativeEditorPlacementGridFrame(appState.facade.document(), editor);
  const cr::CreativeGridTarget target = targetWithFacts(
      cr::resolveCreativeGridTargetFromHit(
          {4.0, 2.9, 5.9}, {1.0, 0.0, 0.0}, placementGrid,
          {1.0, 0.0, 0.0}),
      cr::CreativePlacementTargetSource::AuthoredObject,
      cr::CreativeObjectKind::Wall, 9005U);
  editor.interaction.target.valid = target.valid;
  editor.interaction.target.grid = target;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const CreativeBrushPlacementAdmission admission = admitBrushPlacement(
      held, target, editor.toolSettings.placementYaw);

  iggy3d::FrameInput preview;
  attachCreativeEditorPlacementPreviews(
      editor, false, preview, &appState.facade.document());
  const CreativeBrushPlacementMutationReceipt mutation = applyBrushPlacement(
      appState.facade, admission.plan, 1U);
  const cr::CreativeObject* object =
      appState.facade.findObject(mutation.objectId);

  return expect(placementGrid.anchorKind ==
                    cr::CreativePlacementAnchorKind::Corner &&
                    target.valid && target.anchorSnapped &&
                    target.anchorIndex == 3U &&
                    sameVec3(target.basePlacementAnchor, {4.5, 2.0, 5.5}) &&
                    sameVec3(target.placementAnchor, {4.0, 3.0, 6.0}),
                "corner mode resolves one stable authored target anchor") &&
         expect(admission.allowed &&
                    admission.plan.contact.valid &&
                    sameVec3(admission.plan.contact.targetPoint,
                             target.placementAnchor) &&
                    nearVec3(
                        admission.plan.contact.sourcePointAfterTranslation,
                        target.placementAnchor) &&
                    admission.plan.contact.minimumSignedDistanceMeters >=
                        -1.0e-9,
                "admission aligns its extreme source feature to the anchor") &&
         expect(preview.creativePreview.itemCount == 2U &&
                    preview.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::PlacementValid &&
                    near(preview.creativePreview.items[0].clipFromModel.m[3],
                         static_cast<float>(admission.plan.transform.position.x)) &&
                    near(preview.creativePreview.items[0].clipFromModel.m[7],
                         static_cast<float>(admission.plan.transform.position.y)) &&
                    near(preview.creativePreview.items[0].clipFromModel.m[11],
                         static_cast<float>(admission.plan.transform.position.z)),
                "preview renders the admitted anchored transform") &&
         expect(mutation.accepted && mutation.objectCreated &&
                    object != nullptr &&
                    sameVec3(object->transform.position,
                             admission.plan.transform.position) &&
                    sameBounds(object->bounds, admission.plan.authoredBounds),
                "mutation commits the exact anchored preview plan");
}

bool shapeVolumePreviewsStayBoundedAndFailClosed() {
  cr::CreativeAppState appState;
  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput frame;

  const auto build = [&](cr::CreativeShapeBrushKind kind,
                         cr::CreativeShapeBrushAxis axis) {
    CreativeEditorState editor;
    editor.volume.active = true;
    editor.volume.operation = cr::CreativeVolumeOperationKind::Fill;
    editor.toolSettings.shapeBrushKind = kind;
    editor.toolSettings.shapeBrushAxis = axis;
    static_cast<void>(cr::setCreativeVolumeSelectionCorner(
        editor.volume.selection, cr::CreativeVolumeCorner::First, {0, 0, 0}));
    static_cast<void>(cr::setCreativeVolumeSelectionCorner(
        editor.volume.selection, cr::CreativeVolumeCorner::Second, {4, 2, 4}));
    CreativeEditorOverlayFrame overlay;
    buildAndAttachCreativeEditorOverlayFrame(
        {appState, editor, selection, gizmo, frame, projectionRequest,
         1280U, 720U, 0.03F, false},
        overlay);
    return overlay;
  };

  const CreativeEditorOverlayFrame box =
      build(cr::CreativeShapeBrushKind::Box,
            cr::CreativeShapeBrushAxis::Y);
  const CreativeEditorOverlayFrame ellipsoid =
      build(cr::CreativeShapeBrushKind::Ellipsoid,
            cr::CreativeShapeBrushAxis::Y);
  const CreativeEditorOverlayFrame cylinder =
      build(cr::CreativeShapeBrushKind::Cylinder,
            cr::CreativeShapeBrushAxis::Y);
  const CreativeEditorOverlayFrame line =
      build(cr::CreativeShapeBrushKind::Line,
            cr::CreativeShapeBrushAxis::Y);
  const CreativeEditorOverlayFrame invalid = build(
      static_cast<cr::CreativeShapeBrushKind>(255U),
      cr::CreativeShapeBrushAxis::Y);
  const std::size_t invalidStart =
      invalid.combinedWireLines.size() - invalid.volumeEdgeCount;

  CreativeEditorState oversizedEditor;
  oversizedEditor.volume.active = true;
  oversizedEditor.volume.operation = cr::CreativeVolumeOperationKind::Fill;
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      oversizedEditor.volume.selection, cr::CreativeVolumeCorner::First,
      {0, 0, 0}));
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      oversizedEditor.volume.selection, cr::CreativeVolumeCorner::Second,
      {8, 8, 8}));
  CreativeEditorOverlayFrame oversized;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, oversizedEditor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      oversized);
  const std::size_t oversizedStart =
      oversized.combinedWireLines.size() - oversized.volumeEdgeCount;

  return expect(box.volumeEdgeCount == 12U,
                "box volume preview remains one bounded envelope") &&
         expect(ellipsoid.volumeEdgeCount == 144U,
                "ellipsoid preview uses three fixed loops") &&
         expect(cylinder.volumeEdgeCount == 100U,
                "cylinder preview uses two loops and four rails") &&
         expect(line.volumeEdgeCount == 25U,
                "line preview uses centerline and endpoint cells") &&
         expect(invalid.volumeEdgeCount == 12U &&
                    invalidStart < invalid.combinedWireLines.size() &&
                    invalid.combinedWireLines[invalidStart].color.r == 1.0F &&
                    invalid.combinedWireLines[invalidStart].color.g == 0.15F,
                "invalid shape renders a bounded red rejection envelope") &&
         expect(oversized.volumeEdgeCount == 12U &&
                    oversizedStart < oversized.combinedWireLines.size() &&
                    oversized.combinedWireLines[oversizedStart].color.r ==
                        1.0F &&
                    oversized.combinedWireLines[oversizedStart].color.g ==
                        0.15F,
                "over-budget shape turns red before commit");
}

bool editorVolumeBudgetRejectsBeforeMutation() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 104U);
  CreativeEditorVolumeState volume;
  activateCreativeEditorVolumeMode(volume, 1.0);
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      volume.selection, cr::CreativeVolumeCorner::First, {0, 0, 0}));
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      volume.selection, cr::CreativeVolumeCorner::Second, {8, 8, 8}));
  const cr::CreativeToolSettings settings =
      cr::makeDefaultCreativeToolSettings();
  const cr::CreativeVolumeOperationReceipt receipt =
      applyCreativeEditorVolumeOperationWithHistory(
          appState, volume, cr::CreativeObjectKind::Wall,
          cr::CreativeVolumeOperationKind::Fill, settings,
          "test_editor_volume_budget");
  return expect(kCreativeEditorVolumeCellLimit == 512U,
                "interactive volume budget is explicit") &&
         expect(!receipt.accepted && !receipt.changed &&
                    receipt.status ==
                        cr::CreativeVolumeOperationStatus::OperationLimitExceeded,
                "729-cell editor fill is rejected") &&
         expect(appState.facade.document().objectCount() == 0U &&
                    appState.facade.document().revision() == 0U,
                "over-budget fill leaves document unchanged") &&
         expect(cr::creativeUndoDepth(appState.history) == 0U,
                "over-budget fill creates no history record");
}

bool sceneCacheRefreshesOnlyOnDocumentRevision() {
  cr::CreativeDocument document;
  CreativeEditorSceneCache cache;
  bool ok = expect(refreshCreativeEditorSceneCache(cache, document),
                   "first scene cache access builds") &&
            expect(cache.refreshCount == 1U &&
                       cache.placementClearance.rebuildCount == 1U,
                   "scene and placement caches build once");
  for (std::size_t frame = 0; frame < 300U; ++frame) {
    ok = expect(!refreshCreativeEditorSceneCache(cache, document),
                "idle frame reuses scene cache") &&
         ok;
  }
  ok = expect(cache.placementClearance.rebuildCount == 1U,
              "300 idle frames do not rebuild placement broadphase") &&
       ok;

  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  iggy3d::FrameInput previewFrame;
  for (std::int32_t aim = 0; aim < 8; ++aim) {
    setPlaceTarget(editor, aim);
    attachCreativeEditorPlacementPreviews(editor, false, previewFrame);
    ok = expect(!refreshCreativeEditorSceneCache(cache, document),
                "aim movement does not refresh scene cache") &&
         ok;
  }

  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Crate;
  const cr::CreativeDocumentCreateReceipt first = document.createObject(create);
  ok = expect(first.accepted && first.changed &&
                  refreshCreativeEditorSceneCache(cache, document) &&
                  cache.refreshCount == 2U &&
                  cache.placementClearance.rebuildCount == 2U,
              "accepted mutation refreshes both caches once") &&
       ok;
  ok = expect(!refreshCreativeEditorSceneCache(cache, document) &&
                  cache.refreshCount == 2U,
              "post-mutation idle frame reuses refreshed cache") &&
       ok;
  const cr::CreativeDocumentCreateReceipt second = document.createObject(create);
  ok = expect(second.accepted && second.changed &&
                  refreshCreativeEditorSceneCache(cache, document) &&
                  cache.refreshCount == 3U &&
                  cache.placementClearance.rebuildCount == 3U,
              "each later accepted mutation refreshes exactly once") &&
       ok;

  std::array<cr::CreativeVoxelEdit, 8> voxelEdits{};
  std::size_t editIndex = 0;
  for (std::int32_t z = 0; z < 2; ++z) {
    for (std::int32_t y = 0; y < 2; ++y) {
      for (std::int32_t x = 0; x < 2; ++x) {
        voxelEdits[editIndex++] =
            {{x, y, z}, cr::CreativeObjectKind::Wall};
      }
    }
  }
  const cr::CreativeVoxelMutationReceipt voxelReceipt =
      document.applyVoxelEdits(voxelEdits);
  ok = expect(voxelReceipt.accepted && voxelReceipt.changed &&
                  refreshCreativeEditorSceneCache(cache, document) &&
                  cache.refreshCount == 4U &&
                  cache.voxelChunkMeshBuildCount == 1U,
              "voxel batch refreshes scene cache once") &&
       expect(cache.preview.roomBake.receipt.voxelCellCount == 8U &&
                  cache.preview.roomBake.receipt.voxelChunkCount == 1U &&
                  cache.preview.roomBake.receipt.bakedVoxelCuboidCount == 1U,
              "scene cache greedily bakes one voxel cuboid") &&
       expect(!refreshCreativeEditorSceneCache(cache, document) &&
                  cache.refreshCount == 4U &&
                  cache.voxelChunkMeshBuildCount == 1U,
              "post-voxel idle frame reuses scene cache") &&
       ok;

  const cr::CreativeVoxelEdit secondChunkEdit{
      {16, 0, 0}, cr::CreativeObjectKind::Floor};
  const cr::CreativeVoxelMutationReceipt secondChunkReceipt =
      document.applyVoxelEdits(std::span{&secondChunkEdit, 1U});
  return expect(secondChunkReceipt.changed &&
                    refreshCreativeEditorSceneCache(cache, document) &&
                    cache.voxelChunkMeshBuildCount == 2U &&
                    cache.preview.roomBake.receipt.voxelChunkCount == 2U,
                "new chunk rebuilds only its greedy plan") &&
         ok;
}

bool activeVolumeSelectionRebindsToLoadedDocumentGrid() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 106U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::VolumeFill;
  syncCreativeEditorHeldItem(appState, editor);
  static_cast<void>(cr::setCreativeVolumeSelectionCorner(
      editor.volume.selection, cr::CreativeVolumeCorner::First, {3, 1, 2}));

  cr::CreativeDocument replacement = cr::CreativeDocument::create("new grid");
  static_cast<void>(replacement.assignId(107U));
  cr::CreativeGridSettings grid;
  grid.origin = {4.0, -2.0, 8.0};
  grid.cellSizeMeters = 2.0;
  static_cast<void>(replacement.setGridSettings(grid));
  const cr::CreativeFacadeDocumentInstallReceipt install =
      appState.facade.installDocument(std::move(replacement));
  editor.interaction.synchronizedHeldItemKind =
      cr::CreativeHeldItemKind::Count;

  cr::CreativeWorldActionFrame actions;
  iggy3d::RenderCameraFrame camera;
  CreativeEditorPickFrame pickFrame;
  processCreativeEditorWorldInteractionFrame(
      {appState, editor, actions, cr::kCreativeInputModifierNone, camera,
       pickFrame, iggy3d::RenderContentViewport{0, 0, 800U, 600U}, 0U, false});
  return expect(install.accepted && install.changed,
                "replacement document installs") &&
         expect(editor.interaction.synchronizedHeldItemKind ==
                    cr::CreativeHeldItemKind::VolumeFill,
                "first replacement frame reconciles the held-item lifecycle") &&
         expect(editor.volume.selection.cellSize == 2.0 &&
                    sameVec3(editor.volume.selection.origin, grid.origin),
                "active volume selection adopts loaded document grid") &&
         expect(editor.volume.selection.phase ==
                    cr::CreativeVolumeSelectionPhase::Empty,
                "grid replacement clears incompatible volume corners");
}

bool objectBoundPlacementAnchorsFollowRotatedPickGeometry() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("object anchor target");
  static_cast<void>(document.assignId(122U));
  static_cast<void>(document.setGridSettings(
      {{0.0, 0.0, 0.0}, 1.0, {8, 8, 8}}));
  static_cast<void>(document.setWorldBounds(
      {{0.0, 0.0, 0.0}, {8.0, 8.0, 8.0}}));

  CreativeBrushPlacementPlan receiver =
      planBrushPlacement(cr::CreativeObjectKind::Door,
                         {3.25F, 0.0F, 2.75F});
  constexpr double kHalfPi = 1.57079632679489662;
  receiver.transform.rotationEulerRadians.y = kHalfPi;
  receiver.orientationResolved = true;
  const cr::CreativeDocumentCreateReceipt created = document.createObject(
      buildBrushCreateRequest(receiver, 1U));
  const cr::CreativeObject* object = document.findObject(created.objectId);
  if (!expect(receiver.valid && created.accepted && object != nullptr,
              "rotated object anchor fixture is valid")) {
    return false;
  }

  CreativeEditorPickFrame pickFrame;
  pickFrame.objectPickCandidates.push_back(buildObjectVisualPickBounds(
      *object, iggy3d::identityMat4(), 800U, 600U));
  const ObjectVisualPickBounds& candidate =
      pickFrame.objectPickCandidates.front();
  if (!expect(candidate.orientedBounds.has_value(),
              "rotated object exposes exact oriented pick bounds")) {
    return false;
  }

  const cr::CreativeTransformedBounds worldBounds =
      cr::resolveCreativeObjectBounds(*object);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Door);
  editor.placeCellSize = 0.25;
  editor.toolSettings.placementAnchor = cr::CreativePlacementAnchor::Face;
  const cr::CreativePlacementGridFrame placementGrid =
      creativeEditorPlacementGridFrame(document, editor);
  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {0.0F, static_cast<float>(worldBounds.center.y),
                     static_cast<float>(worldBounds.center.z)};
  camera.worldForward = {1.0F, 0.0F, 0.0F};
  camera.worldUp = {0.0F, 1.0F, 0.0F};
  const CreativeEditorWorldTarget target = resolveCreativeEditorWorldTarget(
      document, camera, pickFrame,
      iggy3d::RenderContentViewport{0, 0, 800U, 600U}, placementGrid);
  const CreativeBrushPlacementAdmission admission = admitBrushPlacement(
      cr::CreativeObjectKind::Door, target.grid);
  const cr::CreativeTransformedBounds admittedBounds =
      cr::resolveCreativeTransformedBounds(
          admission.plan.authoredBounds, admission.plan.transform);

  camera.worldEye.z = static_cast<float>(worldBounds.center.z + 0.26);
  const CreativeEditorWorldTarget retainedTarget =
      resolveCreativeEditorWorldTarget(
          document, camera, pickFrame,
          iggy3d::RenderContentViewport{0, 0, 800U, 600U}, placementGrid,
          &target);
  camera.worldEye.z = static_cast<float>(worldBounds.center.z + 0.37);
  const CreativeEditorWorldTarget switchedTarget =
      resolveCreativeEditorWorldTarget(
          document, camera, pickFrame,
          iggy3d::RenderContentViewport{0, 0, 800U, 600U}, placementGrid,
          &target);

  editor.toolSettings.placementAnchor = cr::CreativePlacementAnchor::Center;
  const cr::CreativePlacementGridFrame centerGrid =
      creativeEditorPlacementGridFrame(document, editor);
  const CreativeEditorWorldTarget centerTarget =
      resolveCreativeEditorWorldTarget(
          document, camera, pickFrame,
          iggy3d::RenderContentViewport{0, 0, 800U, 600U}, centerGrid);
  const CreativeBrushPlacementAdmission centerAdmission =
      admitBrushPlacement(cr::CreativeObjectKind::Door, centerTarget.grid);

  cr::CreativeHotbarEntry assetHeld{cr::CreativeHeldItemKind::Material,
                                    cr::CreativeObjectKind::Door};
  const cr::CreativeBounds assetSourceBounds{{-1.5, -0.4, -0.3},
                                             {0.5, 1.6, 0.3}};
  const bool assetAssigned = cr::setCreativeHotbarAsset(
      assetHeld, "fixture/contact-door", assetSourceBounds);
  const CreativeBrushPlacementAdmission assetAdmission =
      admitBrushPlacement(assetHeld, target.grid);
  const cr::CreativeTransformedBounds assetBounds =
      cr::resolveCreativeTransformedBounds(
          assetAdmission.plan.authoredBounds,
          assetAdmission.plan.transform);
  CreativeBrushPlacementPlan socketOverridePlan = assetAdmission.plan;
  cr::CreativeTransform socketTransform;
  socketTransform.position = {7.0, 2.0, 1.0};
  const bool socketOverrideApplied = applyCreativeAssetPlacementTransform(
      socketOverridePlan, assetSourceBounds, socketTransform);

  return expect(worldBounds.valid && target.valid && target.objectHit &&
                    target.objectId == created.objectId &&
                    target.grid.targetFacts.valid &&
                    target.grid.targetFacts.source ==
                        cr::CreativePlacementTargetSource::AuthoredObject &&
                    target.grid.targetFacts.hostKind ==
                        cr::CreativeObjectKind::Door &&
                    target.grid.targetFacts.hostObjectId == created.objectId &&
                    target.grid.anchorFromObjectBounds &&
                    target.grid.anchorCandidates.valid &&
                    target.grid.anchorCandidates.count == 6U &&
                    target.grid.anchorIndex == 4U &&
                    near(static_cast<float>(target.grid.placementNormal.x),
                         -1.0F) &&
                    near(static_cast<float>(target.grid.placementNormal.y),
                         0.0F) &&
                    near(static_cast<float>(target.grid.placementNormal.z),
                         0.0F) &&
                    near(static_cast<float>(target.grid.placementAnchor.x),
                         3.15F) &&
                    near(static_cast<float>(target.grid.placementAnchor.y),
                         static_cast<float>(worldBounds.center.y)) &&
                    near(static_cast<float>(target.grid.placementAnchor.z),
                         static_cast<float>(worldBounds.center.z)) &&
                    !sameVec3(target.grid.placementAnchor,
                              target.grid.basePlacementAnchor),
                "face mode snaps to the rotated object's exact hit face") &&
         expect(admission.allowed &&
                    admission.plan.contact.valid && admittedBounds.valid &&
                    sameVec3(admission.plan.contact.targetPoint,
                             target.grid.placementAnchor) &&
                    nearVec3(
                        admission.plan.contact.sourcePointAfterTranslation,
                        target.grid.placementAnchor) &&
                    admission.plan.contact.sourceFeatureVertexCount == 4U &&
                    admission.plan.contact.minimumSignedDistanceMeters >=
                        -1.0e-9 &&
                    near(static_cast<float>(admittedBounds.worldBounds.max.x),
                         static_cast<float>(target.grid.placementAnchor.x)),
                "object-bound contact places the source outside the receiver") &&
         expect(retainedTarget.valid &&
                    retainedTarget.grid.anchorFromObjectBounds &&
                    retainedTarget.grid.anchorIndex == 4U &&
                    switchedTarget.valid &&
                    switchedTarget.grid.anchorIndex == 0U,
                "object-anchor hysteresis retains near an edge and releases after the margin") &&
         expect(centerTarget.valid && centerTarget.objectHit &&
                    !centerTarget.grid.anchorFromObjectBounds &&
                    centerTarget.grid.anchorCandidates.count == 1U &&
                    centerAdmission.allowed &&
                    !centerAdmission.plan.contact.valid &&
                    sameVec3(centerTarget.grid.placementAnchor,
                             centerTarget.grid.basePlacementAnchor),
                "center mode preserves historical grid-cell placement") &&
         expect(assetAssigned && assetAdmission.allowed &&
                    assetAdmission.plan.contact.valid && assetBounds.valid &&
                    near(static_cast<float>(assetBounds.worldBounds.max.x),
                         static_cast<float>(target.grid.placementAnchor.x)),
                "asset-backed contact uses the imported source bounds") &&
         expect(socketOverrideApplied && !socketOverridePlan.contact.valid &&
                    sameVec3(socketOverridePlan.transform.position,
                             socketTransform.position),
                "explicit socket transforms remain stronger than contact snapping");
}

bool worldTargetPicksVoxelBeforeGround() {
  cr::CreativeDocument document = cr::CreativeDocument::create("pick voxel");
  static_cast<void>(document.assignId(106U));
  const cr::CreativeVoxelEdit edit{{2, 0, 0}, cr::CreativeObjectKind::Crate};
  static_cast<void>(document.applyVoxelEdits(std::span{&edit, 1U}));

  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {-2.0F, 0.5F, 0.5F};
  camera.worldForward = {1.0F, 0.0F, 0.0F};
  camera.worldUp = {0.0F, 1.0F, 0.0F};
  const CreativeEditorWorldTarget target = resolveCreativeEditorWorldTarget(
      document, camera, CreativeEditorPickFrame{},
      iggy3d::RenderContentViewport{0, 0, 800U, 600U}, 1.0);
  return expect(target.valid && target.voxelHit && !target.objectHit,
                "world target reports voxel hit") &&
         expect(target.voxelCell.x == 2 && target.voxelCell.y == 0 &&
                    target.voxelCell.z == 0,
                "world target preserves hit cell") &&
         expect(target.objectKind == cr::CreativeObjectKind::Crate,
                "world target exposes voxel material") &&
         expect(target.grid.targetFacts.valid &&
                    target.grid.targetFacts.source ==
                        cr::CreativePlacementTargetSource::Voxel &&
                    target.grid.targetFacts.hostKind ==
                        cr::CreativeObjectKind::Crate &&
                    target.grid.targetFacts.hostObjectId == 0U,
                "voxel target carries semantic host provenance") &&
         expect(target.grid.targetCell.x == 2 &&
                    target.grid.adjacentCell.x == 1,
                "world target derives aimed and adjacent cells from face");
}

bool worldTargetMarksEmptyPlaneProvenance() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("pick empty plane");
  static_cast<void>(document.assignId(124U));

  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {0.5F, 10.0F, 0.5F};
  camera.worldForward = {0.0F, -1.0F, 0.0F};
  camera.worldUp = {0.0F, 0.0F, -1.0F};
  const CreativeEditorWorldTarget target = resolveCreativeEditorWorldTarget(
      document, camera, CreativeEditorPickFrame{},
      iggy3d::RenderContentViewport{0, 0, 800U, 600U}, 1.0);

  return expect(target.valid && !target.voxelHit && !target.objectHit &&
                    !target.terrainHit,
                "empty scene resolves the active placement plane") &&
         expect(target.grid.targetFacts.valid &&
                    target.grid.targetFacts.source ==
                        cr::CreativePlacementTargetSource::EmptyPlane &&
                    target.grid.targetFacts.hostKind ==
                        cr::CreativeObjectKind::Unknown &&
                    target.grid.targetFacts.hostObjectId == 0U,
                "empty placement plane is explicit target provenance");
}

bool worldTargetSeparatesVoxelStorageFromAuthoredSnap() {
  cr::CreativeDocument document = cr::CreativeDocument::create("mixed grid");
  static_cast<void>(document.assignId(109U));
  static_cast<void>(document.setGridSettings(
      {{0.0, 0.0, 0.0}, 1.0, {8, 4, 8}}));
  static_cast<void>(document.setWorldBounds(
      {{0.0, 0.0, 0.0}, {8.0, 4.0, 8.0}}));
  const cr::CreativeVoxelEdit edit{{2, 0, 0}, cr::CreativeObjectKind::Crate};
  static_cast<void>(document.applyVoxelEdits(std::span{&edit, 1U}));

  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Door);
  editor.placeCellSize = 0.25;
  const cr::CreativePlacementGridFrame placementGrid =
      creativeEditorPlacementGridFrame(document, editor);
  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {-2.0F, 0.5F, 0.5F};
  camera.worldForward = {1.0F, 0.0F, 0.0F};
  camera.worldUp = {0.0F, 1.0F, 0.0F};
  const CreativeEditorWorldTarget target = resolveCreativeEditorWorldTarget(
      document, camera, CreativeEditorPickFrame{},
      iggy3d::RenderContentViewport{0, 0, 800U, 600U}, placementGrid);

  bool ok = expect(placementGrid.valid && !placementGrid.storageAligned &&
                       placementGrid.storageCellSizeMeters == 1.0 &&
                       placementGrid.stepMeters.x == 0.25,
                   "authored snap keeps the document storage pitch") &&
            expect(target.valid && target.voxelHit &&
                       target.voxelCell == edit.cell &&
                       near(target.distanceMeters, 4.0F) &&
                       target.grid.targetCell.x == 8 &&
                       target.grid.adjacentCell.x == 7,
                   "voxel pick uses one meter storage before quarter snap");

  const cr::CreativeGridTarget edge = cr::resolveCreativeGridTargetFromHit(
      {8.0, 0.5, 0.5}, {1.0, 0.0, 0.0}, placementGrid);
  const CreativeBrushPlacementAdmission edgeAdmission =
      admitBrushPlacement(cr::CreativeObjectKind::Door, edge);
  editor.interaction.target = {};
  editor.interaction.target.valid = edge.valid;
  editor.interaction.target.grid = edge;
  iggy3d::FrameInput previewFrame;
  attachCreativeEditorPlacementPreviews(editor, false, previewFrame,
                                        &document);
  return expect(edge.valid && !edge.adjacentInBounds &&
                    !edgeAdmission.allowed &&
                    previewFrame.creativePreview.itemCount == 2U &&
                    previewFrame.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::PlacementInvalid,
                "outward edge placement shows red and cannot mutate") &&
         ok;
}

bool worldTargetPicksDerivedTerrainAndPreservesVoxelTiePriority() {
  cr::CreativeDocument document = cr::CreativeDocument::create("pick terrain");
  static_cast<void>(document.assignId(108U));
  const cr::CreativeTerrainControlEdit terrainEdit{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 1U}};
  static_cast<void>(document.applyTerrainControlEdits(
      std::span{&terrainEdit, 1U}));

  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {0.5F, 10.0F, 0.5F};
  camera.worldForward = {0.0F, -1.0F, 0.0F};
  camera.worldUp = {0.0F, 0.0F, -1.0F};
  const CreativeEditorWorldTarget terrain = resolveCreativeEditorWorldTarget(
      document, camera, CreativeEditorPickFrame{},
      iggy3d::RenderContentViewport{0, 0, 800U, 600U}, 1.0);
  bool ok = expect(terrain.valid && terrain.terrainHit && !terrain.voxelHit &&
                       !terrain.objectHit && terrain.grid.targetFacts.valid &&
                       terrain.grid.targetFacts.source ==
                           cr::CreativePlacementTargetSource::Terrain &&
                       terrain.grid.targetFacts.hostKind ==
                           cr::CreativeObjectKind::TerrainPatch,
                   "world target reports derived terrain hit") &&
            expect(terrain.terrainCell == cr::CreativeTerrainCoord2{0, 0} &&
                       terrain.objectKind == cr::CreativeObjectKind::TerrainPatch,
                   "terrain target exposes exact authored-grid cell and role") &&
            expect(terrain.grid.targetCell.x == 0 &&
                       terrain.grid.targetCell.y == 3 &&
                       terrain.grid.targetCell.z == 0,
                   "terrain top resolves the occupied surface cell");

  const cr::CreativeVoxelEdit voxelEdit{{0, 3, 0},
                                         cr::CreativeObjectKind::Crate};
  static_cast<void>(document.applyVoxelEdits(std::span{&voxelEdit, 1U}));
  const CreativeEditorWorldTarget tied = resolveCreativeEditorWorldTarget(
      document, camera, CreativeEditorPickFrame{},
      iggy3d::RenderContentViewport{0, 0, 800U, 600U}, 1.0);
  return expect(tied.voxelHit && !tied.terrainHit &&
                    tied.grid.targetFacts.valid &&
                    tied.grid.targetFacts.source ==
                        cr::CreativePlacementTargetSource::Voxel &&
                    tied.grid.targetFacts.hostKind == voxelEdit.material &&
                    tied.voxelCell.x == voxelEdit.cell.x &&
                    tied.voxelCell.y == voxelEdit.cell.y &&
                    tied.voxelCell.z == voxelEdit.cell.z,
                "authored voxel wins an equal-distance derived-terrain tie") &&
         ok;
}

bool removalStrokeDeletesVoxelAndGroupsHistory() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 107U);
  const cr::CreativeVoxelEdit edit{{4, 1, -2}, cr::CreativeObjectKind::Wall};
  static_cast<void>(appState.facade.applyVoxelEdits(std::span{&edit, 1U}));
  appState.history = {};

  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.target.valid = true;
  editor.interaction.target.voxelHit = true;
  editor.interaction.target.voxelCell = edit.cell;
  editor.interaction.target.objectKind = edit.material;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Primary, true, true, false), 0U);
  bool ok = expect(appState.facade.document()
                       .voxelField()
                       .occupiedCellCount() == 0U,
                   "primary press removes voxel immediately") &&
            expect(editor.interaction.materialStroke.transaction.active,
                   "voxel removal keeps gesture transaction open");
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Primary, false, false, true),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  ok = expect(cr::creativeUndoDepth(appState.history) == 1U,
              "voxel removal gesture records one undo") &&
       ok;
  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  return expect(undo.accepted &&
                    appState.facade.document().voxelField().materialAt(
                        edit.cell) == edit.material,
                "undo restores removed voxel") &&
         ok;
}

bool gamepadAcceptPlacesAndRejectRemoves() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 109U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  setPlaceTarget(editor, 3, 0, 2);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true), 1U);
  bool ok = expect(appState.facade.document()
                           .voxelField()
                           .occupiedCellCount() == 1U &&
                       cr::creativeUndoDepth(appState.history) == 1U,
                   "gamepad X accept places and commits one gesture");

  editor.interaction.target.voxelHit = true;
  editor.interaction.target.voxelCell = {3, 0, 2};
  editor.interaction.target.objectKind = cr::CreativeObjectKind::Wall;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Reject, true, true, false), 2U);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Reject, false, false, true), 3U);
  return expect(appState.facade.document()
                        .voxelField()
                        .occupiedCellCount() == 0U &&
                    cr::creativeUndoDepth(appState.history) == 2U,
                "gamepad Circle reject removes and commits one gesture") &&
         ok;
}

bool structuralSpanPreviewPlacementCancelAndUndoStayInParity() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 124U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Beam);
  setPlaceTarget(editor, 0, 0, 0);

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  const bool firstAnchorReady =
      editor.interaction.structuralSpan.active &&
      sameVec3(editor.interaction.structuralSpan.firstAnchor,
               {0.5, 0.0, 0.5}) &&
      appState.facade.document().objectCount() == 0U &&
      cr::creativeUndoDepth(appState.history) == 0U &&
      creativeEditorHeldItemStatusLabel(editor).find("SET END") !=
          std::string::npos;

  setPlaceTarget(editor, 4, 0, 3);
  const cr::CreativeStructuralSpanPlan expected =
      cr::planCreativeStructuralSpan(
          {cr::CreativeObjectKind::Beam, {0.5, 0.0, 0.5},
           {4.5, 0.0, 3.5}});
  iggy3d::FrameInput preview;
  attachCreativeEditorPlacementPreviews(
      editor, false, preview, &appState.facade.document());
  const iggy3d::Mat4& targetPreview =
      preview.creativePreview.items[0].clipFromModel;
  const auto previewAxisLength = [&](std::uint32_t column) {
    const float x = iggy3d::at(targetPreview, 0U, column);
    const float y = iggy3d::at(targetPreview, 1U, column);
    const float z = iggy3d::at(targetPreview, 2U, column);
    return std::sqrt(x * x + y * y + z * z);
  };
  bool ok = expect(firstAnchorReady,
                   "first X press records only the structural start anchor");
  ok = expect(expected.accepted &&
                       preview.creativePreview.itemCount == 2U &&
                       preview.creativePreview.items[0].role ==
                           iggy3d::RenderCreativePreviewRole::PlacementValid &&
                       preview.creativePreview.items[1].role ==
                           iggy3d::RenderCreativePreviewRole::Held &&
                       near(iggy3d::at(targetPreview, 0U, 3U), 2.5F) &&
                       near(iggy3d::at(targetPreview, 1U, 3U), 0.175F) &&
                       near(iggy3d::at(targetPreview, 2U, 3U), 2.0F) &&
                       near(previewAxisLength(0U), 5.0F) &&
                       near(previewAxisLength(1U), 0.35F) &&
                       near(previewAxisLength(2U), 0.35F),
              "draft preview is the exact diagonal span plan") &&
       ok;

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 1U);
  const cr::CreativeObject* placed =
      appState.facade.document().objects().empty()
          ? nullptr
          : &appState.facade.document().objects().front();
  ok = expect(placed != nullptr &&
                  placed->kind == cr::CreativeObjectKind::Beam &&
                  sameTransform(placed->transform, expected.transform) &&
                  sameBounds(placed->bounds, expected.authoredBounds) &&
                  !editor.interaction.structuralSpan.active &&
                  appState.facade.document().objectCount() == 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "second X press creates one exact span and one undo record") &&
       ok;

  setPlaceTarget(editor, 4, 0, 3);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, true, false),
      2U);
  setPlaceTarget(editor, 0, 0, 0);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, true, false),
      3U);
  ok = expect(editor.interaction.structuralSpan.active &&
                  editor.interaction.placementFeedback.status ==
                      CreativeEditorPlacementFeedbackStatus::Rejected &&
                  appState.facade.document().objectCount() == 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "reversed duplicate span is rejected without history") &&
       ok;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Primary, true, true, false), 4U);
  ok = expect(!editor.interaction.structuralSpan.active &&
                  appState.facade.document().objectCount() == 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "left click cancels a live span without mutating the document") &&
       ok;

  editor.interaction.target.objectHit = true;
  editor.interaction.target.objectId = placed->id;
  editor.interaction.target.objectKind = placed->kind;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Reject, true, true, false), 5U);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Reject, false, false, true), 6U);
  ok = expect(appState.facade.document().objectCount() == 0U &&
                  cr::creativeUndoDepth(appState.history) == 2U,
              "Circle retains ordinary remove behavior outside a draft") &&
       ok;

  const cr::CreativeHistoryApplyReceipt undoRemove = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeHistoryApplyReceipt undoPlace = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  setPlaceTarget(editor, 1, 0, 1);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 7U);
  finalizeCreativeEditorContinuousGestures(
      appState, editor, "test_structural_span_interrupted");
  return expect(undoRemove.accepted && undoPlace.accepted &&
                    appState.facade.document().objectCount() == 0U,
                "remove and structural placement undo as separate gestures") &&
         expect(!editor.interaction.structuralSpan.active &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "lifecycle interruption cancels an empty structural draft") &&
         ok;
}

bool structuralSpanEndpointEditPreviewsCommitsAndCancelsAtomically() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 125U);
  const cr::CreativeStructuralSpanPlan sourcePlan =
      cr::planCreativeStructuralSpan(
          {cr::CreativeObjectKind::Beam, {0.5, 0.0, 0.5},
           {4.5, 0.0, 0.5}});
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Beam;
  create.name = "Editable Beam";
  create.transform = sourcePlan.transform;
  create.hasTransformOverride = true;
  create.bounds = sourcePlan.authoredBounds;
  create.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt created =
      appState.facade.createDocumentObject(create);
  const std::array selectedIds{created.objectId};
  const cr::CreativeSelectionReceipt selected =
      appState.facade.selectTargets(selectedIds, created.objectId);

  CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::ObjectMove,
      cr::CreativeObjectKind::Unknown};
  editor.frameIndex = 20U;
  syncCreativeEditorHeldItem(appState, editor);
  syncCreativeEditorStructuralSpanEditState(
      appState, editor.interaction.structuralSpanEdit);
  setPlaceTarget(editor, 7, 0, 2);

  CreativeEditorPickFrame pickFrame;
  PathPointHandleHit endpointHandle;
  endpointHandle.objectId = created.objectId;
  endpointHandle.pointIndex = 1U;
  endpointHandle.aabb.minX = 396.0F;
  endpointHandle.aabb.minY = 296.0F;
  endpointHandle.aabb.maxX = 404.0F;
  endpointHandle.aabb.maxY = 304.0F;
  endpointHandle.aabb.valid = true;
  pickFrame.structuralSpanEndpointHandles.handles[0] = endpointHandle;
  pickFrame.structuralSpanEndpointHandles.count = 1U;
  iggy3d::RenderCameraFrame camera;
  const iggy3d::RenderContentViewport viewport{0, 0, 800U, 600U};
  const cr::CreativeWorldActionFrame accept =
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false);
  const std::uint64_t revisionBefore =
      appState.facade.document().revision();
  processCreativeEditorMoveInteraction(
      {appState, editor, accept, cr::kCreativeInputModifierNone, camera,
       pickFrame, viewport});

  const cr::CreativeObject* source =
      appState.facade.findObject(created.objectId);
  const cr::CreativeStructuralSpanEditPlan expected =
      source != nullptr
          ? cr::planCreativeStructuralSpanEdit(
                {source->kind, source->transform, source->bounds,
                 cr::CreativeStructuralSpanEndpoint::Second,
                 editor.interaction.target.grid.placementAnchor})
          : cr::CreativeStructuralSpanEditPlan{};
  iggy3d::FrameInput previewFrame;
  attachCreativeEditorPlacementPreviews(
      editor, false, previewFrame, &appState.facade.document());
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> endpointLines;
  const std::size_t endpointEdgeCount =
      appendCreativeEditorStructuralSpanEditWireframe(
          appState, editor.interaction.structuralSpanEdit, 0.04F,
          endpointLines);
  bool ok = expect(sourcePlan.accepted && created.accepted && selected.accepted,
                   "structural endpoint fixture creates and selects one beam");
  ok = expect(editor.interaction.structuralSpanEdit.active &&
                  editor.interaction.structuralSpanEdit.selectedEndpoint ==
                      cr::CreativeStructuralSpanEndpoint::Second &&
                  expected.accepted && expected.changed &&
                  appState.facade.document().revision() == revisionBefore &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "first X selects one endpoint without mutating history") &&
       expect(previewFrame.creativePreview.itemCount == 1U &&
                  previewFrame.creativePreview.items[0].role ==
                      iggy3d::RenderCreativePreviewRole::PlacementValid &&
                  endpointEdgeCount == 36U,
              "endpoint edit renders one exact solid ghost and bounded "
              "endpoint wireframes") &&
       ok;

  processCreativeEditorMoveInteraction(
      {appState, editor, accept, cr::kCreativeInputModifierNone, camera,
       pickFrame, viewport});
  const cr::CreativeObject* edited =
      appState.facade.findObject(created.objectId);
  ok = expect(edited != nullptr &&
                  sameTransform(edited->transform, expected.transform) &&
                  sameBounds(edited->bounds, expected.authoredBounds) &&
                  appState.facade.document().revision() == revisionBefore + 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U &&
                  !editor.interaction.structuralSpanEdit.active &&
                  editor.interaction.structuralSpanEdit.available,
              "second X atomically commits transform and bounds as one undo") &&
       expect(creativeEditorHeldItemStatusLabel(editor).find("SPAN ENDS") !=
                  std::string::npos,
              "Object Move status exposes structural endpoint editing") &&
       ok;

  const bool undoAccepted =
      undoLastEdit(appState, "structural_endpoint_edit_undo");
  const cr::CreativeObject* restored =
      appState.facade.findObject(created.objectId);
  static_cast<void>(appState.facade.selectTargets(selectedIds,
                                                   created.objectId));
  syncCreativeEditorStructuralSpanEditState(
      appState, editor.interaction.structuralSpanEdit);
  const std::uint64_t revisionBeforeCancel =
      appState.facade.document().revision();
  processCreativeEditorMoveInteraction(
      {appState, editor, accept, cr::kCreativeInputModifierNone, camera,
       pickFrame, viewport});
  const cr::CreativeWorldActionFrame reject =
      actionFrame(cr::CreativeWorldActionId::Reject, true, true, false);
  processCreativeEditorMoveInteraction(
      {appState, editor, reject, cr::kCreativeInputModifierNone, camera,
       pickFrame, viewport});
  ok = expect(undoAccepted && restored != nullptr &&
                  sameTransform(restored->transform, sourcePlan.transform) &&
                  sameBounds(restored->bounds, sourcePlan.authoredBounds),
              "one undo restores both structural fields exactly") &&
       expect(!editor.interaction.structuralSpanEdit.active &&
                  appState.facade.document().revision() ==
                      revisionBeforeCancel &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "Circle cancels endpoint editing without mutation or history") &&
       ok;

  processCreativeEditorMoveInteraction(
      {appState, editor, accept, cr::kCreativeInputModifierNone, camera,
       pickFrame, viewport});
  finalizeCreativeEditorContinuousGestures(
      appState, editor, "structural_endpoint_interrupted");
  ok = expect(!editor.interaction.structuralSpanEdit.active &&
                  appState.facade.document().revision() ==
                      revisionBeforeCancel &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "focus and modal lifecycle finalization cancels an empty "
              "endpoint edit") &&
       ok;

  processCreativeEditorMoveInteraction(
      {appState, editor, accept, cr::kCreativeInputModifierNone, camera,
       pickFrame, viewport});
  const cr::CreativeDocumentCreateReceipt unrelated =
      appState.facade.createDocumentObject(cr::CreativeObjectKind::Crate);
  const std::uint64_t staleRevision = appState.facade.document().revision();
  processCreativeEditorMoveInteraction(
      {appState, editor, accept, cr::kCreativeInputModifierNone, camera,
       pickFrame, viewport});
  const cr::CreativeObject* afterStaleAttempt =
      appState.facade.findObject(created.objectId);
  return expect(unrelated.accepted && afterStaleAttempt != nullptr &&
                    sameTransform(afterStaleAttempt->transform,
                                  sourcePlan.transform) &&
                    sameBounds(afterStaleAttempt->bounds,
                               sourcePlan.authoredBounds) &&
                    appState.facade.document().revision() == staleRevision &&
                    !editor.interaction.structuralSpanEdit.active &&
                    editor.interaction.placementFeedback.status ==
                        CreativeEditorPlacementFeedbackStatus::Rejected &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "stale revisions reject the endpoint edit without falling "
                "through to ordinary move") &&
         ok;
}

bool materialBrushPaintsErasesPreviewsAndGroupsHistory() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 110U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  syncCreativeEditorHeldItem(appState, editor);
  setPlaceTarget(editor, 0);

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput previewFrame;
  CreativeEditorOverlayFrame previewOverlay;
  const std::uint64_t revisionBeforePreview =
      appState.facade.document().revision();
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, previewFrame, projectionRequest,
       1280U, 720U, 0.03F, false},
      previewOverlay);

  constexpr std::size_t kWireEdgesPerVoxel = 12U;
  const bool previewBrushRangeValid =
      previewOverlay.materialBrushEdgeCount <=
      previewOverlay.combinedWireLines.size();
  const std::size_t previewBrushStart =
      previewBrushRangeValid
          ? previewOverlay.combinedWireLines.size() -
                previewOverlay.materialBrushEdgeCount
          : 0U;
  bool ok = expect(previewOverlay.materialBrushEdgeCount ==
                           7U * kWireEdgesPerVoxel &&
                       previewOverlay.combinedWireLines.size() ==
                           previewOverlay.placementGridLineCount +
                               previewOverlay.materialBrushEdgeCount &&
                       previewBrushRangeValid &&
                       near(previewOverlay.combinedWireLines[previewBrushStart]
                                .color.r,
                            0.22F) &&
                       near(previewOverlay.combinedWireLines[previewBrushStart]
                                .color.g,
                            1.0F) &&
                       previewFrame.creativePreview.itemCount == 1U &&
                       previewFrame.creativePreview.items[0].role ==
                           iggy3d::RenderCreativePreviewRole::Held &&
                       appState.facade.document().revision() ==
                           revisionBeforePreview,
                   "sphere preview shows its exact seven green voxel cells") &&
            expect(editor.quickEdit.options.count == 6U &&
                       creativeEditorQuickEditStatusLabel(editor) ==
                           "BRUSH SHAPE SPHERE" &&
                       creativeEditorHeldItemStatusLabel(editor) ==
                           "Brush | Wall | SPHERE | 3 CELLS | SOLID | FREE | "
                           "OVERWRITE | 7 VOXELS | [BRUSH SHAPE SPHERE]",
                   "material brush status exposes channel and stamp count");

  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cube;
  syncCreativeEditorQuickEdit(editor);
  iggy3d::FrameInput cubePreviewFrame;
  CreativeEditorOverlayFrame cubePreviewOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, cubePreviewFrame, projectionRequest,
       1280U, 720U, 0.03F, false},
      cubePreviewOverlay);
  ok = expect(cubePreviewOverlay.materialBrushEdgeCount ==
                      27U * kWireEdgesPerVoxel &&
                  appState.facade.document().revision() ==
                      revisionBeforePreview &&
                  creativeEditorHeldItemStatusLabel(editor).find(
                      "27 VOXELS") != std::string::npos,
              "cube preview shows all twenty-seven cells without mutation") &&
       ok;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Sphere;
  syncCreativeEditorQuickEdit(editor);
  ok = expect(processCreativeEditorQuickEditAction(
                  editor, cr::CreativeInputActionId::QuickEditNext) &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "BRUSH SIZE 3 CELLS" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditIncrease) &&
                  editor.toolSettings.materialBrushSize ==
                      cr::CreativeMaterialBrushSize::FiveCells,
              "dpad channel selection adjusts the bounded brush size") &&
       ok;
  ok = expect(processCreativeEditorQuickEditAction(
                  editor, cr::CreativeInputActionId::QuickEditNext) &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "BRUSH BODY SOLID" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditIncrease) &&
                  editor.toolSettings.materialBrushFill ==
                      cr::CreativeMaterialBrushFill::Shell &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditNext) &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "BRUSH GUIDE FREE" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditNext) &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "SYMMETRY OFF" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditNext) &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "BRUSH MASK OVERWRITE" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditIncrease) &&
                  editor.toolSettings.materialBrushMask ==
                      cr::CreativeMaterialBrushMask::AddOnly,
              "dpad exposes body guide symmetry and mask as bounded channels") &&
       ok;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::ThreeCells;
  editor.toolSettings.materialBrushFill =
      cr::CreativeMaterialBrushFill::Solid;
  editor.toolSettings.materialBrushMask =
      cr::CreativeMaterialBrushMask::Overwrite;
  editor.quickEdit.selectedIndex = 0U;
  syncCreativeEditorQuickEdit(editor);

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  ok = expect(appState.facade.document()
                          .voxelField()
                          .occupiedCellCount() == 7U &&
                  appState.facade.document().revision() ==
                      revisionBeforePreview + 1U &&
                  editor.interaction.materialStroke.visitedCount == 7U &&
                  editor.interaction.materialStroke.transaction.active,
              "X applies the default sphere as one atomic document mutation") &&
       ok;

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, false, false),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  ok = expect(appState.facade.document()
                      .voxelField()
                      .occupiedCellCount() == 7U &&
                  editor.interaction.materialStroke.visitedCount == 7U,
              "stationary brush hold deduplicates every stamped cell") &&
       ok;

  setPlaceTarget(editor, 4);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, false, false),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);
  ok = expect(appState.facade.document()
                          .voxelField()
                          .occupiedCellCount() == 27U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "fast brush movement fills every crossed center in one undo") &&
       ok;
  ok = expect(undoLastEdit(appState, "test_material_brush_paint_undo") &&
                  appState.facade.document()
                          .voxelField()
                          .occupiedCellCount() == 0U,
              "one undo removes the complete paint gesture") &&
       ok;

  const cr::CreativeMaterialBrushStampPlan erasePlan =
      cr::planCreativeMaterialBrushStamp(
          {cr::CreativeMaterialBrushShape::Sphere,
           cr::CreativeMaterialBrushSize::ThreeCells, {0, 0, 0}});
  std::array<cr::CreativeVoxelEdit,
             cr::kCreativeMaterialBrushStampCapacity>
      seedEdits{};
  for (std::size_t index = 0U; index < erasePlan.cellCount; ++index) {
    seedEdits[index] = {erasePlan.cells[index], cr::CreativeObjectKind::Wall};
  }
  const cr::CreativeVoxelMutationReceipt seeded = appState.facade.applyVoxelEdits(
      {seedEdits.data(), erasePlan.cellCount});
  appState.history = {};
  setPlaceTarget(editor, 0);
  editor.interaction.target.voxelHit = true;
  editor.interaction.target.voxelCell = {0, 0, 0};
  editor.interaction.target.objectKind = cr::CreativeObjectKind::Wall;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Reject, true, true, false), 10U);
  iggy3d::FrameInput erasePreviewFrame;
  CreativeEditorOverlayFrame erasePreviewOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, erasePreviewFrame,
       projectionRequest, 1280U, 720U, 0.03F, false},
      erasePreviewOverlay);
  const bool eraseBrushRangeValid =
      erasePreviewOverlay.materialBrushEdgeCount <=
      erasePreviewOverlay.combinedWireLines.size();
  const std::size_t eraseBrushStart =
      eraseBrushRangeValid
          ? erasePreviewOverlay.combinedWireLines.size() -
                erasePreviewOverlay.materialBrushEdgeCount
          : 0U;
  ok = expect(erasePreviewOverlay.materialBrushEdgeCount ==
                      7U * kWireEdgesPerVoxel &&
                  eraseBrushRangeValid &&
                  near(erasePreviewOverlay.combinedWireLines[eraseBrushStart]
                           .color.r,
                       1.0F) &&
                  near(erasePreviewOverlay.combinedWireLines[eraseBrushStart]
                           .color.g,
                       0.2F),
              "erase preview shows the same seven voxel cells in red") &&
       ok;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Reject, false, false, true), 11U);
  return expect(seeded.accepted && seeded.changed &&
                    appState.facade.document()
                            .voxelField()
                            .occupiedCellCount() == 0U &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "Circle erases the aimed bounded brush shape in one gesture") &&
         expect(undoLastEdit(appState, "test_material_brush_erase_undo") &&
                    appState.facade.document()
                            .voxelField()
                            .occupiedCellCount() == 7U,
                "one undo restores the complete erase gesture") &&
         ok;
}

bool materialBrushShellPreviewMatchesMutationAndUndo() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 119U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Sphere;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::FiveCells;
  editor.toolSettings.materialBrushFill =
      cr::CreativeMaterialBrushFill::Shell;
  syncCreativeEditorHeldItem(appState, editor);
  syncCreativeEditorQuickEdit(editor);
  setPlaceTarget(editor, 0);

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput previewFrame;
  CreativeEditorOverlayFrame previewOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, previewFrame, projectionRequest,
       1280U, 720U, 0.03F, false},
      previewOverlay);

  constexpr std::size_t kWireEdgesPerVoxel = 12U;
  bool ok = expect(previewOverlay.materialBrushEdgeCount ==
                           26U * kWireEdgesPerVoxel &&
                       creativeEditorHeldItemStatusLabel(editor).find(
                           "5 CELLS | SHELL | FREE") != std::string::npos &&
                       creativeEditorHeldItemStatusLabel(editor).find(
                           "26 VOXELS") != std::string::npos,
                   "shell preview shows the exact planned sphere boundary");

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true), 1U);
  const cr::CreativeVoxelField& voxels =
      appState.facade.document().voxelField();
  ok = expect(voxels.occupiedCellCount() == 26U &&
                  voxels.validateInvariants(),
              "shell mutation writes a valid preview-sized voxel field") &&
       ok;
  ok = expect(!voxels.occupied({0, 0, 0}),
              "shell mutation omits its center interior") &&
       ok;
  ok = expect(!voxels.occupied({1, 0, 0}),
              "shell mutation omits its axial interior") &&
       ok;
  ok = expect(voxels.occupied({2, 0, 0}),
              "shell mutation retains its outer boundary") &&
       ok;
  ok = expect(cr::creativeUndoDepth(appState.history) == 1U,
              "shell mutation records one gesture history entry") &&
       ok;
  return expect(undoLastEdit(appState, "test_material_brush_shell_undo") &&
                    appState.facade.document()
                            .voxelField()
                            .occupiedCellCount() == 0U,
                "one undo removes the complete shell gesture") &&
         ok;
}

bool materialBrushCylinderAxisDrivesPreviewAndMutation() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 111U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cylinder;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::ThreeCells;
  editor.toolSettings.materialBrushAxis = cr::CreativeAxis3::X;
  syncCreativeEditorHeldItem(appState, editor);
  syncCreativeEditorQuickEdit(editor);
  setPlaceTarget(editor, 0);

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput previewFrame;
  CreativeEditorOverlayFrame previewOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, previewFrame, projectionRequest,
       1280U, 720U, 0.03F, false},
      previewOverlay);

  constexpr std::size_t kWireEdgesPerVoxel = 12U;
  bool ok = expect(previewOverlay.materialBrushEdgeCount ==
                           15U * kWireEdgesPerVoxel &&
                       editor.quickEdit.options.count == 7U &&
                       editor.quickEdit.options.ids[1] ==
                           cr::CreativeToolOptionId::MaterialBrushAxis &&
                       creativeEditorHeldItemStatusLabel(editor).find(
                           "CYLINDER X | 3 CELLS") != std::string::npos,
                   "cylinder preview and HUD expose the selected X axis");

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true), 1U);
  const cr::CreativeVoxelField& voxels =
      appState.facade.document().voxelField();
  ok = expect(voxels.occupiedCellCount() == 15U &&
                  voxels.occupied({1, 0, 1}) &&
                  !voxels.occupied({0, 1, 1}) &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "stroke mutation uses the same X-cylinder plan as preview") &&
       ok;

  editor.quickEdit.selectedIndex = 0U;
  ok = expect(processCreativeEditorQuickEditAction(
                  editor, cr::CreativeInputActionId::QuickEditNext) &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "CYLINDER AXIS X" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditIncrease) &&
                  editor.toolSettings.materialBrushAxis ==
                      cr::CreativeAxis3::Y,
              "D-pad quick edit rotates the cylinder without a new binding") &&
       ok;
  return ok;
}

bool materialBrushPlaneGuideStaysAnchoredForTheGesture() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 112U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cube;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::ThreeCells;
  editor.toolSettings.materialBrushGuide =
      cr::CreativeMaterialBrushGuide::PlaneY;
  syncCreativeEditorHeldItem(appState, editor);
  syncCreativeEditorQuickEdit(editor);
  setPlaceTarget(editor, 0, 0, 0);

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  bool ok = expect(appState.facade.document()
                           .voxelField()
                           .occupiedCellCount() == 9U &&
                       editor.interaction.materialStroke.hasBrushAnchor &&
                       editor.interaction.materialStroke.brushConfig.guide ==
                           cr::CreativeMaterialBrushGuide::PlaneY,
                   "first sample captures and flattens the gesture plane");

  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Sphere;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::FiveCells;
  editor.toolSettings.materialBrushGuide =
      cr::CreativeMaterialBrushGuide::Free;
  editor.toolSettings.materialBrushMask =
      cr::CreativeMaterialBrushMask::AddOnly;
  setPlaceTarget(editor, 2, 2, 0);
  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput previewFrame;
  CreativeEditorOverlayFrame previewOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, previewFrame, projectionRequest,
       1280U, 720U, 0.03F, false},
      previewOverlay);
  constexpr std::size_t kWireEdgesPerVoxel = 12U;
  const std::size_t previewStart =
      previewOverlay.combinedWireLines.size() -
      previewOverlay.materialBrushEdgeCount;
  const bool previewHeldAtAnchor = std::all_of(
      previewOverlay.combinedWireLines.begin() +
          static_cast<std::ptrdiff_t>(previewStart),
      previewOverlay.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.start.y >= 0.0F && line.start.y <= 1.0F &&
               line.end.y >= 0.0F && line.end.y <= 1.0F;
      });
  ok = expect(previewOverlay.materialBrushEdgeCount ==
                      6U * kWireEdgesPerVoxel &&
                  previewHeldAtAnchor &&
                  creativeEditorHeldItemStatusLabel(editor).find(
                      "3 CELLS | SOLID | PLANE Y") != std::string::npos,
              "active preview freezes settings on the uneven-aim plane") &&
       ok;

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, false, false),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true),
      cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);
  const cr::CreativeVoxelField& voxels =
      appState.facade.document().voxelField();
  return expect(voxels.occupiedCellCount() == 15U &&
                    voxels.occupied({2, 0, 0}) &&
                    !voxels.occupied({2, 2, 0}) &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "constrained sweep fills one flat layer in one undo") &&
         expect(!editor.interaction.materialStroke.hasBrushAnchor,
                "release clears the gesture-local plane anchor") &&
         ok;
}

bool materialBrushLineGuideConstrainsPathAndRendersAxis() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 116U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cube;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::OneCell;
  editor.toolSettings.materialBrushGuide =
      cr::CreativeMaterialBrushGuide::LineX;
  syncCreativeEditorHeldItem(appState, editor);
  syncCreativeEditorQuickEdit(editor);
  editor.quickEdit.selectedIndex = 3U;
  setPlaceTarget(editor, 0, 0, 0);

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  setPlaceTarget(editor, 3, 2, 4);

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput previewFrame;
  CreativeEditorOverlayFrame previewOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, previewFrame, projectionRequest,
       1280U, 720U, 0.03F, false},
      previewOverlay);
  const std::size_t guideIndex =
      previewOverlay.combinedWireLines.size() -
      previewOverlay.materialBrushEdgeCount -
      previewOverlay.materialBrushGuideLineCount;
  const iggy3d::RenderCreativeWireframeDebugLine& guideLine =
      previewOverlay.combinedWireLines[guideIndex];
  bool ok = expect(previewOverlay.materialBrushGuideLineCount == 1U &&
                       previewOverlay.materialBrushEdgeCount == 12U &&
                       near(guideLine.start.x, 0.5F) &&
                       near(guideLine.start.y, 0.5F) &&
                       near(guideLine.start.z, 0.5F) &&
                       near(guideLine.end.x, 3.5F) &&
                       near(guideLine.end.y, 0.5F) &&
                       near(guideLine.end.z, 0.5F) &&
                       near(guideLine.color.r, 1.0F) &&
                       near(guideLine.color.g, 0.22F) &&
                       near(guideLine.color.b, 0.18F) &&
                       creativeEditorQuickEditStatusLabel(editor) ==
                           "BRUSH GUIDE LINE X",
                   "X line guide renders from anchor to constrained target");

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, false, false),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true),
      cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);
  const cr::CreativeVoxelField& voxels =
      appState.facade.document().voxelField();
  ok = expect(voxels.occupiedCellCount() == 4U &&
                  voxels.occupied({0, 0, 0}) && voxels.occupied({1, 0, 0}) &&
                  voxels.occupied({2, 0, 0}) && voxels.occupied({3, 0, 0}) &&
                  !voxels.occupied({3, 2, 4}) &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "X line sweep ignores off-axis aim and commits one undo") &&
       ok;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> axisLines;
  const cr::CreativeGridSettings grid =
      appState.facade.document().gridSettings();
  const bool yLine = appendCreativeMaterialBrushGuideLine(
      axisLines, cr::CreativeMaterialBrushGuide::LineY, {0, 0, 0},
      {0, 2, 0}, grid, 0.05F);
  const bool zLine = appendCreativeMaterialBrushGuideLine(
      axisLines, cr::CreativeMaterialBrushGuide::LineZ, {0, 0, 0},
      {0, 0, 2}, grid, 0.05F);
  const bool planeLine = appendCreativeMaterialBrushGuideLine(
      axisLines, cr::CreativeMaterialBrushGuide::PlaneX, {0, 0, 0},
      {0, 2, 0}, grid, 0.05F);
  ok = expect(yLine && zLine && !planeLine && axisLines.size() == 2U &&
                  near(axisLines[0].color.r, 0.24F) &&
                  near(axisLines[0].color.g, 1.0F) &&
                  near(axisLines[0].color.b, 0.34F) &&
                  near(axisLines[1].color.r, 0.22F) &&
                  near(axisLines[1].color.g, 0.55F) &&
                  near(axisLines[1].color.b, 1.0F),
              "line guide colors consistently encode X red Y green Z blue") &&
       ok;
  return expect(!editor.interaction.materialStroke.hasBrushAnchor,
                "line guide anchor clears on release") &&
         ok;
}

bool materialBrushSymmetryUsesGesturePivotPreviewAndHistory() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 117U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cube;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::OneCell;
  editor.toolSettings.materialBrushSymmetry =
      cr::CreativeMaterialBrushSymmetry::MirrorX;
  syncCreativeEditorHeldItem(appState, editor);
  syncCreativeEditorQuickEdit(editor);
  editor.quickEdit.selectedIndex = 4U;
  setPlaceTarget(editor, 0, 0, 0);

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  setPlaceTarget(editor, 2, 0, 0);

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput previewFrame;
  CreativeEditorOverlayFrame previewOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, previewFrame, projectionRequest,
       1280U, 720U, 0.03F, false},
      previewOverlay);

  constexpr std::size_t kWireEdgesPerVoxel = 12U;
  const bool previewLayoutValid =
      previewOverlay.materialBrushPivotEdgeCount == kWireEdgesPerVoxel &&
      previewOverlay.materialBrushGuideLineCount == 0U &&
      previewOverlay.materialBrushEdgeCount == 2U * kWireEdgesPerVoxel &&
      previewOverlay.combinedWireLines.size() >=
          previewOverlay.materialBrushPivotEdgeCount +
              previewOverlay.materialBrushEdgeCount;
  const std::size_t boxesBegin =
      previewLayoutValid
          ? previewOverlay.combinedWireLines.size() -
                previewOverlay.materialBrushEdgeCount
          : 0U;
  const std::size_t pivotBegin =
      previewLayoutValid
          ? boxesBegin - previewOverlay.materialBrushPivotEdgeCount
          : 0U;
  const iggy3d::RenderCreativeWireframeDebugLine* pivotLine =
      previewLayoutValid
          ? &previewOverlay.combinedWireLines[pivotBegin]
          : nullptr;
  const iggy3d::RenderCreativeWireframeDebugLine* directLine =
      previewLayoutValid
          ? &previewOverlay.combinedWireLines[boxesBegin]
          : nullptr;
  const iggy3d::RenderCreativeWireframeDebugLine* mirroredLine =
      previewLayoutValid
          ? &previewOverlay
                 .combinedWireLines[boxesBegin + kWireEdgesPerVoxel]
          : nullptr;
  bool ok = expect(
      previewLayoutValid &&
          editor.interaction.materialStroke.hasBrushAnchor &&
          editor.interaction.materialStroke.brushAnchor.x == 0 &&
          editor.interaction.materialStroke.brushConfig.symmetry ==
              cr::CreativeMaterialBrushSymmetry::MirrorX &&
          near(pivotLine->color.r, 1.0F) &&
          near(pivotLine->color.g, 0.82F) &&
          near(directLine->color.r, 0.22F) &&
          near(directLine->color.g, 1.0F) &&
          near(mirroredLine->color.r, 0.18F) &&
          near(mirroredLine->color.g, 0.9F) &&
          near(mirroredLine->color.b, 1.0F) &&
          creativeEditorQuickEditStatusLabel(editor) ==
              "SYMMETRY MIRROR X" &&
          creativeEditorHeldItemStatusLabel(editor).find("MIRROR X") !=
              std::string::npos,
      "symmetry preview exposes its frozen pivot direct and mirror roles");

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, false, false),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true),
      cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);

  const cr::CreativeVoxelField& voxels =
      appState.facade.document().voxelField();
  ok = expect(voxels.occupiedCellCount() == 5U &&
                  voxels.occupied({-2, 0, 0}) &&
                  voxels.occupied({-1, 0, 0}) &&
                  voxels.occupied({0, 0, 0}) &&
                  voxels.occupied({1, 0, 0}) &&
                  voxels.occupied({2, 0, 0}) &&
                  cr::creativeUndoDepth(appState.history) == 1U &&
                  !editor.interaction.materialStroke.hasBrushAnchor,
              "one mirrored continuous gesture commits one history entry") &&
       ok;
  return expect(undoLastEdit(appState, "test_material_brush_symmetry_undo") &&
                    appState.facade.document()
                            .voxelField()
                            .occupiedCellCount() == 0U,
                "one undo removes both direct and mirrored stroke cells") &&
         ok;
}

bool materialBrushLockedPivotAvoidsCenterMutationAndPersists() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 118U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cube;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::OneCell;
  editor.toolSettings.materialBrushSymmetry =
      cr::CreativeMaterialBrushSymmetry::MirrorX;
  syncCreativeEditorHeldItem(appState, editor);
  syncCreativeEditorQuickEdit(editor);
  const cr::CreativeDocumentId documentId = appState.facade.document().id();
  updateCreativeMaterialBrushPivotAim(
      editor.interaction.materialBrushPivot, documentId, true, {0, 0, 0});
  const bool pivotLocked = lockCreativeMaterialBrushPivotFromAim(
      editor.interaction.materialBrushPivot);
  setPlaceTarget(editor, 2, 0, 0);

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput previewFrame;
  CreativeEditorOverlayFrame previewOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, previewFrame, projectionRequest,
       1280U, 720U, 0.03F, false},
      previewOverlay);

  constexpr std::size_t kWireEdgesPerVoxel = 12U;
  const bool previewLayoutValid =
      previewOverlay.materialBrushPivotEdgeCount == kWireEdgesPerVoxel &&
      previewOverlay.materialBrushEdgeCount == 2U * kWireEdgesPerVoxel &&
      previewOverlay.combinedWireLines.size() >=
          previewOverlay.materialBrushPivotEdgeCount +
              previewOverlay.materialBrushEdgeCount;
  const std::size_t boxesBegin =
      previewLayoutValid
          ? previewOverlay.combinedWireLines.size() -
                previewOverlay.materialBrushEdgeCount
          : 0U;
  const iggy3d::RenderCreativeWireframeDebugLine* directLine =
      previewLayoutValid
          ? &previewOverlay.combinedWireLines[boxesBegin]
          : nullptr;
  const iggy3d::RenderCreativeWireframeDebugLine* mirroredLine =
      previewLayoutValid
          ? &previewOverlay
                 .combinedWireLines[boxesBegin + kWireEdgesPerVoxel]
          : nullptr;
  bool ok = expect(
      pivotLocked && previewLayoutValid &&
          near(directLine->color.r, 0.22F) &&
          near(directLine->color.g, 1.0F) &&
          near(mirroredLine->color.r, 0.18F) &&
          near(mirroredLine->color.g, 0.9F) &&
          creativeEditorHeldItemStatusLabel(editor).find(
              "PIVOT LOCKED 0 0 0") != std::string::npos,
      "locked pivot previews direct and mirrored cells before a stroke");

  editor.interaction.target = {};
  iggy3d::FrameInput noAimFrame;
  CreativeEditorOverlayFrame noAimOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, noAimFrame, projectionRequest,
       1280U, 720U, 0.03F, false},
      noAimOverlay);
  ok = expect(noAimOverlay.materialBrushPivotEdgeCount ==
                      kWireEdgesPerVoxel &&
                  noAimOverlay.materialBrushEdgeCount == 0U,
              "locked yellow pivot remains visible without a paint target") &&
       ok;

  setPlaceTarget(editor, 2, 0, 0);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  ok = expect(editor.interaction.materialStroke.hasBrushAnchor &&
                  editor.interaction.materialStroke.brushAnchor.x == 2 &&
                  editor.interaction.materialStroke.hasSymmetryPivot &&
                  editor.interaction.materialStroke.symmetryPivot.x == 0,
              "stroke guide anchor remains separate from its locked pivot") &&
       ok;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true), 1U);
  const cr::CreativeVoxelField& voxels =
      appState.facade.document().voxelField();
  ok = expect(voxels.occupiedCellCount() == 2U &&
                  voxels.occupied({-2, 0, 0}) &&
                  !voxels.occupied({0, 0, 0}) &&
                  voxels.occupied({2, 0, 0}) &&
                  cr::creativeUndoDepth(appState.history) == 1U &&
                  editor.interaction.materialBrushPivot.locked,
              "locked pivot mirrors immediately without painting its center") &&
       ok;
  ok = expect(undoLastEdit(appState, "test_locked_symmetry_pivot_undo") &&
                  appState.facade.document()
                          .voxelField()
                          .occupiedCellCount() == 0U &&
                  editor.interaction.materialBrushPivot.locked,
              "one undo removes the mirrored pair without clearing the pivot") &&
       ok;

  resetCreativeMaterialBrushPivot(editor.interaction.materialBrushPivot,
                                  documentId);
  return expect(!editor.interaction.materialBrushPivot.locked &&
                    !editor.interaction.materialBrushPivot.aimAvailable,
                "document replacement reset clears even a same-id pivot") &&
         ok;
}

bool materialBrushInterpolatesDiagonalsAndBreaksOnTargetLoss() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 112U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cube;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::OneCell;
  syncCreativeEditorHeldItem(appState, editor);

  setPlaceTarget(editor, 0, 0, 0);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  setPlaceTarget(editor, 3, 3, 0);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, false, false),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);

  const cr::CreativeVoxelField& diagonalField =
      appState.facade.document().voxelField();
  bool ok = expect(diagonalField.occupiedCellCount() == 10U &&
                       diagonalField.materialAt({1, 0, 0}) ==
                           cr::CreativeObjectKind::Wall &&
                       diagonalField.materialAt({0, 1, 0}) ==
                           cr::CreativeObjectKind::Wall &&
                       diagonalField.materialAt({2, 3, 0}) ==
                           cr::CreativeObjectKind::Wall &&
                       diagonalField.materialAt({3, 2, 0}) ==
                           cr::CreativeObjectKind::Wall,
                   "fast diagonal sweep fills edge-crossing neighbors");

  editor.interaction.target = {};
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, false, false),
      cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);
  ok = expect(!editor.interaction.materialStroke.hasLastBrushCenter,
              "losing the target breaks brush interpolation continuity") &&
       ok;

  setPlaceTarget(editor, 8, 3, 0);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, false, false),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);
  ok = expect(appState.facade.document()
                          .voxelField()
                          .occupiedCellCount() == 11U &&
                  appState.facade.document().voxelField().materialAt(
                      {5, 3, 0}) == cr::CreativeObjectKind::Unknown &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "reacquired target starts a new stamp without bridging the gap") &&
       ok;
  return expect(undoLastEdit(appState, "test_interpolated_brush_undo") &&
                    appState.facade.document()
                            .voxelField()
                            .occupiedCellCount() == 0U,
                "one undo removes the complete interpolated paint gesture") &&
         ok;
}

bool materialBrushInterpolatesEraseSweep() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 113U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cube;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::OneCell;
  syncCreativeEditorHeldItem(appState, editor);

  std::array<cr::CreativeVoxelEdit, 6U> seedEdits{};
  for (std::int32_t x = 0; x < 6; ++x) {
    seedEdits[static_cast<std::size_t>(x)] = {
        {x, 0, 0}, cr::CreativeObjectKind::Wall};
  }
  const cr::CreativeVoxelMutationReceipt seeded =
      appState.facade.applyVoxelEdits(seedEdits);
  appState.history = {};

  setPlaceTarget(editor, 0);
  editor.interaction.target.voxelHit = true;
  editor.interaction.target.voxelCell = {0, 0, 0};
  editor.interaction.target.objectKind = cr::CreativeObjectKind::Wall;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Reject, true, true, false), 0U);
  setPlaceTarget(editor, 5);
  editor.interaction.target.voxelHit = true;
  editor.interaction.target.voxelCell = {5, 0, 0};
  editor.interaction.target.objectKind = cr::CreativeObjectKind::Wall;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Reject, true, false, false),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Reject, false, false, true),
      cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);

  return expect(seeded.accepted && seeded.changed &&
                    appState.facade.document()
                            .voxelField()
                            .occupiedCellCount() == 0U &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "fast erase sweep removes every crossed voxel") &&
         expect(undoLastEdit(appState, "test_interpolated_erase_undo") &&
                    appState.facade.document()
                            .voxelField()
                            .occupiedCellCount() == 6U,
                "one undo restores the complete interpolated erase gesture");
}

bool materialBrushMasksMatchPreviewAndMutation() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 114U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cube;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::OneCell;
  syncCreativeEditorHeldItem(appState, editor);

  const std::array seedEdits{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{2, 0, 0}, cr::CreativeObjectKind::Floor}};
  const cr::CreativeVoxelMutationReceipt seeded =
      appState.facade.applyVoxelEdits(seedEdits);
  appState.history = {};

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  const auto previewAt = [&](std::int32_t x) {
    setPlaceTarget(editor, x);
    iggy3d::FrameInput frame;
    CreativeEditorOverlayFrame overlay;
    buildAndAttachCreativeEditorOverlayFrame(
        {appState, editor, selection, gizmo, frame, projectionRequest,
         1280U, 720U, 0.03F, false},
        overlay);
    return overlay;
  };
  const auto previewIsColor = [](const CreativeEditorOverlayFrame& overlay,
                                 float red,
                                 float green) {
    if (overlay.materialBrushEdgeCount == 0U ||
        overlay.materialBrushEdgeCount > overlay.combinedWireLines.size()) {
      return false;
    }
    const std::size_t first =
        overlay.combinedWireLines.size() - overlay.materialBrushEdgeCount;
    return near(overlay.combinedWireLines[first].color.r, red) &&
           near(overlay.combinedWireLines[first].color.g, green);
  };
  std::uint64_t now = 0U;
  const auto paintAt = [&](std::int32_t x) {
    setPlaceTarget(editor, x);
    processCreativeMaterialStrokeFrame(
        appState, editor,
        actionFrame(cr::CreativeWorldActionId::Accept, true, true, false),
        now++);
    processCreativeMaterialStrokeFrame(
        appState, editor,
        actionFrame(cr::CreativeWorldActionId::Accept, false, false, true),
        now++);
  };

  editor.toolSettings.materialBrushMask =
      cr::CreativeMaterialBrushMask::AddOnly;
  const CreativeEditorOverlayFrame addBlocked = previewAt(0);
  const std::uint64_t revisionBeforeBlockedAdd =
      appState.facade.document().revision();
  paintAt(0);
  bool ok = expect(seeded.accepted && seeded.changed &&
                       addBlocked.materialBrushEdgeCount == 12U &&
                       previewIsColor(addBlocked, 1.0F, 0.15F) &&
                       appState.facade.document().revision() ==
                           revisionBeforeBlockedAdd &&
                       appState.facade.document().voxelField().materialAt(
                           {0, 0, 0}) == cr::CreativeObjectKind::Floor &&
                       cr::creativeUndoDepth(appState.history) == 0U,
                   "add-only previews and preserves occupied cells");

  const CreativeEditorOverlayFrame addAllowed = previewAt(1);
  paintAt(1);
  ok = expect(addAllowed.materialBrushEdgeCount == 12U &&
                  previewIsColor(addAllowed, 0.22F, 1.0F) &&
                  appState.facade.document().voxelField().materialAt(
                      {1, 0, 0}) == cr::CreativeObjectKind::Wall &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "add-only previews and fills empty cells") &&
       ok;

  editor.toolSettings.materialBrushMask =
      cr::CreativeMaterialBrushMask::Replace;
  const CreativeEditorOverlayFrame replaceBlocked = previewAt(3);
  const std::uint64_t revisionBeforeBlockedReplace =
      appState.facade.document().revision();
  paintAt(3);
  ok = expect(previewIsColor(replaceBlocked, 1.0F, 0.15F) &&
                  appState.facade.document().revision() ==
                      revisionBeforeBlockedReplace &&
                  appState.facade.document().voxelField().materialAt(
                      {3, 0, 0}) == cr::CreativeObjectKind::Unknown &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "replace previews and preserves empty cells") &&
       ok;

  const CreativeEditorOverlayFrame replaceAllowed = previewAt(0);
  paintAt(0);
  ok = expect(previewIsColor(replaceAllowed, 0.22F, 1.0F) &&
                  appState.facade.document().voxelField().materialAt(
                      {0, 0, 0}) == cr::CreativeObjectKind::Wall &&
                  cr::creativeUndoDepth(appState.history) == 2U,
              "replace previews and recolors occupied cells") &&
       ok;

  editor.interaction.hotbar.entries[0].objectKind =
      cr::CreativeObjectKind::Ceiling;
  editor.toolSettings.materialBrushReplaceSourceKind =
      cr::CreativeObjectKind::Floor;
  syncCreativeEditorHeldItem(appState, editor);
  syncCreativeEditorQuickEdit(editor);
  editor.quickEdit.selectedIndex = editor.quickEdit.options.count - 1U;
  const CreativeEditorOverlayFrame selectiveBlocked = previewAt(0);
  const std::uint64_t revisionBeforeSelectiveBlock =
      appState.facade.document().revision();
  paintAt(0);
  const CreativeEditorOverlayFrame selectiveAllowed = previewAt(2);
  paintAt(2);
  ok = expect(editor.quickEdit.options.count == 7U &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "REPLACE SOURCE Floor" &&
                  previewIsColor(selectiveBlocked, 1.0F, 0.15F) &&
                  appState.facade.document().voxelField().materialAt(
                      {0, 0, 0}) == cr::CreativeObjectKind::Wall &&
                  revisionBeforeSelectiveBlock + 1U ==
                      appState.facade.document().revision(),
              "selective replace rejects a nonmatching occupied material") &&
       expect(previewIsColor(selectiveAllowed, 0.22F, 1.0F) &&
                  appState.facade.document().voxelField().materialAt(
                      {2, 0, 0}) == cr::CreativeObjectKind::Ceiling &&
                  cr::creativeUndoDepth(appState.history) == 3U,
              "selective replace previews and repaints only its source") &&
       ok;

  editor.interaction.hotbar.entries[0].objectKind =
      cr::CreativeObjectKind::Wall;
  editor.toolSettings.materialBrushMask =
      cr::CreativeMaterialBrushMask::Overwrite;
  editor.toolSettings.materialBrushReplaceSourceKind =
      cr::CreativeObjectKind::Unknown;
  syncCreativeEditorHeldItem(appState, editor);
  const CreativeEditorOverlayFrame overwriteOccupied = previewAt(2);
  paintAt(2);
  const CreativeEditorOverlayFrame overwriteEmpty = previewAt(4);
  paintAt(4);
  ok = expect(previewIsColor(overwriteOccupied, 0.22F, 1.0F) &&
                  previewIsColor(overwriteEmpty, 0.22F, 1.0F) &&
                  appState.facade.document().voxelField().materialAt(
                      {2, 0, 0}) == cr::CreativeObjectKind::Wall &&
                  appState.facade.document().voxelField().materialAt(
                      {4, 0, 0}) == cr::CreativeObjectKind::Wall &&
                  cr::creativeUndoDepth(appState.history) == 5U,
              "overwrite admits both occupied and empty cells") &&
       ok;

  editor.toolSettings.materialBrushMask =
      cr::CreativeMaterialBrushMask::AddOnly;
  setPlaceTarget(editor, 0);
  editor.interaction.target.voxelHit = true;
  editor.interaction.target.voxelCell = {0, 0, 0};
  editor.interaction.target.objectKind = cr::CreativeObjectKind::Wall;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Reject, true, true, false),
      now++);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Reject, false, false, true),
      now++);
  return expect(appState.facade.document().voxelField().materialAt(
                    {0, 0, 0}) == cr::CreativeObjectKind::Unknown &&
                    cr::creativeUndoDepth(appState.history) == 6U,
                "Circle erase ignores the paint occupancy mask") &&
         ok;
}

bool materialBrushCapacityRejectsWholeStamp() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 111U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cube;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::FiveCells;
  syncCreativeEditorHeldItem(appState, editor);

  setPlaceTarget(editor, 0);
  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput previewFrame;
  CreativeEditorOverlayFrame previewOverlay;
  const std::uint64_t revisionBeforePreview =
      appState.facade.document().revision();
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, previewFrame, projectionRequest,
       1280U, 720U, 0.03F, false},
      previewOverlay);
  const bool previewBounded =
      previewOverlay.materialBrushEdgeCount == 125U * 12U &&
      appState.facade.document().revision() == revisionBeforePreview;

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  setPlaceTarget(editor, 5);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, false, false),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  setPlaceTarget(editor, 10);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, false, false),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds);

  const bool bounded =
      appState.facade.document().voxelField().occupiedCellCount() == 250U &&
      editor.interaction.materialStroke.visitedCount == 250U &&
      editor.interaction.materialStroke.capacityReached &&
      editor.interaction.placementFeedback.status ==
          CreativeEditorPlacementFeedbackStatus::Rejected;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);
  return expect(previewBounded,
                "largest brush preview is bounded to 125 voxel boxes") &&
         expect(bounded,
                "brush capacity rejects an entire stamp before partial mutation") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U,
                "capacity stop still commits prior accepted stamps as one undo");
}

bool placementStrokeDeduplicatesAndRecordsOneUndo() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 101U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  setPlaceTarget(editor, 0);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, true, false), 0U);
  bool ok = expect(appState.facade.document().objectCount() == 0U &&
                       appState.facade.document()
                               .voxelField()
                               .occupiedCellCount() == 1U &&
                       editor.interaction.placementFeedback.voxelPlaced &&
                       editor.interaction.placementFeedback.voxelCell.x == 0,
                   "press places one structural voxel immediately") &&
            expect(cr::creativeUndoDepth(appState.history) == 0U &&
                       editor.interaction.materialStroke.transaction.active,
                   "history transaction remains lazy-open during hold");

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput frame;
  CreativeEditorOverlayFrame placementOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      placementOverlay);
  ok = expect(placementOverlay.placementFeedbackEdgeCount == 12U,
              "placed voxel emits one exact lime cell outline") &&
       ok;

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, false, false),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  ok = expect(appState.facade.document()
                      .voxelField()
                      .occupiedCellCount() == 1U,
              "stationary repeated cell mutates once") &&
       ok;

  setPlaceTarget(editor, 1);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, false, false),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds);
  ok = expect(appState.facade.document()
                          .voxelField()
                          .occupiedCellCount() == 2U &&
                  editor.interaction.materialStroke.acceptedMutationCount == 2U,
              "advancing hold places each new cell once") &&
       ok;

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, false, false, true),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);
  ok = expect(cr::creativeUndoDepth(appState.history) == 1U &&
                  !editor.interaction.materialStroke.repeat.active,
              "release commits exactly one undo entry") &&
       ok;
  const bool undone = undoLastEdit(appState, "test_place_stroke_undo");
  return expect(undone && appState.facade.document().objectCount() == 0U &&
                    appState.facade.document()
                            .voxelField()
                            .occupiedCellCount() == 0U,
                "one undo removes the entire placement stroke") &&
         ok;
}

bool identicalPlacementAcrossGesturesIsRejected() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 105U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  setPlaceTarget(editor, 0);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, true, false), 0U);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, false, false, true), 1U);
  bool ok = expect(appState.facade.document().objectCount() == 0U &&
                       appState.facade.document()
                               .voxelField()
                               .occupiedCellCount() == 1U &&
                       cr::creativeUndoDepth(appState.history) == 1U,
                   "first voxel placement commits one history record");

  ++editor.frameIndex;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, true, false), 2U);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, false, false, true), 3U);
  ok = expect(appState.facade.document().objectCount() == 0U &&
                  appState.facade.document()
                          .voxelField()
                          .occupiedCellCount() == 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U &&
                  editor.interaction.placementFeedback.status ==
                      CreativeEditorPlacementFeedbackStatus::Rejected,
              "identical later gesture cannot overwrite occupied voxel") &&
       ok;

  editor.interaction.placementFeedback = {};
  iggy3d::FrameInput duplicatePreview;
  attachCreativeEditorPlacementPreviews(
      editor, false, duplicatePreview, &appState.facade.document());
  ok = expect(duplicatePreview.creativePreview.itemCount == 2U &&
                  duplicatePreview.creativePreview.items[0].role ==
                      iggy3d::RenderCreativePreviewRole::PlacementInvalid,
              "occupied duplicate target remains visibly invalid") &&
       ok;

  setPlaceTarget(editor, 1);
  iggy3d::FrameInput distinctPreview;
  attachCreativeEditorPlacementPreviews(
      editor, false, distinctPreview, &appState.facade.document());
  return expect(distinctPreview.creativePreview.itemCount == 2U &&
                    distinctPreview.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::PlacementValid,
                "same material at distinct geometry remains placeable") &&
         ok;
}

bool untrackedAndEmptyStrokesFailClosed() {
  cr::CreativeAppState unidentified;
  CreativeEditorState unidentifiedEditor =
      materialEditor(cr::CreativeObjectKind::Crate);
  setPlaceTarget(unidentifiedEditor, 0);
  processCreativeMaterialStrokeFrame(
      unidentified, unidentifiedEditor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, true, false), 0U);
  bool ok = expect(unidentified.facade.document().objectCount() == 0U &&
                       unidentifiedEditor.interaction.materialStroke.visitedCount ==
                           0U &&
                       unidentifiedEditor.interaction.placementFeedback.status ==
                           CreativeEditorPlacementFeedbackStatus::Rejected,
                   "stroke rejects mutation when history cannot identify document");

  cr::CreativeAppState empty;
  installHistoryDocument(empty, 104U);
  CreativeEditorState emptyEditor =
      materialEditor(cr::CreativeObjectKind::Crate);
  processCreativeMaterialStrokeFrame(
      empty, emptyEditor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, true, false), 0U);
  processCreativeMaterialStrokeFrame(
      empty, emptyEditor,
      actionFrame(cr::CreativeWorldActionId::Secondary, false, false, true), 1U);
  return expect(empty.facade.document().objectCount() == 0U &&
                    cr::creativeUndoDepth(empty.history) == 0U &&
                    !emptyEditor.interaction.materialStroke.repeat.active &&
                    !emptyEditor.interaction.materialStroke.transaction.active,
                "empty gesture finalizes without an undo record") &&
         ok;
}

bool removalStrokeDeduplicatesObjectsAndGroupsHistory() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 102U);
  cr::CreativeDocumentCreateReceipt first =
      appState.facade.createDocumentObject(cr::CreativeObjectKind::Crate);
  cr::CreativeDocumentCreateReceipt second =
      appState.facade.createDocumentObject(cr::CreativeObjectKind::Crate);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Crate);
  editor.interaction.target.objectHit = true;
  editor.interaction.target.objectId = first.objectId;
  editor.interaction.target.objectKind = cr::CreativeObjectKind::Crate;

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Primary, true, true, false), 0U);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Primary, true, false, false),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  bool ok = expect(appState.facade.document().objectCount() == 1U &&
                       editor.interaction.materialStroke.visitedCount == 1U,
                   "stationary remove target is visited once");

  editor.interaction.target.objectId = second.objectId;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Primary, true, false, false),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Primary, false, false, true),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds + 1U);
  ok = expect(appState.facade.document().objectCount() == 0U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "advancing removal stroke commits one history record") &&
       ok;
  const bool undone = undoLastEdit(appState, "test_remove_stroke_undo");
  return expect(undone && appState.facade.document().objectCount() == 2U,
                "one undo restores the entire removal stroke") &&
         ok;
}

bool strokeCapacityStopsAndInterruptionFinalizes() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 103U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Crate);
  for (std::size_t index = 0;
       index < kCreativeMaterialStrokeVisitedCapacity; ++index) {
    setPlaceTarget(editor, static_cast<std::int32_t>(index));
    processCreativeMaterialStrokeFrame(
        appState, editor,
        actionFrame(cr::CreativeWorldActionId::Secondary, true, index == 0U,
                    false),
        index * cr::kCreativeMaterialStrokeRepeatNanoseconds);
  }
  setPlaceTarget(
      editor,
      static_cast<std::int32_t>(kCreativeMaterialStrokeVisitedCapacity));
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, false, false),
      kCreativeMaterialStrokeVisitedCapacity *
          cr::kCreativeMaterialStrokeRepeatNanoseconds);

  const bool bounded =
      appState.facade.document().objectCount() ==
          kCreativeMaterialStrokeVisitedCapacity &&
      editor.interaction.materialStroke.visitedCount ==
          kCreativeMaterialStrokeVisitedCapacity &&
      editor.interaction.materialStroke.capacityReached &&
      editor.interaction.placementFeedback.status ==
          CreativeEditorPlacementFeedbackStatus::Rejected;
  ++editor.frameIndex;
  iggy3d::FrameInput cappedPreview;
  attachCreativeEditorPlacementPreviews(editor, false, cappedPreview);
  const bool remainsInvalid =
      cappedPreview.creativePreview.itemCount == 2U &&
      cappedPreview.creativePreview.items[0].role ==
          iggy3d::RenderCreativePreviewRole::PlacementInvalid;
  finalizeCreativeMaterialStroke(appState, editor,
                                 "test_capacity_interruption");
  return expect(bounded, "257th distinct target is rejected at fixed capacity") &&
         expect(remainsInvalid,
                "capacity rejection remains red between repeat deadlines") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U &&
                    !editor.interaction.materialStroke.transaction.active,
                "interruption finalizes all bounded mutations as one undo");
}

bool connectedFillPreviewMutationCacheAndHistoryStayInParity() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 700U);
  const std::array seedEdits{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{1, 0, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{1, 1, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{5, 0, 0}, cr::CreativeObjectKind::Wall},
      cr::CreativeVoxelEdit{{0, 0, 1}, cr::CreativeObjectKind::Floor},
  };
  const cr::CreativeVoxelMutationReceipt seeded =
      appState.facade.applyVoxelEdits(seedEdits);

  CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::ConnectedFill, cr::CreativeObjectKind::Floor};
  editor.placeBrush = cr::CreativeObjectKind::Floor;
  editor.frameIndex = 20U;
  setVoxelTarget(editor, {0, 0, 0}, cr::CreativeObjectKind::Wall);
  syncCreativeEditorHeldItem(appState, editor);

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput frame;
  CreativeEditorOverlayFrame firstOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      firstOverlay);
  const bool firstPreviewCyan = std::any_of(
      firstOverlay.combinedWireLines.begin(),
      firstOverlay.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return near(line.color.r, 0.12F) && near(line.color.g, 0.92F) &&
               near(line.color.b, 1.0F);
      });
  CreativeEditorOverlayFrame cachedOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      cachedOverlay);
  const bool cacheReused =
      editor.interaction.connectedFill.refreshCount == 1U;
  const std::string initialStatus =
      creativeEditorHeldItemStatusLabel(editor);
  editor.toolOptions.open = true;
  CreativeEditorOverlayFrame blockedOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      blockedOverlay);
  editor.toolOptions.open = false;
  const CreativeEditorWorldTarget savedTarget = editor.interaction.target;
  editor.interaction.target = {};
  CreativeEditorOverlayFrame noTargetOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      noTargetOverlay);
  editor.interaction.target = savedTarget;

  const bool painted = confirmCreativeEditorHeldItem(
      appState, editor, "test_connected_fill_paint");
  const cr::CreativeVoxelField& paintedField =
      appState.facade.document().voxelField();
  const bool paintExact =
      paintedField.materialAt({0, 0, 0}) == cr::CreativeObjectKind::Floor &&
      paintedField.materialAt({1, 0, 0}) == cr::CreativeObjectKind::Floor &&
      paintedField.materialAt({1, 1, 0}) == cr::CreativeObjectKind::Floor &&
      paintedField.materialAt({5, 0, 0}) == cr::CreativeObjectKind::Wall &&
      paintedField.materialAt({0, 0, 1}) == cr::CreativeObjectKind::Floor;
  const bool onePaintUndo = cr::creativeUndoDepth(appState.history) == 1U;
  const bool paintFeedback =
      editor.interaction.placementFeedback.status ==
          CreativeEditorPlacementFeedbackStatus::Placed &&
      editor.interaction.placementFeedback.voxelPlaced &&
      sameBounds(editor.interaction.placementFeedback.voxelBounds,
                 {{0.0, 0.0, 0.0}, {2.0, 2.0, 1.0}});

  const bool paintUndone =
      undoLastEdit(appState, "test_connected_fill_paint_undo");
  setVoxelTarget(editor, {0, 0, 0}, cr::CreativeObjectKind::Wall);
  CreativeEditorOverlayFrame afterUndoOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      afterUndoOverlay);
  const bool cacheRefreshedAfterRevision =
      editor.interaction.connectedFill.refreshCount == 2U;

  const CreativeEditorConnectedFillReceipt erased =
      applyCreativeEditorConnectedFillWithHistory(
          appState, editor, CreativeConnectedFillEditKind::Erase,
          "test_connected_fill_erase");
  const cr::CreativeVoxelField& erasedField =
      appState.facade.document().voxelField();
  const bool eraseExact =
      erasedField.materialAt({0, 0, 0}) == cr::CreativeObjectKind::Unknown &&
      erasedField.materialAt({1, 0, 0}) == cr::CreativeObjectKind::Unknown &&
      erasedField.materialAt({1, 1, 0}) == cr::CreativeObjectKind::Unknown &&
      erasedField.materialAt({5, 0, 0}) == cr::CreativeObjectKind::Wall &&
      erasedField.materialAt({0, 0, 1}) == cr::CreativeObjectKind::Floor;
  const bool eraseUndone =
      undoLastEdit(appState, "test_connected_fill_erase_undo");

  editor.interaction.hotbar.entries[0].objectKind =
      cr::CreativeObjectKind::Wall;
  setVoxelTarget(editor, {0, 0, 0}, cr::CreativeObjectKind::Wall);
  CreativeEditorOverlayFrame noChangeOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      noChangeOverlay);
  const bool noChangeRed = std::any_of(
      noChangeOverlay.combinedWireLines.begin(),
      noChangeOverlay.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return near(line.color.r, 1.0F) && near(line.color.g, 0.15F) &&
               near(line.color.b, 0.12F);
      });
  const CreativeEditorConnectedFillReceipt noChange =
      applyCreativeEditorConnectedFillWithHistory(
          appState, editor, CreativeConnectedFillEditKind::Paint,
          "test_connected_fill_no_change");

  return expect(seeded.accepted,
                "connected fill fixture seeds through Facade") &&
         expect(firstOverlay.connectedFillEdgeCount == 36U &&
                    firstPreviewCyan && cacheReused &&
                    blockedOverlay.connectedFillEdgeCount == 0U &&
                    noTargetOverlay.connectedFillEdgeCount == 0U &&
                    initialStatus ==
                        "Flood | Floor | 256 CELLS | 3 CELLS | "
                        "[FILL LIMIT 256 CELLS]",
                "exact three-cell preview is cyan and cache-stable") &&
         expect(painted && paintExact && onePaintUndo && paintFeedback,
                "connected paint mutates exactly the preview as one undo") &&
         expect(paintUndone && cacheRefreshedAfterRevision,
                "undo restores voxels and invalidates the preview key") &&
         expect(erased.accepted && erased.changed &&
                    erased.changedCellCount == 3U && eraseExact &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "connected erase removes only the component") &&
         expect(eraseUndone && noChangeOverlay.connectedFillEdgeCount == 36U &&
                    noChangeRed && !noChange.accepted && !noChange.changed &&
                    noChange.reasonCode ==
                        "creative_connected_fill_no_change" &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "same-material fill previews red and creates no history");
}

bool connectedFillLimitRejectsWithoutPartialMutation() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 701U);
  std::vector<cr::CreativeVoxelEdit> seedEdits;
  seedEdits.reserve(65U);
  for (std::int32_t x = 0; x < 65; ++x) {
    seedEdits.push_back({{x, 0, 0}, cr::CreativeObjectKind::Wall});
  }
  const cr::CreativeVoxelMutationReceipt seeded =
      appState.facade.applyVoxelEdits(seedEdits);

  CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::ConnectedFill, cr::CreativeObjectKind::Floor};
  editor.toolSettings.connectedFillLimit =
      cr::CreativeConnectedFillLimit::Cells64;
  editor.frameIndex = 30U;
  setVoxelTarget(editor, {0, 0, 0}, cr::CreativeObjectKind::Wall);
  syncCreativeEditorHeldItem(appState, editor);

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput frame;
  CreativeEditorOverlayFrame overlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      overlay);
  const bool seedIsRed = std::any_of(
      overlay.combinedWireLines.begin(), overlay.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return near(line.color.r, 1.0F) && near(line.color.g, 0.15F) &&
               near(line.color.b, 0.12F);
      });
  const std::uint64_t revisionBefore =
      appState.facade.document().revision();
  const CreativeEditorConnectedFillReceipt rejected =
      applyCreativeEditorConnectedFillWithHistory(
          appState, editor, CreativeConnectedFillEditKind::Paint,
          "test_connected_fill_overflow");

  return expect(seeded.accepted && overlay.connectedFillEdgeCount == 12U &&
                    seedIsRed && editor.interaction.connectedFill.plan.status ==
                                     cr::CreativeConnectedFillStatus::
                                         CapacityExceeded,
                "over-limit preview fails closed to one red seed cell") &&
         expect(!rejected.accepted && !rejected.changed &&
                    rejected.plannedCellCount == 0U &&
                    appState.facade.document().revision() == revisionBefore &&
                    appState.facade.document().voxelField().occupiedCellCount() ==
                        65U &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "over-limit action cannot partially mutate or create history");
}

bool surfaceExtrudePreviewMutationAndRemovalStayAtomic() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 702U);
  const std::array floorEdits{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{1, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{0, 0, 1}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{1, 0, 1}, cr::CreativeObjectKind::Floor},
  };
  const cr::CreativeVoxelMutationReceipt seeded =
      appState.facade.applyVoxelEdits(floorEdits);

  CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::SurfaceExtrude,
      cr::CreativeObjectKind::Wall};
  editor.toolSettings.surfaceExtrudeDepth =
      cr::CreativeSurfaceExtrudeDepth::TwoCells;
  editor.frameIndex = 40U;
  setVoxelTarget(editor, {0, 0, 0}, cr::CreativeObjectKind::Floor);
  syncCreativeEditorHeldItem(appState, editor);

  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  cr::CreativeSpatialProjectionRequest projectionRequest;
  iggy3d::FrameInput frame;
  CreativeEditorOverlayFrame firstOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      firstOverlay);
  const bool previewCyan = std::any_of(
      firstOverlay.combinedWireLines.begin(),
      firstOverlay.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return near(line.color.r, 0.12F) && near(line.color.g, 0.82F) &&
               near(line.color.b, 1.0F);
      });
  CreativeEditorOverlayFrame cachedOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      cachedOverlay);
  const bool cacheReused =
      editor.interaction.surfaceExtrude.refreshCount == 1U;
  const std::string status = creativeEditorHeldItemStatusLabel(editor);
  editor.toolOptions.open = true;
  CreativeEditorOverlayFrame blockedOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      blockedOverlay);
  editor.toolOptions.open = false;

  iggy3d::RenderCameraFrame camera;
  camera.worldEye = {0.5F, 5.0F, 0.5F};
  camera.worldForward = {0.0F, -1.0F, 0.0F};
  camera.worldUp = {0.0F, 0.0F, -1.0F};
  const CreativeEditorPickFrame pickFrame;
  const cr::CreativeWorldActionFrame accept =
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false);
  processCreativeEditorWorldInteractionFrame(
      {appState, editor, accept, cr::kCreativeInputModifierNone, camera,
       pickFrame, iggy3d::RenderContentViewport{0, 0, 800U, 600U}, 0U, false});
  const bool extruded =
      editor.interaction.placementFeedback.status ==
      CreativeEditorPlacementFeedbackStatus::Placed;
  const cr::CreativeVoxelField& extrudedField =
      appState.facade.document().voxelField();
  bool extrusionExact = extrudedField.occupiedCellCount() == 12U;
  for (std::int32_t y : {1, 2}) {
    for (std::int32_t x : {0, 1}) {
      for (std::int32_t z : {0, 1}) {
        extrusionExact =
            extrusionExact &&
            extrudedField.materialAt({x, y, z}) ==
                cr::CreativeObjectKind::Wall;
      }
    }
  }
  const bool extrusionFeedback =
      editor.interaction.placementFeedback.status ==
          CreativeEditorPlacementFeedbackStatus::Placed &&
      sameBounds(editor.interaction.placementFeedback.voxelBounds,
                 {{0.0, 1.0, 0.0}, {2.0, 3.0, 2.0}});
  const bool oneExtrusionUndo =
      cr::creativeUndoDepth(appState.history) == 1U;
  const bool extrusionUndone =
      undoLastEdit(appState, "test_surface_extrude_undo");

  editor.toolSettings.surfaceExtrudeDepth =
      cr::CreativeSurfaceExtrudeDepth::OneCell;
  syncCreativeEditorQuickEdit(editor);
  const cr::CreativeWorldActionFrame reject =
      actionFrame(cr::CreativeWorldActionId::Reject, true, true, false);
  processCreativeEditorWorldInteractionFrame(
      {appState, editor, reject, cr::kCreativeInputModifierNone, camera,
       pickFrame, iggy3d::RenderContentViewport{0, 0, 800U, 600U}, 0U, false});
  const bool removed =
      editor.interaction.placementFeedback.status ==
      CreativeEditorPlacementFeedbackStatus::Placed;
  const cr::CreativeVoxelField& removedField =
      appState.facade.document().voxelField();
  const bool removalExact = removedField.occupiedCellCount() == 0U;
  const bool oneRemovalUndo = cr::creativeUndoDepth(appState.history) == 1U;
  const bool removalUndone =
      undoLastEdit(appState, "test_surface_remove_layer_undo");

  editor.toolSettings.surfaceExtrudeDepth =
      cr::CreativeSurfaceExtrudeDepth::TwoCells;
  syncCreativeEditorQuickEdit(editor);
  const cr::CreativeVoxelEdit blocker{{0, 2, 0},
                                      cr::CreativeObjectKind::Wall};
  const cr::CreativeVoxelMutationReceipt blockerSeeded =
      appState.facade.applyVoxelEdits(std::span{&blocker, 1U});
  setVoxelTarget(editor, {0, 0, 0}, cr::CreativeObjectKind::Floor);
  CreativeEditorOverlayFrame rejectedOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, selection, gizmo, frame, projectionRequest,
       1280U, 720U, 0.03F, false},
      rejectedOverlay);
  const bool rejectedRed = std::any_of(
      rejectedOverlay.combinedWireLines.begin(),
      rejectedOverlay.combinedWireLines.end(),
      [](const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return near(line.color.r, 1.0F) && near(line.color.g, 0.15F) &&
               near(line.color.b, 0.12F);
      });
  const std::uint64_t revisionBeforeRejected =
      appState.facade.document().revision();
  const CreativeEditorSurfaceExtrudeReceipt rejected =
      applyCreativeEditorSurfaceExtrudeWithHistory(
          appState, editor, cr::CreativeSurfaceExtrudeKind::Extrude,
          "test_surface_blocked");

  cr::CreativeGridCoord3 snappedFace{};
  const bool faceSnapped = creativeSurfaceFaceOffset(
      {-0.8, 0.2, 0.1}, snappedFace);
  cr::CreativeGridCoord3 invalidFace{};
  const bool zeroFace = creativeSurfaceFaceOffset({}, invalidFace);

  return expect(seeded.accepted &&
                    firstOverlay.surfaceExtrudeEdgeCount == 96U &&
                    previewCyan && cacheReused &&
                    blockedOverlay.surfaceExtrudeEdgeCount == 0U &&
                    status ==
                        "Extrude | Wall | 2 CELLS | 256 CELLS | "
                        "4 FACE / 8 CELLS | [DEPTH 2 CELLS]",
                "surface preview exactly matches the cached two-layer plan") &&
         expect(extruded && extrusionExact && extrusionFeedback &&
                    oneExtrusionUndo,
                "surface extrusion creates eight cells as one undo") &&
         expect(extrusionUndone && removed && removalExact && oneRemovalUndo &&
                    removalUndone,
                "PS5 Circle removes one complete exposed layer atomically") &&
         expect(blockerSeeded.accepted &&
                    rejectedOverlay.surfaceExtrudeEdgeCount == 12U &&
                    rejectedRed && !rejected.accepted && !rejected.changed &&
                    rejected.planStatus ==
                        cr::CreativeSurfaceExtrudeStatus::
                            DestinationOccupied &&
                    appState.facade.document().revision() ==
                        revisionBeforeRejected &&
                    appState.facade.document().voxelField().occupiedCellCount() ==
                        5U &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "blocked destination rejects without partial geometry or history") &&
         expect(faceSnapped && sameCell(snappedFace, {-1, 0, 0}) &&
                    !zeroFace,
                "app face conversion snaps finite dominant normals only");
}

bool heldShapeToolOwnsItsTwoCornerGesture() {
  CreativeEditorVolumeState volume;
  volume.selection.cellSize = 1.0;
  const CreativeEditorVolumeGestureReceipt unarmed =
      stepCreativeEditorVolumeGesture(
          volume, CreativeEditorVolumeGestureAction::Commit, true, {4, 2, 6});
  const CreativeEditorVolumeGestureReceipt began =
      stepCreativeEditorVolumeGesture(
          volume, CreativeEditorVolumeGestureAction::Begin, true, {1, 0, 2});
  const CreativeEditorVolumeGestureReceipt completed =
      stepCreativeEditorVolumeGesture(
          volume, CreativeEditorVolumeGestureAction::Commit, true, {4, 2, 6});
  const CreativeEditorVolumeGestureReceipt repeated =
      stepCreativeEditorVolumeGesture(
          volume, CreativeEditorVolumeGestureAction::Commit, true, {8, 8, 8});

  CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::VolumeHollow, cr::CreativeObjectKind::Wall};
  editor.placeBrush = cr::CreativeObjectKind::Wall;
  editor.toolSettings.shapeBrushKind = cr::CreativeShapeBrushKind::Cylinder;
  editor.toolSettings.shapeBrushAxis = cr::CreativeShapeBrushAxis::Z;
  editor.volume.selection = volume.selection;
  cr::CreativeAppState appState;

  return expect(!unarmed.accepted &&
                    unarmed.status ==
                        CreativeEditorVolumeGestureStatus::NotArmed &&
                    unarmed.phaseAfter ==
                        cr::CreativeVolumeSelectionPhase::Empty,
                "shape commit requires an explicit first corner") &&
         expect(began.accepted &&
                    began.status == CreativeEditorVolumeGestureStatus::Began &&
                    began.phaseAfter ==
                        cr::CreativeVolumeSelectionPhase::FirstCorner,
                "primary begins a fresh shape gesture") &&
         expect(completed.accepted &&
                    completed.status ==
                        CreativeEditorVolumeGestureStatus::Completed &&
                    completed.phaseAfter ==
                        cr::CreativeVolumeSelectionPhase::Complete &&
                    volume.selection.firstCell.x == 1 &&
                    volume.selection.firstCell.z == 2 &&
                    volume.selection.secondCell.x == 4 &&
                    volume.selection.secondCell.y == 2 &&
                    volume.selection.secondCell.z == 6,
                "secondary completes the aimed second corner") &&
         expect(!repeated.accepted &&
                    repeated.status ==
                        CreativeEditorVolumeGestureStatus::NotArmed &&
                    volume.selection.secondCell.x == 4,
                "completed gesture cannot recommit without a new primary") &&
         expect(!confirmCreativeEditorHeldItem(
                    appState, editor, "test_shape_enter_confirm"),
                "generic confirm cannot bypass the shape corner gesture") &&
         expect(creativeEditorHeldItemStatusLabel(editor) ==
                    "Hollow | CYLINDER Z | Wall | Ready",
                "held tool HUD exposes operation shape axis material and phase");
}

bool radialSelectionRearmsOnlyRightStickLook() {
  const cr::CreativeStickSignal heldDirection =
      cr::shapeCreativeControllerStick(0.8F, 0.0F);
  const cr::CreativeStickSignal neutralDrift =
      cr::shapeCreativeControllerStick(0.05F, -0.04F);
  const CreativeEditorNavigationAdmission selecting =
      admitCreativeEditorNavigation(
          cr::CreativeInputContext::ToolWheel, false, false, heldDirection,
          false, false);
  const CreativeEditorNavigationAdmission awaitingNeutral =
      admitCreativeEditorNavigation(cr::CreativeInputContext::EditorViewport,
                                    false, true, heldDirection, false, false);
  const CreativeEditorNavigationAdmission rearmed =
      admitCreativeEditorNavigation(cr::CreativeInputContext::EditorViewport,
                                    false, true, neutralDrift, false, false);
  const CreativeEditorNavigationAdmission openingWheel =
      admitCreativeEditorNavigation(cr::CreativeInputContext::EditorViewport,
                                    false, false, heldDirection, false, true);

  return expect(!selecting.navigationActive &&
                    !selecting.rightStickLookActive,
                "tool wheel selection owns both sticks while open") &&
         expect(awaitingNeutral.navigationActive &&
                    !awaitingNeutral.rightStickLookActive &&
                    !awaitingNeutral.clearRightStickLookRearm,
                "radial close keeps movement live while camera awaits neutral") &&
         expect(!neutralDrift.active && rearmed.navigationActive &&
                    rearmed.rightStickLookActive &&
                    rearmed.clearRightStickLookRearm,
                "sub-deadzone drift clears rearm and restores camera look") &&
         expect(!openingWheel.navigationActive &&
                    !openingWheel.rightStickLookActive,
                "tool wheel toggle cannot leak motion into its opening frame");
}

bool importedAssetPlacementPreviewAndDocumentStayInParity() {
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Rock);
  cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const cr::CreativeBounds sourceBounds{
      {-0.25, -0.2, -1.0}, {1.25, 1.2, 0.2}};
  const bool assetSet = cr::setCreativeHotbarAsset(
      held, "boulder_01", sourceBounds);
  setPlaceTarget(editor, 2, 0, -1);
  editor.toolSettings.placementYaw = cr::CreativePlacementYaw::Degrees90;
  const CreativeBrushPlacementAdmission admission = admitBrushPlacement(
      held, editor.interaction.target.grid,
      editor.toolSettings.placementYaw);
  const cr::CreativeBoundsMetrics planned =
      cr::measureCreativeBounds(admission.plan.authoredBounds);
  const cr::CreativeTransformedBounds resolved =
      cr::resolveCreativeTransformedBounds(admission.plan.authoredBounds,
                                           admission.plan.transform);
  const cr::CreativeDocumentCreateRequest request =
      buildBrushCreateRequest(admission.plan, 7U,
                              cr::creativeHotbarAssetId(held));

  iggy3d::FrameInput frame;
  attachCreativeEditorPlacementPreviews(editor, false, frame);
  const bool previewIdentity =
      frame.creativePreview.itemCount == 2U &&
      iggy3d::renderCreativePreviewAssetId(
          frame.creativePreview.items[0]) == "boulder_01" &&
      iggy3d::renderCreativePreviewAssetId(
          frame.creativePreview.items[1]) == "boulder_01";
  const iggy3d::Mat4& targetPreview =
      frame.creativePreview.items[0].clipFromModel;

  cr::CreativeAppState appState;
  const CreativeBrushPlacementMutationReceipt placed = applyBrushPlacement(
      appState.facade, admission.plan, 7U, cr::kInvalidObjectId,
      cr::creativeHotbarAssetId(held));
  const cr::CreativeObject* object =
      appState.facade.document().findObject(placed.objectId);
  const bool sameAssetDuplicate = creativeBrushPlacementTargetOccupied(
      appState.facade.document(), admission.plan, "boulder_01");
  const bool differentAssetDistinct = !creativeBrushPlacementTargetOccupied(
      appState.facade.document(), admission.plan, "walkway_stone_01");

  return expect(assetSet && admission.allowed && planned.valid,
                "asset hotbar entry admits normal authored placement") &&
         expect(near(static_cast<float>(planned.size.x), 1.5F) &&
                    near(static_cast<float>(planned.size.y), 1.4F) &&
                    near(static_cast<float>(planned.size.z), 1.2F),
                "asset natural dimensions replace descriptor proxy bounds") &&
         expect(resolved.valid &&
                    near(static_cast<float>(
                             admission.plan.transform.position.x),
                         2.9F) &&
                    near(static_cast<float>(
                             admission.plan.transform.position.y),
                         0.2F) &&
                    near(static_cast<float>(
                             admission.plan.transform.position.z),
                         0.0F) &&
                    near(static_cast<float>(resolved.center.x), 2.5F) &&
                    near(static_cast<float>(resolved.center.y), 0.7F) &&
                    near(static_cast<float>(resolved.center.z), -0.5F),
                "placement retains the rotated source origin under target") &&
         expect(request.assetId == "boulder_01" &&
                    sameBounds(request.bounds, admission.plan.authoredBounds) &&
                    sameTransform(request.transform,
                                  admission.plan.transform),
                "create request shares asset identity, bounds, and pivot") &&
         expect(previewIdentity &&
                    near(iggy3d::at(targetPreview, 0U, 3U), 2.5F) &&
                    near(iggy3d::at(targetPreview, 1U, 3U), 0.7F) &&
                    near(iggy3d::at(targetPreview, 2U, 3U), -0.5F),
                "target and held previews use resolved imported geometry") &&
         expect(placed.accepted && placed.objectCreated && object != nullptr &&
                    object->assetId == "boulder_01" &&
                    sameTransform(object->transform,
                                  admission.plan.transform),
                "accepted placement stores the asset reference and pivot") &&
         expect(sameAssetDuplicate && differentAssetDistinct,
                "duplicate admission distinguishes imported asset identity");
}

bool doorwaySocketPreviewPlacementAndUndoStayInParity() {
  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  const iggy3d::StaticMeshAssetCatalogEntry* frameAsset =
      catalog.find("homestead/modular/door_frame_1p5x2p46");
  const iggy3d::StaticMeshAssetCatalogEntry* leafAsset =
      catalog.find("homestead/modular/door_leaf_1p1x2p2");
  if (!expect(frameAsset != nullptr && leafAsset != nullptr,
              "doorway socket fixtures exist")) {
    return false;
  }
  const auto receiver = std::find_if(
      frameAsset->attachmentSockets.begin(),
      frameAsset->attachmentSockets.end(),
      [](const iggy3d::StaticMeshAttachmentSocket& socket) {
        return socket.role ==
               iggy3d::StaticMeshAttachmentSocketRole::Receiver;
      });
  if (!expect(receiver != frameAsset->attachmentSockets.end(),
              "doorway receiver fixture exists")) {
    return false;
  }

  cr::CreativeAppState appState;
  installHistoryDocument(appState, 140U);
  cr::CreativeDocumentCreateRequest frameRequest;
  frameRequest.kind = cr::CreativeObjectKind::Prop;
  frameRequest.name = "Socket Door Frame";
  frameRequest.assetId = frameAsset->assetId;
  frameRequest.transform.position = {2.0, 0.0, 3.0};
  frameRequest.hasTransformOverride = true;
  frameRequest.bounds = {
      {2.0 + frameAsset->boundsMin.x, frameAsset->boundsMin.y,
       3.0 + frameAsset->boundsMin.z},
      {2.0 + frameAsset->boundsMax.x, frameAsset->boundsMax.y,
       3.0 + frameAsset->boundsMax.z}};
  frameRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt frameCreated =
      appState.facade.createDocumentObject(frameRequest);
  appState.history = {};

  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Door);
  cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const cr::CreativeBounds leafBounds{
      {leafAsset->boundsMin.x, leafAsset->boundsMin.y, leafAsset->boundsMin.z},
      {leafAsset->boundsMax.x, leafAsset->boundsMax.y,
       leafAsset->boundsMax.z}};
  const bool heldSet = cr::setCreativeHotbarAsset(
      held, leafAsset->assetId, leafBounds);
  setPlaceTarget(editor, 2, 0, 3);
  editor.interaction.target.objectHit = true;
  editor.interaction.target.objectId = frameCreated.objectId;
  editor.interaction.target.objectKind = cr::CreativeObjectKind::Prop;
  editor.interaction.target.grid.targetFacts =
      cr::makeCreativePlacementTargetFacts(
          cr::CreativePlacementTargetSource::AuthoredObject,
          cr::CreativeObjectKind::Prop, frameCreated.objectId);
  editor.interaction.target.grid.faceNormal = {0.0, 0.0, 1.0};
  editor.interaction.target.grid.hitPoint = {
      2.0 + receiver->position.x, receiver->position.y,
      3.0 + receiver->position.z};
  editor.interaction.target.grid.placerForward = {0.0, 0.0, -1.0};

  const CreativeEditorPlacementResolution ready =
      resolveCreativeEditorPlacement(
          held, editor.interaction.target, editor.toolSettings.placementYaw,
          appState.facade.document(), &catalog);
  iggy3d::FrameInput readyFrame;
  attachCreativeEditorPlacementPreviews(
      editor, false, readyFrame, &appState.facade.document(), &catalog);
  CreativeEditorSelectionFrame markerSelection;
  CreativeEditorGizmoFrame markerGizmo;
  cr::CreativeSpatialProjectionRequest markerProjection;
  CreativeEditorOverlayFrame readyOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, markerSelection, markerGizmo, readyFrame,
       markerProjection, 1280U, 720U, 0.03F, false,
       cr::CreativeInputContext::EditorViewport,
       cr::CreativeControlDevice::KeyboardMouse, &catalog},
      readyOverlay);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U,
      &catalog);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, false, false, true), 1U,
      &catalog);

  const cr::CreativeObject* door = nullptr;
  for (const cr::CreativeObject& object : appState.facade.document().objects()) {
    if (object.id != frameCreated.objectId) {
      door = &object;
    }
  }
  editor.interaction.placementFeedback = {};
  const CreativeEditorPlacementResolution occupied =
      resolveCreativeEditorPlacement(
          held, editor.interaction.target, editor.toolSettings.placementYaw,
          appState.facade.document(), &catalog);
  iggy3d::FrameInput occupiedFrame;
  attachCreativeEditorPlacementPreviews(
      editor, false, occupiedFrame, &appState.facade.document(), &catalog);
  CreativeEditorOverlayFrame occupiedOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, markerSelection, markerGizmo, occupiedFrame,
       markerProjection, 1280U, 720U, 0.03F, false,
       cr::CreativeInputContext::EditorViewport,
       cr::CreativeControlDevice::KeyboardMouse, &catalog},
      occupiedOverlay);
  editor.toolOptions.open = true;
  iggy3d::FrameInput hiddenFrame;
  CreativeEditorOverlayFrame hiddenOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, markerSelection, markerGizmo, hiddenFrame,
       markerProjection, 1280U, 720U, 0.03F, false,
       cr::CreativeInputContext::ToolOptions,
       cr::CreativeControlDevice::KeyboardMouse, &catalog},
      hiddenOverlay);

  return expect(frameCreated.accepted && heldSet,
                "doorway placement setup accepted") &&
         expect(ready.socketTargeted && ready.admission.allowed &&
                    ready.admission.plan.hasAttachment &&
                    ready.admission.plan.attachmentSatisfiesCompatibility &&
                    ready.admission.plan.attachmentTargetId ==
                        frameCreated.objectId &&
                    ready.admission.plan.attachmentSocket == "door_frame",
                "door leaf plan resolves the aimed frame receiver") &&
         expect(readyFrame.creativePreview.itemCount == 2U &&
                    readyFrame.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::PlacementValid &&
                    readyOverlay.attachmentSocketMarkerEdgeCount == 3U &&
                    readyOverlay.combinedWireLines.size() >= 3U &&
                    near(readyOverlay.combinedWireLines.back().color.g, 1.0F) &&
                    near(readyOverlay.combinedWireLines.back().color.r, 0.20F),
                "available doorway receiver renders green preview and marker") &&
         expect(door != nullptr && door->kind == cr::CreativeObjectKind::Door &&
                    door->assetId == leafAsset->assetId &&
                    door->parentId == frameCreated.objectId &&
                    door->attachmentSocket == "door_frame" &&
                    sameTransform(door->transform,
                                  ready.admission.plan.transform),
                "placed door stores the exact preview transform and socket") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U,
                "socket placement records one gesture undo") &&
         expect(occupied.socketTargeted && !occupied.admission.allowed &&
                    occupied.admission.status ==
                        CreativeBrushPlacementAdmissionStatus::AttachmentOccupied &&
                    occupiedFrame.creativePreview.itemCount == 2U &&
                    occupiedFrame.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::PlacementInvalid,
                "occupied doorway receiver rejects a duplicate") &&
         expect(
                    occupiedOverlay.attachmentSocketMarkerEdgeCount == 3U &&
                    occupiedOverlay.combinedWireLines.size() >= 3U &&
                    near(occupiedOverlay.combinedWireLines.back().color.r,
                         1.0F) &&
                    near(occupiedOverlay.combinedWireLines.back().color.g,
                         0.20F),
                "occupied doorway receiver renders a red marker") &&
         expect(hiddenOverlay.attachmentSocketMarkerEdgeCount == 0U,
                "modal tool options hide attachment socket markers");
}

bool movingPlatformWaypointDwellIsBoundedAndUndoable() {
  const std::vector<cr::CreativePathPoint> metadataPath{
      {{0.0, 0.0, 0.0}, 0.5}, {{1.0, 0.0, 0.0}, 1.0}};
  const CreativeMovingPlatformPathEditPlan appended =
      planCreativeMovingPlatformPathEdit(
          metadataPath, CreativeMovingPlatformPathEditCommand::AppendAtTarget,
          {2.0, 0.0, 0.0});
  const CreativeMovingPlatformPathEditPlan moved =
      planCreativeMovingPlatformPathEdit(
          metadataPath,
          CreativeMovingPlatformPathEditCommand::MoveSelectedToTarget,
          {1.0, 1.0, 0.0}, 1U);
  const CreativeMovingPlatformPathEditPlan removed =
      planCreativeMovingPlatformPathEdit(
          appended.pathPoints,
          CreativeMovingPlatformPathEditCommand::RemoveSelected, {}, 0U);

  cr::CreativeAppState appState;
  installHistoryDocument(appState, 140U);
  const cr::CreativeDocumentCreateReceipt created =
      appState.facade.createDocumentObject(buildBrushCreateRequest(
          cr::CreativeObjectKind::MovingPlatform, {0.5F, 0.375F, 0.5F}, 1U));
  appState.history = {};
  const CreativeMovingPlatformPathEditReceipt changed =
      setCreativeMovingPlatformWaypointDwellWithUndo(
          appState, created.objectId, 1U, 1.25, "dwell_test");
  const CreativeMovingPlatformPathEditReceipt unchanged =
      setCreativeMovingPlatformWaypointDwellWithUndo(
          appState, created.objectId, 1U, 1.25, "dwell_test_noop");
  const CreativeMovingPlatformPathEditReceipt invalid =
      setCreativeMovingPlatformWaypointDwellWithUndo(
          appState, created.objectId, 1U, -0.25, "dwell_test_invalid");
  const CreativeMovingPlatformPathEditReceipt outOfRange =
      setCreativeMovingPlatformWaypointDwellWithUndo(
          appState, created.objectId,
          cr::kCreativeMovingPlatformPathPointCapacity, 0.5,
          "dwell_test_index");
  const cr::CreativeObject* edited =
      appState.facade.findObject(created.objectId);
  const bool editStored = edited != nullptr &&
                          edited->pathPoints[1].dwellSeconds == 1.25;
  const bool undoAccepted = undoLastEdit(appState, "dwell_test_undo");
  const cr::CreativeObject* undone =
      appState.facade.findObject(created.objectId);
  const bool restoredZero = undone != nullptr &&
                            undone->pathPoints[1].dwellSeconds == 0.0;
  const bool redoAccepted = redoLastEdit(appState, "dwell_test_redo");
  const cr::CreativeObject* redone =
      appState.facade.findObject(created.objectId);
  bool stopMarkerRendered = false;
  if (redone != nullptr) {
    CreativeEditorState editor;
    CreativeEditorSelectionFrame selection;
    selection.selectedId = static_cast<cr::Id>(redone->id);
    selection.selected = redone;
    selection.selectedObjectIds = {redone->id};
    selection.selectionCount = 1U;
    selection.hasSelection = true;
    CreativeEditorGizmoFrame gizmo;
    gizmo.selectedIsPathForHandles = true;
    iggy3d::FrameInput frame;
    cr::CreativeSpatialProjectionRequest projection;
    CreativeEditorOverlayFrame overlay;
    buildAndAttachCreativeEditorOverlayFrame(
        {appState, editor, selection, gizmo, frame, projection}, overlay);
    stopMarkerRendered = std::any_of(
        overlay.combinedWireLines.begin(), overlay.combinedWireLines.end(),
        [objectId = redone->id](
            const iggy3d::RenderCreativeWireframeDebugLine& line) {
          return line.objectId == objectId && near(line.color.r, 1.0F) &&
                 near(line.color.g, 0.62F) && near(line.color.b, 0.12F) &&
                 near(line.end.y - line.start.y, 0.4F) &&
                 near(line.thickness, 0.06F);
        });
  }

  return expect(appended.accepted && moved.accepted && removed.accepted &&
                    appended.pathPoints[0].dwellSeconds == 0.5 &&
                    appended.pathPoints[1].dwellSeconds == 1.0 &&
                    appended.pathPoints[2].dwellSeconds == 0.0 &&
                    moved.pathPoints[1].dwellSeconds == 1.0 &&
                    removed.pathPoints[0].dwellSeconds == 1.0,
                "route edits preserve existing dwell metadata") &&
         expect(changed.accepted && changed.changed && editStored,
                "dwell edit applies its bounded waypoint value") &&
         expect(unchanged.accepted && !unchanged.changed &&
                    unchanged.status ==
                        CreativeMovingPlatformPathEditStatus::Unchanged &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "unchanged dwell adds no history entry") &&
         expect(!invalid.accepted && !invalid.changed &&
                    invalid.status ==
                        CreativeMovingPlatformPathEditStatus::InvalidDwell &&
                    !outOfRange.accepted &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "invalid dwell and point index mutate nothing") &&
         expect(undoAccepted && restoredZero && redoAccepted &&
                    redone != nullptr &&
                    redone->pathPoints[1].dwellSeconds == 1.25,
                "one dwell history entry round-trips through undo and redo") &&
         expect(stopMarkerRendered,
                "nonzero waypoint dwell renders an authored stop marker");
}

bool movingPlatformSegmentSpeedIsBoundedAndUndoable() {
  const std::vector<cr::CreativePathPoint> metadataPath{
      {{0.0, 0.0, 0.0}, 0.5, 1.5},
      {{1.0, 0.0, 0.0}, 1.0, 0.5}};
  const CreativeMovingPlatformPathEditPlan appended =
      planCreativeMovingPlatformPathEdit(
          metadataPath, CreativeMovingPlatformPathEditCommand::AppendAtTarget,
          {2.0, 0.0, 0.0});
  const CreativeMovingPlatformPathEditPlan moved =
      planCreativeMovingPlatformPathEdit(
          metadataPath,
          CreativeMovingPlatformPathEditCommand::MoveSelectedToTarget,
          {1.0, 1.0, 0.0}, 1U);
  const CreativeMovingPlatformPathEditPlan removed =
      planCreativeMovingPlatformPathEdit(
          appended.pathPoints,
          CreativeMovingPlatformPathEditCommand::RemoveSelected, {}, 0U);

  cr::CreativeAppState appState;
  installHistoryDocument(appState, 141U);
  const cr::CreativeDocumentCreateReceipt created =
      appState.facade.createDocumentObject(buildBrushCreateRequest(
          cr::CreativeObjectKind::MovingPlatform, {0.5F, 0.375F, 0.5F}, 1U));
  appState.history = {};
  const CreativeMovingPlatformPathEditReceipt changed =
      setCreativeMovingPlatformSegmentSpeedWithUndo(
          appState, created.objectId, 0U, 2.25, "segment_speed_test");
  const CreativeMovingPlatformPathEditReceipt unchanged =
      setCreativeMovingPlatformSegmentSpeedWithUndo(
          appState, created.objectId, 0U, 2.25, "segment_speed_test_noop");
  const CreativeMovingPlatformPathEditReceipt invalid =
      setCreativeMovingPlatformSegmentSpeedWithUndo(
          appState, created.objectId, 0U, 0.0, "segment_speed_test_invalid");
  const CreativeMovingPlatformPathEditReceipt noOutgoing =
      setCreativeMovingPlatformSegmentSpeedWithUndo(
          appState, created.objectId, 1U, 2.0,
          "segment_speed_test_terminal");
  const cr::CreativeObject* edited =
      appState.facade.findObject(created.objectId);
  const bool editStored = edited != nullptr &&
                          edited->pathPoints[0].outgoingSpeedMultiplier == 2.25;
  const bool undoAccepted = undoLastEdit(appState, "segment_speed_test_undo");
  const cr::CreativeObject* undone =
      appState.facade.findObject(created.objectId);
  const bool restoredOne = undone != nullptr &&
                           undone->pathPoints[0].outgoingSpeedMultiplier == 1.0;
  const bool redoAccepted = redoLastEdit(appState, "segment_speed_test_redo");
  const cr::CreativeObject* redone =
      appState.facade.findObject(created.objectId);
  bool speedMarkerRendered = false;
  if (redone != nullptr) {
    CreativeEditorState editor;
    CreativeEditorSelectionFrame selection;
    selection.selectedId = static_cast<cr::Id>(redone->id);
    selection.selected = redone;
    selection.selectedObjectIds = {redone->id};
    selection.selectionCount = 1U;
    selection.hasSelection = true;
    CreativeEditorGizmoFrame gizmo;
    gizmo.selectedIsPathForHandles = true;
    iggy3d::FrameInput frame;
    cr::CreativeSpatialProjectionRequest projection;
    CreativeEditorOverlayFrame overlay;
    buildAndAttachCreativeEditorOverlayFrame(
        {appState, editor, selection, gizmo, frame, projection}, overlay);
    speedMarkerRendered = std::any_of(
        overlay.combinedWireLines.begin(), overlay.combinedWireLines.end(),
        [objectId = redone->id](
            const iggy3d::RenderCreativeWireframeDebugLine& line) {
          return line.objectId == objectId && near(line.color.r, 0.92F) &&
                 near(line.color.g, 0.32F) && near(line.color.b, 1.0F) &&
                 near(line.end.y - line.start.y, 0.325F) &&
                 near(line.thickness, 0.06F);
        });
  }

  return expect(appended.accepted && moved.accepted && removed.accepted &&
                    appended.pathPoints[0].outgoingSpeedMultiplier == 1.5 &&
                    appended.pathPoints[1].outgoingSpeedMultiplier == 0.5 &&
                    appended.pathPoints[2].outgoingSpeedMultiplier == 1.0 &&
                    moved.pathPoints[1].outgoingSpeedMultiplier == 0.5 &&
                    removed.pathPoints[0].outgoingSpeedMultiplier == 0.5,
                "route edits preserve existing segment speed metadata") &&
         expect(changed.accepted && changed.changed && editStored,
                "segment speed edit applies its bounded multiplier") &&
         expect(unchanged.accepted && !unchanged.changed &&
                    unchanged.status ==
                        CreativeMovingPlatformPathEditStatus::Unchanged &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "unchanged segment speed adds no history entry") &&
         expect(!invalid.accepted &&
                    invalid.status == CreativeMovingPlatformPathEditStatus::
                                          InvalidSegmentSpeed &&
                    !noOutgoing.accepted &&
                    noOutgoing.status == CreativeMovingPlatformPathEditStatus::
                                             NoOutgoingSegment &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "invalid and terminal ping-pong segment edits mutate nothing") &&
         expect(undoAccepted && restoredOne && redoAccepted &&
                    redone != nullptr &&
                    redone->pathPoints[0].outgoingSpeedMultiplier == 2.25,
                "one segment speed edit round-trips through undo and redo") &&
         expect(speedMarkerRendered,
                "custom segment speed renders an authored route marker");
}

bool movingPlatformRouteQuickEditIsBoundedAndUndoable() {
  const std::vector<cr::CreativePathPoint> basePath{
      {{0.5, 0.375, 0.5}}, {{0.5, 3.375, 0.5}}};
  const CreativeMovingPlatformPathEditPlan appendedPlan =
      planCreativeMovingPlatformPathEdit(
          basePath, CreativeMovingPlatformPathEditCommand::AppendAtTarget,
          {4.5, 2.0, 0.5});
  const CreativeMovingPlatformPathEditPlan duplicatePlan =
      planCreativeMovingPlatformPathEdit(
          basePath, CreativeMovingPlatformPathEditCommand::AppendAtTarget,
          basePath.back().position);
  const CreativeMovingPlatformPathEditPlan minimumPlan =
      planCreativeMovingPlatformPathEdit(
          basePath, CreativeMovingPlatformPathEditCommand::RemoveLast);
  const CreativeMovingPlatformPathEditPlan nonFinitePlan =
      planCreativeMovingPlatformPathEdit(
          basePath, CreativeMovingPlatformPathEditCommand::AppendAtTarget,
          {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0});

  std::vector<cr::CreativePathPoint> fullPath;
  fullPath.reserve(cr::kCreativeMovingPlatformPathPointCapacity);
  for (std::size_t index = 0U;
       index < cr::kCreativeMovingPlatformPathPointCapacity; ++index) {
    fullPath.push_back({{static_cast<double>(index), 0.0, 0.0}});
  }
  const CreativeMovingPlatformPathEditPlan capacityPlan =
      planCreativeMovingPlatformPathEdit(
          fullPath, CreativeMovingPlatformPathEditCommand::AppendAtTarget,
          {33.0, 0.0, 0.0});

  cr::CreativeAppState appState;
  installHistoryDocument(appState, 141U);
  const cr::CreativeDocumentCreateReceipt created =
      appState.facade.createDocumentObject(buildBrushCreateRequest(
          cr::CreativeObjectKind::MovingPlatform, {0.5F, 0.375F, 0.5F}, 1U));
  const std::array selectedIds{created.objectId};
  const cr::CreativeSelectionReceipt selected =
      appState.facade.selectTargets(selectedIds, created.objectId);

  CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::ObjectMove,
      cr::CreativeObjectKind::Unknown};
  syncCreativeEditorHeldItem(appState, editor);
  syncCreativeMovingPlatformPathEditState(
      appState, editor.interaction.movingPlatformPathEdit);
  setPlaceTarget(editor, 4, 2, 0);

  const cr::CreativeObject* beforeAppend =
      appState.facade.findObject(created.objectId);
  const CreativeMovingPlatformPathTargetPlan targetPreview =
      planCreativeMovingPlatformPathTarget(
          beforeAppend, editor.interaction.target.grid.valid,
          editor.interaction.target.grid.placementAnchor);
  cr::CreativeObject capacityPreviewObject =
      beforeAppend != nullptr ? *beforeAppend : cr::CreativeObject{};
  capacityPreviewObject.pathPoints = fullPath;
  const CreativeMovingPlatformPathTargetPlan capacityTargetPreview =
      planCreativeMovingPlatformPathTarget(
          &capacityPreviewObject, true,
          editor.interaction.target.grid.placementAnchor);
  CreativeEditorSelectionFrame routeSelection;
  routeSelection.selectedId = static_cast<cr::Id>(created.objectId);
  routeSelection.selected = beforeAppend;
  routeSelection.selectedObjectIds = {created.objectId};
  routeSelection.selectionCount = 1U;
  routeSelection.hasSelection = true;
  CreativeEditorGizmoFrame routeGizmo;
  routeGizmo.selectedIsPathForHandles = true;
  routeGizmo.selectedPathHandleObjectId = created.objectId;
  cr::CreativeSpatialProjectionRequest routeProjection;
  iggy3d::FrameInput routePreviewFrame;
  CreativeEditorOverlayFrame routePreviewOverlay;
  const std::uint64_t revisionBeforeRoutePreview =
      appState.facade.document().revision();
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, routeSelection, routeGizmo, routePreviewFrame,
       routeProjection, 1280U, 720U, 0.03F, false},
      routePreviewOverlay);
  const bool routePreviewKeptRevision =
      appState.facade.document().revision() == revisionBeforeRoutePreview;
  const auto greenCandidate = std::find_if(
      routePreviewOverlay.combinedWireLines.begin(),
      routePreviewOverlay.combinedWireLines.end(),
      [&targetPreview, objectId = created.objectId](
          const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.objectId == objectId && near(line.color.r, 0.20F) &&
               near(line.color.g, 1.0F) &&
               near(line.start.x,
                    static_cast<float>(targetPreview.fromPoint.x)) &&
               near(line.start.y,
                    static_cast<float>(targetPreview.fromPoint.y)) &&
               near(line.start.z,
                    static_cast<float>(targetPreview.fromPoint.z)) &&
               near(line.end.x,
                    static_cast<float>(targetPreview.targetPoint.x)) &&
               near(line.end.y,
                    static_cast<float>(targetPreview.targetPoint.y)) &&
               near(line.end.z,
                    static_cast<float>(targetPreview.targetPoint.z));
      });
  editor.toolOptions.open = true;
  iggy3d::FrameInput hiddenRoutePreviewFrame;
  CreativeEditorOverlayFrame hiddenRoutePreviewOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, routeSelection, routeGizmo, hiddenRoutePreviewFrame,
       routeProjection, 1280U, 720U, 0.03F, false},
      hiddenRoutePreviewOverlay);
  editor.toolOptions.open = false;
  iggy3d::FrameInput desktopRoutePreviewFrame;
  CreativeEditorOverlayFrame desktopRoutePreviewOverlay;
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, routeSelection, routeGizmo,
       desktopRoutePreviewFrame, routeProjection, 1280U, 720U, 0.03F, false,
       cr::CreativeInputContext::DesktopUi},
      desktopRoutePreviewOverlay);

  cr::CreativeInputRouteResult appendInput;
  appendInput.context = cr::CreativeInputContext::EditorViewport;
  appendInput.actions[0] = {cr::CreativeInputActionId::QuickEditIncrease,
                            cr::CreativeInputKey::GamepadDpadRight};
  appendInput.actionCount = 1U;
  applyCreativeEditorCommandInput(appendInput, appState, editor, {}, "route");
  const bool routeAvailableBeforeAppend =
      editor.interaction.movingPlatformPathEdit.available;
  const bool appendQueued =
      editor.interaction.movingPlatformPathEdit.pending ==
      CreativeMovingPlatformPathEditCommand::AppendAtTarget;
  const CreativeMovingPlatformPathEditReceipt appended =
      consumeCreativeMovingPlatformPathEdit(
          appState, editor.interaction.movingPlatformPathEdit, true,
          {4.5, 2.0, 0.5}, "route_append_test");
  const cr::CreativeObject* afterAppend =
      appState.facade.findObject(created.objectId);
  const std::size_t pointCountAfterAppend =
      afterAppend != nullptr ? afterAppend->pathPoints.size() : 0U;
  const cr::CreativeVec3 appendedPoint =
      afterAppend != nullptr && afterAppend->pathPoints.size() == 3U
          ? afterAppend->pathPoints.back().position
          : cr::CreativeVec3{};
  const cr::CreativeTransformedBounds appendedBounds =
      afterAppend != nullptr ? cr::resolveCreativeObjectBounds(*afterAppend)
                             : cr::CreativeTransformedBounds{};
  const cr::CreativeVec3 expectedAppendedPoint{
      4.5,
      2.0 + appendedBounds.center.y - appendedBounds.worldBounds.min.y,
      0.5};
  routeSelection.selected = afterAppend;
  const CreativeMovingPlatformPathTargetPlan duplicatePreview =
      planCreativeMovingPlatformPathTarget(
          afterAppend, editor.interaction.target.grid.valid,
          editor.interaction.target.grid.placementAnchor);
  iggy3d::FrameInput duplicateRoutePreviewFrame;
  CreativeEditorOverlayFrame duplicateRoutePreviewOverlay;
  const std::uint64_t revisionBeforeDuplicatePreview =
      appState.facade.document().revision();
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, routeSelection, routeGizmo,
       duplicateRoutePreviewFrame, routeProjection, 1280U, 720U, 0.03F,
       false},
      duplicateRoutePreviewOverlay);
  const bool duplicatePreviewKeptRevision =
      appState.facade.document().revision() == revisionBeforeDuplicatePreview;
  const bool duplicateMarkerIsRed = std::any_of(
      duplicateRoutePreviewOverlay.combinedWireLines.begin(),
      duplicateRoutePreviewOverlay.combinedWireLines.end(),
      [objectId = created.objectId](
          const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.objectId == objectId && near(line.color.r, 1.0F) &&
               near(line.color.g, 0.20F);
      });
  const bool undoAccepted = undoLastEdit(appState, "route_append_undo_test");
  const cr::CreativeObject* afterUndo =
      appState.facade.findObject(created.objectId);
  const std::size_t pointCountAfterUndo =
      afterUndo != nullptr ? afterUndo->pathPoints.size() : 0U;
  const bool redoAccepted = redoLastEdit(appState, "route_append_redo_test");
  const cr::CreativeSelectionReceipt reselected =
      appState.facade.selectTargets(selectedIds, created.objectId);

  syncCreativeMovingPlatformPathEditState(
      appState, editor.interaction.movingPlatformPathEdit);
  const bool cycleSelectedFirst = cycleCreativeMovingPlatformPathPoint(
      editor.interaction.movingPlatformPathEdit, 1);
  const bool cycleWrappedLast = cycleCreativeMovingPlatformPathPoint(
      editor.interaction.movingPlatformPathEdit, -1);
  const std::size_t wrappedPointIndex =
      editor.interaction.movingPlatformPathEdit.selectedPointIndex;
  cr::CreativeInputRouteResult squareTransformInput;
  squareTransformInput.context = cr::CreativeInputContext::EditorViewport;
  squareTransformInput.actions[0] = {
      cr::CreativeInputActionId::QuickEditNext,
      cr::CreativeInputKey::GamepadWest};
  squareTransformInput.actionCount = 1U;
  applyCreativeEditorCommandInput(squareTransformInput, appState, editor, {},
                                  "route");
  const bool squareKeptTransformDistinct =
      editor.transform.active &&
      !editor.interaction.movingPlatformPathEdit.pointSelected;
  static_cast<void>(cancelCreativeEditorSelectionTransformPreview(
      editor.transform, "route_point_transform_distinction_test"));

  CreativeEditorPickFrame pointPickFrame;
  PathPointHandleHit broadHandle;
  broadHandle.objectId = created.objectId;
  broadHandle.pointIndex = 0U;
  broadHandle.aabb.minX = 370.0F;
  broadHandle.aabb.minY = 285.0F;
  broadHandle.aabb.maxX = 410.0F;
  broadHandle.aabb.maxY = 315.0F;
  broadHandle.aabb.valid = true;
  PathPointHandleHit centeredHandle;
  centeredHandle.objectId = created.objectId;
  centeredHandle.pointIndex = 1U;
  centeredHandle.aabb.minX = 396.0F;
  centeredHandle.aabb.minY = 296.0F;
  centeredHandle.aabb.maxX = 404.0F;
  centeredHandle.aabb.maxY = 304.0F;
  centeredHandle.aabb.valid = true;
  pointPickFrame.pathPointHandleHits = {broadHandle, centeredHandle};
  const PathPointHandlePickResult nearestHandle =
      pickPathPointHandleAtPixel(pointPickFrame.pathPointHandleHits,
                                 400.0F, 300.0F);
  const PathPointHandlePickResult invalidHandlePick =
      pickPathPointHandleAtPixel(
          pointPickFrame.pathPointHandleHits,
          std::numeric_limits<float>::quiet_NaN(), 300.0F);
  iggy3d::RenderCameraFrame routeCamera;
  const iggy3d::RenderContentViewport routeViewport{0, 0, 800U, 600U};
  const cr::CreativeWorldActionFrame selectPointActions =
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false);
  const std::uint64_t revisionBeforePointSelection =
      appState.facade.document().revision();
  processCreativeEditorMoveInteraction(
      {appState, editor, selectPointActions,
       cr::kCreativeInputModifierNone, routeCamera, pointPickFrame,
       routeViewport});
  const bool reticleSelectedPoint =
      editor.interaction.movingPlatformPathEdit.pointSelected &&
      editor.interaction.movingPlatformPathEdit.selectedPointIndex == 1U &&
      appState.facade.document().revision() == revisionBeforePointSelection;

  pointPickFrame.pathPointHandleHits.clear();
  setPlaceTarget(editor, 6, 1, 2);
  editor.toolSettings.moveConstraint = cr::CreativeMoveConstraint::X;
  const cr::CreativeObject* beforePointMove =
      appState.facade.findObject(created.objectId);
  const CreativeMovingPlatformPathPointTargetPlan constrainedPointTarget =
      planCreativeMovingPlatformPathPointTarget(
          beforePointMove, 1U, true,
          editor.interaction.target.grid.placementAnchor,
          editor.toolSettings.moveConstraint);
  const CreativeMovingPlatformPathPointTargetPlan invalidPointTarget =
      planCreativeMovingPlatformPathPointTarget(
          beforePointMove, cr::kCreativeMovingPlatformPathPointCapacity,
          true, editor.interaction.target.grid.placementAnchor,
          cr::CreativeMoveConstraint::Free);
  const std::uint64_t undoDepthBeforePointMove =
      cr::creativeUndoDepth(appState.history);
  processCreativeEditorMoveInteraction(
      {appState, editor, selectPointActions,
       cr::kCreativeInputModifierNone, routeCamera, pointPickFrame,
       routeViewport});
  const cr::CreativeObject* afterPointMove =
      appState.facade.findObject(created.objectId);
  const bool pointMoveApplied =
      constrainedPointTarget.moveAllowed && afterPointMove != nullptr &&
      afterPointMove->pathPoints.size() == 3U &&
      sameVec3(afterPointMove->pathPoints[1].position,
               constrainedPointTarget.targetPoint) &&
      sameVec3({0.0, 0.0, afterPointMove->pathPoints[1].position.z},
               {0.0, 0.0,
                constrainedPointTarget.fromPoint.z}) &&
      cr::creativeUndoDepth(appState.history) ==
          undoDepthBeforePointMove + 1U &&
      editor.interaction.movingPlatformPathEdit.pointSelected &&
      editor.interaction.movingPlatformPathEdit.selectedPointIndex == 1U;
  const std::vector<cr::CreativePathPoint> movedPathPoints =
      afterPointMove != nullptr ? afterPointMove->pathPoints
                                : std::vector<cr::CreativePathPoint>{};

  routeSelection.selected = afterPointMove;
  const CreativeEditorGizmoFrame pointGizmo =
      buildCreativeEditorGizmoFrame(
          routeSelection, editor.interaction.movingPlatformPathEdit,
          routeCamera, 800U, 600U, 1.0F);
  setPlaceTarget(editor, 7, 2, 3);
  const CreativeMovingPlatformPathPointTargetPlan nextPointTarget =
      planCreativeMovingPlatformPathPointTarget(
          afterPointMove, 1U, true,
          editor.interaction.target.grid.placementAnchor,
          editor.toolSettings.moveConstraint);
  iggy3d::FrameInput selectedPointPreviewFrame;
  CreativeEditorOverlayFrame selectedPointPreviewOverlay;
  const std::uint64_t revisionBeforePointPreview =
      appState.facade.document().revision();
  buildAndAttachCreativeEditorOverlayFrame(
      {appState, editor, routeSelection, pointGizmo,
       selectedPointPreviewFrame, routeProjection, 800U, 600U, 0.03F,
       false},
      selectedPointPreviewOverlay);
  const bool selectedPointPreviewKeptRevision =
      appState.facade.document().revision() == revisionBeforePointPreview;
  const bool selectedHandleIsYellow = std::any_of(
      selectedPointPreviewOverlay.combinedWireLines.begin(),
      selectedPointPreviewOverlay.combinedWireLines.end(),
      [objectId = created.objectId](
          const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.objectId == objectId && near(line.color.r, 1.0F) &&
               near(line.color.g, 0.92F) && near(line.color.b, 0.20F) &&
               near(line.thickness, 0.06F);
      });
  const bool selectedTargetIsGreen = std::any_of(
      selectedPointPreviewOverlay.combinedWireLines.begin(),
      selectedPointPreviewOverlay.combinedWireLines.end(),
      [&nextPointTarget, objectId = created.objectId](
          const iggy3d::RenderCreativeWireframeDebugLine& line) {
        return line.objectId == objectId && near(line.color.r, 0.20F) &&
               near(line.color.g, 1.0F) &&
               near(line.start.x,
                    static_cast<float>(nextPointTarget.fromPoint.x)) &&
               near(line.start.y,
                    static_cast<float>(nextPointTarget.fromPoint.y)) &&
               near(line.start.z,
                    static_cast<float>(nextPointTarget.fromPoint.z)) &&
               near(line.end.x,
                    static_cast<float>(nextPointTarget.targetPoint.x)) &&
               near(line.end.y,
                    static_cast<float>(nextPointTarget.targetPoint.y)) &&
               near(line.end.z,
                    static_cast<float>(nextPointTarget.targetPoint.z));
      });
  const std::string selectedPointStatus =
      creativeEditorHeldItemStatusLabel(editor);

  const cr::CreativeWorldActionFrame cancelPointActions =
      actionFrame(cr::CreativeWorldActionId::Reject, true, true, false);
  const std::uint64_t revisionBeforePointCancel =
      appState.facade.document().revision();
  processCreativeEditorMoveInteraction(
      {appState, editor, cancelPointActions,
       cr::kCreativeInputModifierNone, routeCamera, pointPickFrame,
       routeViewport});
  const bool pointCancelWasNonMutating =
      !editor.interaction.movingPlatformPathEdit.pointSelected &&
      appState.facade.document().revision() == revisionBeforePointCancel;

  static_cast<void>(selectCreativeMovingPlatformPathPoint(
      editor.interaction.movingPlatformPathEdit, 1U));
  cr::CreativeInputRouteResult removeSelectedInput;
  removeSelectedInput.context = cr::CreativeInputContext::EditorViewport;
  removeSelectedInput.actions[0] = {
      cr::CreativeInputActionId::QuickEditDecrease,
      cr::CreativeInputKey::GamepadDpadLeft};
  removeSelectedInput.actionCount = 1U;
  applyCreativeEditorCommandInput(removeSelectedInput, appState, editor, {},
                                  "route");
  const bool removeSelectedQueued =
      editor.interaction.movingPlatformPathEdit.pending ==
      CreativeMovingPlatformPathEditCommand::RemoveSelected;
  const CreativeMovingPlatformPathEditReceipt removedSelected =
      consumeCreativeMovingPlatformPathEdit(
          appState, editor.interaction.movingPlatformPathEdit, false, {},
          "route_remove_selected_test");
  const cr::CreativeObject* afterSelectedRemoval =
      appState.facade.findObject(created.objectId);
  const bool selectedRemovalKeptNeighbors =
      movedPathPoints.size() == 3U && afterSelectedRemoval != nullptr &&
      afterSelectedRemoval->pathPoints.size() == 2U &&
      sameVec3(afterSelectedRemoval->pathPoints.front().position,
               movedPathPoints.front().position) &&
      sameVec3(afterSelectedRemoval->pathPoints.back().position,
               movedPathPoints.back().position);
  const bool selectedRemovalUndoAccepted =
      undoLastEdit(appState, "route_remove_selected_undo_test");
  const cr::CreativeSelectionReceipt reselectedAfterPointUndo =
      appState.facade.selectTargets(selectedIds, created.objectId);
  syncCreativeMovingPlatformPathEditState(
      appState, editor.interaction.movingPlatformPathEdit);
  const cr::CreativeObject* afterSelectedRemovalUndo =
      appState.facade.findObject(created.objectId);
  const bool selectedRemovalUndoRestored =
      afterSelectedRemovalUndo != nullptr &&
      afterSelectedRemovalUndo->pathPoints.size() == 3U &&
      editor.interaction.movingPlatformPathEdit.pointCount == 3U;
  static_cast<void>(clearCreativeMovingPlatformPathPointSelection(
      editor.interaction.movingPlatformPathEdit));

  cr::CreativeInputRouteResult removeInput;
  removeInput.context = cr::CreativeInputContext::EditorViewport;
  removeInput.actions[0] = {cr::CreativeInputActionId::QuickEditDecrease,
                            cr::CreativeInputKey::GamepadDpadLeft};
  removeInput.actionCount = 1U;
  applyCreativeEditorCommandInput(removeInput, appState, editor, {}, "route");
  const CreativeMovingPlatformPathEditReceipt removed =
      consumeCreativeMovingPlatformPathEdit(
          appState, editor.interaction.movingPlatformPathEdit, false, {},
          "route_remove_test");
  const std::uint64_t undoDepthBeforeMinimum =
      cr::creativeUndoDepth(appState.history);
  applyCreativeEditorCommandInput(removeInput, appState, editor, {}, "route");
  const CreativeMovingPlatformPathEditReceipt minimum =
      consumeCreativeMovingPlatformPathEdit(
          appState, editor.interaction.movingPlatformPathEdit, false, {},
          "route_minimum_test");
  const cr::CreativeObject* finalObject =
      appState.facade.findObject(created.objectId);

  return expect(appendedPlan.accepted && appendedPlan.changed &&
                    appendedPlan.pathPoints.size() == 3U &&
                    sameVec3(appendedPlan.pathPoints.back().position,
                             {4.5, 2.0, 0.5}),
                "moving platform route plan appends the aimed point") &&
         expect(!duplicatePlan.accepted &&
                    duplicatePlan.status ==
                        CreativeMovingPlatformPathEditStatus::DuplicateTarget,
                "moving platform route rejects a duplicate endpoint") &&
         expect(!minimumPlan.accepted &&
                    minimumPlan.status ==
                        CreativeMovingPlatformPathEditStatus::MinimumPointCount,
                "moving platform route preserves its two-point minimum") &&
         expect(!nonFinitePlan.accepted &&
                    nonFinitePlan.status ==
                        CreativeMovingPlatformPathEditStatus::InvalidTarget,
                "moving platform route rejects a non-finite target") &&
         expect(!capacityPlan.accepted &&
                    capacityPlan.status ==
                        CreativeMovingPlatformPathEditStatus::CapacityReached,
                "moving platform route enforces its fixed capacity") &&
         expect(created.accepted && selected.accepted &&
                    routeAvailableBeforeAppend && appendQueued,
                "Object Move D-pad right queues route append semantically") &&
         expect(targetPreview.visible && targetPreview.appendAllowed &&
                    targetPreview.segmentVisible &&
                    capacityTargetPreview.visible &&
                    !capacityTargetPreview.appendAllowed &&
                    capacityTargetPreview.status ==
                        CreativeMovingPlatformPathEditStatus::CapacityReached &&
                    routePreviewOverlay.movingPlatformPathPreviewEdgeCount ==
                        13U &&
                    greenCandidate !=
                        routePreviewOverlay.combinedWireLines.end() &&
                    routePreviewKeptRevision,
                "aiming shows a green route segment and endpoint marker") &&
         expect(hiddenRoutePreviewOverlay
                            .movingPlatformPathPreviewEdgeCount == 0U &&
                    desktopRoutePreviewOverlay
                            .movingPlatformPathPreviewEdgeCount == 0U,
                "modal and desktop UI ownership hide the route target preview") &&
         expect(appended.accepted && appended.changed &&
                    appended.status ==
                        CreativeMovingPlatformPathEditStatus::Applied &&
                    pointCountAfterAppend == 3U &&
                    appendedBounds.valid &&
                    sameVec3(appendedPoint, expectedAppendedPoint) &&
                    cr::creativeUndoDepth(appState.history) >= 1U,
                "queued route append centers the platform above the snapped "
                "anchor and records history") &&
         expect(duplicatePreview.visible && !duplicatePreview.appendAllowed &&
                    !duplicatePreview.segmentVisible &&
                    duplicatePreview.status ==
                        CreativeMovingPlatformPathEditStatus::DuplicateTarget &&
                    duplicateRoutePreviewOverlay
                            .movingPlatformPathPreviewEdgeCount == 12U &&
                    duplicateMarkerIsRed && duplicatePreviewKeptRevision,
                "duplicate aim shows only a red endpoint marker without "
                "mutating the document") &&
         expect(undoAccepted && pointCountAfterUndo == 2U && redoAccepted &&
                    reselected.accepted,
                "route append participates in undo and redo") &&
         expect(cycleSelectedFirst && cycleWrappedLast &&
                    wrappedPointIndex == 2U &&
                    squareKeptTransformDistinct,
                "D-pad point cycling wraps the route while Square remains "
                "the distinct Transform command") &&
         expect(nearestHandle.hit &&
                    nearestHandle.objectId == created.objectId &&
                    nearestHandle.pointIndex == 1U &&
                    !invalidHandlePick.hit && reticleSelectedPoint,
                "center-reticle picking selects the nearest route handle "
                "without mutating the document") &&
         expect(pointMoveApplied &&
                    invalidPointTarget.status ==
                        CreativeMovingPlatformPathEditStatus::InvalidPointIndex,
                "X moves one selected point through the constrained route "
                "planner and records one undo entry") &&
         expect(near(pointGizmo.center.x,
                     static_cast<float>(constrainedPointTarget.targetPoint.x)) &&
                    near(pointGizmo.center.y,
                         static_cast<float>(
                             constrainedPointTarget.targetPoint.y)) &&
                    near(pointGizmo.center.z,
                         static_cast<float>(
                             constrainedPointTarget.targetPoint.z)) &&
                    nextPointTarget.moveAllowed &&
                    selectedHandleIsYellow && selectedTargetIsGreen &&
                    selectedPointPreviewOverlay
                            .movingPlatformPathPreviewEdgeCount == 13U &&
                    selectedPointPreviewKeptRevision,
                "selected route point owns the gizmo, yellow handle, and "
                "non-mutating green target preview") &&
         expect(selectedPointStatus.find("EDIT 2/3") !=
                        std::string::npos &&
                    pointCancelWasNonMutating,
                "route status identifies the point and Circle cancels only "
                "the point edit") &&
         expect(removeSelectedQueued && removedSelected.accepted &&
                    removedSelected.changed &&
                    removedSelected.pointCountBefore == 3U &&
                    removedSelected.pointCountAfter == 2U &&
                    selectedRemovalKeptNeighbors &&
                    selectedRemovalUndoAccepted &&
                    reselectedAfterPointUndo.accepted &&
                    selectedRemovalUndoRestored,
                "D-pad left deletes the selected route point and undo "
                "restores the route") &&
         expect(removed.accepted && removed.changed &&
                    removed.pointCountBefore == 3U &&
                    removed.pointCountAfter == 2U && finalObject != nullptr &&
                    finalObject->pathPoints.size() == 2U,
                "Object Move D-pad left removes the last route point") &&
         expect(minimum.requested && !minimum.accepted && !minimum.changed &&
                    minimum.status ==
                        CreativeMovingPlatformPathEditStatus::MinimumPointCount &&
                    cr::creativeUndoDepth(appState.history) ==
                        undoDepthBeforeMinimum,
                "minimum-point rejection does not create history") &&
         expect(creativeEditorHeldItemStatusLabel(editor).find(
                    "ROUTE 2 POINTS") != std::string::npos,
                "Object Move status exposes the selected route point count");
}

}  // namespace

int main() {
  bool ok = true;
  ok = placementPlanMatchesEveryCreateRequest() && ok;
  ok = standaloneBrushPalettePopulatesEveryCatalogCategory() && ok;
  ok = placementAdmissionOwnsPreviewAndExecutionTruth() && ok;
  ok = semanticCompatibilityOwnsPreviewMutationAndHistory() && ok;
  ok = verticalSurfacePlacementFollowsTheAimedFace() && ok;
  ok = surfaceFramePlacementFollowsExactNormalsAndKeepsUprightPropsUpright() &&
       ok;
  ok = quickEditOrientationFeedsPreviewAndCreatePlan() && ok;
  ok = toolOptionsFollowTheRequestedMaterialEntry() && ok;
  ok = toolOptionsActivateSymmetryPivotCommands() && ok;
  ok = materialBrushPresetsFollowHotbarSlots() && ok;
  ok = heldItemLifecyclePreservesCompatibleDraftsAndCancelsOthers() && ok;
  ok = continuousGestureOwnerIsExclusive() && ok;
  ok = previewFrameUsesWorldTargetAndViewHeldTransforms() && ok;
  ok = generatedTraversalPreviewsUseRoomGeometryProfiles() && ok;
  ok = previewHidesForEveryBlockingSurface() && ok;
  ok = quickEditHudHighlightsTheActiveSetting() && ok;
  ok = previewsDoNotAffectRoomGeometrySignature() && ok;
  ok = roomGeometryRendersStoredEulerRadians() && ok;
  ok = materialAimMovementDoesNotChangeUploadSignature() && ok;
  ok = placementGridDotsRenderOnlyTheActiveDepthLayer() && ok;
  ok = lockedPlacementPlaneFeedsPreviewAdmissionAndMutation() && ok;
  ok = authoredAnchorFeedsPreviewAdmissionAndMutation() && ok;
  ok = shapeVolumePreviewsStayBoundedAndFailClosed() && ok;
  ok = editorVolumeBudgetRejectsBeforeMutation() && ok;
  ok = sceneCacheRefreshesOnlyOnDocumentRevision() && ok;
  ok = activeVolumeSelectionRebindsToLoadedDocumentGrid() && ok;
  ok = objectBoundPlacementAnchorsFollowRotatedPickGeometry() && ok;
  ok = worldTargetPicksVoxelBeforeGround() && ok;
  ok = worldTargetMarksEmptyPlaneProvenance() && ok;
  ok = worldTargetSeparatesVoxelStorageFromAuthoredSnap() && ok;
  ok = worldTargetPicksDerivedTerrainAndPreservesVoxelTiePriority() && ok;
  ok = removalStrokeDeletesVoxelAndGroupsHistory() && ok;
  ok = gamepadAcceptPlacesAndRejectRemoves() && ok;
  ok = structuralSpanPreviewPlacementCancelAndUndoStayInParity() && ok;
  ok = structuralSpanEndpointEditPreviewsCommitsAndCancelsAtomically() && ok;
  ok = materialBrushPaintsErasesPreviewsAndGroupsHistory() && ok;
  ok = materialBrushShellPreviewMatchesMutationAndUndo() && ok;
  ok = materialBrushCylinderAxisDrivesPreviewAndMutation() && ok;
  ok = materialBrushPlaneGuideStaysAnchoredForTheGesture() && ok;
  ok = materialBrushLineGuideConstrainsPathAndRendersAxis() && ok;
  ok = materialBrushSymmetryUsesGesturePivotPreviewAndHistory() && ok;
  ok = materialBrushLockedPivotAvoidsCenterMutationAndPersists() && ok;
  ok = materialBrushInterpolatesDiagonalsAndBreaksOnTargetLoss() && ok;
  ok = materialBrushInterpolatesEraseSweep() && ok;
  ok = materialBrushMasksMatchPreviewAndMutation() && ok;
  ok = materialBrushCapacityRejectsWholeStamp() && ok;
  ok = placementStrokeDeduplicatesAndRecordsOneUndo() && ok;
  ok = identicalPlacementAcrossGesturesIsRejected() && ok;
  ok = untrackedAndEmptyStrokesFailClosed() && ok;
  ok = removalStrokeDeduplicatesObjectsAndGroupsHistory() && ok;
  ok = strokeCapacityStopsAndInterruptionFinalizes() && ok;
  ok = connectedFillPreviewMutationCacheAndHistoryStayInParity() && ok;
  ok = connectedFillLimitRejectsWithoutPartialMutation() && ok;
  ok = surfaceExtrudePreviewMutationAndRemovalStayAtomic() && ok;
  ok = heldShapeToolOwnsItsTwoCornerGesture() && ok;
  ok = radialSelectionRearmsOnlyRightStickLook() && ok;
  ok = importedAssetPlacementPreviewAndDocumentStayInParity() && ok;
  ok = doorwaySocketPreviewPlacementAndUndoStayInParity() && ok;
  ok = movingPlatformWaypointDwellIsBoundedAndUndoable() && ok;
  ok = movingPlatformSegmentSpeedIsBoundedAndUndoable() && ok;
  ok = movingPlatformRouteQuickEditIsBoundedAndUndoable() && ok;
  return ok ? 0 : 1;
}

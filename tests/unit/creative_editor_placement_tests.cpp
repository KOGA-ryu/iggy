#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorInteraction.hpp"
#include "EditorPlacement.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "render/vulkan/BufferImageResources.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <utility>

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
  editor.interaction.target = target;
}

bool placementPlanMatchesEveryCreateRequest() {
  constexpr iggy3d::Vec3 anchor{2.5F, 1.0F, -3.5F};
  bool sawPoint = false;
  bool sawLine = false;
  bool sawPath = false;
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
                                 return sameVec3(lhs.position, rhs.position);
                               }),
                "create request copies fixed plan path") &&
         ok;
    sawPoint = sawPoint ||
               plan.shapeKind == cr::CreativeObjectShapeKind::Point;
    sawLine = sawLine || plan.shapeKind == cr::CreativeObjectShapeKind::Line;
    sawPath = sawPath || plan.shapeKind == cr::CreativeObjectShapeKind::Path;
  }

  const float nan = std::numeric_limits<float>::quiet_NaN();
  const CreativeBrushPlacementPlan nonFinite =
      planBrushPlacement(cr::CreativeObjectKind::Wall, {nan, 0.0F, 0.0F});
  const CreativeBrushPlacementPlan unsupported =
      planBrushPlacement(cr::CreativeObjectKind::Unknown, {});
  const CreativeBrushPlacementPlan overflow = planBrushPlacement(
      cr::CreativeObjectKind::Wall,
      {std::numeric_limits<float>::max(), 0.0F, 0.0F});
  const cr::CreativeDocumentCreateRequest invalidRequest =
      buildBrushCreateRequest(cr::CreativeObjectKind::Unknown, {}, 1U);
  return expect(supportedCount > 0U && sawPoint && sawLine && sawPath,
                "descriptor sweep covers point line and path brushes") &&
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
         expect(!invalidRequest.hasTransformOverride &&
                    !invalidRequest.hasBoundsOverride &&
                    !invalidRequest.hasPathOverride,
                "invalid plan cannot leak create overrides") &&
         ok;
}

bool placementAdmissionOwnsPreviewAndExecutionTruth() {
  cr::CreativeGridTarget target = cr::resolveCreativeGridTargetFromHit(
      {2.25, 1.0, -3.25}, {0.0, 1.0, 0.0}, 1.0);
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

  const CreativeBrushPlacementAdmission objectReady =
      admitBrushPlacement(cr::CreativeObjectKind::Crate, target);
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
  const cr::CreativeGridTarget halfMeterTarget =
      cr::resolveCreativeGridTargetFromHit(
          {2.25, 1.0, -3.25}, {0.0, 1.0, 0.0}, 0.5);
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

bool verticalSurfacePlacementFollowsTheAimedFace() {
  cr::CreativeGridTarget target = cr::resolveCreativeGridTargetFromHit(
      {2.0, 1.0, 2.0}, {1.0, 0.0, 0.0}, 1.0);
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
  target.faceNormal = {1.0, 0.0, 0.0};
  const CreativeBrushPlacementAdmission bridge =
      admitBrushPlacement(cr::CreativeObjectKind::Bridge, target);

  const cr::CreativeTransformedBounds doorXGeometry =
      cr::resolveCreativeTransformedBounds(doorX.plan.authoredBounds,
                                           doorX.plan.transform);
  const cr::CreativeTransformedBounds doorZGeometry =
      cr::resolveCreativeTransformedBounds(doorZ.plan.authoredBounds,
                                           doorZ.plan.transform);
  const cr::CreativeTransformedBounds doorTopGeometry =
      cr::resolveCreativeTransformedBounds(doorTop.plan.authoredBounds,
                                           doorTop.plan.transform);
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
            expect(doorTop.allowed && doorTop.plan.orientationResolved &&
                       doorTop.plan.resolvedForward ==
                           cr::CreativePlacementFace::NegativeX &&
                       near(static_cast<float>(
                                doorTop.plan.transform.rotationEulerRadians.y),
                            static_cast<float>(-kHalfPi)) &&
                       doorTopGeometry.valid &&
                       near(static_cast<float>(extentX(
                                doorTopGeometry.worldBounds)),
                            0.2F) &&
                       near(static_cast<float>(extentZ(
                                doorTopGeometry.worldBounds)),
                            1.0F),
                   "top-face authored placement faces the placer") &&
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
      buildStandaloneRoomBakePreviewScene(
          appState.facade.document(), iggy3d::ProductMapMakerGridSnapshot{});
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
  return expect(receipt.accepted && receipt.objectCreated && placed != nullptr &&
                    sameBounds(placed->bounds, doorX.plan.authoredBounds) &&
                    near(static_cast<float>(
                             placed->transform.rotationEulerRadians.y),
                         static_cast<float>(kHalfPi)),
                "committed attachment keeps authored bounds and admitted yaw") &&
         expect(bakeAligned,
                "room bake carries the authored attachment yaw") &&
         expect(pickBounds.orientedBounds.has_value() &&
                    picked.objectId == receipt.objectId,
                "placed cardinal object is picked through its oriented bounds") &&
         ok;
}

bool quickEditOrientationFeedsPreviewAndCreatePlan() {
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Door);
  setPlaceTarget(editor, 2, 0, -1);
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
  bool sawPath = false;
  bool pathYawIgnored = true;
  for (const cr::CreativeObjectDescriptor& descriptor :
       cr::allObjectDescriptors()) {
    if (descriptor.shapeKind != cr::CreativeObjectShapeKind::Path ||
        !descriptorSupportsBrushPlacement(descriptor)) {
      continue;
    }
    const CreativeBrushPlacementAdmission pathDefault = admitBrushPlacement(
        descriptor.kind, editor.interaction.target.grid,
        cr::CreativePlacementYaw::Degrees0);
    const CreativeBrushPlacementAdmission pathRotated = admitBrushPlacement(
        descriptor.kind, editor.interaction.target.grid,
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
                       creativeEditorQuickEditStatusLabel(voxelEditor).empty(),
                   "voxel brushes omit inapplicable quick-edit channels") &&
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

  const cr::CreativeToolOptionList wallOptions =
      creativeEditorToolOptionsForEntry(wall, editor.toolSettings);
  const cr::CreativeToolOptionList doorOptions =
      creativeEditorToolOptionsForEntry(door, editor.toolSettings);
  const cr::CreativeToolOptionList invalidOptions =
      creativeEditorToolOptionsForEntry(invalid, editor.toolSettings);

  editor.interaction.hotbar.entries[0] = door;
  syncCreativeEditorQuickEdit(editor);
  const bool doorQuickEditReady =
      editor.quickEdit.targetEntry.objectKind == cr::CreativeObjectKind::Door &&
      creativeEditorQuickEditStatusLabel(editor) == "ORIENTATION 0 DEG";
  editor.interaction.hotbar.entries[0] = wall;
  const bool staleDoorStatusHidden =
      creativeEditorQuickEditStatusLabel(editor).empty();
  syncCreativeEditorQuickEdit(editor);

  return expect(wallOptions.count == 0U,
                "fixed-grid material does not advertise authored placement options") &&
         expect(doorOptions.count == 2U &&
                    doorOptions.ids[0] ==
                        cr::CreativeToolOptionId::PlacementYaw &&
                    doorOptions.ids[1] ==
                        cr::CreativeToolOptionId::SnapIncrement,
                "requested authored material owns its orientation and grid options") &&
         expect(invalidOptions.count == 0U,
                "invalid material option targets fail closed") &&
         expect(doorQuickEditReady && staleDoorStatusHidden &&
                    editor.quickEdit.targetEntry.objectKind ==
                        cr::CreativeObjectKind::Wall &&
                    editor.quickEdit.options.count == 0U,
                "quick edit tracks the complete material entry without stale labels");
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
  iggy3d::ProductMapMakerGridSnapshot grid;
  CreativeEditorSceneCache cache;
  bool ok = expect(refreshCreativeEditorSceneCache(cache, document, grid),
                   "first scene cache access builds") &&
            expect(cache.refreshCount == 1U,
                   "first scene cache build counted once");
  for (std::size_t frame = 0; frame < 300U; ++frame) {
    ok = expect(!refreshCreativeEditorSceneCache(cache, document, grid),
                "idle frame reuses scene cache") &&
         ok;
  }

  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  iggy3d::FrameInput previewFrame;
  for (std::int32_t aim = 0; aim < 8; ++aim) {
    setPlaceTarget(editor, aim);
    attachCreativeEditorPlacementPreviews(editor, false, previewFrame);
    ok = expect(!refreshCreativeEditorSceneCache(cache, document, grid),
                "aim movement does not refresh scene cache") &&
         ok;
  }

  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Crate;
  const cr::CreativeDocumentCreateReceipt first = document.createObject(create);
  ok = expect(first.accepted && first.changed &&
                  refreshCreativeEditorSceneCache(cache, document, grid) &&
                  cache.refreshCount == 2U,
              "accepted mutation refreshes cache once") &&
       ok;
  ok = expect(!refreshCreativeEditorSceneCache(cache, document, grid) &&
                  cache.refreshCount == 2U,
              "post-mutation idle frame reuses refreshed cache") &&
       ok;
  const cr::CreativeDocumentCreateReceipt second = document.createObject(create);
  ok = expect(second.accepted && second.changed &&
                  refreshCreativeEditorSceneCache(cache, document, grid) &&
                  cache.refreshCount == 3U,
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
                  refreshCreativeEditorSceneCache(cache, document, grid) &&
                  cache.refreshCount == 4U &&
                  cache.voxelChunkMeshBuildCount == 1U,
              "voxel batch refreshes scene cache once") &&
       expect(cache.preview.roomBake.receipt.voxelCellCount == 8U &&
                  cache.preview.roomBake.receipt.voxelChunkCount == 1U &&
                  cache.preview.roomBake.receipt.bakedVoxelCuboidCount == 1U,
              "scene cache greedily bakes one voxel cuboid") &&
       expect(!refreshCreativeEditorSceneCache(cache, document, grid) &&
                  cache.refreshCount == 4U &&
                  cache.voxelChunkMeshBuildCount == 1U,
              "post-voxel idle frame reuses scene cache") &&
       ok;

  const cr::CreativeVoxelEdit secondChunkEdit{
      {16, 0, 0}, cr::CreativeObjectKind::Floor};
  const cr::CreativeVoxelMutationReceipt secondChunkReceipt =
      document.applyVoxelEdits(std::span{&secondChunkEdit, 1U});
  return expect(secondChunkReceipt.changed &&
                    refreshCreativeEditorSceneCache(cache, document, grid) &&
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

  cr::CreativeWorldActionFrame actions;
  iggy3d::RenderCameraFrame camera;
  CreativeEditorPickFrame pickFrame;
  processCreativeEditorWorldInteractionFrame(
      {appState, editor, actions, cr::kCreativeInputModifierNone, camera,
       pickFrame, 800U, 600U, 0U, false});
  return expect(install.accepted && install.changed,
                "replacement document installs") &&
         expect(editor.volume.selection.cellSize == 2.0 &&
                    sameVec3(editor.volume.selection.origin, grid.origin),
                "active volume selection adopts loaded document grid") &&
         expect(editor.volume.selection.phase ==
                    cr::CreativeVolumeSelectionPhase::Empty,
                "grid replacement clears incompatible volume corners");
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
      document, camera, CreativeEditorPickFrame{}, 800U, 600U, 1.0);
  return expect(target.valid && target.voxelHit && !target.objectHit,
                "world target reports voxel hit") &&
         expect(target.voxelCell.x == 2 && target.voxelCell.y == 0 &&
                    target.voxelCell.z == 0,
                "world target preserves hit cell") &&
         expect(target.objectKind == cr::CreativeObjectKind::Crate,
                "world target exposes voxel material") &&
         expect(target.grid.targetCell.x == 2 &&
                    target.grid.adjacentCell.x == 1,
                "world target derives aimed and adjacent cells from face");
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
  bool ok = expect(previewOverlay.materialBrushEdgeCount ==
                           7U * kWireEdgesPerVoxel &&
                       previewOverlay.combinedWireLines.size() ==
                           previewOverlay.materialBrushEdgeCount &&
                       !previewOverlay.combinedWireLines.empty() &&
                       near(previewOverlay.combinedWireLines.front().color.r,
                            0.22F) &&
                       near(previewOverlay.combinedWireLines.front().color.g,
                            1.0F) &&
                       previewFrame.creativePreview.itemCount == 1U &&
                       previewFrame.creativePreview.items[0].role ==
                           iggy3d::RenderCreativePreviewRole::Held &&
                       appState.facade.document().revision() ==
                           revisionBeforePreview,
                   "sphere preview shows its exact seven green voxel cells") &&
            expect(editor.quickEdit.options.count == 4U &&
                       creativeEditorQuickEditStatusLabel(editor) ==
                           "BRUSH SHAPE SPHERE" &&
                       creativeEditorHeldItemStatusLabel(editor) ==
                           "Brush | Wall | SPHERE | 3 CELLS | FREE | "
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
                      "BRUSH DEPTH FREE" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditNext) &&
                  creativeEditorQuickEditStatusLabel(editor) ==
                      "BRUSH MASK OVERWRITE" &&
                  processCreativeEditorQuickEditAction(
                      editor, cr::CreativeInputActionId::QuickEditIncrease) &&
                  editor.toolSettings.materialBrushMask ==
                      cr::CreativeMaterialBrushMask::AddOnly,
              "dpad exposes plane and occupancy mask as bounded channels") &&
       ok;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::ThreeCells;
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
  ok = expect(erasePreviewOverlay.materialBrushEdgeCount ==
                      7U * kWireEdgesPerVoxel &&
                  !erasePreviewOverlay.combinedWireLines.empty() &&
                  near(erasePreviewOverlay.combinedWireLines.front().color.r,
                       1.0F) &&
                  near(erasePreviewOverlay.combinedWireLines.front().color.g,
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
                       editor.quickEdit.options.count == 5U &&
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

bool materialBrushPlaneStaysAnchoredForTheGesture() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 112U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::MaterialBrush;
  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Cube;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::ThreeCells;
  editor.toolSettings.materialBrushPlane =
      cr::CreativeMaterialBrushPlane::Y;
  syncCreativeEditorHeldItem(appState, editor);
  syncCreativeEditorQuickEdit(editor);
  setPlaceTarget(editor, 0, 0, 0);

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Accept, true, true, false), 0U);
  bool ok = expect(appState.facade.document()
                           .voxelField()
                           .occupiedCellCount() == 9U &&
                       editor.interaction.materialStroke.hasBrushPlaneAnchor &&
                       editor.interaction.materialStroke.brushConfig.plane ==
                           cr::CreativeMaterialBrushPlane::Y,
                   "first sample captures and flattens the gesture plane");

  editor.toolSettings.materialBrushShape =
      cr::CreativeMaterialBrushShape::Sphere;
  editor.toolSettings.materialBrushSize =
      cr::CreativeMaterialBrushSize::FiveCells;
  editor.toolSettings.materialBrushPlane =
      cr::CreativeMaterialBrushPlane::Free;
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
                      "3 CELLS | PLANE Y") != std::string::npos,
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
         expect(!editor.interaction.materialStroke.hasBrushPlaneAnchor,
                "release clears the gesture-local plane anchor") &&
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
  ok = expect(editor.quickEdit.options.count == 5U &&
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

}  // namespace

int main() {
  bool ok = true;
  ok = placementPlanMatchesEveryCreateRequest() && ok;
  ok = placementAdmissionOwnsPreviewAndExecutionTruth() && ok;
  ok = verticalSurfacePlacementFollowsTheAimedFace() && ok;
  ok = quickEditOrientationFeedsPreviewAndCreatePlan() && ok;
  ok = toolOptionsFollowTheRequestedMaterialEntry() && ok;
  ok = previewFrameUsesWorldTargetAndViewHeldTransforms() && ok;
  ok = previewHidesForEveryBlockingSurface() && ok;
  ok = quickEditHudHighlightsTheActiveSetting() && ok;
  ok = previewsDoNotAffectRoomGeometrySignature() && ok;
  ok = roomGeometryRendersStoredEulerRadians() && ok;
  ok = materialAimMovementDoesNotChangeUploadSignature() && ok;
  ok = shapeVolumePreviewsStayBoundedAndFailClosed() && ok;
  ok = editorVolumeBudgetRejectsBeforeMutation() && ok;
  ok = sceneCacheRefreshesOnlyOnDocumentRevision() && ok;
  ok = activeVolumeSelectionRebindsToLoadedDocumentGrid() && ok;
  ok = worldTargetPicksVoxelBeforeGround() && ok;
  ok = removalStrokeDeletesVoxelAndGroupsHistory() && ok;
  ok = gamepadAcceptPlacesAndRejectRemoves() && ok;
  ok = materialBrushPaintsErasesPreviewsAndGroupsHistory() && ok;
  ok = materialBrushCylinderAxisDrivesPreviewAndMutation() && ok;
  ok = materialBrushPlaneStaysAnchoredForTheGesture() && ok;
  ok = materialBrushInterpolatesDiagonalsAndBreaksOnTargetLoss() && ok;
  ok = materialBrushInterpolatesEraseSweep() && ok;
  ok = materialBrushMasksMatchPreviewAndMutation() && ok;
  ok = materialBrushCapacityRejectsWholeStamp() && ok;
  ok = placementStrokeDeduplicatesAndRecordsOneUndo() && ok;
  ok = identicalPlacementAcrossGesturesIsRejected() && ok;
  ok = untrackedAndEmptyStrokesFailClosed() && ok;
  ok = removalStrokeDeduplicatesObjectsAndGroupsHistory() && ok;
  ok = strokeCapacityStopsAndInterruptionFinalizes() && ok;
  ok = heldShapeToolOwnsItsTwoCornerGesture() && ok;
  ok = radialSelectionRearmsOnlyRightStickLook() && ok;
  return ok ? 0 : 1;
}

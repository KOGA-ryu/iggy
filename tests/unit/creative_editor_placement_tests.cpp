#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorInteraction.hpp"
#include "EditorPlacement.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
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
  const CreativeBrushPlacementAdmission ready =
      admitBrushPlacement(cr::CreativeObjectKind::Wall, target);

  bool ok = expect(ready.allowed &&
                       ready.status ==
                           CreativeBrushPlacementAdmissionStatus::Ready &&
                       ready.plan.valid,
                   "valid target produces one admitted placement plan") &&
            expect(toString(ready.status) == "creative_placement_ready",
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
  const cr::CreativeDocumentCreateReceipt receipt =
      placeBrushObject(appState.facade, ready.plan, 1U);
  const cr::CreativeObject* placed = appState.facade.findObject(receipt.objectId);
  const std::uint64_t countBeforeRejected =
      appState.facade.document().objectCount();
  const CreativeBrushPlacementPlan invalidPlan = planBrushPlacement(
      cr::CreativeObjectKind::Wall,
      {std::numeric_limits<float>::max(), 0.0F, 0.0F});
  const cr::CreativeDocumentCreateReceipt rejected =
      placeBrushObject(appState.facade, invalidPlan, 2U);
  return expect(receipt.accepted && receipt.objectCreated && placed != nullptr,
                "admitted plan executes successfully") &&
         expect(placed != nullptr &&
                    sameTransform(placed->transform, ready.plan.transform) &&
                    sameBounds(placed->bounds, ready.plan.authoredBounds),
                "execution consumes the admitted plan without geometry drift") &&
         expect(rejected.requested && !rejected.accepted &&
                    rejected.status ==
                        cr::CreativeDocumentCreateStatus::Rejected &&
                    rejected.reasonCode ==
                        "creative_placement_geometry_invalid" &&
                    appState.facade.document().objectCount() ==
                        countBeforeRejected,
                "invalid plan execution fails closed without mutation") &&
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
  target.faceNormal = {0.0, 1.0, 0.0};
  target.placerForward = {1.0, 0.0, 0.0};
  const CreativeBrushPlacementAdmission wallTop =
      admitBrushPlacement(cr::CreativeObjectKind::Wall, target);
  target.faceNormal = {1.0, 0.0, 0.0};
  const CreativeBrushPlacementAdmission wallRunX =
      admitBrushPlacement(cr::CreativeObjectKind::WallRunSurface, target);
  target.faceNormal = {0.0, 0.0, 1.0};
  const CreativeBrushPlacementAdmission wallRunZ =
      admitBrushPlacement(cr::CreativeObjectKind::WallRunSurface, target);
  target.faceNormal = {1.0, 0.0, 0.0};
  const CreativeBrushPlacementAdmission bridge =
      admitBrushPlacement(cr::CreativeObjectKind::Bridge, target);

  const cr::CreativeTransformedBounds wallXGeometry =
      cr::resolveCreativeTransformedBounds(wallX.plan.authoredBounds,
                                           wallX.plan.transform);
  const cr::CreativeTransformedBounds wallZGeometry =
      cr::resolveCreativeTransformedBounds(wallZ.plan.authoredBounds,
                                           wallZ.plan.transform);
  const cr::CreativeTransformedBounds wallTopGeometry =
      cr::resolveCreativeTransformedBounds(wallTop.plan.authoredBounds,
                                           wallTop.plan.transform);
  const cr::CreativeTransformedBounds wallRunXGeometry =
      cr::resolveCreativeTransformedBounds(wallRunX.plan.authoredBounds,
                                           wallRunX.plan.transform);
  const cr::CreativeTransformedBounds wallRunZGeometry =
      cr::resolveCreativeTransformedBounds(wallRunZ.plan.authoredBounds,
                                           wallRunZ.plan.transform);
  constexpr double kHalfPi = 1.57079632679489662;

  bool ok = expect(wallX.allowed && wallX.plan.orientationResolved &&
                       wallX.plan.resolvedFace ==
                           cr::CreativePlacementFace::PositiveX &&
                       wallX.plan.resolvedForward ==
                           cr::CreativePlacementFace::PositiveX &&
                       near(static_cast<float>(
                                wallX.plan.transform.rotationEulerRadians.y),
                            static_cast<float>(kHalfPi)) &&
                       wallXGeometry.valid &&
                       near(static_cast<float>(extentX(
                                wallXGeometry.worldBounds)),
                            0.25F) &&
                       near(static_cast<float>(extentZ(
                                wallXGeometry.worldBounds)),
                            4.0F),
                   "wall aimed at X face stores +90 yaw and resolves thin on X") &&
            expect(wallZ.allowed && wallZ.plan.orientationResolved &&
                       wallZ.plan.resolvedForward ==
                           cr::CreativePlacementFace::PositiveZ &&
                       near(static_cast<float>(
                                wallZ.plan.transform.rotationEulerRadians.y),
                            0.0F) &&
                       wallZGeometry.valid &&
                       near(static_cast<float>(extentX(
                                wallZGeometry.worldBounds)),
                            4.0F) &&
                       near(static_cast<float>(extentZ(
                                wallZGeometry.worldBounds)),
                            0.25F),
                   "wall aimed at Z face keeps local forward on +Z") &&
            expect(wallTop.allowed && wallTop.plan.orientationResolved &&
                       wallTop.plan.resolvedForward ==
                           cr::CreativePlacementFace::NegativeX &&
                       near(static_cast<float>(
                                wallTop.plan.transform.rotationEulerRadians.y),
                            static_cast<float>(-kHalfPi)) &&
                       wallTopGeometry.valid &&
                       near(static_cast<float>(extentX(
                                wallTopGeometry.worldBounds)),
                            0.25F) &&
                       near(static_cast<float>(extentZ(
                                wallTopGeometry.worldBounds)),
                            4.0F),
                   "top-face placement turns the object front toward the placer") &&
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
  const cr::CreativeDocumentCreateReceipt receipt =
      placeBrushObject(appState.facade, wallX.plan, 1U);
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
                                4.0F) &&
                           near(baked.roomBake.room.staticMeshes.front()
                                    .sizeMeters.z,
                                0.25F) &&
                           near(baked.roomBake.room.staticMeshes.front()
                                    .rotationEulerRadians.y,
                                static_cast<float>(kHalfPi)) &&
                           !baked.scene.room.meshes.empty() &&
                           near(baked.scene.room.meshes.front()
                                    .rotationEulerRadians.y,
                                static_cast<float>(kHalfPi)) &&
                           baked.roomBake.room.staticMeshes.front()
                               .hasWallSegment &&
                           near(baked.roomBake.room.staticMeshes.front()
                                    .wallStartMeters.x,
                                baked.roomBake.room.staticMeshes.front()
                                    .wallEndMeters.x);
  return expect(receipt.accepted && placed != nullptr &&
                    sameBounds(placed->bounds, wallX.plan.authoredBounds) &&
                    near(static_cast<float>(
                             placed->transform.rotationEulerRadians.y),
                         static_cast<float>(kHalfPi)),
                "committed vertical surface keeps authored bounds and admitted yaw") &&
         expect(bakeAligned,
                "room bake carries exact yaw and a cardinal collision segment") &&
         expect(pickBounds.orientedBounds.has_value() &&
                    picked.objectId == receipt.objectId,
                "placed cardinal object is picked through its oriented bounds") &&
         ok;
}

bool previewFrameUsesWorldTargetAndViewHeldTransforms() {
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  setPlaceTarget(editor, 2, 1, -3);
  iggy3d::FrameInput frame;
  attachCreativeEditorPlacementPreviews(editor, false, frame);

  const CreativeBrushPlacementPlan plan = planBrushPlacement(
      cr::CreativeObjectKind::Wall, {2.5F, 1.0F, -2.5F});
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
            expect(near(iggy3d::at(target, 0U, 0U), targetSizeX) &&
                       near(iggy3d::at(target, 1U, 1U), targetSizeY) &&
                       near(iggy3d::at(target, 2U, 2U), targetSizeZ),
                   "target transform preserves exact final boundary") &&
            expect(near(iggy3d::at(held, 0U, 3U), 0.42F) &&
                       near(iggy3d::at(held, 1U, 3U), -0.32F) &&
                       near(iggy3d::at(held, 2U, 3U), -0.82F),
                   "held object uses the fixed view-space position") &&
            expect(near(std::max({heldX, heldY, heldZ}), 0.32F) &&
                       std::min({heldX, heldY, heldZ}) >= 0.06F - 1.0e-4F,
                   "held proportions fit the fixed visible size envelope");

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
  ok = expect(near(targetAxisLength(0U), 4.0F) &&
                  near(targetAxisLength(2U), 0.25F) &&
                  near(iggy3d::at(sideTarget, 2U, 0U), -4.0F) &&
                  near(iggy3d::at(sideTarget, 0U, 2U), 0.25F),
              "target preview uses admitted cardinal yaw without swapping bounds") &&
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
  editor.clipboardPaste.active = true;
  ok = expect(hidden(), "clipboard preview hides placement previews") && ok;
  editor.clipboardPaste.active = false;
  attachCreativeEditorPlacementPreviews(editor, true, frame);
  ok = expect(frame.creativePreview.itemCount == 0U,
              "capture mode hides placement previews") &&
       ok;
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::ObjectSelect;
  return expect(hidden(), "non-material tool hides placement previews") && ok;
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
  return expect(second.accepted && second.changed &&
                    refreshCreativeEditorSceneCache(cache, document, grid) &&
                    cache.refreshCount == 3U,
                "each later accepted mutation refreshes exactly once") &&
         ok;
}

bool placementStrokeDeduplicatesAndRecordsOneUndo() {
  cr::CreativeAppState appState;
  installHistoryDocument(appState, 101U);
  CreativeEditorState editor = materialEditor(cr::CreativeObjectKind::Wall);
  setPlaceTarget(editor, 0);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, true, false), 0U);
  bool ok = expect(appState.facade.document().objectCount() == 1U,
                   "press places immediately") &&
            expect(cr::creativeUndoDepth(appState.history) == 0U &&
                       editor.interaction.materialStroke.transaction.active,
                   "history transaction remains lazy-open during hold");

  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, false, false),
      cr::kCreativeMaterialStrokeRepeatNanoseconds);
  ok = expect(appState.facade.document().objectCount() == 1U,
              "stationary repeated cell mutates once") &&
       ok;

  setPlaceTarget(editor, 1);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, false, false),
      2U * cr::kCreativeMaterialStrokeRepeatNanoseconds);
  ok = expect(appState.facade.document().objectCount() == 2U &&
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
  return expect(undone && appState.facade.document().objectCount() == 0U,
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
  bool ok = expect(appState.facade.document().objectCount() == 1U &&
                       cr::creativeUndoDepth(appState.history) == 1U,
                   "first placement commits one object and history record");

  ++editor.frameIndex;
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, true, true, false), 2U);
  processCreativeMaterialStrokeFrame(
      appState, editor,
      actionFrame(cr::CreativeWorldActionId::Secondary, false, false, true), 3U);
  ok = expect(appState.facade.document().objectCount() == 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U &&
                  editor.interaction.placementFeedback.status ==
                      CreativeEditorPlacementFeedbackStatus::Rejected,
              "identical later gesture cannot stack duplicate geometry") &&
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

}  // namespace

int main() {
  bool ok = true;
  ok = placementPlanMatchesEveryCreateRequest() && ok;
  ok = placementAdmissionOwnsPreviewAndExecutionTruth() && ok;
  ok = verticalSurfacePlacementFollowsTheAimedFace() && ok;
  ok = previewFrameUsesWorldTargetAndViewHeldTransforms() && ok;
  ok = previewHidesForEveryBlockingSurface() && ok;
  ok = previewsDoNotAffectRoomGeometrySignature() && ok;
  ok = roomGeometryRendersStoredEulerRadians() && ok;
  ok = materialAimMovementDoesNotChangeUploadSignature() && ok;
  ok = shapeVolumePreviewsStayBoundedAndFailClosed() && ok;
  ok = editorVolumeBudgetRejectsBeforeMutation() && ok;
  ok = sceneCacheRefreshesOnlyOnDocumentRevision() && ok;
  ok = placementStrokeDeduplicatesAndRecordsOneUndo() && ok;
  ok = identicalPlacementAcrossGesturesIsRejected() && ok;
  ok = untrackedAndEmptyStrokesFailClosed() && ok;
  ok = removalStrokeDeduplicatesObjectsAndGroupsHistory() && ok;
  ok = strokeCapacityStopsAndInterruptionFinalizes() && ok;
  ok = heldShapeToolOwnsItsTwoCornerGesture() && ok;
  return ok ? 0 : 1;
}

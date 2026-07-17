#include "EditorPreviewFrame.hpp"
#include "EditorPreviewFrameInternal.hpp"

#include <algorithm>
#include <cmath>

#include "EditorInteraction.hpp"
#include "EditorAttachmentPlacement.hpp"
#include "EditorPlacement.hpp"
#include "EditorState.hpp"
#include "EditorStructuralPlacement.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "core/math/EulerRotation.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace creative = iggy3d::creative;
using namespace iggy3d;
namespace {

static_assert(cr::kMaximumCreativeGeneratedGeometrySegmentCount ==
              kRenderCreativePreviewMaximumStairSegmentCount);

struct CreativePreviewGeometrySelection {
  RenderCreativePreviewGeometryProfile profile =
      RenderCreativePreviewGeometryProfile::Box;
  std::uint16_t proceduralSegmentCount = 0U;
  bool valid = true;
};

[[nodiscard]] CreativePreviewGeometrySelection previewGeometryFor(
    cr::CreativeObjectKind kind,
    Vec3 resolvedSize,
    bool assetBacked) noexcept {
  if (assetBacked) {
    return {};
  }
  const cr::CreativeObjectDescriptor& descriptor = cr::describeObject(kind);
  switch (descriptor.generatedGeometry.profile) {
    case cr::CreativeGeneratedGeometryProfile::DescriptorDefault:
    case cr::CreativeGeneratedGeometryProfile::SolidPrism:
    case cr::CreativeGeneratedGeometryProfile::WalkableSlab:
      return {};
    case cr::CreativeGeneratedGeometryProfile::RampWedge:
      return {RenderCreativePreviewGeometryProfile::RampWedge, 0U, true};
    case cr::CreativeGeneratedGeometryProfile::OpenFrame:
      return {RenderCreativePreviewGeometryProfile::OpenFrame, 0U, true};
    case cr::CreativeGeneratedGeometryProfile::StairSteps: {
      const std::uint16_t count = cr::creativeGeneratedGeometrySegmentCount(
          descriptor, {resolvedSize.x, resolvedSize.y, resolvedSize.z});
      return {RenderCreativePreviewGeometryProfile::StairSteps, count,
              count > 0U};
    }
  }
  return {RenderCreativePreviewGeometryProfile::Count, 0U, false};
}

[[nodiscard]] Mat4 modelMatrix(Vec3 position,
                               Vec3 rotationRadians,
                               Vec3 scale) {
  const Vec3 axisX = rotateEulerXyz({1.0F, 0.0F, 0.0F}, rotationRadians);
  const Vec3 axisY = rotateEulerXyz({0.0F, 1.0F, 0.0F}, rotationRadians);
  const Vec3 axisZ = rotateEulerXyz({0.0F, 0.0F, 1.0F}, rotationRadians);
  Mat4 matrix = identityMat4();
  matrix.m[0] = axisX.x * scale.x;
  matrix.m[4] = axisX.y * scale.x;
  matrix.m[8] = axisX.z * scale.x;
  matrix.m[1] = axisY.x * scale.y;
  matrix.m[5] = axisY.y * scale.y;
  matrix.m[9] = axisY.z * scale.y;
  matrix.m[2] = axisZ.x * scale.z;
  matrix.m[6] = axisZ.y * scale.z;
  matrix.m[10] = axisZ.z * scale.z;
  matrix.m[3] = position.x;
  matrix.m[7] = position.y;
  matrix.m[11] = position.z;
  return matrix;
}

[[nodiscard]] bool previewBoundsTransform(
    const creative::CreativeBounds& bounds,
    float inset,
    Vec3& center,
    Vec3& size) {
  const creative::CreativeBoundsMetrics metrics =
      creative::measureCreativeBounds(bounds);
  const creative::CreativeCoreVec3Conversion coreCenter =
      creative::creativeVec3ToCoreChecked(metrics.center);
  const creative::CreativeCoreVec3Conversion coreSize =
      creative::creativeVec3ToCoreChecked(metrics.size);
  if (!metrics.valid || !coreCenter.converted || !coreSize.converted ||
      !std::isfinite(inset)) {
    return false;
  }
  center = coreCenter.value;
  size = coreSize.value * inset;
  return isFinite(size) && size.x > 0.0F &&
         size.y > 0.0F && size.z > 0.0F;
}

[[nodiscard]] bool previewPlanTransform(
    const CreativeBrushPlacementPlan& plan,
    const creative::CreativeBounds& bounds,
    float inset,
    Vec3& center,
    Vec3& size,
    Vec3& rotation) {
  if (!plan.valid || !plan.hasTransformOverride) {
    rotation = {};
    return previewBoundsTransform(bounds, inset, center, size);
  }
  const creative::CreativeTransformedBounds resolved =
      creative::resolveCreativeTransformedBounds(bounds, plan.transform);
  const creative::CreativeCoreVec3Conversion coreCenter =
      creative::creativeVec3ToCoreChecked(resolved.center);
  const creative::CreativeCoreVec3Conversion coreSize =
      creative::creativeVec3ToCoreChecked(resolved.size);
  const creative::CreativeCoreVec3Conversion coreRotation =
      creative::creativeVec3ToCoreChecked(resolved.rotationEulerRadians);
  if (!resolved.valid || !coreCenter.converted || !coreSize.converted ||
      !coreRotation.converted || !std::isfinite(inset)) {
    return false;
  }
  center = coreCenter.value;
  size = coreSize.value * inset;
  rotation = coreRotation.value;
  return isFinite(size) && size.x > 0.0F && size.y > 0.0F && size.z > 0.0F;
}

void appendCreativePreview(RenderCreativePreviewFrame& previews,
                           RenderCreativePreviewRole role,
                           const Mat4& clipFromModel,
                           bool includePathWireframe = false,
                           std::string_view assetId = {},
                           RenderCreativePreviewGeometryProfile geometryProfile =
                               RenderCreativePreviewGeometryProfile::Box,
                           std::uint16_t proceduralSegmentCount = 0U) {
  if (previews.itemCount >= previews.items.size()) {
    return;
  }
  RenderCreativePreviewItem& item = previews.items[previews.itemCount++];
  item = {};
  item.role = role;
  item.clipFromModel = clipFromModel;
  item.includePathWireframe = includePathWireframe;
  item.geometryProfile = geometryProfile;
  item.proceduralSegmentCount = proceduralSegmentCount;
  static_cast<void>(setRenderCreativePreviewAssetId(item, assetId));
}

void appendMovingPlatformRoutePreview(
    const CreativeEditorState& editor,
    bool captureMode,
    FrameInput& frame,
    const cr::CreativeDocument* document) {
  const CreativeMovingPlatformPreviewState& preview =
      editor.movingPlatformPreview;
  if (captureMode || document == nullptr || !preview.available ||
      !preview.visible || preview.documentId != document->id()) {
    return;
  }
  const cr::CreativeObject* object = document->findObject(preview.objectId);
  if (object == nullptr || !object->visible ||
      object->kind != cr::CreativeObjectKind::MovingPlatform) {
    return;
  }
  const cr::CreativeTransformedBounds resolved =
      cr::resolveCreativeObjectBounds(*object);
  const cr::CreativeCoreVec3Conversion center =
      cr::creativeVec3ToCoreChecked(resolved.center);
  const cr::CreativeCoreVec3Conversion size =
      cr::creativeVec3ToCoreChecked(resolved.size);
  const cr::CreativeCoreVec3Conversion rotation =
      cr::creativeVec3ToCoreChecked(resolved.rotationEulerRadians);
  if (!resolved.valid || !center.converted || !size.converted ||
      !rotation.converted || !isFinite(preview.runtimeState.positionMeters)) {
    return;
  }
  const Vec3 routeOffset = preview.runtimeState.positionMeters -
                           preview.definition.originPositionMeters;
  appendCreativePreview(
      frame.creativePreview,
      RenderCreativePreviewRole::MovingPlatformRoute,
      frame.camera.clipFromWorld *
          modelMatrix(center.value + routeOffset, rotation.value, size.value),
      false, object->assetId);
}

}  // namespace

bool creativePreviewBoundsTransform(
    const cr::CreativeBounds& bounds,
    float insetScale,
    Vec3& center,
    Vec3& size) noexcept {
  return previewBoundsTransform(bounds, insetScale, center, size);
}

void attachCreativeEditorPlacementPreviews(
    const CreativeEditorState& editor,
    bool captureMode,
    FrameInput& frame,
    const cr::CreativeDocument* document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog,
    const CreativePlacementClearanceCache* clearanceCache) {
  frame.creativePreview = {};
  appendMovingPlatformRoutePreview(editor, captureMode, frame, document);
  const bool modalOpen = editor.catalog.model.open ||
                         editor.catalog.toolWheel.open ||
                         editor.toolOptions.open ||
                         editor.assetReplacement.active ||
                         editor.transform.active;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (captureMode || modalOpen) {
    return;
  }
  const CreativeEditorStructuralSpanEditState& structuralEdit =
      editor.interaction.structuralSpanEdit;
  if (held.kind == cr::CreativeHeldItemKind::ObjectMove &&
      structuralEdit.active) {
    const CreativeBrushPlacementPlan editPlan =
        creativeEditorStructuralSpanEditPreviewPlan(structuralEdit);
    Vec3 center{};
    Vec3 size{};
    Vec3 rotation{};
    if (editPlan.valid &&
        previewPlanTransform(editPlan, editPlan.previewBounds, 1.0F,
                             center, size, rotation)) {
      const CreativePreviewGeometrySelection geometry =
          previewGeometryFor(editPlan.brush, size, false);
      if (geometry.valid) {
        appendCreativePreview(
            frame.creativePreview,
            structuralEdit.preview.accepted &&
                    structuralEdit.preview.changed
                ? RenderCreativePreviewRole::PlacementValid
                : RenderCreativePreviewRole::PlacementInvalid,
            frame.camera.clipFromWorld *
                modelMatrix(center, rotation, size),
            false, {}, geometry.profile, geometry.proceduralSegmentCount);
      }
    }
    return;
  }
  const bool materialPlacement =
      held.kind == cr::CreativeHeldItemKind::Material;
  const bool materialBrush =
      held.kind == cr::CreativeHeldItemKind::MaterialBrush;
  const bool assetScatter =
      creativeEditorUsesAssetScatter(held, editor.toolSettings);
  const bool authoredAsset =
      creativeEditorUsesAuthoredAsset(held, editor.authoredAssets);
  const std::string_view previewAssetId =
      authoredAsset ? std::string_view{}
                    : cr::creativeHotbarAssetId(held);
  if ((!materialPlacement && !materialBrush) ||
      held.objectKind == cr::CreativeObjectKind::Unknown) {
    return;
  }

  CreativeBrushPlacementPlan heldPlan =
      planBrushPlacement(held.objectKind, {});
  if (!cr::creativeHotbarAssetId(held).empty() &&
      (!held.hasAssetBounds ||
       !applyCreativeAssetPlacementBounds(heldPlan,
                                          held.assetSourceBounds))) {
    return;
  }
  Vec3 heldCenter{};
  Vec3 heldSize{};
  const cr::CreativeBounds heldBounds =
      creativeBrushHeldPreviewBounds(heldPlan);
  if (!heldPlan.valid ||
      !previewBoundsTransform(heldBounds, 1.0F, heldCenter,
                              heldSize)) {
    return;
  }
  const bool assetBacked = authoredAsset ||
                           !cr::creativeHotbarAssetId(held).empty();
  const CreativePreviewGeometrySelection heldGeometry =
      previewGeometryFor(heldPlan.brush, heldSize, assetBacked);
  if (!heldGeometry.valid) {
    return;
  }

  const CreativeEditorPlacementFeedback& feedback =
      editor.interaction.placementFeedback;
  const bool mutationAcceptedThisFrame =
      feedback.frameIndex == editor.frameIndex &&
      feedback.status == CreativeEditorPlacementFeedbackStatus::Placed;
  if (materialPlacement && !assetScatter &&
      editor.interaction.target.grid.valid &&
      !mutationAcceptedThisFrame) {
    CreativeEditorPlacementResolution placement;
    if (editor.interaction.structuralSpan.active) {
      if (document != nullptr) {
        placement.admission = resolveCreativeEditorStructuralSpanPlacement(
            editor.interaction.structuralSpan, document->id(), held,
            editor.interaction.target.grid);
        if (placement.admission.allowed) {
          placement.admission.plan.clearance =
              evaluateCreativeBrushPlacementClearance(
                  *document, placement.admission.plan, clearanceCache);
          if (!placement.admission.plan.clearance.allowed) {
            placement.admission.allowed = false;
            placement.admission.status =
                CreativeBrushPlacementAdmissionStatus::ClearanceBlocked;
          }
        }
      }
    } else if (document != nullptr) {
      placement = resolveCreativeEditorPlacement(
          held, editor.interaction.target,
          editor.toolSettings.placementYaw, *document, assetCatalog,
          clearanceCache);
    } else {
      placement.admission = admitBrushPlacement(
          held, editor.interaction.target.grid,
          editor.toolSettings.placementYaw);
    }
    const CreativeBrushPlacementAdmission& admission = placement.admission;
    const CreativeBrushPlacementPlan& targetPlan = admission.plan;
    const cr::CreativeBounds& targetBounds =
        targetPlan.valid ? targetPlan.previewBounds
                         : editor.interaction.target.grid.adjacentCellBounds;
    Vec3 targetCenter{};
    Vec3 targetSize{};
    Vec3 targetRotation{};
    if (previewPlanTransform(targetPlan, targetBounds,
                             authoredAsset ? 0.96F : 1.0F, targetCenter,
                             targetSize, targetRotation)) {
      const CreativePreviewGeometrySelection targetGeometry =
          previewGeometryFor(targetPlan.brush, targetSize, assetBacked);
      if (!targetGeometry.valid) {
        return;
      }
      const bool rejectedThisFrame =
          feedback.frameIndex == editor.frameIndex &&
          feedback.status == CreativeEditorPlacementFeedbackStatus::Rejected;
      const bool duplicate =
          document != nullptr && admission.allowed &&
          creativeBrushPlacementTargetOccupied(
              *document, targetPlan, cr::creativeHotbarAssetId(held));
      const bool targetInvalid =
          !admission.allowed || duplicate || rejectedThisFrame ||
          (authoredAsset
               ? editor.interaction.authoredAssetStroke.capacityReached
               : editor.interaction.materialStroke.capacityReached);
      appendCreativePreview(
          frame.creativePreview,
          targetInvalid ? RenderCreativePreviewRole::PlacementInvalid
                        : RenderCreativePreviewRole::PlacementValid,
          frame.camera.clipFromWorld *
              modelMatrix(targetCenter, targetRotation, targetSize),
          targetPlan.valid &&
              targetPlan.shapeKind == cr::CreativeObjectShapeKind::Path,
          previewAssetId, targetGeometry.profile,
          targetGeometry.proceduralSegmentCount);
    }
  }

  constexpr float kHeldLongestDimension = 0.32F;
  constexpr float kHeldMinimumAxis = 0.06F;
  const float longest = std::max({heldSize.x, heldSize.y, heldSize.z});
  if (!std::isfinite(longest) || longest <= 0.0F) {
    return;
  }
  const float heldScale = kHeldLongestDimension / longest;
  heldSize = {std::max(kHeldMinimumAxis, heldSize.x * heldScale),
              std::max(kHeldMinimumAxis, heldSize.y * heldScale),
              std::max(kHeldMinimumAxis, heldSize.z * heldScale)};
  constexpr float kDegreesToRadians = 0.01745329251994329577F;
  const Vec3 heldRotation{20.0F * kDegreesToRadians,
                          -35.0F * kDegreesToRadians,
                          8.0F * kDegreesToRadians};
  appendCreativePreview(
      frame.creativePreview, RenderCreativePreviewRole::Held,
      frame.camera.clipFromView *
          modelMatrix({0.42F, -0.32F, -0.82F}, heldRotation, heldSize),
      false, previewAssetId, heldGeometry.profile,
      heldGeometry.proceduralSegmentCount);
}

}  // namespace iggy3d_creative_app

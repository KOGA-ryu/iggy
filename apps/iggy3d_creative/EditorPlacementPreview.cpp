#include "EditorPreviewFrame.hpp"
#include "EditorPreviewFrameInternal.hpp"

#include <algorithm>
#include <cmath>

#include "EditorInteraction.hpp"
#include "EditorPlacement.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "core/math/EulerRotation.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace creative = iggy3d::creative;
using namespace iggy3d;
namespace {

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

void appendCreativePreview(RenderCreativePreviewFrame& previews,
                           RenderCreativePreviewRole role,
                           const Mat4& clipFromModel,
                           bool includePathWireframe = false) {
  if (previews.itemCount >= previews.items.size()) {
    return;
  }
  previews.items[previews.itemCount++] = {
      role, clipFromModel, includePathWireframe};
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
    const cr::CreativeDocument* document) {
  frame.creativePreview = {};
  const bool modalOpen = editor.catalog.model.open ||
                         editor.catalog.toolWheel.open ||
                         editor.toolOptions.open ||
                         editor.transform.active;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const bool materialPlacement =
      held.kind == cr::CreativeHeldItemKind::Material;
  const bool materialBrush =
      held.kind == cr::CreativeHeldItemKind::MaterialBrush;
  if (captureMode || modalOpen ||
      (!materialPlacement && !materialBrush) ||
      held.objectKind == cr::CreativeObjectKind::Unknown) {
    return;
  }

  const CreativeBrushPlacementPlan heldPlan =
      planBrushPlacement(held.objectKind, {});
  Vec3 heldCenter{};
  Vec3 heldSize{};
  const cr::CreativeBounds heldBounds =
      creativeBrushHeldPreviewBounds(heldPlan);
  if (!heldPlan.valid ||
      !previewBoundsTransform(heldBounds, 1.0F, heldCenter,
                              heldSize)) {
    return;
  }

  const CreativeEditorPlacementFeedback& feedback =
      editor.interaction.placementFeedback;
  const bool mutationAcceptedThisFrame =
      feedback.frameIndex == editor.frameIndex &&
      feedback.status == CreativeEditorPlacementFeedbackStatus::Placed;
  if (materialPlacement && editor.interaction.target.grid.valid &&
      !mutationAcceptedThisFrame) {
    const CreativeBrushPlacementAdmission admission = admitBrushPlacement(
        held.objectKind, editor.interaction.target.grid,
        editor.toolSettings.placementYaw);
    const CreativeBrushPlacementPlan& targetPlan = admission.plan;
    const cr::CreativeBounds& targetBounds =
        targetPlan.valid ? targetPlan.previewBounds
                         : editor.interaction.target.grid.adjacentCellBounds;
    Vec3 targetCenter{};
    Vec3 targetSize{};
    if (previewBoundsTransform(targetBounds, 1.0F, targetCenter,
                               targetSize)) {
      const bool rejectedThisFrame =
          feedback.frameIndex == editor.frameIndex &&
          feedback.status == CreativeEditorPlacementFeedbackStatus::Rejected;
      const bool duplicate =
          document != nullptr && admission.allowed &&
          creativeBrushPlacementTargetOccupied(*document, targetPlan);
      const bool targetInvalid =
          !admission.allowed || duplicate || rejectedThisFrame ||
          editor.interaction.materialStroke.capacityReached;
      appendCreativePreview(
          frame.creativePreview,
          targetInvalid ? RenderCreativePreviewRole::PlacementInvalid
                        : RenderCreativePreviewRole::PlacementValid,
          frame.camera.clipFromWorld *
              modelMatrix(
                  targetCenter,
                  targetPlan.valid
                      ? cr::creativeVec3ToCoreChecked(
                            targetPlan.transform.rotationEulerRadians)
                            .value
                      : Vec3{},
                  targetSize),
          targetPlan.valid &&
              targetPlan.shapeKind == cr::CreativeObjectShapeKind::Path);
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
          modelMatrix({0.42F, -0.32F, -0.82F}, heldRotation, heldSize));
}

}  // namespace iggy3d_creative_app

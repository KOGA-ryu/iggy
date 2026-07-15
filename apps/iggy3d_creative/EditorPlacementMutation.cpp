#include "EditorPlacement.hpp"

#include <algorithm>
#include <span>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/tools/Volume.hpp"

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] bool sameTransform(
    const iggy3d::creative::CreativeTransform& lhs,
    const iggy3d::creative::CreativeTransform& rhs) noexcept {
  return iggy3d::creative::creativeVec3ExactlyEqual(lhs.position,
                                                     rhs.position) &&
         iggy3d::creative::creativeVec3ExactlyEqual(
             lhs.rotationEulerRadians, rhs.rotationEulerRadians) &&
         iggy3d::creative::creativeVec3ExactlyEqual(lhs.scale, rhs.scale);
}

[[nodiscard]] bool voxelPlanMatchesDocumentGrid(
    const CreativeBrushPlacementPlan& plan,
    iggy3d::creative::CreativeGridSettings grid) noexcept {
  const iggy3d::creative::CreativeBounds expectedBounds =
      iggy3d::creative::creativeVolumeCellBounds(
          plan.voxelCell, grid.cellSizeMeters, grid.origin);
  const iggy3d::creative::CreativeVec3 expectedCenter =
      iggy3d::creative::measureCreativeBounds(expectedBounds).center;
  return plan.hasVoxelCell && plan.hasTransformOverride &&
         plan.hasBoundsOverride && !plan.hasPathOverride &&
         plan.pathPointCount == 0U &&
         iggy3d::creative::creativeBoundsExactlyEqual(plan.authoredBounds,
                                                       expectedBounds) &&
         iggy3d::creative::creativeBoundsExactlyEqual(plan.previewBounds,
                                                       expectedBounds) &&
         iggy3d::creative::creativeVec3ExactlyEqual(plan.transform.position,
                                                     expectedCenter) &&
         iggy3d::creative::creativeVec3ExactlyEqual(
             plan.transform.rotationEulerRadians, {}) &&
         iggy3d::creative::creativeVec3ExactlyEqual(
             plan.transform.scale, {1.0, 1.0, 1.0});
}

[[nodiscard]] bool objectMatchesPlacementPlan(
    const iggy3d::creative::CreativeObject& object,
    const CreativeBrushPlacementPlan& plan,
    std::string_view assetId) noexcept {
  if (object.kind != plan.brush ||
      object.assetId != assetId ||
      !sameTransform(object.transform, plan.transform) ||
      !iggy3d::creative::creativeBoundsExactlyEqual(object.bounds,
                                                     plan.authoredBounds) ||
      object.pathPoints.size() != plan.pathPointCount) {
    return false;
  }
  return std::equal(
      object.pathPoints.begin(), object.pathPoints.end(),
      plan.pathPoints.begin(),
      [](const iggy3d::creative::CreativePathPoint& existing,
         const iggy3d::creative::CreativePathPoint& planned) {
        return iggy3d::creative::creativeVec3ExactlyEqual(existing.position,
                                                           planned.position);
      });
}

[[nodiscard]] std::string_view placementPlanRejectionReason(
    CreativeBrushPlacementPlanStatus status) noexcept {
  switch (status) {
    case CreativeBrushPlacementPlanStatus::InvalidAnchor:
      return "creative_placement_target_invalid";
    case CreativeBrushPlacementPlanStatus::UnsupportedBrush:
      return "creative_placement_brush_unsupported";
    case CreativeBrushPlacementPlanStatus::InvalidGeometry:
      return "creative_placement_geometry_invalid";
    case CreativeBrushPlacementPlanStatus::Ready:
      return "creative_placement_plan_invalid";
  }
  return "creative_placement_plan_invalid";
}

}  // namespace

bool creativeBrushPlacementAlreadyExists(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeBrushPlacementPlan& plan,
    std::string_view assetId) noexcept {
  return creativeBrushPlacementTargetOccupied(document, plan, assetId);
}

bool creativeBrushPlacementTargetOccupied(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeBrushPlacementPlan& plan,
    std::string_view assetId) noexcept {
  if (!plan.valid) {
    return false;
  }
  switch (plan.storagePolicy) {
    case iggy3d::creative::CreativePlacementStoragePolicy::AuthoredObject:
      return std::any_of(
          document.objects().begin(), document.objects().end(),
          [&plan, assetId](const iggy3d::creative::CreativeObject& object) {
            return objectMatchesPlacementPlan(object, plan, assetId);
          });
    case iggy3d::creative::CreativePlacementStoragePolicy::VoxelCell:
      return plan.hasVoxelCell && document.voxelField().occupied(plan.voxelCell);
  }
  return true;
}

iggy3d::creative::CreativeDocumentCreateRequest buildBrushCreateRequest(
    const CreativeBrushPlacementPlan& plan,
    std::uint64_t ordinal,
    std::string_view assetId) {
  iggy3d::creative::CreativeDocumentCreateRequest request;
  request.kind = plan.brush;
  request.assetId = std::string(assetId);
  request.name = std::string(iggy3d::creative::toString(plan.brush)) +
                 " placed#" + std::to_string(ordinal);
  if (!plan.valid) {
    return request;
  }
  if (plan.hasPathOverride) {
    request.pathPoints.assign(
        plan.pathPoints.begin(),
        plan.pathPoints.begin() + plan.pathPointCount);
  }
  request.transform = plan.transform;
  request.bounds = plan.authoredBounds;
  request.hasTransformOverride = plan.hasTransformOverride;
  request.hasBoundsOverride = plan.hasBoundsOverride;
  request.hasPathOverride = plan.hasPathOverride;
  request.visible = true;
  request.hasVisibleOverride = true;
  request.locked = false;
  request.hasLockedOverride = true;
  if (plan.hasAttachment) {
    request.parentId = plan.attachmentTargetId;
    request.attachmentSocket = plan.attachmentSocket;
  }

  return request;
}

iggy3d::creative::CreativeDocumentCreateRequest buildBrushCreateRequest(
    iggy3d::creative::CreativeObjectKind brush,
    iggy3d::Vec3 cellCenter,
    std::uint64_t ordinal) {
  return buildBrushCreateRequest(planBrushPlacement(brush, cellCenter),
                                 ordinal);
}

iggy3d::creative::CreativeDocumentCreateReceipt placeBrushObject(
    iggy3d::creative::Facade& facade,
    const CreativeBrushPlacementPlan& plan,
    std::uint64_t ordinal,
    iggy3d::creative::CreativeObjectId parentObjectId,
    std::string_view assetId) {
  if (!plan.valid || plan.status != CreativeBrushPlacementPlanStatus::Ready) {
    iggy3d::creative::CreativeDocumentCreateReceipt rejected;
    rejected.requested = true;
    rejected.status =
        iggy3d::creative::CreativeDocumentCreateStatus::Rejected;
    rejected.objectKind = plan.brush;
    rejected.revisionBefore = facade.document().revision();
    rejected.revisionAfter = rejected.revisionBefore;
    rejected.message = placementPlanRejectionReason(plan.status);
    rejected.reasonCode = rejected.message;
    return rejected;
  }
  const iggy3d::creative::CreativeObjectDescriptor& descriptor =
      iggy3d::creative::describeObject(plan.brush);
  iggy3d::creative::CreativeDocumentCreateRequest request =
      buildBrushCreateRequest(plan, ordinal, assetId);
  if (!plan.hasAttachment &&
      parentObjectId != iggy3d::creative::kInvalidObjectId) {
    request.parentId = parentObjectId;
  }

  const iggy3d::creative::CreativeDocumentCreateReceipt receipt =
      facade.createDocumentObject(request);
  SDL_Log("iggy3d_creative: PLACE dropped objectId=%llu kind='%s' "
          "pos=(%.3f, %.3f, %.3f) bounds=[(%.3f,%.3f,%.3f)..(%.3f,%.3f,%.3f)] "
          "shape='%s' transformOverride=%d boundsOverride=%d pathOverride=%d "
          "pathPointCount=%zu pathPoints='%s' accepted=%d",
          static_cast<unsigned long long>(receipt.objectId),
          std::string(iggy3d::creative::toString(receipt.objectKind)).c_str(),
          request.transform.position.x, request.transform.position.y,
          request.transform.position.z, request.bounds.min.x,
          request.bounds.min.y, request.bounds.min.z, request.bounds.max.x,
          request.bounds.max.y, request.bounds.max.z,
          std::string(iggy3d::creative::toString(descriptor.shapeKind)).c_str(),
          request.hasTransformOverride ? 1 : 0,
          request.hasBoundsOverride ? 1 : 0,
          request.hasPathOverride ? 1 : 0,
          request.pathPoints.size(),
          pathPointsSummary(request.pathPoints).c_str(),
          receipt.accepted ? 1 : 0);
  return receipt;
}

iggy3d::creative::CreativeDocumentCreateReceipt placeBrushObject(
    iggy3d::creative::Facade& facade,
    iggy3d::creative::CreativeObjectKind brush,
    iggy3d::Vec3 cellCenter,
    std::uint64_t ordinal) {
  return placeBrushObject(facade, planBrushPlacement(brush, cellCenter),
                          ordinal);
}

CreativeBrushPlacementMutationReceipt applyBrushPlacement(
    iggy3d::creative::Facade& facade,
    const CreativeBrushPlacementPlan& plan,
    std::uint64_t ordinal,
    iggy3d::creative::CreativeObjectId parentObjectId,
    std::string_view assetId) {
  CreativeBrushPlacementMutationReceipt receipt;
  receipt.requested = true;
  receipt.storagePolicy = plan.storagePolicy;
  receipt.objectKind = plan.brush;
  receipt.voxelCell = plan.voxelCell;
  receipt.worldBounds = plan.previewBounds;
  receipt.attachmentTargetId = plan.attachmentTargetId;
  receipt.attachmentSocket = plan.attachmentSocket;
  receipt.revisionBefore = facade.document().revision();
  receipt.revisionAfter = receipt.revisionBefore;

  const iggy3d::creative::CreativeObjectPlacementPolicy& policy =
      iggy3d::creative::describeObject(plan.brush).placementPolicy;
  if (!plan.valid || plan.status != CreativeBrushPlacementPlanStatus::Ready ||
      !policy.enabled || policy.storagePolicy != plan.storagePolicy ||
      (plan.hasAttachment &&
       (plan.attachmentTargetId ==
            iggy3d::creative::kInvalidObjectId ||
        !iggy3d::creative::validCreativeAttachmentSocketName(
            plan.attachmentSocket))) ||
      (!assetId.empty() &&
       plan.storagePolicy !=
           iggy3d::creative::CreativePlacementStoragePolicy::AuthoredObject)) {
    receipt.status = CreativeBrushPlacementMutationStatus::InvalidPlan;
    receipt.reasonCode = placementPlanRejectionReason(plan.status);
    return receipt;
  }
  switch (plan.storagePolicy) {
    case iggy3d::creative::CreativePlacementStoragePolicy::AuthoredObject: {
      if (policy.occupancyPolicy !=
              iggy3d::creative::CreativePlacementOccupancyPolicy::AllowOverlap ||
          plan.hasVoxelCell) {
        receipt.status = CreativeBrushPlacementMutationStatus::InvalidPlan;
        receipt.reasonCode = "creative_placement_object_plan_invalid";
        return receipt;
      }
      if (creativeBrushPlacementTargetOccupied(facade.document(), plan,
                                               assetId)) {
        receipt.status = CreativeBrushPlacementMutationStatus::Occupied;
        receipt.reasonCode = "creative_placement_target_occupied";
        return receipt;
      }
      const iggy3d::creative::CreativeDocumentCreateReceipt objectReceipt =
          placeBrushObject(facade, plan, ordinal, parentObjectId, assetId);
      receipt.attached = plan.hasAttachment && objectReceipt.accepted &&
                         objectReceipt.objectCreated && objectReceipt.changed;
      receipt.accepted = objectReceipt.accepted;
      receipt.changed = objectReceipt.changed;
      receipt.objectCreated = objectReceipt.objectCreated;
      receipt.objectId = objectReceipt.objectId;
      receipt.revisionAfter = facade.document().revision();
      receipt.reasonCode = receipt.attached
                               ? "creative_placement_attached"
                               : objectReceipt.reasonCode;
      receipt.status = objectReceipt.accepted && objectReceipt.objectCreated &&
                               objectReceipt.changed
                           ? CreativeBrushPlacementMutationStatus::Applied
                           : CreativeBrushPlacementMutationStatus::
                                 ObjectRejected;
      return receipt;
    }
    case iggy3d::creative::CreativePlacementStoragePolicy::VoxelCell: {
      if (policy.occupancyPolicy !=
              iggy3d::creative::CreativePlacementOccupancyPolicy::
                  RejectOccupied ||
          !voxelPlanMatchesDocumentGrid(plan,
                                        facade.document().gridSettings())) {
        receipt.status = CreativeBrushPlacementMutationStatus::InvalidPlan;
        receipt.reasonCode = "creative_placement_voxel_grid_mismatch";
        return receipt;
      }
      if (creativeBrushPlacementTargetOccupied(facade.document(), plan)) {
        receipt.status = CreativeBrushPlacementMutationStatus::Occupied;
        receipt.reasonCode = "creative_placement_target_occupied";
        return receipt;
      }
      const iggy3d::creative::CreativeVoxelEdit edit{plan.voxelCell,
                                                      plan.brush};
      const iggy3d::creative::CreativeVoxelMutationReceipt voxelReceipt =
          facade.applyVoxelEdits(std::span{&edit, 1U});
      receipt.accepted = voxelReceipt.accepted;
      receipt.changed = voxelReceipt.changed;
      receipt.voxelCreated = voxelReceipt.createdCellCount == 1U;
      receipt.revisionAfter = facade.document().revision();
      receipt.reasonCode = voxelReceipt.reasonCode;
      receipt.status = voxelReceipt.accepted && voxelReceipt.changed &&
                               receipt.voxelCreated
                           ? CreativeBrushPlacementMutationStatus::Applied
                           : CreativeBrushPlacementMutationStatus::
                                 VoxelRejected;
      return receipt;
    }
  }
  receipt.status = CreativeBrushPlacementMutationStatus::InvalidPlan;
  receipt.reasonCode = "creative_placement_storage_policy_invalid";
  return receipt;
}


iggy3d::creative::CreativeDocumentCreateReceipt placeBrushObjectWithUndo(
    iggy3d::creative::Facade& facade,
    StandaloneEditHistory& history,
    iggy3d::creative::CreativeObjectKind brush,
    iggy3d::Vec3 cellCenter,
    std::uint64_t ordinal,
    std::string_view source) {
  const std::uint64_t undoDepthBefore =
      iggy3d::creative::creativeUndoDepth(history);
  StandaloneEditTransaction transaction = beginEditTransaction(facade, source);
  iggy3d::creative::CreativeDocumentCreateReceipt receipt =
      placeBrushObject(facade, brush, cellCenter, ordinal);
  (void)completeEditTransaction(
      history, std::move(transaction), facade,
      receipt.accepted && receipt.objectCreated && receipt.changed,
      receipt.reasonCode);
  SDL_Log("iggy3d_creative: UNDO create source='%s' objectId=%llu "
          "accepted=%d created=%d objectCount=%llu depthBefore=%llu "
          "depthAfter=%llu reasonCode='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(receipt.objectId),
          receipt.accepted ? 1 : 0, receipt.objectCreated ? 1 : 0,
          static_cast<unsigned long long>(facade.document().objectCount()),
          static_cast<unsigned long long>(undoDepthBefore),
          static_cast<unsigned long long>(
              iggy3d::creative::creativeUndoDepth(history)),
          std::string(receipt.reasonCode).c_str());
  return receipt;
}

}  // namespace iggy3d_creative_app

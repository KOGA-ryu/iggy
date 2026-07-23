#include "EditorAttachmentPlacement.hpp"

#include "EditorInteraction.hpp"
#include "EditorPlacementClearance.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

enum class CreativeAssetAlignmentAnchor : std::uint8_t {
  Grid,
  ProjectGridToSurface,
  HitPoint,
};

struct CreativeAssetAlignmentRule {
  cr::CreativePlacementHostPolicy hostPolicy =
      cr::CreativePlacementHostPolicy::AnyKnownTarget;
  CreativeAssetAlignmentAnchor anchor = CreativeAssetAlignmentAnchor::Grid;
  CreativeAssetAlignmentStatus rejection =
      CreativeAssetAlignmentStatus::InvalidTarget;
  bool orientToSurfaceNormal = false;
};

constexpr std::array kAssetAlignmentRules{
    CreativeAssetAlignmentRule{
        cr::CreativePlacementHostPolicy::AnyKnownTarget,
        CreativeAssetAlignmentAnchor::Grid,
        CreativeAssetAlignmentStatus::InvalidTarget, false},
    CreativeAssetAlignmentRule{
        cr::CreativePlacementHostPolicy::SupportingSurface,
        CreativeAssetAlignmentAnchor::ProjectGridToSurface,
        CreativeAssetAlignmentStatus::FloorRequired, false},
    CreativeAssetAlignmentRule{
        cr::CreativePlacementHostPolicy::SolidVerticalSurface,
        CreativeAssetAlignmentAnchor::ProjectGridToSurface,
        CreativeAssetAlignmentStatus::WallRequired, false},
    CreativeAssetAlignmentRule{
        cr::CreativePlacementHostPolicy::SolidSurface,
        CreativeAssetAlignmentAnchor::HitPoint,
        CreativeAssetAlignmentStatus::SurfaceRequired, true},
    CreativeAssetAlignmentRule{
        cr::CreativePlacementHostPolicy::AnyKnownTarget,
        CreativeAssetAlignmentAnchor::HitPoint,
        CreativeAssetAlignmentStatus::InvalidTarget, false},
};

[[nodiscard]] bool stageAbsoluteHierarchyTransform(
    cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> objectIds,
    cr::CreativeObjectId sourceObjectId,
    const cr::CreativeTransform& transform,
    std::string& failure,
    bool& changed) {
  const cr::CreativeObject* source = document.findObject(sourceObjectId);
  if (source == nullptr) {
    failure = "creative_attachment_source_object_missing";
    return false;
  }
  if (source->parentId.has_value() &&
      std::find(objectIds.begin(), objectIds.end(), *source->parentId) ==
          objectIds.end()) {
    const cr::CreativeDocumentMutationReceipt detached =
        cr::applyDocumentMutation(document, sourceObjectId,
                                  cr::CreativeMutationKind::DetachFrom,
                                  cr::CreativeMutationPayload{});
    if (!cr::documentMutationSucceeded(detached.status)) {
      failure = detached.message;
      return false;
    }
  }
  const auto applyPlacement =
      [&](const cr::CreativeSelectionPlacementRequest& request) {
        const cr::CreativeSelectionPlacementReceipt receipt =
            cr::placeDocumentObjectsAtomically(document, objectIds, request);
        if (!receipt.accepted) {
          failure = receipt.reasonCode;
          return false;
        }
        changed = changed || receipt.changed;
        return true;
      };

  const cr::CreativeObject* current = document.findObject(sourceObjectId);
  if (current == nullptr || !cr::isPositiveCreativeVec3(current->transform.scale)) {
    failure = "creative_attachment_source_object_missing";
    return false;
  }
  const cr::CreativeVec3 scaleFactor{
      transform.scale.x / current->transform.scale.x,
      transform.scale.y / current->transform.scale.y,
      transform.scale.z / current->transform.scale.z};
  if (!cr::creativeVec3ExactlyEqual(scaleFactor, {1.0, 1.0, 1.0})) {
    cr::CreativeSelectionPlacementRequest scale;
    scale.mode = cr::CreativeSelectionPlacementMode::Move;
    scale.sourceAnchor = current->transform.position;
    scale.targetAnchor = scale.sourceAnchor;
    scale.scaleFactor = scaleFactor;
    if (!applyPlacement(scale)) {
      return false;
    }
  }

  current = document.findObject(sourceObjectId);
  if (current == nullptr) {
    failure = "creative_attachment_source_object_missing";
    return false;
  }
  const cr::CreativeVec3 pivot = current->transform.position;
  const cr::CreativeVec3 currentRotation = current->transform.rotationEulerRadians;
  const auto rotateAroundPivot =
      [&](cr::CreativeAxis3 axis, double radians) {
        if (radians == 0.0) {
          return true;
        }
        cr::CreativeSelectionPlacementRequest rotation;
        rotation.mode = cr::CreativeSelectionPlacementMode::Move;
        rotation.sourceAnchor = pivot;
        rotation.targetAnchor = pivot;
        rotation.hasAxisAngleRotation = true;
        rotation.rotationAxis = axis;
        rotation.rotationRadians = radians;
        return applyPlacement(rotation);
      };
  if (!rotateAroundPivot(cr::CreativeAxis3::Z, -currentRotation.z) ||
      !rotateAroundPivot(cr::CreativeAxis3::Y, -currentRotation.y) ||
      !rotateAroundPivot(cr::CreativeAxis3::X, -currentRotation.x) ||
      !rotateAroundPivot(cr::CreativeAxis3::X,
                         transform.rotationEulerRadians.x) ||
      !rotateAroundPivot(cr::CreativeAxis3::Y,
                         transform.rotationEulerRadians.y) ||
      !rotateAroundPivot(cr::CreativeAxis3::Z,
                         transform.rotationEulerRadians.z)) {
    if (failure.empty()) {
      failure = "creative_attachment_hierarchy_rotation_rejected";
    }
    return false;
  }

  current = document.findObject(sourceObjectId);
  if (current == nullptr) {
    failure = "creative_attachment_source_object_missing";
    return false;
  }
  if (!cr::creativeVec3ExactlyEqual(current->transform.position,
                                    transform.position)) {
    cr::CreativeSelectionPlacementRequest move;
    move.mode = cr::CreativeSelectionPlacementMode::Move;
    move.sourceAnchor = current->transform.position;
    move.targetAnchor = transform.position;
    if (!applyPlacement(move)) {
      return false;
    }
  }
  return true;
}
static_assert(kAssetAlignmentRules.size() ==
              static_cast<std::size_t>(cr::CreativeAssetAlignmentMode::Count));

[[nodiscard]] double dot(cr::CreativeVec3 lhs,
                         cr::CreativeVec3 rhs) noexcept {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] cr::CreativeVec3 projectedAnchor(
    cr::CreativeVec3 anchor,
    cr::CreativeVec3 point,
    cr::CreativeVec3 normal) noexcept {
  const cr::CreativeVec3 delta{anchor.x - point.x, anchor.y - point.y,
                               anchor.z - point.z};
  const double distance = dot(delta, normal);
  return {anchor.x - normal.x * distance,
          anchor.y - normal.y * distance,
          anchor.z - normal.z * distance};
}

[[nodiscard]] CreativeBrushPlacementAdmissionStatus admissionStatusFor(
    CreativeAssetAlignmentStatus status) noexcept {
  switch (status) {
    case CreativeAssetAlignmentStatus::FloorRequired:
      return CreativeBrushPlacementAdmissionStatus::FloorRequired;
    case CreativeAssetAlignmentStatus::WallRequired:
      return CreativeBrushPlacementAdmissionStatus::WallRequired;
    case CreativeAssetAlignmentStatus::SurfaceRequired:
      return CreativeBrushPlacementAdmissionStatus::SurfaceRequired;
    case CreativeAssetAlignmentStatus::Ready:
    case CreativeAssetAlignmentStatus::InvalidMode:
    case CreativeAssetAlignmentStatus::InvalidTarget:
      return CreativeBrushPlacementAdmissionStatus::InvalidTarget;
  }
  return CreativeBrushPlacementAdmissionStatus::InvalidTarget;
}

}  // namespace

CreativeBrushPlacementAdmissionStatus creativeAssetAlignmentAdmissionStatus(
    CreativeAssetAlignmentStatus status) noexcept {
  return admissionStatusFor(status);
}

CreativeAssetAlignmentPlan resolveCreativeAssetAlignment(
    const cr::CreativeGridTarget& target,
    cr::CreativeAssetAlignmentMode mode) noexcept {
  CreativeAssetAlignmentPlan result;
  result.mode = mode;
  const std::size_t modeIndex = static_cast<std::size_t>(mode);
  if (modeIndex >= kAssetAlignmentRules.size()) {
    result.status = CreativeAssetAlignmentStatus::InvalidMode;
    return result;
  }
  const cr::CreativeVec3 normal =
      cr::creativeGridTargetExactSurfaceNormal(target);
  if (!target.valid || (target.resolved && !target.adjacentInBounds) ||
      !cr::isFiniteCreativeVec3(target.hitPoint) ||
      !cr::isFiniteCreativeVec3(target.placementAnchor) ||
      !cr::isFiniteCreativeVec3(normal)) {
    return result;
  }
  const double normalLengthSquared = dot(normal, normal);
  if (!std::isfinite(normalLengthSquared) || normalLengthSquared <= 1.0e-24) {
    return result;
  }

  const CreativeAssetAlignmentRule& rule = kAssetAlignmentRules[modeIndex];
  result.compatibility = cr::resolveCreativePlacementCompatibility(
      {rule.hostPolicy, target.targetFacts, normal});
  if (!result.compatibility.allowed) {
    result.status = result.compatibility.status ==
                            cr::CreativePlacementCompatibilityStatus::UnknownTarget
                        ? CreativeAssetAlignmentStatus::InvalidTarget
                        : rule.rejection;
    return result;
  }

  result.target = target;
  switch (rule.anchor) {
    case CreativeAssetAlignmentAnchor::Grid:
      break;
    case CreativeAssetAlignmentAnchor::ProjectGridToSurface:
      result.target.placementAnchor =
          projectedAnchor(target.placementAnchor, target.hitPoint, normal);
      result.target.placementNormal = normal;
      result.target.anchorSnapped = true;
      break;
    case CreativeAssetAlignmentAnchor::HitPoint:
      result.target.placementAnchor = target.hitPoint;
      result.target.placementNormal = normal;
      result.target.anchorSnapped = true;
      break;
  }
  if (!cr::isFiniteCreativeVec3(result.target.placementAnchor)) {
    return {};
  }
  result.orientToSurfaceNormal = rule.orientToSurfaceNormal;
  result.status = CreativeAssetAlignmentStatus::Ready;
  result.valid = true;
  return result;
}

std::string_view toString(
    CreativeEditorObjectReattachmentStatus status) noexcept {
  switch (status) {
    case CreativeEditorObjectReattachmentStatus::NotRequested:
      return "creative_reattach_not_requested";
    case CreativeEditorObjectReattachmentStatus::InvalidRequest:
      return "creative_reattach_request_invalid";
    case CreativeEditorObjectReattachmentStatus::SourceObjectMissing:
      return "creative_reattach_source_object_missing";
    case CreativeEditorObjectReattachmentStatus::SourceAssetMissing:
      return "creative_reattach_source_asset_missing";
    case CreativeEditorObjectReattachmentStatus::TargetObjectMissing:
      return "creative_reattach_target_object_missing";
    case CreativeEditorObjectReattachmentStatus::TargetInsideSourceHierarchy:
      return "creative_reattach_target_inside_source_hierarchy";
    case CreativeEditorObjectReattachmentStatus::HierarchyTransformRejected:
      return "creative_reattach_hierarchy_transform_rejected";
    case CreativeEditorObjectReattachmentStatus::SnapRejected:
      return "creative_reattach_snap_rejected";
    case CreativeEditorObjectReattachmentStatus::ClearanceBlocked:
      return "creative_reattach_clearance_blocked";
    case CreativeEditorObjectReattachmentStatus::Ready:
      return "creative_reattach_ready";
    case CreativeEditorObjectReattachmentStatus::MutationRejected:
      return "creative_reattach_mutation_rejected";
    case CreativeEditorObjectReattachmentStatus::Applied:
      return "creative_reattach_applied";
  }
  return "creative_reattach_status_invalid";
}

CreativeEditorObjectReattachmentPlan planCreativeEditorObjectReattachment(
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog& assetCatalog,
    cr::CreativeObjectId sourceObjectId,
    cr::CreativeObjectId targetObjectId,
    cr::CreativeVec3 aimPoint,
    cr::CreativeAssetAttachmentMode attachmentMode,
    const CreativePlacementClearanceCache* clearanceCache) {
  CreativeEditorObjectReattachmentPlan plan;
  if (!document.isValid() || sourceObjectId == cr::kInvalidObjectId ||
      targetObjectId == cr::kInvalidObjectId ||
      sourceObjectId == targetObjectId || !cr::isFiniteCreativeVec3(aimPoint) ||
      attachmentMode >= cr::CreativeAssetAttachmentMode::Count) {
    plan.status = CreativeEditorObjectReattachmentStatus::InvalidRequest;
    return plan;
  }
  plan.documentId = document.id();
  plan.documentRevision = document.revision();
  plan.sourceObjectId = sourceObjectId;
  plan.targetObjectId = targetObjectId;

  const cr::CreativeObject* source = document.findObject(sourceObjectId);
  if (source == nullptr) {
    plan.status = CreativeEditorObjectReattachmentStatus::SourceObjectMissing;
    return plan;
  }
  if (source->assetId.empty() || assetCatalog.find(source->assetId) == nullptr) {
    plan.status = CreativeEditorObjectReattachmentStatus::SourceAssetMissing;
    return plan;
  }
  if (document.findObject(targetObjectId) == nullptr) {
    plan.status = CreativeEditorObjectReattachmentStatus::TargetObjectMissing;
    return plan;
  }

  const cr::CreativeHierarchySelection hierarchy =
      cr::resolveCreativeObjectHierarchy(
          document, std::span<const cr::CreativeObjectId>{&sourceObjectId, 1U});
  if (!hierarchy.accepted || hierarchy.objectIds.empty()) {
    plan.status =
        CreativeEditorObjectReattachmentStatus::HierarchyTransformRejected;
    return plan;
  }
  plan.hierarchyObjectCount = hierarchy.objectIds.size();
  if (std::find(hierarchy.objectIds.begin(), hierarchy.objectIds.end(),
                targetObjectId) != hierarchy.objectIds.end()) {
    plan.status =
        CreativeEditorObjectReattachmentStatus::TargetInsideSourceHierarchy;
    return plan;
  }

  cr::CreativeAttachmentSnapRequest snapRequest;
  snapRequest.document = &document;
  snapRequest.assetCatalog = &assetCatalog;
  snapRequest.sourceAssetId = source->assetId;
  snapRequest.targetObjectId = targetObjectId;
  snapRequest.aimPoint = aimPoint;
  snapRequest.sourceScale = source->transform.scale;
  snapRequest.ignoredOccupantObjectId = sourceObjectId;
  snapRequest.selectionMode =
      attachmentMode == cr::CreativeAssetAttachmentMode::AimSocket
          ? cr::CreativeAttachmentSnapSelectionMode::AimedSocket
          : cr::CreativeAttachmentSnapSelectionMode::BestMatch;
  plan.snap = cr::resolveCreativeAttachmentSnap(snapRequest);
  if (plan.snap.status != cr::CreativeAttachmentSnapStatus::Ready ||
      !plan.snap.positioned || !plan.snap.snapped) {
    plan.status = CreativeEditorObjectReattachmentStatus::SnapRejected;
    return plan;
  }

  cr::CreativeDocument staged = document;
  std::string transformFailure;
  bool transformChanged = false;
  if (!stageAbsoluteHierarchyTransform(
          staged, hierarchy.objectIds, sourceObjectId, plan.snap.transform,
          transformFailure, transformChanged)) {
    plan.status =
        CreativeEditorObjectReattachmentStatus::HierarchyTransformRejected;
    return plan;
  }
  std::vector<cr::CreativeObject> candidates;
  candidates.reserve(hierarchy.objectIds.size());
  for (cr::CreativeObjectId objectId : hierarchy.objectIds) {
    const cr::CreativeObject* candidate = staged.findObject(objectId);
    if (candidate == nullptr) {
      plan.status =
          CreativeEditorObjectReattachmentStatus::HierarchyTransformRejected;
      return plan;
    }
    candidates.push_back(*candidate);
  }
  const CreativeObjectSetClearanceResult clearance =
      evaluateCreativeAttachmentPlacementClearance(
          document, assetCatalog, candidates, hierarchy.objectIds,
          sourceObjectId, targetObjectId, clearanceCache);
  plan.clearance = clearance.clearance;
  if (!plan.clearance.allowed) {
    plan.status = CreativeEditorObjectReattachmentStatus::ClearanceBlocked;
    return plan;
  }
  plan.status = CreativeEditorObjectReattachmentStatus::Ready;
  plan.accepted = true;
  return plan;
}

CreativeEditorObjectReattachmentReceipt applyCreativeEditorObjectReattachment(
    cr::CreativeDocument& document,
    const CreativeEditorObjectReattachmentPlan& plan) {
  CreativeEditorObjectReattachmentReceipt receipt;
  receipt.status = CreativeEditorObjectReattachmentStatus::InvalidRequest;
  receipt.sourceObjectId = plan.sourceObjectId;
  receipt.targetObjectId = plan.targetObjectId;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (!plan.accepted ||
      plan.status != CreativeEditorObjectReattachmentStatus::Ready ||
      plan.documentId != document.id() ||
      plan.documentRevision != document.revision()) {
    return receipt;
  }

  const cr::CreativeHierarchySelection hierarchy =
      cr::resolveCreativeObjectHierarchy(
          document,
          std::span<const cr::CreativeObjectId>{&plan.sourceObjectId, 1U});
  if (!hierarchy.accepted || hierarchy.objectIds.empty() ||
      hierarchy.objectIds.size() != plan.hierarchyObjectCount) {
    receipt.status =
        CreativeEditorObjectReattachmentStatus::HierarchyTransformRejected;
    return receipt;
  }
  const cr::CreativeObject* sourceBefore =
      document.findObject(plan.sourceObjectId);
  if (sourceBefore == nullptr) {
    receipt.status = CreativeEditorObjectReattachmentStatus::SourceObjectMissing;
    return receipt;
  }
  const bool relationshipChanged =
      sourceBefore->parentId !=
          std::optional<cr::CreativeObjectId>{plan.targetObjectId} ||
      sourceBefore->attachmentSocket != plan.snap.targetSocket;
  cr::CreativeDocument staged = document;
  std::string transformFailure;
  bool transformChanged = false;
  if (!stageAbsoluteHierarchyTransform(
          staged, hierarchy.objectIds, plan.sourceObjectId, plan.snap.transform,
          transformFailure, transformChanged)) {
    receipt.status =
        CreativeEditorObjectReattachmentStatus::HierarchyTransformRejected;
    return receipt;
  }
  const cr::CreativeDocumentMutationReceipt attached =
      cr::applyDocumentMutation(
          staged, plan.sourceObjectId, cr::CreativeMutationKind::AttachTo,
          cr::makeAttachPayload(plan.targetObjectId,
                                std::string(plan.snap.targetSocket)));
  if (!cr::documentMutationSucceeded(attached.status)) {
    receipt.status = CreativeEditorObjectReattachmentStatus::MutationRejected;
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = transformChanged || relationshipChanged;
  receipt.status = CreativeEditorObjectReattachmentStatus::Applied;
  if (receipt.changed) {
    document = std::move(staged);
  }
  receipt.revisionAfter = document.revision();
  return receipt;
}

CreativeEditorPlacementResolution resolveCreativeEditorPlacement(
    const cr::CreativeHotbarEntry& held,
    const CreativeEditorWorldTarget& target,
    cr::CreativePlacementYaw placementYaw,
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog,
    const CreativePlacementClearanceCache* clearanceCache,
    cr::CreativeAssetAlignmentMode alignmentMode,
    cr::CreativeAssetAttachmentMode attachmentMode) noexcept {
  CreativeEditorPlacementResolution result;
  const std::string_view sourceAssetId = cr::creativeHotbarAssetId(held);
  const cr::CreativeGridTarget* placementTarget = &target.grid;
  if (!sourceAssetId.empty()) {
    result.alignment = resolveCreativeAssetAlignment(target.grid, alignmentMode);
    if (!result.alignment.valid) {
      result.admission.plan.brush = held.objectKind;
      result.admission.status =
          creativeAssetAlignmentAdmissionStatus(result.alignment.status);
      return result;
    }
    placementTarget = &result.alignment.target;
  }
  result.admission = admitBrushPlacement(
      held, *placementTarget, placementYaw, alignmentMode);
  const bool compatibilityOnlyRejection =
      result.admission.status ==
          CreativeBrushPlacementAdmissionStatus::TargetIncompatible &&
      result.admission.plan.valid;
  if ((!result.admission.allowed && !compatibilityOnlyRejection) ||
      sourceAssetId.empty() ||
      !held.hasAssetBounds || !target.objectHit || !target.grid.valid ||
      assetCatalog == nullptr) {
    applyCreativeBrushPlacementClearance(result.admission, document,
                                         clearanceCache);
    return result;
  }

  cr::CreativeAttachmentSnapRequest request;
  request.document = &document;
  request.assetCatalog = assetCatalog;
  request.sourceAssetId = sourceAssetId;
  request.targetObjectId = target.objectId;
  request.aimPoint = target.grid.hitPoint;
  request.sourceScale = result.admission.plan.transform.scale;
  request.selectionMode =
      attachmentMode == cr::CreativeAssetAttachmentMode::AimSocket
          ? cr::CreativeAttachmentSnapSelectionMode::AimedSocket
          : cr::CreativeAttachmentSnapSelectionMode::BestMatch;
  result.attachment = cr::resolveCreativeAttachmentSnap(request);
  result.socketTargeted =
      result.attachment.status == cr::CreativeAttachmentSnapStatus::Ready ||
      result.attachment.status == cr::CreativeAttachmentSnapStatus::Occupied;
  if (attachmentMode == cr::CreativeAssetAttachmentMode::AimSocket &&
      !result.socketTargeted) {
    result.admission.allowed = false;
    result.admission.status =
        result.attachment.status ==
                cr::CreativeAttachmentSnapStatus::NoCompatibleSocket
            ? CreativeBrushPlacementAdmissionStatus::AttachmentIncompatible
            : CreativeBrushPlacementAdmissionStatus::AttachmentUnavailable;
    return result;
  }
  if (!result.socketTargeted || !result.attachment.positioned) {
    applyCreativeBrushPlacementClearance(result.admission, document,
                                         clearanceCache);
    return result;
  }

  if (!applyCreativeAssetPlacementTransform(
          result.admission.plan, held.assetSourceBounds,
          result.attachment.transform)) {
    result.admission.allowed = false;
    result.admission.status =
        CreativeBrushPlacementAdmissionStatus::InvalidGeometry;
    return result;
  }
  if (result.attachment.status == cr::CreativeAttachmentSnapStatus::Occupied) {
    result.admission.allowed = false;
    result.admission.status =
        CreativeBrushPlacementAdmissionStatus::AttachmentOccupied;
    return result;
  }

  result.admission.plan.hasAttachment = true;
  result.admission.plan.attachmentSatisfiesCompatibility =
      compatibilityOnlyRejection;
  result.admission.plan.attachmentTargetId =
      result.attachment.targetObjectId;
  result.admission.plan.attachmentSocket = result.attachment.targetSocket;
  result.admission.status = CreativeBrushPlacementAdmissionStatus::Ready;
  result.admission.allowed = true;
  cr::CreativeObject candidate;
  candidate.kind = held.objectKind;
  candidate.assetId = std::string(sourceAssetId);
  candidate.bounds = result.admission.plan.authoredBounds;
  candidate.transform = result.admission.plan.transform;
  const std::array candidates{candidate};
  const CreativeObjectSetClearanceResult attachmentClearance =
      evaluateCreativeAttachmentPlacementClearance(
          document, *assetCatalog, candidates, {}, cr::kInvalidObjectId,
          result.attachment.targetObjectId, clearanceCache);
  result.admission.plan.clearance = attachmentClearance.clearance;
  if (!attachmentClearance.clearance.allowed) {
    result.admission.allowed = false;
    result.admission.status =
        CreativeBrushPlacementAdmissionStatus::ClearanceBlocked;
  }
  return result;
}

}  // namespace iggy3d_creative_app

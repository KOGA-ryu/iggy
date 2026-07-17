#include "EditorAttachmentPlacement.hpp"

#include "EditorInteraction.hpp"
#include "EditorPlacementClearance.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
CreativeEditorPlacementResolution resolveCreativeEditorPlacement(
    const cr::CreativeHotbarEntry& held,
    const CreativeEditorWorldTarget& target,
    cr::CreativePlacementYaw placementYaw,
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog,
    const CreativePlacementClearanceCache* clearanceCache) noexcept {
  CreativeEditorPlacementResolution result;
  result.admission = admitBrushPlacement(held, target.grid, placementYaw);
  const std::string_view sourceAssetId = cr::creativeHotbarAssetId(held);
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
  result.attachment = cr::resolveCreativeAttachmentSnap(request);
  result.socketTargeted =
      result.attachment.status == cr::CreativeAttachmentSnapStatus::Ready ||
      result.attachment.status == cr::CreativeAttachmentSnapStatus::Occupied;
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
  applyCreativeBrushPlacementClearance(result.admission, document,
                                       clearanceCache);
  return result;
}

}  // namespace iggy3d_creative_app

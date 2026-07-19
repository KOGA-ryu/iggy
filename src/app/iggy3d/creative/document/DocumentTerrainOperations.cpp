#include "app/iggy3d/creative/document/Document.hpp"

#include "app/iggy3d/creative/document/DocumentInternal.hpp"

#include <utility>

namespace iggy3d::creative {

using document_internal::documentSettingsDirtyFlags;

CreativeTerrainOperationMutationReceipt
CreativeDocument::applyTerrainOperationMutation(
    const CreativeTerrainOperationMutationRequest& request) {
  if (!valid_) {
    CreativeTerrainOperationMutationReceipt receipt;
    receipt.requested = true;
    receipt.kind = request.kind;
    receipt.operationId = request.operationId;
    receipt.status = CreativeTerrainOperationMutationStatus::InvalidTerrain;
    receipt.reasonCode = "creative_terrain_operation_document_invalid";
    return receipt;
  }

  CreativeTerrainOperationMutationPlan plan =
      planCreativeTerrainOperationMutation(
          terrainField_, terrainHeightField_, terrainOperationStack_, request);
  if (!plan.receipt.accepted || !plan.receipt.changed) {
    return plan.receipt;
  }

  terrainOperationStack_ = std::move(plan.stack);
  terrainHeightField_ = std::move(plan.heightField);
  markObjectMutationChanged(
      dirtyFlagsForCreation(CreativeObjectKind::TerrainPatch) |
      documentSettingsDirtyFlags());
  return plan.receipt;
}

}  // namespace iggy3d::creative

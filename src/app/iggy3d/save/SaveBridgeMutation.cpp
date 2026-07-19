#include "app/iggy3d/save/SaveBridge.hpp"

namespace iggy3d {
namespace {

ProductSaveMutationStatus mutationStatusForSoftDelete(
    const ProductSaveSoftDeleteResult& result) {
  // branch-gate: BG-1218
  if (result.ok) {
    return ProductSaveMutationStatus::Succeeded;
  }
  // branch-gate: BG-1218
  if (result.reasonCode == "product_save_delete_id_missing" ||
      result.reasonCode == "soft_delete_source_missing") {
    return ProductSaveMutationStatus::SaveNotFound;
  }
  // branch-gate: BG-1218
  if (result.reasonCode == "soft_delete_snapshot_move_failed") {
    return ProductSaveMutationStatus::SnapshotMoveFailed;
  }
  return ProductSaveMutationStatus::FileOperationFailed;
}

ProductSaveMutationStatus mutationStatusForRecover(
    const ProductSaveRecoverResult& result) {
  // branch-gate: BG-1218
  if (result.ok) {
    return ProductSaveMutationStatus::Succeeded;
  }
  // branch-gate: BG-1218
  if (result.reasonCode == "product_save_recover_id_missing" ||
      result.reasonCode == "recover_save_source_missing") {
    return ProductSaveMutationStatus::SaveNotFound;
  }
  // branch-gate: BG-1218
  if (result.reasonCode == "recover_save_target_exists") {
    return ProductSaveMutationStatus::NotActiveSave;
  }
  // branch-gate: BG-1218
  if (result.reasonCode == "recover_save_snapshot_move_failed") {
    return ProductSaveMutationStatus::SnapshotMoveFailed;
  }
  return ProductSaveMutationStatus::FileOperationFailed;
}

}  // namespace

ProductSaveSoftDeleteResult softDeleteProductSave(
    const ProductSaveSoftDeleteRequest& request) {
  ProductSaveSoftDeleteResult result;
  if (request.saveId.empty()) {
    result.status = "product_save_delete_id_missing";
    result.reasonCode = "product_save_delete_id_missing";
    return result;
  }
  result.saveId = request.saveId;

  const SaveFileSoftDeletePlan plan =
      planSoftDeleteSaveFile(request.saveRoot, request.saveId);
  result.paths = plan.paths;
  if (!plan.ok) {
    result.status = plan.reason;
    result.reasonCode = plan.reason;
    result.softDeleteReason = plan.reason;
    return result;
  }

  const SaveFileSoftDeleteResult deleted = softDeleteSaveFile(plan);
  result.paths = deleted.paths;
  result.softDeleteReason = deleted.reason;
  result.saveMoved = deleted.saveMoved;
  result.snapshotMoved = deleted.snapshotMoved;
  result.snapshotMissing = deleted.snapshotMissing;
  result.targetExisted = deleted.targetExisted;
  result.snapshotTargetExisted = deleted.snapshotTargetExisted;
  if (!deleted.ok) {
    result.status = deleted.reason;
    result.reasonCode = deleted.reason;
    return result;
  }

  result.ok = true;
  result.status = "product_save_soft_deleted";
  result.reasonCode = "product_save_soft_deleted";
  return result;
}

ProductSaveRecoverResult recoverProductSave(
    const ProductSaveRecoverRequest& request) {
  ProductSaveRecoverResult result;
  if (request.saveId.empty()) {
    result.status = "product_save_recover_id_missing";
    result.reasonCode = "product_save_recover_id_missing";
    return result;
  }
  result.saveId = request.saveId;

  const SaveFileRecoverPlan plan =
      planRecoverDeletedSaveFile(request.saveRoot, request.saveId);
  result.paths = plan.paths;
  if (!plan.ok) {
    result.status = plan.reason;
    result.reasonCode = plan.reason;
    result.recoverReason = plan.reason;
    return result;
  }

  const SaveFileRecoverResult recovered = recoverDeletedSaveFile(plan);
  result.paths = recovered.paths;
  result.recoverReason = recovered.reason;
  result.saveRecovered = recovered.saveRecovered;
  result.snapshotRecovered = recovered.snapshotRecovered;
  result.snapshotMissing = recovered.snapshotMissing;
  result.targetExisted = recovered.targetExisted;
  result.snapshotTargetExisted = recovered.snapshotTargetExisted;
  if (!recovered.ok) {
    result.status = recovered.reason;
    result.reasonCode = recovered.reason;
    return result;
  }

  result.ok = true;
  result.status = "product_save_recovered";
  result.reasonCode = "product_save_recovered";
  return result;
}

ProductSaveMutationResult softDeleteProductSaveAndRefresh(
    const ProductSaveMutationRequest& request) {
  ProductSaveMutationResult result;
  // branch-gate: BG-1218
  result.affectedSaveId = request.saveId.empty() ? "none" : request.saveId;
  result.softDelete = softDeleteProductSave({request.saveRoot, request.saveId});
  result.ok = result.softDelete.ok;
  result.status = mutationStatusForSoftDelete(result.softDelete);
  result.reasonCode = result.softDelete.reasonCode;
  result.activeSaves =
      scanProductSaves(request.saveRoot, request.packageId, request.scenarioId);
  result.deletedSaves =
      scanDeletedProductSaves(request.saveRoot, request.packageId, request.scenarioId);
  return result;
}

ProductSaveMutationResult recoverProductSaveAndRefresh(
    const ProductSaveMutationRequest& request) {
  ProductSaveMutationResult result;
  // branch-gate: BG-1218
  result.affectedSaveId = request.saveId.empty() ? "none" : request.saveId;
  result.recover = recoverProductSave({request.saveRoot, request.saveId});
  result.ok = result.recover.ok;
  result.status = mutationStatusForRecover(result.recover);
  result.reasonCode = result.recover.reasonCode;
  result.activeSaves =
      scanProductSaves(request.saveRoot, request.packageId, request.scenarioId);
  result.deletedSaves =
      scanDeletedProductSaves(request.saveRoot, request.packageId, request.scenarioId);
  return result;
}

}  // namespace iggy3d

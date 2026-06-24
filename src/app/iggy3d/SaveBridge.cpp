#include "app/iggy3d/SaveBridge.hpp"

namespace iggy3d {

ProductSaveBridgeResult scanProductSaves(const std::filesystem::path& saveRoot,
                                         std::string_view packageId,
                                         std::string_view scenarioId) {
  ProductSaveBridgeResult result;
  result.saveRoot = saveRoot;
  result.slots = buildSaveSlotList(saveRoot, packageId, scenarioId);
  return result;
}

ProductSaveBridgeResult scanDeletedProductSaves(
    const std::filesystem::path& saveRoot,
    std::string_view packageId,
    std::string_view scenarioId) {
  ProductSaveBridgeResult result;
  result.saveRoot = saveRoot / "deleted";
  result.slots = buildSaveSlotList(result.saveRoot, packageId, scenarioId);
  result.status = "deleted_save_bridge_ready";
  return result;
}

ProductSaveWriteResult writeProductSessionSaveDurably(
    const ProductSaveWriteRequest& request) {
  ProductSaveWriteResult result;
  result.durableWriteRequested = true;
  result.worldId = request.worldId;
  result.worldTitle = request.worldTitle;
  result.saveTitle = request.saveTitle;
  result.saveType = request.saveType;
  result.createdAtUtc = request.createdAtUtc;
  result.savedAtUtc = request.savedAtUtc;

  SaveFileDurableWriteRequest durableRequest;
  durableRequest.root = request.saveRoot;
  durableRequest.idHint = request.saveIdHint;
  durableRequest.attemptToken = request.attemptToken;
  durableRequest.state = request.state;
  durableRequest.authoredRoom = request.authoredRoom;
  durableRequest.productMetadata.worldId = request.worldId;
  durableRequest.productMetadata.worldTitle = request.worldTitle;
  durableRequest.productMetadata.saveTitle = request.saveTitle;
  durableRequest.productMetadata.saveType = request.saveType;
  durableRequest.productMetadata.createdAtUtc = request.createdAtUtc;
  durableRequest.productMetadata.savedAtUtc = request.savedAtUtc;

  const SaveFileDurableWriteResult durable =
      writeSessionSaveFileDurably(durableRequest);
  result.durableReason = durable.reason;
  result.paths = durable.paths;
  result.record = durable.record;
  result.encodedBytes = durable.encodedBytes;
  result.tempWritten = durable.tempWritten;
  result.tempValidated = durable.tempValidated;
  result.committed = durable.committed;
  result.finalValidated = durable.finalValidated;
  result.previousExisted = durable.previousExisted;
  result.previousPreserved = durable.previousPreserved;

  if (!durable.ok) {
    result.status = durable.reason;
    result.reasonCode = durable.reason;
    return result;
  }

  result.ok = true;
  result.status = "product_save_written";
  result.reasonCode = "product_save_written";
  return result;
}

ProductSaveLoadResult loadProductSessionSave(
    const ProductSaveLoadRequest& request) {
  ProductSaveLoadResult result;
  if (request.session == nullptr) {
    result.status = "product_save_load_session_missing";
    result.reasonCode = "product_save_load_session_missing";
    return result;
  }
  if (request.path.empty()) {
    result.status = "product_save_load_path_missing";
    result.reasonCode = "product_save_load_path_missing";
    return result;
  }

  result.previousHash = request.session->stateHash();
  const SaveFileReadResult read = readSaveFile(request.path);
  result.codecStatus = read.codecStatus;
  if (!read.ok) {
    result.status = read.reason;
    result.reasonCode = read.reason;
    return result;
  }
  result.fileRead = true;
  result.decoded = true;
  result.record = read.record;

  SaveEnvelope compatibilityEnvelope;
  const SaveCompatibilityRequest compatibilityRequest{
      compatibilityEnvelope,
      request.expectedPackageId,
      request.expectedScenarioId,
  };
  const LoadStateResult loaded =
      loadEncodedSaveIntoSession(*request.session, read.encodedText,
                                 compatibilityRequest);
  result.loadStatus = loaded.status;
  result.codecStatus = loaded.codecStatus;
  result.compatibilityStatus = loaded.compatibilityStatus;
  result.sessionLoadStatus = loaded.sessionLoadStatus;
  result.previousHash = loaded.previousHash;
  result.loadedHash = loaded.loadedHash;
  result.compatibilityChecked = loaded.status != SaveLoadStatus::DecodeFailed;

  if (loaded.status == SaveLoadStatus::CompatibilityFailed) {
    result.status = "product_save_load_compatibility_failed";
    result.reasonCode = "product_save_load_compatibility_failed";
    return result;
  }
  if (loaded.status == SaveLoadStatus::DecodeFailed) {
    result.status = "product_save_load_decode_failed";
    result.reasonCode = "product_save_load_decode_failed";
    return result;
  }
  if (loaded.status != SaveLoadStatus::Ok) {
    result.status = "product_save_load_failed";
    result.reasonCode = "product_save_load_failed";
    return result;
  }

  result.ok = true;
  result.status = "product_save_loaded";
  result.reasonCode = "product_save_loaded";
  result.sessionLoaded = true;
  return result;
}

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

}  // namespace iggy3d

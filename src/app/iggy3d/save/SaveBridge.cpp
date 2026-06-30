#include "app/iggy3d/save/SaveBridge.hpp"

#include <chrono>

#include "app/iggy3d/save/CatalogProjector.hpp"
#include "runtime/save/SaveCodec.hpp"

namespace iggy3d {
namespace {

std::string timestampLabelForPath(const std::filesystem::path& path) {
  std::error_code error;
  const auto writeTime = std::filesystem::last_write_time(path, error);
  // branch-gate: BG-1218
  if (error) {
    return "unknown";
  }
  const auto seconds =
      std::chrono::duration_cast<std::chrono::seconds>(writeTime.time_since_epoch())
          .count();
  return "file_time_" + std::to_string(seconds);
}

void populateSnapshotInfo(ProductSaveCatalogEntry& entry) {
  entry.snapshotPath = saveSnapshotPathForFilePath(entry.path);
  entry.snapshotAvailable = false;
  entry.snapshotStatus = "missing";

  std::error_code error;
  // branch-gate: BG-1218
  if (!std::filesystem::exists(entry.snapshotPath, error)) {
    // branch-gate: BG-1218
    entry.snapshotStatus = error ? "unavailable" : "missing";
    return;
  }
  // branch-gate: BG-1218
  if (!std::filesystem::is_regular_file(entry.snapshotPath, error)) {
    entry.snapshotStatus = "unavailable";
    return;
  }
  const auto size = std::filesystem::file_size(entry.snapshotPath, error);
  // branch-gate: BG-1218
  if (error) {
    entry.snapshotStatus = "unavailable";
    return;
  }
  // branch-gate: BG-1218
  if (size == 0U) {
    entry.snapshotStatus = "empty";
    return;
  }
  entry.snapshotAvailable = true;
  entry.snapshotStatus = "available";
}

std::string compatibilityReason(std::string_view packageId,
                                std::string_view scenarioId,
                                std::string_view expectedPackageId,
                                std::string_view expectedScenarioId) {
  // branch-gate: BG-1218
  if (!expectedPackageId.empty() && packageId != expectedPackageId) {
    return "incompatible_package";
  }
  // branch-gate: BG-1218
  if (!expectedScenarioId.empty() && scenarioId != expectedScenarioId) {
    return "incompatible_scenario";
  }
  return "compatible";
}

ProductSaveCatalogEntry baseCatalogEntryForPath(
    const std::filesystem::path& path,
    ProductSaveCatalogLocation location) {
  ProductSaveCatalogEntry entry;
  entry.saveId = saveFileIdFromPath(path);
  entry.path = path;
  entry.location = location;
  entry.deleted = location == ProductSaveCatalogLocation::Deleted;
  // branch-gate: BG-1218
  entry.displayTitle = entry.saveId.empty() ? "save" : entry.saveId;
  entry.savedAtUtc = timestampLabelForPath(path);
  populateSnapshotInfo(entry);
  return entry;
}

ProductSaveCatalogEntry catalogEntryForSavePath(
    const std::filesystem::path& path,
    ProductSaveCatalogLocation location,
    std::string_view expectedPackageId,
    std::string_view expectedScenarioId) {
  ProductSaveCatalogEntry entry = baseCatalogEntryForPath(path, location);
  const SaveFileReadResult read = readSaveFile(path);
  // branch-gate: BG-1218
  if (!read.ok) {
    entry.corrupt = true;
    entry.compatible = false;
    entry.loadable = false;
    entry.recoverable = false;
    entry.disabledReason = read.reason;
    return entry;
  }

  const SaveDecodeResult decoded = decodeSaveEnvelope(read.encodedText);
  // branch-gate: BG-1218
  if (decoded.status != SaveCodecStatus::Ok) {
    entry.corrupt = true;
    entry.compatible = false;
    entry.loadable = false;
    entry.recoverable = false;
    entry.disabledReason = "save_file_decode_failed";
    return entry;
  }

  entry.packageId = decoded.envelope.metadata.packageId;
  entry.scenarioId = decoded.envelope.metadata.scenarioId;
  entry.currentTick = decoded.envelope.session.currentTick;
  entry.savedStateHashHex = decoded.envelope.metadata.savedStateHashHex;
  entry.worldId = decoded.envelope.metadata.worldId;
  entry.worldTitle = decoded.envelope.metadata.worldTitle;
  entry.saveTitle = decoded.envelope.metadata.saveTitle;
  entry.saveType = decoded.envelope.metadata.saveType;
  entry.createdAtUtc = decoded.envelope.metadata.createdAtUtc;
  // branch-gate: BG-1218
  if (!decoded.envelope.metadata.savedAtUtc.empty()) {
    entry.savedAtUtc = decoded.envelope.metadata.savedAtUtc;
  }
  entry.authoredFloorCount =
      static_cast<std::uint64_t>(decoded.envelope.authoredRoom.floors.size());
  entry.authoredWallCount =
      static_cast<std::uint64_t>(decoded.envelope.authoredRoom.walls.size());
  entry.authoredObjectCount =
      static_cast<std::uint64_t>(decoded.envelope.authoredRoom.objects.size());
  entry.authoredMarkerCount =
      static_cast<std::uint64_t>(decoded.envelope.authoredRoom.markers.size());

  const std::string reason =
      compatibilityReason(entry.packageId,
                          entry.scenarioId,
                          expectedPackageId,
                          expectedScenarioId);
  entry.compatible = reason == "compatible";
  entry.corrupt = false;
  entry.loadable =
      location == ProductSaveCatalogLocation::Active && entry.compatible;
  entry.recoverable =
      location == ProductSaveCatalogLocation::Deleted && entry.compatible;
  // branch-gate: BG-1218
  entry.disabledReason = entry.compatible ? "none" : reason;
  return entry;
}

ProductSaveCatalogBuildResult scanProductSaveCatalog(
    const std::filesystem::path& scanRoot,
    ProductSaveCatalogLocation location,
    std::string_view packageId,
    std::string_view scenarioId) {
  std::vector<ProductSaveCatalogEntry> entries;
  for (const std::filesystem::path& path : listSaveFilePaths(scanRoot)) {
    entries.push_back(catalogEntryForSavePath(path, location, packageId, scenarioId));
  }
  return buildProductSaveCatalog(std::move(entries));
}

ProductSaveBridgeResult bridgeResultFromCatalog(
    const std::filesystem::path& scanRoot,
    ProductSaveCatalogBuildResult catalog,
    std::string_view status) {
  ProductSaveBridgeResult result;
  result.saveRoot = scanRoot;
  result.catalog = std::move(catalog);
  result.slots = buildSaveSlotListFromCatalog(result.catalog.catalog);
  result.status = status;
  return result;
}

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

std::string_view productSaveMutationStatusName(ProductSaveMutationStatus status) {
  // branch-gate: BG-1218
  switch (status) {
    case ProductSaveMutationStatus::Succeeded:
      return "succeeded";
    case ProductSaveMutationStatus::SaveNotFound:
      return "save_not_found";
    case ProductSaveMutationStatus::NotActiveSave:
      return "not_active_save";
    case ProductSaveMutationStatus::FileOperationFailed:
      return "file_operation_failed";
    case ProductSaveMutationStatus::SnapshotMoveFailed:
      return "snapshot_move_failed";
    case ProductSaveMutationStatus::DecodeFailedAfterMutation:
      return "decode_failed_after_mutation";
  }
  return "file_operation_failed";
}

ProductSaveBridgeResult scanProductSaves(const std::filesystem::path& saveRoot,
                                         std::string_view packageId,
                                         std::string_view scenarioId) {
  return bridgeResultFromCatalog(
      saveRoot,
      scanProductSaveCatalog(saveRoot,
                             ProductSaveCatalogLocation::Active,
                             packageId,
                             scenarioId),
      "save_bridge_ready");
}

ProductSaveBridgeResult scanDeletedProductSaves(
    const std::filesystem::path& saveRoot,
    std::string_view packageId,
    std::string_view scenarioId) {
  const std::filesystem::path deletedRoot = deletedSaveDirectory(saveRoot);
  return bridgeResultFromCatalog(
      deletedRoot,
      scanProductSaveCatalog(deletedRoot,
                             ProductSaveCatalogLocation::Deleted,
                             packageId,
                             scenarioId),
      "deleted_save_bridge_ready");
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
  const SaveDecodeResult decoded = decodeSaveEnvelope(read.encodedText);
  result.codecStatus = decoded.status;
  if (decoded.status == SaveCodecStatus::Ok &&
      decoded.envelope.authoredRoom.present) {
    result.authoredRoomPresent = true;
    result.authoredRoom = decoded.envelope.authoredRoom;
    result.authoredRoomId =
        result.authoredRoom.id.empty() ? "none" : result.authoredRoom.id;
    result.authoredFloorCount =
        static_cast<std::uint64_t>(result.authoredRoom.floors.size());
    result.authoredWallCount =
        static_cast<std::uint64_t>(result.authoredRoom.walls.size());
    result.authoredObjectCount =
        static_cast<std::uint64_t>(result.authoredRoom.objects.size());
    result.authoredMarkerCount =
        static_cast<std::uint64_t>(result.authoredRoom.markers.size());
  }

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

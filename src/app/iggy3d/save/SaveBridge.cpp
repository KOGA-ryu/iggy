#include "app/iggy3d/save/SaveBridge.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <utility>

#include "app/iggy3d/save/CatalogProjector.hpp"
#include "runtime/save/SaveCodec.hpp"

namespace iggy3d {
namespace {

// Product identity carried on a save's durable metadata. Read back from an
// existing save so a re-save preserves the world identity that creation set
// instead of overwriting it with blanks.
struct ExistingSaveIdentity {
  bool found = false;
  std::string worldId;
  std::string worldTitle;
  std::string saveTitle;
  std::string saveType;
  std::string createdAtUtc;
  std::string savedAtUtc;
};

ExistingSaveIdentity readExistingSaveIdentity(const std::filesystem::path& root,
                                              std::string_view idHint) {
  ExistingSaveIdentity existing;
  if (!isValidSaveFileId(idHint)) {
    return existing;
  }
  const std::filesystem::path path = saveFilePathForId(root, idHint);
  std::error_code error;
  if (!std::filesystem::exists(path, error) || error) {
    return existing;
  }
  const SaveFileReadResult read = readSaveFile(path);
  if (!read.ok) {
    return existing;
  }
  const SaveDecodeResult decoded = decodeSaveEnvelope(read.encodedText);
  if (decoded.status != SaveCodecStatus::Ok) {
    return existing;
  }
  existing.found = true;
  existing.worldId = decoded.envelope.metadata.worldId;
  existing.worldTitle = decoded.envelope.metadata.worldTitle;
  existing.saveTitle = decoded.envelope.metadata.saveTitle;
  existing.saveType = decoded.envelope.metadata.saveType;
  existing.createdAtUtc = decoded.envelope.metadata.createdAtUtc;
  existing.savedAtUtc = decoded.envelope.metadata.savedAtUtc;
  return existing;
}

std::uint64_t elapsedMicroseconds(
    std::chrono::steady_clock::time_point started) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - started)
          .count());
}

std::string preferNonEmpty(const std::string& primary,
                           const std::string& fallback) {
  return primary.empty() ? fallback : primary;
}

void mirrorCreativeMetadata(ProductCreativeSaveWriteResult& result,
                            const ProductCreativeSaveWriteRequest& request,
                            const ExistingSaveIdentity& existing) {
  result.packageId = request.packageId;
  result.scenarioId = request.scenarioId;
  result.worldId = preferNonEmpty(request.worldId, existing.worldId);
  result.worldTitle = preferNonEmpty(request.worldTitle, existing.worldTitle);
  result.saveTitle = preferNonEmpty(request.saveTitle, existing.saveTitle);
  result.saveType = preferNonEmpty(request.saveType, existing.saveType);
  result.createdAtUtc =
      preferNonEmpty(request.createdAtUtc, existing.createdAtUtc);
  result.savedAtUtc = preferNonEmpty(request.savedAtUtc, existing.savedAtUtc);
}

void mirrorCreativeMetadata(ProductCreativeSaveLoadResult& result,
                            const SaveEnvelope& envelope) {
  result.packageId = envelope.metadata.packageId;
  result.scenarioId = envelope.metadata.scenarioId;
  result.worldId = envelope.metadata.worldId;
  result.worldTitle = envelope.metadata.worldTitle;
  result.saveTitle = envelope.metadata.saveTitle;
  result.saveType = envelope.metadata.saveType;
  result.createdAtUtc = envelope.metadata.createdAtUtc;
  result.savedAtUtc = envelope.metadata.savedAtUtc;
}

void mirrorCreativeSectionReceipt(
    ProductCreativeSaveWriteResult& result,
    const ProductCreativeDocumentSectionReceipt& receipt) {
  result.sectionReceipt = receipt;
  result.documentId = receipt.documentId;
  result.creativeObjectCount = receipt.objectCount;
  result.creativeNextObjectId = receipt.nextObjectId;
}

void mirrorCreativeSectionReceipt(
    ProductCreativeSaveLoadResult& result,
    const ProductCreativeDocumentSectionReceipt& receipt) {
  result.sectionReceipt = receipt;
  result.documentId = receipt.documentId;
  result.creativeObjectCount = receipt.objectCount;
  result.creativeNextObjectId = receipt.nextObjectId;
}

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
  entry.contentKind = decoded.envelope.creativeDocument.present
                          ? ProductSaveContentKind::CreativeDocument
                          : ProductSaveContentKind::ProductSession;
  if (decoded.envelope.creativeDocument.present) {
    const SaveCreativeDocumentSection& creative =
        decoded.envelope.creativeDocument;
    entry.creativeDocumentPresent = true;
    entry.creativeDocumentId = creative.documentId;
    entry.creativeObjectCount =
        static_cast<std::uint64_t>(creative.objects.size());
    entry.creativeNextObjectId = creative.nextObjectId;
  }

  const std::string reason =
      compatibilityReason(entry.packageId,
                          entry.scenarioId,
                          expectedPackageId,
                          expectedScenarioId);
  entry.compatible = reason == "compatible";
  entry.corrupt = false;
  entry.loadable = location == ProductSaveCatalogLocation::Active &&
                   entry.compatible &&
                   entry.contentKind == ProductSaveContentKind::ProductSession;
  entry.recoverable =
      location == ProductSaveCatalogLocation::Deleted && entry.compatible;
  if (!entry.compatible) {
    entry.disabledReason = reason;
  } else if (location == ProductSaveCatalogLocation::Active &&
             entry.contentKind == ProductSaveContentKind::CreativeDocument) {
    entry.disabledReason = "creative_save_not_product_loadable";
  } else {
    entry.disabledReason = "none";
  }
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
    std::string_view status,
    std::uint64_t scanMicroseconds,
    std::string_view scanStatus) {
  ProductSaveBridgeResult result;
  result.saveRoot = scanRoot;
  result.catalog = std::move(catalog);
  result.slots = buildSaveSlotListFromCatalog(result.catalog.catalog);
  result.status = status;
  result.scanMeasured = true;
  result.scanMicroseconds = scanMicroseconds;
  result.scanEntryCount =
      static_cast<std::uint64_t>(result.catalog.catalog.entries.size());
  result.scanStatus = scanStatus;
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

std::string productSaveTimestampNowUtc() {
  const std::time_t now = std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::now());
  std::tm utc{};
#if defined(_WIN32)
  gmtime_s(&utc, &now);
#else
  gmtime_r(&now, &utc);
#endif
  char buffer[32] = {0};
  // ISO-8601 UTC, e.g. 2026-06-30T05:47:12Z
  if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc) == 0U) {
    return "";
  }
  return std::string(buffer);
}

namespace {

std::uint64_t maxWorldIdNumber(const ProductSaveCatalog& catalog) {
  constexpr std::string_view kPrefix = "world_";
  std::uint64_t maxNumber = 0;
  for (const ProductSaveCatalogEntry& entry : catalog.entries) {
    if (entry.worldId.size() <= kPrefix.size() ||
        entry.worldId.compare(0, kPrefix.size(), kPrefix) != 0) {
      continue;
    }
    std::uint64_t value = 0;
    bool allDigits = true;
    for (std::size_t index = kPrefix.size(); index < entry.worldId.size();
         ++index) {
      const char character = entry.worldId[index];
      if (character < '0' || character > '9') {
        allDigits = false;
        break;
      }
      value = value * 10U + static_cast<std::uint64_t>(character - '0');
    }
    if (allDigits) {
      maxNumber = std::max(maxNumber, value);
    }
  }
  return maxNumber;
}

std::string formatWorldId(std::uint64_t number) {
  std::string digits = std::to_string(number);
  std::string padding(digits.size() < 4U ? 4U - digits.size() : 0U, '0');
  return "world_" + padding + digits;
}

}  // namespace

ProductWorldIdMintResult nextProductWorldIdMeasured(
    const std::filesystem::path& saveRoot) {
  // World id collection is compatibility-independent: scan with empty
  // package/scenario so every existing world_<n> contributes, then take the
  // highest across active and deleted saves and return the next.
  ProductWorldIdMintResult result;
  const auto started = std::chrono::steady_clock::now();
  const ProductSaveBridgeResult active = scanProductSaves(saveRoot, "", "");
  const ProductSaveBridgeResult deleted =
      scanDeletedProductSaves(saveRoot, "", "");
  const std::uint64_t activeMax = maxWorldIdNumber(active.catalog.catalog);
  const std::uint64_t deletedMax = maxWorldIdNumber(deleted.catalog.catalog);
  result.worldId = formatWorldId(std::max(activeMax, deletedMax) + 1U);
  result.scanMeasured = true;
  result.scanMicroseconds = elapsedMicroseconds(started);
  result.scanEntryCount = active.scanEntryCount + deleted.scanEntryCount;
  result.scanStatus = "product_world_id_scan_ready";
  return result;
}

std::string nextProductWorldId(const std::filesystem::path& saveRoot) {
  return nextProductWorldIdMeasured(saveRoot).worldId;
}

ProductSaveBridgeResult scanProductSaves(const std::filesystem::path& saveRoot,
                                         std::string_view packageId,
                                         std::string_view scenarioId) {
  const auto started = std::chrono::steady_clock::now();
  ProductSaveCatalogBuildResult catalog =
      scanProductSaveCatalog(saveRoot,
                             ProductSaveCatalogLocation::Active,
                             packageId,
                             scenarioId);
  return bridgeResultFromCatalog(
      saveRoot,
      std::move(catalog),
      "save_bridge_ready",
      elapsedMicroseconds(started),
      "save_catalog_scan_ready");
}

ProductSaveBridgeResult scanDeletedProductSaves(
    const std::filesystem::path& saveRoot,
    std::string_view packageId,
    std::string_view scenarioId) {
  const std::filesystem::path deletedRoot = deletedSaveDirectory(saveRoot);
  const auto started = std::chrono::steady_clock::now();
  ProductSaveCatalogBuildResult catalog =
      scanProductSaveCatalog(deletedRoot,
                             ProductSaveCatalogLocation::Deleted,
                             packageId,
                             scenarioId);
  return bridgeResultFromCatalog(
      deletedRoot,
      std::move(catalog),
      "deleted_save_bridge_ready",
      elapsedMicroseconds(started),
      "deleted_save_catalog_scan_ready");
}

ProductSaveWriteResult writeProductSessionSaveDurably(
    const ProductSaveWriteRequest& request) {
  // Identity carry-forward: when overwriting an existing save, any identity
  // field the caller leaves empty inherits the value already on disk. This is
  // what stops a pause/progress save (which only knows the save id) from
  // blanking the worldId/worldTitle/saveType/createdAtUtc that world creation
  // wrote. A caller that supplies a field (e.g. saveType="manual" on a manual
  // save, or a future rename) still overrides it. New saves (no existing file)
  // keep the request values verbatim.
  const ExistingSaveIdentity existing =
      readExistingSaveIdentity(request.saveRoot, request.saveIdHint);
  const std::string worldId = preferNonEmpty(request.worldId, existing.worldId);
  const std::string worldTitle =
      preferNonEmpty(request.worldTitle, existing.worldTitle);
  const std::string saveTitle =
      preferNonEmpty(request.saveTitle, existing.saveTitle);
  const std::string saveType =
      preferNonEmpty(request.saveType, existing.saveType);
  const std::string createdAtUtc =
      preferNonEmpty(request.createdAtUtc, existing.createdAtUtc);
  const std::string savedAtUtc =
      preferNonEmpty(request.savedAtUtc, existing.savedAtUtc);

  ProductSaveWriteResult result;
  result.durableWriteRequested = true;
  result.worldId = worldId;
  result.worldTitle = worldTitle;
  result.saveTitle = saveTitle;
  result.saveType = saveType;
  result.createdAtUtc = createdAtUtc;
  result.savedAtUtc = savedAtUtc;

  SaveFileDurableWriteRequest durableRequest;
  durableRequest.root = request.saveRoot;
  durableRequest.idHint = request.saveIdHint;
  durableRequest.attemptToken = request.attemptToken;
  durableRequest.state = request.state;
  durableRequest.authoredRoom = request.authoredRoom;
  durableRequest.productMetadata.worldId = worldId;
  durableRequest.productMetadata.worldTitle = worldTitle;
  durableRequest.productMetadata.saveTitle = saveTitle;
  durableRequest.productMetadata.saveType = saveType;
  durableRequest.productMetadata.createdAtUtc = createdAtUtc;
  durableRequest.productMetadata.savedAtUtc = savedAtUtc;

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

ProductCreativeSaveWriteResult writeCreativeDocumentSaveDurably(
    const ProductCreativeSaveWriteRequest& request) {
  ProductCreativeSaveWriteResult result;
  result.packageId = request.packageId;
  result.scenarioId = request.scenarioId;
  result.worldId = request.worldId;
  result.worldTitle = request.worldTitle;
  result.saveTitle = request.saveTitle;
  result.saveType = request.saveType;
  result.createdAtUtc = request.createdAtUtc;
  result.savedAtUtc = request.savedAtUtc;

  if (request.document == nullptr) {
    result.status = "creative_save_document_missing";
    result.reasonCode = "creative_save_document_missing";
    result.durableReason = "creative_save_document_missing";
    return result;
  }

  const ProductCreativeDocumentSectionBuildResult section =
      buildSaveCreativeDocumentSection(*request.document);
  mirrorCreativeSectionReceipt(result, section.receipt);
  if (!section.receipt.accepted) {
    const std::string reason{section.receipt.reasonCode};
    result.status = reason;
    result.reasonCode = reason;
    result.durableReason = reason;
    return result;
  }

  const ExistingSaveIdentity existing =
      readExistingSaveIdentity(request.saveRoot, request.saveIdHint);
  mirrorCreativeMetadata(result, request, existing);

  SaveEnvelope envelope;
  envelope.metadata.packageId = result.packageId;
  envelope.metadata.scenarioId = result.scenarioId;
  envelope.metadata.worldId = result.worldId;
  envelope.metadata.worldTitle = result.worldTitle;
  envelope.metadata.saveTitle = result.saveTitle;
  envelope.metadata.saveType = result.saveType;
  envelope.metadata.createdAtUtc = result.createdAtUtc;
  envelope.metadata.savedAtUtc = result.savedAtUtc;
  envelope.metadata.savedStateHash = 0;
  envelope.metadata.savedStateHashHex = "0000000000000000";
  envelope.creativeDocument = section.section;
  if (request.creativeWorldLayoutEncoded != nullptr) {
    if (request.creativeWorldLayoutEncoded->empty()) {
      result.status = "creative_world_layout_encoded_empty";
      result.reasonCode = result.status;
      result.durableReason = result.status;
      return result;
    }
    constexpr std::size_t kMaxCreativeWorldLayoutEncodedBytes =
        8U * 1024U * 1024U;
    if (request.creativeWorldLayoutEncoded->size() >
        kMaxCreativeWorldLayoutEncodedBytes) {
      result.status = "creative_world_layout_encoded_size_exceeded";
      result.reasonCode = result.status;
      result.durableReason = result.status;
      return result;
    }
    envelope.creativeWorldLayout.present = true;
    envelope.creativeWorldLayout.version =
        request.creativeWorldLayoutVersion;
    envelope.creativeWorldLayout.encodedText =
        *request.creativeWorldLayoutEncoded;
    result.creativeWorldLayoutPresent = true;
    result.creativeWorldLayoutVersion = request.creativeWorldLayoutVersion;
    result.creativeWorldLayoutEncodedBytes =
        request.creativeWorldLayoutEncoded->size();
  }

  SaveFileEnvelopeDurableWriteRequest durableRequest;
  durableRequest.root = request.saveRoot;
  durableRequest.idHint = request.saveIdHint;
  durableRequest.attemptToken = request.attemptToken;
  durableRequest.envelope = std::move(envelope);

  result.durableWriteRequested = true;
  const SaveFileDurableWriteResult durable =
      writeSaveEnvelopeFileDurably(durableRequest);
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
  result.status = "creative_save_written";
  result.reasonCode = "creative_save_written";
  return result;
}

ProductCreativeSaveLoadResult loadCreativeDocumentSave(
    const ProductCreativeSaveLoadRequest& request) {
  ProductCreativeSaveLoadResult result;
  if (request.path.empty()) {
    result.status = "creative_save_load_path_missing";
    result.reasonCode = "creative_save_load_path_missing";
    return result;
  }

  const SaveFileReadResult read = readSaveFile(request.path);
  result.codecStatus = read.codecStatus;
  if (!read.ok) {
    const bool decodeFailed = read.reason == "save_file_decode_failed";
    result.status =
        decodeFailed ? "creative_save_decode_failed" : "creative_save_read_failed";
    result.reasonCode = result.status;
    return result;
  }

  result.fileRead = true;
  result.record = read.record;
  const SaveDecodeResult decoded = decodeSaveEnvelope(read.encodedText);
  result.codecStatus = decoded.status;
  if (decoded.status != SaveCodecStatus::Ok) {
    result.status = "creative_save_decode_failed";
    result.reasonCode = "creative_save_decode_failed";
    return result;
  }

  result.decoded = true;
  mirrorCreativeMetadata(result, decoded.envelope);
  result.creativeWorldLayoutPresent =
      decoded.envelope.creativeWorldLayout.present;
  result.creativeWorldLayoutVersion =
      decoded.envelope.creativeWorldLayout.version;
  result.creativeWorldLayoutEncoded =
      decoded.envelope.creativeWorldLayout.encodedText;

  const ProductCreativeDocumentSectionRestoreResult restored =
      restoreCreativeDocumentFromSaveSection(decoded.envelope.creativeDocument);
  mirrorCreativeSectionReceipt(result, restored.receipt);
  if (!restored.receipt.accepted) {
    const std::string reason{restored.receipt.reasonCode};
    result.status = reason;
    result.reasonCode = reason;
    return result;
  }

  result.document = restored.document;
  result.sectionRestored = true;
  result.documentId = result.document.id();
  result.creativeObjectCount = result.document.objectCount();
  result.creativeNextObjectId = result.document.nextObjectId();
  result.ok = true;
  result.status = "creative_save_loaded";
  result.reasonCode = "creative_save_loaded";
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

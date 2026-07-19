#include "app/iggy3d/save/SaveBridge.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <utility>
#include <vector>

#include "app/iggy3d/save/CatalogProjector.hpp"
#include "runtime/save/SaveCodec.hpp"

namespace iggy3d {
namespace {

std::uint64_t elapsedMicroseconds(
    std::chrono::steady_clock::time_point started) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - started)
          .count());
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

}  // namespace iggy3d

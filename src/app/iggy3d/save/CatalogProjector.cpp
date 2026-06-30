#include "app/iggy3d/save/CatalogProjector.hpp"

namespace iggy3d {

SaveSlotCompatibility saveSlotCompatibilityFromCatalogEntry(
    const ProductSaveCatalogEntry& entry) {
  // branch-gate: BG-1220
  if (entry.corrupt || entry.disabledReason == "save_file_decode_failed" ||
      entry.disabledReason == "save_file_read_failed") {
    return SaveSlotCompatibility::DecodeFailed;
  }
  // branch-gate: BG-1220
  if (entry.disabledReason == "incompatible_package") {
    return SaveSlotCompatibility::IncompatiblePackage;
  }
  // branch-gate: BG-1220
  if (entry.disabledReason == "incompatible_scenario") {
    return SaveSlotCompatibility::IncompatibleScenario;
  }
  // branch-gate: BG-1220
  if (entry.compatible) {
    return SaveSlotCompatibility::Compatible;
  }
  // branch-gate: BG-1220
  if (!entry.loadable && entry.disabledReason != "none") {
    return SaveSlotCompatibility::LoadFailed;
  }
  return SaveSlotCompatibility::Unknown;
}

SaveSlotPreview saveSlotPreviewFromCatalogEntry(
    const ProductSaveCatalogEntry& entry) {
  SaveSlotPreview preview;
  preview.id = entry.saveId;
  preview.path = entry.path;
  // branch-gate: BG-1220
  preview.packageId = entry.packageId.empty() ? "none" : entry.packageId;
  // branch-gate: BG-1220
  preview.scenarioId = entry.scenarioId.empty() ? "none" : entry.scenarioId;
  preview.currentTick = entry.currentTick;
  // branch-gate: BG-1220
  preview.savedStateHashHex =
      entry.savedStateHashHex.empty() ? "none" : entry.savedStateHashHex;
  preview.authoredFloorCount = entry.authoredFloorCount;
  preview.authoredWallCount = entry.authoredWallCount;
  preview.authoredMarkerCount = entry.authoredMarkerCount;
  preview.compatibility = saveSlotCompatibilityFromCatalogEntry(entry);
  preview.enabled = entry.compatible && !entry.corrupt;
  preview.corrupt = entry.corrupt;
  // branch-gate: BG-1220
  preview.reason = preview.enabled ? "compatible" : entry.disabledReason;
  // branch-gate: BG-1220
  if (preview.reason.empty()) {
    preview.reason = std::string(saveSlotCompatibilityName(preview.compatibility));
  }
  // branch-gate: BG-1220
  preview.displayTitle = entry.displayTitle.empty()
                             ? productSaveDisplayTitle(entry)
                             : entry.displayTitle;
  // branch-gate: BG-1220
  preview.timestampLabel =
      entry.savedAtUtc.empty() ? "unknown" : entry.savedAtUtc;
  preview.snapshotPath = entry.snapshotPath;
  preview.snapshotAvailable = entry.snapshotAvailable;
  preview.snapshotFallback = !entry.snapshotAvailable;
  // branch-gate: BG-1220
  preview.snapshotStatus = entry.snapshotStatus.empty() ? "missing"
                                                        : entry.snapshotStatus;
  return preview;
}

SaveSlotList buildSaveSlotListFromCatalog(const ProductSaveCatalog& catalog) {
  SaveSlotList list;
  list.slots.reserve(catalog.entries.size());
  for (const ProductSaveCatalogEntry& entry : catalog.entries) {
    SaveSlotPreview preview = saveSlotPreviewFromCatalogEntry(entry);
    // branch-gate: BG-1220
    if (preview.enabled) {
      ++list.compatibleCount;
    }
    // branch-gate: BG-1220
    if (preview.corrupt) {
      ++list.corruptCount;
    }
    list.slots.push_back(std::move(preview));
  }
  return list;
}

}  // namespace iggy3d

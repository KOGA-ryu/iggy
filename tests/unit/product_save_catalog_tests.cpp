#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "app/iggy3d/save/Catalog.hpp"

namespace {

using iggy3d::ProductSaveCatalog;
using iggy3d::ProductSaveCatalogEntry;
using iggy3d::ProductSaveCatalogLocation;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

ProductSaveCatalogEntry activeEntry(std::string saveId,
                                    std::string savedAtUtc) {
  ProductSaveCatalogEntry entry;
  entry.saveId = std::move(saveId);
  entry.path = std::filesystem::path{"/tmp"} / (entry.saveId + ".iggy3d.save");
  entry.location = ProductSaveCatalogLocation::Active;
  entry.worldId = "world_001";
  entry.worldTitle = "Training World";
  entry.saveType = "manual";
  entry.savedAtUtc = std::move(savedAtUtc);
  entry.packageId = "iggy3d.product.default";
  entry.scenarioId = "training_ground";
  entry.snapshotStatus = "missing";
  entry.snapshotAvailable = false;
  entry.compatible = true;
  entry.loadable = true;
  entry.disabledReason = "none";
  return entry;
}

ProductSaveCatalogEntry deletedEntry(std::string saveId,
                                     std::string savedAtUtc) {
  ProductSaveCatalogEntry entry = activeEntry(std::move(saveId),
                                              std::move(savedAtUtc));
  entry.location = ProductSaveCatalogLocation::Deleted;
  entry.deleted = true;
  entry.loadable = false;
  entry.recoverable = true;
  entry.path = std::filesystem::path{"/tmp/deleted"} /
               (entry.saveId + ".iggy3d.save");
  return entry;
}

void emptyCatalogYieldsNoContinueSelection() {
  const auto result = iggy3d::selectProductContinueSave(ProductSaveCatalog{});
  expect(!result.selected, "empty catalog should not select continue save");
  expect(result.status == "continue_no_active_saves",
         "empty catalog reports no active saves");
}

void activeCompatibleSaveCanBeSelected() {
  ProductSaveCatalog catalog;
  catalog.entries.push_back(activeEntry("save_001", "2026-06-24T00:00:00Z"));

  const auto result = iggy3d::selectProductContinueSave(catalog);
  expect(result.selected, "compatible active save should be selected");
  expect(result.selectedSaveId == "save_001",
         "selected save id should match compatible active save");
  expect(result.status == "continue_save_selected",
         "compatible active save reports selected status");
}

void newestSavedAtWins() {
  ProductSaveCatalog catalog;
  catalog.entries.push_back(activeEntry("save_001", "2026-06-24T00:00:00Z"));
  catalog.entries.push_back(activeEntry("save_002", "2026-06-24T01:00:00Z"));

  const auto result = iggy3d::selectProductContinueSave(catalog);
  expect(result.selectedSaveId == "save_002", "newest savedAtUtc wins");
}

void sameSavedAtUsesHighestSaveId() {
  ProductSaveCatalog catalog;
  catalog.entries.push_back(activeEntry("save_001", "2026-06-24T01:00:00Z"));
  catalog.entries.push_back(activeEntry("save_003", "2026-06-24T01:00:00Z"));
  catalog.entries.push_back(activeEntry("save_002", "2026-06-24T01:00:00Z"));

  const auto result = iggy3d::selectProductContinueSave(catalog);
  expect(result.selectedSaveId == "save_003",
         "same savedAtUtc tie breaks by highest save id");
}

void deletedSaveIsIgnoredByContinue() {
  ProductSaveCatalog catalog;
  catalog.entries.push_back(activeEntry("save_001", "2026-06-24T00:00:00Z"));
  catalog.entries.push_back(deletedEntry("save_999", "2026-06-24T02:00:00Z"));

  const auto result = iggy3d::selectProductContinueSave(catalog);
  expect(result.selectedSaveId == "save_001",
         "deleted save should never count for continue");
}

void incompatibleCorruptUnloadableRowsRemainRepresentableButIgnored() {
  auto incompatible = activeEntry("save_incompatible", "2026-06-24T03:00:00Z");
  incompatible.compatible = false;
  incompatible.disabledReason = "incompatible_package";

  auto corrupt = activeEntry("save_corrupt", "2026-06-24T04:00:00Z");
  corrupt.corrupt = true;
  corrupt.loadable = false;
  corrupt.disabledReason = "save_file_decode_failed";

  auto unloadable = activeEntry("save_unloadable", "2026-06-24T05:00:00Z");
  unloadable.loadable = false;
  unloadable.disabledReason = "save_load_failed";

  ProductSaveCatalog catalog;
  catalog.entries = {incompatible, corrupt, unloadable,
                     activeEntry("save_001", "2026-06-24T00:00:00Z")};

  expect(!iggy3d::canLoadProductSave(incompatible),
         "incompatible row is not loadable");
  expect(!iggy3d::canLoadProductSave(corrupt), "corrupt row is not loadable");
  expect(!iggy3d::canLoadProductSave(unloadable),
         "unloadable row is not loadable");

  const auto result = iggy3d::selectProductContinueSave(catalog);
  expect(result.selectedSaveId == "save_001",
         "continue ignores incompatible, corrupt, and unloadable rows");
}

void displayTitlePrefersSaveTitleThenWorldTitleThenSaveId() {
  auto entry = activeEntry("save_001", "2026-06-24T00:00:00Z");
  entry.worldTitle = "World Title";
  entry.saveTitle = "Save Title";
  expect(iggy3d::productSaveDisplayTitle(entry) == "Save Title",
         "save title is preferred display title");

  entry.saveTitle.clear();
  expect(iggy3d::productSaveDisplayTitle(entry) == "World Title",
         "world title is display title fallback");

  entry.worldTitle.clear();
  expect(iggy3d::productSaveDisplayTitle(entry) == "save_001",
         "save id is final display title fallback");
}

void missingSnapshotDoesNotBlockLoad() {
  auto entry = activeEntry("save_001", "2026-06-24T00:00:00Z");
  entry.snapshotAvailable = false;
  entry.snapshotStatus = "missing";

  expect(iggy3d::canLoadProductSave(entry),
         "missing snapshot does not make row unloadable");
}

void recoverEligibilityRejectsActiveTargetCollision() {
  const auto active = activeEntry("save_001", "2026-06-24T00:00:00Z");
  const auto deletedCollision = deletedEntry("save_001", "2026-06-24T01:00:00Z");
  const auto deletedNoCollision = deletedEntry("save_002", "2026-06-24T01:00:00Z");
  const std::vector<ProductSaveCatalogEntry> activeEntries{active};

  expect(!iggy3d::canRecoverProductSave(deletedCollision, activeEntries),
         "recover rejects active target collision");
  expect(iggy3d::canRecoverProductSave(deletedNoCollision, activeEntries),
         "recover allows deleted row without active collision");
}

void buildResultCountsAndTitlesAreStable() {
  auto corrupt = activeEntry("save_corrupt", "2026-06-24T01:00:00Z");
  corrupt.corrupt = true;
  corrupt.loadable = false;
  corrupt.displayTitle.clear();
  corrupt.disabledReason = "save_file_decode_failed";

  auto deleted = deletedEntry("save_deleted", "2026-06-24T02:00:00Z");
  deleted.displayTitle.clear();

  auto built = iggy3d::buildProductSaveCatalog(
      {activeEntry("save_001", "2026-06-24T00:00:00Z"), corrupt, deleted});

  expect(built.ok, "catalog build is value-only ready result");
  expect(built.status == "product_save_catalog_ready",
         "catalog build reports ready status");
  expect(built.activeCount == 2, "catalog build counts active entries");
  expect(built.deletedCount == 1, "catalog build counts deleted entries");
  expect(built.compatibleActiveCount == 1,
         "catalog build counts compatible loadable active entries");
  expect(built.recoverableDeletedCount == 1,
         "catalog build counts recoverable deleted entries");
  expect(built.corruptCount == 1, "catalog build counts corrupt entries");
  expect(!built.catalog.entries.front().displayTitle.empty(),
         "catalog build fills display title fallback");
}

void deterministicSortPutsValidNewestBeforeLegacyRows() {
  auto legacy = activeEntry("save_999", "");
  auto newest = activeEntry("save_002", "2026-06-24T01:00:00Z");
  auto older = activeEntry("save_001", "2026-06-24T00:00:00Z");

  const auto sorted = iggy3d::sortProductSaveCatalogEntries({legacy, older, newest});
  expect(sorted.size() == 3, "sorted entry count is stable");
  expect(sorted[0].saveId == "save_002", "newest valid UTC sorts first");
  expect(sorted[1].saveId == "save_001", "older valid UTC sorts second");
  expect(sorted[2].saveId == "save_999", "missing timestamp sorts last");
}

}  // namespace

int main() {
  emptyCatalogYieldsNoContinueSelection();
  activeCompatibleSaveCanBeSelected();
  newestSavedAtWins();
  sameSavedAtUsesHighestSaveId();
  deletedSaveIsIgnoredByContinue();
  incompatibleCorruptUnloadableRowsRemainRepresentableButIgnored();
  displayTitlePrefersSaveTitleThenWorldTitleThenSaveId();
  missingSnapshotDoesNotBlockLoad();
  recoverEligibilityRejectsActiveTargetCollision();
  buildResultCountsAndTitlesAreStable();
  deterministicSortPutsValidNewestBeforeLegacyRows();

  std::cout << "product_save_catalog_tests passed\n";
  return 0;
}

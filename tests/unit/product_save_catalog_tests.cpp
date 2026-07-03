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
using iggy3d::ProductSaveContentKind;

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
  entry.contentKind = ProductSaveContentKind::ProductSession;
  entry.snapshotStatus = "missing";
  entry.snapshotAvailable = false;
  entry.compatible = true;
  entry.loadable = true;
  entry.disabledReason = "none";
  return entry;
}

ProductSaveCatalogEntry creativeEntry(std::string saveId,
                                      std::string savedAtUtc) {
  ProductSaveCatalogEntry entry = activeEntry(std::move(saveId),
                                              std::move(savedAtUtc));
  entry.contentKind = ProductSaveContentKind::CreativeDocument;
  entry.saveType = "creative";
  entry.creativeDocumentPresent = true;
  entry.creativeDocumentId = 9001;
  entry.creativeObjectCount = 2;
  entry.creativeNextObjectId = 100;
  entry.loadable = false;
  entry.disabledReason = "creative_save_not_product_loadable";
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

void contentKindDefaultsAndNamesAreStable() {
  ProductSaveCatalogEntry entry;
  expect(entry.contentKind == ProductSaveContentKind::Unknown,
         "catalog content kind defaults unknown");
  expect(iggy3d::productSaveContentKindName(ProductSaveContentKind::Unknown) ==
             "unknown",
         "unknown content kind name");
  expect(iggy3d::productSaveContentKindName(
             ProductSaveContentKind::ProductSession) == "product_session",
         "product session content kind name");
  expect(iggy3d::productSaveContentKindName(
             ProductSaveContentKind::CreativeDocument) == "creative_document",
         "creative document content kind name");
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

void contentKindSplitsProductLoadAndCreativeOpen() {
  ProductSaveCatalogEntry product =
      activeEntry("save_product", "2026-06-24T00:00:00Z");
  ProductSaveCatalogEntry creative =
      creativeEntry("save_creative", "2026-06-24T01:00:00Z");

  expect(iggy3d::canLoadProductSave(product),
         "product session save is product-loadable");
  expect(!iggy3d::canOpenCreativeWorld(product),
         "product session save is not creative-openable");
  expect(!iggy3d::canLoadProductSave(creative),
         "creative document save is not product-loadable");
  expect(iggy3d::canOpenCreativeWorld(creative),
         "creative document save is creative-openable");
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

void creativeSaveIsIgnoredByProductContinueEvenWhenNewest() {
  ProductSaveCatalog catalog;
  catalog.entries.push_back(activeEntry("save_001", "2026-06-24T00:00:00Z"));
  catalog.entries.push_back(creativeEntry("save_999", "2026-06-24T02:00:00Z"));

  const auto result = iggy3d::selectProductContinueSave(catalog);
  const auto creativeResult = iggy3d::selectCreativeWorldContinueSave(catalog);
  expect(result.selected, "product continue should still select product save");
  expect(result.selectedSaveId == "save_001",
         "creative save should not win product continue");
  expect(result.consideredCount == 2,
         "continue still considers active rows for diagnostics");
  expect(result.compatibleCount == 1,
         "continue compatible count includes only product-loadable rows");
  expect(creativeResult.selected,
         "creative continue should select creative save");
  expect(creativeResult.selectedSaveId == "save_999",
         "creative continue should ignore product save");
  expect(creativeResult.consideredCount == 2,
         "creative selector considers active rows");
  expect(creativeResult.openableCount == 1,
         "creative selector counts only openable creative rows");
}

void emptyCatalogYieldsNoCreativeSelection() {
  const auto result = iggy3d::selectCreativeWorldContinueSave(ProductSaveCatalog{});
  expect(!result.selected, "empty catalog should not select creative save");
  expect(result.status == "creative_open_no_active_saves",
         "empty creative selector reports no active saves");
}

void productOnlyCatalogYieldsNoCreativeSelection() {
  ProductSaveCatalog catalog;
  catalog.entries.push_back(activeEntry("save_001", "2026-06-24T00:00:00Z"));

  const auto result = iggy3d::selectCreativeWorldContinueSave(catalog);
  expect(!result.selected, "product-only catalog should not select creative save");
  expect(result.status == "creative_open_no_openable_saves",
         "product-only creative selector reports no openable saves");
  expect(result.consideredCount == 1,
         "product-only creative selector considers active row");
  expect(result.openableCount == 0,
         "product-only creative selector has zero openable rows");
}

void newestCreativeSavedAtWins() {
  ProductSaveCatalog catalog;
  catalog.entries.push_back(creativeEntry("save_creative_001",
                                          "2026-06-24T00:00:00Z"));
  catalog.entries.push_back(creativeEntry("save_creative_002",
                                          "2026-06-24T01:00:00Z"));

  const auto result = iggy3d::selectCreativeWorldContinueSave(catalog);
  expect(result.selected, "creative selector should select active creative save");
  expect(result.selectedSaveId == "save_creative_002",
         "newest creative savedAtUtc wins");
  expect(result.status == "creative_open_save_selected",
         "creative selector reports selected status");
}

void sameCreativeSavedAtUsesHighestSaveId() {
  ProductSaveCatalog catalog;
  catalog.entries.push_back(creativeEntry("save_creative_001",
                                          "2026-06-24T01:00:00Z"));
  catalog.entries.push_back(creativeEntry("save_creative_003",
                                          "2026-06-24T01:00:00Z"));
  catalog.entries.push_back(creativeEntry("save_creative_002",
                                          "2026-06-24T01:00:00Z"));

  const auto result = iggy3d::selectCreativeWorldContinueSave(catalog);
  expect(result.selectedSaveId == "save_creative_003",
         "same creative savedAtUtc tie breaks by highest save id");
}

void deletedCreativeSaveIsIgnoredByCreativeContinue() {
  ProductSaveCatalog catalog;
  catalog.entries.push_back(deletedEntry("save_creative_999",
                                         "2026-06-24T02:00:00Z"));

  const auto result = iggy3d::selectCreativeWorldContinueSave(catalog);
  expect(!result.selected, "deleted creative save should not be selected");
  expect(result.status == "creative_open_no_active_saves",
         "deleted-only creative selector reports no active saves");
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
  contentKindDefaultsAndNamesAreStable();
  emptyCatalogYieldsNoContinueSelection();
  activeCompatibleSaveCanBeSelected();
  contentKindSplitsProductLoadAndCreativeOpen();
  newestSavedAtWins();
  sameSavedAtUsesHighestSaveId();
  creativeSaveIsIgnoredByProductContinueEvenWhenNewest();
  emptyCatalogYieldsNoCreativeSelection();
  productOnlyCatalogYieldsNoCreativeSelection();
  newestCreativeSavedAtWins();
  sameCreativeSavedAtUsesHighestSaveId();
  deletedCreativeSaveIsIgnoredByCreativeContinue();
  deletedSaveIsIgnoredByContinue();
  incompatibleCorruptUnloadableRowsRemainRepresentableButIgnored();
  displayTitlePrefersSaveTitleThenWorldTitleThenSaveId();
  missingSnapshotDoesNotBlockLoad();
  buildResultCountsAndTitlesAreStable();
  deterministicSortPutsValidNewestBeforeLegacyRows();

  std::cout << "product_save_catalog_tests passed\n";
  return 0;
}

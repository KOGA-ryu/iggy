#include "EditorMapValidationDiagnostics.hpp"
#include "app/iggy3d/creative/history/History.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool descriptorTableIsCompleteAndActionable() {
  const auto descriptors = app::creativeEditorMapDiagnosticDescriptors();
  constexpr std::size_t kExpectedCount =
      static_cast<std::size_t>(
          cr::CreativeMapDiagnosticCode::DiagnosticCapacityExceeded) +
      1U;
  if (!expect(descriptors.size() == kExpectedCount,
              "every map diagnostic code has one descriptor")) {
    return false;
  }

  for (std::size_t index = 0U; index < descriptors.size(); ++index) {
    const auto code = static_cast<cr::CreativeMapDiagnosticCode>(index);
    const app::CreativeEditorMapDiagnosticDescriptor& descriptor =
        app::creativeEditorMapDiagnosticDescriptor(code);
    if (!expect(descriptor.code == code,
                "descriptor lookup preserves contiguous code ownership") ||
        !expect(!descriptor.title.empty(),
                "diagnostic descriptor has a user-facing title") ||
        !expect(!descriptor.remediation.empty(),
                "diagnostic descriptor has a repair action") ||
        !expect(descriptor.area != app::CreativeEditorMapDiagnosticArea::Count,
                "diagnostic descriptor has a visible area")) {
      return false;
    }
  }
  return true;
}

bool validationCacheIsExplicitAndRevisionOwned() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Cache Map");
  if (!expect(document.assignId(91U), "cache fixture document id")) {
    return false;
  }
  iggy3d::StaticMeshAssetCatalog catalog;
  app::CreativeEditorMapValidationCache cache;

  const bool beginsNotRun =
      app::creativeEditorMapValidationCacheStatus(cache, document, &catalog) ==
      app::CreativeEditorMapValidationCacheStatus::NotRun;
  const cr::CreativeMapValidationResult& first =
      app::refreshCreativeEditorMapValidation(cache, document, &catalog);
  const std::uint64_t firstBuildCount = cache.buildCount;
  static_cast<void>(
      app::refreshCreativeEditorMapValidation(cache, document, &catalog));
  const bool idleReused = cache.buildCount == firstBuildCount;
  static_cast<void>(app::refreshCreativeEditorMapValidation(
      cache, document, &catalog, true));
  const bool explicitRerun = cache.buildCount == firstBuildCount + 1U;

  cr::CreativeDocumentCreateRequest floor;
  floor.kind = cr::CreativeObjectKind::Floor;
  floor.name = "Validation Floor";
  floor.bounds = {{-2.0, 0.0, -2.0}, {2.0, 0.25, 2.0}};
  floor.hasBoundsOverride = true;
  if (!expect(document.createObject(floor).accepted,
              "cache fixture document mutation")) {
    return false;
  }
  const bool documentStale =
      app::creativeEditorMapValidationCacheStatus(cache, document, &catalog) ==
      app::CreativeEditorMapValidationCacheStatus::Stale;
  const std::uint64_t beforeDocumentRefresh = cache.buildCount;
  static_cast<void>(
      app::refreshCreativeEditorMapValidation(cache, document, &catalog));
  const bool documentRefreshed =
      cache.buildCount == beforeDocumentRefresh + 1U &&
      cache.documentRevision == document.revision();

  iggy3d::StaticMeshAssetCatalogEntry entry;
  entry.assetId = "validation_asset";
  entry.contentHash = 7U;
  catalog.entries.push_back(entry);
  const bool catalogStale =
      app::creativeEditorMapValidationCacheStatus(cache, document, &catalog) ==
      app::CreativeEditorMapValidationCacheStatus::Stale;
  const std::uint64_t beforeCatalogRefresh = cache.buildCount;
  static_cast<void>(
      app::refreshCreativeEditorMapValidation(cache, document, &catalog));
  const bool catalogRefreshed =
      cache.buildCount == beforeCatalogRefresh + 1U &&
      app::creativeEditorMapValidationCacheStatus(cache, document, &catalog) ==
          app::CreativeEditorMapValidationCacheStatus::Current;

  return expect(beginsNotRun, "validation never runs implicitly") &&
         expect(first.requested && cache.hasResult,
                "explicit validation stores the real kernel result") &&
         expect(idleReused, "unchanged validation result is reused") &&
         expect(explicitRerun, "explicit re-run bypasses current cache") &&
         expect(documentStale,
                "document revision marks validation result stale") &&
         expect(documentRefreshed,
                "stale document refresh runs exactly once") &&
         expect(catalogStale,
                "asset catalog signature marks validation result stale") &&
         expect(catalogRefreshed,
                "stale asset catalog refresh runs exactly once");
}

bool validationCacheCannotHitAnAlternateHistoryBranch() {
  cr::Facade facade;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Branch-Safe Validation Cache");
  static_cast<void>(document.assignId(92U));
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      facade.installDocument(std::move(document));
  cr::CreativeDocumentHistory history;
  iggy3d::StaticMeshAssetCatalog catalog;

  cr::CreativeDocumentHistoryTransaction editA =
      cr::beginCreativeHistoryTransaction(facade, "validation_branch_a");
  cr::CreativeDocumentCreateRequest floorA;
  floorA.kind = cr::CreativeObjectKind::Floor;
  floorA.name = "Floor A";
  floorA.bounds = {{0.0, 0.0, 0.0}, {2.0, 0.25, 2.0}};
  floorA.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt createdA =
      facade.createDocumentObject(floorA);
  const cr::CreativeHistoryRecordReceipt recordedA =
      cr::commitCreativeHistoryTransaction(
          history, std::move(editA), facade);
  const std::uint64_t revisionA = facade.document().revision();

  app::CreativeEditorMapValidationCache cache;
  static_cast<void>(app::refreshCreativeEditorMapValidation(
      cache, facade.document(), &catalog));
  const std::uint64_t buildCountA = cache.buildCount;
  const std::uint64_t cachedRevisionA = cache.documentRevision;

  const cr::CreativeHistoryApplyReceipt undone =
      cr::applyCreativeHistory(
          facade, history, cr::CreativeHistoryDirection::Undo);
  const std::uint64_t revisionAfterUndo = facade.document().revision();

  cr::CreativeDocumentHistoryTransaction editB =
      cr::beginCreativeHistoryTransaction(facade, "validation_branch_b");
  cr::CreativeDocumentCreateRequest floorB = floorA;
  floorB.name = "Floor B";
  floorB.bounds = {{4.0, 0.0, 4.0}, {7.0, 0.25, 7.0}};
  const cr::CreativeDocumentCreateReceipt createdB =
      facade.createDocumentObject(floorB);
  const cr::CreativeHistoryRecordReceipt recordedB =
      cr::commitCreativeHistoryTransaction(
          history, std::move(editB), facade);
  const std::uint64_t revisionB = facade.document().revision();

  const bool alternateBranchIsStale =
      app::creativeEditorMapValidationCacheStatus(
          cache, facade.document(), &catalog) ==
      app::CreativeEditorMapValidationCacheStatus::Stale;
  static_cast<void>(app::refreshCreativeEditorMapValidation(
      cache, facade.document(), &catalog));

  return expect(installed.accepted && installed.initialInstall,
                "validation branch fixture initial install") &&
         expect(createdA.accepted && recordedA.recorded &&
                    cachedRevisionA == revisionA,
                "validation cache is built on edit A") &&
         expect(undone.accepted && revisionAfterUndo > revisionA,
                "validation branch undo advances live revision") &&
         expect(createdB.accepted && recordedB.recorded &&
                    recordedB.clearedRedoCount == 1U &&
                    revisionB > revisionAfterUndo &&
                    revisionB != revisionA,
                "validation alternate branch has a unique revision") &&
         expect(alternateBranchIsStale &&
                    cache.buildCount == buildCountA + 1U &&
                    cache.documentRevision == revisionB &&
                    app::creativeEditorMapValidationCacheStatus(
                        cache, facade.document(), &catalog) ==
                        app::CreativeEditorMapValidationCacheStatus::Current,
                "edit A cache rebuilds exactly once for edit B");
}

bool catalogSignatureDistinguishesUnavailableAndChangedCatalogs() {
  iggy3d::StaticMeshAssetCatalog catalog;
  const std::uint64_t missing =
      app::creativeEditorStaticMeshCatalogSignature(nullptr);
  const std::uint64_t empty =
      app::creativeEditorStaticMeshCatalogSignature(&catalog);
  iggy3d::StaticMeshAssetCatalogEntry entry;
  entry.assetId = "asset";
  entry.contentHash = 1U;
  catalog.entries.push_back(entry);
  const std::uint64_t first =
      app::creativeEditorStaticMeshCatalogSignature(&catalog);
  catalog.entries.front().contentHash = 2U;
  const std::uint64_t second =
      app::creativeEditorStaticMeshCatalogSignature(&catalog);

  return expect(missing != empty,
                "missing and empty asset catalogs are distinct") &&
         expect(empty != first, "catalog membership changes signature") &&
         expect(first != second, "asset content changes signature");
}

}  // namespace

int main() {
  const bool ok = descriptorTableIsCompleteAndActionable() &&
                  validationCacheIsExplicitAndRevisionOwned() &&
                  validationCacheCannotHitAnAlternateHistoryBranch() &&
                  catalogSignatureDistinguishesUnavailableAndChangedCatalogs();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "PASS: creative editor map validation diagnostics\n";
  return EXIT_SUCCESS;
}

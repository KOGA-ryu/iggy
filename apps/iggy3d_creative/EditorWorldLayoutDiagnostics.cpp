#include "EditorWorldLayoutDiagnostics.hpp"

#include "app/iggy3d/creative/input/Catalog.hpp"

#include <bit>
#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d_creative_app {
namespace {

namespace cr = iggy3d::creative;

void appendHashByte(std::uint64_t& hash, std::uint8_t byte) noexcept {
  hash ^= byte;
  hash *= 1099511628211ULL;
}

void appendHashWord(std::uint64_t& hash, std::uint64_t word) noexcept {
  for (std::size_t index = 0U; index < sizeof(word); ++index) {
    appendHashByte(hash, static_cast<std::uint8_t>(word & 0xFFU));
    word >>= 8U;
  }
}

void appendHashBounds(std::uint64_t& hash,
                      cr::CreativeBounds bounds) noexcept {
  appendHashWord(hash, std::bit_cast<std::uint64_t>(bounds.min.x));
  appendHashWord(hash, std::bit_cast<std::uint64_t>(bounds.min.y));
  appendHashWord(hash, std::bit_cast<std::uint64_t>(bounds.min.z));
  appendHashWord(hash, std::bit_cast<std::uint64_t>(bounds.max.x));
  appendHashWord(hash, std::bit_cast<std::uint64_t>(bounds.max.y));
  appendHashWord(hash, std::bit_cast<std::uint64_t>(bounds.max.z));
}

std::uint64_t assetCatalogSignature(
    const cr::CreativeCatalogState* catalog) noexcept {
  if (catalog == nullptr) {
    return 0U;
  }
  std::uint64_t hash = 1469598103934665603ULL;
  for (const cr::CreativeCatalogEntry& entry : catalog->entries) {
    if (entry.category != cr::CreativeCatalogEntryCategory::Asset) {
      continue;
    }
    for (const char byte : cr::creativeHotbarAssetId(entry.hotbarEntry)) {
      appendHashByte(hash, static_cast<std::uint8_t>(byte));
    }
    appendHashByte(hash, 0xFFU);
    appendHashByte(hash, entry.hotbarEntry.hasAssetBounds ? 1U : 0U);
    appendHashBounds(hash, entry.hotbarEntry.assetSourceBounds);
  }
  return hash;
}

const cr::CreativeCatalogEntry* findCatalogAsset(
    const cr::CreativeCatalogState& catalog,
    std::string_view assetId) noexcept {
  for (const cr::CreativeCatalogEntry& entry : catalog.entries) {
    if (entry.category == cr::CreativeCatalogEntryCategory::Asset &&
        cr::creativeHotbarAssetId(entry.hotbarEntry) == assetId) {
      return &entry;
    }
  }
  return nullptr;
}

void appendAssetSourceDiagnostic(
    CreativeEditorWorldLayoutDiagnosticReport& report,
    const cr::CreativeCatalogState& catalog,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    std::string_view name, std::string_view assetId,
    bool hasSourceBounds, cr::CreativeBounds sourceBounds,
    bool active) {
  if (!active || assetId.empty() || !hasSourceBounds ||
      report.issueCount >= report.issues.size()) {
    return;
  }
  const cr::CreativeCatalogEntry* entry = findCatalogAsset(catalog, assetId);
  const bool missing = entry == nullptr;
  const bool stale = !missing &&
                     (!entry->hotbarEntry.hasAssetBounds ||
                      !cr::creativeBoundsExactlyEqual(
                          sourceBounds,
                          entry->hotbarEntry.assetSourceBounds));
  if (!missing && !stale) {
    return;
  }

  CreativeEditorWorldLayoutDiagnostic& issue =
      report.issues[report.issueCount++];
  issue.severity = CreativeEditorWorldLayoutDiagnosticSeverity::Warning;
  issue.status = cr::CreativeWorldLayoutStatus::Ready;
  issue.table = table;
  issue.index = index;
  if (missing) {
    issue.message = std::string(name) +
                    (table == cr::CreativeWorldLayoutTable::Opening
                         ? " asset is unavailable; using a procedural preview"
                         : " asset is unavailable in the current catalog");
    issue.reasonCode =
        table == cr::CreativeWorldLayoutTable::Opening
            ? "creative_world_layout_opening_asset_missing"
            : "creative_world_layout_object_asset_missing";
    return;
  }
  issue.message =
      std::string(name) + " asset dimensions differ from the current catalog";
  issue.reasonCode =
      table == cr::CreativeWorldLayoutTable::Opening
          ? "creative_world_layout_opening_asset_bounds_stale"
          : "creative_world_layout_object_asset_bounds_stale";
}

std::string diagnosticSubject(const cr::CreativeWorldLayoutReceipt& receipt) {
  if (receipt.failedTable == cr::CreativeWorldLayoutTable::None ||
      receipt.failedIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    return "Layout";
  }
  return std::string(cr::toString(receipt.failedTable)) + " " +
         std::to_string(receipt.failedIndex + 1U);
}

std::string diagnosticMessage(const cr::CreativeWorldLayoutReceipt& receipt) {
  const std::string subject = diagnosticSubject(receipt);
  switch (receipt.status) {
    case cr::CreativeWorldLayoutStatus::InvalidDocument:
      return "Document is not ready for layout generation";
    case cr::CreativeWorldLayoutStatus::InvalidSchema:
      return "Layout header or ownership policy is invalid";
    case cr::CreativeWorldLayoutStatus::Empty:
      return "Add at least one layout symbol";
    case cr::CreativeWorldLayoutStatus::DuplicateStableKey:
      return subject + " duplicates an existing stable key";
    case cr::CreativeWorldLayoutStatus::InvalidSymbol:
      return subject + " has invalid settings or ownership";
    case cr::CreativeWorldLayoutStatus::KernelRejected:
      return subject + " failed geometry validation";
    case cr::CreativeWorldLayoutStatus::CapacityExceeded:
      return "Generated layout exceeds engine capacity";
    case cr::CreativeWorldLayoutStatus::MutationRejected:
      return subject + " could not stage its terrain changes";
    case cr::CreativeWorldLayoutStatus::ObjectRejected:
      return subject + " could not materialize its object recipe";
    case cr::CreativeWorldLayoutStatus::InstallRejected:
      return "Generated layout could not be installed";
    case cr::CreativeWorldLayoutStatus::StalePlan:
      return "Layout source changed during generation";
    case cr::CreativeWorldLayoutStatus::NotRequested:
      return "Layout preflight was not requested";
    case cr::CreativeWorldLayoutStatus::NoChange:
    case cr::CreativeWorldLayoutStatus::Ready:
    case cr::CreativeWorldLayoutStatus::Applied:
      break;
  }
  return subject + " is not ready";
}

}  // namespace

CreativeEditorWorldLayoutDiagnosticReport
buildCreativeEditorWorldLayoutDiagnosticReport(
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeCatalogState* assetCatalog) {
  CreativeEditorWorldLayoutDiagnosticReport report;
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  report.compileReceipt = compiled.receipt;
  report.ready = compiled.receipt.accepted;
  report.hasChanges = compiled.receipt.accepted &&
                      compiled.receipt.status ==
                          cr::CreativeWorldLayoutStatus::Ready;
  if (!report.ready) {
    CreativeEditorWorldLayoutDiagnostic& issue = report.issues[0];
    issue.status = compiled.receipt.status;
    issue.table = compiled.receipt.failedTable;
    issue.index = compiled.receipt.failedIndex;
    issue.message = diagnosticMessage(compiled.receipt);
    issue.reasonCode = compiled.receipt.reasonCode;
    issue.kernelReasonCode = compiled.receipt.kernelReasonCode;
    report.issueCount = 1U;
    return report;
  }
  if (assetCatalog == nullptr) {
    return report;
  }
  for (std::size_t index = 0U;
       index < layout.openings.size() &&
       report.issueCount < report.issues.size();
       ++index) {
    const cr::CreativeWorldLayoutOpening& opening = layout.openings[index];
    appendAssetSourceDiagnostic(
        report, *assetCatalog, cr::CreativeWorldLayoutTable::Opening,
        index, opening.name, opening.insertAssetId,
        opening.hasInsertAssetSourceBounds,
        opening.insertAssetSourceBoundsMeters, opening.includeInsert);
  }
  for (std::size_t index = 0U;
       index < layout.objects.size() &&
       report.issueCount < report.issues.size();
       ++index) {
    const cr::CreativeWorldLayoutObject& object = layout.objects[index];
    appendAssetSourceDiagnostic(
        report, *assetCatalog, cr::CreativeWorldLayoutTable::Object,
        index, object.name, object.assetId, object.hasAssetSourceBounds,
        object.assetSourceBoundsMeters, true);
  }
  return report;
}

const CreativeEditorWorldLayoutDiagnosticReport&
refreshCreativeEditorWorldLayoutDiagnostics(
    CreativeEditorWorldLayoutDiagnosticCache& cache,
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout, std::uint64_t layoutRevision,
    const cr::CreativeCatalogState* assetCatalog) {
  const std::uint64_t terrainRevision = document.terrainField().revision();
  const std::uint64_t materialRevision =
      document.terrainMaterialField().revision();
  const std::uint64_t catalogSignature =
      assetCatalogSignature(assetCatalog);
  if (cache.valid && cache.layoutRevision == layoutRevision &&
      cache.documentId == document.id() &&
      cache.documentRevision == document.revision() &&
      cache.terrainRevision == terrainRevision &&
      cache.materialRevision == materialRevision &&
      cache.assetCatalogSignature == catalogSignature) {
    return cache.report;
  }
  cache.layoutRevision = layoutRevision;
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.terrainRevision = terrainRevision;
  cache.materialRevision = materialRevision;
  cache.assetCatalogSignature = catalogSignature;
  cache.report = buildCreativeEditorWorldLayoutDiagnosticReport(document,
                                                                 layout,
                                                                 assetCatalog);
  cache.valid = true;
  ++cache.buildCount;
  return cache.report;
}


}  // namespace iggy3d_creative_app

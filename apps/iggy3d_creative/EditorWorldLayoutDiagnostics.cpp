#include "EditorWorldLayoutDiagnostics.hpp"

#include "app/iggy3d/creative/input/Catalog.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d_creative_app {
namespace {

namespace cr = iggy3d::creative;

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
      hash ^= static_cast<unsigned char>(byte);
      hash *= 1099511628211ULL;
    }
    hash ^= 0xFFU;
    hash *= 1099511628211ULL;
  }
  return hash;
}

bool catalogContainsAsset(const cr::CreativeCatalogState& catalog,
                          std::string_view assetId) noexcept {
  for (const cr::CreativeCatalogEntry& entry : catalog.entries) {
    if (entry.category == cr::CreativeCatalogEntryCategory::Asset &&
        cr::creativeHotbarAssetId(entry.hotbarEntry) == assetId) {
      return true;
    }
  }
  return false;
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
    if (!opening.includeInsert || !opening.hasInsertAssetSourceBounds ||
        opening.insertAssetId.empty() ||
        catalogContainsAsset(*assetCatalog, opening.insertAssetId)) {
      continue;
    }
    CreativeEditorWorldLayoutDiagnostic& issue =
        report.issues[report.issueCount++];
    issue.severity = CreativeEditorWorldLayoutDiagnosticSeverity::Warning;
    issue.status = cr::CreativeWorldLayoutStatus::Ready;
    issue.table = cr::CreativeWorldLayoutTable::Opening;
    issue.index = index;
    issue.message = opening.name +
                    " asset is unavailable; using a procedural preview";
    issue.reasonCode = "creative_world_layout_opening_asset_missing";
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

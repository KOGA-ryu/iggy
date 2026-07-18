#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "app/iggy3d/creative/world/WorldLayout.hpp"

namespace iggy3d::creative {
struct CreativeCatalogState;
}

namespace iggy3d_creative_app {

inline constexpr std::size_t kCreativeEditorWorldLayoutDiagnosticCapacity = 8U;

enum class CreativeEditorWorldLayoutDiagnosticSeverity : std::uint8_t {
  Info,
  Warning,
  Error,
  Count,
};

enum class CreativeEditorWorldLayoutAssetIssue : std::uint8_t {
  None,
  Missing,
  StaleBounds,
  Count,
};

struct CreativeEditorWorldLayoutDiagnostic {
  CreativeEditorWorldLayoutDiagnosticSeverity severity =
      CreativeEditorWorldLayoutDiagnosticSeverity::Error;
  iggy3d::creative::CreativeWorldLayoutStatus status =
      iggy3d::creative::CreativeWorldLayoutStatus::NotRequested;
  iggy3d::creative::CreativeWorldLayoutTable table =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t index =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string message;
  std::string reasonCode;
  std::string kernelReasonCode;
  CreativeEditorWorldLayoutAssetIssue assetIssue =
      CreativeEditorWorldLayoutAssetIssue::None;
  std::string stableKey;
  std::string assetId;
};

struct CreativeEditorWorldLayoutDiagnosticReport {
  bool ready = false;
  bool hasChanges = false;
  std::array<CreativeEditorWorldLayoutDiagnostic,
             kCreativeEditorWorldLayoutDiagnosticCapacity>
      issues{};
  std::size_t issueCount = 0U;
  iggy3d::creative::CreativeWorldLayoutReceipt compileReceipt;
};

// Transient preflight cache. The compiler can materialize a large exact plan,
// so desktop idle frames reuse the report until either source truth changes.
struct CreativeEditorWorldLayoutDiagnosticCache {
  bool valid = false;
  std::uint64_t layoutRevision = 0U;
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  std::uint64_t terrainRevision = 0U;
  std::uint64_t materialRevision = 0U;
  std::uint64_t assetCatalogSignature = 0U;
  std::uint64_t buildCount = 0U;
  CreativeEditorWorldLayoutDiagnosticReport report;
};

[[nodiscard]] CreativeEditorWorldLayoutDiagnosticReport
buildCreativeEditorWorldLayoutDiagnosticReport(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeWorldLayout& layout,
    const iggy3d::creative::CreativeCatalogState* assetCatalog = nullptr);

[[nodiscard]] const CreativeEditorWorldLayoutDiagnosticReport&
refreshCreativeEditorWorldLayoutDiagnostics(
    CreativeEditorWorldLayoutDiagnosticCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeWorldLayout& layout,
    std::uint64_t layoutRevision,
    const iggy3d::creative::CreativeCatalogState* assetCatalog = nullptr);

}  // namespace iggy3d_creative_app

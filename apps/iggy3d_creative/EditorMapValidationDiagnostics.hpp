#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "app/iggy3d/creative/validation/MapValidation.hpp"
#include "content/assets/StaticMeshAsset.hpp"

namespace iggy3d_creative_app {

enum class CreativeEditorMapValidationCacheStatus : std::uint8_t {
  NotRun,
  Current,
  Stale,
};

enum class CreativeEditorMapDiagnosticArea : std::uint8_t {
  Document,
  Assets,
  PlayerSpawn,
  Runtime,
  Collision,
  Navigation,
  Logic,
  Validation,
  Count,
};

struct CreativeEditorMapDiagnosticDescriptor {
  iggy3d::creative::CreativeMapDiagnosticCode code =
      iggy3d::creative::CreativeMapDiagnosticCode::Unknown;
  CreativeEditorMapDiagnosticArea area =
      CreativeEditorMapDiagnosticArea::Validation;
  std::string_view title;
  std::string_view remediation;
};

// UI-owned, on-demand cache. Whole-map validation includes room baking and
// reachability, so document or catalog changes make the last result stale but
// never trigger hidden frame-time work.
struct CreativeEditorMapValidationCache {
  bool hasResult = false;
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  std::uint64_t assetCatalogSignature = 0U;
  std::uint64_t buildCount = 0U;
  iggy3d::creative::CreativeMapValidationResult result;
};

[[nodiscard]] std::span<const CreativeEditorMapDiagnosticDescriptor>
creativeEditorMapDiagnosticDescriptors() noexcept;
[[nodiscard]] const CreativeEditorMapDiagnosticDescriptor&
creativeEditorMapDiagnosticDescriptor(
    iggy3d::creative::CreativeMapDiagnosticCode code) noexcept;
[[nodiscard]] std::string_view creativeEditorMapDiagnosticAreaLabel(
    CreativeEditorMapDiagnosticArea area) noexcept;

[[nodiscard]] std::uint64_t creativeEditorStaticMeshCatalogSignature(
    const iggy3d::StaticMeshAssetCatalog* catalog) noexcept;
[[nodiscard]] CreativeEditorMapValidationCacheStatus
creativeEditorMapValidationCacheStatus(
    const CreativeEditorMapValidationCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* catalog) noexcept;

// Reuses a current result unless force is true. The UI uses force for an
// explicit re-run; callers that only need current-or-refresh retain idle reuse.
[[nodiscard]] const iggy3d::creative::CreativeMapValidationResult&
refreshCreativeEditorMapValidation(
    CreativeEditorMapValidationCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* catalog,
    bool force = false);

}  // namespace iggy3d_creative_app

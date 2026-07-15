#pragma once

#include "app/iggy3d/creative/adapters/RoomBake.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeMapDiagnosticCapacity = 256U;

enum class CreativeMapValidationStatus : std::uint8_t {
  NotRequested,
  MissingDocument,
  InvalidDocument,
  Validated,
};

enum class CreativeMapDiagnosticSeverity : std::uint8_t {
  Info,
  Warning,
  Error,
};

enum class CreativeMapDiagnosticCode : std::uint8_t {
  Unknown,
  DocumentMissing,
  DocumentInvalid,
  AssetCatalogUnavailable,
  MissingStaticMeshAsset,
  UnsupportedStaticMeshCollision,
  InvalidStaticMeshMetadata,
  NoPlayerSpawn,
  MultiplePlayerSpawns,
  RoomBakeRejected,
  RuntimeObjectSkipped,
  InvalidRuntimeBounds,
  AssetWalkabilitySkipped,
  NoWalkableSurface,
  ReachabilityNotChecked,
  ReachabilityInvalidCellSize,
  ReachabilityGridTooLarge,
  ReachabilityNoUsableSeeds,
  ReachabilityBlockedSeed,
  ReachabilityIslands,
  LogicSourceUnlinked,
  LogicTargetMissing,
  LogicLinkInvalid,
  ConflictingPressurePlates,
  DiagnosticCapacityExceeded,
};

struct CreativeMapDiagnostic {
  CreativeMapDiagnosticSeverity severity =
      CreativeMapDiagnosticSeverity::Info;
  CreativeMapDiagnosticCode code = CreativeMapDiagnosticCode::Unknown;
  CreativeObjectId objectId = kInvalidObjectId;
  std::string subject;
  std::string detail;
  std::uint64_t fact = 0;
};

struct CreativeMapValidationSummary {
  std::uint64_t runtimeObjectCount = 0;
  std::uint64_t playerSpawnCount = 0;
  std::uint64_t referencedStaticMeshAssetCount = 0;
  std::uint32_t infoCount = 0;
  std::uint32_t warningCount = 0;
  std::uint32_t errorCount = 0;
  // Severity counts include diagnostics that did not fit in storage. The
  // truncation fields report that difference explicitly.
  bool diagnosticsTruncated = false;
  std::uint64_t droppedDiagnosticCount = 0;
};

struct CreativeMapValidationRequest {
  const CreativeDocument* document = nullptr;
  const StaticMeshAssetCatalog* staticMeshAssetCatalog = nullptr;
  std::string roomId = "creative_validation";
  float reachabilityCellSizeMeters = 1.0F;
  bool includeHidden = false;
};

struct CreativeMapValidationResult {
  bool requested = false;
  // Accepted means the kernel completed against a valid document. Passed
  // additionally requires that it emitted no error-severity diagnostics.
  bool accepted = false;
  bool passed = false;
  CreativeMapValidationStatus status =
      CreativeMapValidationStatus::NotRequested;
  CreativeDocumentId documentId = kInvalidDocumentId;
  std::uint64_t documentRevision = 0;
  CreativeRoomBakeReceipt roomBake;
  CreativeRoomBakeReachabilityReceipt reachability;
  CreativeMapValidationSummary summary;
  std::vector<CreativeMapDiagnostic> diagnostics;
};

struct CreativeMapEvaluationResult {
  CreativeMapValidationResult validation;
  CreativeRoomBakeResult roomBake;
};

[[nodiscard]] std::string_view toString(
    CreativeMapValidationStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMapDiagnosticSeverity severity) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMapDiagnosticCode code) noexcept;

// This is an on-demand authoring check, not frame state. Diagnostics are
// emitted in document order within fixed validation phases and remain bounded.
// evaluateCreativeMap exposes the exact bake used by validation so activation
// preparation can move it into a payload without baking the document again.
[[nodiscard]] CreativeMapEvaluationResult evaluateCreativeMap(
    const CreativeMapValidationRequest& request);
[[nodiscard]] CreativeMapValidationResult validateCreativeMap(
    const CreativeMapValidationRequest& request);

}  // namespace iggy3d::creative

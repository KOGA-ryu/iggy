#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d::creative {

struct CreativeWorldLayoutBuildingBounds {
  bool valid = false;
  CreativeTerrainCoord2 minimum;
  CreativeTerrainCoord2 maximum;
};

enum class CreativeWorldLayoutBuildingEditStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidOwnership,
  EmptyBuilding,
  NoChange,
  CoordinateOverflow,
  Ready,
};

struct CreativeWorldLayoutBuildingEditResult {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeWorldLayoutBuildingEditStatus status =
      CreativeWorldLayoutBuildingEditStatus::NotRequested;
  std::size_t sourceBuildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t resultBuildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::uint64_t nextStableOrdinal = 1U;
  CreativeWorldLayout edited;
  std::string reasonCode = "creative_world_layout_building_edit_not_requested";
};

struct CreativeWorldLayoutBuildingMoveRequest {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::int64_t deltaXCells = 0;
  std::int64_t deltaZCells = 0;
};

struct CreativeWorldLayoutBuildingDuplicateRequest {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::int64_t deltaXCells = 0;
  std::int64_t deltaZCells = 0;
  std::uint64_t nextStableOrdinal = 1U;
};

struct CreativeWorldLayoutBuildingDeleteRequest {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
};

enum class CreativeWorldLayoutBuildingTemplateStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidOwnership,
  EmptyBuilding,
  InvalidTemplate,
  CoordinateOverflow,
  Ready,
};

struct CreativeWorldLayoutBuildingTemplateFingerprint {
  bool valid = false;
  std::uint64_t value = 0U;

  friend bool operator==(
      const CreativeWorldLayoutBuildingTemplateFingerprint&,
      const CreativeWorldLayoutBuildingTemplateFingerprint&) = default;
};

// The eight rigid transforms of a rectangle. This is persisted as provenance
// so a changed template can be rebuilt at the instance's authored pose.
enum class CreativeWorldLayoutBuildingTemplateOrientation : std::uint8_t {
  Identity,
  RotateRight90,
  Rotate180,
  RotateLeft90,
  MirrorX,
  MirrorZ,
  MirrorDiagonal,
  MirrorAntiDiagonal,
  Count,
};

struct CreativeWorldLayoutBuildingTemplate {
  std::string templateId;
  std::string label;
  CreativeWorldLayout normalizedLayout;
  CreativeWorldLayoutBuildingBounds bounds;
  CreativeWorldLayoutBuildingTemplateFingerprint sourceFingerprint;
  CreativeWorldLayoutBuildingTemplateOrientation orientation =
      CreativeWorldLayoutBuildingTemplateOrientation::Identity;
};

struct CreativeWorldLayoutBuildingTemplateCaptureRequest {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::string templateId;
  std::string label;
};

struct CreativeWorldLayoutBuildingTemplateResult {
  bool requested = false;
  bool accepted = false;
  CreativeWorldLayoutBuildingTemplateStatus status =
      CreativeWorldLayoutBuildingTemplateStatus::NotRequested;
  CreativeWorldLayoutBuildingTemplate value;
  std::string reasonCode =
      "creative_world_layout_building_template_not_requested";
};

struct CreativeWorldLayoutBuildingTemplateStampRequest {
  CreativeTerrainCoord2 anchor;
  std::uint64_t nextStableOrdinal = 1U;
  bool appendCopySuffix = false;
  bool linkTemplateInstance = true;
};

struct CreativeWorldLayoutBuildingTemplateInstanceProvenance {
  bool present = false;
  bool valid = false;
  std::string templateId;
  std::uint64_t sourceFingerprint = 0U;
  std::uint64_t instanceBaselineFingerprint = 0U;
  CreativeWorldLayoutBuildingTemplateOrientation orientation =
      CreativeWorldLayoutBuildingTemplateOrientation::Identity;
  CreativeTerrainCoord2 anchor;
};

enum class CreativeWorldLayoutBuildingTemplateSyncState : std::uint8_t {
  Unlinked,
  Current,
  SourceChanged,
  LocallyModified,
  Conflict,
  SourceMissing,
};

struct CreativeWorldLayoutBuildingTemplateSyncReceipt {
  bool requested = false;
  bool accepted = false;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutBuildingTemplateSyncState state =
      CreativeWorldLayoutBuildingTemplateSyncState::Unlinked;
  CreativeWorldLayoutBuildingTemplateInstanceProvenance provenance;
  CreativeWorldLayoutBuildingTemplateFingerprint sourceFingerprint;
  CreativeWorldLayoutBuildingTemplateFingerprint instanceFingerprint;
  std::string reasonCode =
      "creative_world_layout_building_template_sync_not_requested";
};

enum class CreativeWorldLayoutBuildingTemplateRefreshMode : std::uint8_t {
  SelectedInstance,
  SafeInstances,
  ForceAll,
  Count,
};

enum class CreativeWorldLayoutBuildingTemplateRefreshStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidOwnership,
  NoMatchingInstances,
  NoEligibleInstances,
  CoordinateOverflow,
  Ready,
};

struct CreativeWorldLayoutBuildingTemplateRefreshRequest {
  const CreativeWorldLayoutBuildingTemplate* sourceTemplate = nullptr;
  CreativeWorldLayoutBuildingTemplateRefreshMode mode =
      CreativeWorldLayoutBuildingTemplateRefreshMode::SafeInstances;
  std::size_t selectedBuildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::uint64_t nextStableOrdinal = 1U;
};

struct CreativeWorldLayoutBuildingTemplateRefreshResult {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeWorldLayoutBuildingTemplateRefreshStatus status =
      CreativeWorldLayoutBuildingTemplateRefreshStatus::NotRequested;
  CreativeWorldLayoutBuildingTemplateRefreshMode mode =
      CreativeWorldLayoutBuildingTemplateRefreshMode::SafeInstances;
  std::size_t matchedInstanceCount = 0U;
  std::size_t currentInstanceCount = 0U;
  std::size_t sourceChangedInstanceCount = 0U;
  std::size_t locallyModifiedInstanceCount = 0U;
  std::size_t conflictInstanceCount = 0U;
  std::size_t refreshedInstanceCount = 0U;
  std::uint64_t nextStableOrdinal = 1U;
  CreativeWorldLayout edited;
  std::string reasonCode =
      "creative_world_layout_building_template_refresh_not_requested";
};

enum class CreativeWorldLayoutBuildingTransformOperation : std::uint8_t {
  RotateLeft90,
  RotateRight90,
  MirrorX,
  MirrorZ,
  Count,
};

enum class CreativeWorldLayoutBuildingTransformStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidOwnership,
  EmptyBuilding,
  InvalidGeometry,
  CoordinateOverflow,
  Ready,
};

struct CreativeWorldLayoutBuildingTransformRequest {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutBuildingTransformOperation operation =
      CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
};

struct CreativeWorldLayoutBuildingTransformResult {
  bool requested = false;
  bool accepted = false;
  CreativeWorldLayoutBuildingTransformStatus status =
      CreativeWorldLayoutBuildingTransformStatus::NotRequested;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutBuildingTransformOperation operation =
      CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
  CreativeWorldLayoutBuildingBounds sourceBounds;
  CreativeWorldLayoutBuildingBounds transformedBounds;
  CreativeWorldLayout transformed;
  std::string reasonCode =
      "creative_world_layout_building_transform_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingEditStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingTemplateStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingTemplateOrientation orientation) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingTemplateSyncState state) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingTemplateRefreshMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingTemplateRefreshStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingTransformStatus status) noexcept;

// Validates every flat-table owner and opening host before an operation that
// must remap indices. Terrain is intentionally outside building ownership.
[[nodiscard]] bool validCreativeWorldLayoutBuildingOwnership(
    const CreativeWorldLayout& layout) noexcept;

[[nodiscard]] bool measureCreativeWorldLayoutBuildingBounds(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    CreativeWorldLayoutBuildingBounds& output) noexcept;

// This preview check is allocation-free. Accepted edit operations below copy
// the layout once and publish no partial candidate on failure.
[[nodiscard]] bool canMoveCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutBuildingMoveRequest& request) noexcept;
[[nodiscard]] bool defaultCreativeWorldLayoutBuildingDuplicateOffset(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    std::int64_t& deltaXCells,
    std::int64_t& deltaZCells) noexcept;

[[nodiscard]] CreativeWorldLayoutBuildingEditResult
moveCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingMoveRequest& request);
[[nodiscard]] CreativeWorldLayoutBuildingEditResult
duplicateCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingDuplicateRequest& request);
[[nodiscard]] CreativeWorldLayoutBuildingEditResult
deleteCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingDeleteRequest& request);

[[nodiscard]] bool validCreativeWorldLayoutBuildingTemplate(
    const CreativeWorldLayoutBuildingTemplate& value) noexcept;
[[nodiscard]] bool isCreativeWorldLayoutBuildingTemplateProvenanceTag(
    std::string_view tag) noexcept;
[[nodiscard]] CreativeWorldLayoutBuildingTemplateFingerprint
fingerprintCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex) noexcept;
[[nodiscard]] CreativeWorldLayoutBuildingTemplateInstanceProvenance
creativeWorldLayoutBuildingTemplateInstanceProvenance(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex);
[[nodiscard]] bool setCreativeWorldLayoutBuildingTemplateInstanceProvenance(
    CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    const CreativeWorldLayoutBuildingTemplateInstanceProvenance& provenance);
[[nodiscard]] CreativeWorldLayoutBuildingTemplateOrientation
composeCreativeWorldLayoutBuildingTemplateOrientation(
    CreativeWorldLayoutBuildingTemplateOrientation current,
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept;
[[nodiscard]] CreativeWorldLayoutBuildingTemplateOrientation
inverseCreativeWorldLayoutBuildingTemplateOrientation(
    CreativeWorldLayoutBuildingTemplateOrientation orientation) noexcept;
[[nodiscard]] CreativeWorldLayoutBuildingTemplateResult
captureCreativeWorldLayoutBuildingTemplate(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingTemplateCaptureRequest& request);
[[nodiscard]] CreativeWorldLayoutBuildingTemplateResult
loadCreativeWorldLayoutBuildingTemplate(CreativeWorldLayout normalizedLayout);
[[nodiscard]] CreativeWorldLayoutBuildingTemplateResult
transformCreativeWorldLayoutBuildingTemplate(
    const CreativeWorldLayoutBuildingTemplate& source,
    CreativeWorldLayoutBuildingTransformOperation operation);
[[nodiscard]] CreativeWorldLayoutBuildingTemplateResult
orientCreativeWorldLayoutBuildingTemplate(
    const CreativeWorldLayoutBuildingTemplate& source,
    CreativeWorldLayoutBuildingTemplateOrientation orientation);
[[nodiscard]] CreativeWorldLayoutBuildingEditResult
stampCreativeWorldLayoutBuildingTemplate(
    const CreativeWorldLayout& destination,
    const CreativeWorldLayoutBuildingTemplate& source,
    const CreativeWorldLayoutBuildingTemplateStampRequest& request);
[[nodiscard]] CreativeWorldLayoutBuildingTemplateSyncReceipt
inspectCreativeWorldLayoutBuildingTemplateSync(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    const CreativeWorldLayoutBuildingTemplate* sourceTemplate);
[[nodiscard]] CreativeWorldLayoutBuildingTemplateRefreshResult
refreshCreativeWorldLayoutBuildingTemplateInstances(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingTemplateRefreshRequest& request);

// Replaces every flat-table row owned by one building with the rows from a
// one-building replacement layout. Existing stable keys are reused by
// deterministic owner order; newly introduced rows mint keys from the supplied
// ordinal. The caller must pass a scratch candidate and discard it on false.
[[nodiscard]] bool replaceCreativeWorldLayoutBuildingInCandidate(
    CreativeWorldLayout& candidate, std::size_t buildingIndex,
    const CreativeWorldLayout& replacement,
    std::uint64_t& nextStableOrdinal, bool preserveExistingNames);

// O(buildings + rooms + boxes + walls + openings), with one full layout copy.
// Stable keys, source order, heights, and terrain remain unchanged. Quarter
// turns keep the aggregate minimum grid line fixed, avoiding half-cell pivots;
// axis spans swap. Hosted openings and open-door poses are remapped so the
// generated 3D geometry follows the same exact transform. Rejection leaves the
// source untouched and returns no partial candidate.
[[nodiscard]] CreativeWorldLayoutBuildingTransformResult
transformCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingTransformRequest& request);

}  // namespace iggy3d::creative

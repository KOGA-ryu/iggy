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

struct CreativeWorldLayoutBuildingTemplate {
  std::string templateId;
  std::string label;
  CreativeWorldLayout normalizedLayout;
  CreativeWorldLayoutBuildingBounds bounds;
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
[[nodiscard]] CreativeWorldLayoutBuildingEditResult
stampCreativeWorldLayoutBuildingTemplate(
    const CreativeWorldLayout& destination,
    const CreativeWorldLayoutBuildingTemplate& source,
    const CreativeWorldLayoutBuildingTemplateStampRequest& request);

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

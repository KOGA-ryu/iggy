#pragma once

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeDocumentWireframeStatus : std::uint8_t {
  Unknown,
  MissingSource,
  EmptySource,
  InvalidProjection,
  NoVisibleItems,
  NoRenderableItems,
  Built,
};

enum class CreativeDocumentWireframeItemKind : std::uint8_t {
  Unknown,
  Box,
  Point,
  Line,
};

enum class CreativeDocumentWireframeStyle : std::uint8_t {
  Unknown,
  Structural,
  Collision,
  Navigation,
  Trigger,
  Gameplay,
  Light,
  Audio,
  Camera,
  Testing,
  Authoring,
};

enum class CreativeDocumentWireframeSegmentStatus : std::uint8_t {
  Unknown,
  MissingSource,
  EmptySource,
  Built,
  NoSegments,
};

enum class CreativeDocumentWireframeSegmentKind : std::uint8_t {
  Unknown,
  BoxEdge,
  Line,
};

struct CreativeDocumentWireframeItem {
  CreativeDocumentWireframeItemKind itemKind =
      CreativeDocumentWireframeItemKind::Unknown;
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  bool visible = false;
  CreativeSpatialProjectionProfile projectionProfile =
      CreativeSpatialProjectionProfile::Unknown;
  CreativeSpatialOccupancyKind occupancyKind =
      CreativeSpatialOccupancyKind::Unknown;
  CreativeDocumentWireframeStyle style =
      CreativeDocumentWireframeStyle::Unknown;
  CreativeBounds bounds;
  CreativeVec3 start;
  CreativeVec3 end;
  CreativeGridBounds3 projectedBounds;
  std::uint64_t projectedCellCount = 0;
};

struct CreativeDocumentWireframeDrawList {
  std::vector<CreativeDocumentWireframeItem> items;
};

struct CreativeDocumentWireframeSegment {
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  CreativeDocumentWireframeStyle style =
      CreativeDocumentWireframeStyle::Unknown;
  CreativeDocumentWireframeSegmentKind segmentKind =
      CreativeDocumentWireframeSegmentKind::Unknown;
  CreativeVec3 start;
  CreativeVec3 end;
};

struct CreativeDocumentWireframeSegmentList {
  std::vector<CreativeDocumentWireframeSegment> segments;
};

struct CreativeDocumentWireframeBuildRequest {
  const CreativeObject* objects = nullptr;
  std::size_t objectCount = 0;
  CreativeSpatialProjectionRequest projectionRequest;
  bool documentAvailable = false;
};

struct CreativeDocumentWireframeReceipt {
  bool requested = false;
  bool documentAvailable = false;
  bool sourceAvailable = false;
  bool projectionValid = false;
  std::uint64_t objectCount = 0;
  std::uint64_t visibleObjectCount = 0;
  std::uint64_t hiddenObjectCount = 0;
  std::uint64_t projectableObjectCount = 0;
  std::uint64_t projectedObjectCount = 0;
  std::uint64_t nonProjectableObjectCount = 0;
  std::uint64_t projectionCellCount = 0;
  std::uint64_t itemCount = 0;
  CreativeDocumentWireframeStatus status =
      CreativeDocumentWireframeStatus::Unknown;
  std::string_view message = "creative_document_wireframe_not_requested";
  std::string_view reasonCode = "creative_document_wireframe_not_requested";
};

struct CreativeDocumentWireframeBuildResult {
  CreativeDocumentWireframeDrawList drawList;
  CreativeDocumentWireframeReceipt receipt;
};

struct CreativeDocumentWireframeSegmentBuildRequest {
  const CreativeDocumentWireframeItem* items = nullptr;
  std::size_t itemCount = 0;
  bool sourceAvailable = false;
};

struct CreativeDocumentWireframeSegmentReceipt {
  bool requested = false;
  bool sourceAvailable = false;
  std::uint64_t itemCount = 0;
  std::uint64_t boxItemCount = 0;
  std::uint64_t lineItemCount = 0;
  std::uint64_t pointItemCount = 0;
  std::uint64_t segmentCount = 0;
  std::uint64_t skippedDegenerateCount = 0;
  CreativeDocumentWireframeSegmentStatus status =
      CreativeDocumentWireframeSegmentStatus::Unknown;
  std::string_view message =
      "creative_document_wireframe_segments_not_requested";
  std::string_view reasonCode =
      "creative_document_wireframe_segments_not_requested";
};

struct CreativeDocumentWireframeSegmentBuildResult {
  CreativeDocumentWireframeSegmentList segmentList;
  CreativeDocumentWireframeSegmentReceipt receipt;
};

[[nodiscard]] std::string_view toString(
    CreativeDocumentWireframeStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeDocumentWireframeItemKind itemKind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeDocumentWireframeStyle style) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeDocumentWireframeSegmentStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeDocumentWireframeSegmentKind segmentKind) noexcept;

[[nodiscard]] CreativeDocumentWireframeStyle wireframeStyleForOccupancy(
    CreativeSpatialOccupancyKind occupancyKind) noexcept;
[[nodiscard]] CreativeDocumentWireframeItemKind wireframeItemKindForProjection(
    CreativeSpatialProjectionProfile profile) noexcept;

[[nodiscard]] CreativeDocumentWireframeBuildResult
buildCreativeDocumentWireframeList(
    const CreativeDocumentWireframeBuildRequest& request);
[[nodiscard]] CreativeDocumentWireframeBuildResult
buildCreativeObjectWireframeList(
    std::span<const CreativeObject> objects,
    const CreativeSpatialProjectionRequest& projectionRequest);
[[nodiscard]] CreativeDocumentWireframeBuildResult
buildCreativeDocumentWireframeList(
    const CreativeDocument& document,
    const CreativeSpatialProjectionRequest& projectionRequest);

[[nodiscard]] CreativeDocumentWireframeSegmentBuildResult
buildCreativeDocumentWireframeSegments(
    const CreativeDocumentWireframeSegmentBuildRequest& request);
[[nodiscard]] CreativeDocumentWireframeSegmentBuildResult
buildCreativeDocumentWireframeSegments(
    const CreativeDocumentWireframeDrawList& drawList);
[[nodiscard]] CreativeDocumentWireframeSegmentBuildResult
buildCreativeDocumentWireframeSegments(
    const CreativeDocument& document,
    const CreativeSpatialProjectionRequest& projectionRequest);

}  // namespace iggy3d::creative

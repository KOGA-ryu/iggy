#pragma once

#include "app/iggy3d/creative/DocumentWireframe.hpp"
#include "core/math/Vec3.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d {

enum class ProductCreativeWireframeDebugLineStatus : std::uint8_t {
  Unknown,
  MissingSource,
  NoLines,
  Built,
};

struct ProductCreativeWireframeDebugLineColor {
  float r = 1.0F;
  float g = 1.0F;
  float b = 1.0F;
  float a = 1.0F;
};

struct ProductCreativeWireframeDebugLine {
  Vec3 start;
  Vec3 end;
  ProductCreativeWireframeDebugLineColor color;
  creative::CreativeObjectId objectId = creative::kInvalidObjectId;
  creative::CreativeObjectKind objectKind =
      creative::CreativeObjectKind::Unknown;
  creative::CreativeDocumentWireframeStyle style =
      creative::CreativeDocumentWireframeStyle::Unknown;
  creative::CreativeDocumentWireframeSegmentKind segmentKind =
      creative::CreativeDocumentWireframeSegmentKind::Unknown;
  float thickness = 1.0F;
};

struct ProductCreativeWireframeDebugLineList {
  std::vector<ProductCreativeWireframeDebugLine> lines;
};

struct ProductCreativeWireframeDebugLineBuildRequest {
  const creative::CreativeDocumentWireframeSegment* segments = nullptr;
  std::size_t segmentCount = 0;
  bool sourceAvailable = false;
  float thickness = 1.0F;
};

struct ProductCreativeWireframeDebugLineReceipt {
  bool requested = false;
  bool sourceAvailable = false;
  std::uint64_t segmentCount = 0;
  std::uint64_t lineCount = 0;
  std::uint64_t skippedDegenerateCount = 0;
  ProductCreativeWireframeDebugLineStatus status =
      ProductCreativeWireframeDebugLineStatus::Unknown;
  std::string_view message =
      "product_creative_wireframe_debug_lines_not_requested";
  std::string_view reasonCode =
      "product_creative_wireframe_debug_lines_not_requested";
};

struct ProductCreativeWireframeDebugLineBuildResult {
  ProductCreativeWireframeDebugLineList lineList;
  ProductCreativeWireframeDebugLineReceipt receipt;
};

[[nodiscard]] std::string_view toString(
    ProductCreativeWireframeDebugLineStatus status) noexcept;
[[nodiscard]] ProductCreativeWireframeDebugLineColor
productCreativeWireframeDebugLineColorForStyle(
    creative::CreativeDocumentWireframeStyle style) noexcept;

[[nodiscard]] ProductCreativeWireframeDebugLineBuildResult
buildProductCreativeWireframeDebugLines(
    const ProductCreativeWireframeDebugLineBuildRequest& request);
[[nodiscard]] ProductCreativeWireframeDebugLineBuildResult
buildProductCreativeWireframeDebugLines(
    std::span<const creative::CreativeDocumentWireframeSegment> segments);
[[nodiscard]] ProductCreativeWireframeDebugLineBuildResult
buildProductCreativeWireframeDebugLines(
    const creative::CreativeDocumentWireframeSegmentList& segmentList);

}  // namespace iggy3d

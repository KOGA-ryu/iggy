#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/SpatialProjection.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeViewportPickStatus : std::uint8_t {
  Unknown,
  InvalidGrid,
  InvalidViewport,
  NoCells,
  OutOfViewport,
  OutOfGrid,
  Miss,
  Hit,
};

enum class CreativeViewportPickDepthMode : std::uint8_t {
  FixedZ,
  HighestZFirst,
  LowestZFirst,
};

struct CreativeViewportPickViewport {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  float height = 0.0F;
};

struct CreativeViewportPickRequest {
  CreativeViewportPickViewport viewport;
  CreativeGridSize3 gridSize;
  const CreativeSpatialCell* cells = nullptr;
  std::size_t cellCount = 0;
  float pointerX = 0.0F;
  float pointerY = 0.0F;
  std::int32_t z = 0;
  CreativeViewportPickDepthMode depthMode =
      CreativeViewportPickDepthMode::FixedZ;
};

struct CreativeViewportPickReceipt {
  bool requested = false;
  bool hit = false;
  CreativeViewportPickStatus status = CreativeViewportPickStatus::Unknown;
  CreativeGridCoord3 coord;
  CreativeGridIndex index = 0;
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  CreativeSpatialOccupancyKind occupancyKind =
      CreativeSpatialOccupancyKind::Unknown;
  TargetRef target;
  std::size_t cellIndex = 0;
  std::string message;
};

[[nodiscard]] std::string_view toString(
    CreativeViewportPickStatus status) noexcept;
[[nodiscard]] bool isValidCreativeViewportPickViewport(
    CreativeViewportPickViewport viewport) noexcept;
[[nodiscard]] CreativeGridCoord3 pointerToCreativeGridCoord(
    CreativeViewportPickViewport viewport,
    CreativeGridSize3 gridSize,
    float pointerX,
    float pointerY,
    std::int32_t z) noexcept;
[[nodiscard]] CreativeViewportPickReceipt pickCreativeViewportCell(
    const CreativeViewportPickRequest& request);

}  // namespace iggy3d::creative

#pragma once

#include <cstdint>
#include <type_traits>

#include "app/iggy3d/creative/document/TerrainField.hpp"

namespace iggy3d::creative {

enum class CreativeTerrainSurfacePoseStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  InvalidRequest,
  MissingSurface,
  Ready,
};

struct CreativeTerrainSurfacePoseRequest {
  const CreativeTerrainField* field = nullptr;
  CreativeVec3 worldPoint{};
  CreativeVec3 gridOrigin{};
  double cellSizeMeters = 1.0;
};

struct CreativeTerrainSurfacePose {
  CreativeTerrainSurfacePoseStatus status =
      CreativeTerrainSurfacePoseStatus::NotRequested;
  CreativeTerrainCoord2 cell{};
  CreativeVec3 position{};
  CreativeVec3 normal{};
  double slopeRadians = 0.0;
  bool requested = false;
  bool accepted = false;
  bool present = false;
};

static_assert(std::is_trivially_copyable_v<CreativeTerrainSurfacePoseRequest>);
static_assert(std::is_standard_layout_v<CreativeTerrainSurfacePoseRequest>);
static_assert(std::is_trivially_copyable_v<CreativeTerrainSurfacePose>);
static_assert(std::is_standard_layout_v<CreativeTerrainSurfacePose>);

// Samples the same four-corner bilinear surface emitted by TerrainRender.
[[nodiscard]] CreativeTerrainSurfacePose sampleCreativeTerrainSurfacePose(
    const CreativeTerrainSurfacePoseRequest& request) noexcept;

}  // namespace iggy3d::creative

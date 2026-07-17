#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/TerrainField.hpp"

namespace iggy3d::creative {

enum class CreativePlacementClearanceStatus : std::uint8_t {
  NotEvaluated,
  InvalidRequest,
  TraversalLimitExceeded,
  OutsideWorldBounds,
  AuthoredObjectBlocked,
  VoxelBlocked,
  TerrainBlocked,
  Ready,
};

struct CreativePlacementClearanceResult {
  CreativePlacementClearanceStatus status =
      CreativePlacementClearanceStatus::NotEvaluated;
  CreativeObjectId blockingObjectId = kInvalidObjectId;
  CreativeGridCoord3 blockingVoxelCell{};
  CreativeTerrainCoord2 blockingTerrainCell{};
  std::uint64_t testedAuthoredObjectCount = 0;
  std::uint64_t testedVoxelCellCount = 0;
  std::uint64_t testedTerrainSampleCount = 0;
  bool evaluated = false;
  bool allowed = false;
};

static_assert(std::is_trivially_copyable_v<CreativePlacementClearanceResult>);
static_assert(std::is_standard_layout_v<CreativePlacementClearanceResult>);

[[nodiscard]] std::string_view toString(
    CreativePlacementClearanceStatus status) noexcept;

}  // namespace iggy3d::creative

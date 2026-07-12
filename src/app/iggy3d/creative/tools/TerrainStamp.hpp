#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeTerrainStampEditCapacity =
    kCreativeTerrainControlCapacity * 2U;

enum class CreativeTerrainStampMode : std::uint8_t {
  Merge,
  Replace,
  Count,
};

enum class CreativeTerrainStampCopyStatus : std::uint8_t {
  NotRequested,
  InvalidSource,
  InvalidBounds,
  EmptyRegion,
  Copied,
};

struct CreativeTerrainStamp {
  std::uint64_t sourceDocumentId = 0U;
  std::uint64_t sourceRevision = 0U;
  std::uint64_t contentSignature = 0U;
  CreativeTerrainCoord2 sourceMinimum{};
  std::uint32_t widthCells = 0U;
  std::uint32_t depthCells = 0U;
  std::array<CreativeTerrainControlPoint, kCreativeTerrainControlCapacity>
      controls{};
  std::uint16_t controlCount = 0U;

  [[nodiscard]] std::span<const CreativeTerrainControlPoint> items()
      const noexcept {
    return {controls.data(), controlCount};
  }
};

struct CreativeTerrainStampCopyReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainStampCopyStatus status =
      CreativeTerrainStampCopyStatus::NotRequested;
  CreativeTerrainCoord2 minimumCoord{};
  CreativeTerrainCoord2 maximumCoord{};
  std::uint16_t copiedControlCount = 0U;
  std::string_view reasonCode = "creative_terrain_stamp_copy_not_requested";
};

enum class CreativeTerrainStampPlanStatus : std::uint8_t {
  NotRequested,
  InvalidStamp,
  InvalidDestination,
  InvalidRequest,
  CoordinateOverflow,
  CapacityExceeded,
  NoChange,
  Ready,
};

struct CreativeTerrainStampRequest {
  const CreativeTerrainStamp* stamp = nullptr;
  std::span<const CreativeTerrainControlPoint> destinationControls{};
  CreativeTerrainCoord2 targetMinimum{};
  std::uint8_t quarterTurns = 0U;
  bool mirrorX = false;
  bool mirrorZ = false;
  CreativeTerrainStampMode mode = CreativeTerrainStampMode::Merge;
};

struct CreativeTerrainStampPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainStampPlanStatus status =
      CreativeTerrainStampPlanStatus::NotRequested;
  CreativeTerrainStampMode mode = CreativeTerrainStampMode::Merge;
  CreativeTerrainCoord2 targetMinimum{};
  CreativeTerrainCoord2 targetMaximum{};
  std::uint32_t transformedWidthCells = 0U;
  std::uint32_t transformedDepthCells = 0U;
  std::array<CreativeTerrainControlPoint, kCreativeTerrainControlCapacity>
      finalControls{};
  std::uint16_t finalControlCount = 0U;
  std::array<CreativeTerrainControlEdit, kCreativeTerrainStampEditCapacity>
      edits{};
  std::uint16_t editCount = 0U;
  std::uint16_t insertedControlCount = 0U;
  std::uint16_t updatedControlCount = 0U;
  std::uint16_t removedControlCount = 0U;
  std::string_view reasonCode = "creative_terrain_stamp_not_requested";

  [[nodiscard]] std::span<const CreativeTerrainControlPoint> controls()
      const noexcept {
    return {finalControls.data(), finalControlCount};
  }

  [[nodiscard]] std::span<const CreativeTerrainControlEdit> items()
      const noexcept {
    return {edits.data(), editCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeTerrainStamp>);
static_assert(std::is_standard_layout_v<CreativeTerrainStamp>);
static_assert(std::is_trivially_copyable_v<CreativeTerrainStampPlan>);
static_assert(std::is_standard_layout_v<CreativeTerrainStampPlan>);

[[nodiscard]] std::string_view toString(
    CreativeTerrainStampMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainStampCopyStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainStampPlanStatus status) noexcept;

[[nodiscard]] bool isValidCreativeTerrainStamp(
    const CreativeTerrainStamp& stamp) noexcept;
[[nodiscard]] bool creativeTerrainStampEmpty(
    const CreativeTerrainStamp& stamp) noexcept;
void clearCreativeTerrainStamp(CreativeTerrainStamp& stamp) noexcept;

[[nodiscard]] CreativeTerrainStampCopyReceipt copyCreativeTerrainRegionToStamp(
    std::uint64_t sourceDocumentId,
    std::uint64_t sourceRevision,
    std::span<const CreativeTerrainControlPoint> controls,
    CreativeTerrainCoord2 minimumCoord,
    CreativeTerrainCoord2 maximumCoord,
    CreativeTerrainStamp& outStamp) noexcept;

// O(n log n), n <= 256 source and destination controls. Rotation and mirrors
// operate in source-local integer coordinates, then normalize the transformed
// footprint so targetMinimum remains the lower X/Z corner. Replace removes
// destination-only rods inside that footprint; Merge preserves them.
[[nodiscard]] CreativeTerrainStampPlan buildCreativeTerrainStampPlan(
    const CreativeTerrainStampRequest& request) noexcept;

}  // namespace iggy3d::creative

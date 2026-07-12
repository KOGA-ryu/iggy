#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "app/iggy3d/creative/tools/ConnectedFill.hpp"

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeSurfaceExtrudeCapacity =
    kCreativeConnectedFillCapacity;

enum class CreativeSurfaceExtrudeDepth : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  Count,
};

enum class CreativeSurfaceExtrudeKind : std::uint8_t {
  Extrude,
  Inset,
  Count,
};

enum class CreativeSurfaceExtrudeStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  InvalidFace,
  InvalidKind,
  InvalidDepth,
  InvalidLimit,
  EmptySeed,
  FaceOccluded,
  CapacityExceeded,
  CoordinateOverflow,
  DestinationOccupied,
  SourceMissing,
  Planned,
};

struct CreativeSurfaceExtrudeRequest {
  const CreativeVoxelField* field = nullptr;
  CreativeGridCoord3 seedCell{};
  CreativeGridCoord3 outward{};
  CreativeSurfaceExtrudeKind kind = CreativeSurfaceExtrudeKind::Extrude;
  CreativeSurfaceExtrudeDepth depth =
      CreativeSurfaceExtrudeDepth::OneCell;
  CreativeConnectedFillLimit affectedCellLimit =
      CreativeConnectedFillLimit::Cells256;
};

struct CreativeSurfaceExtrudePlan {
  bool requested = false;
  bool accepted = false;
  CreativeSurfaceExtrudeKind kind = CreativeSurfaceExtrudeKind::Extrude;
  CreativeSurfaceExtrudeDepth depth =
      CreativeSurfaceExtrudeDepth::OneCell;
  CreativeConnectedFillLimit affectedCellLimit =
      CreativeConnectedFillLimit::Cells256;
  CreativeSurfaceExtrudeStatus status =
      CreativeSurfaceExtrudeStatus::NotRequested;
  CreativeObjectKind sourceMaterial = CreativeObjectKind::Unknown;
  CreativeGridCoord3 seedCell{};
  CreativeGridCoord3 outward{};
  CreativeGridCoord3 minMutationCell{};
  CreativeGridCoord3 maxMutationCell{};
  std::array<CreativeGridCoord3, kCreativeSurfaceExtrudeCapacity>
      surfaceCells{};
  std::array<CreativeGridCoord3, kCreativeSurfaceExtrudeCapacity>
      mutationCells{};
  std::uint16_t surfaceCellCount = 0U;
  std::uint16_t mutationCellCount = 0U;
  std::string_view reasonCode = "creative_surface_extrude_not_requested";

  [[nodiscard]] std::span<const CreativeGridCoord3> surfacePatch()
      const noexcept {
    return {surfaceCells.data(), surfaceCellCount};
  }

  [[nodiscard]] std::span<const CreativeGridCoord3> generatedCells()
      const noexcept {
    return {mutationCells.data(), mutationCellCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeSurfaceExtrudePlan>);
static_assert(std::is_standard_layout_v<CreativeSurfaceExtrudePlan>);

[[nodiscard]] std::uint8_t creativeSurfaceExtrudeDepthCells(
    CreativeSurfaceExtrudeDepth depth) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeSurfaceExtrudeDepth depth) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeSurfaceExtrudeKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeSurfaceExtrudeStatus status) noexcept;

// Finds the connected, coplanar, exposed patch containing seedCell, then plans
// every outward destination or inward removal cell. Total affected cells are
// bounded by affectedCellLimit; any invalid member rejects the whole plan.
[[nodiscard]] CreativeSurfaceExtrudePlan planCreativeSurfaceExtrude(
    const CreativeSurfaceExtrudeRequest& request) noexcept;

}  // namespace iggy3d::creative

#include "app/iggy3d/creative/adapters/RoomBakeReachability.hpp"

#include "core/grid/GridFootprint.hpp"
#include "core/grid/Reachability.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d::creative {

namespace {

struct ReachabilityProjection {
  ReachabilityGrid grid;
  std::int32_t originCellX = 0;
  std::int32_t originCellZ = 0;
};

enum class ReachabilityProjectionStatus {
  Built,
  NoWalkableCells,
  GridTooLarge,
};

void setReachabilityStatus(CreativeRoomBakeReachabilityReceipt& receipt,
                           CreativeRoomBakeReachabilityStatus status,
                           std::string reasonCode,
                           bool checked = false) {
  receipt.status = status;
  receipt.reasonCode = std::move(reasonCode);
  receipt.message = receipt.reasonCode;
  receipt.checked = checked;
}

[[nodiscard]] bool finiteVec3(Vec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

[[nodiscard]] bool walkableFootprintForSurface(
    const RoomSpatialSurface& surface,
    float cellSizeMeters,
    GridFootprint& out) {
  if (surface.role != RoomSpatialSurfaceRole::Walkable ||
      surface.pointsMeters.empty()) {
    return false;
  }

  Vec3 min = surface.pointsMeters.front();
  Vec3 max = min;
  for (const Vec3 point : surface.pointsMeters) {
    if (!finiteVec3(point)) {
      return false;
    }
    min.x = std::min(min.x, point.x);
    min.z = std::min(min.z, point.z);
    max.x = std::max(max.x, point.x);
    max.z = std::max(max.z, point.z);
  }

  const GridFootprintResult result =
      gridFootprintForContainingBounds(min.x,
                                       min.z,
                                       max.x,
                                       max.z,
                                       cellSizeMeters);
  if (!result.ok) {
    return false;
  }
  out = result.footprint;
  return true;
}

[[nodiscard]] ReachabilityProjectionStatus buildReachabilityProjection(
    const RoomAsset& room,
    float cellSizeMeters,
    ReachabilityProjection& out) {
  std::vector<GridFootprint> footprints;
  footprints.reserve(room.spatialSurfaces.size());
  for (const RoomSpatialSurface& surface : room.spatialSurfaces) {
    GridFootprint footprint;
    if (walkableFootprintForSurface(surface, cellSizeMeters, footprint)) {
      footprints.push_back(footprint);
    }
  }

  if (footprints.empty()) {
    return ReachabilityProjectionStatus::NoWalkableCells;
  }

  std::int32_t minX = footprints.front().minCellX;
  std::int32_t minZ = footprints.front().minCellZ;
  std::int32_t maxX = footprints.front().maxCellXExclusive;
  std::int32_t maxZ = footprints.front().maxCellZExclusive;
  for (const GridFootprint& footprint : footprints) {
    minX = std::min(minX, footprint.minCellX);
    minZ = std::min(minZ, footprint.minCellZ);
    maxX = std::max(maxX, footprint.maxCellXExclusive);
    maxZ = std::max(maxZ, footprint.maxCellZExclusive);
  }

  const std::int64_t width64 =
      static_cast<std::int64_t>(maxX) - static_cast<std::int64_t>(minX);
  const std::int64_t depth64 =
      static_cast<std::int64_t>(maxZ) - static_cast<std::int64_t>(minZ);
  constexpr std::int64_t kMaxReachabilityCells = 1'000'000;
  if (width64 <= 0 || depth64 <= 0 ||
      width64 >
          static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) ||
      depth64 >
          static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) ||
      width64 * depth64 > kMaxReachabilityCells) {
    return ReachabilityProjectionStatus::GridTooLarge;
  }

  out.originCellX = minX;
  out.originCellZ = minZ;
  out.grid.width = static_cast<std::int32_t>(width64);
  out.grid.depth = static_cast<std::int32_t>(depth64);
  out.grid.walkable.assign(
      static_cast<std::size_t>(out.grid.width) *
          static_cast<std::size_t>(out.grid.depth),
      0);

  for (const GridFootprint& footprint : footprints) {
    for (std::int32_t z = footprint.minCellZ;
         z < footprint.maxCellZExclusive; ++z) {
      for (std::int32_t x = footprint.minCellX;
           x < footprint.maxCellXExclusive; ++x) {
        const std::int32_t localX = x - out.originCellX;
        const std::int32_t localZ = z - out.originCellZ;
        const std::size_t index =
            static_cast<std::size_t>(localZ) *
                static_cast<std::size_t>(out.grid.width) +
            static_cast<std::size_t>(localX);
        out.grid.walkable[index] = 1;
      }
    }
  }

  return ReachabilityProjectionStatus::Built;
}

[[nodiscard]] bool anchorKindCanSeedReachability(
    std::string_view kind) noexcept {
  return kind == "spawn" || kind == "npc" || kind == "monster";
}

[[nodiscard]] bool seedForAnchor(const RoomAnchorAsset& anchor,
                                 const ReachabilityProjection& projection,
                                 float cellSizeMeters,
                                 ReachabilityCoord& out) {
  if (!anchorKindCanSeedReachability(anchor.kind) ||
      !finiteVec3(anchor.positionMeters)) {
    return false;
  }

  const GridCellCoordResult seed =
      gridCellForPoint(anchor.positionMeters.x,
                       anchor.positionMeters.z,
                       cellSizeMeters);
  if (!seed.ok) {
    return false;
  }

  const std::int32_t localX = seed.cell.x - projection.originCellX;
  const std::int32_t localZ = seed.cell.z - projection.originCellZ;
  if (localX < 0 || localZ < 0 || localX >= projection.grid.width ||
      localZ >= projection.grid.depth) {
    return false;
  }

  const std::size_t index =
      static_cast<std::size_t>(localZ) *
          static_cast<std::size_t>(projection.grid.width) +
      static_cast<std::size_t>(localX);
  if (projection.grid.walkable[index] == 0) {
    return false;
  }

  out = {localX, localZ};
  return true;
}

}  // namespace

CreativeRoomBakeReachabilityReceipt
initialCreativeRoomBakeReachabilityReceipt(
    const CreativeRoomBakeRequest& request) {
  CreativeRoomBakeReachabilityReceipt receipt;
  receipt.requested = request.validateReachability;
  receipt.cellSizeMeters = request.reachabilityCellSizeMeters;
  if (!receipt.requested) {
    setReachabilityStatus(
        receipt,
        CreativeRoomBakeReachabilityStatus::NotRequested,
        "creative_room_bake_reachability_not_requested");
    return receipt;
  }

  setReachabilityStatus(receipt,
                        CreativeRoomBakeReachabilityStatus::NotChecked,
                        "creative_room_bake_reachability_not_checked");
  return receipt;
}

CreativeRoomBakeReachabilityReceipt validateCreativeRoomBakeReachability(
    const RoomAsset& room,
    const CreativeRoomBakeRequest& request) {
  CreativeRoomBakeReachabilityReceipt receipt =
      initialCreativeRoomBakeReachabilityReceipt(request);
  if (!receipt.requested) {
    return receipt;
  }

  if (!std::isfinite(request.reachabilityCellSizeMeters) ||
      request.reachabilityCellSizeMeters <= 0.0F) {
    setReachabilityStatus(
        receipt,
        CreativeRoomBakeReachabilityStatus::InvalidCellSize,
        "creative_room_bake_reachability_invalid_cell_size");
    return receipt;
  }

  ReachabilityProjection projection;
  const ReachabilityProjectionStatus projectionStatus =
      buildReachabilityProjection(room,
                                  request.reachabilityCellSizeMeters,
                                  projection);
  if (projectionStatus == ReachabilityProjectionStatus::GridTooLarge) {
    setReachabilityStatus(
        receipt,
        CreativeRoomBakeReachabilityStatus::GridTooLarge,
        "creative_room_bake_reachability_grid_too_large");
    return receipt;
  }
  if (projectionStatus != ReachabilityProjectionStatus::Built) {
    setReachabilityStatus(
        receipt,
        CreativeRoomBakeReachabilityStatus::NoWalkableCells,
        "creative_room_bake_reachability_no_walkable_cells");
    return receipt;
  }

  for (const std::uint8_t cell : projection.grid.walkable) {
    if (cell != 0) {
      ++receipt.walkableCellCount;
    }
  }

  std::vector<ReachabilityCoord> seeds;
  seeds.reserve(room.anchors.size());
  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (!anchorKindCanSeedReachability(anchor.kind)) {
      continue;
    }
    ++receipt.seedAnchorCount;
    ReachabilityCoord seed;
    if (seedForAnchor(anchor,
                      projection,
                      request.reachabilityCellSizeMeters,
                      seed)) {
      seeds.push_back(seed);
      ++receipt.usableSeedCount;
    } else {
      ++receipt.blockedSeedCount;
    }
  }

  if (seeds.empty()) {
    receipt.strandedCellCount = receipt.walkableCellCount;
    receipt.hasIslands = receipt.walkableCellCount > 0U;
    setReachabilityStatus(
        receipt,
        CreativeRoomBakeReachabilityStatus::NoUsableSeeds,
        "creative_room_bake_reachability_no_usable_seeds");
    return receipt;
  }

  const ReachabilityReceipt flood =
      floodFillReachability(projection.grid,
                            std::span<const ReachabilityCoord>(seeds),
                            ReachabilityConnectivity::FourWay);
  if (!flood.ok) {
    setReachabilityStatus(receipt,
                          CreativeRoomBakeReachabilityStatus::NotChecked,
                          flood.reasonCode);
    return receipt;
  }

  receipt.walkableCellCount = flood.walkableCellCount;
  receipt.reachedCellCount = flood.reachedCellCount;
  receipt.strandedCellCount = flood.strandedWalkableCount;
  receipt.hasIslands = flood.strandedWalkableCount > 0U;
  setReachabilityStatus(
      receipt,
      receipt.hasIslands
          ? CreativeRoomBakeReachabilityStatus::IslandsFound
          : CreativeRoomBakeReachabilityStatus::Reachable,
      receipt.hasIslands
          ? "creative_room_bake_reachability_islands_found"
          : "creative_room_bake_reachability_connected",
      true);
  return receipt;
}

}  // namespace iggy3d::creative

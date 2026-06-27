#include "app/iggy3d/room/ProductRoomGeometryOptimization.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace iggy3d {
namespace {

constexpr float kEpsilon = 0.0001F;
constexpr float kQuantizeScale = 10000.0F;
constexpr std::uint64_t kFloorTrianglesPerRect = 2;
constexpr std::uint64_t kWallTrianglesPerRun = 12;

std::int64_t q(float value) {
  return static_cast<std::int64_t>(std::llround(value * kQuantizeScale));
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= kEpsilon;
}

bool finitePositive(float value) {
  return std::isfinite(value) && value > kEpsilon;
}

std::uint64_t saturatingAvoided(std::uint64_t naive, std::uint64_t optimized) {
  return naive > optimized ? naive - optimized : 0;
}

auto semanticsKey(const EditableRoomSemantics& semantics) {
  return std::make_tuple(semantics.materialId,
                         semantics.traversalTags,
                         semantics.gameplayTags,
                         semantics.walkable,
                         semantics.blocksActor,
                         semantics.blocksProjectile);
}

struct FloorGroupKey {
  std::int32_t storyIndex = 0;
  std::int64_t centerY = 0;
  std::int64_t sizeX = 0;
  std::int64_t sizeY = 0;
  std::int64_t sizeZ = 0;
  decltype(semanticsKey(EditableRoomSemantics{})) semantics;
  bool locked = false;
  bool hidden = false;

  bool operator<(const FloorGroupKey& rhs) const {
    return std::tie(storyIndex,
                    centerY,
                    sizeX,
                    sizeY,
                    sizeZ,
                    semantics,
                    locked,
                    hidden) <
           std::tie(rhs.storyIndex,
                    rhs.centerY,
                    rhs.sizeX,
                    rhs.sizeY,
                    rhs.sizeZ,
                    rhs.semantics,
                    rhs.locked,
                    rhs.hidden);
  }
};

struct FloorCell {
  std::int64_t x = 0;
  std::int64_t z = 0;

  bool operator<(const FloorCell& rhs) const {
    return std::tie(z, x) < std::tie(rhs.z, rhs.x);
  }
};

bool floorToGridCell(const EditableRoomFloor& floor, FloorGroupKey& key, FloorCell& cell) {
  if (!std::isfinite(floor.centerMeters.x) || !std::isfinite(floor.centerMeters.y) ||
      !std::isfinite(floor.centerMeters.z) || !finitePositive(floor.sizeMeters.x) ||
      !finitePositive(floor.sizeMeters.y) || !finitePositive(floor.sizeMeters.z)) {
    return false;
  }

  const float gridX = floor.centerMeters.x / floor.sizeMeters.x;
  const float gridZ = floor.centerMeters.z / floor.sizeMeters.z;
  const auto roundedX = static_cast<std::int64_t>(std::llround(gridX));
  const auto roundedZ = static_cast<std::int64_t>(std::llround(gridZ));
  if (!near(gridX, static_cast<float>(roundedX)) ||
      !near(gridZ, static_cast<float>(roundedZ))) {
    return false;
  }

  key.storyIndex = floor.storyIndex;
  key.centerY = q(floor.centerMeters.y);
  key.sizeX = q(floor.sizeMeters.x);
  key.sizeY = q(floor.sizeMeters.y);
  key.sizeZ = q(floor.sizeMeters.z);
  key.semantics = semanticsKey(floor.semantics);
  key.locked = floor.locked;
  key.hidden = floor.hidden;
  cell.x = roundedX;
  cell.z = roundedZ;
  return true;
}

std::uint64_t countFloorRectangles(const std::vector<FloorCell>& cells) {
  std::set<FloorCell> remaining;
  std::uint64_t duplicateCount = 0;
  for (const FloorCell& cell : cells) {
    if (!remaining.insert(cell).second) {
      ++duplicateCount;
    }
  }

  std::uint64_t rectCount = duplicateCount;
  while (!remaining.empty()) {
    const FloorCell origin = *remaining.begin();
    std::int64_t width = 1;
    while (remaining.contains({origin.x + width, origin.z})) {
      ++width;
    }

    std::int64_t height = 1;
    bool canGrow = true;
    while (canGrow) {
      for (std::int64_t dx = 0; dx < width; ++dx) {
        if (!remaining.contains({origin.x + dx, origin.z + height})) {
          canGrow = false;
          break;
        }
      }
      if (canGrow) {
        ++height;
      }
    }

    for (std::int64_t dz = 0; dz < height; ++dz) {
      for (std::int64_t dx = 0; dx < width; ++dx) {
        remaining.erase({origin.x + dx, origin.z + dz});
      }
    }
    ++rectCount;
  }
  return rectCount;
}

std::uint64_t estimateFloorRectangles(const EditableRoomDocument& document) {
  std::map<FloorGroupKey, std::vector<FloorCell>> groups;
  std::uint64_t unmergedCount = 0;
  for (const EditableRoomFloor& floor : document.floors) {
    FloorGroupKey key;
    FloorCell cell;
    if (!floorToGridCell(floor, key, cell)) {
      ++unmergedCount;
      continue;
    }
    groups[key].push_back(cell);
  }

  std::uint64_t rectCount = unmergedCount;
  for (const auto& [key, cells] : groups) {
    (void)key;
    rectCount += countFloorRectangles(cells);
  }
  return rectCount;
}

struct WallGroupKey {
  std::int32_t storyIndex = 0;
  bool runsAlongX = true;
  std::int64_t constantAxis = 0;
  std::int64_t endpointY = 0;
  std::int64_t bottomY = 0;
  std::int64_t height = 0;
  std::int64_t thickness = 0;
  decltype(semanticsKey(EditableRoomSemantics{})) semantics;
  bool locked = false;
  bool hidden = false;

  bool operator<(const WallGroupKey& rhs) const {
    return std::tie(storyIndex,
                    runsAlongX,
                    constantAxis,
                    endpointY,
                    bottomY,
                    height,
                    thickness,
                    semantics,
                    locked,
                    hidden) <
           std::tie(rhs.storyIndex,
                    rhs.runsAlongX,
                    rhs.constantAxis,
                    rhs.endpointY,
                    rhs.bottomY,
                    rhs.height,
                    rhs.thickness,
                    rhs.semantics,
                    rhs.locked,
                    rhs.hidden);
  }
};

struct WallInterval {
  float start = 0.0F;
  float end = 0.0F;
};

bool wallToInterval(const EditableRoomWall& wall,
                    WallGroupKey& key,
                    WallInterval& interval) {
  if (!std::isfinite(wall.startMeters.x) || !std::isfinite(wall.startMeters.y) ||
      !std::isfinite(wall.startMeters.z) || !std::isfinite(wall.endMeters.x) ||
      !std::isfinite(wall.endMeters.y) || !std::isfinite(wall.endMeters.z) ||
      !std::isfinite(wall.bottomY) || !finitePositive(wall.heightMeters) ||
      !finitePositive(wall.thicknessMeters) ||
      !near(wall.startMeters.y, wall.endMeters.y)) {
    return false;
  }

  const bool sameZ = near(wall.startMeters.z, wall.endMeters.z);
  const bool sameX = near(wall.startMeters.x, wall.endMeters.x);
  if (sameZ == sameX) {
    return false;
  }

  key.storyIndex = wall.storyIndex;
  key.runsAlongX = sameZ;
  key.constantAxis = q(sameZ ? wall.startMeters.z : wall.startMeters.x);
  key.endpointY = q(wall.startMeters.y);
  key.bottomY = q(wall.bottomY);
  key.height = q(wall.heightMeters);
  key.thickness = q(wall.thicknessMeters);
  key.semantics = semanticsKey(wall.semantics);
  key.locked = wall.locked;
  key.hidden = wall.hidden;

  const float start = sameZ ? wall.startMeters.x : wall.startMeters.z;
  const float end = sameZ ? wall.endMeters.x : wall.endMeters.z;
  interval.start = std::min(start, end);
  interval.end = std::max(start, end);
  return interval.end - interval.start > kEpsilon;
}

std::uint64_t countWallRuns(std::vector<WallInterval> intervals) {
  std::sort(intervals.begin(), intervals.end(), [](const WallInterval& lhs,
                                                   const WallInterval& rhs) {
    return std::tie(lhs.start, lhs.end) < std::tie(rhs.start, rhs.end);
  });

  std::uint64_t runCount = 0;
  float currentEnd = 0.0F;
  bool hasRun = false;
  for (const WallInterval& interval : intervals) {
    if (!hasRun) {
      hasRun = true;
      ++runCount;
      currentEnd = interval.end;
      continue;
    }
    if (near(interval.start, currentEnd)) {
      currentEnd = interval.end;
      continue;
    }
    ++runCount;
    currentEnd = interval.end;
  }
  return runCount;
}

std::uint64_t estimateWallRuns(const EditableRoomDocument& document) {
  std::map<WallGroupKey, std::vector<WallInterval>> groups;
  std::uint64_t unmergedCount = 0;
  for (const EditableRoomWall& wall : document.walls) {
    WallGroupKey key;
    WallInterval interval;
    if (!wallToInterval(wall, key, interval)) {
      ++unmergedCount;
      continue;
    }
    groups[key].push_back(interval);
  }

  std::uint64_t runCount = unmergedCount;
  for (auto& [key, intervals] : groups) {
    (void)key;
    runCount += countWallRuns(std::move(intervals));
  }
  return runCount;
}

}  // namespace

ProductRoomGeometryOptimizationReport buildProductRoomGeometryOptimizationReport(
    const EditableRoomDocument* document) {
  ProductRoomGeometryOptimizationReport report;
  if (document == nullptr) {
    report.status = "product_room_geometry_document_missing";
    report.reasonCode = report.status;
    return report;
  }

  report.ok = true;
  report.status = "product_room_geometry_optimization_ready";
  report.reasonCode = report.status;
  report.sourceFloorCount = static_cast<std::uint64_t>(document->floors.size());
  report.sourceWallCount = static_cast<std::uint64_t>(document->walls.size());
  report.naiveDrawCount = report.sourceFloorCount + report.sourceWallCount;
  report.naiveTriangleCount = report.sourceFloorCount * kFloorTrianglesPerRect +
                              report.sourceWallCount * kWallTrianglesPerRun;

  report.optimizedFloorRectCount = estimateFloorRectangles(*document);
  report.optimizedWallRunCount = estimateWallRuns(*document);
  report.optimizedDrawCount =
      report.optimizedFloorRectCount + report.optimizedWallRunCount;
  report.optimizedTriangleCount =
      report.optimizedFloorRectCount * kFloorTrianglesPerRect +
      report.optimizedWallRunCount * kWallTrianglesPerRun;
  report.drawCountAvoided =
      saturatingAvoided(report.naiveDrawCount, report.optimizedDrawCount);
  report.triangleCountAvoided =
      saturatingAvoided(report.naiveTriangleCount, report.optimizedTriangleCount);
  report.floorRectMergeCandidateCount =
      saturatingAvoided(report.sourceFloorCount, report.optimizedFloorRectCount);
  report.wallRunMergeCandidateCount =
      saturatingAvoided(report.sourceWallCount, report.optimizedWallRunCount);
  return report;
}

}  // namespace iggy3d

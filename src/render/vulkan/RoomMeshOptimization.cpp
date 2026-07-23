#include "render/vulkan/RoomMeshCpuGeometryInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace iggy3d::vulkan::room_mesh_detail {

struct FloorMergeKey {
  std::string materialId;
  std::string semanticRole;
  std::int64_t positionY = 0;
  std::int64_t sizeX = 0;
  std::int64_t sizeY = 0;
  std::int64_t sizeZ = 0;

  bool operator<(const FloorMergeKey& rhs) const {
    return std::tie(materialId, semanticRole, positionY, sizeX, sizeY, sizeZ) <
           std::tie(rhs.materialId, rhs.semanticRole, rhs.positionY, rhs.sizeX,
                    rhs.sizeY, rhs.sizeZ);
  }
};

struct FloorCell {
  std::int64_t x = 0;
  std::int64_t z = 0;
  Vec3 position;
  Vec3 size;

  bool operator<(const FloorCell& rhs) const {
    return std::tie(z, x) < std::tie(rhs.z, rhs.x);
  }
};

bool floorToGridCell(const SceneRoomMeshItem& mesh,
                     FloorMergeKey& key,
                     FloorCell& cell) {
  if (mesh.role != "floor" || !std::isfinite(mesh.position.x) ||
      !std::isfinite(mesh.position.y) || !std::isfinite(mesh.position.z) ||
      !finitePositive(mesh.size.x) || !finitePositive(mesh.size.y) ||
      !finitePositive(mesh.size.z) || hasRotation(mesh.rotationEulerRadians)) {
    return false;
  }

  const float gridX = mesh.position.x / mesh.size.x;
  const float gridZ = mesh.position.z / mesh.size.z;
  const auto roundedX = static_cast<std::int64_t>(std::llround(gridX));
  const auto roundedZ = static_cast<std::int64_t>(std::llround(gridZ));
  if (!near(gridX, static_cast<float>(roundedX)) ||
      !near(gridZ, static_cast<float>(roundedZ))) {
    return false;
  }

  key.materialId = mesh.materialId;
  key.semanticRole = mesh.semanticRole;
  key.positionY = quantized(mesh.position.y);
  key.sizeX = quantized(mesh.size.x);
  key.sizeY = quantized(mesh.size.y);
  key.sizeZ = quantized(mesh.size.z);
  cell.x = roundedX;
  cell.z = roundedZ;
  cell.position = mesh.position;
  cell.size = mesh.size;
  return true;
}

void appendFloorRectsForGroup(const std::vector<FloorCell>& cells,
                              std::string_view materialId,
                              std::string_view semanticRole,
                              std::vector<FloorDraw>& floorDraws) {
  std::map<std::pair<std::int64_t, std::int64_t>, FloorCell> remaining;
  std::vector<FloorCell> duplicates;
  for (const FloorCell& cell : cells) {
    const auto key = std::make_pair(cell.x, cell.z);
    if (!remaining.emplace(key, cell).second) {
      duplicates.push_back(cell);
    }
  }

  for (const FloorCell& duplicate : duplicates) {
    floorDraws.push_back({duplicate.position, duplicate.size, {},
                          std::string(materialId), std::string(semanticRole)});
  }

  while (!remaining.empty()) {
    const FloorCell origin = remaining.begin()->second;
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

    FloorDraw draw;
    draw.size = {static_cast<float>(width) * origin.size.x,
                 origin.size.y,
                 static_cast<float>(height) * origin.size.z};
    draw.position = {
        origin.position.x + (static_cast<float>(width - 1) * origin.size.x * 0.5F),
        origin.position.y,
        origin.position.z + (static_cast<float>(height - 1) * origin.size.z * 0.5F),
    };
    draw.materialId = materialId;
    draw.semanticRole = semanticRole;
    floorDraws.push_back(draw);

    for (std::int64_t dz = 0; dz < height; ++dz) {
      for (std::int64_t dx = 0; dx < width; ++dx) {
        remaining.erase({origin.x + dx, origin.z + dz});
      }
    }
  }
}

std::vector<FloorDraw> buildOptimizedFloorDraws(const SceneRoomProjection& room) {
  std::map<FloorMergeKey, std::vector<FloorCell>> groups;
  std::vector<FloorDraw> floorDraws;
  for (const SceneRoomMeshItem& mesh : room.meshes) {
    if (mesh.role != "floor") {
      continue;
    }
    std::string_view externalAssetId;
    if (externalStaticMeshId(mesh.meshId, externalAssetId)) {
      continue;
    }
    FloorMergeKey key;
    FloorCell cell;
    if (!floorToGridCell(mesh, key, cell)) {
      floorDraws.push_back(
          {mesh.position, mesh.size, mesh.rotationEulerRadians,
           mesh.materialId, mesh.semanticRole});
      continue;
    }
    groups[key].push_back(cell);
  }

  for (const auto& [key, cells] : groups) {
    appendFloorRectsForGroup(cells, key.materialId, key.semanticRole,
                             floorDraws);
  }
  return floorDraws;
}

bool canEmitFloorDraw(const FloorDraw& floor) noexcept {
  return std::isfinite(floor.position.x) && std::isfinite(floor.position.y) &&
         std::isfinite(floor.position.z) && finitePositive(floor.size.x) &&
         finitePositive(floor.size.y) && finitePositive(floor.size.z) &&
         finiteVec3(floor.rotationEulerRadians);
}

enum class WallRunOrientation : std::uint8_t {
  AlongX,
  AlongZ,
};

struct WallSegmentSource {
  WallRunOrientation orientation = WallRunOrientation::AlongX;
  std::string materialId;
  std::string semanticRole;
  std::int64_t constantAxis = 0;
  std::int64_t endpointY = 0;
  std::int64_t bottomY = 0;
  std::int64_t height = 0;
  std::int64_t thickness = 0;
  float constantAxisMeters = 0.0F;
  float endpointYMeters = 0.0F;
  float bottomYMeters = 0.0F;
  float heightMeters = 0.0F;
  float thicknessMeters = 0.0F;
  float minCoordMeters = 0.0F;
  float maxCoordMeters = 0.0F;
};

struct WallRunKey {
  std::string materialId;
  std::string semanticRole;
  WallRunOrientation orientation = WallRunOrientation::AlongX;
  std::int64_t constantAxis = 0;
  std::int64_t endpointY = 0;
  std::int64_t bottomY = 0;
  std::int64_t height = 0;
  std::int64_t thickness = 0;

  bool operator<(const WallRunKey& rhs) const {
    return std::tie(materialId, semanticRole, orientation, constantAxis,
                    endpointY, bottomY, height, thickness) <
           std::tie(rhs.materialId, rhs.semanticRole, rhs.orientation,
                    rhs.constantAxis, rhs.endpointY, rhs.bottomY, rhs.height,
                    rhs.thickness);
  }
};

bool wallBoxFromSegment(const SceneRoomMeshItem& mesh, WallBoxDraw& draw) {
  if (!finiteVec3(mesh.wallStartMeters) || !finiteVec3(mesh.wallEndMeters) ||
      !std::isfinite(mesh.wallBottomY) || !finitePositive(mesh.wallHeightMeters) ||
      !finitePositive(mesh.wallThicknessMeters) ||
      !near(mesh.wallStartMeters.y, mesh.wallEndMeters.y)) {
    return false;
  }

  const float dx = mesh.wallEndMeters.x - mesh.wallStartMeters.x;
  const float dz = mesh.wallEndMeters.z - mesh.wallStartMeters.z;
  const bool runsAlongX = !near(dx, 0.0F) && near(dz, 0.0F);
  const bool runsAlongZ = near(dx, 0.0F) && !near(dz, 0.0F);
  if (!runsAlongX && !runsAlongZ) {
    return false;
  }

  const float length = runsAlongX ? std::fabs(dx) : std::fabs(dz);
  if (!finitePositive(length)) {
    return false;
  }

  draw.position = {(mesh.wallStartMeters.x + mesh.wallEndMeters.x) * 0.5F,
                   mesh.wallBottomY + mesh.wallHeightMeters * 0.5F,
                   (mesh.wallStartMeters.z + mesh.wallEndMeters.z) * 0.5F};
  draw.size = runsAlongX
                  ? Vec3{length, mesh.wallHeightMeters, mesh.wallThicknessMeters}
                  : Vec3{mesh.wallThicknessMeters, mesh.wallHeightMeters, length};
  draw.rotationEulerRadians = {};
  draw.materialId = mesh.materialId;
  draw.semanticRole = mesh.semanticRole;
  return true;
}

bool wallSegmentSourceFromMesh(const SceneRoomMeshItem& mesh,
                               WallSegmentSource& source) {
  if (mesh.role != "wall" || !mesh.hasWallSegment ||
      !finiteVec3(mesh.wallStartMeters) || !finiteVec3(mesh.wallEndMeters) ||
      !std::isfinite(mesh.wallBottomY) || !finitePositive(mesh.wallHeightMeters) ||
      !finitePositive(mesh.wallThicknessMeters) ||
      !near(mesh.wallStartMeters.y, mesh.wallEndMeters.y)) {
    return false;
  }

  const float dx = mesh.wallEndMeters.x - mesh.wallStartMeters.x;
  const float dz = mesh.wallEndMeters.z - mesh.wallStartMeters.z;
  const bool runsAlongX = !near(dx, 0.0F) && near(dz, 0.0F);
  const bool runsAlongZ = near(dx, 0.0F) && !near(dz, 0.0F);
  if (!runsAlongX && !runsAlongZ) {
    return false;
  }

  source.orientation = runsAlongX ? WallRunOrientation::AlongX
                                  : WallRunOrientation::AlongZ;
  source.materialId = mesh.materialId;
  source.semanticRole = mesh.semanticRole;
  source.endpointYMeters = mesh.wallStartMeters.y;
  source.bottomYMeters = mesh.wallBottomY;
  source.heightMeters = mesh.wallHeightMeters;
  source.thicknessMeters = mesh.wallThicknessMeters;
  if (runsAlongX) {
    source.constantAxisMeters = mesh.wallStartMeters.z;
    source.minCoordMeters = std::min(mesh.wallStartMeters.x, mesh.wallEndMeters.x);
    source.maxCoordMeters = std::max(mesh.wallStartMeters.x, mesh.wallEndMeters.x);
  } else {
    source.constantAxisMeters = mesh.wallStartMeters.x;
    source.minCoordMeters = std::min(mesh.wallStartMeters.z, mesh.wallEndMeters.z);
    source.maxCoordMeters = std::max(mesh.wallStartMeters.z, mesh.wallEndMeters.z);
  }
  if (!finitePositive(source.maxCoordMeters - source.minCoordMeters)) {
    return false;
  }

  source.constantAxis = quantized(source.constantAxisMeters);
  source.endpointY = quantized(source.endpointYMeters);
  source.bottomY = quantized(source.bottomYMeters);
  source.height = quantized(source.heightMeters);
  source.thickness = quantized(source.thicknessMeters);
  return true;
}

bool wallBoxForMesh(const SceneRoomMeshItem& mesh,
                    WallBoxDraw& draw) noexcept {
  if (mesh.hasWallSegment) {
    return wallBoxFromSegment(mesh, draw);
  }
  draw.position = mesh.position;
  draw.size = mesh.size;
  draw.rotationEulerRadians = mesh.rotationEulerRadians;
  draw.materialId = mesh.materialId;
  draw.semanticRole = mesh.semanticRole;
  return std::isfinite(draw.position.x) && std::isfinite(draw.position.y) &&
         std::isfinite(draw.position.z) && finitePositive(draw.size.x) &&
         finitePositive(draw.size.y) && finitePositive(draw.size.z) &&
         finiteVec3(draw.rotationEulerRadians);
}

WallBoxDraw wallBoxFromRun(const WallSegmentSource& run) {
  const float length = run.maxCoordMeters - run.minCoordMeters;
  WallBoxDraw draw;
  draw.materialId = run.materialId;
  draw.semanticRole = run.semanticRole;
  if (run.orientation == WallRunOrientation::AlongX) {
    draw.position = {(run.minCoordMeters + run.maxCoordMeters) * 0.5F,
                     run.bottomYMeters + run.heightMeters * 0.5F,
                     run.constantAxisMeters};
    draw.size = {length, run.heightMeters, run.thicknessMeters};
  } else {
    draw.position = {run.constantAxisMeters,
                     run.bottomYMeters + run.heightMeters * 0.5F,
                     (run.minCoordMeters + run.maxCoordMeters) * 0.5F};
    draw.size = {run.thicknessMeters, run.heightMeters, length};
  }
  return draw;
}

std::vector<WallBoxDraw> appendWallRunsForGroup(
    std::vector<WallSegmentSource> segments) {
  std::vector<WallBoxDraw> draws;
  std::sort(segments.begin(), segments.end(),
            [](const WallSegmentSource& lhs, const WallSegmentSource& rhs) {
              return std::tie(lhs.minCoordMeters, lhs.maxCoordMeters) <
                     std::tie(rhs.minCoordMeters, rhs.maxCoordMeters);
            });

  std::size_t index = 0;
  while (index < segments.size()) {
    WallSegmentSource run = segments[index];
    ++index;
    while (index < segments.size() &&
           near(run.maxCoordMeters, segments[index].minCoordMeters)) {
      run.maxCoordMeters = segments[index].maxCoordMeters;
      ++index;
    }
    draws.push_back(wallBoxFromRun(run));
  }
  return draws;
}

bool buildOptimizedWallDraws(const SceneRoomProjection& room,
                             std::vector<WallBoxDraw>& wallDraws) {
  std::map<WallRunKey, std::vector<WallSegmentSource>> groups;
  for (const SceneRoomMeshItem& mesh : room.meshes) {
    if (mesh.role != "wall") {
      continue;
    }
    std::string_view externalAssetId;
    if (externalStaticMeshId(mesh.meshId, externalAssetId)) {
      continue;
    }
    if (!mesh.hasWallSegment) {
      WallBoxDraw fallback;
      if (!wallBoxForMesh(mesh, fallback)) {
        return false;
      }
      wallDraws.push_back(fallback);
      continue;
    }

    WallSegmentSource segment;
    if (!wallSegmentSourceFromMesh(mesh, segment)) {
      return false;
    }
    const WallRunKey key{segment.materialId,
                         segment.semanticRole,
                         segment.orientation,
                         segment.constantAxis,
                         segment.endpointY,
                         segment.bottomY,
                         segment.height,
                         segment.thickness};
    groups[key].push_back(segment);
  }

  for (auto& [key, segments] : groups) {
    (void)key;
    std::vector<WallBoxDraw> merged = appendWallRunsForGroup(std::move(segments));
    wallDraws.insert(wallDraws.end(), merged.begin(), merged.end());
  }
  return true;
}

}  // namespace iggy3d::vulkan::room_mesh_detail

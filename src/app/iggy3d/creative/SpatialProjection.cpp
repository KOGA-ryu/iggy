#include "app/iggy3d/creative/SpatialProjection.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool isValidRequest(
    const CreativeSpatialProjectionRequest& request) noexcept {
  return isValidGridSize(request.gridSize) && request.cellSize > 0.0;
}

[[nodiscard]] bool isValidObject(const CreativeObject& object) noexcept {
  return object.id != kInvalidObjectId &&
         object.kind != CreativeObjectKind::Unknown;
}

[[nodiscard]] CreativeSpatialProjectionReceipt makeReceipt(
    CreativeSpatialProjectionStatus status,
    const CreativeObject& object,
    CreativeSpatialProjectionProfile profile,
    CreativeSpatialOccupancyKind occupancyKind,
    CreativeGridBounds3 projectedBounds,
    std::string message) {
  CreativeSpatialProjectionReceipt receipt;
  receipt.status = status;
  receipt.objectId = object.id;
  receipt.objectKind = object.kind;
  receipt.profile = profile;
  receipt.occupancyKind = occupancyKind;
  receipt.projectedBounds = projectedBounds;
  receipt.message = std::move(message);
  return receipt;
}

[[nodiscard]] CreativeSpatialProjectionReceipt makeAggregateReceipt(
    CreativeSpatialProjectionStatus status,
    std::string message) {
  CreativeObject object;
  return makeReceipt(status,
                     object,
                     CreativeSpatialProjectionProfile::Unknown,
                     CreativeSpatialOccupancyKind::Unknown,
                     {},
                     std::move(message));
}

[[nodiscard]] CreativeSpatialProjectionReceipt rejectInvalidGrid(
    const CreativeObject& object,
    CreativeSpatialProjectionProfile profile,
    CreativeSpatialOccupancyKind occupancyKind) {
  return makeReceipt(CreativeSpatialProjectionStatus::InvalidGrid,
                     object,
                     profile,
                     occupancyKind,
                     {},
                     "invalid_grid");
}

[[nodiscard]] CreativeSpatialProjectionReceipt rejectInvalidObject(
    const CreativeObject& object,
    CreativeSpatialProjectionProfile profile,
    CreativeSpatialOccupancyKind occupancyKind) {
  return makeReceipt(CreativeSpatialProjectionStatus::InvalidObject,
                     object,
                     profile,
                     occupancyKind,
                     {},
                     "invalid_object");
}

[[nodiscard]] CreativeSpatialProjectionReceipt rejectHiddenObject(
    const CreativeObject& object,
    CreativeSpatialProjectionProfile profile,
    CreativeSpatialOccupancyKind occupancyKind) {
  return makeReceipt(CreativeSpatialProjectionStatus::NoProjection,
                     object,
                     profile,
                     occupancyKind,
                     {},
                     "object_hidden");
}

[[nodiscard]] CreativeGridCoord3 clampedCoord(
    CreativeGridCoord3 coord,
    CreativeGridSize3 size) noexcept {
  return CreativeGridCoord3{
      std::clamp(coord.x, std::int32_t{0}, size.width - 1),
      std::clamp(coord.y, std::int32_t{0}, size.height - 1),
      std::clamp(coord.z, std::int32_t{0}, size.depth - 1),
  };
}

[[nodiscard]] bool boundsOutsideGrid(CreativeGridBounds3 bounds,
                                     CreativeGridSize3 size) noexcept {
  return bounds.min.x < 0 || bounds.min.y < 0 || bounds.min.z < 0 ||
         bounds.max.x > size.width || bounds.max.y > size.height ||
         bounds.max.z > size.depth;
}

[[nodiscard]] std::uint64_t cellCount(CreativeGridBounds3 bounds) noexcept {
  const auto width = static_cast<std::uint64_t>(bounds.max.x - bounds.min.x);
  const auto height = static_cast<std::uint64_t>(bounds.max.y - bounds.min.y);
  const auto depth = static_cast<std::uint64_t>(bounds.max.z - bounds.min.z);
  return width * height * depth;
}

void fillBoundsCells(std::vector<CreativeSpatialCell>& cells,
                     CreativeGridSize3 size,
                     CreativeGridBounds3 bounds,
                     const CreativeObject& object,
                     CreativeSpatialOccupancyKind occupancyKind) {
  for (std::int32_t z = bounds.min.z; z < bounds.max.z; ++z) {
    for (std::int32_t y = bounds.min.y; y < bounds.max.y; ++y) {
      for (std::int32_t x = bounds.min.x; x < bounds.max.x; ++x) {
        const CreativeGridCoord3 coord{x, y, z};
        cells.push_back(CreativeSpatialCell{toGridIndex(coord, size),
                                            coord,
                                            object.id,
                                            object.kind,
                                            occupancyKind});
      }
    }
  }
}

[[nodiscard]] CreativeGridBounds3 pointBounds(CreativeGridCoord3 coord) noexcept {
  return CreativeGridBounds3{
      coord,
      CreativeGridCoord3{coord.x + 1, coord.y + 1, coord.z + 1},
  };
}

[[nodiscard]] CreativeGridBounds3 lineBounds(CreativeGridCoord3 start,
                                             CreativeGridCoord3 end) noexcept {
  return CreativeGridBounds3{
      CreativeGridCoord3{
          std::min(start.x, end.x),
          std::min(start.y, end.y),
          std::min(start.z, end.z),
      },
      CreativeGridCoord3{
          std::max(start.x, end.x) + 1,
          std::max(start.y, end.y) + 1,
          std::max(start.z, end.z) + 1,
      },
  };
}

[[nodiscard]] CreativeSpatialProjectionReceipt projectBoundsObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    CreativeSpatialProjectionProfile profile) {
  const CreativeSpatialOccupancyKind occupancyKind =
      occupancyKindForObject(object.kind);
  if (!isValidRequest(request)) {
    return rejectInvalidGrid(object, profile, occupancyKind);
  }
  if (!isValidObject(object)) {
    return rejectInvalidObject(object, profile, occupancyKind);
  }
  if (!object.visible) {
    return rejectHiddenObject(object, profile, occupancyKind);
  }

  CreativeGridBounds3 bounds =
      worldBoundsToGridBounds(object.bounds, request.cellSize);
  if (!request.clampToGrid && boundsOutsideGrid(bounds, request.gridSize)) {
    return makeReceipt(CreativeSpatialProjectionStatus::OutOfBounds,
                       object,
                       profile,
                       occupancyKind,
                       bounds,
                       "out_of_bounds");
  }

  bounds = clampGridBounds(bounds, request.gridSize);
  if (isEmptyGridBounds(bounds)) {
    return makeReceipt(CreativeSpatialProjectionStatus::EmptyProjection,
                       object,
                       profile,
                       occupancyKind,
                       bounds,
                       "empty_projection");
  }

  CreativeSpatialProjectionReceipt receipt =
      makeReceipt(CreativeSpatialProjectionStatus::Projected,
                  object,
                  profile,
                  occupancyKind,
                  bounds,
                  "projected");
  receipt.cells.reserve(static_cast<std::size_t>(cellCount(bounds)));
  fillBoundsCells(receipt.cells,
                  request.gridSize,
                  bounds,
                  object,
                  occupancyKind);
  return receipt;
}

[[nodiscard]] CreativeSpatialProjectionStatus mergeAggregateStatus(
    CreativeSpatialProjectionStatus current,
    CreativeSpatialProjectionStatus next) noexcept {
  if (current == CreativeSpatialProjectionStatus::Unknown) {
    return next;
  }
  if (current == CreativeSpatialProjectionStatus::InvalidGrid ||
      next == CreativeSpatialProjectionStatus::InvalidGrid) {
    return CreativeSpatialProjectionStatus::InvalidGrid;
  }
  if (current == CreativeSpatialProjectionStatus::InvalidObject ||
      next == CreativeSpatialProjectionStatus::InvalidObject) {
    return CreativeSpatialProjectionStatus::InvalidObject;
  }
  if (current == CreativeSpatialProjectionStatus::OutOfBounds ||
      next == CreativeSpatialProjectionStatus::OutOfBounds) {
    return CreativeSpatialProjectionStatus::OutOfBounds;
  }
  if (current == CreativeSpatialProjectionStatus::EmptyProjection ||
      next == CreativeSpatialProjectionStatus::EmptyProjection) {
    return CreativeSpatialProjectionStatus::EmptyProjection;
  }
  return CreativeSpatialProjectionStatus::NoProjection;
}

}  // namespace

std::string_view toString(CreativeSpatialProjectionProfile profile) noexcept {
  switch (profile) {
    case CreativeSpatialProjectionProfile::Unknown:
      return "Unknown";
    case CreativeSpatialProjectionProfile::NoProjection:
      return "NoProjection";
    case CreativeSpatialProjectionProfile::PointProjection:
      return "PointProjection";
    case CreativeSpatialProjectionProfile::BoxProjection:
      return "BoxProjection";
    case CreativeSpatialProjectionProfile::VolumeProjection:
      return "VolumeProjection";
    case CreativeSpatialProjectionProfile::LineProjection:
      return "LineProjection";
    case CreativeSpatialProjectionProfile::LinkProjection:
      return "LinkProjection";
  }
  return "Unknown";
}

std::string_view toString(
    CreativeSpatialOccupancyKind occupancyKind) noexcept {
  switch (occupancyKind) {
    case CreativeSpatialOccupancyKind::Unknown:
      return "Unknown";
    case CreativeSpatialOccupancyKind::Structural:
      return "Structural";
    case CreativeSpatialOccupancyKind::Collision:
      return "Collision";
    case CreativeSpatialOccupancyKind::Navigation:
      return "Navigation";
    case CreativeSpatialOccupancyKind::Trigger:
      return "Trigger";
    case CreativeSpatialOccupancyKind::Gameplay:
      return "Gameplay";
    case CreativeSpatialOccupancyKind::Light:
      return "Light";
    case CreativeSpatialOccupancyKind::Audio:
      return "Audio";
    case CreativeSpatialOccupancyKind::Camera:
      return "Camera";
    case CreativeSpatialOccupancyKind::Testing:
      return "Testing";
    case CreativeSpatialOccupancyKind::Authoring:
      return "Authoring";
  }
  return "Unknown";
}

std::string_view toString(CreativeSpatialProjectionStatus status) noexcept {
  switch (status) {
    case CreativeSpatialProjectionStatus::Unknown:
      return "Unknown";
    case CreativeSpatialProjectionStatus::InvalidGrid:
      return "InvalidGrid";
    case CreativeSpatialProjectionStatus::InvalidObject:
      return "InvalidObject";
    case CreativeSpatialProjectionStatus::NoProjection:
      return "NoProjection";
    case CreativeSpatialProjectionStatus::EmptyProjection:
      return "EmptyProjection";
    case CreativeSpatialProjectionStatus::OutOfBounds:
      return "OutOfBounds";
    case CreativeSpatialProjectionStatus::Projected:
      return "Projected";
  }
  return "Unknown";
}

bool isValidGridSize(CreativeGridSize3 size) noexcept {
  return size.width > 0 && size.height > 0 && size.depth > 0;
}

bool isInsideGrid(CreativeGridCoord3 coord, CreativeGridSize3 size) noexcept {
  return coord.x >= 0 && coord.x < size.width && coord.y >= 0 &&
         coord.y < size.height && coord.z >= 0 && coord.z < size.depth;
}

CreativeGridIndex toGridIndex(CreativeGridCoord3 coord,
                              CreativeGridSize3 size) noexcept {
  const auto width = static_cast<CreativeGridIndex>(size.width);
  const auto height = static_cast<CreativeGridIndex>(size.height);
  return static_cast<CreativeGridIndex>(coord.z) * width * height +
         static_cast<CreativeGridIndex>(coord.y) * width +
         static_cast<CreativeGridIndex>(coord.x);
}

CreativeGridCoord3 toGridCoord(CreativeGridIndex index,
                               CreativeGridSize3 size) noexcept {
  if (size.width <= 0 || size.height <= 0) {
    return {};
  }
  const auto width = static_cast<CreativeGridIndex>(size.width);
  const auto height = static_cast<CreativeGridIndex>(size.height);
  const auto layer = width * height;
  return CreativeGridCoord3{
      static_cast<std::int32_t>(index % width),
      static_cast<std::int32_t>((index / width) % height),
      static_cast<std::int32_t>(index / layer),
  };
}

CreativeGridCoord3 worldToGridCoord(CreativeVec3 position,
                                    double cellSize) noexcept {
  if (cellSize <= 0.0) {
    return {};
  }
  return CreativeGridCoord3{
      static_cast<std::int32_t>(std::floor(position.x / cellSize)),
      static_cast<std::int32_t>(std::floor(position.y / cellSize)),
      static_cast<std::int32_t>(std::floor(position.z / cellSize)),
  };
}

CreativeGridBounds3 worldBoundsToGridBounds(CreativeBounds bounds,
                                            double cellSize) noexcept {
  if (cellSize <= 0.0) {
    return {};
  }
  return CreativeGridBounds3{
      worldToGridCoord(bounds.min, cellSize),
      CreativeGridCoord3{
          static_cast<std::int32_t>(std::ceil(bounds.max.x / cellSize)),
          static_cast<std::int32_t>(std::ceil(bounds.max.y / cellSize)),
          static_cast<std::int32_t>(std::ceil(bounds.max.z / cellSize)),
      },
  };
}

CreativeGridBounds3 clampGridBounds(CreativeGridBounds3 bounds,
                                    CreativeGridSize3 size) noexcept {
  return CreativeGridBounds3{
      CreativeGridCoord3{
          std::clamp(bounds.min.x, std::int32_t{0}, size.width),
          std::clamp(bounds.min.y, std::int32_t{0}, size.height),
          std::clamp(bounds.min.z, std::int32_t{0}, size.depth),
      },
      CreativeGridCoord3{
          std::clamp(bounds.max.x, std::int32_t{0}, size.width),
          std::clamp(bounds.max.y, std::int32_t{0}, size.height),
          std::clamp(bounds.max.z, std::int32_t{0}, size.depth),
      },
  };
}

bool isEmptyGridBounds(CreativeGridBounds3 bounds) noexcept {
  return bounds.min.x >= bounds.max.x || bounds.min.y >= bounds.max.y ||
         bounds.min.z >= bounds.max.z;
}

CreativeSpatialProjectionProfile projectionProfileForObject(
    CreativeObjectKind kind) noexcept {
  switch (kind) {
    case CreativeObjectKind::Unknown:
    case CreativeObjectKind::ConditionGate:
    case CreativeObjectKind::EventRelay:
    case CreativeObjectKind::NavLink:
    case CreativeObjectKind::JumpLink:
    case CreativeObjectKind::ClimbLink:
    case CreativeObjectKind::PatrolRoute:
    case CreativeObjectKind::CutsceneMarker:
    case CreativeObjectKind::Group:
      return CreativeSpatialProjectionProfile::NoProjection;

    case CreativeObjectKind::SpawnPoint:
    case CreativeObjectKind::ExitPoint:
    case CreativeObjectKind::EntrancePoint:
    case CreativeObjectKind::Checkpoint:
    case CreativeObjectKind::CoverPoint:
    case CreativeObjectKind::PatrolNode:
    case CreativeObjectKind::Switch:
    case CreativeObjectKind::Lever:
    case CreativeObjectKind::Button:
    case CreativeObjectKind::Spawner:
    case CreativeObjectKind::ScriptMarker:
    case CreativeObjectKind::CollisionProbe:
    case CreativeObjectKind::PhysicsProbe:
    case CreativeObjectKind::TestStart:
    case CreativeObjectKind::TestEnd:
    case CreativeObjectKind::Decoration:
    case CreativeObjectKind::PointLight:
    case CreativeObjectKind::SpotLight:
    case CreativeObjectKind::SoundEmitter:
    case CreativeObjectKind::CameraMarker:
    case CreativeObjectKind::CameraTarget:
    case CreativeObjectKind::Note:
    case CreativeObjectKind::Label:
    case CreativeObjectKind::Comment:
    case CreativeObjectKind::MeasurementMarker:
    case CreativeObjectKind::GridAnchor:
    case CreativeObjectKind::SnapAnchor:
    case CreativeObjectKind::Socket:
    case CreativeObjectKind::AttachmentPoint:
    case CreativeObjectKind::EnemySpawn:
    case CreativeObjectKind::NpcSpawn:
    case CreativeObjectKind::InterestPoint:
    case CreativeObjectKind::ResourceNode:
    case CreativeObjectKind::LootPoint:
    case CreativeObjectKind::QuestMarker:
    case CreativeObjectKind::DialogueMarker:
      return CreativeSpatialProjectionProfile::PointProjection;

    case CreativeObjectKind::Room:
    case CreativeObjectKind::Wall:
    case CreativeObjectKind::Floor:
    case CreativeObjectKind::Ceiling:
    case CreativeObjectKind::Roof:
    case CreativeObjectKind::Door:
    case CreativeObjectKind::Window:
    case CreativeObjectKind::Stair:
    case CreativeObjectKind::Ramp:
    case CreativeObjectKind::Platform:
    case CreativeObjectKind::MovingPlatform:
    case CreativeObjectKind::Column:
    case CreativeObjectKind::Pillar:
    case CreativeObjectKind::Beam:
    case CreativeObjectKind::Arch:
    case CreativeObjectKind::Fence:
    case CreativeObjectKind::Railing:
    case CreativeObjectKind::Bridge:
    case CreativeObjectKind::Ladder:
    case CreativeObjectKind::TerrainPatch:
    case CreativeObjectKind::Slope:
    case CreativeObjectKind::Cliff:
    case CreativeObjectKind::CaveOpening:
    case CreativeObjectKind::WallRunSurface:
    case CreativeObjectKind::SlideSurface:
    case CreativeObjectKind::TestLane:
    case CreativeObjectKind::DistanceMarker:
    case CreativeObjectKind::SpeedMarker:
    case CreativeObjectKind::JumpTarget:
    case CreativeObjectKind::CoyoteTimeLedge:
    case CreativeObjectKind::FallShaft:
    case CreativeObjectKind::TimingGate:
    case CreativeObjectKind::Prop:
    case CreativeObjectKind::Decal:
    case CreativeObjectKind::Sign:
    case CreativeObjectKind::Banner:
    case CreativeObjectKind::FoliagePatch:
    case CreativeObjectKind::Rock:
    case CreativeObjectKind::Crate:
    case CreativeObjectKind::Barrel:
    case CreativeObjectKind::Furniture:
    case CreativeObjectKind::AreaLight:
    case CreativeObjectKind::ReferenceImage:
    case CreativeObjectKind::BlueprintOverlay:
    case CreativeObjectKind::MeasurementLine:
    case CreativeObjectKind::MeasurementBox:
    case CreativeObjectKind::PrefabInstance:
      return CreativeSpatialProjectionProfile::BoxProjection;

    case CreativeObjectKind::WaterVolume:
    case CreativeObjectKind::LavaVolume:
    case CreativeObjectKind::Pit:
    case CreativeObjectKind::BoundaryVolume:
    case CreativeObjectKind::KillPlane:
    case CreativeObjectKind::NavRegion:
    case CreativeObjectKind::TriggerZone:
    case CreativeObjectKind::PressurePlate:
    case CreativeObjectKind::DespawnZone:
    case CreativeObjectKind::AlertZone:
    case CreativeObjectKind::SafeZone:
    case CreativeObjectKind::DangerZone:
    case CreativeObjectKind::AmbientZone:
    case CreativeObjectKind::ReverbZone:
    case CreativeObjectKind::MusicZone:
      return CreativeSpatialProjectionProfile::VolumeProjection;

    case CreativeObjectKind::CameraRail:
      return CreativeSpatialProjectionProfile::LineProjection;
  }
  return CreativeSpatialProjectionProfile::Unknown;
}

CreativeSpatialOccupancyKind occupancyKindForObject(
    CreativeObjectKind kind) noexcept {
  switch (kind) {
    case CreativeObjectKind::BoundaryVolume:
    case CreativeObjectKind::KillPlane:
    case CreativeObjectKind::Pit:
      return CreativeSpatialOccupancyKind::Collision;

    case CreativeObjectKind::SpawnPoint:
    case CreativeObjectKind::ExitPoint:
    case CreativeObjectKind::EntrancePoint:
    case CreativeObjectKind::Checkpoint:
    case CreativeObjectKind::NavRegion:
    case CreativeObjectKind::NavLink:
    case CreativeObjectKind::JumpLink:
    case CreativeObjectKind::ClimbLink:
    case CreativeObjectKind::WallRunSurface:
    case CreativeObjectKind::SlideSurface:
    case CreativeObjectKind::CoverPoint:
    case CreativeObjectKind::PatrolNode:
    case CreativeObjectKind::Stair:
    case CreativeObjectKind::Ramp:
    case CreativeObjectKind::Platform:
    case CreativeObjectKind::MovingPlatform:
    case CreativeObjectKind::Bridge:
    case CreativeObjectKind::Ladder:
      return CreativeSpatialOccupancyKind::Navigation;

    case CreativeObjectKind::TriggerZone:
    case CreativeObjectKind::PressurePlate:
    case CreativeObjectKind::DespawnZone:
      return CreativeSpatialOccupancyKind::Trigger;

    case CreativeObjectKind::EnemySpawn:
    case CreativeObjectKind::NpcSpawn:
    case CreativeObjectKind::PatrolRoute:
    case CreativeObjectKind::InterestPoint:
    case CreativeObjectKind::AlertZone:
    case CreativeObjectKind::SafeZone:
    case CreativeObjectKind::DangerZone:
    case CreativeObjectKind::ResourceNode:
    case CreativeObjectKind::LootPoint:
    case CreativeObjectKind::QuestMarker:
    case CreativeObjectKind::DialogueMarker:
    case CreativeObjectKind::Spawner:
    case CreativeObjectKind::Switch:
    case CreativeObjectKind::Lever:
    case CreativeObjectKind::Button:
    case CreativeObjectKind::ConditionGate:
    case CreativeObjectKind::EventRelay:
    case CreativeObjectKind::ScriptMarker:
      return CreativeSpatialOccupancyKind::Gameplay;

    case CreativeObjectKind::PointLight:
    case CreativeObjectKind::SpotLight:
    case CreativeObjectKind::AreaLight:
    case CreativeObjectKind::AmbientZone:
      return CreativeSpatialOccupancyKind::Light;

    case CreativeObjectKind::ReverbZone:
    case CreativeObjectKind::SoundEmitter:
    case CreativeObjectKind::MusicZone:
      return CreativeSpatialOccupancyKind::Audio;

    case CreativeObjectKind::CameraMarker:
    case CreativeObjectKind::CameraRail:
    case CreativeObjectKind::CameraTarget:
    case CreativeObjectKind::CutsceneMarker:
      return CreativeSpatialOccupancyKind::Camera;

    case CreativeObjectKind::TestLane:
    case CreativeObjectKind::DistanceMarker:
    case CreativeObjectKind::SpeedMarker:
    case CreativeObjectKind::JumpTarget:
    case CreativeObjectKind::CoyoteTimeLedge:
    case CreativeObjectKind::FallShaft:
    case CreativeObjectKind::CollisionProbe:
    case CreativeObjectKind::PhysicsProbe:
    case CreativeObjectKind::TimingGate:
    case CreativeObjectKind::TestStart:
    case CreativeObjectKind::TestEnd:
      return CreativeSpatialOccupancyKind::Testing;

    case CreativeObjectKind::Note:
    case CreativeObjectKind::Label:
    case CreativeObjectKind::Comment:
    case CreativeObjectKind::MeasurementMarker:
    case CreativeObjectKind::MeasurementLine:
    case CreativeObjectKind::MeasurementBox:
    case CreativeObjectKind::GridAnchor:
    case CreativeObjectKind::SnapAnchor:
    case CreativeObjectKind::ReferenceImage:
    case CreativeObjectKind::BlueprintOverlay:
    case CreativeObjectKind::Group:
    case CreativeObjectKind::Socket:
    case CreativeObjectKind::AttachmentPoint:
      return CreativeSpatialOccupancyKind::Authoring;

    case CreativeObjectKind::Room:
    case CreativeObjectKind::Wall:
    case CreativeObjectKind::Floor:
    case CreativeObjectKind::Ceiling:
    case CreativeObjectKind::Roof:
    case CreativeObjectKind::Door:
    case CreativeObjectKind::Window:
    case CreativeObjectKind::Column:
    case CreativeObjectKind::Pillar:
    case CreativeObjectKind::Beam:
    case CreativeObjectKind::Arch:
    case CreativeObjectKind::Fence:
    case CreativeObjectKind::Railing:
    case CreativeObjectKind::TerrainPatch:
    case CreativeObjectKind::Slope:
    case CreativeObjectKind::Cliff:
    case CreativeObjectKind::CaveOpening:
    case CreativeObjectKind::Rock:
    case CreativeObjectKind::Crate:
    case CreativeObjectKind::Barrel:
    case CreativeObjectKind::Furniture:
    case CreativeObjectKind::Prop:
      return CreativeSpatialOccupancyKind::Structural;

    case CreativeObjectKind::Unknown:
    case CreativeObjectKind::WaterVolume:
    case CreativeObjectKind::LavaVolume:
    case CreativeObjectKind::Decal:
    case CreativeObjectKind::Sign:
    case CreativeObjectKind::Banner:
    case CreativeObjectKind::FoliagePatch:
    case CreativeObjectKind::Decoration:
    case CreativeObjectKind::PrefabInstance:
      return CreativeSpatialOccupancyKind::Unknown;
  }
  return CreativeSpatialOccupancyKind::Unknown;
}

CreativeSpatialProjectionReceipt projectObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request) {
  const CreativeSpatialProjectionProfile profile =
      projectionProfileForObject(object.kind);
  const CreativeSpatialOccupancyKind occupancyKind =
      occupancyKindForObject(object.kind);

  if (!isValidRequest(request)) {
    return rejectInvalidGrid(object, profile, occupancyKind);
  }
  if (!isValidObject(object)) {
    return rejectInvalidObject(object, profile, occupancyKind);
  }
  if (!object.visible) {
    return rejectHiddenObject(object, profile, occupancyKind);
  }
  if (!request.includeAuthoringOnly &&
      occupancyKind == CreativeSpatialOccupancyKind::Authoring) {
    return makeReceipt(CreativeSpatialProjectionStatus::NoProjection,
                       object,
                       profile,
                       occupancyKind,
                       {},
                       "authoring_excluded");
  }

  switch (profile) {
    case CreativeSpatialProjectionProfile::Unknown:
    case CreativeSpatialProjectionProfile::NoProjection:
      return makeReceipt(CreativeSpatialProjectionStatus::NoProjection,
                         object,
                         profile,
                         occupancyKind,
                         {},
                         "no_projection");
    case CreativeSpatialProjectionProfile::PointProjection:
      return projectPointObjectToGrid(object, request);
    case CreativeSpatialProjectionProfile::BoxProjection:
      return projectBoxObjectToGrid(object, request);
    case CreativeSpatialProjectionProfile::VolumeProjection:
      return projectVolumeObjectToGrid(object, request);
    case CreativeSpatialProjectionProfile::LineProjection:
      return projectLineObjectToGrid(object, request);
    case CreativeSpatialProjectionProfile::LinkProjection:
      return projectLinkObjectToGrid(object, request);
  }

  return makeReceipt(CreativeSpatialProjectionStatus::NoProjection,
                     object,
                     CreativeSpatialProjectionProfile::Unknown,
                     occupancyKind,
                     {},
                     "no_projection");
}

CreativeSpatialProjectionReceipt projectPointObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request) {
  const CreativeSpatialProjectionProfile profile =
      CreativeSpatialProjectionProfile::PointProjection;
  const CreativeSpatialOccupancyKind occupancyKind =
      occupancyKindForObject(object.kind);
  if (!isValidRequest(request)) {
    return rejectInvalidGrid(object, profile, occupancyKind);
  }
  if (!isValidObject(object)) {
    return rejectInvalidObject(object, profile, occupancyKind);
  }
  if (!object.visible) {
    return rejectHiddenObject(object, profile, occupancyKind);
  }

  CreativeGridCoord3 coord =
      worldToGridCoord(object.transform.position, request.cellSize);
  if (!isInsideGrid(coord, request.gridSize)) {
    if (!request.clampToGrid) {
      return makeReceipt(CreativeSpatialProjectionStatus::OutOfBounds,
                         object,
                         profile,
                         occupancyKind,
                         pointBounds(coord),
                         "out_of_bounds");
    }
    coord = clampedCoord(coord, request.gridSize);
  }

  CreativeSpatialProjectionReceipt receipt =
      makeReceipt(CreativeSpatialProjectionStatus::Projected,
                  object,
                  profile,
                  occupancyKind,
                  pointBounds(coord),
                  "projected");
  receipt.cells.reserve(1);
  receipt.cells.push_back(CreativeSpatialCell{toGridIndex(coord,
                                                         request.gridSize),
                                              coord,
                                              object.id,
                                              object.kind,
                                              occupancyKind});
  return receipt;
}

CreativeSpatialProjectionReceipt projectBoxObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request) {
  return projectBoundsObjectToGrid(object,
                                   request,
                                   CreativeSpatialProjectionProfile::BoxProjection);
}

CreativeSpatialProjectionReceipt projectVolumeObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request) {
  return projectBoundsObjectToGrid(
      object,
      request,
      CreativeSpatialProjectionProfile::VolumeProjection);
}

CreativeSpatialProjectionReceipt projectLineObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request) {
  const CreativeSpatialProjectionProfile profile =
      CreativeSpatialProjectionProfile::LineProjection;
  const CreativeSpatialOccupancyKind occupancyKind =
      occupancyKindForObject(object.kind);
  if (!isValidRequest(request)) {
    return rejectInvalidGrid(object, profile, occupancyKind);
  }
  if (!isValidObject(object)) {
    return rejectInvalidObject(object, profile, occupancyKind);
  }
  if (!object.visible) {
    return rejectHiddenObject(object, profile, occupancyKind);
  }

  CreativeGridCoord3 start =
      worldToGridCoord(object.bounds.min, request.cellSize);
  CreativeGridCoord3 end =
      worldToGridCoord(object.bounds.max, request.cellSize);
  if (!isInsideGrid(start, request.gridSize) ||
      !isInsideGrid(end, request.gridSize)) {
    if (!request.clampToGrid) {
      return makeReceipt(CreativeSpatialProjectionStatus::OutOfBounds,
                         object,
                         profile,
                         occupancyKind,
                         lineBounds(start, end),
                         "out_of_bounds");
    }
    start = clampedCoord(start, request.gridSize);
    end = clampedCoord(end, request.gridSize);
  }

  const std::int32_t dx = end.x - start.x;
  const std::int32_t dy = end.y - start.y;
  const std::int32_t dz = end.z - start.z;
  const std::int32_t steps = std::max({std::abs(dx), std::abs(dy), std::abs(dz)});

  CreativeSpatialProjectionReceipt receipt =
      makeReceipt(CreativeSpatialProjectionStatus::Projected,
                  object,
                  profile,
                  occupancyKind,
                  lineBounds(start, end),
                  "projected");
  receipt.cells.reserve(static_cast<std::size_t>(steps + 1));

  CreativeGridCoord3 previous{-1, -1, -1};
  for (std::int32_t step = 0; step <= steps; ++step) {
    const double t = steps == 0 ? 0.0 : static_cast<double>(step) / steps;
    const CreativeGridCoord3 coord{
        start.x + static_cast<std::int32_t>(std::round(dx * t)),
        start.y + static_cast<std::int32_t>(std::round(dy * t)),
        start.z + static_cast<std::int32_t>(std::round(dz * t)),
    };
    if (coord.x == previous.x && coord.y == previous.y &&
        coord.z == previous.z) {
      continue;
    }
    previous = coord;
    receipt.cells.push_back(CreativeSpatialCell{toGridIndex(coord,
                                                           request.gridSize),
                                                coord,
                                                object.id,
                                                object.kind,
                                                occupancyKind});
  }

  if (receipt.cells.empty()) {
    receipt.status = CreativeSpatialProjectionStatus::EmptyProjection;
    receipt.message = "empty_projection";
  }
  return receipt;
}

CreativeSpatialProjectionReceipt projectLinkObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request) {
  (void)request;
  const CreativeSpatialProjectionProfile profile =
      CreativeSpatialProjectionProfile::LinkProjection;
  const CreativeSpatialOccupancyKind occupancyKind =
      occupancyKindForObject(object.kind);
  if (!isValidObject(object)) {
    return rejectInvalidObject(object, profile, occupancyKind);
  }
  if (!object.visible) {
    return rejectHiddenObject(object, profile, occupancyKind);
  }
  return makeReceipt(CreativeSpatialProjectionStatus::NoProjection,
                     object,
                     profile,
                     occupancyKind,
                     {},
                     "no_projection");
}

CreativeSpatialProjectionReceipt projectObjectsToGrid(
    std::span<const CreativeObject> objects,
    const CreativeSpatialProjectionRequest& request) {
  if (!isValidRequest(request)) {
    return makeAggregateReceipt(CreativeSpatialProjectionStatus::InvalidGrid,
                                "invalid_grid");
  }

  CreativeSpatialProjectionReceipt aggregate =
      makeAggregateReceipt(CreativeSpatialProjectionStatus::Unknown,
                           "aggregate_empty");

  for (const CreativeObject& object : objects) {
    CreativeSpatialProjectionReceipt receipt = projectObjectToGrid(object,
                                                                   request);
    if (!receipt.cells.empty()) {
      aggregate.cells.reserve(aggregate.cells.size() + receipt.cells.size());
      aggregate.cells.insert(aggregate.cells.end(),
                             receipt.cells.begin(),
                             receipt.cells.end());
    }
    aggregate.status = mergeAggregateStatus(aggregate.status, receipt.status);
  }

  if (!aggregate.cells.empty()) {
    aggregate.status = CreativeSpatialProjectionStatus::Projected;
    aggregate.message = "projected";
  } else if (aggregate.status == CreativeSpatialProjectionStatus::Unknown) {
    aggregate.status = CreativeSpatialProjectionStatus::EmptyProjection;
    aggregate.message = "empty_projection";
  } else {
    aggregate.message = toString(aggregate.status);
  }

  return aggregate;
}

}  // namespace iggy3d::creative

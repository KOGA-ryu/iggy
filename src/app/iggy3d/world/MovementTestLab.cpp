#include "app/iggy3d/world/MovementTestLab.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d {
namespace {

enum class MovementLabObjectKind {
  Marker,
  Platform,
  WallRunSurface,
  CollisionSurface,
  SnagProp,
  Ledge,
};

constexpr std::size_t kMovementLabObjectKindCount = 6U;

struct MovementLabObjectSpec {
  std::string_view laneId;
  std::string_view objectId;
  std::string_view assetId;
  MovementLabObjectKind kind = MovementLabObjectKind::Marker;
  std::size_t row = 0U;
  std::size_t column = 0U;
  Vec3 sizeMeters{1.0F, 1.0F, 1.0F};
  float yawDegrees = 0.0F;
};

struct MovementLabObjectKindDescriptor {
  std::array<std::string_view, 2> tags;
  std::size_t tagCount = 0U;
  bool blocksMovement = false;
};

constexpr std::array<MovementLabObjectKindDescriptor, kMovementLabObjectKindCount>
    kObjectKindDescriptors{{
        {{{"marker", ""}}, 1U, false},
        {{{"platform", ""}}, 1U, true},
        {{{"wall_run", ""}}, 1U, true},
        {{{"collision_slide", ""}}, 1U, true},
        {{{"snag", "crate"}}, 2U, true},
        {{{"ledge", "clamber"}}, 2U, true},
    }};

constexpr std::array<MovementLabObjectSpec, 22> kMovementLabObjects{{
    {"flat_speed_lane", "ten_meter_marker", "stone_floor_slab",
     MovementLabObjectKind::Marker, 2U, 8U, {0.30F, 0.08F, 1.40F}, 0.0F},
    {"flat_speed_lane", "twenty_meter_marker", "stone_floor_slab",
     MovementLabObjectKind::Marker, 2U, 18U, {0.30F, 0.08F, 1.40F}, 0.0F},
    {"flat_speed_lane", "thirty_meter_marker", "stone_floor_slab",
     MovementLabObjectKind::Marker, 2U, 28U, {0.30F, 0.08F, 1.40F}, 0.0F},
    {"flat_speed_lane", "finish_marker", "stone_floor_slab",
     MovementLabObjectKind::Marker, 2U, 42U, {0.30F, 0.08F, 1.40F}, 0.0F},

    {"jump_coyote_lane", "coyote_edge_platform", "stone_block_proxy",
     MovementLabObjectKind::Platform, 5U, 12U, {3.0F, 0.60F, 2.0F}, 0.0F},
    {"jump_coyote_lane", "short_hop_platform", "stone_block_proxy",
     MovementLabObjectKind::Platform, 5U, 21U, {2.0F, 1.00F, 2.0F}, 0.0F},
    {"jump_coyote_lane", "full_jump_platform", "stone_block_proxy",
     MovementLabObjectKind::Platform, 5U, 30U, {2.5F, 1.40F, 2.0F}, 0.0F},
    {"jump_coyote_lane", "landing_platform", "stone_block_proxy",
     MovementLabObjectKind::Platform, 5U, 42U, {4.0F, 0.80F, 2.0F}, 0.0F},
    {"jump_coyote_lane", "fall_shaft_placeholder", "stone_floor_slab",
     MovementLabObjectKind::Marker, 5U, 51U, {1.0F, 0.08F, 2.0F}, 0.0F},

    {"wall_run_corridor", "left_wall_run_surface", "stone_wall_panel",
     MovementLabObjectKind::WallRunSurface, 8U, 26U, {24.0F, 2.5F, 0.45F}, 0.0F},
    {"wall_run_corridor", "right_wall_run_surface", "stone_wall_panel",
     MovementLabObjectKind::WallRunSurface, 10U, 26U, {24.0F, 2.5F, 0.45F}, 0.0F},
    {"wall_run_corridor", "runup_marker", "stone_floor_slab",
     MovementLabObjectKind::Marker, 9U, 10U, {0.35F, 0.08F, 1.40F}, 0.0F},
    {"wall_run_corridor", "exit_target_marker", "stone_floor_slab",
     MovementLabObjectKind::Marker, 9U, 42U, {0.35F, 0.08F, 1.40F}, 0.0F},

    {"collision_slide_lane", "straight_wall", "stone_wall_panel",
     MovementLabObjectKind::CollisionSurface, 12U, 18U, {11.0F, 2.5F, 0.50F}, 0.0F},
    {"collision_slide_lane", "corner_wall_x", "stone_wall_panel",
     MovementLabObjectKind::CollisionSurface, 12U, 38U, {7.0F, 2.5F, 0.50F}, 0.0F},
    {"collision_slide_lane", "corner_wall_z", "stone_wall_panel",
     MovementLabObjectKind::CollisionSurface, 13U, 41U, {0.50F, 2.5F, 5.0F}, 0.0F},
    {"collision_slide_lane", "snag_crate_a", "wood_crate_proxy",
     MovementLabObjectKind::SnagProp, 13U, 26U, {0.8F, 0.8F, 0.8F}, 0.0F},
    {"collision_slide_lane", "snag_crate_b", "wood_crate_proxy",
     MovementLabObjectKind::SnagProp, 13U, 29U, {0.8F, 0.8F, 0.8F}, 0.0F},

    {"ledge_mantle_lane", "low_ledge", "movement_clamber_ledge_proxy",
     MovementLabObjectKind::Ledge, 15U, 12U, {2.0F, 1.00F, 1.0F}, 0.0F},
    {"ledge_mantle_lane", "mid_ledge", "movement_clamber_ledge_proxy",
     MovementLabObjectKind::Ledge, 15U, 22U, {2.0F, 1.20F, 1.0F}, 0.0F},
    {"ledge_mantle_lane", "high_ledge", "movement_clamber_ledge_proxy",
     MovementLabObjectKind::Ledge, 15U, 32U, {2.0F, 1.40F, 1.0F}, 0.0F},
    {"ledge_mantle_lane", "snap_volume_placeholder", "stone_floor_slab",
     MovementLabObjectKind::Marker, 15U, 42U, {1.2F, 0.08F, 1.2F}, 0.0F},
}};

std::string objectIdFor(const MovementLabObjectSpec& spec) {
  return "movement_lab_" + std::string(spec.laneId) + "_" +
         std::string(spec.objectId);
}

Vec3 objectCenter(const MovementLabObjectSpec& spec,
                  const AsciiRoomGrid& grid,
                  const ProductMovementTestLabBuildConfig& config) {
  const float tileSize = config.tileSizeMeters;
  const Vec3 uncentered{static_cast<float>(spec.column) * tileSize,
                        spec.sizeMeters.y * 0.5F,
                        static_cast<float>(spec.row) * tileSize};
  const AsciiRoomWorldPosition centeredCell =
      asciiRoomCellCenter(spec.row, spec.column, grid.width, grid.height, tileSize, 0.0);
  const Vec3 centered{static_cast<float>(centeredCell.x),
                      spec.sizeMeters.y * 0.5F,
                      static_cast<float>(centeredCell.z)};
  const std::array<Vec3, 2U> centers{uncentered, centered};
  return centers[static_cast<std::size_t>(config.centerOnOrigin)];
}

const MovementLabObjectKindDescriptor& descriptorFor(MovementLabObjectKind kind) {
  return kObjectKindDescriptors[static_cast<std::size_t>(kind)];
}

std::vector<std::string> traversalTagsFor(const MovementLabObjectSpec& spec) {
  const MovementLabObjectKindDescriptor& descriptor = descriptorFor(spec.kind);
  std::vector<std::string> tags{"object", "prop", "movement_lab",
                                std::string(spec.laneId)};
  for (std::size_t index = 0U; index < descriptor.tagCount; ++index) {
    tags.emplace_back(descriptor.tags[index]);
  }
  return tags;
}

SaveAuthoredRoomObjectRecord authoredObjectFrom(
    const MovementLabObjectSpec& spec,
    const AsciiRoomGrid& grid,
    const ProductMovementTestLabBuildConfig& config) {
  SaveAuthoredRoomObjectRecord object;
  object.id = objectIdFor(spec);
  object.assetId = std::string(spec.assetId);
  object.storyIndex = config.storyIndex;
  object.positionMeters = objectCenter(spec, grid, config);
  object.sizeMeters = spec.sizeMeters;
  object.yawDegrees = spec.yawDegrees;
  object.semantics.materialId = object.assetId;
  object.semantics.traversalTags = traversalTagsFor(spec);
  object.semantics.gameplayTags = object.semantics.traversalTags;
  object.semantics.walkable = false;
  object.semantics.blocksActor = descriptorFor(spec.kind).blocksMovement;
  object.semantics.blocksProjectile = descriptorFor(spec.kind).blocksMovement;
  object.locked = false;
  object.hidden = false;
  return object;
}

}  // namespace

bool productMovementTestLabRoomId(std::string_view roomId) {
  return roomId == "movement_test_lab_v0_1";
}

std::vector<SaveAuthoredRoomObjectRecord> buildProductMovementTestLabObjects(
    const AsciiRoomGrid& grid,
    const ProductMovementTestLabBuildConfig& config) {
  std::vector<SaveAuthoredRoomObjectRecord> objects;
  const bool usableGrid =
      config.enabled && grid.width > 0U && grid.height > 0U && !grid.cells.empty();
  const std::size_t objectCount =
      kMovementLabObjects.size() * static_cast<std::size_t>(usableGrid);
  objects.reserve(objectCount);
  for (std::size_t index = 0U; index < objectCount; ++index) {
    objects.push_back(authoredObjectFrom(kMovementLabObjects[index], grid, config));
  }
  return objects;
}

}  // namespace iggy3d

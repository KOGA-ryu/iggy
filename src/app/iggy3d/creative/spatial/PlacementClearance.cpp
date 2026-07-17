#include "app/iggy3d/creative/spatial/PlacementClearance.hpp"

namespace iggy3d::creative {

std::string_view toString(CreativePlacementClearanceStatus status) noexcept {
  switch (status) {
    case CreativePlacementClearanceStatus::NotEvaluated:
      return "creative_placement_clearance_not_evaluated";
    case CreativePlacementClearanceStatus::InvalidRequest:
      return "creative_placement_clearance_invalid";
    case CreativePlacementClearanceStatus::TraversalLimitExceeded:
      return "creative_placement_clearance_limit_exceeded";
    case CreativePlacementClearanceStatus::OutsideWorldBounds:
      return "creative_placement_outside_world_bounds";
    case CreativePlacementClearanceStatus::AuthoredObjectBlocked:
      return "creative_placement_authored_object_blocked";
    case CreativePlacementClearanceStatus::VoxelBlocked:
      return "creative_placement_voxel_blocked";
    case CreativePlacementClearanceStatus::TerrainBlocked:
      return "creative_placement_terrain_blocked";
    case CreativePlacementClearanceStatus::Ready:
      return "creative_placement_clearance_ready";
  }
  return "creative_placement_clearance_status_invalid";
}

}  // namespace iggy3d::creative

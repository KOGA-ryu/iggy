#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCompileInternal.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d::creative {
using world_layout_compile::validStableKey;

std::string_view toString(CreativeWorldLayoutTable table) noexcept {
  switch (table) {
    case CreativeWorldLayoutTable::None: return "None";
    case CreativeWorldLayoutTable::Building: return "Building";
    case CreativeWorldLayoutTable::Level: return "Level";
    case CreativeWorldLayoutTable::Room: return "Room";
    case CreativeWorldLayoutTable::VerticalConnector:
      return "VerticalConnector";
    case CreativeWorldLayoutTable::Box:
      return "Box";
    case CreativeWorldLayoutTable::Wall: return "Wall";
    case CreativeWorldLayoutTable::Opening: return "Opening";
    case CreativeWorldLayoutTable::Object: return "Object";
    case CreativeWorldLayoutTable::TerrainProfile: return "TerrainProfile";
    case CreativeWorldLayoutTable::TerrainPath: return "TerrainPath";
    case CreativeWorldLayoutTable::TerrainPathPoint: return "TerrainPathPoint";
  }
  return "Unknown";
}

std::string_view toString(CreativeWorldLayoutStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutStatus::NotRequested: return "NotRequested";
    case CreativeWorldLayoutStatus::InvalidDocument: return "InvalidDocument";
    case CreativeWorldLayoutStatus::InvalidSchema: return "InvalidSchema";
    case CreativeWorldLayoutStatus::Empty: return "Empty";
    case CreativeWorldLayoutStatus::DuplicateStableKey:
      return "DuplicateStableKey";
    case CreativeWorldLayoutStatus::InvalidSymbol: return "InvalidSymbol";
    case CreativeWorldLayoutStatus::KernelRejected: return "KernelRejected";
    case CreativeWorldLayoutStatus::CapacityExceeded: return "CapacityExceeded";
    case CreativeWorldLayoutStatus::NoChange: return "NoChange";
    case CreativeWorldLayoutStatus::Ready: return "Ready";
    case CreativeWorldLayoutStatus::StalePlan: return "StalePlan";
    case CreativeWorldLayoutStatus::MutationRejected:
      return "MutationRejected";
    case CreativeWorldLayoutStatus::ObjectRejected: return "ObjectRejected";
    case CreativeWorldLayoutStatus::InstallRejected: return "InstallRejected";
    case CreativeWorldLayoutStatus::Applied: return "Applied";
    case CreativeWorldLayoutStatus::RefinementConflict:
      return "RefinementConflict";
  }
  return "Unknown";
}

std::string_view toString(CreativeWorldLayoutRoomEdge edge) noexcept {
  switch (edge) {
    case CreativeWorldLayoutRoomEdge::North: return "North";
    case CreativeWorldLayoutRoomEdge::East: return "East";
    case CreativeWorldLayoutRoomEdge::South: return "South";
    case CreativeWorldLayoutRoomEdge::West: return "West";
    case CreativeWorldLayoutRoomEdge::Count: break;
  }
  return "Unknown";
}

std::string_view creativeWorldLayoutRoomEdgeKey(
    CreativeWorldLayoutRoomEdge edge) noexcept {
  switch (edge) {
    case CreativeWorldLayoutRoomEdge::North: return "north";
    case CreativeWorldLayoutRoomEdge::East: return "east";
    case CreativeWorldLayoutRoomEdge::South: return "south";
    case CreativeWorldLayoutRoomEdge::West: return "west";
    case CreativeWorldLayoutRoomEdge::Count: break;
  }
  return "unknown";
}

std::string creativeWorldLayoutTag(std::string_view layoutKey) {
  return "creative_world_layout:" + std::string(layoutKey);
}

bool validCreativeWorldLayoutStableKey(std::string_view key) noexcept {
  return validStableKey(key);
}

bool creativeWorldLayoutStableKeyExists(const CreativeWorldLayout& layout,
                                        std::string_view key) noexcept {
  const auto matches = [&](const auto& value) {
    return value.stableKey == key;
  };
  return std::any_of(layout.buildings.begin(), layout.buildings.end(),
                     matches) ||
         std::any_of(layout.levels.begin(), layout.levels.end(), matches) ||
         std::any_of(layout.rooms.begin(), layout.rooms.end(), matches) ||
         std::any_of(layout.verticalConnectors.begin(),
                     layout.verticalConnectors.end(), matches) ||
         std::any_of(layout.boxes.begin(), layout.boxes.end(), matches) ||
         std::any_of(layout.walls.begin(), layout.walls.end(), matches) ||
         std::any_of(layout.openings.begin(), layout.openings.end(), matches) ||
         std::any_of(layout.objects.begin(), layout.objects.end(), matches) ||
         std::any_of(layout.terrainProfiles.begin(),
                     layout.terrainProfiles.end(), matches) ||
         std::any_of(layout.terrainPaths.begin(), layout.terrainPaths.end(),
                     matches);
}

std::string mintCreativeWorldLayoutStableKey(
    const CreativeWorldLayout& layout,
    std::uint64_t& nextOrdinal,
    std::string_view prefix) {
  for (;;) {
    const std::string candidate =
        std::string(prefix) + "_" + std::to_string(nextOrdinal++);
    if (!creativeWorldLayoutStableKeyExists(layout, candidate)) {
      return candidate;
    }
  }
}


}  // namespace iggy3d::creative

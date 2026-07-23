#include "app/iggy3d/creative/world/WorldLayoutBuildingTraversal.hpp"

#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"
#include "config/RuntimeConfig.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;

[[nodiscard]] bool validConfig(
    const CreativeWorldLayoutBuildingTraversalConfig& config) noexcept {
  return std::isfinite(config.actorRadiusMeters) &&
         config.actorRadiusMeters > 0.0 &&
         std::isfinite(config.actorHeightMeters) &&
         config.actorHeightMeters > config.actorRadiusMeters * 2.0 &&
         std::isfinite(config.maximumStepMeters) &&
         config.maximumStepMeters >= 0.0 &&
         std::isfinite(config.groundSnapMeters) &&
         config.groundSnapMeters >= 0.0 && std::isfinite(config.skinMeters) &&
         config.skinMeters >= 0.0 &&
         std::isfinite(config.floorHeightToleranceMeters) &&
         config.floorHeightToleranceMeters >= 0.0 &&
         std::isfinite(config.openingApproachMarginMeters) &&
         config.openingApproachMarginMeters >= 0.0 &&
         std::isfinite(config.pathEndpointToleranceMeters) &&
         config.pathEndpointToleranceMeters > 0.0 &&
         std::isfinite(config.rampMoveStepMeters) &&
         config.rampMoveStepMeters > 0.0 &&
         config.maximumMotorStepsPerPath > 0U;
}

void appendIssue(CreativeWorldLayoutBuildingTraversalReceipt& receipt,
                 CreativeWorldLayoutBuildingTraversalIssueKind kind,
                 CreativeWorldLayoutTable table, std::size_t index,
                 std::size_t buildingIndex) noexcept {
  if (receipt.issueCount < receipt.issues.size()) {
    receipt.issues[receipt.issueCount++] = {kind, table, index, buildingIndex};
    return;
  }
  receipt.capacityExceeded = true;
  ++receipt.droppedIssueCount;
}

[[nodiscard]] bool structuralTable(
    CreativeWorldLayoutTable table) noexcept {
  switch (table) {
    case CreativeWorldLayoutTable::Building:
    case CreativeWorldLayoutTable::Level:
    case CreativeWorldLayoutTable::Room:
    case CreativeWorldLayoutTable::VerticalConnector:
    case CreativeWorldLayoutTable::Box:
    case CreativeWorldLayoutTable::Wall:
    case CreativeWorldLayoutTable::Opening:
    case CreativeWorldLayoutTable::TopologyEdge:
    case CreativeWorldLayoutTable::RoofAperture:
      return true;
    case CreativeWorldLayoutTable::None:
    case CreativeWorldLayoutTable::Object:
    case CreativeWorldLayoutTable::TerrainProfile:
    case CreativeWorldLayoutTable::TerrainPath:
    case CreativeWorldLayoutTable::TerrainPathPoint:
      return false;
  }
  return false;
}

[[nodiscard]] RoomAsset structuralRoomWithoutDoorLeaves(
    const CreativeWorldLayout& layout,
    const CreativeDocument& document,
    const CreativeRoomBakeResult& baked) {
  std::unordered_map<std::string, CreativeObjectId> objectBySurface;
  objectBySurface.reserve(baked.spatialSurfaceSources.size());
  for (const CreativeRoomBakeSpatialSurfaceSource& source :
       baked.spatialSurfaceSources) {
    objectBySurface.emplace(source.surfaceId, source.objectId);
  }

  RoomAsset room = baked.room;
  std::erase_if(room.spatialSurfaces, [&](const RoomSpatialSurface& surface) {
    const auto source = objectBySurface.find(surface.id);
    if (source == objectBySurface.end()) {
      return true;
    }
    const CreativeObject* object = document.findObject(source->second);
    if (object == nullptr || object->kind == CreativeObjectKind::Door) {
      return true;
    }
    const CreativeWorldLayoutObjectProvenance provenance =
        resolveCreativeWorldLayoutObjectProvenance(layout, *object);
    if (provenance.owned &&
        provenance.table == CreativeWorldLayoutTable::Opening &&
        provenance.index < layout.openings.size() &&
        layout.openings[provenance.index].kind ==
            CreativeBuildingOpeningKind::Door) {
      return true;
    }
    return !provenance.owned || !structuralTable(provenance.table);
  });
  return room;
}

[[nodiscard]] Vec3 toCore(CreativeVec3 value) noexcept {
  return {static_cast<float>(value.x), static_cast<float>(value.y),
          static_cast<float>(value.z)};
}

[[nodiscard]] double worldCoordinate(double origin, double cellSize,
                                     double cells) noexcept {
  return origin + cellSize * cells;
}

[[nodiscard]] bool bodyClearAt(const SpatialSurfaceSet& surfaces,
                               Vec3 footMeters,
                               const CreativeWorldLayoutBuildingTraversalConfig&
                                   config) noexcept {
  const float radius = static_cast<float>(config.actorRadiusMeters);
  const float skin = static_cast<float>(config.skinMeters);
  const float height = static_cast<float>(config.actorHeightMeters);
  const Aabb3 body = makeAabb3(
      {footMeters.x - radius, footMeters.y + skin, footMeters.z - radius},
      {footMeters.x + radius, footMeters.y + height, footMeters.z + radius});
  const CollisionQueryResult overlap =
      queryAabbOverlap(surfaces, body, CollisionQueryKind::Actor);
  return overlap.status == CollisionQueryStatus::NoHit;
}

[[nodiscard]] std::array<CreativeWorldLayoutOpeningHostPoint, 5U>
roomRectSamples(CreativeWorldLayoutRect rect, double insetCells) noexcept {
  const double minimumX = static_cast<double>(rect.minimum.x);
  const double maximumX = static_cast<double>(rect.maximum.x);
  const double minimumZ = static_cast<double>(rect.minimum.z);
  const double maximumZ = static_cast<double>(rect.maximum.z);
  const double centerX = (minimumX + maximumX) * 0.5;
  const double centerZ = (minimumZ + maximumZ) * 0.5;
  const double insetMinimumX = std::min(centerX, minimumX + insetCells);
  const double insetMaximumX = std::max(centerX, maximumX - insetCells);
  const double insetMinimumZ = std::min(centerZ, minimumZ + insetCells);
  const double insetMaximumZ = std::max(centerZ, maximumZ - insetCells);
  return {{{centerX, centerZ},
           {insetMinimumX, insetMinimumZ},
           {insetMaximumX, insetMinimumZ},
           {insetMaximumX, insetMaximumZ},
           {insetMinimumX, insetMaximumZ}}};
}

struct RoomOccupancyResult {
  bool floorContact = false;
  bool standingClearance = false;
};

[[nodiscard]] RoomOccupancyResult validateRoomOccupancy(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex,
    const SpatialSurfaceSet& surfaces,
    const CreativeWorldLayoutBuildingTraversalConfig& config) noexcept {
  RoomOccupancyResult result;
  if (roomIndex >= layout.rooms.size() ||
      layout.rooms[roomIndex].levelIndex >= layout.levels.size()) {
    return result;
  }
  const CreativeWorldLayoutLevelDimensions dimensions =
      measureCreativeWorldLayoutLevelDimensions(
          grid, layout, layout.rooms[roomIndex].levelIndex);
  if (!dimensions.accepted) {
    return result;
  }
  double halfWallThicknessCells =
      layout.rooms[roomIndex].wallThicknessCells * 0.5;
  for (const CreativeWorldLayoutRoomBoundary& boundary :
       creativeWorldLayoutRoomBoundaries(graph, roomIndex)) {
    if (boundary.topologyEdgeIndex < graph.edges.size()) {
      halfWallThicknessCells = std::max(
          halfWallThicknessCells,
          graph.edges[boundary.topologyEdgeIndex].wallThicknessCells * 0.5);
    }
  }
  const double insetCells = halfWallThicknessCells +
      (config.actorRadiusMeters + config.skinMeters) / grid.cellSizeMeters;
  for (const CreativeWorldLayoutRect rect :
       creativeWorldLayoutRoomSurfaceRects(graph, roomIndex)) {
    for (const CreativeWorldLayoutOpeningHostPoint sample :
         roomRectSamples(rect, insetCells)) {
      const Vec3 point{
          static_cast<float>(worldCoordinate(grid.origin.x,
                                             grid.cellSizeMeters, sample.x)),
          static_cast<float>(dimensions.floorTopMeters),
          static_cast<float>(worldCoordinate(grid.origin.z,
                                             grid.cellSizeMeters, sample.z))};
      const CollisionQueryResult ground = sampleSurfaceHeightAtOrBelow(
          surfaces, point,
          static_cast<float>(dimensions.floorTopMeters +
                             std::max(config.floorHeightToleranceMeters,
                                      config.maximumStepMeters)),
          static_cast<float>(config.actorRadiusMeters));
      // Stacked generated slabs can expose a collision top above the authored
      // floor plane. Accept only heights the runtime player can actually step
      // onto, then perform clearance from that physical contact height.
      if (ground.status != CollisionQueryStatus::Hit ||
          std::fabs(static_cast<double>(ground.heightMeters) -
                    dimensions.floorTopMeters) >
              std::max(config.floorHeightToleranceMeters,
                       config.maximumStepMeters)) {
        continue;
      }
      result.floorContact = true;
      if (bodyClearAt(surfaces,
                      {point.x, ground.heightMeters, point.z}, config)) {
        result.standingClearance = true;
        return result;
      }
    }
  }
  return result;
}

[[nodiscard]] PlayerPhysicsMovePlannerConfig plannerConfig(
    const CreativeWorldLayoutBuildingTraversalConfig& config,
    double maximumMoveMeters) noexcept {
  PlayerPhysicsMovePlannerConfig output;
  output.motor.skinMeters = static_cast<float>(config.skinMeters);
  output.motor.groundProbeDistanceMeters =
      static_cast<float>(config.groundSnapMeters);
  output.motor.groundSnapDistanceMeters =
      static_cast<float>(config.groundSnapMeters);
  output.motor.maxMoveDistanceMeters = std::max(
      output.motor.maxMoveDistanceMeters,
      static_cast<float>(maximumMoveMeters + config.skinMeters + 0.001));
  output.maxStepHeightMeters = static_cast<float>(config.maximumStepMeters);
  return output;
}

[[nodiscard]] bool plannerReaches(
    const SpatialSurfaceSet& surfaces,
    const PhysicsSpatialSurfaceColliderBakeResult& surfaceBake,
    Vec3 startCenter, Vec3 endCenter,
    const CreativeWorldLayoutBuildingTraversalConfig& config) {
  PlayerPhysicsMovePlannerRequest request;
  request.collisionSurfaces = &surfaces;
  request.precomputedSurfaceBake = &surfaceBake;
  request.startCenterMeters = startCenter;
  request.bodyHalfExtentsMeters = {
      static_cast<float>(config.actorRadiusMeters),
      static_cast<float>(config.actorHeightMeters * 0.5),
      static_cast<float>(config.actorRadiusMeters)};
  request.desiredDisplacementMeters = endCenter - startCenter;
  request.config = plannerConfig(
      config, std::sqrt(static_cast<double>(
                  distanceSquared(startCenter, endCenter))));
  const PlayerPhysicsMovePlannerResult planned =
      planPlayerPhysicsMove(request);
  const Vec3 horizontalError{planned.finalCenterMeters.x - endCenter.x, 0.0F,
                             planned.finalCenterMeters.z - endCenter.z};
  // Exterior openings are valid without authored terrain on both sides. The
  // doorway proof therefore owns the horizontal body sweep; room occupancy
  // and connector proofs own grounded vertical checks.
  return planned.ok &&
         std::sqrt(static_cast<double>(lengthSquared(horizontalError))) <=
             config.pathEndpointToleranceMeters;
}

[[nodiscard]] bool validateOpeningPassage(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& layout,
    std::size_t openingIndex,
    const SpatialSurfaceSet& surfaces,
    const PhysicsSpatialSurfaceColliderBakeResult& surfaceBake,
    const CreativeWorldLayoutBuildingTraversalConfig& config) {
  if (openingIndex >= layout.openings.size()) {
    return false;
  }
  const CreativeWorldLayoutOpening& opening = layout.openings[openingIndex];
  const CreativeWorldLayoutOpeningHostFrame host =
      resolveCreativeWorldLayoutOpeningHost(layout, opening);
  if (!host.accepted || host.lengthCells <= 0.0 ||
      !std::isfinite(host.wallThicknessCells)) {
    return false;
  }
  const CreativeWorldLayoutOpeningHostPoint center =
      creativeWorldLayoutOpeningHostPoint(host, opening.centerOffsetCells);
  const double deltaX = static_cast<double>(host.end.x - host.start.x);
  const double deltaZ = static_cast<double>(host.end.z - host.start.z);
  const double length = std::hypot(deltaX, deltaZ);
  if (!std::isfinite(length) || length <= kGeometryEpsilon) {
    return false;
  }
  const double normalX = -deltaZ / length;
  const double normalZ = deltaX / length;
  const double offsetMeters =
      host.wallThicknessCells * grid.cellSizeMeters * 0.5 +
      config.actorRadiusMeters + config.skinMeters +
      config.openingApproachMarginMeters;
  const double centerX =
      worldCoordinate(grid.origin.x, grid.cellSizeMeters, center.x);
  const double centerZ =
      worldCoordinate(grid.origin.z, grid.cellSizeMeters, center.z);
  const double floorY =
      worldCoordinate(grid.origin.y, grid.cellSizeMeters, host.baseLayer);
  const float centerY =
      static_cast<float>(floorY + config.actorHeightMeters * 0.5);
  const Vec3 first{
      static_cast<float>(centerX - normalX * offsetMeters), centerY,
      static_cast<float>(centerZ - normalZ * offsetMeters)};
  const Vec3 second{
      static_cast<float>(centerX + normalX * offsetMeters), centerY,
      static_cast<float>(centerZ + normalZ * offsetMeters)};
  return plannerReaches(surfaces, surfaceBake, first, second, config) &&
         plannerReaches(surfaces, surfaceBake, second, first, config);
}

enum class MovementPathStatus : std::uint8_t {
  Reached,
  Blocked,
  CapacityExceeded,
};

[[nodiscard]] MovementPathStatus traverseMovementPath(
    const SpatialSurfaceSet& surfaces,
    const PhysicsSpatialSurfaceColliderBakeResult& surfaceBake,
    CreativeVec3 start, CreativeVec3 end, double moveStepMeters,
    const CreativeWorldLayoutBuildingTraversalConfig& config) {
  const double distance = std::hypot(end.x - start.x, end.z - start.z);
  if (!std::isfinite(distance) || distance <= kGeometryEpsilon ||
      !std::isfinite(moveStepMeters) || moveStepMeters <= 0.0) {
    return MovementPathStatus::Blocked;
  }
  const double moveCountExact = std::ceil(distance / moveStepMeters);
  if (!std::isfinite(moveCountExact) || moveCountExact < 1.0 ||
      moveCountExact >
          static_cast<double>(config.maximumMotorStepsPerPath) ||
      moveCountExact >=
          static_cast<double>(std::numeric_limits<std::size_t>::max())) {
    return MovementPathStatus::CapacityExceeded;
  }
  const std::size_t moveCount = static_cast<std::size_t>(moveCountExact);

  WorldState world;
  EntityState player;
  player.id = {1U};
  player.stableName = "building_traversal_player";
  player.kind = EntityKind::Player;
  player.transform = identityTransform3();
  player.transform.position = toCore(start);
  player.localBounds = makeAabb3(
      {-static_cast<float>(config.actorRadiusMeters), 0.0F,
       -static_cast<float>(config.actorRadiusMeters)},
      {static_cast<float>(config.actorRadiusMeters),
       static_cast<float>(config.actorHeightMeters),
       static_cast<float>(config.actorRadiusMeters)});
  if (world.seedEntity(player).status != WorldStatus::Ok) {
    return MovementPathStatus::Blocked;
  }
  RuntimeConfig runtimeConfig = makeDefaultRuntimeConfig();
  MovementSystemContext context{&world, &runtimeConfig, &surfaces, true,
                                &surfaceBake};
  const double directionX = (end.x - start.x) / distance;
  const double directionZ = (end.z - start.z) / distance;
  for (std::size_t moveIndex = 0U; moveIndex < moveCount; ++moveIndex) {
    const EntityState* current = world.findById({1U});
    if (current == nullptr) {
      return MovementPathStatus::Blocked;
    }
    const double remaining =
        std::hypot(end.x - current->transform.position.x,
                   end.z - current->transform.position.z);
    const double step = std::min(moveStepMeters, remaining);
    MovementRequest request;
    request.actor = {1U};
    request.destination = {
        current->transform.position.x + static_cast<float>(directionX * step),
        current->transform.position.y,
        current->transform.position.z + static_cast<float>(directionZ * step)};
    request.mode = MovementMode::Walk;
    request.maxDistanceMeters =
        static_cast<float>(std::max(1.0, moveStepMeters + 0.01));
    const MovementResult moved = executeMovement(context, request);
    if (moved.blocked != MovementBlockedReason::None) {
      return MovementPathStatus::Blocked;
    }
  }
  const EntityState* finalPlayer = world.findById({1U});
  if (finalPlayer == nullptr ||
      std::hypot(finalPlayer->transform.position.x - end.x,
                 finalPlayer->transform.position.z - end.z) >
          config.pathEndpointToleranceMeters) {
    return MovementPathStatus::Blocked;
  }
  const CollisionQueryResult endpointGround = sampleSurfaceHeightAtOrBelow(
      surfaces, finalPlayer->transform.position,
      static_cast<float>(end.y + config.maximumStepMeters +
                         config.floorHeightToleranceMeters),
      static_cast<float>(config.actorRadiusMeters));
  if (endpointGround.status != CollisionQueryStatus::Hit ||
      std::fabs(finalPlayer->transform.position.y -
                endpointGround.heightMeters) >
          config.pathEndpointToleranceMeters) {
    return MovementPathStatus::Blocked;
  }
  return MovementPathStatus::Reached;
}

[[nodiscard]] MovementPathStatus validateConnectorTraversal(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& layout,
    std::size_t connectorIndex,
    const SpatialSurfaceSet& surfaces,
    const PhysicsSpatialSurfaceColliderBakeResult& surfaceBake,
    const CreativeWorldLayoutBuildingTraversalConfig& config) {
  const CreativeWorldLayoutVerticalConnectorPlan connector =
      planCreativeWorldLayoutVerticalConnector(grid, layout, connectorIndex);
  if (!connector.accepted) {
    return MovementPathStatus::Blocked;
  }
  CreativeVec3 lower;
  CreativeVec3 upper;
  double moveStepMeters = config.rampMoveStepMeters;
  if (connector.objectKind == CreativeObjectKind::Stair) {
    lower = connector.stair.lowerLanding.centerMeters;
    upper = connector.stair.upperLanding.centerMeters;
    moveStepMeters = connector.stair.treadDepthMeters;
  } else if (connector.objectKind == CreativeObjectKind::Ramp) {
    lower = connector.ramp.lowerLanding.centerMeters;
    upper = connector.ramp.upperLanding.centerMeters;
  } else {
    return MovementPathStatus::Blocked;
  }
  const MovementPathStatus ascending = traverseMovementPath(
      surfaces, surfaceBake, lower, upper, moveStepMeters, config);
  if (ascending != MovementPathStatus::Reached) {
    return ascending;
  }
  return traverseMovementPath(surfaces, surfaceBake, upper, lower,
                              moveStepMeters, config);
}

[[nodiscard]] std::size_t openingBuildingIndex(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutOpening& opening) noexcept {
  const CreativeWorldLayoutOpeningHostFrame host =
      resolveCreativeWorldLayoutOpeningHost(layout, opening);
  return host.accepted ? host.buildingIndex : kInvalidCreativeWorldLayoutIndex;
}

}  // namespace

std::string_view creativeWorldLayoutBuildingTraversalReasonCode(
    CreativeWorldLayoutBuildingTraversalIssueKind kind) noexcept {
  switch (kind) {
    case CreativeWorldLayoutBuildingTraversalIssueKind::
        MissingRoomFloorContact:
      return "creative_world_layout_building_room_floor_contact_missing";
    case CreativeWorldLayoutBuildingTraversalIssueKind::
        RoomStandingClearanceBlocked:
      return "creative_world_layout_building_room_standing_clearance_blocked";
    case CreativeWorldLayoutBuildingTraversalIssueKind::
        OpeningPassageBlocked:
      return "creative_world_layout_building_opening_passage_blocked";
    case CreativeWorldLayoutBuildingTraversalIssueKind::
        ConnectorTraversalBlocked:
      return "creative_world_layout_building_connector_traversal_blocked";
    case CreativeWorldLayoutBuildingTraversalIssueKind::
        TraversalCapacityExceeded:
      return "creative_world_layout_building_traversal_capacity_exceeded";
    case CreativeWorldLayoutBuildingTraversalIssueKind::Count:
      break;
  }
  return "creative_world_layout_building_traversal_issue_invalid";
}

CreativeWorldLayoutBuildingTraversalReceipt
validateCreativeWorldLayoutBuildingTraversal(
    const CreativeWorldLayoutBuildingTraversalRequest& request) {
  CreativeWorldLayoutBuildingTraversalReceipt receipt;
  receipt.requested = true;
  if (request.layout == nullptr || request.generatedDocument == nullptr ||
      !request.generatedDocument->isValid() || !validConfig(request.config)) {
    receipt.status = CreativeWorldLayoutBuildingTraversalStatus::InvalidRequest;
    receipt.reasonCode =
        "creative_world_layout_building_traversal_request_invalid";
    return receipt;
  }

  const CreativeWorldLayout& layout = *request.layout;
  const CreativeGridSettings grid = request.generatedDocument->gridSettings();
  if (!std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    receipt.status = CreativeWorldLayoutBuildingTraversalStatus::InvalidRequest;
    receipt.reasonCode =
        "creative_world_layout_building_traversal_grid_invalid";
    return receipt;
  }
  const CreativeWorldLayoutRoomGraph roomGraph =
      buildCreativeWorldLayoutRoomGraph(layout);
  if (!roomGraph.accepted) {
    receipt.status = CreativeWorldLayoutBuildingTraversalStatus::InvalidRequest;
    receipt.reasonCode =
        "creative_world_layout_building_traversal_room_graph_invalid";
    return receipt;
  }

  CreativeRoomBakeRequest roomBakeRequest;
  roomBakeRequest.document = request.generatedDocument;
  roomBakeRequest.roomId = "creative_world_layout_building_traversal";
  roomBakeRequest.sourceName = "Creative World Layout Building Traversal";
  roomBakeRequest.sourceSubset = "generated_building_structure";
  roomBakeRequest.validateReachability = false;
  const CreativeRoomBakeResult roomBake =
      buildRoomAssetFromCreativeDocument(roomBakeRequest);
  if (!roomBake.receipt.accepted) {
    receipt.status = CreativeWorldLayoutBuildingTraversalStatus::RoomBakeFailed;
    receipt.reasonCode = "creative_world_layout_building_traversal_room_bake_failed";
    return receipt;
  }
  const RoomAsset structuralRoom = structuralRoomWithoutDoorLeaves(
      layout, *request.generatedDocument, roomBake);
  const SpatialSurfaceSet surfaces = buildSpatialSurfaceSet(structuralRoom);
  const PhysicsSpatialSurfaceColliderBakeResult surfaceBake =
      bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});
  receipt.surfaceBakeStatus = surfaceBake.status;
  if (!surfaceBake.ok) {
    receipt.status =
        CreativeWorldLayoutBuildingTraversalStatus::SurfaceBakeFailed;
    receipt.reasonCode =
        "creative_world_layout_building_traversal_surface_bake_failed";
    return receipt;
  }

  receipt.accepted = true;
  receipt.roomCount = layout.rooms.size();
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    const CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
    const RoomOccupancyResult occupancy = validateRoomOccupancy(
        grid, layout, roomGraph, roomIndex, surfaces, request.config);
    receipt.roomFloorContactCount += occupancy.floorContact ? 1U : 0U;
    receipt.roomStandingClearanceCount +=
        occupancy.standingClearance ? 1U : 0U;
    if (!occupancy.floorContact) {
      appendIssue(
          receipt,
          CreativeWorldLayoutBuildingTraversalIssueKind::
              MissingRoomFloorContact,
          CreativeWorldLayoutTable::Room, roomIndex, room.buildingIndex);
    } else if (!occupancy.standingClearance) {
      appendIssue(
          receipt,
          CreativeWorldLayoutBuildingTraversalIssueKind::
              RoomStandingClearanceBlocked,
          CreativeWorldLayoutTable::Room, roomIndex, room.buildingIndex);
    }
  }

  for (std::size_t openingIndex = 0U;
       openingIndex < layout.openings.size(); ++openingIndex) {
    const CreativeWorldLayoutOpening& opening = layout.openings[openingIndex];
    if (opening.kind != CreativeBuildingOpeningKind::Door) {
      continue;
    }
    ++receipt.passageCount;
    const bool traversable = validateOpeningPassage(
        grid, layout, openingIndex, surfaces, surfaceBake, request.config);
    receipt.traversablePassageCount += traversable ? 1U : 0U;
    if (!traversable) {
      appendIssue(
          receipt,
          CreativeWorldLayoutBuildingTraversalIssueKind::OpeningPassageBlocked,
          CreativeWorldLayoutTable::Opening, openingIndex,
          openingBuildingIndex(layout, opening));
    }
  }

  receipt.connectorCount = layout.verticalConnectors.size();
  for (std::size_t connectorIndex = 0U;
       connectorIndex < layout.verticalConnectors.size(); ++connectorIndex) {
    const MovementPathStatus traversal = validateConnectorTraversal(
        grid, layout, connectorIndex, surfaces, surfaceBake, request.config);
    if (traversal == MovementPathStatus::Reached) {
      ++receipt.traversableConnectorCount;
      continue;
    }
    const CreativeWorldLayoutVerticalConnector& connector =
        layout.verticalConnectors[connectorIndex];
    appendIssue(
        receipt,
        traversal == MovementPathStatus::CapacityExceeded
            ? CreativeWorldLayoutBuildingTraversalIssueKind::
                  TraversalCapacityExceeded
            : CreativeWorldLayoutBuildingTraversalIssueKind::
                  ConnectorTraversalBlocked,
        CreativeWorldLayoutTable::VerticalConnector, connectorIndex,
        connector.buildingIndex);
  }

  receipt.traversable = receipt.issueCount == 0U &&
                        receipt.droppedIssueCount == 0U;
  receipt.status = receipt.traversable
                       ? CreativeWorldLayoutBuildingTraversalStatus::Ready
                       : CreativeWorldLayoutBuildingTraversalStatus::IssuesFound;
  receipt.reasonCode =
      receipt.traversable
          ? "creative_world_layout_building_traversal_ready"
          : "creative_world_layout_building_traversal_issues_found";
  return receipt;
}

}  // namespace iggy3d::creative

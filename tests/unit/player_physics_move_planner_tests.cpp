#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs, float epsilon = 0.001F) {
  const float delta = lhs - rhs;
  return delta >= -epsilon && delta <= epsilon;
}

iggy3d::RoomSpatialSurface floorSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "floor";
  surface.sourceStaticMeshId = "floor_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-4.0F, 0.0F, -4.0F},
      {4.0F, 0.0F, -4.0F},
      {4.0F, 0.0F, 4.0F},
      {-4.0F, 0.0F, 4.0F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.collisionMask = {"actor"};
  surface.runtimeOwnerStableName = "owner.floor";
  return surface;
}

iggy3d::RoomSpatialSurface wallSurface(std::string_view id = "wall",
                                       float minX = 2.0F,
                                       float maxX = 3.0F) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::string(id);
  surface.sourceStaticMeshId = "wall_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {minX, 0.0F, -4.0F},
      {maxX, 0.0F, -4.0F},
      {maxX, 2.4F, 4.0F},
      {minX, 2.4F, 4.0F},
  };
  surface.normal = {-1.0F, 0.0F, 0.0F};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  surface.runtimeOwnerStableName = "owner.wall";
  return surface;
}

iggy3d::RoomSpatialSurface projectileOnlySurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "projectile_wall";
  surface.sourceStaticMeshId = "projectile_wall_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker;
  surface.pointsMeters = {
      {2.0F, 0.0F, -4.0F},
      {3.0F, 0.0F, -4.0F},
      {3.0F, 2.4F, 4.0F},
      {2.0F, 2.4F, 4.0F},
  };
  surface.normal = {-1.0F, 0.0F, 0.0F};
  surface.collisionMask = {"projectile"};
  surface.blocksProjectile = true;
  surface.runtimeOwnerStableName = "owner.projectile_wall";
  return surface;
}

iggy3d::SpatialSurfaceSet makeSurfaceSet(
    std::initializer_list<iggy3d::RoomSpatialSurface> surfaces) {
  iggy3d::RoomAsset room;
  room.id = "synthetic_room";
  room.spatialSurfaces.assign(surfaces.begin(), surfaces.end());
  return iggy3d::buildSpatialSurfaceSet(room);
}

std::vector<std::string> surfaceIds(const iggy3d::SpatialSurfaceSet& set) {
  std::vector<std::string> ids;
  for (const iggy3d::CollisionSurfaceView& surface : set.surfaces()) {
    ids.push_back(surface.id);
  }
  return ids;
}

iggy3d::PlayerPhysicsMovePlannerConfig plannerConfig() {
  iggy3d::PlayerPhysicsMovePlannerConfig config;
  config.motor.skinMeters = 0.10F;
  config.motor.groundProbeDistanceMeters = 0.15F;
  config.motor.groundSnapDistanceMeters = 0.0F;
  return config;
}

iggy3d::PlayerPhysicsMovePlannerRequest requestFor(
    const iggy3d::SpatialSurfaceSet* surfaces,
    iggy3d::Vec3 start,
    iggy3d::Vec3 desired,
    iggy3d::PlayerPhysicsMovePlannerConfig config = plannerConfig()) {
  iggy3d::PlayerPhysicsMovePlannerRequest request;
  request.collisionSurfaces = surfaces;
  request.startCenterMeters = start;
  request.desiredDisplacementMeters = desired;
  request.config = config;
  return request;
}

bool displacementFactsMatch(const iggy3d::PlayerPhysicsMovePlannerResult& result,
                            iggy3d::Vec3 desiredDisplacementMeters) {
  const iggy3d::Vec3 expectedApplied =
      result.finalCenterMeters - result.startCenterMeters;
  const iggy3d::Vec3 expectedRemaining =
      desiredDisplacementMeters - result.appliedDisplacementMeters;
  return iggy3d::nearlyEqual(result.appliedDisplacementMeters,
                             expectedApplied) &&
         iggy3d::nearlyEqual(result.remainingDisplacementMeters,
                             expectedRemaining);
}

bool statusNamesAndConfigValidation() {
  iggy3d::PlayerPhysicsMovePlannerConfig valid;
  iggy3d::PlayerPhysicsMovePlannerConfig invalidBake = valid;
  invalidBake.surfaceBake.firstGeneratedBodyId = {};
  iggy3d::PlayerPhysicsMovePlannerConfig invalidMotor = valid;
  invalidMotor.motor.maxIterations = 0U;

  return expect(iggy3d::playerPhysicsMovePlannerStatusName(
                    iggy3d::PlayerPhysicsMovePlannerStatus::Planned) ==
                    "player_physics_move_planner_planned",
                "planned status") &&
         expect(iggy3d::playerPhysicsMovePlannerStatusName(
                    iggy3d::PlayerPhysicsMovePlannerStatus::
                        MissingCollisionSurfaces) ==
                    "player_physics_move_planner_missing_collision_surfaces",
                "missing status") &&
         expect(iggy3d::playerPhysicsMovePlannerStatusName(
                    iggy3d::PlayerPhysicsMovePlannerStatus::InvalidStartCenter) ==
                    "player_physics_move_planner_invalid_start_center",
                "invalid start status") &&
         expect(iggy3d::playerPhysicsMovePlannerStatusName(
                    iggy3d::PlayerPhysicsMovePlannerStatus::
                        InvalidBodyHalfExtents) ==
                    "player_physics_move_planner_invalid_body_half_extents",
                "invalid half extents status") &&
         expect(iggy3d::playerPhysicsMovePlannerStatusName(
                    iggy3d::PlayerPhysicsMovePlannerStatus::InvalidBodyId) ==
                    "player_physics_move_planner_invalid_body_id",
                "invalid body id status") &&
         expect(iggy3d::playerPhysicsMovePlannerStatusName(
                    iggy3d::PlayerPhysicsMovePlannerStatus::
                        InvalidDesiredDisplacement) ==
                    "player_physics_move_planner_invalid_desired_displacement",
                "invalid displacement status") &&
         expect(iggy3d::playerPhysicsMovePlannerStatusName(
                    iggy3d::PlayerPhysicsMovePlannerStatus::
                        InvalidSurfaceBakeConfig) ==
                    "player_physics_move_planner_invalid_surface_bake_config",
                "invalid bake config status") &&
         expect(iggy3d::playerPhysicsMovePlannerStatusName(
                    iggy3d::PlayerPhysicsMovePlannerStatus::InvalidMotorConfig) ==
                    "player_physics_move_planner_invalid_motor_config",
                "invalid motor config status") &&
         expect(iggy3d::playerPhysicsMovePlannerStatusName(
                    iggy3d::PlayerPhysicsMovePlannerStatus::SurfaceBakeFailed) ==
                    "player_physics_move_planner_surface_bake_failed",
                "bake failed status") &&
         expect(iggy3d::playerPhysicsMovePlannerStatusName(
                    iggy3d::PlayerPhysicsMovePlannerStatus::MotorPlanFailed) ==
                    "player_physics_move_planner_motor_plan_failed",
                "motor failed status") &&
         expect(iggy3d::isValidPlayerPhysicsMovePlannerConfig(valid),
                "valid config") &&
         expect(!iggy3d::isValidPlayerPhysicsMovePlannerConfig(invalidBake),
                "invalid bake config") &&
         expect(!iggy3d::isValidPlayerPhysicsMovePlannerConfig(invalidMotor),
                "invalid motor config");
}

bool invalidRequestsRejectBeforeMovement() {
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({floorSurface()});
  iggy3d::PlayerPhysicsMovePlannerRequest missing =
      requestFor(nullptr, {0.0F, 0.9F, 0.0F}, {});
  iggy3d::PlayerPhysicsMovePlannerRequest badStart =
      requestFor(&surfaces, {0.0F, 0.9F, 0.0F}, {});
  badStart.startCenterMeters.x = std::numeric_limits<float>::infinity();
  iggy3d::PlayerPhysicsMovePlannerRequest badHalf =
      requestFor(&surfaces, {0.0F, 0.9F, 0.0F}, {});
  badHalf.bodyHalfExtentsMeters.x = 0.0F;
  iggy3d::PlayerPhysicsMovePlannerRequest badBody =
      requestFor(&surfaces, {0.0F, 0.9F, 0.0F}, {});
  badBody.physicsBodyId = {};
  iggy3d::PlayerPhysicsMovePlannerRequest badDisplacement =
      requestFor(&surfaces, {0.0F, 0.9F, 0.0F}, {});
  badDisplacement.desiredDisplacementMeters.z =
      std::numeric_limits<float>::infinity();
  iggy3d::PlayerPhysicsMovePlannerRequest badBake =
      requestFor(&surfaces, {0.0F, 0.9F, 0.0F}, {});
  badBake.config.surfaceBake.planeThicknessMeters = 0.0F;
  iggy3d::PlayerPhysicsMovePlannerRequest badMotor =
      requestFor(&surfaces, {0.0F, 0.9F, 0.0F}, {});
  badMotor.config.motor.maxIterations = 0U;

  const iggy3d::PlayerPhysicsMovePlannerResult missingResult =
      iggy3d::planPlayerPhysicsMove(missing);
  const iggy3d::PlayerPhysicsMovePlannerResult startResult =
      iggy3d::planPlayerPhysicsMove(badStart);
  const iggy3d::PlayerPhysicsMovePlannerResult halfResult =
      iggy3d::planPlayerPhysicsMove(badHalf);
  const iggy3d::PlayerPhysicsMovePlannerResult bodyResult =
      iggy3d::planPlayerPhysicsMove(badBody);
  const iggy3d::PlayerPhysicsMovePlannerResult displacementResult =
      iggy3d::planPlayerPhysicsMove(badDisplacement);
  const iggy3d::PlayerPhysicsMovePlannerResult bakeResult =
      iggy3d::planPlayerPhysicsMove(badBake);
  const iggy3d::PlayerPhysicsMovePlannerResult motorResult =
      iggy3d::planPlayerPhysicsMove(badMotor);

  return expect(!missingResult.ok, "missing surfaces rejected") &&
         expect(missingResult.reasonCode ==
                    "player_physics_move_planner_missing_collision_surfaces",
                "missing surfaces reason") &&
         expect(!startResult.ok, "bad start rejected") &&
         expect(startResult.reasonCode ==
                    "player_physics_move_planner_invalid_start_center",
                "bad start reason") &&
         expect(!halfResult.ok, "bad half extents rejected") &&
         expect(halfResult.reasonCode ==
                    "player_physics_move_planner_invalid_body_half_extents",
                "bad half reason") &&
         expect(!bodyResult.ok, "bad body id rejected") &&
         expect(bodyResult.reasonCode ==
                    "player_physics_move_planner_invalid_body_id",
                "bad body id reason") &&
         expect(!displacementResult.ok, "bad displacement rejected") &&
         expect(displacementResult.reasonCode ==
                    "player_physics_move_planner_invalid_desired_displacement",
                "bad displacement reason") &&
         expect(!bakeResult.ok, "bad bake config rejected") &&
         expect(bakeResult.reasonCode ==
                    "player_physics_move_planner_invalid_surface_bake_config",
                "bad bake config reason") &&
         expect(bakeResult.upstreamReasonCode ==
                    "physics_spatial_surface_bake_invalid_config",
                "bad bake upstream reason") &&
         expect(!motorResult.ok, "bad motor config rejected") &&
         expect(motorResult.reasonCode ==
                    "player_physics_move_planner_invalid_motor_config",
                "bad motor config reason") &&
         expect(motorResult.upstreamReasonCode ==
                    "physics_kinematic_motor_invalid_config",
                "bad motor upstream reason") &&
         expect(!missingResult.debugGeometryAvailable,
                "invalid request no debug geometry") &&
         expect(iggy3d::nearlyEqual(startResult.appliedDisplacementMeters, {}),
                "invalid request no movement");
}

bool emptySurfaceSetPlansClearMovement() {
  const iggy3d::SpatialSurfaceSet empty;
  const iggy3d::Vec3 desired = {1.0F, 0.0F, 0.5F};
  const iggy3d::PlayerPhysicsMovePlannerResult result =
      iggy3d::planPlayerPhysicsMove(
          requestFor(&empty, {0.0F, 0.9F, 0.0F}, desired));

  return expect(result.ok, "empty surface move ok") &&
         expect(result.bakedSurfaceCount == 0U, "empty surface count") &&
         expect(result.bakedColliderCount == 0U, "empty collider count") &&
         expect(result.debugGeometryAvailable,
                "empty planner debug geometry available") &&
         expect(result.debugAabbColliders.empty(),
                "empty planner debug colliders empty") &&
         expect(!result.blocked, "empty move not blocked") &&
         expect(!result.grounded, "empty move not grounded") &&
         expect(iggy3d::nearlyEqual(result.finalCenterMeters,
                                    {1.0F, 0.9F, 0.5F}),
                "empty move final center") &&
         expect(iggy3d::nearlyEqual(result.appliedDisplacementMeters,
                                    {1.0F, 0.0F, 0.5F}),
                "empty move applied") &&
         expect(displacementFactsMatch(result, desired),
                "empty move displacement facts");
}

bool floorGroundsBodyThroughBakeAndMotor() {
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({floorSurface()});
  const iggy3d::PlayerPhysicsMovePlannerResult result =
      iggy3d::planPlayerPhysicsMove(
          requestFor(&surfaces, {0.0F, 0.9F, 0.0F}, {}));

  return expect(result.ok, "floor move ok") &&
         expect(result.bakedSurfaceCount == 1U, "floor surface count") &&
         expect(result.bakedColliderCount == 1U, "floor collider count") &&
         expect(result.grounded, "floor grounded") &&
         expect(!result.blocked, "floor not blocked") &&
         expect(iggy3d::nearlyEqual(result.finalCenterMeters,
                                    {0.0F, 0.9F, 0.0F}),
                "floor final center");
}

bool wallBlocksAndReportsSurfaceId() {
  const iggy3d::SpatialSurfaceSet surfaces =
      makeSurfaceSet({floorSurface(), wallSurface("actor_wall")});
  const std::vector<std::string> beforeIds = surfaceIds(surfaces);
  const iggy3d::PlayerPhysicsMovePlannerResult result =
      iggy3d::planPlayerPhysicsMove(
          requestFor(&surfaces, {0.0F, 0.9F, 0.0F}, {3.0F, 0.0F, 0.0F}));

  return expect(result.ok, "wall move ok") &&
         expect(result.bakedSurfaceCount == 2U, "wall surface count") &&
         expect(result.bakedColliderCount == 2U, "wall collider count") &&
         expect(result.debugGeometryAvailable, "wall debug geometry available") &&
         expect(result.debugAabbColliders.size() == 2U,
                "wall debug collider count") &&
         expect(result.debugAabbSourceSurfaceIds.size() == 2U,
                "wall debug source id count") &&
         expect(result.debugAabbSourceSurfaceIds[0] == "floor",
                "wall debug floor source id") &&
         expect(result.debugAabbSourceSurfaceIds[1] == "actor_wall",
                "wall debug actor source id") &&
         expect(result.debugAabbColliders[1].bodyId.value == 2U,
                "wall debug actor body id") &&
         expect(result.debugAabbColliders[1].bounds.max.x > 2.0F,
                "wall debug actor bounds copied") &&
         expect(result.blocked, "wall blocks") &&
         expect(result.hitCount == 1U, "wall hit count") &&
         expect(result.firstHitBodyId.value == 2U, "wall hit body id") &&
         expect(result.firstHitSourceSurfaceId == "actor_wall",
                "wall hit source id") &&
         expect(result.hitSourceSurfaceIds[0] == "actor_wall",
                "wall hit source vector") &&
         expect(near(result.finalCenterMeters.x, 1.55F),
                "wall stop x") &&
         expect(surfaceIds(surfaces) == beforeIds, "wall inputs unchanged");
}

bool diagonalMoveSlidesAlongActorWall() {
  const iggy3d::SpatialSurfaceSet surfaces =
      makeSurfaceSet({floorSurface(), wallSurface("slide_wall")});
  const iggy3d::PlayerPhysicsMovePlannerResult result =
      iggy3d::planPlayerPhysicsMove(
          requestFor(&surfaces, {0.0F, 0.9F, 0.0F}, {3.0F, 0.0F, 1.0F}));

  return expect(result.ok, "slide move ok") &&
         expect(result.blocked, "slide blocked") &&
         expect(result.hitCount == 1U, "slide hit count") &&
         expect(result.firstHitSourceSurfaceId == "slide_wall",
                "slide hit source id") &&
         expect(near(result.finalCenterMeters.x, 1.555F, 0.002F),
                "slide final x") &&
         expect(result.finalCenterMeters.z > 0.99F, "slide final z") &&
         expect(iggy3d::nearlyEqual(result.remainingDisplacementMeters, {}),
                "slide no remaining");
}

bool groundSnapRefreshesDisplacementFacts() {
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({floorSurface()});
  iggy3d::PlayerPhysicsMovePlannerConfig config = plannerConfig();
  config.motor.groundProbeDistanceMeters = 0.05F;
  config.motor.groundSnapDistanceMeters = 0.30F;
  const iggy3d::Vec3 start = {0.0F, 1.05F, 0.0F};
  const iggy3d::Vec3 desired = {1.0F, 0.0F, 0.0F};
  const iggy3d::PlayerPhysicsMovePlannerResult result =
      iggy3d::planPlayerPhysicsMove(
          requestFor(&surfaces, start, desired, config));

  return expect(result.ok, "snap move ok") &&
         expect(result.grounded, "snap grounded") &&
         expect(result.snappedToGround, "snap flag") &&
         expect(near(result.finalCenterMeters.y, 0.90F),
                "snap final y") &&
         expect(iggy3d::nearlyEqual(result.appliedDisplacementMeters,
                                    {1.0F, -0.15F, 0.0F}),
                "snap applied") &&
         expect(iggy3d::nearlyEqual(result.remainingDisplacementMeters,
                                    {0.0F, 0.15F, 0.0F}),
                "snap remaining") &&
         expect(displacementFactsMatch(result, desired),
                "snap displacement facts");
}

bool projectileOnlySurfacePolicyIsExplicit() {
  const iggy3d::SpatialSurfaceSet surfaces =
      makeSurfaceSet({floorSurface(), projectileOnlySurface()});
  iggy3d::PlayerPhysicsMovePlannerConfig defaultConfig = plannerConfig();
  const iggy3d::PlayerPhysicsMovePlannerResult skipped =
      iggy3d::planPlayerPhysicsMove(
          requestFor(&surfaces, {0.0F, 0.9F, 0.0F}, {3.0F, 0.0F, 0.0F},
                     defaultConfig));

  iggy3d::PlayerPhysicsMovePlannerConfig includeProjectile = plannerConfig();
  includeProjectile.surfaceBake.includeProjectileBlockers = true;
  const iggy3d::PlayerPhysicsMovePlannerResult included =
      iggy3d::planPlayerPhysicsMove(
          requestFor(&surfaces, {0.0F, 0.9F, 0.0F}, {3.0F, 0.0F, 0.0F},
                     includeProjectile));

  return expect(skipped.ok, "projectile skipped ok") &&
         expect(skipped.bakedColliderCount == 1U,
                "projectile skipped collider count") &&
         expect(!skipped.blocked, "projectile skipped does not block") &&
         expect(iggy3d::nearlyEqual(skipped.finalCenterMeters,
                                    {3.0F, 0.9F, 0.0F}),
                "projectile skipped full movement") &&
         expect(included.ok, "projectile included ok") &&
         expect(included.bakedColliderCount == 2U,
                "projectile included collider count") &&
         expect(included.blocked, "projectile included blocks") &&
         expect(included.firstHitSourceSurfaceId == "projectile_wall",
                "projectile included source id");
}

}  // namespace

int main() {
  const bool ok = statusNamesAndConfigValidation() &&
                  invalidRequestsRejectBeforeMovement() &&
                  emptySurfaceSetPlansClearMovement() &&
                  floorGroundsBodyThroughBakeAndMotor() &&
                  wallBlocksAndReportsSurfaceId() &&
                  diagonalMoveSlidesAlongActorWall() &&
                  groundSnapRefreshesDisplacementFacts() &&
                  projectileOnlySurfacePolicyIsExplicit();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

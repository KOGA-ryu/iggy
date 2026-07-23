#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/physics/PhysicsCollisionQueries.hpp"
#include "runtime/physics/PhysicsKinematicMotor.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F) {
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

iggy3d::RoomSpatialSurface heightPatchSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "height_patch";
  surface.sourceStaticMeshId = "terrain_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::HeightPatch;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {0.5F, 0.18F, 0.5F},
      {0.0F, 0.0F, 0.0F},
      {1.0F, 0.36F, 0.0F},
      {1.0F, 0.36F, 1.0F},
      {0.0F, 0.0F, 1.0F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.collisionMask = {"actor"};
  surface.runtimeOwnerStableName = "owner.height_patch";
  return surface;
}

iggy3d::RoomSpatialSurface wallSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "wall";
  surface.sourceStaticMeshId = "wall_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {2.0F, 0.0F, -4.0F},
      {3.0F, 0.0F, -4.0F},
      {3.0F, 2.0F, 4.0F},
      {2.0F, 2.0F, 4.0F},
  };
  surface.normal = {-1.0F, 0.0F, 0.0F};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  surface.runtimeOwnerStableName = "owner.wall";
  return surface;
}

iggy3d::RoomSpatialSurface transparentWallSurface() {
  iggy3d::RoomSpatialSurface surface = wallSurface();
  surface.id = "transparent_wall";
  surface.blocksVision = false;
  surface.runtimeOwnerStableName = "owner.transparent_wall";
  return surface;
}

iggy3d::RoomSpatialSurface projectileOnlySurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "projectile";
  surface.sourceStaticMeshId = "projectile_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker;
  surface.pointsMeters = {
      {-3.0F, 0.0F, -1.0F},
      {-2.0F, 0.0F, -1.0F},
      {-2.0F, 1.0F, 1.0F},
      {-3.0F, 1.0F, 1.0F},
  };
  surface.normal = {1.0F, 0.0F, 0.0F};
  surface.collisionMask = {"projectile"};
  surface.blocksProjectile = true;
  surface.runtimeOwnerStableName = "owner.projectile";
  return surface;
}

iggy3d::RoomSpatialSurface actorMaskBlockerSurface() {
  iggy3d::RoomSpatialSurface surface = wallSurface();
  surface.id = "actor_mask_wall";
  surface.blocksActor = false;
  surface.collisionMask = {"actor"};
  surface.runtimeOwnerStableName = "owner.actor_mask_wall";
  return surface;
}

iggy3d::RoomSpatialSurface blockerRoleProjectileOnlySurface() {
  iggy3d::RoomSpatialSurface surface = projectileOnlySurface();
  surface.id = "blocker_role_projectile";
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.runtimeOwnerStableName = "owner.blocker_role_projectile";
  return surface;
}

iggy3d::RoomSpatialSurface openingSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "opening";
  surface.sourceStaticMeshId = "opening_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Opening;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Opening;
  surface.pointsMeters = {
      {5.0F, 0.0F, -1.0F},
      {5.0F, 2.0F, -1.0F},
      {5.0F, 2.0F, 1.0F},
      {5.0F, 0.0F, 1.0F},
  };
  surface.normal = {1.0F, 0.0F, 0.0F};
  surface.openingId = "door";
  surface.runtimeOwnerStableName = "owner.opening";
  return surface;
}

iggy3d::RoomSpatialSurface malformedSurface() {
  iggy3d::RoomSpatialSurface surface = floorSurface();
  surface.id = "malformed";
  surface.pointsMeters[0].x = std::numeric_limits<float>::infinity();
  return surface;
}

iggy3d::SpatialSurfaceSet surfaceSet(
    std::initializer_list<iggy3d::RoomSpatialSurface> surfaces) {
  iggy3d::RoomAsset room;
  room.id = "synthetic";
  room.spatialSurfaces.assign(surfaces.begin(), surfaces.end());
  return iggy3d::buildSpatialSurfaceSet(room);
}

iggy3d::PhysicsAabbCollider bodyAt(iggy3d::Vec3 centerMeters) {
  iggy3d::PhysicsAabbCollider body;
  body.bodyId = {1000U};
  body.worldCenterMeters = centerMeters;
  body.halfExtentsMeters = {0.5F, 0.5F, 0.5F};
  body.bounds = iggy3d::aabbFromCenterExtents(body.worldCenterMeters,
                                              body.halfExtentsMeters);
  return body;
}

std::vector<std::string> surfaceIds(const iggy3d::SpatialSurfaceSet& set) {
  std::vector<std::string> ids;
  for (const iggy3d::CollisionSurfaceView& surface : set.surfaces()) {
    ids.push_back(surface.id);
  }
  return ids;
}

bool statusNamesAndConfigValidation() {
  iggy3d::PhysicsSpatialSurfaceColliderBakeConfig valid;
  iggy3d::PhysicsSpatialSurfaceColliderBakeConfig invalidThickness = valid;
  invalidThickness.planeThicknessMeters = 0.0F;
  iggy3d::PhysicsSpatialSurfaceColliderBakeConfig invalidHalf = valid;
  invalidHalf.minHalfExtentMeters = -0.001F;
  iggy3d::PhysicsSpatialSurfaceColliderBakeConfig invalidBody = valid;
  invalidBody.firstGeneratedBodyId = {};

  return expect(iggy3d::physicsSpatialSurfaceColliderBakeStatusName(
                    iggy3d::PhysicsSpatialSurfaceColliderBakeStatus::Baked) ==
                    "physics_spatial_surface_bake_baked",
                "baked status") &&
         expect(iggy3d::physicsSpatialSurfaceColliderBakeStatusName(
                    iggy3d::PhysicsSpatialSurfaceColliderBakeStatus::
                        MissingSurfaceSet) ==
                    "physics_spatial_surface_bake_missing_surface_set",
                "missing status") &&
         expect(iggy3d::physicsSpatialSurfaceColliderBakeStatusName(
                    iggy3d::PhysicsSpatialSurfaceColliderBakeStatus::
                        InvalidConfig) ==
                    "physics_spatial_surface_bake_invalid_config",
                "invalid config status") &&
         expect(iggy3d::physicsSpatialSurfaceColliderBakeStatusName(
                    iggy3d::PhysicsSpatialSurfaceColliderBakeStatus::
                        InvalidSurfaceBounds) ==
                    "physics_spatial_surface_bake_invalid_surface_bounds",
                "invalid bounds status") &&
         expect(iggy3d::physicsSpatialSurfaceColliderBakeStatusName(
                    iggy3d::PhysicsSpatialSurfaceColliderBakeStatus::
                        InvalidSurfaceShape) ==
                    "physics_spatial_surface_bake_invalid_surface_shape",
                "invalid shape status") &&
         expect(iggy3d::physicsSpatialSurfaceColliderBakeStatusName(
                    iggy3d::PhysicsSpatialSurfaceColliderBakeStatus::
                        InvalidGeneratedCollider) ==
                    "physics_spatial_surface_bake_invalid_generated_collider",
                "invalid collider status") &&
         expect(iggy3d::isValidPhysicsSpatialSurfaceColliderBakeConfig(valid),
                "valid config") &&
         expect(!iggy3d::isValidPhysicsSpatialSurfaceColliderBakeConfig(
                    invalidThickness),
                "invalid thickness") &&
         expect(!iggy3d::isValidPhysicsSpatialSurfaceColliderBakeConfig(
                    invalidHalf),
                "invalid half") &&
         expect(!iggy3d::isValidPhysicsSpatialSurfaceColliderBakeConfig(
                    invalidBody),
                "invalid body id");
}

bool missingAndEmptyInputsAreStable() {
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult missing =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({});
  const iggy3d::SpatialSurfaceSet empty;
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult bakedEmpty =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&empty, {}});

  return expect(!missing.ok, "missing surface set rejected") &&
         expect(missing.reasonCode ==
                    "physics_spatial_surface_bake_missing_surface_set",
                "missing surface reason") &&
         expect(bakedEmpty.ok, "empty set bakes ok") &&
         expect(bakedEmpty.surfaceCount == 0U, "empty surface count") &&
         expect(bakedEmpty.colliderCount == 0U, "empty collider count") &&
         expect(bakedEmpty.colliders.empty(), "empty colliders");
}

bool roomSurfacesBakeInDeterministicOrderAndPreserveMetadata() {
  const iggy3d::SpatialSurfaceSet set =
      surfaceSet({floorSurface(), wallSurface(), projectileOnlySurface(),
                  openingSurface()});
  const std::vector<std::string> beforeIds = surfaceIds(set);

  iggy3d::PhysicsSpatialSurfaceColliderBakeConfig config;
  config.firstGeneratedBodyId = {50U};
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult result =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&set, config});

  return expect(result.ok, "surface bake ok") &&
         expect(result.surfaceCount == 4U, "source surface count") &&
         expect(result.colliderCount == 2U, "default collider count") &&
         expect(result.skippedSurfaceCount == 2U, "default skipped count") &&
         expect(result.sourceSurfaceIndices[0] == 0U,
                "floor source index") &&
         expect(result.sourceSurfaceIndices[1] == 1U,
                "wall source index") &&
         expect(result.sourceSurfaceIds[0] == "floor", "floor source id") &&
         expect(result.sourceSurfaceIds[1] == "wall", "wall source id") &&
         expect(result.sourceRoles[0] == iggy3d::CollisionSurfaceRole::Walkable,
                "floor role") &&
         expect(result.sourceRoles[1] == iggy3d::CollisionSurfaceRole::Blocker,
                "wall role") &&
         expect(result.sourceShapes[0] == iggy3d::CollisionSurfaceShape::Plane,
                "floor shape") &&
         expect(result.sourceShapes[1] == iggy3d::CollisionSurfaceShape::Box,
                "wall shape") &&
         expect(result.runtimeOwnerStableNames[0] == "owner.floor",
                "floor owner") &&
         expect(result.runtimeOwnerStableNames[1] == "owner.wall",
                "wall owner") &&
         expect(result.colliders[0].bodyId.value == 50U,
                "floor generated body id") &&
         expect(result.colliders[1].bodyId.value == 51U,
                "wall generated body id") &&
         expect(iggy3d::isValidPhysicsAabbCollider(result.colliders[0]),
                "floor collider valid") &&
         expect(iggy3d::isValidPhysicsAabbCollider(result.colliders[1]),
                "wall collider valid") &&
         expect(surfaceIds(set) == beforeIds, "surface ids unchanged");
}

bool heightPatchStaysQueryOwnedWhileCliffBoxesStillBake() {
  const iggy3d::SpatialSurfaceSet set =
      surfaceSet({heightPatchSurface(), wallSurface()});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult result =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&set, {}});

  return expect(result.ok, "height patch and cliff bake succeeds") &&
         expect(result.surfaceCount == 2U &&
                    result.skippedSurfaceCount == 1U &&
                    result.colliderCount == 1U,
                "height patch skips one AABB while cliff keeps one") &&
         expect(result.sourceSurfaceIndices[0] == 1U &&
                    result.sourceSurfaceIds[0] == "wall" &&
                    result.sourceShapes[0] ==
                        iggy3d::CollisionSurfaceShape::Box,
                "only the cliff box enters physics collider storage");
}

bool defaultPolicySkipsProjectileOnlyAndOpeningSurfaces() {
  const iggy3d::SpatialSurfaceSet set =
      surfaceSet({projectileOnlySurface(), openingSurface()});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult skipped =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&set, {}});

  iggy3d::PhysicsSpatialSurfaceColliderBakeConfig includeProjectile;
  includeProjectile.includeProjectileBlockers = true;
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult projectile =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(
          {&set, includeProjectile});

  iggy3d::PhysicsSpatialSurfaceColliderBakeConfig includeOpening;
  includeOpening.includeOpenings = true;
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult opening =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(
          {&set, includeOpening});

  return expect(skipped.ok, "default skip ok") &&
         expect(skipped.colliderCount == 0U, "default skips all") &&
         expect(skipped.skippedSurfaceCount == 2U, "default skip count") &&
         expect(projectile.ok, "include projectile ok") &&
         expect(projectile.colliderCount == 1U, "projectile included") &&
         expect(projectile.sourceSurfaceIds[0] == "projectile",
                "projectile id") &&
         expect(!opening.ok, "opening include rejected") &&
         expect(opening.reasonCode ==
                    "physics_spatial_surface_bake_invalid_surface_shape",
                "opening unsupported reason") &&
         expect(opening.invalidSurfaceIndex == 1U,
                "opening invalid source index");
}

bool actorBlockerPolicyFollowsActorQuerySemantics() {
  const iggy3d::SpatialSurfaceSet set =
      surfaceSet({actorMaskBlockerSurface()});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult result =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&set, {}});

  return expect(result.ok, "actor mask blocker bakes ok") &&
         expect(result.colliderCount == 1U, "actor mask blocker included") &&
         expect(result.sourceSurfaceIds[0] == "actor_mask_wall",
                "actor mask blocker id") &&
         expect(result.sourceRoles[0] == iggy3d::CollisionSurfaceRole::Blocker,
                "actor mask blocker role") &&
         expect(result.runtimeOwnerStableNames[0] == "owner.actor_mask_wall",
                "actor mask blocker owner");
}

bool visionOcclusionMetadataReachesPhysicsCollider() {
  const iggy3d::SpatialSurfaceSet set =
      surfaceSet({transparentWallSurface()});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult result =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&set, {}});

  return expect(set.surfaces().size() == 1U &&
                    !set.surfaces().front().blocksVision,
                "surface set preserves transparent blocker metadata") &&
         expect(result.ok && result.colliderCount == 1U,
                "transparent actor blocker still bakes") &&
         expect(!result.colliders.front().occludesVision,
                "physics collider preserves non-occluding semantics");
}

bool blockerRoleProjectileMaskIsNotActorBlockerByDefault() {
  const iggy3d::SpatialSurfaceSet set =
      surfaceSet({blockerRoleProjectileOnlySurface()});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult skipped =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&set, {}});

  iggy3d::PhysicsSpatialSurfaceColliderBakeConfig includeProjectile;
  includeProjectile.includeProjectileBlockers = true;
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult included =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(
          {&set, includeProjectile});

  return expect(skipped.ok, "blocker role projectile default ok") &&
         expect(skipped.colliderCount == 0U,
                "blocker role projectile skipped by default") &&
         expect(skipped.skippedSurfaceCount == 1U,
                "blocker role projectile skip count") &&
         expect(included.ok, "blocker role projectile included ok") &&
         expect(included.colliderCount == 1U,
                "blocker role projectile included") &&
         expect(included.sourceSurfaceIds[0] == "blocker_role_projectile",
                "blocker role projectile id") &&
         expect(included.sourceRoles[0] == iggy3d::CollisionSurfaceRole::Blocker,
                "blocker role projectile preserves role") &&
         expect(included.runtimeOwnerStableNames[0] ==
                    "owner.blocker_role_projectile",
                "blocker role projectile owner");
}

bool malformedAuthoredSurfaceIsFilteredBeforeBake() {
  const iggy3d::SpatialSurfaceSet set = surfaceSet({malformedSurface()});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult result =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&set, {}});

  return expect(set.empty(), "malformed surface filtered by spatial set") &&
         expect(result.ok, "filtered malformed set bakes ok") &&
         expect(result.surfaceCount == 0U, "filtered malformed surface count") &&
         expect(result.colliderCount == 0U, "filtered malformed collider count");
}

bool floorPlaneThicknessPolicyIsExact() {
  iggy3d::PhysicsSpatialSurfaceColliderBakeConfig config;
  config.planeThicknessMeters = 0.20F;
  config.minHalfExtentMeters = 0.01F;
  const iggy3d::SpatialSurfaceSet set = surfaceSet({floorSurface()});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult result =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&set, config});

  return expect(result.ok, "floor thickness bake ok") &&
         expect(result.colliderCount == 1U, "floor thickness collider count") &&
         expect(nearlyEqual(result.colliders[0].bounds.max.y, 0.0F),
                "floor top face preserved") &&
         expect(nearlyEqual(result.colliders[0].bounds.min.y, -0.20F),
                "floor bottom thickness") &&
         expect(nearlyEqual(result.colliders[0].worldCenterMeters.y, -0.10F),
                "floor center y") &&
         expect(nearlyEqual(result.colliders[0].halfExtentsMeters.y, 0.10F),
                "floor half y");
}

bool authoredFloorThicknessOverridesFallbackPolicy() {
  iggy3d::PhysicsSpatialSurfaceColliderBakeConfig config;
  config.planeThicknessMeters = 0.20F;
  iggy3d::RoomSpatialSurface floor = floorSurface();
  floor.collisionThicknessMeters = 0.05F;
  const iggy3d::SpatialSurfaceSet set = surfaceSet({floor});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult result =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&set, config});

  return expect(result.ok && result.colliderCount == 1U,
                "authored floor thickness bakes") &&
         expect(nearlyEqual(result.colliders[0].bounds.max.y, 0.0F),
                "authored floor preserves top face") &&
         expect(nearlyEqual(result.colliders[0].bounds.min.y, -0.05F),
                "authored floor thickness overrides fallback");
}

bool bakedFloorWorksWithGroundCheck() {
  const iggy3d::SpatialSurfaceSet set = surfaceSet({floorSurface()});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult bake =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&set, {}});
  const iggy3d::PhysicsAabbCollider body = bodyAt({0.0F, 0.6F, 0.0F});

  iggy3d::PhysicsGroundCheckQueryRequest request;
  request.colliders = &bake.colliders;
  request.movingCollider = &body;
  request.probeDistanceMeters = 0.20F;
  const iggy3d::PhysicsGroundCheckQueryResult ground =
      iggy3d::checkPhysicsGround(request);

  return expect(bake.ok, "ground bake ok") &&
         expect(ground.ok, "ground query ok") &&
         expect(ground.grounded, "baked floor grounded") &&
         expect(nearlyEqual(ground.groundDistanceMeters, 0.10F),
                "baked floor ground distance") &&
         expect(iggy3d::nearlyEqual(ground.groundNormal,
                                    {0.0F, 1.0F, 0.0F}),
                "baked floor ground normal");
}

bool bakedWallWorksWithKinematicMotor() {
  const iggy3d::SpatialSurfaceSet set = surfaceSet({wallSurface()});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult bake =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&set, {}});
  const iggy3d::PhysicsAabbCollider body = bodyAt({0.0F, 1.0F, 0.0F});

  iggy3d::PhysicsKinematicMotorConfig config;
  config.skinMeters = 0.10F;
  config.groundProbeDistanceMeters = 0.0F;
  config.groundSnapDistanceMeters = 0.0F;
  iggy3d::PhysicsKinematicMotorRequest request;
  request.colliders = &bake.colliders;
  request.bodyCollider = &body;
  request.desiredDisplacementMeters = {3.0F, 0.0F, 1.0F};
  request.config = config;
  const iggy3d::PhysicsKinematicMotorResult move =
      iggy3d::planPhysicsKinematicAabbMove(request);

  return expect(bake.ok, "motor bake ok") &&
         expect(move.ok, "motor move ok") &&
         expect(move.blocked, "baked wall blocks motor") &&
         expect(move.hitCount == 1U, "baked wall hit count") &&
         expect(move.hits[0].bodyId.value == 1U, "baked wall body id") &&
         expect(nearlyEqual(move.finalCenterMeters.x, 1.405F, 0.001F),
                "baked wall skin stop x") &&
         expect(move.finalCenterMeters.z > 0.99F,
                "baked wall slide z") &&
         expect(iggy3d::nearlyEqual(move.remainingDisplacementMeters, {}),
                "baked wall no remaining");
}

}  // namespace

int main() {
  const bool ok =
      statusNamesAndConfigValidation() &&
      missingAndEmptyInputsAreStable() &&
      roomSurfacesBakeInDeterministicOrderAndPreserveMetadata() &&
      heightPatchStaysQueryOwnedWhileCliffBoxesStillBake() &&
      defaultPolicySkipsProjectileOnlyAndOpeningSurfaces() &&
      actorBlockerPolicyFollowsActorQuerySemantics() &&
      visionOcclusionMetadataReachesPhysicsCollider() &&
      blockerRoleProjectileMaskIsNotActorBlockerByDefault() &&
      malformedAuthoredSurfaceIsFilteredBeforeBake() &&
      floorPlaneThicknessPolicyIsExact() &&
      authoredFloorThicknessOverridesFallbackPolicy() &&
      bakedFloorWorksWithGroundCheck() &&
      bakedWallWorksWithKinematicMotor();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

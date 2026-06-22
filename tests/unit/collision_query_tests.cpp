#include "content/PackageLoader.hpp"
#include "runtime/collision/CollisionQuery.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool approx(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

iggy3d::SpatialSurfaceSet loadFirstRoomSurfaceSet() {
  const std::filesystem::path packagePath =
      std::filesystem::current_path() / "fixtures/demos/first_room/package.iggy3d.toml";
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({packagePath.generic_string()});
  if (package.status != iggy3d::PackageLoadStatus::Ok || package.rooms.empty()) {
    return {};
  }
  return iggy3d::buildSpatialSurfaceSet(package.rooms.front());
}

iggy3d::RoomSpatialSurface makeBlocker(std::string_view id) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::string(id);
  surface.sourceStaticMeshId = "synthetic";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {0.0F, 0.0F, 1.0F},
      {2.0F, 0.0F, 1.0F},
      {2.0F, 2.0F, 1.1F},
      {0.0F, 2.0F, 1.1F},
  };
  surface.normal = {0.0F, 0.0F, -1.0F};
  surface.traversalTags = {"blocker"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

bool firstRoomHeightAndNormalQueries() {
  const iggy3d::SpatialSurfaceSet surfaces = loadFirstRoomSurfaceSet();
  const iggy3d::Vec3 spawn{10.0F * 0.3048F, 2.0F, 9.0F * 0.3048F};
  const iggy3d::CollisionQueryResult height =
      iggy3d::sampleSurfaceHeight(surfaces, spawn);
  const iggy3d::CollisionQueryResult normal =
      iggy3d::sampleSurfaceNormal(surfaces, spawn);
  return expect(surfaces.size() >= 4U, "surface set loaded") &&
         expect(height.status == iggy3d::CollisionQueryStatus::Hit,
                "surface_height_sampled=true") &&
         expect(height.surfaceId == "spawn_floor_walkable", "height floor id") &&
         expect(std::isfinite(height.heightMeters), "finite height") &&
         expect(normal.status == iggy3d::CollisionQueryStatus::Hit,
                "surface_normal_valid=true") &&
         expect(approx(normal.normal.x, 0.0F) && approx(normal.normal.y, 1.0F) &&
                    approx(normal.normal.z, 0.0F),
                "floor normal world up");
}

bool offsetSurfaceSetAlignsPlayerSpawnToOrigin() {
  const std::filesystem::path packagePath =
      std::filesystem::current_path() / "fixtures/demos/first_room/package.iggy3d.toml";
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({packagePath.generic_string()});
  if (package.status != iggy3d::PackageLoadStatus::Ok || package.rooms.empty()) {
    return expect(false, "package loaded");
  }

  iggy3d::Vec3 playerSpawn;
  bool foundPlayerSpawn = false;
  for (const iggy3d::RoomAnchorAsset& anchor : package.rooms.front().anchors) {
    if (anchor.id == "player_spawn") {
      playerSpawn = anchor.positionMeters;
      foundPlayerSpawn = true;
      break;
    }
  }
  const iggy3d::Vec3 worldOffset{-playerSpawn.x, -playerSpawn.y, -playerSpawn.z};
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(package.rooms.front(), worldOffset);
  const iggy3d::CollisionQueryResult height =
      iggy3d::sampleSurfaceHeight(surfaces, {0.0F, 2.0F, 0.0F});
  const iggy3d::CollisionQueryResult centerStride =
      iggy3d::querySegment(surfaces,
                           {0.0F, 1.0F, 0.0F},
                           {0.5F, 1.0F, 0.0F},
                           iggy3d::CollisionQueryKind::Actor);
  return expect(foundPlayerSpawn, "player spawn anchor") &&
         expect(height.status == iggy3d::CollisionQueryStatus::Hit,
                "offset height hits player origin") &&
         expect(height.surfaceId == "spawn_floor_walkable", "offset floor id") &&
         expect(centerStride.status == iggy3d::CollisionQueryStatus::NoHit,
                "offset center stride is not wall blocked");
}

bool actorQueriesRespectWallOpeningAndProjectileOnlyBlocker() {
  const iggy3d::SpatialSurfaceSet surfaces = loadFirstRoomSurfaceSet();
  const iggy3d::CollisionQueryResult wallHit =
      iggy3d::querySegment(surfaces,
                           {10.0F * 0.3048F, 1.0F, 2.0F},
                           {10.0F * 0.3048F, 1.0F, -1.0F},
                           iggy3d::CollisionQueryKind::Actor);
  const iggy3d::CollisionQueryResult openingHit =
      iggy3d::querySegment(surfaces,
                           {18.0F * 0.3048F, 1.0F, 9.0F * 0.3048F},
                           {22.0F * 0.3048F, 1.0F, 9.0F * 0.3048F},
                           iggy3d::CollisionQueryKind::Actor);
  const iggy3d::CollisionQueryResult actorCrateHit =
      iggy3d::querySegment(surfaces,
                           {4.0F * 0.3048F, 1.0F * 0.3048F, 14.0F * 0.3048F},
                           {4.0F * 0.3048F, 1.0F * 0.3048F, 18.0F * 0.3048F},
                           iggy3d::CollisionQueryKind::Actor);
  return expect(wallHit.status == iggy3d::CollisionQueryStatus::Hit,
                "actor_blocker_hit=true") &&
         expect(wallHit.surfaceId == "north_wall_actor_blocker", "north wall hit id") &&
         expect(openingHit.status == iggy3d::CollisionQueryStatus::NoHit,
                "opening_blocks_actor=false") &&
         expect(actorCrateHit.status == iggy3d::CollisionQueryStatus::NoHit,
                "projectile blocker not actor blocker");
}

bool projectileQueriesRespectProjectileBlockers() {
  const iggy3d::SpatialSurfaceSet surfaces = loadFirstRoomSurfaceSet();
  const iggy3d::CollisionQueryResult projectileHit =
      iggy3d::querySegment(surfaces,
                           {4.0F * 0.3048F, 1.0F * 0.3048F, 14.0F * 0.3048F},
                           {4.0F * 0.3048F, 1.0F * 0.3048F, 18.0F * 0.3048F},
                           iggy3d::CollisionQueryKind::Projectile);
  return expect(projectileHit.status == iggy3d::CollisionQueryStatus::Hit,
                "projectile_blocker_hit=true") &&
         expect(projectileHit.surfaceId == "spawn_crate_projectile_blocker",
                "projectile blocker id");
}

bool overlapQueriesAreValueResults() {
  const iggy3d::SpatialSurfaceSet surfaces = loadFirstRoomSurfaceSet();
  const iggy3d::CollisionQueryResult point =
      iggy3d::queryPointOverlap(surfaces,
                                {4.0F * 0.3048F, 1.0F * 0.3048F, 16.0F * 0.3048F},
                                iggy3d::CollisionQueryKind::Projectile);
  const iggy3d::CollisionQueryResult box =
      iggy3d::queryAabbOverlap(surfaces,
                               iggy3d::makeAabb3({9.0F * 0.3048F, 0.0F, 0.0F},
                                                  {11.0F * 0.3048F, 1.0F, 1.0F}),
                               iggy3d::CollisionQueryKind::Actor);
  return expect(point.status == iggy3d::CollisionQueryStatus::Hit, "point overlap hit") &&
         expect(point.surfaceId == "spawn_crate_projectile_blocker", "point overlap id") &&
         expect(box.status == iggy3d::CollisionQueryStatus::Hit, "aabb overlap hit") &&
         expect(box.surfaceId == "north_wall_actor_blocker", "aabb overlap id");
}

bool emptyAndInvalidInputsAreDiagnosed() {
  const iggy3d::SpatialSurfaceSet empty;
  const iggy3d::CollisionQueryResult emptyHeight =
      iggy3d::sampleSurfaceHeight(empty, {0.0F, 0.0F, 0.0F});
  const iggy3d::CollisionQueryResult invalidHeight =
      iggy3d::sampleSurfaceHeight(loadFirstRoomSurfaceSet(),
                                  {std::numeric_limits<float>::infinity(), 0.0F, 0.0F});
  const iggy3d::CollisionQueryResult invalidSegment =
      iggy3d::querySegment(loadFirstRoomSurfaceSet(),
                           {0.0F, 0.0F, 0.0F},
                           {0.0F, 0.0F, 0.0F},
                           iggy3d::CollisionQueryKind::Actor);
  return expect(emptyHeight.status == iggy3d::CollisionQueryStatus::EmptySurfaceSet,
                "empty surface set") &&
         expect(emptyHeight.reasonCode == "collision_empty_surface_set", "empty reason") &&
         expect(invalidHeight.status == iggy3d::CollisionQueryStatus::InvalidInput,
                "invalid height input") &&
         expect(invalidSegment.status == iggy3d::CollisionQueryStatus::InvalidInput,
                "invalid segment input");
}

bool deterministicTieOrderingUsesStableId() {
  iggy3d::RoomAsset room;
  room.spatialSurfaces.push_back(makeBlocker("wall_b"));
  room.spatialSurfaces.push_back(makeBlocker("wall_a"));
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::CollisionQueryResult result =
      iggy3d::querySegment(surfaces,
                           {1.0F, 1.0F, 0.0F},
                           {1.0F, 1.0F, 2.0F},
                           iggy3d::CollisionQueryKind::Actor);
  return expect(result.status == iggy3d::CollisionQueryStatus::Hit,
                "collision_sweep_count=1") &&
         expect(result.surfaceId == "wall_a", "query_result_order=stable");
}

}  // namespace

int main() {
  const bool ok = firstRoomHeightAndNormalQueries() &&
                  offsetSurfaceSetAlignsPlayerSpawnToOrigin() &&
                  actorQueriesRespectWallOpeningAndProjectileOnlyBlocker() &&
                  projectileQueriesRespectProjectileBlockers() &&
                  overlapQueriesAreValueResults() &&
                  emptyAndInvalidInputsAreDiagnosed() &&
                  deterministicTieOrderingUsesStableId();
  return ok ? 0 : 1;
}

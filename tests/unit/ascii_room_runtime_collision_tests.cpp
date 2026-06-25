#include "content/PackageLoader.hpp"
#include "runtime/collision/CollisionQuery.hpp"

#include <cstddef>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

constexpr std::string_view kPackagePath =
    "fixtures/demos/ascii_training_room/package.iggy3d.toml";

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool approx(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

iggy3d::PackageLoadResult loadAsciiPackage() {
  return iggy3d::loadPackage({std::string(kPackagePath)});
}

const iggy3d::RoomAnchorAsset* findAnchor(const iggy3d::RoomAsset& room,
                                          std::string_view id) {
  for (const iggy3d::RoomAnchorAsset& anchor : room.anchors) {
    if (anchor.id == id) {
      return &anchor;
    }
  }
  return nullptr;
}

const iggy3d::RoomSpatialSurface* findSurface(const iggy3d::RoomAsset& room,
                                              std::string_view id) {
  for (const iggy3d::RoomSpatialSurface& surface : room.spatialSurfaces) {
    if (surface.id == id) {
      return &surface;
    }
  }
  return nullptr;
}

std::size_t countViewsWithRole(const iggy3d::SpatialSurfaceSet& set,
                               iggy3d::CollisionSurfaceRole role) {
  std::size_t count = 0;
  for (const iggy3d::CollisionSurfaceView& surface : set.surfaces()) {
    if (surface.role == role) {
      ++count;
    }
  }
  return count;
}

iggy3d::RoomAsset roomWithOnlySurface(const iggy3d::RoomSpatialSurface& surface) {
  iggy3d::RoomAsset room;
  room.id = "surface_probe";
  room.spatialSurfaces.push_back(surface);
  return room;
}

bool packageBuildsExpectedRuntimeSurfaces() {
  const iggy3d::PackageLoadResult package = loadAsciiPackage();
  if (!expect(package.status == iggy3d::PackageLoadStatus::Ok, "package load ok") ||
      !expect(package.rooms.size() == 1U, "one room")) {
    return false;
  }
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(package.rooms.front());
  return expect(surfaces.size() == 55U, "surface set size") &&
         expect(countViewsWithRole(surfaces, iggy3d::CollisionSurfaceRole::Walkable) == 15U,
                "walkable view count") &&
         expect(countViewsWithRole(surfaces, iggy3d::CollisionSurfaceRole::Blocker) == 20U,
                "blocker view count") &&
         expect(countViewsWithRole(surfaces,
                                   iggy3d::CollisionSurfaceRole::ProjectileBlocker) == 20U,
                "projectile blocker view count");
}

bool playerSpawnSamplesGroundAndNormal() {
  const iggy3d::PackageLoadResult package = loadAsciiPackage();
  if (!expect(package.status == iggy3d::PackageLoadStatus::Ok, "package load ok") ||
      !expect(!package.rooms.empty(), "room exists")) {
    return false;
  }
  const iggy3d::RoomAsset& room = package.rooms.front();
  const iggy3d::RoomAnchorAsset* player = findAnchor(room, "marker_player_spawn_r1_c1");
  if (!expect(player != nullptr, "player spawn anchor") ||
      !expect(player->kind == "spawn", "player spawn kind")) {
    return false;
  }

  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::Vec3 queryPoint{player->positionMeters.x, 2.0F, player->positionMeters.z};
  const iggy3d::CollisionQueryResult height =
      iggy3d::sampleSurfaceHeight(surfaces, queryPoint);
  const iggy3d::CollisionQueryResult normal =
      iggy3d::sampleSurfaceNormal(surfaces, queryPoint);
  return expect(height.status == iggy3d::CollisionQueryStatus::Hit, "height hit") &&
         expect(height.surfaceId == "floor_r1_c1_walkable", "height floor id") &&
         expect(approx(height.heightMeters, 0.0F), "height at floor top") &&
         expect(normal.status == iggy3d::CollisionQueryStatus::Hit, "normal hit") &&
         expect(normal.surfaceId == "floor_r1_c1_walkable", "normal floor id") &&
         expect(approx(normal.normal.x, 0.0F) && approx(normal.normal.y, 1.0F) &&
                    approx(normal.normal.z, 0.0F),
                "normal world up");
}

bool actorMovementQueriesUseAsciiWalls() {
  const iggy3d::PackageLoadResult package = loadAsciiPackage();
  if (!expect(package.status == iggy3d::PackageLoadStatus::Ok, "package load ok") ||
      !expect(!package.rooms.empty(), "room exists")) {
    return false;
  }
  const iggy3d::RoomAsset& room = package.rooms.front();
  const iggy3d::RoomAnchorAsset* player = findAnchor(room, "marker_player_spawn_r1_c1");
  const iggy3d::RoomSpatialSurface* projectileOnly =
      findSurface(room, "wall_r0_c1_projectile_blocker");
  if (!expect(player != nullptr, "player spawn anchor") ||
      !expect(projectileOnly != nullptr, "projectile surface exists")) {
    return false;
  }

  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::Vec3 start{player->positionMeters.x, 0.0F, player->positionMeters.z};
  const iggy3d::CollisionQueryResult northWallHit =
      iggy3d::querySegment(surfaces,
                           start,
                           {player->positionMeters.x, 0.0F, -2.25F},
                           iggy3d::CollisionQueryKind::Actor);
  const iggy3d::CollisionQueryResult openStride =
      iggy3d::querySegment(surfaces,
                           start,
                           {player->positionMeters.x + 0.25F, 0.0F, player->positionMeters.z},
                           iggy3d::CollisionQueryKind::Actor);

  const iggy3d::SpatialSurfaceSet projectileOnlySet =
      iggy3d::buildSpatialSurfaceSet(roomWithOnlySurface(*projectileOnly));
  const iggy3d::CollisionQueryResult actorAgainstProjectileOnly =
      iggy3d::querySegment(projectileOnlySet,
                           {player->positionMeters.x, 0.0F, -1.25F},
                           {player->positionMeters.x, 0.0F, -2.25F},
                           iggy3d::CollisionQueryKind::Actor);

  return expect(northWallHit.status == iggy3d::CollisionQueryStatus::Hit,
                "actor north wall hit") &&
         expect(northWallHit.surfaceId == "wall_r0_c1_actor_blocker",
                "actor north wall id") &&
         expect(northWallHit.role == iggy3d::CollisionSurfaceRole::Blocker,
                "actor wall role") &&
         expect(openStride.status == iggy3d::CollisionQueryStatus::NoHit,
                "open floor stride no hit") &&
         expect(actorAgainstProjectileOnly.status == iggy3d::CollisionQueryStatus::NoHit,
                "projectile-only surface not actor blocker");
}

bool projectileQueriesUseAsciiProjectileWalls() {
  const iggy3d::PackageLoadResult package = loadAsciiPackage();
  if (!expect(package.status == iggy3d::PackageLoadStatus::Ok, "package load ok") ||
      !expect(!package.rooms.empty(), "room exists")) {
    return false;
  }
  const iggy3d::RoomAsset& room = package.rooms.front();
  const iggy3d::RoomAnchorAsset* player = findAnchor(room, "marker_player_spawn_r1_c1");
  if (!expect(player != nullptr, "player spawn anchor")) {
    return false;
  }

  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::CollisionQueryResult projectileHit =
      iggy3d::querySegment(surfaces,
                           {player->positionMeters.x, 0.0F, player->positionMeters.z},
                           {player->positionMeters.x, 0.0F, -2.25F},
                           iggy3d::CollisionQueryKind::Projectile);
  return expect(projectileHit.status == iggy3d::CollisionQueryStatus::Hit,
                "projectile north wall hit") &&
         expect(projectileHit.reasonCode == "collision_hit", "projectile reason") &&
         expect(projectileHit.surfaceId == "wall_r0_c1_projectile_blocker",
                "projectile wall id") &&
         expect(projectileHit.role == iggy3d::CollisionSurfaceRole::ProjectileBlocker,
                "projectile wall role");
}

bool offsetSurfaceSetKeepsSpawnCentered() {
  const iggy3d::PackageLoadResult package = loadAsciiPackage();
  if (!expect(package.status == iggy3d::PackageLoadStatus::Ok, "package load ok") ||
      !expect(!package.rooms.empty(), "room exists")) {
    return false;
  }
  const iggy3d::RoomAsset& room = package.rooms.front();
  const iggy3d::RoomAnchorAsset* player = findAnchor(room, "marker_player_spawn_r1_c1");
  if (!expect(player != nullptr, "player spawn anchor")) {
    return false;
  }

  const iggy3d::Vec3 offset{-player->positionMeters.x,
                            -player->positionMeters.y,
                            -player->positionMeters.z};
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room, offset);
  const iggy3d::CollisionQueryResult height =
      iggy3d::sampleSurfaceHeight(surfaces, {0.0F, 2.0F, 0.0F});
  const iggy3d::CollisionQueryResult stride =
      iggy3d::querySegment(surfaces,
                           {0.0F, -player->positionMeters.y, 0.0F},
                           {0.25F, -player->positionMeters.y, 0.0F},
                           iggy3d::CollisionQueryKind::Actor);
  return expect(height.status == iggy3d::CollisionQueryStatus::Hit,
                "offset height hit") &&
         expect(height.surfaceId == "floor_r1_c1_walkable", "offset floor id") &&
         expect(approx(height.heightMeters, -player->positionMeters.y),
                "offset floor height") &&
         expect(stride.status == iggy3d::CollisionQueryStatus::NoHit,
                "offset short stride unblocked");
}

bool emptyAndInvalidQueriesAreStable() {
  const iggy3d::PackageLoadResult package = loadAsciiPackage();
  if (!expect(package.status == iggy3d::PackageLoadStatus::Ok, "package load ok") ||
      !expect(!package.rooms.empty(), "room exists")) {
    return false;
  }
  const iggy3d::SpatialSurfaceSet empty;
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(package.rooms.front());
  const iggy3d::CollisionQueryResult emptyHeight =
      iggy3d::sampleSurfaceHeight(empty, {0.0F, 2.0F, 0.0F});
  const iggy3d::CollisionQueryResult invalidSegment =
      iggy3d::querySegment(surfaces,
                           {0.0F, 0.0F, 0.0F},
                           {0.0F, 0.0F, 0.0F},
                           iggy3d::CollisionQueryKind::Actor);
  return expect(emptyHeight.status == iggy3d::CollisionQueryStatus::EmptySurfaceSet,
                "empty height status") &&
         expect(emptyHeight.reasonCode == "collision_empty_surface_set",
                "empty height reason") &&
         expect(invalidSegment.status == iggy3d::CollisionQueryStatus::InvalidInput,
                "invalid segment status") &&
         expect(invalidSegment.reasonCode == "collision_invalid_input",
                "invalid segment reason");
}

}  // namespace

int main() {
  const bool ok = packageBuildsExpectedRuntimeSurfaces() &&
                  playerSpawnSamplesGroundAndNormal() &&
                  actorMovementQueriesUseAsciiWalls() &&
                  projectileQueriesUseAsciiProjectileWalls() &&
                  offsetSurfaceSetKeepsSpawnCentered() &&
                  emptyAndInvalidQueriesAreStable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

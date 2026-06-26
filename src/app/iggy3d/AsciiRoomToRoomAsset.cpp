#include "app/iggy3d/AsciiRoomToRoomAsset.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d {
namespace {

float segmentLength(const Vec3& start, const Vec3& end) {
  const float dx = end.x - start.x;
  const float dy = end.y - start.y;
  const float dz = end.z - start.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 0.0001F;
}

Vec3 midpoint(const SaveAuthoredRoomWallRecord& wall) {
  return {(wall.startMeters.x + wall.endMeters.x) / 2.0F,
          wall.bottomY + wall.heightMeters / 2.0F,
          (wall.startMeters.z + wall.endMeters.z) / 2.0F};
}

std::vector<Vec3> floorTopFacePoints(const SaveAuthoredRoomFloorRecord& floor) {
  const float halfX = floor.sizeMeters.x / 2.0F;
  const float halfZ = floor.sizeMeters.z / 2.0F;
  const float topY = floor.centerMeters.y + floor.sizeMeters.y / 2.0F;
  return {{floor.centerMeters.x - halfX, topY, floor.centerMeters.z - halfZ},
          {floor.centerMeters.x + halfX, topY, floor.centerMeters.z - halfZ},
          {floor.centerMeters.x + halfX, topY, floor.centerMeters.z + halfZ},
          {floor.centerMeters.x - halfX, topY, floor.centerMeters.z + halfZ}};
}

std::vector<Vec3> wallBoxPoints(const SaveAuthoredRoomWallRecord& wall) {
  const float halfThickness = wall.thicknessMeters / 2.0F;
  float minX = 0.0F;
  float maxX = 0.0F;
  float minZ = 0.0F;
  float maxZ = 0.0F;
  if (near(wall.startMeters.x, wall.endMeters.x) &&
      !near(wall.startMeters.z, wall.endMeters.z)) {
    const float centerX = (wall.startMeters.x + wall.endMeters.x) / 2.0F;
    minX = centerX - halfThickness;
    maxX = centerX + halfThickness;
    minZ = std::min(wall.startMeters.z, wall.endMeters.z);
    maxZ = std::max(wall.startMeters.z, wall.endMeters.z);
  } else {
    minX = std::min(wall.startMeters.x, wall.endMeters.x);
    maxX = std::max(wall.startMeters.x, wall.endMeters.x);
    const float centerZ = (wall.startMeters.z + wall.endMeters.z) / 2.0F;
    minZ = centerZ - halfThickness;
    maxZ = centerZ + halfThickness;
  }
  const float minY = wall.bottomY;
  const float maxY = wall.bottomY + wall.heightMeters;
  return {{minX, minY, minZ}, {maxX, minY, minZ}, {maxX, minY, maxZ},
          {minX, minY, maxZ}, {minX, maxY, minZ}, {maxX, maxY, minZ},
          {maxX, maxY, maxZ}, {minX, maxY, maxZ}};
}

std::vector<std::string> actorBlockerTraversalTags(
    const SaveAuthoredRoomWallRecord& wall) {
  std::vector<std::string> tags{"blocker"};
  for (const std::string& tag : wall.semantics.traversalTags) {
    if (std::find(tags.begin(), tags.end(), tag) == tags.end()) {
      tags.push_back(tag);
    }
  }
  return tags;
}

RoomStaticMeshAsset floorMesh(const SaveAuthoredRoomFloorRecord& floor,
                              const AsciiRoomToRoomAssetConfig& config) {
  RoomStaticMeshAsset mesh;
  mesh.id = floor.id;
  mesh.meshId = config.floorMeshId;
  mesh.materialId = floor.semantics.materialId;
  mesh.role = config.floorRole;
  mesh.positionMeters = floor.centerMeters;
  mesh.sizeMeters = floor.sizeMeters;
  return mesh;
}

RoomStaticMeshAsset wallMesh(const SaveAuthoredRoomWallRecord& wall,
                             const AsciiRoomToRoomAssetConfig& config) {
  RoomStaticMeshAsset mesh;
  mesh.id = wall.id;
  mesh.meshId = config.wallMeshId;
  mesh.materialId = wall.semantics.materialId;
  mesh.role = config.wallRole;
  mesh.positionMeters = midpoint(wall);
  mesh.sizeMeters = {segmentLength(wall.startMeters, wall.endMeters),
                     wall.heightMeters,
                     wall.thicknessMeters};
  mesh.hasWallSegment = true;
  mesh.wallStartMeters = wall.startMeters;
  mesh.wallEndMeters = wall.endMeters;
  mesh.wallBottomY = wall.bottomY;
  mesh.wallHeightMeters = wall.heightMeters;
  mesh.wallThicknessMeters = wall.thicknessMeters;
  return mesh;
}

bool markerIsDoor(const AsciiRoomMarker& marker) {
  return marker.tag == "door" || marker.tag == "secret_door";
}

RoomStaticMeshAsset doorMesh(const AsciiRoomMarker& marker,
                             const AsciiRoomToRoomAssetConfig& config) {
  RoomStaticMeshAsset mesh;
  mesh.id = marker.id + "_panel";
  mesh.meshId = config.doorMeshId;
  mesh.materialId = "debug_door";
  mesh.role = config.doorRole;
  mesh.positionMeters = {static_cast<float>(marker.worldPosition.x),
                         config.wallHeightMeters / 2.0F,
                         static_cast<float>(marker.worldPosition.z)};
  mesh.sizeMeters = {config.tileSizeMeters,
                     config.wallHeightMeters,
                     config.tileSizeMeters};
  return mesh;
}

const AsciiRoomTerrainSurface* terrainForFloor(
    const AsciiRoomAuthoredRoomResult& authored,
    std::string_view floorId) {
  for (const AsciiRoomTerrainSurface& terrain : authored.terrainSurfaces) {
    if (terrain.floorId == floorId) {
      return &terrain;
    }
  }
  return nullptr;
}

RoomSpatialSurface walkableSurface(const SaveAuthoredRoomFloorRecord& floor,
                                   const AsciiRoomTerrainSurface* terrain) {
  RoomSpatialSurface surface;
  surface.id = floor.id + "_walkable";
  surface.sourceStaticMeshId = floor.id;
  surface.shape = RoomSpatialSurfaceShape::Plane;
  surface.role = RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters =
      terrain != nullptr && !terrain->topFacePoints.empty() ? terrain->topFacePoints
                                                            : floorTopFacePoints(floor);
  surface.normal = terrain != nullptr ? terrain->normal : Vec3{0.0F, 1.0F, 0.0F};
  surface.traversalTags = floor.semantics.traversalTags;
  surface.collisionMask = {"actor"};
  surface.blocksActor = false;
  surface.blocksProjectile = false;
  return surface;
}

RoomSpatialSurface actorBlockerSurface(const SaveAuthoredRoomWallRecord& wall) {
  RoomSpatialSurface surface;
  surface.id = wall.id + "_actor_blocker";
  surface.sourceStaticMeshId = wall.id;
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = wallBoxPoints(wall);
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = actorBlockerTraversalTags(wall);
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  surface.blocksProjectile = false;
  return surface;
}

RoomSpatialSurface projectileBlockerSurface(const SaveAuthoredRoomWallRecord& wall) {
  RoomSpatialSurface surface;
  surface.id = wall.id + "_projectile_blocker";
  surface.sourceStaticMeshId = wall.id;
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = RoomSpatialSurfaceRole::ProjectileBlocker;
  surface.pointsMeters = wallBoxPoints(wall);
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"projectile_blocker"};
  surface.collisionMask = {"projectile"};
  surface.blocksActor = false;
  surface.blocksProjectile = true;
  return surface;
}

std::vector<Vec3> doorBoxPoints(const AsciiRoomMarker& marker,
                                const AsciiRoomToRoomAssetConfig& config) {
  const float centerX = static_cast<float>(marker.worldPosition.x);
  const float centerZ = static_cast<float>(marker.worldPosition.z);
  const float halfTile = config.tileSizeMeters / 2.0F;
  const float minX = centerX - halfTile;
  const float maxX = centerX + halfTile;
  const float minZ = centerZ - halfTile;
  const float maxZ = centerZ + halfTile;
  const float minY = 0.0F;
  const float maxY = config.wallHeightMeters;
  return {{minX, minY, minZ}, {maxX, minY, minZ}, {maxX, minY, maxZ},
          {minX, minY, maxZ}, {minX, maxY, minZ}, {maxX, maxY, minZ},
          {maxX, maxY, maxZ}, {minX, maxY, maxZ}};
}

RoomSpatialSurface doorBlockerSurface(
    const AsciiRoomMarker& marker,
    const AsciiRoomToRoomAssetConfig& config) {
  RoomSpatialSurface surface;
  surface.id = marker.id + "_door_blocker";
  surface.sourceStaticMeshId = marker.id + "_panel";
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = doorBoxPoints(marker, config);
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"blocker"};
  surface.collisionMask = {"actor", "projectile"};
  surface.blocksActor = true;
  surface.blocksProjectile = true;
  surface.runtimeOwnerStableName = marker.id;
  return surface;
}

std::string anchorKindForMarkerTag(std::string_view tag) {
  if (tag == "player_spawn") {
    return "spawn";
  }
  if (tag == "npc_spawn" || tag == "monster_spawn") {
    return "npc";
  }
  if (tag == "treasure") {
    return "treasure";
  }
  if (tag == "key") {
    return "key";
  }
  if (tag == "trap") {
    return "trap";
  }
  if (tag == "exit") {
    return "exit";
  }
  if (tag == "door") {
    return "door";
  }
  if (tag == "secret_door") {
    return "secret_door";
  }
  return "marker";
}

RoomAnchorAsset anchorFromMarker(const AsciiRoomMarker& marker) {
  RoomAnchorAsset anchor;
  anchor.id = marker.id;
  anchor.kind = anchorKindForMarkerTag(marker.tag);
  anchor.runtimeStableName = marker.id;
  anchor.positionMeters = {static_cast<float>(marker.worldPosition.x),
                           static_cast<float>(marker.worldPosition.y),
                           static_cast<float>(marker.worldPosition.z)};
  return anchor;
}

bool floorIsWalkable(const SaveAuthoredRoomFloorRecord& floor) {
  return floor.semantics.walkable;
}

bool wallBlocksActor(const SaveAuthoredRoomWallRecord& wall) {
  return wall.semantics.blocksActor;
}

bool wallBlocksProjectile(const SaveAuthoredRoomWallRecord& wall) {
  return wall.semantics.blocksProjectile;
}

AsciiRoomMarker markerFromSavedRecord(
    const SaveAuthoredRoomMarkerRecord& record) {
  AsciiRoomMarker marker;
  marker.id = record.id;
  marker.tag = record.tag;
  marker.glyph = record.glyph.empty() ? '\0' : record.glyph.front();
  marker.row = record.row;
  marker.column = record.column;
  marker.worldPosition = {record.positionMeters.x,
                          record.positionMeters.y,
                          record.positionMeters.z};
  marker.sourceLine = record.sourceLine;
  marker.sourceColumn = record.sourceColumn;
  return marker;
}

std::vector<AsciiRoomMarker> markersForRoomAsset(
    const AsciiRoomAuthoredRoomResult& authored) {
  if (!authored.markers.empty()) {
    return authored.markers;
  }

  std::vector<AsciiRoomMarker> markers;
  markers.reserve(authored.authoredRoom.markers.size());
  for (const SaveAuthoredRoomMarkerRecord& marker : authored.authoredRoom.markers) {
    markers.push_back(markerFromSavedRecord(marker));
  }
  return markers;
}

}  // namespace

AsciiRoomToRoomAssetResult buildRoomAssetFromAsciiRoom(
    const AsciiRoomAuthoredRoomResult& authored,
    const AsciiRoomToRoomAssetConfig& config) {
  AsciiRoomToRoomAssetResult result;
  if (!authored.ok) {
    result.status = authored.status;
    result.reasonCode = authored.reasonCode;
    result.diagnostics = authored.diagnostics;
    return result;
  }
  if (!authored.authoredRoom.present) {
    result.status = "ascii_room_asset_missing_authored_room";
    result.reasonCode = result.status;
    return result;
  }

  result.room.id = config.roomId.empty() ? authored.authoredRoom.id : config.roomId;
  result.room.version = 1;
  result.room.units = config.units;
  result.room.source = config.source;
  if (!config.sourceName.empty()) {
    result.room.sourceFile = config.sourceName;
  } else if (!authored.authoredRoom.sourceFile.empty()) {
    result.room.sourceFile = authored.authoredRoom.sourceFile;
  } else {
    result.room.sourceFile = "ascii_room";
  }
  result.room.sourceSubset = config.sourceSubset;

  for (const SaveAuthoredRoomFloorRecord& floor : authored.authoredRoom.floors) {
    result.room.staticMeshes.push_back(floorMesh(floor, config));
    if (floorIsWalkable(floor)) {
      result.room.spatialSurfaces.push_back(
          walkableSurface(floor, terrainForFloor(authored, floor.id)));
      ++result.walkableSurfaceCount;
    }
  }

  for (const SaveAuthoredRoomWallRecord& wall : authored.authoredRoom.walls) {
    result.room.staticMeshes.push_back(wallMesh(wall, config));
    if (wallBlocksActor(wall)) {
      result.room.spatialSurfaces.push_back(actorBlockerSurface(wall));
      ++result.actorBlockerSurfaceCount;
    }
    if (wallBlocksProjectile(wall)) {
      result.room.spatialSurfaces.push_back(projectileBlockerSurface(wall));
      ++result.projectileBlockerSurfaceCount;
    }
  }

  const std::vector<AsciiRoomMarker> markers = markersForRoomAsset(authored);
  for (const AsciiRoomMarker& marker : markers) {
    if (markerIsDoor(marker)) {
      result.room.staticMeshes.push_back(doorMesh(marker, config));
      result.room.spatialSurfaces.push_back(
          doorBlockerSurface(marker, config));
      ++result.actorBlockerSurfaceCount;
      ++result.projectileBlockerSurfaceCount;
    }
  }

  for (const AsciiRoomMarker& marker : markers) {
    result.room.anchors.push_back(anchorFromMarker(marker));
  }

  result.staticMeshCount = result.room.staticMeshes.size();
  result.anchorCount = result.room.anchors.size();
  result.spatialSurfaceCount = result.room.spatialSurfaces.size();
  result.ok = true;
  result.status = "ascii_room_ok";
  result.reasonCode = "ascii_room_ok";
  return result;
}

}  // namespace iggy3d

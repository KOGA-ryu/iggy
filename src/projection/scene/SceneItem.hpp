#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/interaction/InteractionDefinition.hpp"
#include "runtime/player/PlayerSlot.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {

enum class SceneItemKind : std::uint8_t {
  Player,
  Npc,
  Pickup,
  Interactable,
  ObjectiveMarker,
  TacticalMarker,
  DebugOnly,
};

struct SceneItem {
  EntityId entityId;
  std::string stableName;
  SceneItemKind kind = SceneItemKind::DebugOnly;
  EntityKind entityKind = EntityKind::Unknown;
  Transform3 transform;
  Aabb3 worldBounds;
  bool active = false;
  bool visible = false;
  bool targetable = false;
  bool interactable = false;
  bool tactical = false;
  std::string assetRef;
  std::string modelRef;
  std::string itemId;
  std::string objectiveId;
  InteractionKind interactionKind = InteractionKind::None;
  PlayerSlotId owningPlayerSlot = kInvalidPlayerSlotId;
};

struct SceneRoomMeshItem {
  std::string id;
  std::string meshId;
  std::string role;
  std::string materialId;
  Vec3 position;
  Vec3 size;
  Vec3 rotationEulerRadians;
  std::uint16_t proceduralSegmentCount = 0;
  bool hasWallSegment = false;
  Vec3 wallStartMeters;
  Vec3 wallEndMeters;
  float wallBottomY = 0.0F;
  float wallHeightMeters = 0.0F;
  float wallThicknessMeters = 0.0F;
};

struct SceneRoomSurfacePatchItem {
  std::string role;
  Vec3 center;
  std::array<Vec3, 4U> corners{};
};

struct SceneRoomProjection {
  bool loaded = false;
  std::string assetId;
  std::uint32_t version = 0;
  std::string sourceToml;
  std::string sourceSubset;
  std::size_t staticMeshCount = 0;
  std::size_t materialCount = 0;
  std::size_t anchorCount = 0;
  bool floorVisible = false;
  bool wallVisible = false;
  bool openingVisible = false;
  bool propVisible = false;
  bool keyAnchorVisible = false;
  bool dummyAnchorVisible = false;
  std::vector<SceneRoomMeshItem> meshes;
  std::vector<SceneRoomSurfacePatchItem> surfacePatches;
};

struct SceneProjectileItem {
  std::string id;
  Vec3 positionMeters;
  Vec3 previousPositionMeters;
  Vec3 impactPointMeters;
  Vec3 impactNormal;
  bool active = false;
  bool impact = false;
  std::string hitSurfaceId;
};

}  // namespace iggy3d

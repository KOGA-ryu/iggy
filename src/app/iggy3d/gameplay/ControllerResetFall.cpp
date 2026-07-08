#include "app/iggy3d/gameplay/ControllerResetFall.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ControllerGroundQueries.hpp"
#include "app/iggy3d/gameplay/ControllerJumpDashState.hpp"
#include "app/iggy3d/gameplay/ControllerPlayerAccess.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "runtime/world/EntityState.hpp"

#include <cmath>
#include <string>

namespace iggy3d {
namespace {

constexpr float kGameplayResetBelowLowestFloorMeters = 6.0F;
constexpr float kGameplayResetZoneRadiusMeters = 0.70F;
constexpr float kGameplayResetZoneVerticalToleranceMeters = 1.20F;

float horizontalDistanceSquared(Vec3 lhs, Vec3 rhs) {
  const float dx = lhs.x - rhs.x;
  const float dz = lhs.z - rhs.z;
  return dx * dx + dz * dz;
}

const RoomAnchorAsset* findRoomAnchorByKind(const ProductAppWindowState& window,
                                            std::string_view kind) {
  for (const RoomAnchorAsset& anchor : activeRoom(window).room.anchors) {
    // branch-gate: BG-1175
    if (anchor.kind == kind) {
      return &anchor;
    }
  }
  return nullptr;
}

const RoomAnchorAsset* findResetZoneAt(const ProductAppWindowState& window,
                                       Vec3 position) {
  const float radiusSq =
      kGameplayResetZoneRadiusMeters * kGameplayResetZoneRadiusMeters;
  for (const RoomAnchorAsset& anchor : activeRoom(window).room.anchors) {
    // branch-gate: BG-1179
    if (anchor.kind != "reset_zone") {
      continue;
    }
    // branch-gate: BG-1179
    if (horizontalDistanceSquared(position, anchor.positionMeters) > radiusSq ||
        std::fabs(position.y - anchor.positionMeters.y) >
            kGameplayResetZoneVerticalToleranceMeters) {
      continue;
    }
    return &anchor;
  }
  return nullptr;
}

void recordProductGameplayReset(ProductAppWindowState& window,
                                std::string_view reason,
                                const RoomAnchorAsset& spawn,
                                const RoomAnchorAsset* source,
                                float startY,
                                float finalY) {
  window.gameplay.gameplayReset.triggered = true;
  window.gameplay.gameplayReset.status = "reset";
  window.gameplay.gameplayReset.reasonCode = std::string(reason);
  // branch-gate: BG-1185
  window.gameplay.gameplayReset.spawnAnchorId = spawn.id.empty() ? "spawn" : spawn.id;
  // branch-gate: BG-1186
  window.gameplay.gameplayReset.sourceAnchorId =
      source == nullptr || source->id.empty() ? "none" : source->id;
  window.gameplay.gameplayReset.startY = startY;
  window.gameplay.gameplayReset.finalY = finalY;
}

}  // namespace

bool resetProductPlayerToSpawn(Session& session,
                               ProductAppWindowState& window,
                               std::string_view reason,
                               const RoomAnchorAsset* source) {
  const EntityId actor = productPlayerActor(session);
  const EntityState* entity = productPlayerEntity(session);
  const RoomAnchorAsset* spawn = findRoomAnchorByKind(window, "spawn");
  // branch-gate: BG-1180
  if (entity == nullptr || spawn == nullptr) {
    return false;
  }
  const float startY = entity->transform.position.y;
  // branch-gate: BG-1180
  if (!setProductPlayerPosition(session, actor, spawn->positionMeters)) {
    return false;
  }
  recordProductGameplayReset(
      window, reason, *spawn, source, startY, spawn->positionMeters.y);
  window.gameplay.gameplayJump.active = false;
  window.gameplay.gameplayJump.velocityMetersPerSecond = 0.0F;
  clearProductJumpTiming(window);
  window.gameplay.gameplayJump.status = "reset";
  window.gameplay.gameplayJump.reasonCode = std::string(reason);
  window.gameplay.playerPositionChanged = true;
  return true;
}

bool applyProductGameplayResetIfNeeded(Session& session,
                                       ProductAppWindowState& window,
                                       const SpatialSurfaceSet* surfaces) {
  const EntityState* entity = productPlayerEntity(session);
  // branch-gate: BG-1181
  if (entity == nullptr || !activeRoom(window).loaded) {
    return false;
  }

  const RoomAnchorAsset* resetZone =
      findResetZoneAt(window, entity->transform.position);
  // branch-gate: BG-1179
  if (resetZone != nullptr) {
    return resetProductPlayerToSpawn(
        session, window, "gameplay_reset_zone", resetZone);
  }

  float lowestFloorY = 0.0F;
  // branch-gate: BG-1176
  if (!findLowestWalkableFloorY(surfaces, lowestFloorY)) {
    return false;
  }
  // branch-gate: BG-1181
  if (entity->transform.position.y <
      lowestFloorY - kGameplayResetBelowLowestFloorMeters) {
    return resetProductPlayerToSpawn(
        session, window, "gameplay_reset_fall_out", nullptr);
  }
  return false;
}

bool beginProductFallIfUnsupported(Session& session,
                                   ProductAppWindowState& window,
                                   const SpatialSurfaceSet* collisionSurfaces) {
  const EntityState* groundedEntity = productPlayerEntity(session);
  // branch-gate: BG-1173
  if (groundedEntity == nullptr ||
      collisionSurfaces == nullptr ||
      playerHasNearbyGround(collisionSurfaces, groundedEntity->transform.position)) {
    return false;
  }
  window.gameplay.gameplayJump.requested = false;
  window.gameplay.gameplayJump.accepted = false;
  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond = 0.0F;
  window.gameplay.gameplayJump.coyoteSecondsRemaining =
      window.gameplay.gameplayMovement.tuning.coyoteTimeSeconds;
  window.gameplay.gameplayJump.cutApplied = false;
  window.gameplay.gameplayJump.held = false;
  float groundY = groundedEntity->transform.position.y;
  findHighestWalkableGroundAtOrBelow(
      collisionSurfaces,
      groundedEntity->transform.position,
      groundedEntity->transform.position.y,
      groundY);
  recordProductJumpPosition(window,
                            groundY,
                            groundedEntity->transform.position.y,
                            groundedEntity->transform.position.y);
  window.gameplay.gameplayJump.status = "falling";
  window.gameplay.gameplayJump.reasonCode = "gameplay_jump_falling";
  return true;
}

}  // namespace iggy3d

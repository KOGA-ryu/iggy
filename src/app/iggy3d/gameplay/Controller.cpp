#include "app/iggy3d/gameplay/Controller.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>

#include "app/input/ActionState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/movement/MovementTraversal.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kGameplayGroundFootprintToleranceMeters = 0.35F;
constexpr float kGameplayGroundContactToleranceMeters = 0.12F;
constexpr float kGameplayResetBelowLowestFloorMeters = 6.0F;
constexpr float kGameplayResetZoneRadiusMeters = 0.70F;
constexpr float kGameplayResetZoneVerticalToleranceMeters = 1.20F;
constexpr float kMovementStateSpeedEpsilonMetersPerSecond = 0.001F;
constexpr float kMovementStateDistanceEpsilonMeters = 0.0001F;
constexpr float kWallRunSurfaceVerticalSlackMeters = 0.35F;
constexpr float kWallRunAlongWallDotThreshold = 0.35F;

struct ProductInteractionOutcomeSnapshot {
  EntityId target;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::string itemId;
  std::string objectiveId;
  std::uint32_t itemCountBefore = 0;
  bool objectiveCompleteBefore = false;
  std::size_t eventCountBefore = 0;
};

std::string commandKindName(CommandKind kind) {
  switch (kind) {
    case CommandKind::Move:
      return "move";
    case CommandKind::Interact:
      return "interact";
    case CommandKind::Attack:
      return "attack";
    case CommandKind::Retry:
      return "retry";
    case CommandKind::Reset:
      return "reset";
    case CommandKind::None:
      return "none";
    default:
      break;
  }
  return "other";
}

std::string targetQueryStatusName(TargetQueryStatus status) {
  switch (status) {
    case TargetQueryStatus::Found:
      return "found";
    case TargetQueryStatus::NotFound:
      return "not_found";
    case TargetQueryStatus::InvalidWorld:
      return "invalid_world";
    case TargetQueryStatus::InvalidActor:
      return "invalid_actor";
    case TargetQueryStatus::InvalidOrigin:
      return "invalid_origin";
  }
  return "unknown";
}

std::string entityKindName(EntityKind kind) {
  switch (kind) {
    case EntityKind::Player:
      return "player";
    case EntityKind::Pickup:
      return "pickup";
    case EntityKind::Door:
      return "door";
    case EntityKind::Marker:
      return "marker";
    case EntityKind::Npc:
      return "npc";
    case EntityKind::Unknown:
      break;
  }
  return "unknown";
}

std::string commandRejectionReasonName(CommandRejectionReason reason) {
  switch (reason) {
    case CommandRejectionReason::None:
      return "none";
    case CommandRejectionReason::InvalidTarget:
      return "invalid_target";
    case CommandRejectionReason::OutOfRange:
      return "out_of_range";
    case CommandRejectionReason::TargetNotReachable:
      return "target_not_reachable";
    case CommandRejectionReason::InvalidDamage:
      return "invalid_damage";
    case CommandRejectionReason::TargetDefeated:
      return "target_defeated";
    case CommandRejectionReason::FriendlyFireBlocked:
      return "friendly_fire_blocked";
    case CommandRejectionReason::InvalidActor:
      return "invalid_actor";
    case CommandRejectionReason::RequiredItemMissing:
      return "required_item_missing";
    default:
      break;
  }
  return "rejected";
}

std::string reachGateName(CommandRejectionReason reason) {
  if (reason == CommandRejectionReason::None) {
    return "pass";
  }
  if (reason == CommandRejectionReason::OutOfRange ||
      reason == CommandRejectionReason::TargetNotReachable) {
    return "fail";
  }
  return "not_attempted";
}

EntityId productPlayerActor(const Session& session) {
  return session.state().players.actorForSlot(0);
}

const EntityState* productPlayerEntity(const Session& session) {
  const EntityId actor = productPlayerActor(session);
  return session.state().world.findById(actor);
}

void clearProductTargetProof(ProductAppWindowState& window);
void clearProductOutcomeProof(ProductAppWindowState& window);
void submitProductGameplayCommand(Session& session,
                                  ProductAppWindowState& window,
                                  CommandRecord command,
                                  const SpatialSurfaceSet* collisionSurfaces);
Vec3 manualFirstPersonDirection(float moveX, float moveY, float yawDegrees);
void advanceProductJump(Session& session,
                        ProductAppWindowState& window,
                        const SpatialSurfaceSet* collisionSurfaces);
void recordProductJumpPosition(ProductAppWindowState& window,
                               float groundY,
                               float startY,
                               float finalY);
void clearProductJumpTiming(ProductAppWindowState& window);

TargetQueryResult queryProductGameplayTarget(const Session& session, CommandKind kind) {
  const EntityId actor = productPlayerActor(session);
  return queryTarget(TargetQueryRequest{&session.state().world, actor, false, {},
                                        kind, 0.0F, false, true});
}

void clearProductMovementDebug(ProductAppWindowState& window) {
  window.gameplayMovementDebugAvailable = false;
  window.gameplayMovementReasonCode = "not_requested";
  window.gameplayMovementBlockedReason = "none";
  window.gameplayMovementHitSurfaceId = "none";
  window.gameplayMovementGroundSnapApplied = false;
  window.gameplayMovementClamped = false;
  window.gameplayMovementSlid = false;
  window.gameplayMovementCollisionSweepCount = 0;
  window.gameplayMovementPolicyBand = "none";
  window.gameplayMovementSlopeTravelDirection = "stationary";
  window.gameplayMovementSlopeAngleDegrees = 0.0F;
  window.gameplayMovementSpeedMultiplier = 1.0F;
  window.gameplayMovementStartX = 0.0F;
  window.gameplayMovementStartY = 0.0F;
  window.gameplayMovementStartZ = 0.0F;
  window.gameplayMovementFinalX = 0.0F;
  window.gameplayMovementFinalY = 0.0F;
  window.gameplayMovementFinalZ = 0.0F;
  window.gameplayMovementHorizontalDistanceMeters = 0.0F;
  window.gameplayMovementVerticalDeltaMeters = 0.0F;
  window.gameplayMovementGradePercent = 0.0F;
}

float productHorizontalMovementSpeedMetersPerSecond(
    const ProductAppWindowState& window) {
  const float speedSquared =
      window.gameplayMovementGroundVelocityX *
          window.gameplayMovementGroundVelocityX +
      window.gameplayMovementGroundVelocityZ *
          window.gameplayMovementGroundVelocityZ;
  const float retainedSpeed = std::sqrt(std::max(0.0F, speedSquared));
  const float dt = std::max(0.0F, window.gameplayMovementTuning.inputStepSeconds);
  // branch-gate: BG-1161
  const float debugSpeed =
      dt > 0.0F ? window.gameplayMovementHorizontalDistanceMeters / dt : 0.0F;
  return std::max(retainedSpeed, debugSpeed);
}

void clearProductWallRunCandidateProof(ProductAppWindowState& window,
                                       std::string_view reason) {
  window.gameplayWallRunCandidateAvailable = false;
  window.gameplayWallRunCandidateStatus = std::string{reason};
  window.gameplayWallRunCandidateReasonCode = std::string{reason};
  window.gameplayWallRunSide = "none";
  window.gameplayWallRunSurfaceId = "none";
  window.gameplayWallRunNormalX = 0.0F;
  window.gameplayWallRunNormalY = 0.0F;
  window.gameplayWallRunNormalZ = 0.0F;
  window.gameplayWallRunApproachSpeedMetersPerSecond = 0.0F;
}

void clearProductWallRunActiveProof(ProductAppWindowState& window,
                                    std::string_view reason) {
  window.gameplayWallRunActive = false;
  window.gameplayWallRunStatus = std::string{reason};
  window.gameplayWallRunReasonCode = std::string{reason};
  window.gameplayWallRunRemainingSeconds = 0.0F;
  window.gameplayWallRunDurationSeconds = 0.0F;
  window.gameplayWallRunGravityMultiplier = 1.0F;
  window.gameplayWallRunSpeedMultiplier = 1.0F;
}

void updateProductMovementStateProof(ProductAppWindowState& window) {
  window.gameplayMovementHorizontalSpeedMetersPerSecond =
      productHorizontalMovementSpeedMetersPerSecond(window);
  window.gameplayMovementGrounded = !window.gameplayJumpActive;

  const bool blockedOrSliding =
      window.gameplayMovementBlocked ||
      window.gameplayMovementClamped ||
      window.gameplayMovementSlid ||
      (window.gameplayMovementBlockedReason != "none" &&
       window.gameplayMovementBlockedReason != "movement_ok");
  // branch-gate: BG-1161
  if (blockedOrSliding) {
    window.gameplayMovementState =
        ProductGameplayMovementState::BlockedOrSliding;
    return;
  }

  // branch-gate: BG-1153
  if (window.gameplayJumpActive) {
    // branch-gate: BG-1157
    if (window.gameplayWallRunActive) {
      window.gameplayMovementState = ProductGameplayMovementState::WallRunning;
      return;
    }
    // branch-gate: BG-1161
    if (window.gameplayMovementReasonCode == "airborne_manual_move" &&
        window.gameplayMovementHorizontalDistanceMeters >
            kMovementStateDistanceEpsilonMeters) {
      window.gameplayMovementState =
          ProductGameplayMovementState::AirborneControl;
      return;
    }
    // branch-gate: BG-1153
    if (window.gameplayJumpStatus == "accepted") {
      window.gameplayMovementState = ProductGameplayMovementState::Jumping;
      return;
    }
    // branch-gate: BG-1153
    window.gameplayMovementState =
        window.gameplayJumpVelocityMetersPerSecond > 0.0F
            ? ProductGameplayMovementState::Rising
            : ProductGameplayMovementState::Falling;
    return;
  }

  const bool horizontalVelocityActive =
      window.gameplayMovementHorizontalSpeedMetersPerSecond >
      kMovementStateSpeedEpsilonMetersPerSecond;
  const bool movementDebugMoved =
      window.gameplayMovementStatus == "moved" &&
      window.gameplayMovementHorizontalDistanceMeters >
          kMovementStateDistanceEpsilonMeters;
  // branch-gate: BG-1161
  window.gameplayMovementState =
      horizontalVelocityActive || movementDebugMoved
          ? ProductGameplayMovementState::MovingGrounded
          : ProductGameplayMovementState::IdleGrounded;
}

float manualFirstPersonMaxSpeedMetersPerSecond(
    const ProductGameplayMovementTuning& tuning,
    bool sprinting) {
  const std::array<float, 2U> speeds{tuning.walkSpeedMetersPerSecond,
                                     tuning.sprintSpeedMetersPerSecond};
  return speeds[static_cast<std::size_t>(sprinting)];
}

std::string_view manualFirstPersonMovementProfile(
    const ProductGameplayMovementTuning& tuning,
    bool sprinting) {
  const std::array<std::string_view, 2U> profiles{tuning.walkProfile,
                                                  tuning.sprintProfile};
  return profiles[static_cast<std::size_t>(sprinting)];
}

void recordProductMovementProfile(ProductAppWindowState& window, bool sprinting) {
  window.gameplayMovementProfile =
      std::string{manualFirstPersonMovementProfile(window.gameplayMovementTuning,
                                                   sprinting)};
  window.gameplayMovementMaxSpeedMetersPerSecond =
      manualFirstPersonMaxSpeedMetersPerSecond(window.gameplayMovementTuning,
                                               sprinting);
}

bool setProductPlayerPosition(Session& session, EntityId actor, const Vec3& position) {
  SessionState& state = session.mutableStateForOwnedSystems();
  const EntityState* entity = state.world.findById(actor);
  // branch-gate: BG-1153
  if (entity == nullptr) {
    return false;
  }
  Transform3 transform = entity->transform;
  transform.position = position;
  const WorldMutationResult mutation = state.world.updateTransform(actor, transform);
  // branch-gate: BG-1153
  if (mutation.status != WorldStatus::Ok) {
    return false;
  }
  state.currentStateHash = computeStateHash(state);
  return true;
}

bool surfaceContainsXZ(const CollisionSurfaceView& surface,
                       Vec3 position,
                       float toleranceMeters) {
  return position.x >= surface.bounds.min.x - toleranceMeters &&
         position.x <= surface.bounds.max.x + toleranceMeters &&
         position.z >= surface.bounds.min.z - toleranceMeters &&
         position.z <= surface.bounds.max.z + toleranceMeters;
}

bool walkableSurfaceHeightAt(const CollisionSurfaceView& surface,
                             Vec3 position,
                             float& heightMeters) {
  // branch-gate: BG-1169
  if (surface.role != CollisionSurfaceRole::Walkable ||
      std::fabs(surface.normal.y) <= 0.0001F ||
      !surfaceContainsXZ(surface, position, kGameplayGroundFootprintToleranceMeters)) {
    return false;
  }

  const float height =
      surface.planePoint.y -
      ((surface.normal.x * (position.x - surface.planePoint.x)) +
       (surface.normal.z * (position.z - surface.planePoint.z))) /
          surface.normal.y;
  // branch-gate: BG-1170
  if (!std::isfinite(height)) {
    return false;
  }
  heightMeters = height;
  return true;
}

bool findHighestWalkableGroundAtOrBelow(const SpatialSurfaceSet* surfaces,
                                        Vec3 position,
                                        float maxY,
                                        float& groundY) {
  // branch-gate: BG-1171
  if (surfaces == nullptr) {
    return false;
  }

  bool found = false;
  float bestY = 0.0F;
  for (const CollisionSurfaceView& surface : surfaces->surfaces()) {
    float height = 0.0F;
    // branch-gate: BG-1169
    if (!walkableSurfaceHeightAt(surface, position, height) ||
        height > maxY + kGameplayGroundContactToleranceMeters) {
      continue;
    }
    // branch-gate: BG-1172
    if (!found || height > bestY) {
      found = true;
      bestY = height;
    }
  }
  // branch-gate: BG-1171
  if (!found) {
    return false;
  }
  groundY = bestY;
  return true;
}

bool playerHasNearbyGround(const SpatialSurfaceSet* surfaces, Vec3 position) {
  float groundY = 0.0F;
  return findHighestWalkableGroundAtOrBelow(
             surfaces,
             position,
             position.y + kGameplayGroundContactToleranceMeters,
             groundY) &&
         std::fabs(position.y - groundY) <= kGameplayGroundContactToleranceMeters;
}

float horizontalDistanceSquared(Vec3 lhs, Vec3 rhs) {
  const float dx = lhs.x - rhs.x;
  const float dz = lhs.z - rhs.z;
  return dx * dx + dz * dz;
}

const RoomAnchorAsset* findRoomAnchorByKind(const ProductAppWindowState& window,
                                            std::string_view kind) {
  for (const RoomAnchorAsset& anchor : window.activeRoom.room.anchors) {
    // branch-gate: BG-1175
    if (anchor.kind == kind) {
      return &anchor;
    }
  }
  return nullptr;
}

bool findLowestWalkableFloorY(const SpatialSurfaceSet* surfaces, float& floorY) {
  // branch-gate: BG-1183
  if (surfaces == nullptr) {
    return false;
  }

  bool found = false;
  float lowest = 0.0F;
  for (const CollisionSurfaceView& surface : surfaces->surfaces()) {
    // branch-gate: BG-1176
    if (surface.role != CollisionSurfaceRole::Walkable ||
        std::fabs(surface.normal.y) <= 0.0001F) {
      continue;
    }
    const float candidate = surface.planePoint.y;
    // branch-gate: BG-1177
    if (!std::isfinite(candidate)) {
      continue;
    }
    // branch-gate: BG-1178
    if (!found || candidate < lowest) {
      found = true;
      lowest = candidate;
    }
  }
  // branch-gate: BG-1184
  if (!found) {
    return false;
  }
  floorY = lowest;
  return true;
}

const RoomAnchorAsset* findResetZoneAt(const ProductAppWindowState& window,
                                       Vec3 position) {
  const float radiusSq =
      kGameplayResetZoneRadiusMeters * kGameplayResetZoneRadiusMeters;
  for (const RoomAnchorAsset& anchor : window.activeRoom.room.anchors) {
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
  window.gameplayResetTriggered = true;
  window.gameplayResetStatus = "reset";
  window.gameplayResetReasonCode = std::string(reason);
  // branch-gate: BG-1185
  window.gameplayResetSpawnAnchorId = spawn.id.empty() ? "spawn" : spawn.id;
  // branch-gate: BG-1186
  window.gameplayResetSourceAnchorId =
      source == nullptr || source->id.empty() ? "none" : source->id;
  window.gameplayResetStartY = startY;
  window.gameplayResetFinalY = finalY;
}

bool resetProductPlayerToSpawn(Session& session,
                               ProductAppWindowState& window,
                               std::string_view reason,
                               const RoomAnchorAsset* source) {
  const EntityId actor = productPlayerActor(session);
  const EntityState* entity = session.state().world.findById(actor);
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
  window.gameplayJumpActive = false;
  window.gameplayJumpVelocityMetersPerSecond = 0.0F;
  clearProductJumpTiming(window);
  window.gameplayJumpStatus = "reset";
  window.gameplayJumpReasonCode = std::string(reason);
  window.playerPositionChanged = true;
  window.runtimeStateHash = session.stateHash();
  return true;
}

bool applyProductGameplayResetIfNeeded(Session& session,
                                       ProductAppWindowState& window,
                                       const SpatialSurfaceSet* surfaces) {
  const EntityState* entity = productPlayerEntity(session);
  // branch-gate: BG-1181
  if (entity == nullptr || !window.activeRoom.loaded) {
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
  window.gameplayJumpRequested = false;
  window.gameplayJumpAccepted = false;
  window.gameplayJumpActive = true;
  window.gameplayJumpVelocityMetersPerSecond = 0.0F;
  window.gameplayJumpCoyoteSecondsRemaining =
      window.gameplayMovementTuning.coyoteTimeSeconds;
  window.gameplayJumpCutApplied = false;
  window.gameplayJumpHeld = false;
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
  window.gameplayJumpStatus = "falling";
  window.gameplayJumpReasonCode = "gameplay_jump_falling";
  return true;
}

void recordProductJumpPosition(ProductAppWindowState& window,
                               float groundY,
                               float startY,
                               float finalY) {
  window.gameplayJumpGroundY = groundY;
  window.gameplayJumpStartY = startY;
  window.gameplayJumpFinalY = finalY;
  window.gameplayJumpHeightMeters = std::max(0.0F, finalY - groundY);
}

void clearProductJumpTiming(ProductAppWindowState& window) {
  window.gameplayJumpCoyoteSecondsRemaining = 0.0F;
  window.gameplayJumpBufferSecondsRemaining = 0.0F;
  window.gameplayJumpHeld = false;
  window.gameplayJumpCutApplied = false;
}

void rejectProductJump(ProductAppWindowState& window,
                       std::string_view status,
                       std::string_view reason) {
  window.gameplayJumpRequested = true;
  window.gameplayJumpAccepted = false;
  window.gameplayJumpStatus = std::string{status};
  window.gameplayJumpReasonCode = std::string{reason};
}

bool productJumpBufferLive(const ProductAppWindowState& window) {
  return window.gameplayJumpBufferSecondsRemaining > 0.0F;
}

void bufferProductJump(ProductAppWindowState& window) {
  window.gameplayJumpBufferSecondsRemaining =
      window.gameplayMovementTuning.jumpBufferSeconds;
}

void beginProductJumpArc(Session& session,
                         ProductAppWindowState& window,
                         const EntityState& actor,
                         float groundY,
                         std::string_view reason) {
  const ProductGameplayMovementTuning& tuning = window.gameplayMovementTuning;
  window.gameplayJumpAccepted = true;
  window.gameplayJumpActive = true;
  window.gameplayJumpVelocityMetersPerSecond =
      tuning.jumpImpulseMetersPerSecond;
  window.gameplayJumpCoyoteSecondsRemaining = 0.0F;
  window.gameplayJumpBufferSecondsRemaining = 0.0F;
  window.gameplayJumpHeld = true;
  window.gameplayJumpCutApplied = false;
  recordProductJumpPosition(window, groundY, actor.transform.position.y,
                            actor.transform.position.y);
  window.gameplayJumpStatus = "accepted";
  window.gameplayJumpReasonCode = std::string{reason};
  advanceProductJump(session,
                     window,
                     productActiveRoomCollisionSurfaces(window.activeRoomCollision));
}

bool tryProductCoyoteJump(Session& session, ProductAppWindowState& window) {
  // branch-gate: BG-1153
  if (!window.gameplayJumpActive ||
      window.gameplayJumpCoyoteSecondsRemaining <= 0.0F) {
    return false;
  }
  const EntityState* actor = productPlayerEntity(session);
  // branch-gate: BG-1153
  if (actor == nullptr) {
    rejectProductJump(window, "missing_player", "gameplay_jump_missing_player");
    return true;
  }
  beginProductJumpArc(session,
                      window,
                      *actor,
                      window.gameplayJumpGroundY,
                      "gameplay_jump_coyote");
  return true;
}

void applyProductJumpReleaseCut(ProductAppWindowState& window) {
  ProductGameplayMovementTuning& tuning = window.gameplayMovementTuning;
  window.gameplayJumpHeld = false;
  // branch-gate: BG-1153
  if (!window.gameplayJumpActive || window.gameplayJumpCutApplied ||
      window.gameplayJumpVelocityMetersPerSecond <= 0.0F) {
    return;
  }
  const float multiplier = std::clamp(tuning.jumpCutMultiplier, 0.1F, 1.0F);
  window.gameplayJumpVelocityMetersPerSecond *= multiplier;
  window.gameplayJumpCutApplied = true;
}

void clearProductTraversalProof(ProductAppWindowState& window) {
  window.gameplayTraversalRequested = false;
  window.gameplayTraversalConsumed = false;
  window.gameplayTraversalAccepted = false;
  window.gameplayTraversalFallbackJumpAllowed = false;
  window.gameplayTraversalStatus = "not_requested";
  window.gameplayTraversalReasonCode = "not_requested";
  window.gameplayTraversalMechanic = "none";
  window.gameplayTraversalSlotId = "none";
  window.gameplayTraversalTargetId = "none";
  window.gameplayTraversalLandingSurfaceId = "none";
  window.gameplayTraversalStartX = 0.0F;
  window.gameplayTraversalStartY = 0.0F;
  window.gameplayTraversalStartZ = 0.0F;
  window.gameplayTraversalFinalX = 0.0F;
  window.gameplayTraversalFinalY = 0.0F;
  window.gameplayTraversalFinalZ = 0.0F;
}

void recordProductTraversalProof(ProductAppWindowState& window,
                                 const TraversalIntentResult& result) {
  window.gameplayTraversalRequested = result.requested;
  window.gameplayTraversalConsumed = result.consumedInput;
  window.gameplayTraversalAccepted = result.accepted;
  window.gameplayTraversalFallbackJumpAllowed = result.fallbackJumpAllowed;
  window.gameplayTraversalStatus = traversalIntentStatusName(result.status);
  // branch-gate: BG-1156
  window.gameplayTraversalReasonCode =
      result.reasonCode == nullptr ? "unknown" : result.reasonCode;
  // branch-gate: BG-1156
  window.gameplayTraversalMechanic =
      result.traversalAttempted ? traversalMechanicName(result.selectedMechanic)
                                : "none";
  // branch-gate: BG-1156
  window.gameplayTraversalSlotId =
      result.traversal.slotId.empty() ? "none" : result.traversal.slotId;
  // branch-gate: BG-1156
  window.gameplayTraversalTargetId =
      result.traversal.targetId.empty() ? "none" : result.traversal.targetId;
  // branch-gate: BG-1156
  window.gameplayTraversalLandingSurfaceId =
      result.traversal.landingSurfaceId.empty() ? "none"
                                                : result.traversal.landingSurfaceId;
  window.gameplayTraversalStartX = result.traversal.start.x;
  window.gameplayTraversalStartY = result.traversal.start.y;
  window.gameplayTraversalStartZ = result.traversal.start.z;
  window.gameplayTraversalFinalX = result.traversal.finalPosition.x;
  window.gameplayTraversalFinalY = result.traversal.finalPosition.y;
  window.gameplayTraversalFinalZ = result.traversal.finalPosition.z;
}

bool actorBlockingSurface(const CollisionSurfaceView& surface) {
  return surface.blocksActor || surface.hasActorMask;
}

bool hasTraversalTag(const CollisionSurfaceView& surface, std::string_view expected) {
  for (const std::string& tag : surface.traversalTags) {
    // branch-gate: BG-1157
    if (tag == expected) {
      return true;
    }
  }
  return false;
}

bool horizontalNormal(Vec3 normal, Vec3& out) {
  normal.y = 0.0F;
  const float lenSq = lengthSquared(normal);
  // branch-gate: BG-1157
  if (!isFinite(normal) || lenSq <= 0.0001F) {
    return false;
  }
  out = normal / std::sqrt(lenSq);
  return true;
}

bool isNearVerticalSurface(Vec3 position,
                           const CollisionSurfaceView& surface,
                           Vec3& awayNormal,
                           float& distanceSq,
                           const ProductGameplayMovementTuning& tuning) {
  // branch-gate: BG-1157
  if (!actorBlockingSurface(surface) || surface.opening ||
      surface.role != CollisionSurfaceRole::Blocker ||
      !hasTraversalTag(surface, "wall_jump") ||
      !isValid(surface.bounds)) {
    return false;
  }

  Vec3 normal;
  // branch-gate: BG-1157
  if (!horizontalNormal(surface.normal, normal)) {
    return false;
  }

  const float verticalSlack = 0.35F;
  // branch-gate: BG-1157
  if (position.y < surface.bounds.min.y - verticalSlack ||
      position.y > surface.bounds.max.y + verticalSlack) {
    return false;
  }

  const float clampedX =
      std::clamp(position.x, surface.bounds.min.x, surface.bounds.max.x);
  const float clampedZ =
      std::clamp(position.z, surface.bounds.min.z, surface.bounds.max.z);
  const Vec3 nearest{clampedX, position.y, clampedZ};
  Vec3 fromSurface = position - nearest;
  fromSurface.y = 0.0F;
  const float fromSurfaceSq = lengthSquared(fromSurface);
  // branch-gate: BG-1157
  if (fromSurfaceSq > 0.0001F) {
    awayNormal = fromSurface / std::sqrt(fromSurfaceSq);
  } else {
    const Vec3 centerToPlayer = position - center(surface.bounds);
    // branch-gate: BG-1157
    awayNormal = dot(centerToPlayer, normal) < 0.0F ? normal * -1.0F : normal;
  }

  distanceSq = fromSurfaceSq;
  return distanceSq <= tuning.wallJumpProbeMeters * tuning.wallJumpProbeMeters;
}

const CollisionSurfaceView* findWallJumpSurface(const SpatialSurfaceSet& surfaces,
                                                Vec3 position,
                                                Vec3& awayNormal,
                                                const ProductGameplayMovementTuning& tuning) {
  const CollisionSurfaceView* best = nullptr;
  float bestDistanceSq = tuning.wallJumpProbeMeters * tuning.wallJumpProbeMeters;
  for (const CollisionSurfaceView& surface : surfaces.surfaces()) {
    Vec3 candidateNormal;
    float candidateDistanceSq = 0.0F;
    // branch-gate: BG-1157
    if (!isNearVerticalSurface(position, surface, candidateNormal,
                               candidateDistanceSq, tuning)) {
      continue;
    }
    // branch-gate: BG-1157
    if (best != nullptr && candidateDistanceSq >= bestDistanceSq) {
      continue;
    }
    best = &surface;
    bestDistanceSq = candidateDistanceSq;
    awayNormal = candidateNormal;
  }
  return best;
}

bool isNearWallRunSurface(Vec3 position,
                          const CollisionSurfaceView& surface,
                          Vec3& awayNormal,
                          float& distanceSq,
                          const ProductGameplayMovementTuning& tuning) {
  // branch-gate: BG-1157
  if (!actorBlockingSurface(surface) || surface.opening ||
      surface.role != CollisionSurfaceRole::Blocker ||
      !isValid(surface.bounds) ||
      std::fabs(surface.normal.y) >
          std::clamp(tuning.wallRunMaxWallNormalY, 0.0F, 1.0F)) {
    return false;
  }

  Vec3 normal;
  // branch-gate: BG-1157
  if (!horizontalNormal(surface.normal, normal)) {
    return false;
  }

  // branch-gate: BG-1157
  if (position.y < surface.bounds.min.y - kWallRunSurfaceVerticalSlackMeters ||
      position.y > surface.bounds.max.y + kWallRunSurfaceVerticalSlackMeters) {
    return false;
  }

  const float clampedX =
      std::clamp(position.x, surface.bounds.min.x, surface.bounds.max.x);
  const float clampedZ =
      std::clamp(position.z, surface.bounds.min.z, surface.bounds.max.z);
  const Vec3 nearest{clampedX, position.y, clampedZ};
  Vec3 fromSurface = position - nearest;
  fromSurface.y = 0.0F;
  const float fromSurfaceSq = lengthSquared(fromSurface);
  // branch-gate: BG-1157
  if (fromSurfaceSq > 0.0001F) {
    awayNormal = fromSurface / std::sqrt(fromSurfaceSq);
  } else {
    const Vec3 centerToPlayer = position - center(surface.bounds);
    // branch-gate: BG-1157
    awayNormal = dot(centerToPlayer, normal) < 0.0F ? normal * -1.0F : normal;
  }

  distanceSq = fromSurfaceSq;
  return distanceSq <= tuning.wallJumpProbeMeters * tuning.wallJumpProbeMeters;
}

const CollisionSurfaceView* findWallRunSurface(const SpatialSurfaceSet& surfaces,
                                               Vec3 position,
                                               Vec3& awayNormal,
                                               const ProductGameplayMovementTuning& tuning) {
  const CollisionSurfaceView* best = nullptr;
  float bestDistanceSq = tuning.wallJumpProbeMeters * tuning.wallJumpProbeMeters;
  for (const CollisionSurfaceView& surface : surfaces.surfaces()) {
    Vec3 candidateNormal;
    float candidateDistanceSq = 0.0F;
    // branch-gate: BG-1157
    if (!isNearWallRunSurface(position,
                              surface,
                              candidateNormal,
                              candidateDistanceSq,
                              tuning)) {
      continue;
    }
    // branch-gate: BG-1157
    if (best != nullptr && candidateDistanceSq >= bestDistanceSq) {
      continue;
    }
    best = &surface;
    bestDistanceSq = candidateDistanceSq;
    awayNormal = candidateNormal;
  }
  return best;
}

std::string wallRunSideName(Vec3 awayNormal, float yawDegrees) {
  const float yawRadians = yawDegrees * kPi / 180.0F;
  const Vec3 forward{std::sin(yawRadians), 0.0F, -std::cos(yawRadians)};
  const Vec3 right{std::cos(yawRadians), 0.0F, std::sin(yawRadians)};
  const float rightDot = dot(awayNormal, right);
  const float forwardDot = dot(awayNormal, forward);
  // branch-gate: BG-1157
  if (std::fabs(rightDot) >= 0.35F) {
    // branch-gate: BG-1157
    return rightDot > 0.0F ? "left" : "right";
  }
  // branch-gate: BG-1157
  if (std::fabs(forwardDot) >= 0.35F) {
    // branch-gate: BG-1157
    return forwardDot < 0.0F ? "front" : "back";
  }
  return "unknown";
}

bool productMovementDebugAlongWall(const ProductAppWindowState& window,
                                   Vec3 awayNormal) {
  Vec3 travel{window.gameplayMovementFinalX - window.gameplayMovementStartX,
              0.0F,
              window.gameplayMovementFinalZ - window.gameplayMovementStartZ};
  const float travelLenSq = lengthSquared(travel);
  // branch-gate: BG-1161
  if (!isFinite(travel) || travelLenSq <= kMovementStateDistanceEpsilonMeters) {
    return false;
  }
  travel = travel / std::sqrt(travelLenSq);
  const Vec3 tangent{-awayNormal.z, 0.0F, awayNormal.x};
  return std::fabs(dot(travel, tangent)) >= kWallRunAlongWallDotThreshold;
}

bool wallRunProofNormal(const ProductAppWindowState& window, Vec3& normal) {
  normal = {window.gameplayWallRunNormalX, 0.0F, window.gameplayWallRunNormalZ};
  const float lenSq = lengthSquared(normal);
  // branch-gate: BG-1157
  if (!isFinite(normal) || lenSq <= 0.0001F) {
    return false;
  }
  normal = normal / std::sqrt(lenSq);
  return true;
}

bool wallRunTangentDirection(const ProductAppWindowState& window,
                             float moveX,
                             float moveY,
                             Vec3& direction) {
  Vec3 normal;
  // branch-gate: BG-1157
  if (!wallRunProofNormal(window, normal)) {
    return false;
  }
  const Vec3 desired =
      manualFirstPersonDirection(moveX, moveY, window.viewport.cameraYawDegrees);
  const Vec3 tangent{-normal.z, 0.0F, normal.x};
  const float tangentDot = dot(desired, tangent);
  // branch-gate: BG-1157
  if (std::fabs(tangentDot) < kWallRunAlongWallDotThreshold) {
    return false;
  }
  // branch-gate: BG-1157
  direction = tangentDot >= 0.0F ? tangent : tangent * -1.0F;
  return true;
}

void recordProductWallRunProof(ProductAppWindowState& window,
                               std::string_view status,
                               std::string_view reason) {
  window.gameplayWallRunStatus = std::string{status};
  window.gameplayWallRunReasonCode = std::string{reason};
  window.gameplayWallRunDurationSeconds =
      std::clamp(window.gameplayMovementTuning.wallRunDurationSeconds, 0.1F, 2.0F);
  window.gameplayWallRunGravityMultiplier =
      std::clamp(window.gameplayMovementTuning.wallRunGravityMultiplier, 0.0F, 1.0F);
  window.gameplayWallRunSpeedMultiplier =
      std::clamp(window.gameplayMovementTuning.wallRunSpeedMultiplier, 0.25F, 2.0F);
}

void updateProductWallRunActiveProof(ProductAppWindowState& window,
                                     float moveX,
                                     float moveY) {
  const bool hasMoveInput = moveX != 0.0F || moveY != 0.0F;
  // branch-gate: BG-1157
  if (window.gameplayWallRunActive && !window.gameplayJumpActive) {
    clearProductWallRunActiveProof(window, "wall_run_landed");
    return;
  }
  // branch-gate: BG-1157
  if (window.gameplayWallRunActive && !hasMoveInput) {
    clearProductWallRunActiveProof(window, "wall_run_input_stopped");
    return;
  }
  Vec3 tangent;
  // branch-gate: BG-1157
  if (window.gameplayWallRunActive &&
      !wallRunTangentDirection(window, moveX, moveY, tangent)) {
    clearProductWallRunActiveProof(window, "wall_run_input_away");
    return;
  }
  // branch-gate: BG-1157
  if (window.gameplayWallRunActive &&
      !window.gameplayWallRunCandidateAvailable) {
    clearProductWallRunActiveProof(
        window, window.gameplayWallRunCandidateReasonCode);
    return;
  }
  // branch-gate: BG-1157
  if (!window.gameplayWallRunActive &&
      (!window.gameplayWallRunCandidateAvailable || !hasMoveInput)) {
    clearProductWallRunActiveProof(window, "wall_run_inactive");
    return;
  }
  // branch-gate: BG-1157
  if (!window.gameplayWallRunActive &&
      !wallRunTangentDirection(window, moveX, moveY, tangent)) {
    clearProductWallRunActiveProof(window, "wall_run_input_away");
    return;
  }

  const float dt = std::max(0.0F, window.gameplayMovementTuning.inputStepSeconds);
  // branch-gate: BG-1157
  if (!window.gameplayWallRunActive) {
    window.gameplayWallRunActive = true;
    window.gameplayWallRunRemainingSeconds =
        std::clamp(window.gameplayMovementTuning.wallRunDurationSeconds,
                   0.1F,
                   2.0F);
    recordProductWallRunProof(window, "wall_run_active", "wall_run_started");
    return;
  }

  window.gameplayWallRunRemainingSeconds =
      std::max(0.0F, window.gameplayWallRunRemainingSeconds - dt);
  // branch-gate: BG-1157
  if (window.gameplayWallRunRemainingSeconds <= 0.0F) {
    clearProductWallRunActiveProof(window, "wall_run_expired");
    return;
  }
  recordProductWallRunProof(window, "wall_run_active", "wall_run_active");
}

void updateProductWallRunCandidateProof(const Session& session,
                                        ProductAppWindowState& window,
                                        const SpatialSurfaceSet* collisionSurfaces) {
  window.gameplayWallRunApproachSpeedMetersPerSecond =
      window.gameplayMovementHorizontalSpeedMetersPerSecond;

  // branch-gate: BG-1153
  if (!window.gameplayJumpActive) {
    clearProductWallRunCandidateProof(window, "wall_run_grounded");
    return;
  }
  const float minSpeed =
      std::max(0.0F, window.gameplayMovementTuning.wallRunMinSpeedMetersPerSecond);
  // branch-gate: BG-1161
  if (window.gameplayMovementHorizontalSpeedMetersPerSecond < minSpeed) {
    clearProductWallRunCandidateProof(window, "wall_run_low_speed");
    window.gameplayWallRunApproachSpeedMetersPerSecond =
        window.gameplayMovementHorizontalSpeedMetersPerSecond;
    return;
  }
  // branch-gate: BG-1157
  if (collisionSurfaces == nullptr) {
    clearProductWallRunCandidateProof(window, "wall_run_no_surfaces");
    return;
  }
  const EntityState* actor = productPlayerEntity(session);
  // branch-gate: BG-1153
  if (actor == nullptr) {
    clearProductWallRunCandidateProof(window, "wall_run_missing_player");
    return;
  }

  Vec3 awayNormal;
  const CollisionSurfaceView* surface =
      findWallRunSurface(*collisionSurfaces,
                         actor->transform.position,
                         awayNormal,
                         window.gameplayMovementTuning);
  // branch-gate: BG-1157
  if (surface == nullptr) {
    clearProductWallRunCandidateProof(window, "wall_run_no_wall_contact");
    window.gameplayWallRunApproachSpeedMetersPerSecond =
        window.gameplayMovementHorizontalSpeedMetersPerSecond;
    return;
  }
  // branch-gate: BG-1161
  if (!productMovementDebugAlongWall(window, awayNormal)) {
    clearProductWallRunCandidateProof(window, "wall_run_not_along_wall");
    window.gameplayWallRunSurfaceId = surface->id.empty() ? "wall_run_surface"
                                                          : surface->id;
    window.gameplayWallRunNormalX = awayNormal.x;
    window.gameplayWallRunNormalY = surface->normal.y;
    window.gameplayWallRunNormalZ = awayNormal.z;
    window.gameplayWallRunApproachSpeedMetersPerSecond =
        window.gameplayMovementHorizontalSpeedMetersPerSecond;
    return;
  }

  window.gameplayWallRunCandidateAvailable = true;
  window.gameplayWallRunCandidateStatus = "wall_run_candidate";
  window.gameplayWallRunCandidateReasonCode = "wall_run_candidate";
  window.gameplayWallRunSide =
      wallRunSideName(awayNormal, window.viewport.cameraYawDegrees);
  // branch-gate: BG-1157
  window.gameplayWallRunSurfaceId =
      surface->id.empty() ? "wall_run_surface" : surface->id;
  window.gameplayWallRunNormalX = awayNormal.x;
  window.gameplayWallRunNormalY = surface->normal.y;
  window.gameplayWallRunNormalZ = awayNormal.z;
  window.gameplayWallRunApproachSpeedMetersPerSecond =
      window.gameplayMovementHorizontalSpeedMetersPerSecond;
}

void recordProductWallJumpTraversalProof(ProductAppWindowState& window,
                                         const CollisionSurfaceView& surface,
                                         Vec3 start,
                                         Vec3 finalPosition) {
  window.gameplayTraversalRequested = true;
  window.gameplayTraversalConsumed = true;
  window.gameplayTraversalAccepted = true;
  window.gameplayTraversalFallbackJumpAllowed = false;
  window.gameplayTraversalStatus = "traversal_intent_applied";
  window.gameplayTraversalReasonCode = "traversal_intent_applied";
  window.gameplayTraversalMechanic = "wall_jump";
  window.gameplayTraversalSlotId = "wall_jump";
  window.gameplayTraversalLandingSurfaceId = "wall_jump_surface";
  // branch-gate: BG-1157
  if (!surface.id.empty()) {
    window.gameplayTraversalSlotId = surface.id;
    window.gameplayTraversalLandingSurfaceId = surface.id;
  }
  window.gameplayTraversalTargetId = window.gameplayTraversalSlotId;
  // branch-gate: BG-1157
  if (!surface.runtimeOwnerStableName.empty()) {
    window.gameplayTraversalTargetId = surface.runtimeOwnerStableName;
  }
  window.gameplayTraversalStartX = start.x;
  window.gameplayTraversalStartY = start.y;
  window.gameplayTraversalStartZ = start.z;
  window.gameplayTraversalFinalX = finalPosition.x;
  window.gameplayTraversalFinalY = finalPosition.y;
  window.gameplayTraversalFinalZ = finalPosition.z;
}

bool tryProductWallJump(Session& session, ProductAppWindowState& window) {
  const ProductGameplayMovementTuning& tuning = window.gameplayMovementTuning;
  const SpatialSurfaceSet* surfaces =
      productActiveRoomCollisionSurfaces(window.activeRoomCollision);
  const EntityId actor = productPlayerActor(session);
  const EntityState* entity = session.state().world.findById(actor);
  // branch-gate: BG-1157
  if (surfaces == nullptr || entity == nullptr) {
    return false;
  }

  const Vec3 start = entity->transform.position;
  // branch-gate: BG-1157
  if (!window.gameplayJumpActive &&
      start.y <= tuning.wallJumpMinAirborneHeightMeters) {
    return false;
  }

  Vec3 awayNormal;
  const CollisionSurfaceView* surface =
      findWallJumpSurface(*surfaces, start, awayNormal, tuning);
  // branch-gate: BG-1157
  if (surface == nullptr) {
    return false;
  }

  Vec3 finalPosition = start + awayNormal * tuning.wallJumpPushMeters;
  finalPosition.y = start.y + tuning.wallJumpRiseMeters;
  // branch-gate: BG-1157
  if (!setProductPlayerPosition(session, actor, finalPosition)) {
    return false;
  }

  recordProductWallJumpTraversalProof(window, *surface, start, finalPosition);
  window.gameplayJumpAccepted = true;
  window.gameplayJumpActive = true;
  window.gameplayJumpVelocityMetersPerSecond =
      tuning.jumpImpulseMetersPerSecond;
  recordProductJumpPosition(window, 0.0F, start.y, finalPosition.y);
  window.gameplayJumpStatus = "wall_jump";
  window.gameplayJumpReasonCode = "gameplay_jump_wall_jump";
  window.playerPositionChanged = true;
  window.runtimeStateHash = session.stateHash();
  return true;
}

bool tryProductTraversalJump(Session& session, ProductAppWindowState& window) {
  clearProductTraversalProof(window);
  const SpatialSurfaceSet* surfaces =
      productActiveRoomCollisionSurfaces(window.activeRoomCollision);
  // branch-gate: BG-1156
  if (!window.activeRoom.loaded || surfaces == nullptr) {
    return false;
  }

  TraversalIntentRequest request;
  request.actor = productPlayerActor(session);
  request.jumpPressed = true;
  request.forward = manualFirstPersonDirection(0.0F,
                                               1.0F,
                                               window.viewport.cameraYawDegrees);
  request.room = &window.activeRoom.room;
  request.collisionSurfaces = surfaces;

  SessionState& state = session.mutableStateForOwnedSystems();
  const TraversalIntentResult result = executeTraversalIntent(state.world, request);
  // If the player is already on top of a clamberable wall, the clamber slot is
  // height-rejected. Do not consume jump in that state; let normal jump run.
  // branch-gate: BG-1160
  if (result.status == TraversalIntentStatus::TraversalRejected &&
      result.selectedMechanic == TraversalMechanic::Clamber &&
      result.traversal.status == TraversalStatus::HeightRejected) {
    return false;
  }
  recordProductTraversalProof(window, result);
  // branch-gate: BG-1156
  if (result.traversalAttempted) {
    state.currentStateHash = computeStateHash(state);
    window.runtimeStateHash = session.stateHash();
  }
  // branch-gate: BG-1156
  if (result.accepted) {
    window.playerPositionChanged = true;
  }
  return result.consumedInput;
}

void advanceProductJump(Session& session,
                        ProductAppWindowState& window,
                        const SpatialSurfaceSet* collisionSurfaces) {
  const ProductGameplayMovementTuning& tuning = window.gameplayMovementTuning;
  const float dt = std::max(0.0F, tuning.inputStepSeconds);
  window.gameplayJumpBufferSecondsRemaining =
      std::max(0.0F, window.gameplayJumpBufferSecondsRemaining - dt);
  window.gameplayJumpCoyoteSecondsRemaining =
      std::max(0.0F, window.gameplayJumpCoyoteSecondsRemaining - dt);
  // branch-gate: BG-1181
  if (applyProductGameplayResetIfNeeded(session, window, collisionSurfaces)) {
    return;
  }
  // branch-gate: BG-1173
  if (!window.gameplayJumpActive &&
      !beginProductFallIfUnsupported(session, window, collisionSurfaces)) {
    return;
  }

  const EntityId actor = productPlayerActor(session);
  const EntityState* entity = session.state().world.findById(actor);
  // branch-gate: BG-1153
  if (entity == nullptr) {
    window.gameplayJumpActive = false;
    window.gameplayJumpVelocityMetersPerSecond = 0.0F;
    window.gameplayJumpStatus = "missing_player";
    window.gameplayJumpReasonCode = "gameplay_jump_missing_player";
    return;
  }

  const float previousY = entity->transform.position.y;
  float gravityMultiplier = 1.0F;
  // branch-gate: BG-1153
  if (window.gameplayJumpVelocityMetersPerSecond <= 0.0F) {
    // branch-gate: BG-1157
    gravityMultiplier =
        window.gameplayWallRunActive
            ? std::clamp(tuning.wallRunGravityMultiplier, 0.0F, 1.0F)
            : tuning.fallGravityMultiplier;
  }
  const float gravity =
      tuning.gravityMetersPerSecondSquared *
      std::clamp(gravityMultiplier, 0.0F, 4.0F);
  const float nextVelocity =
      window.gameplayJumpVelocityMetersPerSecond - gravity * dt;
  float nextY = previousY +
                window.gameplayJumpVelocityMetersPerSecond * dt -
                0.5F * gravity * dt * dt;
  bool landed = false;
  float landingY = window.gameplayJumpGroundY;
  const bool collisionLanding =
      nextVelocity <= 0.0F &&
      findHighestWalkableGroundAtOrBelow(
          collisionSurfaces,
          entity->transform.position,
          previousY,
          landingY) &&
      nextY <= landingY + kGameplayGroundContactToleranceMeters;
  if ((collisionSurfaces == nullptr && nextY <= window.gameplayJumpGroundY &&
       nextVelocity <= 0.0F) ||
      collisionLanding) {
    nextY = landingY;
    landed = true;
  }

  Vec3 position = entity->transform.position;
  position.y = nextY;
  // branch-gate: BG-1153
  if (!setProductPlayerPosition(session, actor, position)) {
    window.gameplayJumpActive = false;
    window.gameplayJumpVelocityMetersPerSecond = 0.0F;
    window.gameplayJumpStatus = "mutation_failed";
    window.gameplayJumpReasonCode = "gameplay_jump_mutation_failed";
    return;
  }

  window.playerPositionChanged =
      window.playerPositionChanged || std::fabs(previousY - nextY) > 0.0001F;
  window.gameplayJumpActive = !landed;
  // branch-gate: BG-1153
  window.gameplayJumpVelocityMetersPerSecond = landed ? 0.0F : nextVelocity;
  // branch-gate: BG-1153
  if (landed) {
    clearProductWallRunActiveProof(window, "wall_run_landed");
    window.gameplayJumpCoyoteSecondsRemaining = 0.0F;
    window.gameplayJumpHeld = false;
    window.gameplayJumpCutApplied = false;
  }
  const std::array<float, 2U> groundProofYs{window.gameplayJumpGroundY, landingY};
  recordProductJumpPosition(
      window,
      groundProofYs[static_cast<std::size_t>(landed || collisionLanding)],
      window.gameplayJumpStartY,
      nextY);
  // branch-gate: BG-1153
  window.gameplayJumpStatus = landed ? "landed" : "airborne";
  // branch-gate: BG-1153
  window.gameplayJumpReasonCode =
      landed ? "gameplay_jump_landed" : "gameplay_jump_airborne";
  window.runtimeStateHash = session.stateHash();
  // branch-gate: BG-1153
  if (landed && productJumpBufferLive(window)) {
    const EntityState* landedEntity = productPlayerEntity(session);
    // branch-gate: BG-1153
    if (landedEntity != nullptr) {
      beginProductJumpArc(session,
                          window,
                          *landedEntity,
                          landingY,
                          "gameplay_jump_buffered");
      return;
    }
  }
  applyProductGameplayResetIfNeeded(session, window, collisionSurfaces);
}

void submitProductJump(Session& session,
                       ProductAppWindowState& window,
                       std::string_view source) {
  clearProductTargetProof(window);
  clearProductOutcomeProof(window);
  window.gameplayInputUsed = true;
  window.gameplayInputSource = std::string{source};
  window.gameplayJumpRequested = true;

  // branch-gate: BG-1156
  if (tryProductTraversalJump(session, window)) {
    window.gameplayJumpAccepted = false;
    // branch-gate: BG-1156
    window.gameplayJumpStatus = window.gameplayTraversalAccepted ? "traversal"
                                                                 : "traversal_rejected";
    window.gameplayJumpReasonCode = window.gameplayTraversalReasonCode;
    return;
  }

  // branch-gate: BG-1157
  if (tryProductWallJump(session, window)) {
    return;
  }

  // branch-gate: BG-1153
  if (window.gameplayJumpActive) {
    // branch-gate: BG-1153
    if (tryProductCoyoteJump(session, window)) {
      return;
    }
    bufferProductJump(window);
    rejectProductJump(window, "already_airborne",
                      "gameplay_jump_already_airborne");
    return;
  }

  const EntityState* actor = productPlayerEntity(session);
  // branch-gate: BG-1153
  if (actor == nullptr) {
    rejectProductJump(window, "missing_player", "gameplay_jump_missing_player");
    return;
  }

  beginProductJumpArc(session,
                      window,
                      *actor,
                      actor->transform.position.y,
                      "gameplay_jump_accepted");
}

Vec3 manualFirstPersonDirection(float moveX, float moveY, float yawDegrees) {
  const float yawRadians = yawDegrees * kPi / 180.0F;
  const float cosYaw = std::cos(yawRadians);
  const float sinYaw = std::sin(yawRadians);
  const Vec3 forward{sinYaw, 0.0F, -cosYaw};
  const Vec3 right{cosYaw, 0.0F, sinYaw};
  const Vec3 raw = right * moveX + forward * moveY;
  const float magnitude = std::sqrt(raw.x * raw.x + raw.z * raw.z);
  // branch-gate: BG-1155
  if (magnitude <= 0.0001F) {
    return forward;
  }
  return raw * (1.0F / magnitude);
}

void advanceProductDashCooldown(ProductAppWindowState& window) {
  const ProductGameplayMovementTuning& tuning = window.gameplayMovementTuning;
  // branch-gate: BG-1155
  if (window.gameplayDashCooldownRemainingSeconds <= 0.0F) {
    window.gameplayDashCooldownRemainingSeconds = 0.0F;
    return;
  }
  window.gameplayDashCooldownRemainingSeconds =
      std::max(0.0F,
               window.gameplayDashCooldownRemainingSeconds -
                   tuning.inputStepSeconds);
}

void rejectProductDash(ProductAppWindowState& window,
                       std::string_view status,
                       std::string_view reason) {
  window.gameplayDashRequested = true;
  window.gameplayDashAccepted = false;
  window.gameplayDashStatus = std::string{status};
  window.gameplayDashReasonCode = std::string{reason};
}

void submitProductDash(Session& session,
                       ProductAppWindowState& window,
                       float moveX,
                       float moveY,
                       std::string_view source,
                       const SpatialSurfaceSet* collisionSurfaces) {
  const ProductGameplayMovementTuning& tuning = window.gameplayMovementTuning;
  clearProductTargetProof(window);
  clearProductOutcomeProof(window);
  window.gameplayInputUsed = true;
  window.gameplayInputSource = std::string{source};
  window.gameplayDashRequested = true;

  // branch-gate: BG-1155
  if (window.gameplayDashCooldownRemainingSeconds > 0.0F) {
    rejectProductDash(window, "cooldown", "gameplay_dash_cooldown");
    return;
  }

  const EntityState* actor = productPlayerEntity(session);
  // branch-gate: BG-1155
  if (actor == nullptr) {
    rejectProductDash(window, "missing_player", "gameplay_dash_missing_player");
    return;
  }

  const Vec3 direction =
      manualFirstPersonDirection(moveX, moveY, window.viewport.cameraYawDegrees);
  const float dashDistance = tuning.dashSpeedMetersPerSecond *
                             tuning.dashDurationSeconds;
  Vec3 destination = actor->transform.position + direction * dashDistance;
  destination.y = actor->transform.position.y;

  window.gameplayDashAccepted = true;
  window.gameplayDashStatus = "accepted";
  window.gameplayDashReasonCode = "gameplay_dash_accepted";
  window.gameplayDashSpeedMetersPerSecond = tuning.dashSpeedMetersPerSecond;
  window.gameplayDashDistanceMeters = dashDistance;
  window.gameplayDashCooldownRemainingSeconds =
      tuning.dashCooldownSeconds;
  window.gameplayDashDirectionX = direction.x;
  window.gameplayDashDirectionZ = direction.z;
  window.gameplayMovementProfile = std::string{tuning.dashProfile};
  window.gameplayMovementMaxSpeedMetersPerSecond =
      tuning.dashSpeedMetersPerSecond;

  CommandRecord command;
  command.playerSlot = 0;
  command.actor = actor->id;
  command.kind = CommandKind::Move;
  command.source = CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = destination;
  submitProductGameplayCommand(session, window, command, collisionSurfaces);
}

Vec3 manualFirstPersonMoveDelta(float moveX,
                                float moveY,
                                float yawDegrees,
                                bool sprinting,
                                const ProductGameplayMovementTuning& tuning,
                                float responseMultiplier) {
  const float magnitude = std::sqrt(moveX * moveX + moveY * moveY);
  const float scale = 1.0F / std::max(1.0F, magnitude);
  const float stepMeters =
      manualFirstPersonMaxSpeedMetersPerSecond(tuning, sprinting) *
      tuning.inputStepSeconds *
      std::clamp(responseMultiplier, 0.0F, 4.0F);
  const float yawRadians = yawDegrees * kPi / 180.0F;
  const float cosYaw = std::cos(yawRadians);
  const float sinYaw = std::sin(yawRadians);
  const Vec3 forward{sinYaw, 0.0F, -cosYaw};
  const Vec3 right{cosYaw, 0.0F, sinYaw};
  return (right * moveX + forward * moveY) * (scale * stepMeters);
}

Vec3 manualFirstPersonDesiredVelocity(float moveX,
                                      float moveY,
                                      float yawDegrees,
                                      bool sprinting,
                                      const ProductGameplayMovementTuning& tuning) {
  const float magnitude = std::sqrt(moveX * moveX + moveY * moveY);
  // branch-gate: BG-1161
  if (magnitude <= 0.0F || !std::isfinite(magnitude)) {
    return {};
  }
  const float scale = 1.0F / std::max(1.0F, magnitude);
  const float speed =
      manualFirstPersonMaxSpeedMetersPerSecond(tuning, sprinting);
  const float yawRadians = yawDegrees * kPi / 180.0F;
  const float cosYaw = std::cos(yawRadians);
  const float sinYaw = std::sin(yawRadians);
  const Vec3 forward{sinYaw, 0.0F, -cosYaw};
  const Vec3 right{cosYaw, 0.0F, sinYaw};
  return (right * moveX + forward * moveY) * (scale * speed);
}

Vec3 moveHorizontalVelocityToward(Vec3 current, Vec3 target, float maxDelta) {
  Vec3 delta{target.x - current.x, 0.0F, target.z - current.z};
  const float distance = std::sqrt(delta.x * delta.x + delta.z * delta.z);
  // branch-gate: BG-1161
  if (distance <= 0.0001F || !std::isfinite(distance)) {
    return target;
  }
  // branch-gate: BG-1161
  if (maxDelta >= distance) {
    return target;
  }
  const float scale = std::max(0.0F, maxDelta) / distance;
  return {current.x + delta.x * scale, 0.0F, current.z + delta.z * scale};
}

Vec3 clampHorizontalVelocity(Vec3 velocity, float maxSpeed) {
  const float speed =
      std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
  // branch-gate: BG-1161
  if (speed <= maxSpeed || speed <= 0.0001F || !std::isfinite(speed)) {
    return velocity;
  }
  const float scale = maxSpeed / speed;
  return {velocity.x * scale, 0.0F, velocity.z * scale};
}

bool horizontalVelocityActive(const ProductAppWindowState& window) {
  const float speedSquared =
      window.gameplayMovementGroundVelocityX *
          window.gameplayMovementGroundVelocityX +
      window.gameplayMovementGroundVelocityZ *
          window.gameplayMovementGroundVelocityZ;
  return speedSquared > 0.000001F;
}

Vec3 updateProductGroundMovementVelocity(ProductAppWindowState& window,
                                         float moveX,
                                         float moveY,
                                         bool sprinting) {
  const ProductGameplayMovementTuning& tuning = window.gameplayMovementTuning;
  const float dt = std::max(0.0F, tuning.inputStepSeconds);
  Vec3 current{window.gameplayMovementGroundVelocityX,
               0.0F,
               window.gameplayMovementGroundVelocityZ};
  const Vec3 target = manualFirstPersonDesiredVelocity(
      moveX, moveY, window.viewport.cameraYawDegrees, sprinting, tuning);
  const bool hasIntent = target.x != 0.0F || target.z != 0.0F;
  const float rate =
      hasIntent ? tuning.groundAccelerationMetersPerSecondSquared
                : tuning.groundDecelerationMetersPerSecondSquared;  // branch-gate: BG-1161
  const float maxSpeed =
      manualFirstPersonMaxSpeedMetersPerSecond(tuning, sprinting);
  Vec3 next = moveHorizontalVelocityToward(
      current, target, std::max(0.0F, rate) * dt);
  next = clampHorizontalVelocity(next, maxSpeed);
  window.gameplayMovementGroundVelocityX = next.x;
  window.gameplayMovementGroundVelocityZ = next.z;
  return next;
}

void recordProductAirborneMovementDebug(ProductAppWindowState& window,
                                        Vec3 start,
                                        Vec3 finalPosition) {
  const MovementTravelFacts facts =
      computeMovementTravelFacts(start, finalPosition);
  window.gameplayMovementDebugAvailable = true;
  window.gameplayMovementReasonCode = "airborne_manual_move";
  window.gameplayMovementBlockedReason = "movement_ok";
  window.gameplayMovementHitSurfaceId = "none";
  window.gameplayMovementGroundSnapApplied = false;
  window.gameplayMovementClamped = false;
  window.gameplayMovementSlid = false;
  window.gameplayMovementCollisionSweepCount = 0;
  window.gameplayMovementPolicyBand = "airborne";
  window.gameplayMovementSlopeTravelDirection =
      movementTravelDirectionName(facts.direction);
  window.gameplayMovementSlopeAngleDegrees = 0.0F;
  window.gameplayMovementSpeedMultiplier = 1.0F;
  window.gameplayMovementStartX = start.x;
  window.gameplayMovementStartY = start.y;
  window.gameplayMovementStartZ = start.z;
  window.gameplayMovementFinalX = finalPosition.x;
  window.gameplayMovementFinalY = finalPosition.y;
  window.gameplayMovementFinalZ = finalPosition.z;
  window.gameplayMovementHorizontalDistanceMeters = facts.horizontalDistanceMeters;
  window.gameplayMovementVerticalDeltaMeters = facts.verticalDeltaMeters;
  window.gameplayMovementGradePercent = facts.gradePercent;
}

void recordProductLedgeFallMovementDebug(ProductAppWindowState& window,
                                         Vec3 start,
                                         Vec3 finalPosition) {
  const MovementTravelFacts facts =
      computeMovementTravelFacts(start, finalPosition);
  window.gameplayMovementDebugAvailable = true;
  window.gameplayMovementReasonCode = "grounded_ledge_fall";
  window.gameplayMovementBlockedReason = "movement_ok";
  window.gameplayMovementHitSurfaceId = "none";
  window.gameplayMovementGroundSnapApplied = false;
  window.gameplayMovementClamped = false;
  window.gameplayMovementSlid = false;
  window.gameplayMovementCollisionSweepCount = 0;
  window.gameplayMovementPolicyBand = "falling";
  window.gameplayMovementSlopeTravelDirection =
      movementTravelDirectionName(facts.direction);
  window.gameplayMovementSlopeAngleDegrees = 0.0F;
  window.gameplayMovementSpeedMultiplier = 1.0F;
  window.gameplayMovementStartX = start.x;
  window.gameplayMovementStartY = start.y;
  window.gameplayMovementStartZ = start.z;
  window.gameplayMovementFinalX = finalPosition.x;
  window.gameplayMovementFinalY = finalPosition.y;
  window.gameplayMovementFinalZ = finalPosition.z;
  window.gameplayMovementHorizontalDistanceMeters = facts.horizontalDistanceMeters;
  window.gameplayMovementVerticalDeltaMeters = facts.verticalDeltaMeters;
  window.gameplayMovementGradePercent = facts.gradePercent;
}

void submitProductAirborneMove(Session& session,
                               ProductAppWindowState& window,
                               const EntityState& actor,
                               float moveX,
                               float moveY,
                               bool sprinting,
                               std::string_view source) {
  window.gameplayInputUsed = true;
  window.gameplayInputSource = std::string{source};
  window.gameplayCommandSubmitted = false;
  window.gameplayCommandKind = "move";
  window.gameplayCommandAccepted = false;
  window.gameplayCommandStatus = "airborne";
  window.gameplayReachGate = "not_attempted";
  window.gameplayLastRejection = "none";
  window.gameplayTickAdvanced = false;
  window.gameplayMovementAttempted = true;
  window.gameplayMovementBlocked = false;
  recordProductMovementProfile(window, sprinting);

  const Vec3 start = actor.transform.position;
  Vec3 finalPosition =
      start +
      manualFirstPersonMoveDelta(moveX,
                                 moveY,
                                 window.viewport.cameraYawDegrees,
                                 sprinting,
                                 window.gameplayMovementTuning,
                                 window.gameplayMovementTuning.airControlMultiplier);
  // branch-gate: BG-1157
  if (window.gameplayWallRunActive) {
    Vec3 wallRunDirection;
    // branch-gate: BG-1157
    if (wallRunTangentDirection(window, moveX, moveY, wallRunDirection)) {
      const float speed =
          manualFirstPersonMaxSpeedMetersPerSecond(window.gameplayMovementTuning,
                                                   sprinting) *
          std::clamp(window.gameplayMovementTuning.wallRunSpeedMultiplier,
                     0.25F,
                     2.0F);
      finalPosition = start + wallRunDirection *
                                  (speed *
                                   window.gameplayMovementTuning.inputStepSeconds);
    } else {
      clearProductWallRunActiveProof(window, "wall_run_input_away");
    }
  }
  // branch-gate: BG-1161
  if (!setProductPlayerPosition(session, actor.id, finalPosition)) {
    window.gameplayMovementBlocked = true;
    window.gameplayMovementStatus = "mutation_failed";
    window.gameplayCommandStatus = "mutation_failed";
    window.gameplayMovementReasonCode = "airborne_manual_move_mutation_failed";
    return;
  }

  window.gameplayMovementStatus = "moved";
  window.playerPositionChanged = true;
  recordProductAirborneMovementDebug(window, start, finalPosition);
  window.runtimeStateHash = session.stateHash();
}

bool productMovementDebugChangedPosition(const ProductAppWindowState& window) {
  if (!window.gameplayMovementDebugAvailable) {
    return false;
  }
  const Vec3 start{window.gameplayMovementStartX,
                   window.gameplayMovementStartY,
                   window.gameplayMovementStartZ};
  const Vec3 final{window.gameplayMovementFinalX,
                   window.gameplayMovementFinalY,
                   window.gameplayMovementFinalZ};
  return !nearlyEqual(start, final);
}

void clearProductTargetProof(ProductAppWindowState& window) {
  window.targetDiscovered = false;
  window.gameplayTargetStatus = "not_requested";
  window.gameplayTargetAction = "none";
  window.gameplayTargetEntityId = 0;
  window.gameplayTargetStableName = "none";
  window.gameplayTargetKind = "none";
  window.gameplayTargetDistanceMeters = 0.0F;
  window.gameplayTargetSupportsCommand = false;
}

void clearProductOutcomeProof(ProductAppWindowState& window) {
  window.gameplayOutcomeStatus = "not_requested";
  window.gameplayOutcomeTargetActiveAfter = false;
  window.gameplayOutcomeInventoryChanged = false;
  window.gameplayOutcomeItemId = "none";
  window.gameplayOutcomeItemCount = 0;
  window.gameplayOutcomeObjectiveChanged = false;
  window.gameplayOutcomeEventCount = 0;
}

std::uint32_t inventoryItemCount(const InventoryState& inventory,
                                 PlayerSlotId playerSlot,
                                 const std::string& itemId) {
  if (itemId.empty()) {
    return 0;
  }
  const PlayerInventory* playerInventory = findInventory(inventory, playerSlot);
  if (playerInventory == nullptr) {
    return 0;
  }
  for (const InventoryStack& stack : playerInventory->stacks) {
    if (stack.itemId == itemId) {
      return stack.count;
    }
  }
  return 0;
}

ProductInteractionOutcomeSnapshot makeProductInteractionOutcomeSnapshot(
    const Session& session,
    PlayerSlotId playerSlot,
    EntityId target) {
  ProductInteractionOutcomeSnapshot snapshot;
  snapshot.target = target;
  snapshot.playerSlot = playerSlot;
  snapshot.eventCountBefore = session.state().transient.events.size();
  const EntityState* entity = session.state().world.findById(target);
  if (entity == nullptr) {
    return snapshot;
  }
  snapshot.itemId = entity->interaction.itemId;
  snapshot.objectiveId = entity->interaction.objectiveId;
  snapshot.itemCountBefore =
      inventoryItemCount(session.state().inventory, playerSlot, snapshot.itemId);
  snapshot.objectiveCompleteBefore =
      !snapshot.objectiveId.empty() &&
      objectiveComplete(session.state().objectives, snapshot.objectiveId);
  return snapshot;
}

void recordProductInteractionOutcomeProof(
    const Session& session,
    ProductAppWindowState& window,
    const ProductInteractionOutcomeSnapshot& before) {
  const EntityState* target = session.state().world.findById(before.target);
  window.gameplayOutcomeTargetActiveAfter =
      target != nullptr && target->active;
  window.gameplayOutcomeItemId = before.itemId.empty() ? "none" : before.itemId;
  const std::uint32_t itemCountAfter =
      inventoryItemCount(session.state().inventory,
                         before.playerSlot,
                         before.itemId);
  window.gameplayOutcomeItemCount = itemCountAfter;
  window.gameplayOutcomeInventoryChanged =
      itemCountAfter != before.itemCountBefore;
  const bool objectiveCompleteAfter =
      !before.objectiveId.empty() &&
      objectiveComplete(session.state().objectives, before.objectiveId);
  window.gameplayOutcomeObjectiveChanged =
      objectiveCompleteAfter != before.objectiveCompleteBefore;
  const std::size_t eventCountAfter = session.state().transient.events.size();
  window.gameplayOutcomeEventCount =
      eventCountAfter >= before.eventCountBefore
          ? static_cast<std::uint64_t>(eventCountAfter - before.eventCountBefore)
          : 0U;

  if (!window.gameplayCommandAccepted) {
    window.gameplayOutcomeStatus = "rejected";
    return;
  }
  if (!window.gameplayTickAdvanced) {
    window.gameplayOutcomeStatus = "tick_failed";
    return;
  }
  window.gameplayOutcomeStatus = "succeeded";
}

void recordProductTargetProof(const Session& session,
                              ProductAppWindowState& window,
                              CommandKind kind,
                              const TargetQueryResult& target) {
  window.targetDiscovered = target.status == TargetQueryStatus::Found;
  window.gameplayTargetStatus = targetQueryStatusName(target.status);
  window.gameplayTargetAction = commandKindName(kind);
  window.gameplayTargetEntityId = toUint64(target.target);
  window.gameplayTargetStableName = "none";
  window.gameplayTargetKind = "none";
  window.gameplayTargetDistanceMeters = target.distanceMeters;
  window.gameplayTargetSupportsCommand = target.targetSupportsCommand;

  if (!window.targetDiscovered) {
    window.gameplayTargetEntityId = 0;
    window.gameplayTargetDistanceMeters = 0.0F;
    window.gameplayTargetSupportsCommand = false;
    return;
  }

  const EntityState* entity = session.state().world.findById(target.target);
  if (entity == nullptr) {
    window.gameplayTargetStatus = "found_missing_entity";
    return;
  }
  window.gameplayTargetStableName =
      entity->stableName.empty() ? "none" : entity->stableName;
  window.gameplayTargetKind = entityKindName(entity->kind);
}

void recordProductMovementDebug(const Session& session, ProductAppWindowState& window) {
  const SessionTransientState& transient = session.state().transient;
  if (!transient.lastMovementResultAvailable) {
    clearProductMovementDebug(window);
    return;
  }

  const MovementResult& movement = transient.lastMovementResult;
  window.gameplayMovementDebugAvailable = true;
  window.gameplayMovementReasonCode =
      movement.reasonCode.empty() ? movementBlockedReasonName(movement.blocked)
                                  : movement.reasonCode;
  window.gameplayMovementBlockedReason = movementBlockedReasonName(movement.blocked);
  window.gameplayMovementHitSurfaceId =
      movement.hitSurfaceId.empty() ? "none" : movement.hitSurfaceId;
  window.gameplayMovementGroundSnapApplied = movement.groundSnapApplied;
  window.gameplayMovementClamped = movement.movementClamped;
  window.gameplayMovementSlid = movement.movementSlid;
  window.gameplayMovementCollisionSweepCount = movement.collisionSweepCount;
  window.gameplayMovementPolicyBand =
      movement.movementPolicyBand.empty() ? "none" : movement.movementPolicyBand;
  window.gameplayMovementSlopeTravelDirection = movement.slopeTravelDirection;
  window.gameplayMovementSlopeAngleDegrees = movement.slopeAngleDegrees;
  window.gameplayMovementSpeedMultiplier = movement.speedMultiplier;
  window.gameplayMovementStartX = movement.start.x;
  window.gameplayMovementStartY = movement.start.y;
  window.gameplayMovementStartZ = movement.start.z;
  window.gameplayMovementFinalX = movement.finalPosition.x;
  window.gameplayMovementFinalY = movement.finalPosition.y;
  window.gameplayMovementFinalZ = movement.finalPosition.z;
  window.gameplayMovementHorizontalDistanceMeters = movement.horizontalDistanceMeters;
  window.gameplayMovementVerticalDeltaMeters = movement.verticalDeltaMeters;
  window.gameplayMovementGradePercent = movement.gradePercent;
}

bool commandMovementHasPhysicsFrameStats(const Session& session,
                                         CommandKind kind) {
  // branch-gate: BG-1115
  if (kind != CommandKind::Move) {
    return false;
  }
  const SessionTransientState& transient = session.state().transient;
  return transient.lastMovementResultAvailable &&
         transient.lastMovementResult.physicsFrameStatsAvailable;
}

StatusResult tickProductGameplayCommand(Session& session,
                                        ProductAppWindowState& window,
                                        const SpatialSurfaceSet* collisionSurfaces) {
  // branch-gate: BG-1115
  if (!window.physicsMovementPlannerEnabled) {
    recordProductPhysicsMovementPlannerTickProof(window, false,
                                                 collisionSurfaces != nullptr,
                                                 false);
    return session.tick(collisionSurfaces);
  }
  return session.tickWithOptions(SessionTickOptions{collisionSurfaces, true});
}

bool applyProductLedgeFallMoveFallback(Session& session,
                                       ProductAppWindowState& window,
                                       const CommandRecord& command,
                                       Vec3 before,
                                       const SpatialSurfaceSet* collisionSurfaces) {
  const bool ledgeBlockedReason =
      window.gameplayMovementBlockedReason == "no_walkable_ground" ||
      window.gameplayMovementBlockedReason == "slope_rejected";
  // branch-gate: BG-1188
  if (window.gameplayJumpActive ||
      command.kind != CommandKind::Move ||
      !command.payload.target.hasPoint ||
      !ledgeBlockedReason ||
      !playerHasNearbyGround(collisionSurfaces, before)) {
    return false;
  }

  Vec3 finalPosition = command.payload.target.point;
  finalPosition.y = before.y;
  float destinationGroundY = before.y;
  const bool hasDestinationGround =
      findHighestWalkableGroundAtOrBelow(
          collisionSurfaces, finalPosition, before.y, destinationGroundY);
  // branch-gate: BG-1188
  if (hasDestinationGround &&
      destinationGroundY >= before.y - kGameplayGroundContactToleranceMeters) {
    return false;
  }
  // branch-gate: BG-1188
  if (!setProductPlayerPosition(session, command.actor, finalPosition)) {
    return false;
  }

  window.playerPositionChanged = true;
  window.gameplayMovementBlocked = false;
  window.gameplayMovementStatus = "moved";
  recordProductLedgeFallMovementDebug(window, before, finalPosition);
  beginProductFallIfUnsupported(session, window, collisionSurfaces);
  window.runtimeStateHash = session.stateHash();
  return true;
}

void submitProductGameplayCommand(Session& session,
                                  ProductAppWindowState& window,
                                  CommandRecord command,
                                  const SpatialSurfaceSet* collisionSurfaces) {
  const EntityState* beforePlayer = productPlayerEntity(session);
  const Vec3 before = beforePlayer == nullptr ? Vec3{} : beforePlayer->transform.position;
  window.gameplayInputUsed = true;
  window.gameplayCommandSubmitted = true;
  window.gameplayCommandKind = commandKindName(command.kind);
  if (command.kind == CommandKind::Move) {
    window.gameplayMovementAttempted = true;
    window.gameplayMovementBlocked = false;
    window.gameplayMovementStatus = "submitted";
    clearProductMovementDebug(window);
  }
  window.gameplayCollisionSurfacesUsed = collisionSurfaces != nullptr;
  window.gameplayCollisionSurfaceCount =
      collisionSurfaces == nullptr
          ? 0U
          : static_cast<std::uint64_t>(collisionSurfaces->size());
  // branch-gate: BG-1115
  if (!window.physicsMovementPlannerEnabled) {
    recordProductPhysicsMovementPlannerTickProof(window, false,
                                                 collisionSurfaces != nullptr,
                                                 false);
  }

  const SessionCommandResult submitted = session.submitCommand(command);
  window.gameplayCommandAccepted =
      submitted.command.admission == CommandAdmissionStatus::Accepted;
  window.gameplayLastRejection = commandRejectionReasonName(submitted.command.rejection);
  window.gameplayReachGate = reachGateName(submitted.command.rejection);
  window.gameplayCommandStatus = window.gameplayCommandAccepted ? "accepted" : "rejected";

  if (window.gameplayCommandAccepted) {
    const StatusResult tick =
        tickProductGameplayCommand(session, window, collisionSurfaces);
    window.gameplayTickAdvanced = tick.status == ResultStatus::Ok;
    window.gameplayTickReasonCode =
        tick.status == ResultStatus::Ok
            ? "ok"
            : (tick.error.code.empty() ? "tick_failed" : tick.error.code);
    if (command.kind == CommandKind::Move) {
      recordProductMovementDebug(session, window);
    }
    recordProductPhysicsMovementPlannerTickProof(
        window,
        window.physicsMovementPlannerEnabled,
        collisionSurfaces != nullptr,
        commandMovementHasPhysicsFrameStats(session, command.kind));
  }

  const EntityState* afterPlayer = productPlayerEntity(session);
  bool movedThisCommand = false;
  if (afterPlayer != nullptr && beforePlayer != nullptr) {
    movedThisCommand = !nearlyEqual(before, afterPlayer->transform.position);
    window.playerPositionChanged = window.playerPositionChanged || movedThisCommand;
  }
  if (command.kind == CommandKind::Move && window.gameplayCommandAccepted) {
    const bool runtimeMovementBlocked =
        window.gameplayMovementDebugAvailable &&
        window.gameplayMovementBlockedReason != "movement_ok";
    const bool runtimeMovementChanged = productMovementDebugChangedPosition(window);
    window.playerPositionChanged =
        window.playerPositionChanged || movedThisCommand || runtimeMovementChanged;
    if (!window.gameplayTickAdvanced) {
      window.gameplayMovementStatus = "tick_failed";
    } else if (runtimeMovementBlocked) {
      window.gameplayMovementBlocked = true;
      window.gameplayMovementStatus = "blocked";
    } else if (movedThisCommand || runtimeMovementChanged) {
      window.gameplayMovementStatus = "moved";
    } else {
      window.gameplayMovementBlocked = true;
      window.gameplayMovementStatus = "blocked";
    }
    const bool ledgeFallFallbackApplied =
        applyProductLedgeFallMoveFallback(
            session, window, command, before, collisionSurfaces);
    // branch-gate: BG-1187
    if (!ledgeFallFallbackApplied && !window.gameplayJumpActive) {
      beginProductFallIfUnsupported(session, window, collisionSurfaces);
    }
  }
  window.runtimeStateHash = session.stateHash();
}

void submitProductMove(Session& session,
                       ProductAppWindowState& window,
                       float moveX,
                       float moveY,
                       bool sprinting,
                       std::string_view source,
                       const SpatialSurfaceSet* collisionSurfaces) {
  clearProductTargetProof(window);
  clearProductOutcomeProof(window);
  const EntityState* actor = productPlayerEntity(session);
  if (actor == nullptr) {
    window.gameplayCommandStatus = "missing_player";
    window.gameplayMovementGroundVelocityX = 0.0F;
    window.gameplayMovementGroundVelocityZ = 0.0F;
    return;
  }
  recordProductMovementProfile(window, sprinting);
  // Jump/fall owns vertical motion. While airborne, apply manual X/Z intent
  // directly so holding movement with jump does not get snapped back to ground.
  // branch-gate: BG-1161
  if (window.gameplayJumpActive) {
    window.gameplayMovementGroundVelocityX = 0.0F;
    window.gameplayMovementGroundVelocityZ = 0.0F;
    submitProductAirborneMove(session, window, *actor, moveX, moveY, sprinting, source);
    return;
  }
  const Vec3 retainedVelocity =
      updateProductGroundMovementVelocity(window, moveX, moveY, sprinting);
  const Vec3 retainedDelta =
      retainedVelocity * window.gameplayMovementTuning.inputStepSeconds;
  // branch-gate: BG-1161
  if (retainedDelta.x == 0.0F && retainedDelta.z == 0.0F) {
    return;
  }
  Vec3 destination = actor->transform.position;
  destination = destination + retainedDelta;

  CommandRecord command;
  command.playerSlot = 0;
  command.actor = actor->id;
  command.kind = CommandKind::Move;
  command.source = CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = destination;
  window.gameplayInputSource = std::string(source);
  submitProductGameplayCommand(session, window, command, collisionSurfaces);
}

void submitProductTargetCommand(Session& session,
                                ProductAppWindowState& window,
                                CommandKind kind,
                                std::string_view source,
                                const SpatialSurfaceSet* collisionSurfaces) {
  clearProductOutcomeProof(window);
  const EntityId actor = productPlayerActor(session);
  const TargetQueryResult target = queryProductGameplayTarget(session, kind);
  recordProductTargetProof(session, window, kind, target);
  if (!window.targetDiscovered) {
    window.gameplayInputUsed = true;
    window.gameplayInputSource = std::string(source);
    window.gameplayCommandKind = commandKindName(kind);
    window.gameplayCommandStatus = "no_target";
    window.gameplayReachGate = "not_attempted";
    if (kind == CommandKind::Interact) {
      window.gameplayOutcomeStatus = "no_target";
    }
    return;
  }

  const ReachQueryResult reach =
      queryReach(ReachQueryRequest{&session.state().world, actor, target.target, false, {},
                                   session.state().config.interactionRangeMeters, true});
  const CommandRejectionReason reachReason = rejectionReasonForReach(reach);
  window.gameplayReachGate = reachGateName(reachReason);

  CommandRecord command;
  command.playerSlot = 0;
  command.actor = actor;
  command.kind = kind;
  command.source = CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target.target;
  if (kind == CommandKind::Attack) {
    command.payload.attackDamage = 3;
  }
  const ProductInteractionOutcomeSnapshot outcomeBefore =
      kind == CommandKind::Interact
          ? makeProductInteractionOutcomeSnapshot(session,
                                                  command.playerSlot,
                                                  target.target)
          : ProductInteractionOutcomeSnapshot{};
  window.gameplayInputSource = std::string(source);
  submitProductGameplayCommand(session, window, command, collisionSurfaces);
  if (kind == CommandKind::Interact) {
    recordProductInteractionOutcomeProof(session, window, outcomeBefore);
  }
  if (kind == CommandKind::Interact && window.gameplayCommandAccepted &&
      window.gameplayTickAdvanced) {
    window.interactionExecuted = true;
    if (window.activeRoom.loaded) {
      window.activeRoomCollision =
          buildProductActiveRoomCollision(window.activeRoom, session.state());
    }
  }
  if (kind == CommandKind::Attack && window.gameplayCommandAccepted &&
      window.gameplayTickAdvanced) {
    window.attackExecuted = true;
  }
}

}  // namespace

void applyProductGameplayActions(Session& session,
                                 const ActionState& actions,
                                 ProductAppWindowState& window,
                                 std::string_view source,
                                 const SpatialSurfaceSet* collisionSurfaces) {
  const float moveX = actionAxisValue(actions, InputAction::PlayerMoveX);
  const float moveY = actionAxisValue(actions, InputAction::PlayerMoveY);
  const bool sprinting = actionIsDown(actions, InputAction::PlayerSprint);
  const bool jumpPressed = actionWasPressed(actions, InputAction::PlayerJump);
  const bool jumpReleased = actionWasReleased(actions, InputAction::PlayerJump);
  advanceProductDashCooldown(window);
  // branch-gate: BG-1153
  if (jumpPressed) {
    // branch-gate: BG-1157
    if (window.gameplayWallRunActive) {
      clearProductWallRunActiveProof(window, "wall_run_exit_jump");
    }
    submitProductJump(session, window, source);
  } else {
    // branch-gate: BG-1153
    if (jumpReleased) {
      applyProductJumpReleaseCut(window);
    }
    advanceProductJump(session, window, collisionSurfaces);
  }
  // branch-gate: BG-1155
  if (actionWasPressed(actions, InputAction::PlayerDash)) {
    submitProductDash(session, window, moveX, moveY, source, collisionSurfaces);
    updateProductMovementStateProof(window);
    updateProductWallRunCandidateProof(session, window, collisionSurfaces);
    updateProductWallRunActiveProof(window, moveX, moveY);
    // branch-gate: BG-1157
    if (jumpPressed) {
      clearProductWallRunActiveProof(window, "wall_run_exit_jump");
    }
    updateProductMovementStateProof(window);
    return;
  }
  if (moveX != 0.0F || moveY != 0.0F || horizontalVelocityActive(window)) {
    submitProductMove(session, window, moveX, moveY, sprinting, source,
                      collisionSurfaces);
  }
  if (actionWasPressed(actions, InputAction::PlayerInteract)) {
    submitProductTargetCommand(session, window, CommandKind::Interact, source,
                               collisionSurfaces);
  }
  if (actionWasPressed(actions, InputAction::PlayerAttack)) {
    submitProductTargetCommand(session, window, CommandKind::Attack, source,
                               collisionSurfaces);
  }
  if (actionWasPressed(actions, InputAction::PlayerRetryOrReset)) {
    const SessionResetResult reset = session.resetToBaseline();
    clearProductTargetProof(window);
    clearProductOutcomeProof(window);
    window.gameplayInputUsed = true;
    window.gameplayInputSource = std::string(source);
    window.gameplayCommandKind = "reset";
    window.gameplayCommandSubmitted = true;
    window.gameplayCommandAccepted = reset.reset;
    window.gameplayCommandStatus = reset.reset ? "accepted" : "rejected";
    window.runtimeStateHash = session.stateHash();
  }
  updateProductMovementStateProof(window);
  updateProductWallRunCandidateProof(session, window, collisionSurfaces);
  updateProductWallRunActiveProof(window, moveX, moveY);
  // branch-gate: BG-1157
  if (jumpPressed) {
    clearProductWallRunActiveProof(window, "wall_run_exit_jump");
  }
  updateProductMovementStateProof(window);
}

}  // namespace iggy3d

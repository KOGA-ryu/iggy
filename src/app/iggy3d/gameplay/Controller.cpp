#include "app/iggy3d/gameplay/Controller.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>

#include "app/input/ActionState.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ControllerGroundQueries.hpp"
#include "app/iggy3d/gameplay/ControllerKinematics.hpp"
#include "app/iggy3d/gameplay/ControllerMovementProof.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
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
constexpr float kGameplayResetBelowLowestFloorMeters = 6.0F;
constexpr float kGameplayResetZoneRadiusMeters = 0.70F;
constexpr float kGameplayResetZoneVerticalToleranceMeters = 1.20F;
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

struct ProductWallRunCandidateEvaluationResult {
  bool available = false;
  std::string status = "wall_run_not_checked";
  std::string reasonCode = "wall_run_not_checked";
  std::string side = "none";
  std::string surfaceId = "none";
  Vec3 normal{0.0F, 0.0F, 0.0F};
  float approachSpeedMetersPerSecond = 0.0F;
};

struct ProductWallRunActiveEvaluationResult {
  bool active = false;
  std::string status = "wall_run_inactive";
  std::string reasonCode = "wall_run_inactive";
  float remainingSeconds = 0.0F;
  float durationSeconds = 0.0F;
  float gravityMultiplier = 1.0F;
  float speedMultiplier = 1.0F;
};

struct ProductWallRunEvaluationRequest {
  const Session& session;
  const ProductAppWindowState& window;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  float moveX = 0.0F;
  float moveY = 0.0F;
  bool jumpPressed = false;
};

struct ProductWallRunEvaluationResult {
  ProductWallRunCandidateEvaluationResult candidate;
  ProductWallRunActiveEvaluationResult active;
};

ProductWallRunCandidateEvaluationResult productWallRunCandidateRejected(
    std::string_view reason) {
  ProductWallRunCandidateEvaluationResult result;
  result.status = std::string{reason};
  result.reasonCode = std::string{reason};
  return result;
}

ProductWallRunActiveEvaluationResult productWallRunActiveRejected(
    std::string_view reason) {
  ProductWallRunActiveEvaluationResult result;
  result.status = std::string{reason};
  result.reasonCode = std::string{reason};
  return result;
}

ProductWallRunActiveEvaluationResult productWallRunActiveRecorded(
    const ProductAppWindowState& window,
    std::string_view status,
    std::string_view reason,
    float remainingSeconds) {
  ProductWallRunActiveEvaluationResult result;
  result.active = true;
  result.status = std::string{status};
  result.reasonCode = std::string{reason};
  result.remainingSeconds = remainingSeconds;
  result.durationSeconds =
      std::clamp(window.gameplay.gameplayMovement.tuning.wallRunDurationSeconds, 0.1F, 2.0F);
  result.gravityMultiplier =
      std::clamp(window.gameplay.gameplayMovement.tuning.wallRunGravityMultiplier, 0.0F, 1.0F);
  result.speedMultiplier =
      std::clamp(window.gameplay.gameplayMovement.tuning.wallRunSpeedMultiplier, 0.25F, 2.0F);
  return result;
}

void publishProductWallRunCandidateEvaluation(
    ProductAppWindowState& window,
    const ProductWallRunCandidateEvaluationResult& result) {
  window.gameplay.gameplayWallRun.candidateAvailable = result.available;
  window.gameplay.gameplayWallRun.candidateStatus = result.status;
  window.gameplay.gameplayWallRun.candidateReasonCode = result.reasonCode;
  window.gameplay.gameplayWallRun.side = result.side;
  window.gameplay.gameplayWallRun.surfaceId = result.surfaceId;
  window.gameplay.gameplayWallRun.normalX = result.normal.x;
  window.gameplay.gameplayWallRun.normalY = result.normal.y;
  window.gameplay.gameplayWallRun.normalZ = result.normal.z;
  window.gameplay.gameplayWallRun.approachSpeedMetersPerSecond =
      result.approachSpeedMetersPerSecond;
}

void publishProductWallRunActiveEvaluation(
    ProductAppWindowState& window,
    const ProductWallRunActiveEvaluationResult& result) {
  window.gameplay.gameplayWallRun.active = result.active;
  window.gameplay.gameplayWallRun.status = result.status;
  window.gameplay.gameplayWallRun.reasonCode = result.reasonCode;
  window.gameplay.gameplayWallRun.remainingSeconds = result.remainingSeconds;
  window.gameplay.gameplayWallRun.durationSeconds = result.durationSeconds;
  window.gameplay.gameplayWallRun.gravityMultiplier = result.gravityMultiplier;
  window.gameplay.gameplayWallRun.speedMultiplier = result.speedMultiplier;
}

void publishProductWallRunEvaluation(
    ProductAppWindowState& window,
    const ProductWallRunEvaluationResult& result) {
  publishProductWallRunCandidateEvaluation(window, result.candidate);
  publishProductWallRunActiveEvaluation(window, result.active);
}

void clearProductWallRunActiveProof(ProductAppWindowState& window,
                                    std::string_view reason) {
  publishProductWallRunActiveEvaluation(
      window, productWallRunActiveRejected(reason));
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

void recordProductJumpPosition(ProductAppWindowState& window,
                               float groundY,
                               float startY,
                               float finalY) {
  window.gameplay.gameplayJump.groundY = groundY;
  window.gameplay.gameplayJump.startY = startY;
  window.gameplay.gameplayJump.finalY = finalY;
  window.gameplay.gameplayJump.heightMeters = std::max(0.0F, finalY - groundY);
}

void clearProductJumpTiming(ProductAppWindowState& window) {
  window.gameplay.gameplayJump.coyoteSecondsRemaining = 0.0F;
  window.gameplay.gameplayJump.bufferSecondsRemaining = 0.0F;
  window.gameplay.gameplayJump.held = false;
  window.gameplay.gameplayJump.cutApplied = false;
}

void rejectProductJump(ProductAppWindowState& window,
                       std::string_view status,
                       std::string_view reason) {
  window.gameplay.gameplayJump.requested = true;
  window.gameplay.gameplayJump.accepted = false;
  window.gameplay.gameplayJump.status = std::string{status};
  window.gameplay.gameplayJump.reasonCode = std::string{reason};
}

bool productJumpBufferLive(const ProductAppWindowState& window) {
  return window.gameplay.gameplayJump.bufferSecondsRemaining > 0.0F;
}

void bufferProductJump(ProductAppWindowState& window) {
  window.gameplay.gameplayJump.bufferSecondsRemaining =
      window.gameplay.gameplayMovement.tuning.jumpBufferSeconds;
}

void beginProductJumpArc(Session& session,
                         ProductAppWindowState& window,
                         const EntityState& actor,
                         float groundY,
                         std::string_view reason) {
  const ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  window.gameplay.gameplayJump.accepted = true;
  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond =
      tuning.jumpImpulseMetersPerSecond;
  window.gameplay.gameplayJump.coyoteSecondsRemaining = 0.0F;
  window.gameplay.gameplayJump.bufferSecondsRemaining = 0.0F;
  window.gameplay.gameplayJump.held = true;
  window.gameplay.gameplayJump.cutApplied = false;
  recordProductJumpPosition(window, groundY, actor.transform.position.y,
                            actor.transform.position.y);
  window.gameplay.gameplayJump.status = "accepted";
  window.gameplay.gameplayJump.reasonCode = std::string{reason};
  advanceProductJump(session,
                     window,
                     productActiveRoomCollisionSurfaces(activeRoomCollision(window)));
}

bool tryProductCoyoteJump(Session& session, ProductAppWindowState& window) {
  // branch-gate: BG-1153
  if (!window.gameplay.gameplayJump.active ||
      window.gameplay.gameplayJump.coyoteSecondsRemaining <= 0.0F) {
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
                      window.gameplay.gameplayJump.groundY,
                      "gameplay_jump_coyote");
  return true;
}

void applyProductJumpReleaseCut(ProductAppWindowState& window) {
  ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  window.gameplay.gameplayJump.held = false;
  // branch-gate: BG-1153
  if (!window.gameplay.gameplayJump.active || window.gameplay.gameplayJump.cutApplied ||
      window.gameplay.gameplayJump.velocityMetersPerSecond <= 0.0F) {
    return;
  }
  const float multiplier = std::clamp(tuning.jumpCutMultiplier, 0.1F, 1.0F);
  window.gameplay.gameplayJump.velocityMetersPerSecond *= multiplier;
  window.gameplay.gameplayJump.cutApplied = true;
}

void clearProductTraversalProof(ProductAppWindowState& window) {
  window.gameplay.gameplayTraversal.requested = false;
  window.gameplay.gameplayTraversal.consumed = false;
  window.gameplay.gameplayTraversal.accepted = false;
  window.gameplay.gameplayTraversal.fallbackJumpAllowed = false;
  window.gameplay.gameplayTraversal.status = "not_requested";
  window.gameplay.gameplayTraversal.reasonCode = "not_requested";
  window.gameplay.gameplayTraversal.mechanic = "none";
  window.gameplay.gameplayTraversal.slotId = "none";
  window.gameplay.gameplayTraversal.targetId = "none";
  window.gameplay.gameplayTraversal.landingSurfaceId = "none";
  window.gameplay.gameplayTraversal.startX = 0.0F;
  window.gameplay.gameplayTraversal.startY = 0.0F;
  window.gameplay.gameplayTraversal.startZ = 0.0F;
  window.gameplay.gameplayTraversal.finalX = 0.0F;
  window.gameplay.gameplayTraversal.finalY = 0.0F;
  window.gameplay.gameplayTraversal.finalZ = 0.0F;
}

void recordProductTraversalProof(ProductAppWindowState& window,
                                 const TraversalIntentResult& result) {
  window.gameplay.gameplayTraversal.requested = result.requested;
  window.gameplay.gameplayTraversal.consumed = result.consumedInput;
  window.gameplay.gameplayTraversal.accepted = result.accepted;
  window.gameplay.gameplayTraversal.fallbackJumpAllowed = result.fallbackJumpAllowed;
  window.gameplay.gameplayTraversal.status = traversalIntentStatusName(result.status);
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.reasonCode =
      result.reasonCode == nullptr ? "unknown" : result.reasonCode;
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.mechanic =
      result.traversalAttempted ? traversalMechanicName(result.selectedMechanic)
                                : "none";
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.slotId =
      result.traversal.slotId.empty() ? "none" : result.traversal.slotId;
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.targetId =
      result.traversal.targetId.empty() ? "none" : result.traversal.targetId;
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.landingSurfaceId =
      result.traversal.landingSurfaceId.empty() ? "none"
                                                : result.traversal.landingSurfaceId;
  window.gameplay.gameplayTraversal.startX = result.traversal.start.x;
  window.gameplay.gameplayTraversal.startY = result.traversal.start.y;
  window.gameplay.gameplayTraversal.startZ = result.traversal.start.z;
  window.gameplay.gameplayTraversal.finalX = result.traversal.finalPosition.x;
  window.gameplay.gameplayTraversal.finalY = result.traversal.finalPosition.y;
  window.gameplay.gameplayTraversal.finalZ = result.traversal.finalPosition.z;
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
  Vec3 travel{window.gameplay.gameplayMovement.finalX - window.gameplay.gameplayMovement.startX,
              0.0F,
              window.gameplay.gameplayMovement.finalZ - window.gameplay.gameplayMovement.startZ};
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
  normal = {window.gameplay.gameplayWallRun.normalX, 0.0F, window.gameplay.gameplayWallRun.normalZ};
  const float lenSq = lengthSquared(normal);
  // branch-gate: BG-1157
  if (!isFinite(normal) || lenSq <= 0.0001F) {
    return false;
  }
  normal = normal / std::sqrt(lenSq);
  return true;
}

bool wallRunTangentDirectionFromNormal(const ProductAppWindowState& window,
                                       Vec3 normal,
                                       float moveX,
                                       float moveY,
                                       Vec3& direction) {
  normal.y = 0.0F;
  const float lenSq = lengthSquared(normal);
  // branch-gate: BG-1157
  if (!isFinite(normal) || lenSq <= 0.0001F) {
    return false;
  }
  normal = normal / std::sqrt(lenSq);
  const Vec3 desired = productManualFirstPersonDirection(
      moveX, moveY, window.viewport.cameraYawDegrees);
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

bool wallRunTangentDirection(const ProductAppWindowState& window,
                             float moveX,
                             float moveY,
                             Vec3& direction) {
  Vec3 normal;
  // branch-gate: BG-1157
  if (!wallRunProofNormal(window, normal)) {
    return false;
  }
  return wallRunTangentDirectionFromNormal(
      window, normal, moveX, moveY, direction);
}

ProductWallRunActiveEvaluationResult evaluateProductWallRunActiveWithoutJumpExit(
    const ProductWallRunEvaluationRequest& request,
    const ProductWallRunCandidateEvaluationResult& candidate) {
  const ProductAppWindowState& window = request.window;
  const bool hasMoveInput = request.moveX != 0.0F || request.moveY != 0.0F;
  // branch-gate: BG-1157
  if (window.gameplay.gameplayWallRun.active && !window.gameplay.gameplayJump.active) {
    return productWallRunActiveRejected("wall_run_landed");
  }
  // branch-gate: BG-1157
  if (window.gameplay.gameplayWallRun.active && !hasMoveInput) {
    return productWallRunActiveRejected("wall_run_input_stopped");
  }
  Vec3 tangent;
  // branch-gate: BG-1157
  if (window.gameplay.gameplayWallRun.active &&
      !wallRunTangentDirectionFromNormal(
          window, candidate.normal, request.moveX, request.moveY, tangent)) {
    return productWallRunActiveRejected("wall_run_input_away");
  }
  // branch-gate: BG-1157
  if (window.gameplay.gameplayWallRun.active && !candidate.available) {
    return productWallRunActiveRejected(candidate.reasonCode);
  }
  // branch-gate: BG-1157
  if (!window.gameplay.gameplayWallRun.active && (!candidate.available || !hasMoveInput)) {
    return productWallRunActiveRejected("wall_run_inactive");
  }
  // branch-gate: BG-1157
  if (!window.gameplay.gameplayWallRun.active &&
      !wallRunTangentDirectionFromNormal(
          window, candidate.normal, request.moveX, request.moveY, tangent)) {
    return productWallRunActiveRejected("wall_run_input_away");
  }

  const float dt = std::max(0.0F, window.gameplay.gameplayMovement.tuning.inputStepSeconds);
  // branch-gate: BG-1157
  if (!window.gameplay.gameplayWallRun.active) {
    const float remaining =
        std::clamp(window.gameplay.gameplayMovement.tuning.wallRunDurationSeconds,
                   0.1F,
                   2.0F);
    ProductWallRunActiveEvaluationResult result =
        productWallRunActiveRecorded(
            window, "wall_run_active", "wall_run_started", remaining);
    return result;
  }

  const float remaining =
      std::max(0.0F, window.gameplay.gameplayWallRun.remainingSeconds - dt);
  // branch-gate: BG-1157
  if (remaining <= 0.0F) {
    return productWallRunActiveRejected("wall_run_expired");
  }
  ProductWallRunActiveEvaluationResult result =
      productWallRunActiveRecorded(
          window, "wall_run_active", "wall_run_active", remaining);
  return result;
}

ProductWallRunCandidateEvaluationResult evaluateProductWallRunCandidate(
    const ProductWallRunEvaluationRequest& request) {
  const ProductAppWindowState& window = request.window;
  ProductWallRunCandidateEvaluationResult result;
  result.approachSpeedMetersPerSecond =
      window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond;

  // branch-gate: BG-1153
  if (!window.gameplay.gameplayJump.active) {
    return productWallRunCandidateRejected("wall_run_grounded");
  }
  const float minSpeed =
      std::max(0.0F, window.gameplay.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond);
  // branch-gate: BG-1161
  if (window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond < minSpeed) {
    result = productWallRunCandidateRejected("wall_run_low_speed");
    result.approachSpeedMetersPerSecond =
        window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond;
    return result;
  }
  // branch-gate: BG-1157
  if (request.collisionSurfaces == nullptr) {
    return productWallRunCandidateRejected("wall_run_no_surfaces");
  }
  const EntityState* actor = productPlayerEntity(request.session);
  // branch-gate: BG-1153
  if (actor == nullptr) {
    return productWallRunCandidateRejected("wall_run_missing_player");
  }

  Vec3 awayNormal;
  const CollisionSurfaceView* surface =
      findWallRunSurface(*request.collisionSurfaces,
                         actor->transform.position,
                         awayNormal,
                         window.gameplay.gameplayMovement.tuning);
  // branch-gate: BG-1157
  if (surface == nullptr) {
    result = productWallRunCandidateRejected("wall_run_no_wall_contact");
    result.approachSpeedMetersPerSecond =
        window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond;
    return result;
  }
  // branch-gate: BG-1161
  if (!productMovementDebugAlongWall(window, awayNormal)) {
    result = productWallRunCandidateRejected("wall_run_not_along_wall");
    // branch-gate: BG-1157
    result.surfaceId = surface->id.empty() ? "wall_run_surface" : surface->id;
    result.normal = {awayNormal.x, surface->normal.y, awayNormal.z};
    result.approachSpeedMetersPerSecond =
        window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond;
    return result;
  }

  result.available = true;
  result.status = "wall_run_candidate";
  result.reasonCode = "wall_run_candidate";
  result.side = wallRunSideName(awayNormal, window.viewport.cameraYawDegrees);
  // branch-gate: BG-1157
  result.surfaceId = surface->id.empty() ? "wall_run_surface" : surface->id;
  result.normal = {awayNormal.x, surface->normal.y, awayNormal.z};
  result.approachSpeedMetersPerSecond =
      window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond;
  return result;
}

ProductWallRunEvaluationResult evaluateProductWallRun(
    const ProductWallRunEvaluationRequest& request) {
  ProductWallRunEvaluationResult result;
  result.candidate = evaluateProductWallRunCandidate(request);
  result.active = evaluateProductWallRunActiveWithoutJumpExit(
      request, result.candidate);
  // branch-gate: BG-1157
  if (request.jumpPressed) {
    result.active = productWallRunActiveRejected("wall_run_exit_jump");
  }
  return result;
}

void recordProductWallJumpTraversalProof(ProductAppWindowState& window,
                                         const CollisionSurfaceView& surface,
                                         Vec3 start,
                                         Vec3 finalPosition) {
  window.gameplay.gameplayTraversal.requested = true;
  window.gameplay.gameplayTraversal.consumed = true;
  window.gameplay.gameplayTraversal.accepted = true;
  window.gameplay.gameplayTraversal.fallbackJumpAllowed = false;
  window.gameplay.gameplayTraversal.status = "traversal_intent_applied";
  window.gameplay.gameplayTraversal.reasonCode = "traversal_intent_applied";
  window.gameplay.gameplayTraversal.mechanic = "wall_jump";
  window.gameplay.gameplayTraversal.slotId = "wall_jump";
  window.gameplay.gameplayTraversal.landingSurfaceId = "wall_jump_surface";
  // branch-gate: BG-1157
  if (!surface.id.empty()) {
    window.gameplay.gameplayTraversal.slotId = surface.id;
    window.gameplay.gameplayTraversal.landingSurfaceId = surface.id;
  }
  window.gameplay.gameplayTraversal.targetId = window.gameplay.gameplayTraversal.slotId;
  // branch-gate: BG-1157
  if (!surface.runtimeOwnerStableName.empty()) {
    window.gameplay.gameplayTraversal.targetId = surface.runtimeOwnerStableName;
  }
  window.gameplay.gameplayTraversal.startX = start.x;
  window.gameplay.gameplayTraversal.startY = start.y;
  window.gameplay.gameplayTraversal.startZ = start.z;
  window.gameplay.gameplayTraversal.finalX = finalPosition.x;
  window.gameplay.gameplayTraversal.finalY = finalPosition.y;
  window.gameplay.gameplayTraversal.finalZ = finalPosition.z;
}

bool tryProductWallJump(Session& session, ProductAppWindowState& window) {
  const ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  const SpatialSurfaceSet* surfaces =
      productActiveRoomCollisionSurfaces(activeRoomCollision(window));
  const EntityId actor = productPlayerActor(session);
  const EntityState* entity = session.state().world.findById(actor);
  // branch-gate: BG-1157
  if (surfaces == nullptr || entity == nullptr) {
    return false;
  }

  const Vec3 start = entity->transform.position;
  // branch-gate: BG-1157
  if (!window.gameplay.gameplayJump.active &&
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
  window.gameplay.gameplayJump.accepted = true;
  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond =
      tuning.jumpImpulseMetersPerSecond;
  recordProductJumpPosition(window, 0.0F, start.y, finalPosition.y);
  window.gameplay.gameplayJump.status = "wall_jump";
  window.gameplay.gameplayJump.reasonCode = "gameplay_jump_wall_jump";
  window.gameplay.playerPositionChanged = true;
  return true;
}

bool tryProductTraversalJump(Session& session, ProductAppWindowState& window) {
  clearProductTraversalProof(window);
  const SpatialSurfaceSet* surfaces =
      productActiveRoomCollisionSurfaces(activeRoomCollision(window));
  // branch-gate: BG-1156
  if (!activeRoom(window).loaded || surfaces == nullptr) {
    return false;
  }

  TraversalIntentRequest request;
  request.actor = productPlayerActor(session);
  request.jumpPressed = true;
  request.forward = productManualFirstPersonDirection(
      0.0F, 1.0F, window.viewport.cameraYawDegrees);
  request.room = &activeRoom(window).room;
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
  }
  // branch-gate: BG-1156
  if (result.accepted) {
    window.gameplay.playerPositionChanged = true;
  }
  return result.consumedInput;
}

void advanceProductJump(Session& session,
                        ProductAppWindowState& window,
                        const SpatialSurfaceSet* collisionSurfaces) {
  const ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  const float dt = std::max(0.0F, tuning.inputStepSeconds);
  window.gameplay.gameplayJump.bufferSecondsRemaining =
      std::max(0.0F, window.gameplay.gameplayJump.bufferSecondsRemaining - dt);
  window.gameplay.gameplayJump.coyoteSecondsRemaining =
      std::max(0.0F, window.gameplay.gameplayJump.coyoteSecondsRemaining - dt);
  // branch-gate: BG-1181
  if (applyProductGameplayResetIfNeeded(session, window, collisionSurfaces)) {
    return;
  }
  // branch-gate: BG-1173
  if (!window.gameplay.gameplayJump.active &&
      !beginProductFallIfUnsupported(session, window, collisionSurfaces)) {
    return;
  }

  const EntityId actor = productPlayerActor(session);
  const EntityState* entity = session.state().world.findById(actor);
  // branch-gate: BG-1153
  if (entity == nullptr) {
    window.gameplay.gameplayJump.active = false;
    window.gameplay.gameplayJump.velocityMetersPerSecond = 0.0F;
    window.gameplay.gameplayJump.status = "missing_player";
    window.gameplay.gameplayJump.reasonCode = "gameplay_jump_missing_player";
    return;
  }

  const float previousY = entity->transform.position.y;
  float gravityMultiplier = 1.0F;
  // branch-gate: BG-1153
  if (window.gameplay.gameplayJump.velocityMetersPerSecond <= 0.0F) {
    // branch-gate: BG-1157
    gravityMultiplier =
        window.gameplay.gameplayWallRun.active
            ? std::clamp(tuning.wallRunGravityMultiplier, 0.0F, 1.0F)
            : tuning.fallGravityMultiplier;
  }
  const float gravity =
      tuning.gravityMetersPerSecondSquared *
      std::clamp(gravityMultiplier, 0.0F, 4.0F);
  const float nextVelocity =
      window.gameplay.gameplayJump.velocityMetersPerSecond - gravity * dt;
  float nextY = previousY +
                window.gameplay.gameplayJump.velocityMetersPerSecond * dt -
                0.5F * gravity * dt * dt;
  bool landed = false;
  float landingY = window.gameplay.gameplayJump.groundY;
  const bool collisionLanding =
      nextVelocity <= 0.0F &&
      findHighestWalkableGroundAtOrBelow(
          collisionSurfaces,
          entity->transform.position,
          previousY,
          landingY) &&
      nextY <= landingY + kProductGameplayGroundContactToleranceMeters;
  if ((collisionSurfaces == nullptr && nextY <= window.gameplay.gameplayJump.groundY &&
       nextVelocity <= 0.0F) ||
      collisionLanding) {
    nextY = landingY;
    landed = true;
  }

  Vec3 position = entity->transform.position;
  position.y = nextY;
  // branch-gate: BG-1153
  if (!setProductPlayerPosition(session, actor, position)) {
    window.gameplay.gameplayJump.active = false;
    window.gameplay.gameplayJump.velocityMetersPerSecond = 0.0F;
    window.gameplay.gameplayJump.status = "mutation_failed";
    window.gameplay.gameplayJump.reasonCode = "gameplay_jump_mutation_failed";
    return;
  }

  window.gameplay.playerPositionChanged =
      window.gameplay.playerPositionChanged || std::fabs(previousY - nextY) > 0.0001F;
  window.gameplay.gameplayJump.active = !landed;
  // branch-gate: BG-1153
  window.gameplay.gameplayJump.velocityMetersPerSecond = landed ? 0.0F : nextVelocity;
  // branch-gate: BG-1153
  if (landed) {
    clearProductWallRunActiveProof(window, "wall_run_landed");
    window.gameplay.gameplayJump.coyoteSecondsRemaining = 0.0F;
    window.gameplay.gameplayJump.held = false;
    window.gameplay.gameplayJump.cutApplied = false;
  }
  const std::array<float, 2U> groundProofYs{window.gameplay.gameplayJump.groundY, landingY};
  recordProductJumpPosition(
      window,
      groundProofYs[static_cast<std::size_t>(landed || collisionLanding)],
      window.gameplay.gameplayJump.startY,
      nextY);
  // branch-gate: BG-1153
  window.gameplay.gameplayJump.status = landed ? "landed" : "airborne";
  // branch-gate: BG-1153
  window.gameplay.gameplayJump.reasonCode =
      landed ? "gameplay_jump_landed" : "gameplay_jump_airborne";
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
  window.gameplay.gameplayInputUsed = true;
  window.gameplay.gameplayInputSource = std::string{source};
  window.gameplay.gameplayJump.requested = true;

  // branch-gate: BG-1156
  if (tryProductTraversalJump(session, window)) {
    window.gameplay.gameplayJump.accepted = false;
    // branch-gate: BG-1156
    window.gameplay.gameplayJump.status = window.gameplay.gameplayTraversal.accepted ? "traversal"
                                                                 : "traversal_rejected";
    window.gameplay.gameplayJump.reasonCode = window.gameplay.gameplayTraversal.reasonCode;
    return;
  }

  // branch-gate: BG-1157
  if (tryProductWallJump(session, window)) {
    return;
  }

  // branch-gate: BG-1153
  if (window.gameplay.gameplayJump.active) {
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

void advanceProductDashCooldown(ProductAppWindowState& window) {
  const ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  // branch-gate: BG-1155
  if (window.gameplay.gameplayDash.cooldownRemainingSeconds <= 0.0F) {
    window.gameplay.gameplayDash.cooldownRemainingSeconds = 0.0F;
    return;
  }
  window.gameplay.gameplayDash.cooldownRemainingSeconds =
      std::max(0.0F,
               window.gameplay.gameplayDash.cooldownRemainingSeconds -
                   tuning.inputStepSeconds);
}

void rejectProductDash(ProductAppWindowState& window,
                       std::string_view status,
                       std::string_view reason) {
  window.gameplay.gameplayDash.requested = true;
  window.gameplay.gameplayDash.accepted = false;
  window.gameplay.gameplayDash.status = std::string{status};
  window.gameplay.gameplayDash.reasonCode = std::string{reason};
}

void submitProductDash(Session& session,
                       ProductAppWindowState& window,
                       float moveX,
                       float moveY,
                       std::string_view source,
                       const SpatialSurfaceSet* collisionSurfaces) {
  const ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  clearProductTargetProof(window);
  clearProductOutcomeProof(window);
  window.gameplay.gameplayInputUsed = true;
  window.gameplay.gameplayInputSource = std::string{source};
  window.gameplay.gameplayDash.requested = true;

  // branch-gate: BG-1155
  if (window.gameplay.gameplayDash.cooldownRemainingSeconds > 0.0F) {
    rejectProductDash(window, "cooldown", "gameplay_dash_cooldown");
    return;
  }

  const EntityState* actor = productPlayerEntity(session);
  // branch-gate: BG-1155
  if (actor == nullptr) {
    rejectProductDash(window, "missing_player", "gameplay_dash_missing_player");
    return;
  }

  const Vec3 direction = productManualFirstPersonDirection(
      moveX, moveY, window.viewport.cameraYawDegrees);
  const float dashDistance = tuning.dashSpeedMetersPerSecond *
                             tuning.dashDurationSeconds;
  Vec3 destination = actor->transform.position + direction * dashDistance;
  destination.y = actor->transform.position.y;

  window.gameplay.gameplayDash.accepted = true;
  window.gameplay.gameplayDash.status = "accepted";
  window.gameplay.gameplayDash.reasonCode = "gameplay_dash_accepted";
  window.gameplay.gameplayDash.speedMetersPerSecond = tuning.dashSpeedMetersPerSecond;
  window.gameplay.gameplayDash.distanceMeters = dashDistance;
  window.gameplay.gameplayDash.cooldownRemainingSeconds =
      tuning.dashCooldownSeconds;
  window.gameplay.gameplayDash.directionX = direction.x;
  window.gameplay.gameplayDash.directionZ = direction.z;
  window.gameplay.gameplayMovement.profile = std::string{tuning.dashProfile};
  window.gameplay.gameplayMovement.maxSpeedMetersPerSecond =
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

bool horizontalVelocityActive(const ProductAppWindowState& window) {
  const float speedSquared =
      window.gameplay.gameplayMovement.groundVelocityX *
          window.gameplay.gameplayMovement.groundVelocityX +
      window.gameplay.gameplayMovement.groundVelocityZ *
          window.gameplay.gameplayMovement.groundVelocityZ;
  return speedSquared > 0.000001F;
}

Vec3 updateProductGroundMovementVelocity(ProductAppWindowState& window,
                                         float moveX,
                                         float moveY,
                                         bool sprinting) {
  const ProductGameplayMovementTuning& tuning = window.gameplay.gameplayMovement.tuning;
  const float dt = std::max(0.0F, tuning.inputStepSeconds);
  Vec3 current{window.gameplay.gameplayMovement.groundVelocityX,
               0.0F,
               window.gameplay.gameplayMovement.groundVelocityZ};
  const Vec3 target = productManualFirstPersonDesiredVelocity(
      moveX, moveY, window.viewport.cameraYawDegrees, sprinting, tuning);
  const bool hasIntent = target.x != 0.0F || target.z != 0.0F;
  const float rate =
      hasIntent ? tuning.groundAccelerationMetersPerSecondSquared
                : tuning.groundDecelerationMetersPerSecondSquared;  // branch-gate: BG-1161
  const float maxSpeed =
      productManualFirstPersonMaxSpeedMetersPerSecond(tuning, sprinting);
  Vec3 next = moveProductHorizontalVelocityToward(
      current, target, std::max(0.0F, rate) * dt);
  next = clampProductHorizontalVelocity(next, maxSpeed);
  window.gameplay.gameplayMovement.groundVelocityX = next.x;
  window.gameplay.gameplayMovement.groundVelocityZ = next.z;
  return next;
}

void submitProductAirborneMove(Session& session,
                               ProductAppWindowState& window,
                               const EntityState& actor,
                               float moveX,
                               float moveY,
                               bool sprinting,
                               std::string_view source) {
  window.gameplay.gameplayInputUsed = true;
  window.gameplay.gameplayInputSource = std::string{source};
  window.gameplay.gameplayCommand.submitted = false;
  window.gameplay.gameplayCommand.kind = "move";
  window.gameplay.gameplayCommand.accepted = false;
  window.gameplay.gameplayCommand.status = "airborne";
  window.gameplay.gameplayReachGate = "not_attempted";
  window.gameplay.gameplayLastRejection = "none";
  window.gameplay.gameplayTickAdvanced = false;
  window.gameplay.gameplayMovement.attempted = true;
  window.gameplay.gameplayMovement.blocked = false;
  recordProductMovementProfile(window, sprinting);

  const Vec3 start = actor.transform.position;
  Vec3 finalPosition =
      start +
      productManualFirstPersonMoveDelta(
          moveX,
          moveY,
          window.viewport.cameraYawDegrees,
          sprinting,
          window.gameplay.gameplayMovement.tuning,
          window.gameplay.gameplayMovement.tuning.airControlMultiplier);
  // branch-gate: BG-1157
  if (window.gameplay.gameplayWallRun.active) {
    Vec3 wallRunDirection;
    // branch-gate: BG-1157
    if (wallRunTangentDirection(window, moveX, moveY, wallRunDirection)) {
      const float speed =
          productManualFirstPersonMaxSpeedMetersPerSecond(
              window.gameplay.gameplayMovement.tuning, sprinting) *
          std::clamp(window.gameplay.gameplayMovement.tuning.wallRunSpeedMultiplier,
                     0.25F,
                     2.0F);
      finalPosition = start + wallRunDirection *
                                  (speed *
                                   window.gameplay.gameplayMovement.tuning.inputStepSeconds);
    } else {
      clearProductWallRunActiveProof(window, "wall_run_input_away");
    }
  }
  // branch-gate: BG-1161
  if (!setProductPlayerPosition(session, actor.id, finalPosition)) {
    window.gameplay.gameplayMovement.blocked = true;
    window.gameplay.gameplayMovement.status = "mutation_failed";
    window.gameplay.gameplayCommand.status = "mutation_failed";
    window.gameplay.gameplayMovement.reasonCode = "airborne_manual_move_mutation_failed";
    return;
  }

  window.gameplay.gameplayMovement.status = "moved";
  window.gameplay.playerPositionChanged = true;
  recordProductAirborneMovementDebug(window, start, finalPosition);
}

void clearProductTargetProof(ProductAppWindowState& window) {
  window.gameplay.targetDiscovered = false;
  window.gameplay.gameplayTarget.status = "not_requested";
  window.gameplay.gameplayTarget.action = "none";
  window.gameplay.gameplayTarget.entityId = 0;
  window.gameplay.gameplayTarget.stableName = "none";
  window.gameplay.gameplayTarget.kind = "none";
  window.gameplay.gameplayTarget.distanceMeters = 0.0F;
  window.gameplay.gameplayTarget.supportsCommand = false;
}

void clearProductOutcomeProof(ProductAppWindowState& window) {
  window.gameplay.gameplayOutcome.status = "not_requested";
  window.gameplay.gameplayOutcome.targetActiveAfter = false;
  window.gameplay.gameplayOutcome.inventoryChanged = false;
  window.gameplay.gameplayOutcome.itemId = "none";
  window.gameplay.gameplayOutcome.itemCount = 0;
  window.gameplay.gameplayOutcome.objectiveChanged = false;
  window.gameplay.gameplayOutcome.eventCount = 0;
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
  window.gameplay.gameplayOutcome.targetActiveAfter =
      target != nullptr && target->active;
  window.gameplay.gameplayOutcome.itemId = before.itemId.empty() ? "none" : before.itemId;
  const std::uint32_t itemCountAfter =
      inventoryItemCount(session.state().inventory,
                         before.playerSlot,
                         before.itemId);
  window.gameplay.gameplayOutcome.itemCount = itemCountAfter;
  window.gameplay.gameplayOutcome.inventoryChanged =
      itemCountAfter != before.itemCountBefore;
  const bool objectiveCompleteAfter =
      !before.objectiveId.empty() &&
      objectiveComplete(session.state().objectives, before.objectiveId);
  window.gameplay.gameplayOutcome.objectiveChanged =
      objectiveCompleteAfter != before.objectiveCompleteBefore;
  const std::size_t eventCountAfter = session.state().transient.events.size();
  window.gameplay.gameplayOutcome.eventCount =
      eventCountAfter >= before.eventCountBefore
          ? static_cast<std::uint64_t>(eventCountAfter - before.eventCountBefore)
          : 0U;

  if (!window.gameplay.gameplayCommand.accepted) {
    window.gameplay.gameplayOutcome.status = "rejected";
    return;
  }
  if (!window.gameplay.gameplayTickAdvanced) {
    window.gameplay.gameplayOutcome.status = "tick_failed";
    return;
  }
  window.gameplay.gameplayOutcome.status = "succeeded";
}

void recordProductTargetProof(const Session& session,
                              ProductAppWindowState& window,
                              CommandKind kind,
                              const TargetQueryResult& target) {
  window.gameplay.targetDiscovered = target.status == TargetQueryStatus::Found;
  window.gameplay.gameplayTarget.status = targetQueryStatusName(target.status);
  window.gameplay.gameplayTarget.action = commandKindName(kind);
  window.gameplay.gameplayTarget.entityId = toUint64(target.target);
  window.gameplay.gameplayTarget.stableName = "none";
  window.gameplay.gameplayTarget.kind = "none";
  window.gameplay.gameplayTarget.distanceMeters = target.distanceMeters;
  window.gameplay.gameplayTarget.supportsCommand = target.targetSupportsCommand;

  if (!window.gameplay.targetDiscovered) {
    window.gameplay.gameplayTarget.entityId = 0;
    window.gameplay.gameplayTarget.distanceMeters = 0.0F;
    window.gameplay.gameplayTarget.supportsCommand = false;
    return;
  }

  const EntityState* entity = session.state().world.findById(target.target);
  if (entity == nullptr) {
    window.gameplay.gameplayTarget.status = "found_missing_entity";
    return;
  }
  window.gameplay.gameplayTarget.stableName =
      entity->stableName.empty() ? "none" : entity->stableName;
  window.gameplay.gameplayTarget.kind = entityKindName(entity->kind);
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
  if (!window.gameplay.physicsMovementPlanner.enabled) {
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
      window.gameplay.gameplayMovement.blockedReason == "no_walkable_ground" ||
      window.gameplay.gameplayMovement.blockedReason == "slope_rejected";
  // branch-gate: BG-1188
  if (window.gameplay.gameplayJump.active ||
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
      destinationGroundY >= before.y - kProductGameplayGroundContactToleranceMeters) {
    return false;
  }
  // branch-gate: BG-1188
  if (!setProductPlayerPosition(session, command.actor, finalPosition)) {
    return false;
  }

  window.gameplay.playerPositionChanged = true;
  window.gameplay.gameplayMovement.blocked = false;
  window.gameplay.gameplayMovement.status = "moved";
  recordProductLedgeFallMovementDebug(window, before, finalPosition);
  beginProductFallIfUnsupported(session, window, collisionSurfaces);
  return true;
}

void submitProductGameplayCommand(Session& session,
                                  ProductAppWindowState& window,
                                  CommandRecord command,
                                  const SpatialSurfaceSet* collisionSurfaces) {
  const EntityState* beforePlayer = productPlayerEntity(session);
  const Vec3 before = beforePlayer == nullptr ? Vec3{} : beforePlayer->transform.position;
  window.gameplay.gameplayInputUsed = true;
  window.gameplay.gameplayCommand.submitted = true;
  window.gameplay.gameplayCommand.kind = commandKindName(command.kind);
  if (command.kind == CommandKind::Move) {
    window.gameplay.gameplayMovement.attempted = true;
    window.gameplay.gameplayMovement.blocked = false;
    window.gameplay.gameplayMovement.status = "submitted";
    clearProductMovementDebug(window);
  }
  window.gameplay.gameplayCollision.surfacesUsed = collisionSurfaces != nullptr;
  window.gameplay.gameplayCollision.surfaceCount =
      collisionSurfaces == nullptr
          ? 0U
          : static_cast<std::uint64_t>(collisionSurfaces->size());
  // branch-gate: BG-1115
  if (!window.gameplay.physicsMovementPlanner.enabled) {
    recordProductPhysicsMovementPlannerTickProof(window, false,
                                                 collisionSurfaces != nullptr,
                                                 false);
  }

  const SessionCommandResult submitted = session.submitCommand(command);
  window.gameplay.gameplayCommand.accepted =
      submitted.command.admission == CommandAdmissionStatus::Accepted;
  window.gameplay.gameplayLastRejection = commandRejectionReasonName(submitted.command.rejection);
  window.gameplay.gameplayReachGate = reachGateName(submitted.command.rejection);
  window.gameplay.gameplayCommand.status = window.gameplay.gameplayCommand.accepted ? "accepted" : "rejected";

  if (window.gameplay.gameplayCommand.accepted) {
    const StatusResult tick =
        tickProductGameplayCommand(session, window, collisionSurfaces);
    window.gameplay.gameplayTickAdvanced = tick.status == ResultStatus::Ok;
    window.gameplay.gameplayTickReasonCode =
        tick.status == ResultStatus::Ok
            ? "ok"
            : (tick.error.code.empty() ? "tick_failed" : tick.error.code);
    if (command.kind == CommandKind::Move) {
      recordProductMovementDebug(session, window);
    }
    recordProductPhysicsMovementPlannerTickProof(
        window,
        window.gameplay.physicsMovementPlanner.enabled,
        collisionSurfaces != nullptr,
        commandMovementHasPhysicsFrameStats(session, command.kind));
  }

  const EntityState* afterPlayer = productPlayerEntity(session);
  bool movedThisCommand = false;
  if (afterPlayer != nullptr && beforePlayer != nullptr) {
    movedThisCommand = !nearlyEqual(before, afterPlayer->transform.position);
    window.gameplay.playerPositionChanged = window.gameplay.playerPositionChanged || movedThisCommand;
  }
  if (command.kind == CommandKind::Move && window.gameplay.gameplayCommand.accepted) {
    const bool runtimeMovementBlocked =
        window.gameplay.gameplayMovement.debugAvailable &&
        window.gameplay.gameplayMovement.blockedReason != "movement_ok";
    const bool runtimeMovementChanged = productMovementDebugChangedPosition(window);
    window.gameplay.playerPositionChanged =
        window.gameplay.playerPositionChanged || movedThisCommand || runtimeMovementChanged;
    if (!window.gameplay.gameplayTickAdvanced) {
      window.gameplay.gameplayMovement.status = "tick_failed";
    } else if (runtimeMovementBlocked) {
      window.gameplay.gameplayMovement.blocked = true;
      window.gameplay.gameplayMovement.status = "blocked";
    } else if (movedThisCommand || runtimeMovementChanged) {
      window.gameplay.gameplayMovement.status = "moved";
    } else {
      window.gameplay.gameplayMovement.blocked = true;
      window.gameplay.gameplayMovement.status = "blocked";
    }
    const bool ledgeFallFallbackApplied =
        applyProductLedgeFallMoveFallback(
            session, window, command, before, collisionSurfaces);
    // branch-gate: BG-1187
    if (!ledgeFallFallbackApplied && !window.gameplay.gameplayJump.active) {
      beginProductFallIfUnsupported(session, window, collisionSurfaces);
    }
  }
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
    window.gameplay.gameplayCommand.status = "missing_player";
    window.gameplay.gameplayMovement.groundVelocityX = 0.0F;
    window.gameplay.gameplayMovement.groundVelocityZ = 0.0F;
    return;
  }
  recordProductMovementProfile(window, sprinting);
  // Jump/fall owns vertical motion. While airborne, apply manual X/Z intent
  // directly so holding movement with jump does not get snapped back to ground.
  // branch-gate: BG-1161
  if (window.gameplay.gameplayJump.active) {
    window.gameplay.gameplayMovement.groundVelocityX = 0.0F;
    window.gameplay.gameplayMovement.groundVelocityZ = 0.0F;
    submitProductAirborneMove(session, window, *actor, moveX, moveY, sprinting, source);
    return;
  }
  const Vec3 retainedVelocity =
      updateProductGroundMovementVelocity(window, moveX, moveY, sprinting);
  const Vec3 retainedDelta =
      retainedVelocity * window.gameplay.gameplayMovement.tuning.inputStepSeconds;
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
  window.gameplay.gameplayInputSource = std::string(source);
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
  if (!window.gameplay.targetDiscovered) {
    window.gameplay.gameplayInputUsed = true;
    window.gameplay.gameplayInputSource = std::string(source);
    window.gameplay.gameplayCommand.kind = commandKindName(kind);
    window.gameplay.gameplayCommand.status = "no_target";
    window.gameplay.gameplayReachGate = "not_attempted";
    if (kind == CommandKind::Interact) {
      window.gameplay.gameplayOutcome.status = "no_target";
    }
    return;
  }

  const ReachQueryResult reach =
      queryReach(ReachQueryRequest{&session.state().world, actor, target.target, false, {},
                                   session.state().config.interactionRangeMeters, true});
  const CommandRejectionReason reachReason = rejectionReasonForReach(reach);
  window.gameplay.gameplayReachGate = reachGateName(reachReason);

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
  window.gameplay.gameplayInputSource = std::string(source);
  submitProductGameplayCommand(session, window, command, collisionSurfaces);
  if (kind == CommandKind::Interact) {
    recordProductInteractionOutcomeProof(session, window, outcomeBefore);
  }
  if (kind == CommandKind::Interact && window.gameplay.gameplayCommand.accepted &&
      window.gameplay.gameplayTickAdvanced) {
    window.gameplay.interactionExecuted = true;
  }
  if (kind == CommandKind::Attack && window.gameplay.gameplayCommand.accepted &&
      window.gameplay.gameplayTickAdvanced) {
    window.gameplay.attackExecuted = true;
  }
}

struct ProductGameplayInputIntent {
  float moveX = 0.0F;
  float moveY = 0.0F;
  bool sprinting = false;
  bool jumpPressed = false;
  bool jumpReleased = false;
  bool dashPressed = false;
  bool interactPressed = false;
  bool attackPressed = false;
  bool resetPressed = false;
};

ProductGameplayInputIntent sampleProductGameplayInputIntent(
    const ActionState& actions) {
  ProductGameplayInputIntent intent;
  intent.moveX = actionAxisValue(actions, InputAction::PlayerMoveX);
  intent.moveY = actionAxisValue(actions, InputAction::PlayerMoveY);
  intent.sprinting = actionIsDown(actions, InputAction::PlayerSprint);
  intent.jumpPressed = actionWasPressed(actions, InputAction::PlayerJump);
  intent.jumpReleased = actionWasReleased(actions, InputAction::PlayerJump);
  intent.dashPressed = actionWasPressed(actions, InputAction::PlayerDash);
  intent.interactPressed = actionWasPressed(actions, InputAction::PlayerInteract);
  intent.attackPressed = actionWasPressed(actions, InputAction::PlayerAttack);
  intent.resetPressed =
      actionWasPressed(actions, InputAction::PlayerRetryOrReset);
  return intent;
}

bool productGameplayIntentHasMovement(
    const ProductGameplayInputIntent& intent,
    const ProductAppWindowState& window) {
  return intent.moveX != 0.0F || intent.moveY != 0.0F ||
         horizontalVelocityActive(window);
}

void updateProductJumpTimingPhase(Session& session,
                                  ProductAppWindowState& window,
                                  const ProductGameplayInputIntent& intent,
                                  std::string_view source,
                                  const SpatialSurfaceSet* collisionSurfaces) {
  advanceProductDashCooldown(window);
  // branch-gate: BG-1153
  if (intent.jumpPressed) {
    // branch-gate: BG-1157
    if (window.gameplay.gameplayWallRun.active) {
      clearProductWallRunActiveProof(window, "wall_run_exit_jump");
    }
    submitProductJump(session, window, source);
    return;
  }

  // branch-gate: BG-1153
  if (intent.jumpReleased) {
    applyProductJumpReleaseCut(window);
  }
  advanceProductJump(session, window, collisionSurfaces);
}

ProductWallRunEvaluationResult resolveProductWallRunCandidatePhase(
    const Session& session,
    const ProductAppWindowState& window,
    const ProductGameplayInputIntent& intent,
    const SpatialSurfaceSet* collisionSurfaces) {
  return evaluateProductWallRun(ProductWallRunEvaluationRequest{
      session, window, collisionSurfaces, intent.moveX, intent.moveY,
      intent.jumpPressed});
}

void applyProductActiveMovementStatePhase(
    ProductAppWindowState& window,
    const ProductWallRunEvaluationResult& wallRun) {
  publishProductWallRunEvaluation(window, wallRun);
}

void publishProductMovementProofPhase(
    const Session& session,
    ProductAppWindowState& window,
    const ProductGameplayInputIntent& intent,
    const SpatialSurfaceSet* collisionSurfaces) {
  updateProductMovementStateProof(window);
  const ProductWallRunEvaluationResult wallRun =
      resolveProductWallRunCandidatePhase(session, window, intent, collisionSurfaces);
  applyProductActiveMovementStatePhase(window, wallRun);
  updateProductMovementStateProof(window);
}

bool applyProductDashPhase(Session& session,
                           ProductAppWindowState& window,
                           const ProductGameplayInputIntent& intent,
                           std::string_view source,
                           const SpatialSurfaceSet* collisionSurfaces) {
  // branch-gate: BG-1155
  if (!intent.dashPressed) {
    return false;
  }
  submitProductDash(session,
                    window,
                    intent.moveX,
                    intent.moveY,
                    source,
                    collisionSurfaces);
  publishProductMovementProofPhase(session, window, intent, collisionSurfaces);
  return true;
}

void updateProductRetainedHorizontalVelocityPhase(
    Session& session,
    ProductAppWindowState& window,
    const ProductGameplayInputIntent& intent,
    std::string_view source,
    const SpatialSurfaceSet* collisionSurfaces) {
  // branch-gate: BG-1161
  if (!productGameplayIntentHasMovement(intent, window)) {
    return;
  }
  submitProductMove(session,
                    window,
                    intent.moveX,
                    intent.moveY,
                    intent.sprinting,
                    source,
                    collisionSurfaces);
}

void applyProductTargetActionPhase(Session& session,
                                   ProductAppWindowState& window,
                                   const ProductGameplayInputIntent& intent,
                                   std::string_view source,
                                   const SpatialSurfaceSet* collisionSurfaces) {
  // branch-gate: BG-1155
  if (intent.interactPressed) {
    submitProductTargetCommand(session,
                               window,
                               CommandKind::Interact,
                               source,
                               collisionSurfaces);
  }
  // branch-gate: BG-1155
  if (intent.attackPressed) {
    submitProductTargetCommand(session,
                               window,
                               CommandKind::Attack,
                               source,
                               collisionSurfaces);
  }
}

void applyProductResetActionPhase(Session& session,
                                  ProductAppWindowState& window,
                                  const ProductGameplayInputIntent& intent,
                                  std::string_view source) {
  // branch-gate: BG-1155
  if (!intent.resetPressed) {
    return;
  }
  const SessionResetResult reset = session.resetToBaseline();
  clearProductTargetProof(window);
  clearProductOutcomeProof(window);
  window.gameplay.gameplayInputUsed = true;
  window.gameplay.gameplayInputSource = std::string(source);
  window.gameplay.gameplayCommand.kind = "reset";
  window.gameplay.gameplayCommand.submitted = true;
  window.gameplay.gameplayCommand.accepted = reset.reset;
  window.gameplay.gameplayCommand.status = reset.reset ? "accepted" : "rejected";  // branch-gate: BG-1155
}

}  // namespace

void applyProductGameplayActions(Session& session,
                                 const ActionState& actions,
                                 ProductAppWindowState& window,
                                 std::string_view source,
                                 const SpatialSurfaceSet* collisionSurfaces) {
  const ProductGameplayInputIntent intent =
      sampleProductGameplayInputIntent(actions);
  updateProductJumpTimingPhase(session, window, intent, source, collisionSurfaces);
  if (applyProductDashPhase(
          session, window, intent, source, collisionSurfaces)) {
    return;
  }
  updateProductRetainedHorizontalVelocityPhase(
      session, window, intent, source, collisionSurfaces);
  applyProductTargetActionPhase(
      session, window, intent, source, collisionSurfaces);
  applyProductResetActionPhase(session, window, intent, source);
  publishProductMovementProofPhase(session, window, intent, collisionSurfaces);
}

}  // namespace iggy3d

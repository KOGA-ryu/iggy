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

float manualFirstPersonMaxSpeedMetersPerSecond(bool sprinting) {
  const ProductGameplayMovementTuning& tuning = productGameplayMovementTuning();
  const std::array<float, 2U> speeds{tuning.walkSpeedMetersPerSecond,
                                     tuning.sprintSpeedMetersPerSecond};
  return speeds[static_cast<std::size_t>(sprinting)];
}

std::string_view manualFirstPersonMovementProfile(bool sprinting) {
  const ProductGameplayMovementTuning& tuning = productGameplayMovementTuning();
  const std::array<std::string_view, 2U> profiles{tuning.walkProfile,
                                                  tuning.sprintProfile};
  return profiles[static_cast<std::size_t>(sprinting)];
}

void recordProductMovementProfile(ProductAppWindowState& window, bool sprinting) {
  window.gameplayMovementProfile =
      std::string{manualFirstPersonMovementProfile(sprinting)};
  window.gameplayMovementMaxSpeedMetersPerSecond =
      manualFirstPersonMaxSpeedMetersPerSecond(sprinting);
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

void recordProductJumpPosition(ProductAppWindowState& window,
                               float groundY,
                               float startY,
                               float finalY) {
  window.gameplayJumpGroundY = groundY;
  window.gameplayJumpStartY = startY;
  window.gameplayJumpFinalY = finalY;
  window.gameplayJumpHeightMeters = std::max(0.0F, finalY - groundY);
}

void rejectProductJump(ProductAppWindowState& window,
                       std::string_view status,
                       std::string_view reason) {
  window.gameplayJumpRequested = true;
  window.gameplayJumpAccepted = false;
  window.gameplayJumpStatus = std::string{status};
  window.gameplayJumpReasonCode = std::string{reason};
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
                           float& distanceSq) {
  const ProductGameplayMovementTuning& tuning = productGameplayMovementTuning();
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
                                                Vec3& awayNormal) {
  const ProductGameplayMovementTuning& tuning = productGameplayMovementTuning();
  const CollisionSurfaceView* best = nullptr;
  float bestDistanceSq = tuning.wallJumpProbeMeters * tuning.wallJumpProbeMeters;
  for (const CollisionSurfaceView& surface : surfaces.surfaces()) {
    Vec3 candidateNormal;
    float candidateDistanceSq = 0.0F;
    // branch-gate: BG-1157
    if (!isNearVerticalSurface(position, surface, candidateNormal,
                               candidateDistanceSq)) {
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
  const ProductGameplayMovementTuning& tuning = productGameplayMovementTuning();
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
      findWallJumpSurface(*surfaces, start, awayNormal);
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
  const ProductGameplayMovementTuning& tuning = productGameplayMovementTuning();
  if (!window.gameplayJumpActive) {
    const EntityState* groundedEntity = productPlayerEntity(session);
    // branch-gate: BG-1173
    if (groundedEntity == nullptr ||
        collisionSurfaces == nullptr ||
        playerHasNearbyGround(collisionSurfaces, groundedEntity->transform.position)) {
      return;
    }
    window.gameplayJumpRequested = false;
    window.gameplayJumpAccepted = false;
    window.gameplayJumpActive = true;
    window.gameplayJumpVelocityMetersPerSecond = 0.0F;
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
  const float nextVelocity = window.gameplayJumpVelocityMetersPerSecond -
                             tuning.gravityMetersPerSecondSquared *
                                 tuning.inputStepSeconds;
  float nextY = previousY +
                window.gameplayJumpVelocityMetersPerSecond *
                    tuning.inputStepSeconds -
                0.5F * tuning.gravityMetersPerSecondSquared *
                    tuning.inputStepSeconds *
                    tuning.inputStepSeconds;
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
}

void submitProductJump(Session& session,
                       ProductAppWindowState& window,
                       std::string_view source) {
  const ProductGameplayMovementTuning& tuning = productGameplayMovementTuning();
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

  const float groundY = actor->transform.position.y;
  window.gameplayJumpAccepted = true;
  window.gameplayJumpActive = true;
  window.gameplayJumpVelocityMetersPerSecond =
      tuning.jumpImpulseMetersPerSecond;
  recordProductJumpPosition(window, groundY, groundY, groundY);
  window.gameplayJumpStatus = "accepted";
  window.gameplayJumpReasonCode = "gameplay_jump_accepted";
  advanceProductJump(session,
                     window,
                     productActiveRoomCollisionSurfaces(window.activeRoomCollision));
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
  const ProductGameplayMovementTuning& tuning = productGameplayMovementTuning();
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
  const ProductGameplayMovementTuning& tuning = productGameplayMovementTuning();
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
                                bool sprinting) {
  const ProductGameplayMovementTuning& tuning = productGameplayMovementTuning();
  const float magnitude = std::sqrt(moveX * moveX + moveY * moveY);
  const float scale = 1.0F / std::max(1.0F, magnitude);
  const float stepMeters = manualFirstPersonMaxSpeedMetersPerSecond(sprinting) *
                           tuning.inputStepSeconds;
  const float yawRadians = yawDegrees * kPi / 180.0F;
  const float cosYaw = std::cos(yawRadians);
  const float sinYaw = std::sin(yawRadians);
  const Vec3 forward{sinYaw, 0.0F, -cosYaw};
  const Vec3 right{cosYaw, 0.0F, sinYaw};
  return (right * moveX + forward * moveY) * (scale * stepMeters);
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
  const Vec3 finalPosition =
      start +
      manualFirstPersonMoveDelta(moveX,
                                 moveY,
                                 window.viewport.cameraYawDegrees,
                                 sprinting);
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
    return;
  }
  if (moveX == 0.0F && moveY == 0.0F) {
    return;
  }
  recordProductMovementProfile(window, sprinting);
  // Jump/fall owns vertical motion. While airborne, apply manual X/Z intent
  // directly so holding movement with jump does not get snapped back to ground.
  // branch-gate: BG-1161
  if (window.gameplayJumpActive) {
    submitProductAirborneMove(session, window, *actor, moveX, moveY, sprinting, source);
    return;
  }
  Vec3 destination = actor->transform.position;
  destination = destination + manualFirstPersonMoveDelta(
                                  moveX,
                                  moveY,
                                  window.viewport.cameraYawDegrees,
                                  sprinting);

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
  advanceProductDashCooldown(window);
  // branch-gate: BG-1153
  if (actionWasPressed(actions, InputAction::PlayerJump)) {
    submitProductJump(session, window, source);
  } else {
    advanceProductJump(session, window, collisionSurfaces);
  }
  // branch-gate: BG-1155
  if (actionWasPressed(actions, InputAction::PlayerDash)) {
    submitProductDash(session, window, moveX, moveY, source, collisionSurfaces);
    return;
  }
  if (moveX != 0.0F || moveY != 0.0F) {
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
}

}  // namespace iggy3d

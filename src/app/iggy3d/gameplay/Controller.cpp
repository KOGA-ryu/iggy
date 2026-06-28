#include "app/iggy3d/gameplay/Controller.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

#include "app/input/ActionState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
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

constexpr std::string_view kManualFirstPersonMovementProfile = "manual_first_person";
constexpr std::string_view kManualFirstPersonSprintMovementProfile =
    "manual_first_person_sprint";
constexpr float kManualFirstPersonMaxSpeedMetersPerSecond = 1.6F;
constexpr float kManualFirstPersonSprintMaxSpeedMetersPerSecond = 3.2F;
constexpr float kManualFirstPersonInputStepSeconds = 1.0F / 60.0F;
constexpr float kManualFirstPersonJumpImpulseMetersPerSecond = 5.8F;
constexpr float kManualFirstPersonGravityMetersPerSecondSquared = 18.0F;
constexpr std::string_view kManualFirstPersonDashMovementProfile =
    "manual_first_person_dash";
constexpr float kManualFirstPersonDashSpeedMetersPerSecond = 9.5F;
constexpr float kManualFirstPersonDashDurationSeconds = 0.18F;
constexpr float kManualFirstPersonDashCooldownSeconds = 0.45F;
constexpr float kPi = 3.14159265358979323846F;

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
  // branch-gate: BG-1151
  return sprinting ? kManualFirstPersonSprintMaxSpeedMetersPerSecond
                   : kManualFirstPersonMaxSpeedMetersPerSecond;
}

std::string_view manualFirstPersonMovementProfile(bool sprinting) {
  // branch-gate: BG-1151
  return sprinting ? kManualFirstPersonSprintMovementProfile
                   : kManualFirstPersonMovementProfile;
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

void advanceProductJump(Session& session, ProductAppWindowState& window) {
  // branch-gate: BG-1153
  if (!window.gameplayJumpActive) {
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
  const float nextVelocity = window.gameplayJumpVelocityMetersPerSecond -
                             kManualFirstPersonGravityMetersPerSecondSquared *
                                 kManualFirstPersonInputStepSeconds;
  float nextY = previousY +
                window.gameplayJumpVelocityMetersPerSecond *
                    kManualFirstPersonInputStepSeconds -
                0.5F * kManualFirstPersonGravityMetersPerSecondSquared *
                    kManualFirstPersonInputStepSeconds *
                    kManualFirstPersonInputStepSeconds;
  bool landed = false;
  // branch-gate: BG-1153
  if (nextY <= window.gameplayJumpGroundY && nextVelocity <= 0.0F) {
    nextY = window.gameplayJumpGroundY;
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
  recordProductJumpPosition(window,
                            window.gameplayJumpGroundY,
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
      kManualFirstPersonJumpImpulseMetersPerSecond;
  recordProductJumpPosition(window, groundY, groundY, groundY);
  window.gameplayJumpStatus = "accepted";
  window.gameplayJumpReasonCode = "gameplay_jump_accepted";
  advanceProductJump(session, window);
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
  // branch-gate: BG-1155
  if (window.gameplayDashCooldownRemainingSeconds <= 0.0F) {
    window.gameplayDashCooldownRemainingSeconds = 0.0F;
    return;
  }
  window.gameplayDashCooldownRemainingSeconds =
      std::max(0.0F,
               window.gameplayDashCooldownRemainingSeconds -
                   kManualFirstPersonInputStepSeconds);
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
  const float dashDistance = kManualFirstPersonDashSpeedMetersPerSecond *
                             kManualFirstPersonDashDurationSeconds;
  Vec3 destination = actor->transform.position + direction * dashDistance;
  destination.y = actor->transform.position.y;

  window.gameplayDashAccepted = true;
  window.gameplayDashStatus = "accepted";
  window.gameplayDashReasonCode = "gameplay_dash_accepted";
  window.gameplayDashSpeedMetersPerSecond =
      kManualFirstPersonDashSpeedMetersPerSecond;
  window.gameplayDashDistanceMeters = dashDistance;
  window.gameplayDashCooldownRemainingSeconds =
      kManualFirstPersonDashCooldownSeconds;
  window.gameplayDashDirectionX = direction.x;
  window.gameplayDashDirectionZ = direction.z;
  window.gameplayMovementProfile =
      std::string{kManualFirstPersonDashMovementProfile};
  window.gameplayMovementMaxSpeedMetersPerSecond =
      kManualFirstPersonDashSpeedMetersPerSecond;

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
  const float magnitude = std::sqrt(moveX * moveX + moveY * moveY);
  const float scale = 1.0F / std::max(1.0F, magnitude);
  const float stepMeters = manualFirstPersonMaxSpeedMetersPerSecond(sprinting) *
                           kManualFirstPersonInputStepSeconds;
  const float yawRadians = yawDegrees * kPi / 180.0F;
  const float cosYaw = std::cos(yawRadians);
  const float sinYaw = std::sin(yawRadians);
  const Vec3 forward{sinYaw, 0.0F, -cosYaw};
  const Vec3 right{cosYaw, 0.0F, sinYaw};
  return (right * moveX + forward * moveY) * (scale * stepMeters);
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
    advanceProductJump(session, window);
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

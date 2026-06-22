#include "runtime/debug/RuntimeDebugSnapshot.hpp"

#include <cmath>

#include "runtime/movement/MovementTraversal.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {
namespace {

float vectorLength(Vec3 value) {
  return std::sqrt(lengthSquared(value));
}

Vec3 horizontal(Vec3 value) {
  return {value.x, 0.0F, value.z};
}

std::string noneIfEmpty(const std::string& value) {
  return value.empty() ? "none" : value;
}

std::string unavailableIfNull(const char* value) {
  return value == nullptr ? "unavailable" : value;
}

EntityId actorFromRequest(const RuntimeDebugSnapshotRequest& request) {
  if (isValid(request.actor)) {
    return request.actor;
  }
  if (request.session == nullptr) {
    return kInvalidEntityId;
  }
  for (const PlayerSlot& slot : request.session->players.slots()) {
    if (isActorControllingSlotKind(slot.kind) && isValid(slot.actor)) {
      return slot.actor;
    }
  }
  return kInvalidEntityId;
}

void copyMotorFacts(const RuntimeDebugSnapshotRequest& request, RuntimeDebugSnapshot& snapshot) {
  if (request.motorState != nullptr) {
    snapshot.grounded = request.motorState->grounded;
    snapshot.jumpAvailable = request.motorState->jumpAvailable;
    snapshot.motorPhase = request.motorState->phase;
    snapshot.horizontalSpeedMetersPerSecond =
        vectorLength(request.motorState->horizontalVelocityMetersPerSecond);
    snapshot.verticalSpeedMetersPerSecond =
        request.motorState->verticalVelocityMetersPerSecond;
    snapshot.dashCooldownRemainingSeconds =
        request.motorState->dashCooldownRemainingSeconds;
  }

  if (request.motorResult != nullptr && playerMotorSucceeded(*request.motorResult)) {
    snapshot.grounded = request.motorResult->grounded;
    snapshot.motorPhase = request.motorResult->phase;
    snapshot.horizontalSpeedMetersPerSecond =
        request.motorResult->horizontalSpeedMetersPerSecond;
    snapshot.verticalSpeedMetersPerSecond =
        request.motorResult->verticalVelocityMetersPerSecond;
    snapshot.dashCooldownRemainingSeconds =
        request.motorResult->dashCooldownRemainingSeconds;
    snapshot.groundSampleValid = request.motorResult->groundSampleValid;
    snapshot.groundContact = request.motorResult->groundContact;
    snapshot.groundWalkable = request.motorResult->groundWalkable;
    snapshot.carefulFooting = request.motorResult->carefulFooting;
    snapshot.groundNormal = request.motorResult->groundNormal;
    snapshot.groundDistanceMeters = request.motorResult->groundDistanceMeters;
    snapshot.slopeAngleDegrees = request.motorResult->slopeAngleDegrees;
    snapshot.slopeUpDot = request.motorResult->slopeUpDot;
    snapshot.speedMultiplier = request.motorResult->speedMultiplier;
    snapshot.staminaCostMultiplier = request.motorResult->staminaCostMultiplier;
    snapshot.stepPenaltyMultiplier = request.motorResult->stepPenaltyMultiplier;
    if (!request.motorResult->movementPolicyBand.empty()) {
      snapshot.movementPolicyBand = request.motorResult->movementPolicyBand;
    }
    if (!request.motorResult->groundSurfaceId.empty()) {
      snapshot.groundSurfaceId = request.motorResult->groundSurfaceId;
    }
    if (!request.motorResult->hitSurfaceId.empty()) {
      snapshot.hitSurfaceId = request.motorResult->hitSurfaceId;
    }
  }
}

void copyMovementFacts(const RuntimeDebugSnapshotRequest& request,
                       RuntimeDebugSnapshot& snapshot) {
  if (request.movementResult == nullptr) {
    return;
  }
  if (!request.movementResult->movementPolicyBand.empty()) {
    snapshot.movementPolicyBand = request.movementResult->movementPolicyBand;
  }
  snapshot.groundSampleValid =
      snapshot.groundSampleValid || !request.movementResult->movementPolicyBand.empty();
  snapshot.groundWalkable =
      request.movementResult->slopeUpDot > 0.0F &&
      request.movementResult->speedMultiplier > 0.0F &&
      request.movementResult->movementPolicyBand != "blocked";
  snapshot.carefulFooting = request.movementResult->carefulFooting;
  snapshot.slopeAngleDegrees = request.movementResult->slopeAngleDegrees;
  snapshot.slopeUpDot = request.movementResult->slopeUpDot;
  snapshot.speedMultiplier = request.movementResult->speedMultiplier;
  snapshot.staminaCostMultiplier = request.movementResult->staminaCostMultiplier;
  snapshot.stepPenaltyMultiplier = request.movementResult->stepPenaltyMultiplier;
  snapshot.movementHorizontalDistanceMeters =
      request.movementResult->horizontalDistanceMeters;
  snapshot.movementVerticalDeltaMeters = request.movementResult->verticalDeltaMeters;
  snapshot.movementGradePercent = request.movementResult->gradePercent;
  snapshot.slopeTravelDirection = request.movementResult->slopeTravelDirection;
  if (!request.movementResult->hitSurfaceId.empty()) {
    snapshot.hitSurfaceId = request.movementResult->hitSurfaceId;
  }
}

void copyTraversalResultFacts(const TraversalResult& result,
                              RuntimeDebugSnapshot& snapshot) {
  snapshot.traversalDebugAvailable = true;
  snapshot.traversalAttempted = true;
  snapshot.traversalAccepted = traversalApplied(result);
  snapshot.traversalMechanic = traversalMechanicName(result.mechanic);
  snapshot.traversalReason = unavailableIfNull(result.reasonCode);
  snapshot.traversalSlotId = noneIfEmpty(result.slotId);
  snapshot.traversalSlotKind = noneIfEmpty(result.slotKind);
  snapshot.traversalSlotHeightBand = noneIfEmpty(result.slotHeightBand);
  snapshot.traversalTargetId = noneIfEmpty(result.targetId);
  snapshot.traversalLandingSurfaceId = noneIfEmpty(result.landingSurfaceId);
  snapshot.traversalSlotLedgeHeightMeters = result.slotLedgeHeightMeters;
  snapshot.traversalSlotUsableWidthMeters = result.slotUsableWidthMeters;
  snapshot.traversalSlotStartRangeMeters = result.slotStartRangeMeters;
  snapshot.traversalSlotFacingDot = result.slotFacingDot;
}

void copyTraversalIntentFacts(const TraversalIntentResult& result,
                              RuntimeDebugSnapshot& snapshot) {
  snapshot.traversalDebugAvailable = result.requested || result.traversalAttempted;
  snapshot.traversalIntentRequested = result.requested;
  snapshot.traversalIntentConsumed = result.consumedInput;
  snapshot.traversalIntentAccepted = result.accepted;
  snapshot.traversalIntentFallbackJumpAllowed = result.fallbackJumpAllowed;
  snapshot.traversalIntentTrigger = traversalIntentTriggerName(result.trigger);
  snapshot.traversalIntentStatus = unavailableIfNull(result.reasonCode);
  snapshot.traversalIntentSelectedMechanic =
      result.traversalAttempted ? traversalMechanicName(result.selectedMechanic) : "none";
  if (result.traversalAttempted) {
    copyTraversalResultFacts(result.traversal, snapshot);
  }
}

void copyTraversalPreviewFacts(const TraversalCandidatePreviewResult& result,
                               RuntimeDebugSnapshot& snapshot) {
  snapshot.traversalPreviewAvailable = true;
  snapshot.traversalPreviewReady = result.ready;
  snapshot.traversalPreviewCandidateAvailable = result.candidateAvailable;
  snapshot.traversalPreviewStatus = unavailableIfNull(result.reasonCode);
  snapshot.traversalPreviewHudCode = unavailableIfNull(result.hudCode);
  snapshot.traversalPreviewMechanic = traversalMechanicName(result.selectedMechanic);
  snapshot.traversalPreviewSlotId = noneIfEmpty(result.slotId);
  snapshot.traversalPreviewSlotKind = noneIfEmpty(result.slotKind);
  snapshot.traversalPreviewSlotHeightBand = noneIfEmpty(result.slotHeightBand);
  snapshot.traversalPreviewTargetId = noneIfEmpty(result.targetId);
  snapshot.traversalPreviewLandingSurfaceId = noneIfEmpty(result.landingSurfaceId);
  snapshot.traversalPreviewSlotLedgeHeightMeters = result.slotLedgeHeightMeters;
  snapshot.traversalPreviewSlotUsableWidthMeters = result.slotUsableWidthMeters;
  snapshot.traversalPreviewSlotStartRangeMeters = result.slotStartRangeMeters;
  snapshot.traversalPreviewSlotFacingDot = result.slotFacingDot;
}

void copyTraversalFacts(const RuntimeDebugSnapshotRequest& request,
                        RuntimeDebugSnapshot& snapshot) {
  if (request.traversalPreviewResult != nullptr) {
    copyTraversalPreviewFacts(*request.traversalPreviewResult, snapshot);
  }
  if (request.traversalIntentResult != nullptr) {
    copyTraversalIntentFacts(*request.traversalIntentResult, snapshot);
    return;
  }
  if (request.traversalResult != nullptr) {
    copyTraversalResultFacts(*request.traversalResult, snapshot);
  }
}

}  // namespace

const char* runtimeDebugSnapshotStatusName(RuntimeDebugSnapshotStatus status) {
  switch (status) {
    case RuntimeDebugSnapshotStatus::Ok:
      return "debug_overlay_ok";
    case RuntimeDebugSnapshotStatus::Disabled:
      return "debug_overlay_disabled";
    case RuntimeDebugSnapshotStatus::MissingSession:
      return "debug_overlay_missing_session";
    case RuntimeDebugSnapshotStatus::InvalidActor:
      return "debug_overlay_invalid_actor";
    case RuntimeDebugSnapshotStatus::ActorInactive:
      return "debug_overlay_actor_inactive";
    case RuntimeDebugSnapshotStatus::InvalidDelta:
      return "debug_overlay_invalid_delta";
  }
  return "debug_overlay_invalid_delta";
}

RuntimeDebugSnapshot buildRuntimeDebugSnapshot(const RuntimeDebugSnapshotRequest& request) {
  RuntimeDebugSnapshot snapshot;
  snapshot.enabled = request.enabled;
  snapshot.reasonCode = runtimeDebugSnapshotStatusName(snapshot.status);
  if (!request.enabled) {
    return snapshot;
  }
  if (request.session == nullptr) {
    snapshot.status = RuntimeDebugSnapshotStatus::MissingSession;
    snapshot.reasonCode = runtimeDebugSnapshotStatusName(snapshot.status);
    return snapshot;
  }
  if (!std::isfinite(request.deltaSeconds) || request.deltaSeconds < 0.0F) {
    snapshot.status = RuntimeDebugSnapshotStatus::InvalidDelta;
    snapshot.reasonCode = runtimeDebugSnapshotStatusName(snapshot.status);
    return snapshot;
  }

  const EntityId actor = actorFromRequest(request);
  if (!isValid(actor)) {
    snapshot.status = RuntimeDebugSnapshotStatus::InvalidActor;
    snapshot.reasonCode = runtimeDebugSnapshotStatusName(snapshot.status);
    return snapshot;
  }
  const EntityState* entity = request.session->world.findById(actor);
  if (entity == nullptr) {
    snapshot.status = RuntimeDebugSnapshotStatus::InvalidActor;
    snapshot.actor = actor;
    snapshot.reasonCode = runtimeDebugSnapshotStatusName(snapshot.status);
    return snapshot;
  }
  if (!entity->active) {
    snapshot.status = RuntimeDebugSnapshotStatus::ActorInactive;
    snapshot.actor = actor;
    snapshot.reasonCode = runtimeDebugSnapshotStatusName(snapshot.status);
    return snapshot;
  }

  snapshot.status = RuntimeDebugSnapshotStatus::Ok;
  snapshot.reasonCode = runtimeDebugSnapshotStatusName(snapshot.status);
  snapshot.actor = actor;
  snapshot.sourceTick = request.session->clock.tickIndex;
  snapshot.playerPositionAvailable = true;
  snapshot.position = entity->transform.position;
  snapshot.previousPosition =
      request.hasPreviousPosition ? request.previousPosition : snapshot.position;
  snapshot.displacementThisFrame = snapshot.position - snapshot.previousPosition;
  snapshot.deltaSeconds = request.deltaSeconds;
  snapshot.movedThisFrameMeters = vectorLength(snapshot.displacementThisFrame);
  snapshot.speedAvailable = request.deltaSeconds > 0.0F;
  if (snapshot.speedAvailable) {
    snapshot.horizontalSpeedMetersPerSecond =
        vectorLength(horizontal(snapshot.displacementThisFrame)) / request.deltaSeconds;
    snapshot.verticalSpeedMetersPerSecond =
        snapshot.displacementThisFrame.y / request.deltaSeconds;
  }
  snapshot.hasSpawnDistance = request.hasSpawnPosition;
  if (snapshot.hasSpawnDistance) {
    snapshot.distanceFromSpawnMeters =
        vectorLength(snapshot.position - request.spawnPosition);
  }
  snapshot.yawRadians = request.yawRadians;
  snapshot.pitchRadians = request.pitchRadians;

  copyMotorFacts(request, snapshot);
  copyMovementFacts(request, snapshot);
  copyTraversalFacts(request, snapshot);
  return snapshot;
}

}  // namespace iggy3d

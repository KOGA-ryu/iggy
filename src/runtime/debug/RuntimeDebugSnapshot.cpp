#include "runtime/debug/RuntimeDebugSnapshot.hpp"

#include <cmath>

#include "runtime/world/EntityState.hpp"

namespace iggy3d {
namespace {

float vectorLength(Vec3 value) {
  return std::sqrt(lengthSquared(value));
}

Vec3 horizontal(Vec3 value) {
  return {value.x, 0.0F, value.z};
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
  snapshot.movementPolicyBand = request.movementResult->movementPolicyBand;
  if (!request.movementResult->hitSurfaceId.empty()) {
    snapshot.hitSurfaceId = request.movementResult->hitSurfaceId;
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
  return snapshot;
}

}  // namespace iggy3d

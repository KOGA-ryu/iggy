#include "projection/debug/DebugProjection.hpp"
#include "runtime/debug/RuntimeDebugSnapshot.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::Transform3 transformAt(float x, float y, float z) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = {x, y, z};
  return transform;
}

iggy3d::SessionState makeSessionAt(iggy3d::Vec3 position) {
  iggy3d::SessionState state;
  state.clock.tickIndex = 42;

  iggy3d::EntityState player;
  player.id = {1};
  player.stableName = "player";
  player.kind = iggy3d::EntityKind::Player;
  player.transform = transformAt(position.x, position.y, position.z);
  player.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  (void)state.world.seedEntity(player);

  iggy3d::PlayerSlot slot;
  slot.id = 0;
  slot.kind = iggy3d::PlayerSlotKind::Local;
  slot.actor = {1};
  slot.stableName = "local";
  (void)state.players.addSlot(slot);
  return state;
}

bool disabledSnapshotIsStable() {
  const iggy3d::RuntimeDebugSnapshot snapshot =
      iggy3d::buildRuntimeDebugSnapshot({});
  return expect(snapshot.status == iggy3d::RuntimeDebugSnapshotStatus::Disabled,
                "disabled status") &&
         expect(!snapshot.enabled, "disabled flag") &&
         expect(std::string_view(snapshot.reasonCode) == "debug_overlay_disabled",
                "disabled reason");
}

bool playerTelemetryComputesMovementFacts() {
  const iggy3d::SessionState state = makeSessionAt({3.0F, 1.0F, 4.0F});
  iggy3d::PlayerMotorState motor;
  motor.actor = {1};
  motor.phase = iggy3d::PlayerMotorPhase::Airborne;
  motor.grounded = false;
  motor.jumpAvailable = false;
  motor.horizontalVelocityMetersPerSecond = {2.0F, 0.0F, 0.0F};
  motor.verticalVelocityMetersPerSecond = 5.0F;
  motor.dashCooldownRemainingSeconds = 0.25F;

  iggy3d::MovementResult movement;
  movement.movementPolicyBand = "flat";
  movement.hitSurfaceId = "floor_a";

  iggy3d::RuntimeDebugSnapshotRequest request;
  request.enabled = true;
  request.session = &state;
  request.hasPreviousPosition = true;
  request.previousPosition = {2.0F, 1.0F, 4.0F};
  request.hasSpawnPosition = true;
  request.spawnPosition = {0.0F, 0.0F, 0.0F};
  request.deltaSeconds = 0.50F;
  request.motorState = &motor;
  request.movementResult = &movement;
  request.yawRadians = 0.25F;
  request.pitchRadians = -0.10F;

  const iggy3d::RuntimeDebugSnapshot snapshot =
      iggy3d::buildRuntimeDebugSnapshot(request);
  return expect(snapshot.status == iggy3d::RuntimeDebugSnapshotStatus::Ok, "snapshot ok") &&
         expect(snapshot.actor == iggy3d::EntityId{1}, "actor fallback") &&
         expect(snapshot.playerPositionAvailable, "position available") &&
         expect(iggy3d::nearlyEqual(snapshot.position, {3.0F, 1.0F, 4.0F}), "position") &&
         expect(snapshot.speedAvailable, "speed available") &&
         expect(snapshot.movedThisFrameMeters > 0.99F &&
                    snapshot.movedThisFrameMeters < 1.01F,
                "moved this frame") &&
         expect(snapshot.horizontalSpeedMetersPerSecond > 1.99F &&
                    snapshot.horizontalSpeedMetersPerSecond < 2.01F,
                "horizontal speed") &&
         expect(snapshot.distanceFromSpawnMeters > 5.09F &&
                    snapshot.distanceFromSpawnMeters < 5.10F,
                "spawn distance") &&
         expect(!snapshot.grounded, "airborne grounded flag") &&
         expect(snapshot.motorPhase == iggy3d::PlayerMotorPhase::Airborne, "airborne phase") &&
         expect(snapshot.dashCooldownRemainingSeconds > 0.24F, "dash cooldown") &&
         expect(snapshot.movementPolicyBand == "flat", "policy band") &&
         expect(snapshot.hitSurfaceId == "floor_a", "surface id");
}

bool invalidInputsReportReasons() {
  iggy3d::RuntimeDebugSnapshotRequest missing;
  missing.enabled = true;
  const iggy3d::RuntimeDebugSnapshot noSession =
      iggy3d::buildRuntimeDebugSnapshot(missing);

  iggy3d::SessionState state;
  iggy3d::RuntimeDebugSnapshotRequest noActor;
  noActor.enabled = true;
  noActor.session = &state;
  const iggy3d::RuntimeDebugSnapshot invalidActor =
      iggy3d::buildRuntimeDebugSnapshot(noActor);

  iggy3d::RuntimeDebugSnapshotRequest badDelta;
  badDelta.enabled = true;
  badDelta.session = &state;
  badDelta.deltaSeconds = -1.0F;
  const iggy3d::RuntimeDebugSnapshot invalidDelta =
      iggy3d::buildRuntimeDebugSnapshot(badDelta);

  return expect(noSession.status == iggy3d::RuntimeDebugSnapshotStatus::MissingSession,
                "missing session") &&
         expect(invalidActor.status == iggy3d::RuntimeDebugSnapshotStatus::InvalidActor,
                "invalid actor") &&
         expect(invalidDelta.status == iggy3d::RuntimeDebugSnapshotStatus::InvalidDelta,
                "invalid delta");
}

bool snapshotAppendsProjectionItem() {
  const iggy3d::SessionState state = makeSessionAt({1.0F, 0.0F, 2.0F});
  iggy3d::RuntimeDebugSnapshotRequest request;
  request.enabled = true;
  request.session = &state;
  request.deltaSeconds = 0.10F;
  const iggy3d::RuntimeDebugSnapshot snapshot =
      iggy3d::buildRuntimeDebugSnapshot(request);
  iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(state);
  const std::size_t before = debug.items.size();
  iggy3d::appendRuntimeDebugSnapshot(debug, snapshot);

  const iggy3d::DebugProjectionItem& item = debug.items.back();
  return expect(debug.items.size() == before + 1U, "debug item appended") &&
         expect(item.kind == iggy3d::DebugProjectionKind::RuntimeTelemetry,
                "runtime telemetry kind") &&
         expect(item.actor == iggy3d::EntityId{1}, "debug actor") &&
         expect(item.hasWorldPoint && iggy3d::nearlyEqual(item.worldPoint, {1.0F, 0.0F, 2.0F}),
                "debug world point") &&
         expect(item.hasScalar, "debug scalar") &&
         expect(item.labelCode == "runtime.debug.overlay", "debug label");
}

}  // namespace

int main() {
  const bool ok = disabledSnapshotIsStable() && playerTelemetryComputesMovementFacts() &&
                  invalidInputsReportReasons() && snapshotAppendsProjectionItem();
  return ok ? 0 : 1;
}

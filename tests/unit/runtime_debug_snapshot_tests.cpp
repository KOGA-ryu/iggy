#include "projection/debug/DebugProjection.hpp"
#include "runtime/debug/RuntimeDebugSnapshot.hpp"
#include "runtime/movement/MovementTraversal.hpp"

#include <iostream>
#include <string>
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

iggy3d::TraversalIntentResult makeClamberIntentResult() {
  iggy3d::TraversalIntentResult intent;
  intent.status = iggy3d::TraversalIntentStatus::Applied;
  intent.trigger = iggy3d::TraversalIntentTrigger::Jump;
  intent.selectedMechanic = iggy3d::TraversalMechanic::Clamber;
  intent.requested = true;
  intent.traversalAttempted = true;
  intent.consumedInput = true;
  intent.accepted = true;
  intent.fallbackJumpAllowed = false;
  intent.reasonCode = "traversal_intent_applied";

  intent.traversal.status = iggy3d::TraversalStatus::Applied;
  intent.traversal.mechanic = iggy3d::TraversalMechanic::Clamber;
  intent.traversal.actor = {1};
  intent.traversal.slotId = "clamber_block:clamber_top_walkable";
  intent.traversal.slotKind = "clamber";
  intent.traversal.slotHeightBand = "clamber_low";
  intent.traversal.targetId = "clamber_block";
  intent.traversal.landingSurfaceId = "clamber_top_walkable";
  intent.traversal.slotLedgeHeightMeters = 0.61F;
  intent.traversal.slotUsableWidthMeters = 1.829F;
  intent.traversal.slotStartRangeMeters = 0.152F;
  intent.traversal.slotFacingDot = 1.0F;
  intent.traversal.reasonCode = "traversal_applied";
  return intent;
}

iggy3d::TraversalCandidatePreviewResult makeClamberPreviewResult() {
  iggy3d::TraversalCandidatePreviewResult preview;
  preview.status = iggy3d::TraversalCandidatePreviewStatus::Ready;
  preview.selectedMechanic = iggy3d::TraversalMechanic::Clamber;
  preview.actor = {1};
  preview.candidateAvailable = true;
  preview.ready = true;
  preview.slotId = "clamber_block:clamber_top_walkable";
  preview.slotKind = "clamber";
  preview.slotHeightBand = "clamber_low";
  preview.targetId = "clamber_block";
  preview.landingSurfaceId = "clamber_top_walkable";
  preview.slotLedgeHeightMeters = 0.61F;
  preview.slotUsableWidthMeters = 1.829F;
  preview.slotStartRangeMeters = 0.152F;
  preview.slotFacingDot = 1.0F;
  preview.reasonCode = "traversal_preview_ready";
  preview.hudCode = "READY";
  return preview;
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

  iggy3d::PlayerMotorResult motorResult;
  motorResult.status = iggy3d::PlayerMotorStatus::Ok;
  motorResult.actor = {1};
  motorResult.phase = iggy3d::PlayerMotorPhase::Airborne;
  motorResult.grounded = false;
  motorResult.horizontalSpeedMetersPerSecond = 2.0F;
  motorResult.verticalVelocityMetersPerSecond = 5.0F;
  motorResult.dashCooldownRemainingSeconds = 0.25F;
  motorResult.groundSampleValid = true;
  motorResult.groundContact = false;
  motorResult.groundWalkable = true;
  motorResult.groundNormal = {0.0F, 1.0F, 0.0F};
  motorResult.groundDistanceMeters = 0.50F;
  motorResult.slopeAngleDegrees = 0.0F;
  motorResult.slopeUpDot = 1.0F;
  motorResult.speedMultiplier = 1.0F;
  motorResult.staminaCostMultiplier = 1.0F;
  motorResult.stepPenaltyMultiplier = 1.0F;
  motorResult.movementPolicyBand = "flat";
  motorResult.groundSurfaceId = "floor_a";

  iggy3d::MovementResult movement;
  movement.movementPolicyBand = "flat";
  movement.slopeUpDot = 1.0F;
  movement.speedMultiplier = 1.0F;
  movement.staminaCostMultiplier = 1.0F;
  movement.stepPenaltyMultiplier = 1.0F;
  movement.horizontalDistanceMeters = 0.75F;
  movement.verticalDeltaMeters = 0.25F;
  movement.gradePercent = 33.33F;
  movement.slopeTravelDirection = "uphill";
  movement.hitSurfaceId = "floor_a";

  const iggy3d::TraversalIntentResult traversalIntent = makeClamberIntentResult();
  const iggy3d::TraversalCandidatePreviewResult traversalPreview =
      makeClamberPreviewResult();

  iggy3d::RuntimeDebugSnapshotRequest request;
  request.enabled = true;
  request.session = &state;
  request.hasPreviousPosition = true;
  request.previousPosition = {2.0F, 1.0F, 4.0F};
  request.hasSpawnPosition = true;
  request.spawnPosition = {0.0F, 0.0F, 0.0F};
  request.deltaSeconds = 0.50F;
  request.motorState = &motor;
  request.motorResult = &motorResult;
  request.movementResult = &movement;
  request.traversalIntentResult = &traversalIntent;
  request.traversalPreviewResult = &traversalPreview;
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
         expect(snapshot.groundSampleValid, "ground sample valid") &&
         expect(!snapshot.groundContact, "ground contact false") &&
         expect(snapshot.groundWalkable, "ground walkable") &&
         expect(iggy3d::nearlyEqual(snapshot.groundNormal, {0.0F, 1.0F, 0.0F}),
                "ground normal") &&
         expect(snapshot.groundDistanceMeters > 0.49F &&
                    snapshot.groundDistanceMeters < 0.51F,
                "ground distance") &&
         expect(snapshot.slopeAngleDegrees > -0.01F &&
                    snapshot.slopeAngleDegrees < 0.01F,
                "slope angle") &&
         expect(snapshot.slopeUpDot > 0.99F, "slope up dot") &&
         expect(snapshot.speedMultiplier > 0.99F, "speed multiplier") &&
         expect(snapshot.movementHorizontalDistanceMeters > 0.74F &&
                    snapshot.movementHorizontalDistanceMeters < 0.76F,
                "movement horizontal distance") &&
         expect(snapshot.movementVerticalDeltaMeters > 0.24F &&
                    snapshot.movementVerticalDeltaMeters < 0.26F,
                "movement vertical delta") &&
         expect(snapshot.movementGradePercent > 33.0F &&
                    snapshot.movementGradePercent < 33.4F,
                "movement grade percent") &&
         expect(snapshot.slopeTravelDirection == "uphill", "slope travel direction") &&
         expect(snapshot.movementPolicyBand == "flat", "policy band") &&
         expect(snapshot.groundSurfaceId == "floor_a", "ground surface id") &&
         expect(snapshot.hitSurfaceId == "floor_a", "surface id") &&
         expect(snapshot.traversalPreviewAvailable, "preview available") &&
         expect(snapshot.traversalPreviewReady, "preview ready") &&
         expect(snapshot.traversalPreviewCandidateAvailable, "preview candidate") &&
         expect(snapshot.traversalPreviewStatus == "traversal_preview_ready",
                "preview status") &&
         expect(snapshot.traversalPreviewHudCode == "READY", "preview hud code") &&
         expect(snapshot.traversalPreviewMechanic == "clamber", "preview mechanic") &&
         expect(snapshot.traversalPreviewSlotId == "clamber_block:clamber_top_walkable",
                "preview slot") &&
         expect(snapshot.traversalPreviewSlotHeightBand == "clamber_low",
                "preview band") &&
         expect(snapshot.traversalDebugAvailable, "traversal debug available") &&
         expect(snapshot.traversalIntentRequested, "traversal intent requested") &&
         expect(snapshot.traversalIntentConsumed, "traversal intent consumed") &&
         expect(snapshot.traversalIntentAccepted, "traversal intent accepted") &&
         expect(snapshot.traversalAttempted, "traversal attempted") &&
         expect(snapshot.traversalAccepted, "traversal accepted") &&
         expect(snapshot.traversalIntentTrigger == "jump", "traversal intent trigger") &&
         expect(snapshot.traversalIntentStatus == "traversal_intent_applied",
                "traversal intent status") &&
         expect(snapshot.traversalIntentSelectedMechanic == "clamber",
                "traversal intent selected mechanic") &&
         expect(snapshot.traversalMechanic == "clamber", "traversal mechanic") &&
         expect(snapshot.traversalReason == "traversal_applied", "traversal reason") &&
         expect(snapshot.traversalSlotId == "clamber_block:clamber_top_walkable",
                "traversal slot id") &&
         expect(snapshot.traversalSlotHeightBand == "clamber_low",
                "traversal slot height band") &&
         expect(snapshot.traversalTargetId == "clamber_block", "traversal target id") &&
         expect(snapshot.traversalLandingSurfaceId == "clamber_top_walkable",
                "traversal landing surface") &&
         expect(snapshot.traversalSlotLedgeHeightMeters > 0.60F &&
                    snapshot.traversalSlotLedgeHeightMeters < 0.62F,
                "traversal ledge height") &&
         expect(snapshot.traversalSlotFacingDot > 0.99F, "traversal facing dot");
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
         expect(item.labelCode == "runtime.debug.overlay", "debug label") &&
         expect(debug.runtimeDebugHudLines.size() >= 8U, "hud lines projected") &&
         expect(debug.runtimeDebugHudLines[0].starts_with("POS "), "hud pos line") &&
         expect(debug.runtimeDebugHudLines[1].starts_with("SPD "), "hud speed line") &&
         expect(debug.runtimeDebugHudLines[2].starts_with("UP "), "hud up line") &&
         expect(debug.runtimeDebugHudLines[3].starts_with("MOVE "), "hud move line") &&
         expect(debug.runtimeDebugHudLines[4].starts_with("DIST "), "hud dist line") &&
         expect(debug.runtimeDebugHudLines[5].starts_with("PHASE "), "hud phase line") &&
         expect(debug.runtimeDebugHudLines[7].starts_with("GRADE "), "hud grade line");
}

bool traversalPreviewProjectsHudLines() {
  const iggy3d::SessionState state = makeSessionAt({1.0F, 0.0F, 2.0F});
  const iggy3d::TraversalCandidatePreviewResult traversalPreview =
      makeClamberPreviewResult();
  iggy3d::RuntimeDebugSnapshotRequest request;
  request.enabled = true;
  request.session = &state;
  request.deltaSeconds = 0.10F;
  request.traversalPreviewResult = &traversalPreview;
  const iggy3d::RuntimeDebugSnapshot snapshot =
      iggy3d::buildRuntimeDebugSnapshot(request);

  iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(state);
  iggy3d::appendRuntimeDebugSnapshot(debug, snapshot);

  bool hasNextLine = false;
  bool hasGateLine = false;
  for (const std::string& line : debug.runtimeDebugHudLines) {
    hasNextLine =
        hasNextLine ||
        (line.starts_with("NEXT READY clamber ") &&
         line.find("clamber_block:clamber_top_walkable") != std::string::npos);
    hasGateLine =
        hasGateLine ||
        (line.starts_with("GATE traversal_preview_ready ") &&
         line.find("clamber_low") != std::string::npos &&
         line.find("dot 1.000") != std::string::npos);
  }

  return expect(debug.runtimeDebugHudLines.size() >= 11U, "preview hud lines projected") &&
         expect(hasNextLine, "preview next line") &&
         expect(hasGateLine, "preview gate line");
}

bool traversalDebugProjectsHudLines() {
  const iggy3d::SessionState state = makeSessionAt({1.0F, 0.0F, 2.0F});
  const iggy3d::TraversalIntentResult traversalIntent = makeClamberIntentResult();
  iggy3d::RuntimeDebugSnapshotRequest request;
  request.enabled = true;
  request.session = &state;
  request.deltaSeconds = 0.10F;
  request.traversalIntentResult = &traversalIntent;
  const iggy3d::RuntimeDebugSnapshot snapshot =
      iggy3d::buildRuntimeDebugSnapshot(request);

  iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(state);
  iggy3d::appendRuntimeDebugSnapshot(debug, snapshot);

  bool hasTraversalLine = false;
  bool hasSlotLine = false;
  for (const std::string& line : debug.runtimeDebugHudLines) {
    hasTraversalLine =
        hasTraversalLine ||
        (line.starts_with("TRAV ") &&
         line.find("traversal_intent_applied") != std::string::npos &&
         line.find("clamber") != std::string::npos);
    hasSlotLine =
        hasSlotLine ||
        (line.starts_with("SLOT ") &&
         line.find("clamber_block:clamber_top_walkable") != std::string::npos &&
         line.find("clamber_low") != std::string::npos &&
         line.find("dot 1.000") != std::string::npos);
  }

  return expect(debug.runtimeDebugHudLines.size() >= 11U, "traversal hud lines projected") &&
         expect(hasTraversalLine, "traversal hud line") &&
         expect(hasSlotLine, "slot hud line");
}

}  // namespace

int main() {
  const bool ok = disabledSnapshotIsStable() && playerTelemetryComputesMovementFacts() &&
                  invalidInputsReportReasons() && snapshotAppendsProjectionItem() &&
                  traversalPreviewProjectsHudLines() &&
                  traversalDebugProjectsHudLines();
  return ok ? 0 : 1;
}

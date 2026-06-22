#include "projection/debug/DebugProjection.hpp"

#include <iomanip>
#include <sstream>

#include "runtime/world/EntityState.hpp"

namespace iggy3d {
namespace {

Aabb3 worldBoundsFor(const EntityState& entity) {
  return makeAabb3(entity.localBounds.min + entity.transform.position,
                   entity.localBounds.max + entity.transform.position);
}

void appendTargetCandidates(const SessionState& state, DebugProjectionResult& result) {
  for (const EntityState& entity : state.world.entities()) {
    if (!entity.active || !entity.targeting.targetable) {
      continue;
    }

    DebugProjectionItem item;
    item.kind = DebugProjectionKind::TargetCandidate;
    item.sourceTick = state.clock.tickIndex;
    item.target = entity.id;
    item.hasWorldPoint = true;
    item.worldPoint = entity.transform.position;
    item.hasBounds = true;
    item.worldBounds = worldBoundsFor(entity);
    item.labelCode = "target.candidate";
    result.items.push_back(std::move(item));
  }
}

void appendReachRadii(const SessionState& state, DebugProjectionResult& result) {
  for (const PlayerSlot& slot : state.players.slots()) {
    DebugProjectionItem item;
    item.kind = DebugProjectionKind::ReachRadius;
    item.sourceTick = state.clock.tickIndex;
    item.playerSlot = slot.id;
    item.actor = slot.actor;
    item.radiusMeters = state.config.interactionRangeMeters;
    item.labelCode = "reach.interaction";
    result.items.push_back(std::move(item));
  }
}

void appendCommandRejections(const SessionState& state, DebugProjectionResult& result) {
  for (const CommandRecord& command : state.commandLog.records()) {
    if (command.admission != CommandAdmissionStatus::Rejected) {
      continue;
    }

    DebugProjectionItem item;
    item.kind = DebugProjectionKind::CommandRejected;
    item.sourceTick = state.clock.tickIndex;
    item.commandId = command.commandId;
    item.sequence = command.sequence;
    item.playerSlot = command.playerSlot;
    item.actor = command.actor;
    item.target = command.payload.target.entity;
    item.rejection = command.rejection;
    item.hasWorldPoint = command.payload.target.hasPoint;
    item.worldPoint = command.payload.target.point;
    item.labelCode = "command.rejected";
    result.items.push_back(std::move(item));
  }
}

void appendSessionFacts(const SessionState& state, DebugProjectionResult& result) {
  DebugProjectionItem clock;
  clock.kind = DebugProjectionKind::ClockMode;
  clock.sourceTick = state.clock.tickIndex;
  clock.labelCode = "clock.mode";
  result.items.push_back(std::move(clock));

  DebugProjectionItem camera;
  camera.kind = DebugProjectionKind::CameraMode;
  camera.sourceTick = state.clock.tickIndex;
  camera.target = state.camera.target.entity;
  camera.hasWorldPoint = state.camera.target.hasPoint;
  camera.worldPoint = state.camera.target.point;
  camera.labelCode = "camera.mode";
  result.items.push_back(std::move(camera));

  for (const ObjectiveRecord& objective : state.objectives.objectives) {
    DebugProjectionItem item;
    item.kind = DebugProjectionKind::ObjectiveState;
    item.sourceTick = state.clock.tickIndex;
    item.playerSlot = objective.condition.playerSlot;
    item.objectiveId = objective.objectiveId;
    item.labelCode = "objective.state";
    result.items.push_back(std::move(item));
  }

  DebugProjectionItem hash;
  hash.kind = DebugProjectionKind::StateHash;
  hash.sourceTick = state.clock.tickIndex;
  hash.labelCode = "state.hash";
  result.items.push_back(std::move(hash));
}

std::string fixed3(float value) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(3) << value;
  return out.str();
}

std::string phaseName(const RuntimeDebugSnapshot& snapshot) {
  switch (snapshot.motorPhase) {
    case PlayerMotorPhase::Grounded:
      return "grounded";
    case PlayerMotorPhase::Airborne:
      return "airborne";
  }
  return snapshot.grounded ? "grounded" : "airborne";
}

void appendRuntimeDebugHudLines(DebugProjectionResult& result,
                                const RuntimeDebugSnapshot& snapshot) {
  result.runtimeDebugHudLines.clear();
  if (snapshot.status != RuntimeDebugSnapshotStatus::Ok ||
      !snapshot.playerPositionAvailable) {
    return;
  }

  result.runtimeDebugHudLines.push_back("POS " + fixed3(snapshot.position.x) + " " +
                                        fixed3(snapshot.position.y) + " " +
                                        fixed3(snapshot.position.z));
  result.runtimeDebugHudLines.push_back("SPD " +
                                        fixed3(snapshot.horizontalSpeedMetersPerSecond));
  result.runtimeDebugHudLines.push_back("UP " +
                                        fixed3(snapshot.verticalSpeedMetersPerSecond));
  result.runtimeDebugHudLines.push_back("MOVE " +
                                        fixed3(snapshot.movedThisFrameMeters));
  result.runtimeDebugHudLines.push_back("DIST " +
                                        fixed3(snapshot.distanceFromSpawnMeters));
  result.runtimeDebugHudLines.push_back("PHASE " + phaseName(snapshot));
  result.runtimeDebugHudLines.push_back("SLOPE " + snapshot.movementPolicyBand + " " +
                                        fixed3(snapshot.slopeAngleDegrees) + " " +
                                        fixed3(snapshot.slopeUpDot));
  result.runtimeDebugHudLines.push_back("GRADE " + snapshot.slopeTravelDirection + " " +
                                        fixed3(snapshot.movementGradePercent) + "% dy " +
                                        fixed3(snapshot.movementVerticalDeltaMeters));
  result.runtimeDebugHudLines.push_back("GROUND " +
                                        std::string(snapshot.groundContact ? "contact" : "air") +
                                        " " +
                                        (snapshot.groundWalkable ? "walkable" : "blocked") +
                                        " ny " + fixed3(snapshot.groundNormal.y));
  if (snapshot.traversalPreviewAvailable) {
    result.runtimeDebugHudLines.push_back("NEXT " + snapshot.traversalPreviewHudCode +
                                          " " + snapshot.traversalPreviewMechanic +
                                          " " + snapshot.traversalPreviewSlotId);
    result.runtimeDebugHudLines.push_back(
        "GATE " + snapshot.traversalPreviewStatus + " " +
        snapshot.traversalPreviewSlotHeightBand + " h " +
        fixed3(snapshot.traversalPreviewSlotLedgeHeightMeters) + " w " +
        fixed3(snapshot.traversalPreviewSlotUsableWidthMeters) + " r " +
        fixed3(snapshot.traversalPreviewSlotStartRangeMeters) + " dot " +
        fixed3(snapshot.traversalPreviewSlotFacingDot));
  }
  if (snapshot.traversalDebugAvailable) {
    result.runtimeDebugHudLines.push_back(
        "TRAV " + snapshot.traversalIntentTrigger + " " +
        snapshot.traversalIntentStatus + " " +
        snapshot.traversalIntentSelectedMechanic + " " + snapshot.traversalMechanic +
        " " + snapshot.traversalReason);
    result.runtimeDebugHudLines.push_back(
        "SLOT " + snapshot.traversalSlotId + " " + snapshot.traversalSlotHeightBand +
        " h " + fixed3(snapshot.traversalSlotLedgeHeightMeters) + " w " +
        fixed3(snapshot.traversalSlotUsableWidthMeters) + " r " +
        fixed3(snapshot.traversalSlotStartRangeMeters) + " dot " +
        fixed3(snapshot.traversalSlotFacingDot));
  }
}

}  // namespace

DebugProjectionResult buildDebugProjection(const SessionState& state,
                                           const DebugProjectionConfig& config) {
  DebugProjectionResult result;
  result.sourceStateHash = state.currentStateHash;
  result.sourceTick = state.clock.tickIndex;

  if (config.includeTargetCandidates) {
    appendTargetCandidates(state, result);
  }
  if (config.includeReach) {
    appendReachRadii(state, result);
  }
  if (config.includeCommandRejections) {
    appendCommandRejections(state, result);
  }
  if (config.includeSessionFacts) {
    appendSessionFacts(state, result);
  }

  return result;
}

void appendRuntimeDebugSnapshot(DebugProjectionResult& result,
                                const RuntimeDebugSnapshot& snapshot) {
  if (snapshot.status != RuntimeDebugSnapshotStatus::Ok ||
      !snapshot.playerPositionAvailable) {
    return;
  }

  DebugProjectionItem item;
  item.kind = DebugProjectionKind::RuntimeTelemetry;
  item.sourceTick = snapshot.sourceTick;
  item.actor = snapshot.actor;
  item.hasWorldPoint = true;
  item.worldPoint = snapshot.position;
  item.hasScalar = true;
  item.scalarValue = snapshot.horizontalSpeedMetersPerSecond;
  item.labelCode = "runtime.debug.overlay";
  item.valueCode = snapshot.grounded ? "phase.grounded" : "phase.airborne";
  result.items.push_back(std::move(item));
  appendRuntimeDebugHudLines(result, snapshot);
}

}  // namespace iggy3d

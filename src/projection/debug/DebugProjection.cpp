#include "projection/debug/DebugProjection.hpp"

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

}  // namespace iggy3d

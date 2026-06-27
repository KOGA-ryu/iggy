#include "projection/debug/DebugProjection.hpp"

#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

#include "runtime/ai/NpcBehaviorDebugSnapshot.hpp"
#include "runtime/physics/PhysicsAabbCollisionBatch.hpp"
#include "runtime/physics/PhysicsDebugSnapshot.hpp"
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

std::string bit(bool value) {
  static constexpr std::array<std::string_view, 2> kValues{"0", "1"};
  return std::string(kValues[static_cast<std::size_t>(value)]);
}

std::string upstreamCode(std::string_view value) {
  // branch-gate: BG-1107
  if (value.empty()) {
    return "none";
  }
  return std::string(value);
}

std::string sensorValueCode(bool sensor) {
  static constexpr std::array<std::string_view, 2> kValues{"solid", "sensor"};
  return std::string(kValues[static_cast<std::size_t>(sensor)]);
}

float sensorScalar(bool sensor) {
  static constexpr std::array<float, 2> kValues{0.0F, 1.0F};
  return kValues[static_cast<std::size_t>(sensor)];
}

Vec3 physicsAabbPairMidpoint(const PhysicsAabbCollider& first,
                             const PhysicsAabbCollider& second) {
  return (center(first.bounds) + center(second.bounds)) * 0.5F;
}

EntityId physicsBodyDebugEntityId(PhysicsBodyId id) {
  return EntityId{static_cast<std::uint64_t>(id.value)};
}

std::string phaseName(const RuntimeDebugSnapshot& snapshot) {
  switch (snapshot.motorPhase) {
    case PlayerMotorPhase::Grounded:
      return "grounded";
    case PlayerMotorPhase::Airborne:
      return "airborne";
    case PlayerMotorPhase::WireWalk:
      return "wire_walk";
  }
  return snapshot.grounded ? "grounded" : "airborne";
}

std::string entityIdCode(EntityId id) {
  if (!isValid(id)) {
    return "none";
  }
  return std::to_string(toUint64(id));
}

std::string npcBehaviorValueCode(const NpcBehaviorDebugActorRow& row) {
  return row.behaviorProfileId + ":" + std::string(aiBehaviorKindName(row.behavior)) +
         ":" + std::string(aiIntentKindName(row.lastIntent)) + ":" +
         row.profileStatus;
}

void appendNpcBehaviorHudLines(DebugProjectionResult& result,
                               const NpcBehaviorDebugSnapshot& snapshot) {
  result.npcBehaviorDebugHudLines.push_back(
      "NPCS world=" + std::to_string(snapshot.npcWorldCount) + " ai=" +
      std::to_string(snapshot.aiActorCount) + " resolved=" +
      std::to_string(snapshot.resolvedProfileCount) + " failed=" +
      std::to_string(snapshot.failedProfileCount) + " hostile=" +
      std::to_string(snapshot.hostileCount) + " passive=" +
      std::to_string(snapshot.passiveCount));

  constexpr std::size_t kMaxNpcHudActorRows = 8;
  std::size_t emittedRows = 0;
  for (const NpcBehaviorDebugActorRow& row : snapshot.actors) {
    if (emittedRows >= kMaxNpcHudActorRows) {
      break;
    }

    std::string line = "NPC " + entityIdCode(row.actor) + " " + row.stableName +
                       " " + row.behaviorProfileId + " " +
                       std::string(aiBehaviorKindName(row.behavior)) + "/" +
                       std::string(aiIntentKindName(row.lastIntent)) + " tgt=" +
                       row.targetStableName + " cd=" +
                       std::to_string(row.cooldownTicksRemaining);
    if (!row.profileResolved) {
      line += " unresolved=" + row.profileStatus;
    }
    result.npcBehaviorDebugHudLines.push_back(std::move(line));
    ++emittedRows;
  }
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

void appendPhysicsAabbDebugItems(DebugProjectionResult& result,
                                 const PhysicsAabbCollisionBatchResult& batch,
                                 std::size_t maxAabbs) {
  std::size_t emitted = 0U;
  for (const PhysicsAabbCollider& collider : batch.colliders) {
    // branch-gate: BG-1111
    if (emitted >= maxAabbs) {
      break;
    }

    DebugProjectionItem item;
    item.kind = DebugProjectionKind::PhysicsAabb;
    item.actor = physicsBodyDebugEntityId(collider.bodyId);
    item.hasBounds = true;
    item.worldBounds = collider.bounds;
    item.hasScalar = true;
    item.scalarValue = sensorScalar(collider.sensor);
    item.labelCode = "physics.aabb";
    item.valueCode = sensorValueCode(collider.sensor);
    result.items.push_back(std::move(item));
    ++emitted;
  }
}

void appendPhysicsContactNormalDebugItems(
    DebugProjectionResult& result,
    const PhysicsAabbCollisionBatchResult& batch,
    std::size_t maxContacts) {
  std::size_t emitted = 0U;
  for (const PhysicsAabbContact& contact : batch.contacts) {
    // branch-gate: BG-1111
    if (emitted >= maxContacts) {
      break;
    }

    DebugProjectionItem item;
    item.kind = DebugProjectionKind::PhysicsContactNormal;
    item.actor = physicsBodyDebugEntityId(contact.firstBodyId);
    item.target = physicsBodyDebugEntityId(contact.secondBodyId);
    item.hasWorldPoint = true;
    item.worldPoint = contact.pointMeters;
    item.hasScalar = true;
    item.scalarValue = contact.penetrationMeters;
    item.labelCode = "physics.contact_normal";
    item.valueCode = sensorValueCode(contact.includesSensor);
    result.items.push_back(std::move(item));
    ++emitted;
  }
}

void appendPhysicsBroadphasePairDebugItems(
    DebugProjectionResult& result,
    const PhysicsAabbCollisionBatchResult& batch,
    std::size_t maxPairs) {
  std::size_t emitted = 0U;
  for (const PhysicsBroadphasePair& pair : batch.broadphasePairs) {
    // branch-gate: BG-1111
    if (emitted >= maxPairs) {
      break;
    }

    DebugProjectionItem item;
    item.kind = DebugProjectionKind::PhysicsBroadphasePair;
    item.actor = physicsBodyDebugEntityId(pair.firstBodyId);
    item.target = physicsBodyDebugEntityId(pair.secondBodyId);
    // branch-gate: BG-1111
    if (pair.firstColliderIndex < batch.colliders.size() &&
        pair.secondColliderIndex < batch.colliders.size()) {
      item.hasWorldPoint = true;
      item.worldPoint = physicsAabbPairMidpoint(
          batch.colliders[pair.firstColliderIndex],
          batch.colliders[pair.secondColliderIndex]);
    }
    item.hasScalar = true;
    item.scalarValue = sensorScalar(pair.includesSensor);
    item.labelCode = "physics.broadphase_pair";
    item.valueCode = sensorValueCode(pair.includesSensor);
    result.items.push_back(std::move(item));
    ++emitted;
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

void appendNpcBehaviorDebugSnapshot(DebugProjectionResult& result,
                                    const NpcBehaviorDebugSnapshot& snapshot) {
  if (snapshot.status != NpcBehaviorDebugSnapshotStatus::Ok) {
    return;
  }

  for (const NpcBehaviorDebugActorRow& row : snapshot.actors) {
    DebugProjectionItem item;
    item.kind = DebugProjectionKind::NpcBehavior;
    item.sourceTick = snapshot.sourceTick;
    item.actor = row.actor;
    item.target = row.target;
    item.hasScalar = row.targetResolved;
    if (row.targetResolved) {
      item.scalarValue = row.targetDistanceMeters;
    }
    item.labelCode = "npc.behavior";
    item.valueCode = npcBehaviorValueCode(row);
    result.items.push_back(std::move(item));
  }

  appendNpcBehaviorHudLines(result, snapshot);
}

void appendPhysicsDebugSnapshot(DebugProjectionResult& result,
                                const PhysicsDebugSnapshot& snapshot) {
  // branch-gate: BG-1107
  if (snapshot.status != PhysicsDebugSnapshotStatus::Ready &&
      snapshot.status != PhysicsDebugSnapshotStatus::StatsFailed) {
    return;
  }

  result.physicsDebugHudLines.push_back(
      "PHYS packets=" + std::to_string(snapshot.sourcePacketCount) +
      " failed=" + std::to_string(snapshot.failedPacketCount) +
      " bodies=" + std::to_string(snapshot.bodyCount) +
      " colliders=" + std::to_string(snapshot.colliderCount) +
      " contacts=" + std::to_string(snapshot.contactCount) +
      " sensors=" + std::to_string(snapshot.sensorContactCount));

  result.physicsDebugHudLines.push_back(
      "PHYS BP cells=" +
      std::to_string(snapshot.broadphaseOccupiedCellCount) +
      " entries=" + std::to_string(snapshot.broadphaseCellEntryCount) +
      " bucket=" + std::to_string(snapshot.broadphaseMaxBucketSize) +
      " candidates=" +
      std::to_string(snapshot.broadphaseCandidatePairCount) +
      " tested=" + std::to_string(snapshot.broadphaseTestedPairCount) +
      " dup=" +
      std::to_string(snapshot.broadphaseDuplicatePairRejectedCount) +
      " overlaps=" +
      std::to_string(snapshot.broadphaseOverlappingPairCount));

  result.physicsDebugHudLines.push_back(
      "PHYS SOLVE plans=" + std::to_string(snapshot.solvePlanCount) +
      " pos=" + std::to_string(snapshot.positionCorrectionAppliedCount) +
      " vel=" + std::to_string(snapshot.velocityImpulseAppliedCount) +
      " fric=" + std::to_string(snapshot.frictionImpulseAppliedCount) +
      " pen=" + fixed3(snapshot.maxPenetrationMeters) +
      " ni=" + fixed3(snapshot.totalNormalImpulseMagnitude) + "/" +
      fixed3(snapshot.maxNormalImpulseMagnitude) + " fi=" +
      fixed3(snapshot.totalFrictionImpulseMagnitude) + "/" +
      fixed3(snapshot.maxFrictionImpulseMagnitude));

  result.physicsDebugHudLines.push_back(
      "PHYS MOVE kin_iter=" +
      std::to_string(snapshot.kinematicIterationCount) +
      " kin_hits=" + std::to_string(snapshot.kinematicHitCount) +
      " player_iter=" + std::to_string(snapshot.playerIterationCount) +
      " player_hits=" + std::to_string(snapshot.playerHitCount) +
      " baked=" + std::to_string(snapshot.playerBakedColliderCount) +
      " skipped=" + std::to_string(snapshot.playerSkippedSurfaceCount));

  // branch-gate: BG-1107
  if (snapshot.hasWarnings) {
    result.physicsDebugHudLines.push_back(
        "PHYS WARN status=" + std::string(snapshot.reasonCode) +
        " upstream=" + upstreamCode(snapshot.upstreamReasonCode) +
        " bp=" + bit(snapshot.hasBroadphasePressure) +
        " pen=" + bit(snapshot.hasPenetrationWarning) +
        " impulse=" + bit(snapshot.hasImpulseWarning));
  }
}

void appendPhysicsCollisionBatchDebugProjection(
    DebugProjectionResult& result,
    const PhysicsAabbCollisionBatchResult& batch,
    const PhysicsDebugGeometryProjectionConfig& config) {
  // branch-gate: BG-1111
  if (!batch.ok) {
    return;
  }
  // branch-gate: BG-1111
  if (config.includeAabbs) {
    appendPhysicsAabbDebugItems(result, batch, config.maxAabbs);
  }
  // branch-gate: BG-1111
  if (config.includeContacts) {
    appendPhysicsContactNormalDebugItems(result, batch, config.maxContacts);
  }
  // branch-gate: BG-1111
  if (config.includeBroadphasePairs) {
    appendPhysicsBroadphasePairDebugItems(result, batch, config.maxPairs);
  }
}

}  // namespace iggy3d

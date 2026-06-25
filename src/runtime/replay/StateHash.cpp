#include "runtime/replay/StateHash.hpp"

#include <iomanip>
#include <sstream>
#include <string_view>

#include "core/hash/StableHash.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d {

namespace {

template <typename Enum>
void addEnum(StableHasher& hasher, std::string_view tag, Enum value) {
  hasher.addString(tag);
  hasher.addByte(static_cast<std::uint8_t>(value));
}

void addU64(StableHasher& hasher, std::string_view tag, std::uint64_t value) {
  hasher.addString(tag);
  hasher.addU64(value);
}

void addStringField(StableHasher& hasher, std::string_view tag, std::string_view value) {
  hasher.addString(tag);
  hasher.addString(value);
}

void addBoolField(StableHasher& hasher, std::string_view tag, bool value) {
  hasher.addString(tag);
  hasher.addBool(value);
}

void addVec3Field(StableHasher& hasher, std::string_view tag, Vec3 value) {
  hasher.addString(tag);
  addVec3Quantized(hasher, value);
}

void addFloatField(StableHasher& hasher, std::string_view tag, float value) {
  hasher.addString(tag);
  hasher.addFloatQuantized(value);
}

void addTransform(StableHasher& hasher, const Transform3& transform) {
  addVec3Field(hasher, "position", transform.position);
  addVec3Field(hasher, "rotationEulerRadians", transform.rotationEulerRadians);
  addVec3Field(hasher, "scale", transform.scale);
}

void addBounds(StableHasher& hasher, const Aabb3& bounds) {
  addVec3Field(hasher, "boundsMin", bounds.min);
  addVec3Field(hasher, "boundsMax", bounds.max);
}

void addInteraction(StableHasher& hasher, const InteractionDefinition& interaction) {
  addEnum(hasher, "interactionKind", interaction.kind);
  addEnum(hasher, "interactionPrimaryEffect", interaction.primaryEffect);
  addStringField(hasher, "interactionItemId", interaction.itemId);
  addU64(hasher, "interactionItemCount", interaction.itemCount);
  addStringField(hasher, "interactionObjectiveId", interaction.objectiveId);
  addStringField(hasher, "interactionRequiredItemId", interaction.requiredItemId);
  addU64(hasher, "interactionRequiredItemCount", interaction.requiredItemCount);
  addBoolField(hasher, "interactionRepeatable", interaction.repeatable);
  addBoolField(hasher, "interactionDeactivateTargetOnSuccess",
               interaction.deactivateTargetOnSuccess);
}

void addCommandTarget(StableHasher& hasher, const CommandTarget& target) {
  addBoolField(hasher, "target.hasEntity", target.hasEntity);
  addU64(hasher, "target.entity", toUint64(target.entity));
  addBoolField(hasher, "target.hasPoint", target.hasPoint);
  addVec3Field(hasher, "target.point", target.point);
}

void addCommandRecord(StableHasher& hasher, const CommandRecord& record) {
  addU64(hasher, "commandId", record.commandId);
  addU64(hasher, "sequence", record.sequence);
  addU64(hasher, "playerSlot", record.playerSlot);
  addU64(hasher, "actor", toUint64(record.actor));
  addEnum(hasher, "commandKind", record.kind);
  addEnum(hasher, "commandSource", record.source);
  addCommandTarget(hasher, record.payload.target);
  addU64(hasher, "retrySourceCommandId", record.payload.retrySourceCommandId);
  addU64(hasher, "attackDamage", static_cast<std::uint64_t>(
                                     static_cast<std::int64_t>(record.payload.attackDamage)));
  if (record.kind == CommandKind::CastAbility) {
    addEnum(hasher, "ability", record.payload.ability);
    addVec3Field(hasher, "abilityDirection", record.payload.abilityDirection);
  }
  addU64(hasher, "userData0", record.payload.userData0);
  addU64(hasher, "userData1", record.payload.userData1);
  addU64(hasher, "issuedTick", record.issuedTick);
  addU64(hasher, "scheduledTick", record.scheduledTick);
  addEnum(hasher, "admission", record.admission);
  addEnum(hasher, "rejection", record.rejection);
}

}  // namespace

StateHashValue computeStateHash(const SessionState& state) {
  StableHasher hasher;
  addU64(hasher, "schemaVersion", state.identity.schemaVersion);
  addU64(hasher, "minimumReadableSchemaVersion", 1);
  addU64(hasher, "runtimeSaveVersion", 1);
  addStringField(hasher, "packageId", state.identity.packageId);
  addStringField(hasher, "scenarioId", state.identity.scenarioId);
  addU64(hasher, "sessionSeed", state.identity.sessionSeed);
  addEnum(hasher, "lifecycle", state.lifecycle);
  addEnum(hasher, "outcome", state.outcome);
  addU64(hasher, "config.fixedTickRateHz", state.config.fixedTickRateHz);
  hasher.addString("config.interactionRangeMeters");
  hasher.addFloatQuantized(state.config.interactionRangeMeters);
  hasher.addString("config.movementDistanceMeters");
  hasher.addFloatQuantized(state.config.movementDistanceMeters);
  hasher.addString("config.slowTimeScale");
  hasher.addFloatQuantized(state.config.slowTimeScale);
  addU64(hasher, "currentTick", state.clock.tickIndex);
  addU64(hasher, "nextCommandId", state.nextCommandId);

  addU64(hasher, "world.count", state.world.entities().size());
  for (const EntityState& entity : state.world.entities()) {
    addU64(hasher, "entity.id", toUint64(entity.id));
    addStringField(hasher, "entity.stableName", entity.stableName);
    addEnum(hasher, "entity.kind", entity.kind);
    addTransform(hasher, entity.transform);
    addBounds(hasher, entity.localBounds);
    addBoolField(hasher, "entity.active", entity.active);
    addBoolField(hasher, "entity.persistent", entity.persistent);
    addBoolField(hasher, "entity.targetable", entity.targeting.targetable);
    addU64(hasher, "entity.actions.count", entity.targeting.actions.size());
    for (TargetAction action : entity.targeting.actions) {
      addEnum(hasher, "entity.action", action);
    }
    addInteraction(hasher, entity.interaction);
  }
  addU64(hasher, "world.nextEntityId", toUint64(state.world.nextEntityId()));

  addU64(hasher, "players.count", state.players.slots().size());
  for (const PlayerSlot& slot : state.players.slots()) {
    addU64(hasher, "slot.id", slot.id);
    addEnum(hasher, "slot.kind", slot.kind);
    addU64(hasher, "slot.actor", toUint64(slot.actor));
    addStringField(hasher, "slot.stableName", slot.stableName);
  }

  addEnum(hasher, "clock.mode", state.clock.mode);
  addEnum(hasher, "clock.previousUnpausedMode", state.clock.previousUnpausedMode);
  hasher.addString("clock.previousUnpausedTimeScale");
  hasher.addFloatQuantized(state.clock.previousUnpausedTimeScale);
  addU64(hasher, "clock.tickIndex", state.clock.tickIndex);
  addU64(hasher, "clock.fixedTickRateHz", state.clock.fixedTickRateHz);
  hasher.addString("clock.timeScale");
  hasher.addFloatQuantized(state.clock.timeScale);

  addEnum(hasher, "camera.activeMode", state.camera.activeMode);
  addEnum(hasher, "camera.previousRealtimeMode", state.camera.previousRealtimeMode);
  addU64(hasher, "camera.target.entity", toUint64(state.camera.target.entity));
  addVec3Field(hasher, "camera.target.point", state.camera.target.point);
  addBoolField(hasher, "camera.target.hasPoint", state.camera.target.hasPoint);
  hasher.addString("camera.yawDegrees");
  hasher.addFloatQuantized(state.camera.yawDegrees);
  hasher.addString("camera.pitchDegrees");
  hasher.addFloatQuantized(state.camera.pitchDegrees);
  hasher.addString("camera.orbitDistance");
  hasher.addFloatQuantized(state.camera.orbitDistance);

  addU64(hasher, "abilities.actor.count", state.abilities.actors.size());
  for (const AbilityActorState& actor : state.abilities.actors) {
    addU64(hasher, "abilities.actor", toUint64(actor.actor));
    addU64(hasher, "abilities.arcaneFocus", actor.arcaneFocus);
    addU64(hasher, "abilities.arcaneBoltReadyTick", actor.arcaneBoltReadyTick);
    addU64(hasher, "abilities.arcaneFocusNextRechargeTick",
           actor.arcaneFocusNextRechargeTick);
  }

  addU64(hasher, "commandLog.count", state.commandLog.records().size());
  for (const CommandRecord& record : state.commandLog.records()) {
    addCommandRecord(hasher, record);
  }
  addU64(hasher, "commandLog.nextSequence", state.commandLog.nextSequence());
  addU64(hasher, "commandLog.epoch", state.commandLog.epoch());

  addU64(hasher, "inventory.players.count", state.inventory.players.size());
  for (const PlayerInventory& inventory : state.inventory.players) {
    addU64(hasher, "inventory.playerSlot", inventory.playerSlot);
    addU64(hasher, "inventory.stacks.count", inventory.stacks.size());
    for (const InventoryStack& stack : inventory.stacks) {
      addStringField(hasher, "inventory.itemId", stack.itemId);
      addU64(hasher, "inventory.count", stack.count);
    }
  }

  addU64(hasher, "combat.count", state.combat.combatants.size());
  for (const CombatantState& combatant : state.combat.combatants) {
    addU64(hasher, "combat.entity", toUint64(combatant.entity));
    addU64(hasher, "combat.factionId", combatant.factionId);
    addU64(hasher, "combat.hitPoints", static_cast<std::uint64_t>(combatant.hitPoints));
    addU64(hasher, "combat.maxHitPoints", static_cast<std::uint64_t>(combatant.maxHitPoints));
    addBoolField(hasher, "combat.defeated", combatant.defeated);
  }

  addU64(hasher, "ai.count", state.ai.actors.size());
  for (const AiActorState& actor : state.ai.actors) {
    addU64(hasher, "ai.actor", toUint64(actor.actor));
    addU64(hasher, "ai.nextDecisionTick", actor.nextDecisionTick);
    addU64(hasher, "ai.deterministicPolicy", actor.deterministicPolicy);
    addBoolField(hasher, "ai.enabled", actor.enabled);
    addStringField(hasher, "ai.behaviorProfileId", actor.behaviorProfileId);
    addU64(hasher, "ai.target", toUint64(actor.target));
    addEnum(hasher, "ai.behavior", actor.behavior);
    addEnum(hasher, "ai.lastIntent", actor.lastIntent);
    addU64(hasher, "ai.cooldownTicksRemaining", actor.cooldownTicksRemaining);
    addBoolField(hasher, "ai.hasHomePosition", actor.hasHomePosition);
    addVec3Field(hasher, "ai.homePosition", actor.homePosition);
    addStringField(hasher, "ai.homeStableName", actor.homeStableName);
    addFloatField(hasher, "ai.leashRadiusMeters", actor.leashRadiusMeters);
    addFloatField(hasher, "ai.returnRadiusMeters", actor.returnRadiusMeters);
    addFloatField(hasher, "ai.homeToleranceMeters", actor.homeToleranceMeters);
  }

  addU64(hasher, "objectives.count", state.objectives.objectives.size());
  for (const ObjectiveRecord& objective : state.objectives.objectives) {
    addStringField(hasher, "objective.id", objective.objectiveId);
    addEnum(hasher, "objective.status", objective.status);
    addEnum(hasher, "objective.condition.kind", objective.condition.kind);
    addU64(hasher, "objective.condition.playerSlot", objective.condition.playerSlot);
    addStringField(hasher, "objective.condition.itemId", objective.condition.itemId);
    addU64(hasher, "objective.condition.itemCount", objective.condition.itemCount);
  }

  return hasher.value();
}

std::string formatStateHash(StateHashValue value) {
  std::ostringstream out;
  out << std::hex << std::nouppercase << std::setfill('0') << std::setw(16) << value;
  return out.str();
}

}  // namespace iggy3d

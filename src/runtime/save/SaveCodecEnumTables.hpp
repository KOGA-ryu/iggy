#pragma once

#include <array>
#include <string>
#include <string_view>

#include "runtime/save/SaveEnvelope.hpp"

namespace iggy3d::save_codec_detail {

template <typename Enum>
struct SaveEnumName {
  Enum value;
  std::string_view name;
};

template <typename Enum>
struct SaveEnumTable;

template <typename Enum>
std::string enumText(Enum value) {
  static_assert(!SaveEnumTable<Enum>::entries.empty());
  SaveEnumTable<Enum>::requireExhaustive(value);
  for (const SaveEnumName<Enum>& entry : SaveEnumTable<Enum>::entries) {
    // branch-gate: BG-1235
    if (entry.value == value) {
      return std::string(entry.name);
    }
  }
  return std::string(SaveEnumTable<Enum>::entries.front().name);
}

template <typename Enum>
bool parseEnum(std::string_view name, Enum& out) {
  for (const SaveEnumName<Enum>& entry : SaveEnumTable<Enum>::entries) {
    // branch-gate: BG-1235
    if (entry.name == name) {
      out = entry.value;
      return true;
    }
  }
  return false;
}

#define IGGY3D_SAVE_ENUM_ENTRY(Type, Value, Name) \
  SaveEnumName<Type>{Type::Value, Name},

#define IGGY3D_SAVE_ENUM_CASE(Type, Value, Name) \
  case Type::Value:                              \
    break;

#define IGGY3D_DEFINE_SAVE_ENUM_TABLE(Type, Values)                            \
  template <>                                                                 \
  struct SaveEnumTable<Type> {                                                \
    inline static constexpr auto entries = std::to_array<SaveEnumName<Type>>({ \
        Values(IGGY3D_SAVE_ENUM_ENTRY)                                         \
    });                                                                        \
    static constexpr void requireExhaustive(Type value) {                     \
      /* branch-gate: BG-1235 */                                               \
      switch (value) {                                                         \
        Values(IGGY3D_SAVE_ENUM_CASE)                                          \
      }                                                                        \
    }                                                                          \
  }

// The first row in each list is the invalid-value encoder fallback.
#define IGGY3D_SAVE_SESSION_LIFECYCLE_VALUES(X) \
  X(SessionLifecycle, Loading, "Loading") \
  X(SessionLifecycle, Playing, "Playing") \
  X(SessionLifecycle, Paused, "Paused") \
  X(SessionLifecycle, Complete, "Complete") \
  X(SessionLifecycle, Failed, "Failed")

#define IGGY3D_SAVE_SESSION_OUTCOME_VALUES(X) \
  X(SessionOutcome, None, "None") \
  X(SessionOutcome, DemoComplete, "DemoComplete") \
  X(SessionOutcome, Victory, "Victory") \
  X(SessionOutcome, Defeat, "Defeat") \
  X(SessionOutcome, Failed, "Failed")

#define IGGY3D_SAVE_ENTITY_KIND_VALUES(X) \
  X(EntityKind, Unknown, "Unknown") \
  X(EntityKind, Player, "Player") \
  X(EntityKind, Pickup, "Pickup") \
  X(EntityKind, Door, "Door") \
  X(EntityKind, Marker, "Marker") \
  X(EntityKind, Npc, "Npc")

#define IGGY3D_SAVE_TARGET_ACTION_VALUES(X) \
  X(TargetAction, Interact, "Interact") \
  X(TargetAction, Inspect, "Inspect") \
  X(TargetAction, Attack, "Attack") \
  X(TargetAction, Move, "Move")

#define IGGY3D_SAVE_INTERACTION_KIND_VALUES(X) \
  X(InteractionKind, None, "None") \
  X(InteractionKind, Pickup, "Pickup") \
  X(InteractionKind, Activate, "Activate") \
  X(InteractionKind, OpenDoor, "OpenDoor") \
  X(InteractionKind, Inspect, "Inspect") \
  X(InteractionKind, ObjectiveTrigger, "ObjectiveTrigger")

#define IGGY3D_SAVE_INTERACTION_EFFECT_KIND_VALUES(X) \
  X(InteractionEffectKind, None, "None") \
  X(InteractionEffectKind, AddItemToInventory, "AddItemToInventory") \
  X(InteractionEffectKind, DeactivateTarget, "DeactivateTarget") \
  X(InteractionEffectKind, CompleteObjective, "CompleteObjective") \
  X(InteractionEffectKind, EmitEventOnly, "EmitEventOnly")

#define IGGY3D_SAVE_PLAYER_SLOT_KIND_VALUES(X) \
  X(PlayerSlotKind, Unknown, "Unknown") \
  X(PlayerSlotKind, Local, "Local") \
  X(PlayerSlotKind, Remote, "Remote") \
  X(PlayerSlotKind, Ai, "Ai") \
  X(PlayerSlotKind, Observer, "Observer")

#define IGGY3D_SAVE_AI_BEHAVIOR_KIND_VALUES(X) \
  X(AiBehaviorKind, None, "none") \
  X(AiBehaviorKind, Idle, "idle") \
  X(AiBehaviorKind, Alert, "alert") \
  X(AiBehaviorKind, Chasing, "chasing") \
  X(AiBehaviorKind, Attacking, "attacking") \
  X(AiBehaviorKind, Defeated, "defeated") \
  X(AiBehaviorKind, Returning, "returning") \
  X(AiBehaviorKind, Observant, "observant") \
  X(AiBehaviorKind, Suspicious, "suspicious") \
  X(AiBehaviorKind, Searching, "searching")

#define IGGY3D_SAVE_AI_INTENT_KIND_VALUES(X) \
  X(AiIntentKind, None, "none") \
  X(AiIntentKind, Wait, "wait") \
  X(AiIntentKind, MoveTowardTarget, "move_toward_target") \
  X(AiIntentKind, AttackTarget, "attack_target") \
  X(AiIntentKind, ReturnToAnchor, "return_to_anchor") \
  X(AiIntentKind, Patrol, "patrol") \
  X(AiIntentKind, Investigate, "investigate")

#define IGGY3D_SAVE_PATROL_MODE_VALUES(X) \
  X(PatrolMode, Loop, "loop") \
  X(PatrolMode, PingPong, "ping_pong")

#define IGGY3D_SAVE_CLOCK_MODE_VALUES(X) \
  X(ClockMode, Normal, "Normal") \
  X(ClockMode, Slow, "Slow") \
  X(ClockMode, Paused, "Paused")

#define IGGY3D_SAVE_CAMERA_MODE_VALUES(X) \
  X(CameraMode, ThirdPerson, "ThirdPerson") \
  X(CameraMode, FirstPerson, "FirstPerson") \
  X(CameraMode, TacticalOverhead, "TacticalOverhead")

#define IGGY3D_SAVE_COMMAND_LOG_RESET_POLICY_VALUES(X) \
  X(CommandLogResetPolicy, Clear, "Clear") \
  X(CommandLogResetPolicy, NewEpoch, "NewEpoch")

#define IGGY3D_SAVE_COMMAND_KIND_VALUES(X) \
  X(CommandKind, None, "None") \
  X(CommandKind, Move, "Move") \
  X(CommandKind, Interact, "Interact") \
  X(CommandKind, Inspect, "Inspect") \
  X(CommandKind, Attack, "Attack") \
  X(CommandKind, CastAbility, "CastAbility") \
  X(CommandKind, Wait, "Wait") \
  X(CommandKind, ToggleTacticalMode, "ToggleTacticalMode") \
  X(CommandKind, Pause, "Pause") \
  X(CommandKind, Resume, "Resume") \
  X(CommandKind, StepTacticalTick, "StepTacticalTick") \
  X(CommandKind, Retry, "Retry") \
  X(CommandKind, Reset, "Reset") \
  X(CommandKind, Save, "Save") \
  X(CommandKind, Load, "Load")

#define IGGY3D_SAVE_COMMAND_ABILITY_KIND_VALUES(X) \
  X(CommandAbilityKind, None, "None") \
  X(CommandAbilityKind, ArcaneBolt, "ArcaneBolt")

#define IGGY3D_SAVE_COMMAND_SOURCE_VALUES(X) \
  X(CommandSource, Unknown, "Unknown") \
  X(CommandSource, LocalPlayer, "LocalPlayer") \
  X(CommandSource, Ai, "Ai") \
  X(CommandSource, Script, "Script") \
  X(CommandSource, Replay, "Replay") \
  X(CommandSource, RemotePlayer, "RemotePlayer") \
  X(CommandSource, Tool, "Tool")

#define IGGY3D_SAVE_COMMAND_ADMISSION_STATUS_VALUES(X) \
  X(CommandAdmissionStatus, Pending, "Pending") \
  X(CommandAdmissionStatus, Accepted, "Accepted") \
  X(CommandAdmissionStatus, Rejected, "Rejected")

#define IGGY3D_SAVE_COMMAND_REJECTION_REASON_VALUES(X) \
  X(CommandRejectionReason, None, "None") \
  X(CommandRejectionReason, InvalidCommand, "InvalidCommand") \
  X(CommandRejectionReason, InvalidPlayerSlot, "InvalidPlayerSlot") \
  X(CommandRejectionReason, UnauthorizedSlot, "UnauthorizedSlot") \
  X(CommandRejectionReason, InvalidActor, "InvalidActor") \
  X(CommandRejectionReason, ActorNotControlledBySlot, "ActorNotControlledBySlot") \
  X(CommandRejectionReason, InvalidTarget, "InvalidTarget") \
  X(CommandRejectionReason, TargetInactive, "TargetInactive") \
  X(CommandRejectionReason, TargetNotReachable, "TargetNotReachable") \
  X(CommandRejectionReason, OutOfRange, "OutOfRange") \
  X(CommandRejectionReason, InvalidTargetPoint, "InvalidTargetPoint") \
  X(CommandRejectionReason, MovementTooFar, "MovementTooFar") \
  X(CommandRejectionReason, InvalidDamage, "InvalidDamage") \
  X(CommandRejectionReason, TargetDefeated, "TargetDefeated") \
  X(CommandRejectionReason, AttackerDefeated, "AttackerDefeated") \
  X(CommandRejectionReason, FriendlyFireBlocked, "FriendlyFireBlocked") \
  X(CommandRejectionReason, SessionNotPlaying, "SessionNotPlaying") \
  X(CommandRejectionReason, SessionPaused, "SessionPaused") \
  X(CommandRejectionReason, StepRequiresPaused, "StepRequiresPaused") \
  X(CommandRejectionReason, RetrySourceMissing, "RetrySourceMissing") \
  X(CommandRejectionReason, RetrySourceNotRejected, "RetrySourceNotRejected") \
  X(CommandRejectionReason, RetryUnsupportedKind, "RetryUnsupportedKind") \
  X(CommandRejectionReason, ResetUnavailable, "ResetUnavailable") \
  X(CommandRejectionReason, SaveUnavailable, "SaveUnavailable") \
  X(CommandRejectionReason, LoadUnavailable, "LoadUnavailable") \
  X(CommandRejectionReason, IncompatibleSave, "IncompatibleSave") \
  X(CommandRejectionReason, RequiredItemMissing, "RequiredItemMissing") \
  X(CommandRejectionReason, AbilitySlotBusy, "AbilitySlotBusy") \
  X(CommandRejectionReason, AbilityOnCooldown, "AbilityOnCooldown") \
  X(CommandRejectionReason, AbilityInsufficientResource, "AbilityInsufficientResource") \
  X(CommandRejectionReason, InternalError, "InternalError")

#define IGGY3D_SAVE_OBJECTIVE_STATUS_VALUES(X) \
  X(ObjectiveStatus, Inactive, "Inactive") \
  X(ObjectiveStatus, Active, "Active") \
  X(ObjectiveStatus, Complete, "Complete") \
  X(ObjectiveStatus, Failed, "Failed")

#define IGGY3D_SAVE_OBJECTIVE_CONDITION_KIND_VALUES(X) \
  X(ObjectiveConditionKind, None, "None") \
  X(ObjectiveConditionKind, PlayerHasItem, "PlayerHasItem")

IGGY3D_DEFINE_SAVE_ENUM_TABLE(SessionLifecycle, IGGY3D_SAVE_SESSION_LIFECYCLE_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(SessionOutcome, IGGY3D_SAVE_SESSION_OUTCOME_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(EntityKind, IGGY3D_SAVE_ENTITY_KIND_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(TargetAction, IGGY3D_SAVE_TARGET_ACTION_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(InteractionKind, IGGY3D_SAVE_INTERACTION_KIND_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(InteractionEffectKind, IGGY3D_SAVE_INTERACTION_EFFECT_KIND_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(PlayerSlotKind, IGGY3D_SAVE_PLAYER_SLOT_KIND_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(AiBehaviorKind, IGGY3D_SAVE_AI_BEHAVIOR_KIND_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(AiIntentKind, IGGY3D_SAVE_AI_INTENT_KIND_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(PatrolMode, IGGY3D_SAVE_PATROL_MODE_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(ClockMode, IGGY3D_SAVE_CLOCK_MODE_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(CameraMode, IGGY3D_SAVE_CAMERA_MODE_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(CommandLogResetPolicy, IGGY3D_SAVE_COMMAND_LOG_RESET_POLICY_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(CommandKind, IGGY3D_SAVE_COMMAND_KIND_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(CommandAbilityKind, IGGY3D_SAVE_COMMAND_ABILITY_KIND_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(CommandSource, IGGY3D_SAVE_COMMAND_SOURCE_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(CommandAdmissionStatus, IGGY3D_SAVE_COMMAND_ADMISSION_STATUS_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(CommandRejectionReason, IGGY3D_SAVE_COMMAND_REJECTION_REASON_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(ObjectiveStatus, IGGY3D_SAVE_OBJECTIVE_STATUS_VALUES);
IGGY3D_DEFINE_SAVE_ENUM_TABLE(ObjectiveConditionKind, IGGY3D_SAVE_OBJECTIVE_CONDITION_KIND_VALUES);

#undef IGGY3D_SAVE_SESSION_LIFECYCLE_VALUES
#undef IGGY3D_SAVE_SESSION_OUTCOME_VALUES
#undef IGGY3D_SAVE_ENTITY_KIND_VALUES
#undef IGGY3D_SAVE_TARGET_ACTION_VALUES
#undef IGGY3D_SAVE_INTERACTION_KIND_VALUES
#undef IGGY3D_SAVE_INTERACTION_EFFECT_KIND_VALUES
#undef IGGY3D_SAVE_PLAYER_SLOT_KIND_VALUES
#undef IGGY3D_SAVE_AI_BEHAVIOR_KIND_VALUES
#undef IGGY3D_SAVE_AI_INTENT_KIND_VALUES
#undef IGGY3D_SAVE_PATROL_MODE_VALUES
#undef IGGY3D_SAVE_CLOCK_MODE_VALUES
#undef IGGY3D_SAVE_CAMERA_MODE_VALUES
#undef IGGY3D_SAVE_COMMAND_LOG_RESET_POLICY_VALUES
#undef IGGY3D_SAVE_COMMAND_KIND_VALUES
#undef IGGY3D_SAVE_COMMAND_ABILITY_KIND_VALUES
#undef IGGY3D_SAVE_COMMAND_SOURCE_VALUES
#undef IGGY3D_SAVE_COMMAND_ADMISSION_STATUS_VALUES
#undef IGGY3D_SAVE_COMMAND_REJECTION_REASON_VALUES
#undef IGGY3D_SAVE_OBJECTIVE_STATUS_VALUES
#undef IGGY3D_SAVE_OBJECTIVE_CONDITION_KIND_VALUES
#undef IGGY3D_DEFINE_SAVE_ENUM_TABLE
#undef IGGY3D_SAVE_ENUM_CASE
#undef IGGY3D_SAVE_ENUM_ENTRY

}  // namespace iggy3d::save_codec_detail

#pragma once

#include <cstdint>
#include <string>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/player/PlayerSlot.hpp"

namespace iggy3d {

using CommandId = std::uint64_t;
using CommandSequence = std::uint64_t;
using CommandTick = std::uint64_t;

inline constexpr CommandId kInvalidCommandId = 0;
inline constexpr CommandSequence kInvalidCommandSequence = 0;
inline constexpr CommandTick kInvalidCommandTick = 0;

enum class CommandKind : std::uint8_t {
  None,
  Move,
  Interact,
  Inspect,
  Attack,
  Wait,
  ToggleTacticalMode,
  Pause,
  Resume,
  StepTacticalTick,
  Retry,
  Reset,
  Save,
  Load,
  CastAbility,
};

enum class CommandAbilityKind : std::uint8_t {
  None,
  ArcaneBolt,
};

enum class CommandSource : std::uint8_t {
  Unknown,
  LocalPlayer,
  Ai,
  Script,
  Replay,
  RemotePlayer,
  Tool,
};

enum class CommandAdmissionStatus : std::uint8_t {
  Pending,
  Accepted,
  Rejected,
};

enum class CommandRejectionReason : std::uint8_t {
  None,
  InvalidCommand,
  InvalidPlayerSlot,
  UnauthorizedSlot,
  InvalidActor,
  ActorNotControlledBySlot,
  InvalidTarget,
  TargetInactive,
  TargetNotReachable,
  OutOfRange,
  InvalidTargetPoint,
  MovementTooFar,
  InvalidDamage,
  TargetDefeated,
  AttackerDefeated,
  FriendlyFireBlocked,
  SessionNotPlaying,
  SessionPaused,
  StepRequiresPaused,
  RetrySourceMissing,
  RetrySourceNotRejected,
  RetryUnsupportedKind,
  ResetUnavailable,
  SaveUnavailable,
  LoadUnavailable,
  IncompatibleSave,
  RequiredItemMissing,
  AbilitySlotBusy,
  AbilityOnCooldown,
  AbilityInsufficientResource,
  InternalError,
};

struct CommandTarget {
  bool hasEntity = false;
  EntityId entity;
  bool hasPoint = false;
  Vec3 point;
};

// MA1 s2: the sneak stance rides an EXISTING userData0 bit on Move (a documented constant, NOT a new
// named payload field). userData0 is already hashed (deterministic/replay-sound) and now saved; the
// bit is the single carrier of per-command stance from admission through to footstep loudness.
inline constexpr std::uint64_t kMoveSneakBit = 1ULL << 0;

struct CommandPayload {
  CommandTarget target;
  CommandId retrySourceCommandId = kInvalidCommandId;
  std::int32_t attackDamage = 0;
  CommandAbilityKind ability = CommandAbilityKind::None;
  Vec3 abilityDirection;
  std::uint64_t userData0 = 0;  // bitfield; bit 0 = kMoveSneakBit (Move stance)
  std::uint64_t userData1 = 0;
};

struct CommandRecord {
  CommandId commandId = kInvalidCommandId;
  CommandSequence sequence = kInvalidCommandSequence;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  EntityId actor;
  CommandKind kind = CommandKind::None;
  CommandSource source = CommandSource::Unknown;
  CommandPayload payload;
  CommandTick issuedTick = kInvalidCommandTick;
  CommandTick scheduledTick = kInvalidCommandTick;
  CommandAdmissionStatus admission = CommandAdmissionStatus::Pending;
  CommandRejectionReason rejection = CommandRejectionReason::None;
};

inline bool requiresActor(CommandKind kind) {
  return kind == CommandKind::Move || kind == CommandKind::Interact ||
         kind == CommandKind::Inspect || kind == CommandKind::Attack ||
         kind == CommandKind::CastAbility || kind == CommandKind::Wait ||
         kind == CommandKind::Retry;
}

inline bool requiresEntityTarget(CommandKind kind) {
  return kind == CommandKind::Interact || kind == CommandKind::Inspect ||
         kind == CommandKind::Attack;
}

inline bool requiresPointTarget(CommandKind kind) {
  return kind == CommandKind::Move;
}

inline bool requiresAbilityPayload(CommandKind kind) {
  return kind == CommandKind::CastAbility;
}

inline bool isValidCommandAbility(CommandAbilityKind ability) {
  return ability == CommandAbilityKind::ArcaneBolt;
}

}  // namespace iggy3d

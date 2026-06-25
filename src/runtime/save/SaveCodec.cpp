#include "runtime/save/SaveCodec.hpp"

#include <charconv>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace iggy3d {

namespace {

inline constexpr std::string_view kHeader = "iggy3d.save_envelope.v1";
inline constexpr std::size_t kMaxSaveBytes = 1024U * 1024U;

std::string sectionForKey(const std::string& key) {
  const std::size_t dot = key.find('.');
  return dot == std::string::npos ? key : key.substr(0, dot);
}

std::string escapeString(const std::string& value) {
  std::string out;
  for (char c : value) {
    switch (c) {
      case '%':
        out += "%25";
        break;
      case '\n':
        out += "%0A";
        break;
      case '\r':
        out += "%0D";
        break;
      case '=':
        out += "%3D";
        break;
      default:
        out += c;
        break;
    }
  }
  return out;
}

bool hexDigit(char c, int& value) {
  if (c >= '0' && c <= '9') {
    value = c - '0';
    return true;
  }
  if (c >= 'A' && c <= 'F') {
    value = 10 + c - 'A';
    return true;
  }
  return false;
}

bool unescapeString(std::string_view value, std::string& out) {
  out.clear();
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] != '%') {
      out.push_back(value[index]);
      continue;
    }
    if (index + 2U >= value.size()) {
      return false;
    }
    int high = 0;
    int low = 0;
    if (!hexDigit(value[index + 1U], high) || !hexDigit(value[index + 2U], low)) {
      return false;
    }
    const int byte = high * 16 + low;
    if (byte == 0x25) {
      out.push_back('%');
    } else if (byte == 0x0A) {
      out.push_back('\n');
    } else if (byte == 0x0D) {
      out.push_back('\r');
    } else if (byte == 0x3D) {
      out.push_back('=');
    } else {
      return false;
    }
    index += 2U;
  }
  return true;
}

std::string formatFloat(float value) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(3) << value;
  return out.str();
}

std::string formatVec3(Vec3 value) {
  return formatFloat(value.x) + "," + formatFloat(value.y) + "," + formatFloat(value.z);
}

bool parseFloat(std::string_view value, float& out) {
  std::string text(value);
  char* end = nullptr;
  out = std::strtof(text.c_str(), &end);
  return end != text.c_str() && *end == '\0' && std::isfinite(out);
}

bool parseVec3(std::string_view value, Vec3& out) {
  const std::size_t first = value.find(',');
  const std::size_t second = first == std::string_view::npos ? std::string_view::npos
                                                             : value.find(',', first + 1U);
  if (first == std::string_view::npos || second == std::string_view::npos ||
      value.find(',', second + 1U) != std::string_view::npos) {
    return false;
  }
  return parseFloat(value.substr(0, first), out.x) &&
         parseFloat(value.substr(first + 1U, second - first - 1U), out.y) &&
         parseFloat(value.substr(second + 1U), out.z);
}

template <typename T>
std::string unsignedText(T value) {
  return std::to_string(static_cast<std::uint64_t>(value));
}

template <typename T>
bool parseUnsigned(std::string_view value, T& out) {
  if (value.empty() || value.front() == '+') {
    return false;
  }
  std::uint64_t parsed = 0;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end ||
      parsed > static_cast<std::uint64_t>(std::numeric_limits<T>::max())) {
    return false;
  }
  out = static_cast<T>(parsed);
  return true;
}

bool parseI32(std::string_view value, std::int32_t& out) {
  std::int64_t parsed = 0;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end ||
      parsed < std::numeric_limits<std::int32_t>::min() ||
      parsed > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  out = static_cast<std::int32_t>(parsed);
  return true;
}

bool parseBool(std::string_view value, bool& out) {
  if (value == "true") {
    out = true;
    return true;
  }
  if (value == "false") {
    out = false;
    return true;
  }
  return false;
}

template <typename Enum>
std::string enumText(Enum value);

template <>
std::string enumText(SessionLifecycle value) {
  switch (value) {
    case SessionLifecycle::Loading: return "Loading";
    case SessionLifecycle::Playing: return "Playing";
    case SessionLifecycle::Paused: return "Paused";
    case SessionLifecycle::Complete: return "Complete";
    case SessionLifecycle::Failed: return "Failed";
  }
  return "Loading";
}

template <>
std::string enumText(SessionOutcome value) {
  switch (value) {
    case SessionOutcome::None: return "None";
    case SessionOutcome::DemoComplete: return "DemoComplete";
    case SessionOutcome::Victory: return "Victory";
    case SessionOutcome::Defeat: return "Defeat";
    case SessionOutcome::Failed: return "Failed";
  }
  return "None";
}

template <>
std::string enumText(EntityKind value) {
  switch (value) {
    case EntityKind::Unknown: return "Unknown";
    case EntityKind::Player: return "Player";
    case EntityKind::Pickup: return "Pickup";
    case EntityKind::Door: return "Door";
    case EntityKind::Marker: return "Marker";
    case EntityKind::Npc: return "Npc";
  }
  return "Unknown";
}

template <>
std::string enumText(TargetAction value) {
  switch (value) {
    case TargetAction::Interact: return "Interact";
    case TargetAction::Inspect: return "Inspect";
    case TargetAction::Attack: return "Attack";
    case TargetAction::Move: return "Move";
  }
  return "Interact";
}

template <>
std::string enumText(InteractionKind value) {
  switch (value) {
    case InteractionKind::None: return "None";
    case InteractionKind::Pickup: return "Pickup";
    case InteractionKind::Activate: return "Activate";
    case InteractionKind::OpenDoor: return "OpenDoor";
    case InteractionKind::Inspect: return "Inspect";
    case InteractionKind::ObjectiveTrigger: return "ObjectiveTrigger";
  }
  return "None";
}

template <>
std::string enumText(InteractionEffectKind value) {
  switch (value) {
    case InteractionEffectKind::None: return "None";
    case InteractionEffectKind::AddItemToInventory: return "AddItemToInventory";
    case InteractionEffectKind::DeactivateTarget: return "DeactivateTarget";
    case InteractionEffectKind::CompleteObjective: return "CompleteObjective";
    case InteractionEffectKind::EmitEventOnly: return "EmitEventOnly";
  }
  return "None";
}

template <>
std::string enumText(PlayerSlotKind value) {
  switch (value) {
    case PlayerSlotKind::Unknown: return "Unknown";
    case PlayerSlotKind::Local: return "Local";
    case PlayerSlotKind::Remote: return "Remote";
    case PlayerSlotKind::Ai: return "Ai";
    case PlayerSlotKind::Observer: return "Observer";
  }
  return "Unknown";
}

template <>
std::string enumText(AiBehaviorKind value) {
  return std::string(aiBehaviorKindName(value));
}

template <>
std::string enumText(AiIntentKind value) {
  return std::string(aiIntentKindName(value));
}

template <>
std::string enumText(ClockMode value) {
  switch (value) {
    case ClockMode::Normal: return "Normal";
    case ClockMode::Slow: return "Slow";
    case ClockMode::Paused: return "Paused";
  }
  return "Normal";
}

template <>
std::string enumText(CameraMode value) {
  switch (value) {
    case CameraMode::FirstPerson: return "FirstPerson";
    case CameraMode::ThirdPerson: return "ThirdPerson";
    case CameraMode::TacticalOverhead: return "TacticalOverhead";
  }
  return "ThirdPerson";
}

template <>
std::string enumText(CommandLogResetPolicy value) {
  switch (value) {
    case CommandLogResetPolicy::Clear: return "Clear";
    case CommandLogResetPolicy::NewEpoch: return "NewEpoch";
  }
  return "Clear";
}

template <>
std::string enumText(CommandKind value) {
  switch (value) {
    case CommandKind::None: return "None";
    case CommandKind::Move: return "Move";
    case CommandKind::Interact: return "Interact";
    case CommandKind::Inspect: return "Inspect";
    case CommandKind::Attack: return "Attack";
    case CommandKind::CastAbility: return "CastAbility";
    case CommandKind::Wait: return "Wait";
    case CommandKind::ToggleTacticalMode: return "ToggleTacticalMode";
    case CommandKind::Pause: return "Pause";
    case CommandKind::Resume: return "Resume";
    case CommandKind::StepTacticalTick: return "StepTacticalTick";
    case CommandKind::Retry: return "Retry";
    case CommandKind::Reset: return "Reset";
    case CommandKind::Save: return "Save";
    case CommandKind::Load: return "Load";
  }
  return "None";
}

template <>
std::string enumText(CommandAbilityKind value) {
  switch (value) {
    case CommandAbilityKind::None: return "None";
    case CommandAbilityKind::ArcaneBolt: return "ArcaneBolt";
  }
  return "None";
}

template <>
std::string enumText(CommandSource value) {
  switch (value) {
    case CommandSource::Unknown: return "Unknown";
    case CommandSource::LocalPlayer: return "LocalPlayer";
    case CommandSource::Ai: return "Ai";
    case CommandSource::Script: return "Script";
    case CommandSource::Replay: return "Replay";
    case CommandSource::RemotePlayer: return "RemotePlayer";
    case CommandSource::Tool: return "Tool";
  }
  return "Unknown";
}

template <>
std::string enumText(CommandAdmissionStatus value) {
  switch (value) {
    case CommandAdmissionStatus::Pending: return "Pending";
    case CommandAdmissionStatus::Accepted: return "Accepted";
    case CommandAdmissionStatus::Rejected: return "Rejected";
  }
  return "Pending";
}

template <>
std::string enumText(CommandRejectionReason value) {
  switch (value) {
    case CommandRejectionReason::None: return "None";
    case CommandRejectionReason::InvalidCommand: return "InvalidCommand";
    case CommandRejectionReason::InvalidPlayerSlot: return "InvalidPlayerSlot";
    case CommandRejectionReason::UnauthorizedSlot: return "UnauthorizedSlot";
    case CommandRejectionReason::InvalidActor: return "InvalidActor";
    case CommandRejectionReason::ActorNotControlledBySlot: return "ActorNotControlledBySlot";
    case CommandRejectionReason::InvalidTarget: return "InvalidTarget";
    case CommandRejectionReason::TargetInactive: return "TargetInactive";
    case CommandRejectionReason::TargetNotReachable: return "TargetNotReachable";
    case CommandRejectionReason::OutOfRange: return "OutOfRange";
    case CommandRejectionReason::InvalidTargetPoint: return "InvalidTargetPoint";
    case CommandRejectionReason::MovementTooFar: return "MovementTooFar";
    case CommandRejectionReason::InvalidDamage: return "InvalidDamage";
    case CommandRejectionReason::TargetDefeated: return "TargetDefeated";
    case CommandRejectionReason::AttackerDefeated: return "AttackerDefeated";
    case CommandRejectionReason::FriendlyFireBlocked: return "FriendlyFireBlocked";
    case CommandRejectionReason::SessionNotPlaying: return "SessionNotPlaying";
    case CommandRejectionReason::SessionPaused: return "SessionPaused";
    case CommandRejectionReason::StepRequiresPaused: return "StepRequiresPaused";
    case CommandRejectionReason::RetrySourceMissing: return "RetrySourceMissing";
    case CommandRejectionReason::RetrySourceNotRejected: return "RetrySourceNotRejected";
    case CommandRejectionReason::RetryUnsupportedKind: return "RetryUnsupportedKind";
    case CommandRejectionReason::ResetUnavailable: return "ResetUnavailable";
    case CommandRejectionReason::SaveUnavailable: return "SaveUnavailable";
    case CommandRejectionReason::LoadUnavailable: return "LoadUnavailable";
    case CommandRejectionReason::IncompatibleSave: return "IncompatibleSave";
    case CommandRejectionReason::RequiredItemMissing: return "RequiredItemMissing";
    case CommandRejectionReason::AbilitySlotBusy: return "AbilitySlotBusy";
    case CommandRejectionReason::AbilityOnCooldown: return "AbilityOnCooldown";
    case CommandRejectionReason::AbilityInsufficientResource: return "AbilityInsufficientResource";
    case CommandRejectionReason::InternalError: return "InternalError";
  }
  return "None";
}

template <>
std::string enumText(ObjectiveStatus value) {
  switch (value) {
    case ObjectiveStatus::Inactive: return "Inactive";
    case ObjectiveStatus::Active: return "Active";
    case ObjectiveStatus::Complete: return "Complete";
    case ObjectiveStatus::Failed: return "Failed";
  }
  return "Inactive";
}

template <>
std::string enumText(ObjectiveConditionKind value) {
  switch (value) {
    case ObjectiveConditionKind::None: return "None";
    case ObjectiveConditionKind::PlayerHasItem: return "PlayerHasItem";
  }
  return "None";
}

template <typename Enum>
bool parseEnum(std::string_view value, Enum& out);

#define IGGY3D_ENUM_PARSE(Type, Value) \
  if (value == #Value) { out = Type::Value; return true; }

template <>
bool parseEnum(std::string_view value, SessionLifecycle& out) {
  IGGY3D_ENUM_PARSE(SessionLifecycle, Loading)
  IGGY3D_ENUM_PARSE(SessionLifecycle, Playing)
  IGGY3D_ENUM_PARSE(SessionLifecycle, Paused)
  IGGY3D_ENUM_PARSE(SessionLifecycle, Complete)
  IGGY3D_ENUM_PARSE(SessionLifecycle, Failed)
  return false;
}

template <>
bool parseEnum(std::string_view value, SessionOutcome& out) {
  IGGY3D_ENUM_PARSE(SessionOutcome, None)
  IGGY3D_ENUM_PARSE(SessionOutcome, DemoComplete)
  IGGY3D_ENUM_PARSE(SessionOutcome, Victory)
  IGGY3D_ENUM_PARSE(SessionOutcome, Defeat)
  IGGY3D_ENUM_PARSE(SessionOutcome, Failed)
  return false;
}

template <>
bool parseEnum(std::string_view value, EntityKind& out) {
  IGGY3D_ENUM_PARSE(EntityKind, Unknown)
  IGGY3D_ENUM_PARSE(EntityKind, Player)
  IGGY3D_ENUM_PARSE(EntityKind, Pickup)
  IGGY3D_ENUM_PARSE(EntityKind, Door)
  IGGY3D_ENUM_PARSE(EntityKind, Marker)
  IGGY3D_ENUM_PARSE(EntityKind, Npc)
  return false;
}

template <>
bool parseEnum(std::string_view value, TargetAction& out) {
  IGGY3D_ENUM_PARSE(TargetAction, Interact)
  IGGY3D_ENUM_PARSE(TargetAction, Inspect)
  IGGY3D_ENUM_PARSE(TargetAction, Attack)
  IGGY3D_ENUM_PARSE(TargetAction, Move)
  return false;
}

template <>
bool parseEnum(std::string_view value, InteractionKind& out) {
  IGGY3D_ENUM_PARSE(InteractionKind, None)
  IGGY3D_ENUM_PARSE(InteractionKind, Pickup)
  IGGY3D_ENUM_PARSE(InteractionKind, Activate)
  IGGY3D_ENUM_PARSE(InteractionKind, OpenDoor)
  IGGY3D_ENUM_PARSE(InteractionKind, Inspect)
  IGGY3D_ENUM_PARSE(InteractionKind, ObjectiveTrigger)
  return false;
}

template <>
bool parseEnum(std::string_view value, InteractionEffectKind& out) {
  IGGY3D_ENUM_PARSE(InteractionEffectKind, None)
  IGGY3D_ENUM_PARSE(InteractionEffectKind, AddItemToInventory)
  IGGY3D_ENUM_PARSE(InteractionEffectKind, DeactivateTarget)
  IGGY3D_ENUM_PARSE(InteractionEffectKind, CompleteObjective)
  IGGY3D_ENUM_PARSE(InteractionEffectKind, EmitEventOnly)
  return false;
}

template <>
bool parseEnum(std::string_view value, PlayerSlotKind& out) {
  IGGY3D_ENUM_PARSE(PlayerSlotKind, Unknown)
  IGGY3D_ENUM_PARSE(PlayerSlotKind, Local)
  IGGY3D_ENUM_PARSE(PlayerSlotKind, Remote)
  IGGY3D_ENUM_PARSE(PlayerSlotKind, Ai)
  IGGY3D_ENUM_PARSE(PlayerSlotKind, Observer)
  return false;
}

template <>
bool parseEnum(std::string_view value, AiBehaviorKind& out) {
  if (value == "none") {
    out = AiBehaviorKind::None;
    return true;
  }
  if (value == "idle") {
    out = AiBehaviorKind::Idle;
    return true;
  }
  if (value == "alert") {
    out = AiBehaviorKind::Alert;
    return true;
  }
  if (value == "chasing") {
    out = AiBehaviorKind::Chasing;
    return true;
  }
  if (value == "attacking") {
    out = AiBehaviorKind::Attacking;
    return true;
  }
  if (value == "defeated") {
    out = AiBehaviorKind::Defeated;
    return true;
  }
  if (value == "returning") {
    out = AiBehaviorKind::Returning;
    return true;
  }
  return false;
}

template <>
bool parseEnum(std::string_view value, AiIntentKind& out) {
  if (value == "none") {
    out = AiIntentKind::None;
    return true;
  }
  if (value == "wait") {
    out = AiIntentKind::Wait;
    return true;
  }
  if (value == "move_toward_target") {
    out = AiIntentKind::MoveTowardTarget;
    return true;
  }
  if (value == "attack_target") {
    out = AiIntentKind::AttackTarget;
    return true;
  }
  if (value == "return_to_anchor") {
    out = AiIntentKind::ReturnToAnchor;
    return true;
  }
  return false;
}

template <>
bool parseEnum(std::string_view value, ClockMode& out) {
  IGGY3D_ENUM_PARSE(ClockMode, Normal)
  IGGY3D_ENUM_PARSE(ClockMode, Slow)
  IGGY3D_ENUM_PARSE(ClockMode, Paused)
  return false;
}

template <>
bool parseEnum(std::string_view value, CameraMode& out) {
  IGGY3D_ENUM_PARSE(CameraMode, FirstPerson)
  IGGY3D_ENUM_PARSE(CameraMode, ThirdPerson)
  IGGY3D_ENUM_PARSE(CameraMode, TacticalOverhead)
  return false;
}

template <>
bool parseEnum(std::string_view value, CommandLogResetPolicy& out) {
  IGGY3D_ENUM_PARSE(CommandLogResetPolicy, Clear)
  IGGY3D_ENUM_PARSE(CommandLogResetPolicy, NewEpoch)
  return false;
}

template <>
bool parseEnum(std::string_view value, CommandKind& out) {
  IGGY3D_ENUM_PARSE(CommandKind, None)
  IGGY3D_ENUM_PARSE(CommandKind, Move)
  IGGY3D_ENUM_PARSE(CommandKind, Interact)
  IGGY3D_ENUM_PARSE(CommandKind, Inspect)
  IGGY3D_ENUM_PARSE(CommandKind, Attack)
  IGGY3D_ENUM_PARSE(CommandKind, CastAbility)
  IGGY3D_ENUM_PARSE(CommandKind, Wait)
  IGGY3D_ENUM_PARSE(CommandKind, ToggleTacticalMode)
  IGGY3D_ENUM_PARSE(CommandKind, Pause)
  IGGY3D_ENUM_PARSE(CommandKind, Resume)
  IGGY3D_ENUM_PARSE(CommandKind, StepTacticalTick)
  IGGY3D_ENUM_PARSE(CommandKind, Retry)
  IGGY3D_ENUM_PARSE(CommandKind, Reset)
  IGGY3D_ENUM_PARSE(CommandKind, Save)
  IGGY3D_ENUM_PARSE(CommandKind, Load)
  return false;
}

template <>
bool parseEnum(std::string_view value, CommandAbilityKind& out) {
  IGGY3D_ENUM_PARSE(CommandAbilityKind, None)
  IGGY3D_ENUM_PARSE(CommandAbilityKind, ArcaneBolt)
  return false;
}

template <>
bool parseEnum(std::string_view value, CommandSource& out) {
  IGGY3D_ENUM_PARSE(CommandSource, Unknown)
  IGGY3D_ENUM_PARSE(CommandSource, LocalPlayer)
  IGGY3D_ENUM_PARSE(CommandSource, Ai)
  IGGY3D_ENUM_PARSE(CommandSource, Script)
  IGGY3D_ENUM_PARSE(CommandSource, Replay)
  IGGY3D_ENUM_PARSE(CommandSource, RemotePlayer)
  IGGY3D_ENUM_PARSE(CommandSource, Tool)
  return false;
}

template <>
bool parseEnum(std::string_view value, CommandAdmissionStatus& out) {
  IGGY3D_ENUM_PARSE(CommandAdmissionStatus, Pending)
  IGGY3D_ENUM_PARSE(CommandAdmissionStatus, Accepted)
  IGGY3D_ENUM_PARSE(CommandAdmissionStatus, Rejected)
  return false;
}

template <>
bool parseEnum(std::string_view value, CommandRejectionReason& out) {
  IGGY3D_ENUM_PARSE(CommandRejectionReason, None)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, InvalidCommand)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, InvalidPlayerSlot)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, UnauthorizedSlot)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, InvalidActor)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, ActorNotControlledBySlot)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, InvalidTarget)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, TargetInactive)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, TargetNotReachable)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, OutOfRange)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, InvalidTargetPoint)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, MovementTooFar)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, InvalidDamage)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, TargetDefeated)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, AttackerDefeated)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, FriendlyFireBlocked)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, SessionNotPlaying)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, SessionPaused)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, StepRequiresPaused)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, RetrySourceMissing)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, RetrySourceNotRejected)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, RetryUnsupportedKind)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, ResetUnavailable)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, SaveUnavailable)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, LoadUnavailable)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, IncompatibleSave)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, RequiredItemMissing)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, AbilitySlotBusy)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, AbilityOnCooldown)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, AbilityInsufficientResource)
  IGGY3D_ENUM_PARSE(CommandRejectionReason, InternalError)
  return false;
}

template <>
bool parseEnum(std::string_view value, ObjectiveStatus& out) {
  IGGY3D_ENUM_PARSE(ObjectiveStatus, Inactive)
  IGGY3D_ENUM_PARSE(ObjectiveStatus, Active)
  IGGY3D_ENUM_PARSE(ObjectiveStatus, Complete)
  IGGY3D_ENUM_PARSE(ObjectiveStatus, Failed)
  return false;
}

template <>
bool parseEnum(std::string_view value, ObjectiveConditionKind& out) {
  IGGY3D_ENUM_PARSE(ObjectiveConditionKind, None)
  IGGY3D_ENUM_PARSE(ObjectiveConditionKind, PlayerHasItem)
  return false;
}

#undef IGGY3D_ENUM_PARSE

class Writer {
public:
  explicit Writer(const SaveEnvelope& envelope) : envelope_(envelope) {}

  SaveEncodeResult finish() {
    out_ << kHeader << '\n';
    writeMetadata();
    writeSession();
    writeWorld();
    writeAuthoredRoom();
    writePlayers();
    writeClock();
    writeCamera();
    writeAbilities();
    writeCommandLog();
    writeInventory();
    writeCombat();
    writeAi();
    writeObjectives();
    SaveEncodeResult result;
    result.encodedText = out_.str();
    result.savedStateHash = envelope_.metadata.savedStateHash;
    return result;
  }

private:
  void line(const std::string& key, const std::string& value) {
    out_ << key << '=' << value << '\n';
  }
  void lineString(const std::string& key, const std::string& value) {
    line(key, escapeString(value));
  }
  void lineBool(const std::string& key, bool value) {
    line(key, value ? "true" : "false");
  }
  template <typename Enum>
  void lineEnum(const std::string& key, Enum value) {
    line(key, enumText(value));
  }

  void writeMetadata() {
    line("metadata.schemaVersion", unsignedText(envelope_.metadata.schemaVersion));
    line("metadata.minimumReadableSchemaVersion", unsignedText(envelope_.metadata.minimumReadableSchemaVersion));
    line("metadata.runtimeSaveVersion", unsignedText(envelope_.metadata.runtimeSaveVersion));
    lineString("metadata.packageId", envelope_.metadata.packageId);
    lineString("metadata.scenarioId", envelope_.metadata.scenarioId);
    lineString("metadata.createdByToolId", envelope_.metadata.createdByToolId);
    lineString("metadata.saveId", envelope_.metadata.saveId);
    lineString("metadata.worldId", envelope_.metadata.worldId);
    lineString("metadata.worldTitle", envelope_.metadata.worldTitle);
    lineString("metadata.saveTitle", envelope_.metadata.saveTitle);
    lineString("metadata.saveType", envelope_.metadata.saveType);
    lineString("metadata.createdAtUtc", envelope_.metadata.createdAtUtc);
    lineString("metadata.savedAtUtc", envelope_.metadata.savedAtUtc);
    line("metadata.savedStateHash", unsignedText(envelope_.metadata.savedStateHash));
    lineString("metadata.savedStateHashHex", envelope_.metadata.savedStateHashHex);
  }

  void writeSession() {
    lineEnum("session.lifecycle", envelope_.session.lifecycle);
    lineEnum("session.outcome", envelope_.session.outcome);
    line("session.currentTick", unsignedText(envelope_.session.currentTick));
    line("session.nextCommandId", unsignedText(envelope_.session.nextCommandId));
    line("session.sessionSeed", unsignedText(envelope_.session.sessionSeed));
    line("session.sessionSchemaVersion", unsignedText(envelope_.session.sessionSchemaVersion));
    line("session.fixedTickRateHz", unsignedText(envelope_.session.fixedTickRateHz));
    line("session.interactionRangeMeters", formatFloat(envelope_.session.interactionRangeMeters));
    line("session.movementDistanceMeters", formatFloat(envelope_.session.movementDistanceMeters));
    line("session.slowTimeScale", formatFloat(envelope_.session.slowTimeScale));
    lineString("session.packageId", envelope_.session.packageId);
    lineString("session.scenarioId", envelope_.session.scenarioId);
  }

  void writeWorld() {
    line("world.nextEntityId", unsignedText(toUint64(envelope_.world.nextEntityId)));
    line("world.entity.count", unsignedText(envelope_.world.entities.size()));
    for (std::size_t index = 0; index < envelope_.world.entities.size(); ++index) {
      const SaveEntityRecord& entity = envelope_.world.entities[index];
      const std::string p = "world.entity." + std::to_string(index) + ".";
      line(p + "id", unsignedText(toUint64(entity.id)));
      lineString(p + "stableName", entity.stableName);
      lineEnum(p + "kind", entity.kind);
      line(p + "transform.position", formatVec3(entity.transform.position));
      line(p + "transform.rotation", formatVec3(entity.transform.rotationEulerRadians));
      line(p + "transform.scale", formatVec3(entity.transform.scale));
      line(p + "localBounds.min", formatVec3(entity.localBounds.min));
      line(p + "localBounds.max", formatVec3(entity.localBounds.max));
      lineBool(p + "active", entity.active);
      lineBool(p + "persistent", entity.persistent);
      lineBool(p + "targetable", entity.targetable);
      line(p + "targetAction.count", unsignedText(entity.targetActions.size()));
      for (std::size_t action = 0; action < entity.targetActions.size(); ++action) {
        lineEnum(p + "targetAction." + std::to_string(action), entity.targetActions[action]);
      }
      lineEnum(p + "interactionKind", entity.interactionKind);
      lineEnum(p + "interactionPrimaryEffect", entity.interactionPrimaryEffect);
      lineString(p + "interactionItemId", entity.interactionItemId);
      line(p + "interactionItemCount", unsignedText(entity.interactionItemCount));
      lineString(p + "interactionObjectiveId", entity.interactionObjectiveId);
      lineString(p + "interactionRequiredItemId", entity.interactionRequiredItemId);
      line(p + "interactionRequiredItemCount", unsignedText(entity.interactionRequiredItemCount));
      lineBool(p + "interactionRepeatable", entity.interactionRepeatable);
      lineBool(p + "interactionDeactivateTargetOnSuccess", entity.interactionDeactivateTargetOnSuccess);
    }
  }

  void writeAuthoredRoomSemantics(const std::string& p,
                                  const SaveAuthoredRoomSemanticsRecord& semantics) {
    lineString(p + "materialId", semantics.materialId);
    line(p + "traversalTag.count", unsignedText(semantics.traversalTags.size()));
    for (std::size_t index = 0; index < semantics.traversalTags.size(); ++index) {
      lineString(p + "traversalTag." + std::to_string(index), semantics.traversalTags[index]);
    }
    line(p + "gameplayTag.count", unsignedText(semantics.gameplayTags.size()));
    for (std::size_t index = 0; index < semantics.gameplayTags.size(); ++index) {
      lineString(p + "gameplayTag." + std::to_string(index), semantics.gameplayTags[index]);
    }
    lineBool(p + "walkable", semantics.walkable);
    lineBool(p + "blocksActor", semantics.blocksActor);
    lineBool(p + "blocksProjectile", semantics.blocksProjectile);
  }

  void writeAuthoredRoom() {
    if (!envelope_.authoredRoom.present) {
      return;
    }
    lineBool("authoredRoom.present", true);
    lineString("authoredRoom.id", envelope_.authoredRoom.id);
    line("authoredRoom.version", unsignedText(envelope_.authoredRoom.version));
    lineString("authoredRoom.source", envelope_.authoredRoom.source);
    lineString("authoredRoom.sourceFile", envelope_.authoredRoom.sourceFile);
    lineString("authoredRoom.sourceSubset", envelope_.authoredRoom.sourceSubset);
    line("authoredRoom.floor.count", unsignedText(envelope_.authoredRoom.floors.size()));
    for (std::size_t index = 0; index < envelope_.authoredRoom.floors.size(); ++index) {
      const SaveAuthoredRoomFloorRecord& floor = envelope_.authoredRoom.floors[index];
      const std::string p = "authoredRoom.floor." + std::to_string(index) + ".";
      lineString(p + "id", floor.id);
      line(p + "storyIndex", std::to_string(floor.storyIndex));
      line(p + "centerMeters", formatVec3(floor.centerMeters));
      line(p + "sizeMeters", formatVec3(floor.sizeMeters));
      writeAuthoredRoomSemantics(p + "semantics.", floor.semantics);
      lineBool(p + "locked", floor.locked);
      lineBool(p + "hidden", floor.hidden);
    }
    line("authoredRoom.wall.count", unsignedText(envelope_.authoredRoom.walls.size()));
    for (std::size_t index = 0; index < envelope_.authoredRoom.walls.size(); ++index) {
      const SaveAuthoredRoomWallRecord& wall = envelope_.authoredRoom.walls[index];
      const std::string p = "authoredRoom.wall." + std::to_string(index) + ".";
      lineString(p + "id", wall.id);
      line(p + "storyIndex", std::to_string(wall.storyIndex));
      line(p + "startMeters", formatVec3(wall.startMeters));
      line(p + "endMeters", formatVec3(wall.endMeters));
      line(p + "bottomY", formatFloat(wall.bottomY));
      line(p + "heightMeters", formatFloat(wall.heightMeters));
      line(p + "thicknessMeters", formatFloat(wall.thicknessMeters));
      writeAuthoredRoomSemantics(p + "semantics.", wall.semantics);
      lineBool(p + "locked", wall.locked);
      lineBool(p + "hidden", wall.hidden);
    }
    line("authoredRoom.marker.count", unsignedText(envelope_.authoredRoom.markers.size()));
    for (std::size_t index = 0; index < envelope_.authoredRoom.markers.size(); ++index) {
      const SaveAuthoredRoomMarkerRecord& marker = envelope_.authoredRoom.markers[index];
      const std::string p = "authoredRoom.marker." + std::to_string(index) + ".";
      lineString(p + "id", marker.id);
      lineString(p + "tag", marker.tag);
      lineString(p + "glyph", marker.glyph);
      line(p + "row", unsignedText(marker.row));
      line(p + "column", unsignedText(marker.column));
      line(p + "positionMeters", formatVec3(marker.positionMeters));
      line(p + "sourceLine", unsignedText(marker.sourceLine));
      line(p + "sourceColumn", unsignedText(marker.sourceColumn));
    }
  }

  void writePlayers() {
    line("players.slot.count", unsignedText(envelope_.players.slots.size()));
    for (std::size_t index = 0; index < envelope_.players.slots.size(); ++index) {
      const SavePlayerSlotRecord& slot = envelope_.players.slots[index];
      const std::string p = "players.slot." + std::to_string(index) + ".";
      line(p + "slotId", unsignedText(slot.slotId));
      lineEnum(p + "kind", slot.kind);
      line(p + "controlledActor", unsignedText(toUint64(slot.controlledActor)));
      lineString(p + "stableName", slot.stableName);
    }
  }

  void writeClock() {
    lineEnum("clock.mode", envelope_.clock.mode);
    lineEnum("clock.previousUnpausedMode", envelope_.clock.previousUnpausedMode);
    line("clock.previousUnpausedTimeScale", formatFloat(envelope_.clock.previousUnpausedTimeScale));
    line("clock.tickIndex", unsignedText(envelope_.clock.tickIndex));
    line("clock.fixedTickRateHz", unsignedText(envelope_.clock.fixedTickRateHz));
    line("clock.timeScale", formatFloat(envelope_.clock.timeScale));
  }

  void writeCamera() {
    lineEnum("camera.activeMode", envelope_.camera.activeMode);
    lineEnum("camera.previousRealtimeMode", envelope_.camera.previousRealtimeMode);
    line("camera.targetEntity", unsignedText(toUint64(envelope_.camera.targetEntity)));
    line("camera.targetPoint", formatVec3(envelope_.camera.targetPoint));
    lineBool("camera.targetHasPoint", envelope_.camera.targetHasPoint);
    line("camera.yawDegrees", formatFloat(envelope_.camera.yawDegrees));
    line("camera.pitchDegrees", formatFloat(envelope_.camera.pitchDegrees));
    line("camera.orbitDistance", formatFloat(envelope_.camera.orbitDistance));
  }

  void writeAbilities() {
    line("abilities.actor.count", unsignedText(envelope_.abilities.actors.size()));
    for (std::size_t index = 0; index < envelope_.abilities.actors.size(); ++index) {
      const SaveAbilityActorRecord& actor = envelope_.abilities.actors[index];
      const std::string p = "abilities.actor." + std::to_string(index) + ".";
      line(p + "actor", unsignedText(toUint64(actor.actor)));
      line(p + "arcaneFocus", unsignedText(actor.arcaneFocus));
      line(p + "arcaneBoltReadyTick", unsignedText(actor.arcaneBoltReadyTick));
      line(p + "arcaneFocusNextRechargeTick",
           unsignedText(actor.arcaneFocusNextRechargeTick));
    }
  }

  void writeCommandLog() {
    lineEnum("commandLog.resetPolicy", envelope_.commandLog.resetPolicy);
    line("commandLog.nextSequence", unsignedText(envelope_.commandLog.nextSequence));
    line("commandLog.epoch", unsignedText(envelope_.commandLog.epoch));
    line("commandLog.record.count", unsignedText(envelope_.commandLog.records.size()));
    for (std::size_t index = 0; index < envelope_.commandLog.records.size(); ++index) {
      const SaveCommandRecord& record = envelope_.commandLog.records[index];
      const std::string p = "commandLog.record." + std::to_string(index) + ".";
      line(p + "commandId", unsignedText(record.commandId));
      line(p + "sequence", unsignedText(record.sequence));
      lineEnum(p + "kind", record.kind);
      lineEnum(p + "source", record.source);
      line(p + "playerSlot", unsignedText(record.playerSlot));
      line(p + "actor", unsignedText(toUint64(record.actor)));
      lineBool(p + "hasTargetEntity", record.hasTargetEntity);
      line(p + "targetEntity", unsignedText(toUint64(record.targetEntity)));
      lineBool(p + "hasTargetPoint", record.hasTargetPoint);
      line(p + "targetPoint", formatVec3(record.targetPoint));
      line(p + "retrySourceCommandId", unsignedText(record.retrySourceCommandId));
      line(p + "attackDamage", std::to_string(record.attackDamage));
      lineEnum(p + "ability", record.ability);
      line(p + "abilityDirection", formatVec3(record.abilityDirection));
      line(p + "issuedTick", unsignedText(record.issuedTick));
      line(p + "scheduledTick", unsignedText(record.scheduledTick));
      lineEnum(p + "admission", record.admission);
      lineEnum(p + "rejection", record.rejection);
    }
  }

  void writeInventory() {
    line("inventory.player.count", unsignedText(envelope_.inventory.players.size()));
    for (std::size_t index = 0; index < envelope_.inventory.players.size(); ++index) {
      const SavePlayerInventoryRecord& player = envelope_.inventory.players[index];
      const std::string p = "inventory.player." + std::to_string(index) + ".";
      line(p + "playerSlot", unsignedText(player.playerSlot));
      line(p + "stack.count", unsignedText(player.stacks.size()));
      for (std::size_t stack = 0; stack < player.stacks.size(); ++stack) {
        const std::string sp = p + "stack." + std::to_string(stack) + ".";
        lineString(sp + "itemId", player.stacks[stack].itemId);
        line(sp + "count", unsignedText(player.stacks[stack].count));
      }
    }
  }

  void writeCombat() {
    line("combat.combatant.count", unsignedText(envelope_.combat.combatants.size()));
    for (std::size_t index = 0; index < envelope_.combat.combatants.size(); ++index) {
      const SaveCombatantRecord& combatant = envelope_.combat.combatants[index];
      const std::string p = "combat.combatant." + std::to_string(index) + ".";
      line(p + "entity", unsignedText(toUint64(combatant.entity)));
      line(p + "factionId", unsignedText(combatant.factionId));
      line(p + "hitPoints", std::to_string(combatant.hitPoints));
      line(p + "maxHitPoints", std::to_string(combatant.maxHitPoints));
      lineBool(p + "defeated", combatant.defeated);
    }
  }

  void writeAi() {
    line("ai.actor.count", unsignedText(envelope_.ai.actors.size()));
    for (std::size_t index = 0; index < envelope_.ai.actors.size(); ++index) {
      const SaveAiActorRecord& actor = envelope_.ai.actors[index];
      const std::string p = "ai.actor." + std::to_string(index) + ".";
      line(p + "actor", unsignedText(toUint64(actor.actor)));
      line(p + "nextDecisionTick", unsignedText(actor.nextDecisionTick));
      line(p + "deterministicPolicy", unsignedText(actor.deterministicPolicy));
      lineBool(p + "enabled", actor.enabled);
      lineString(p + "behavior_profile_id", actor.behaviorProfileId);
      line(p + "target", unsignedText(toUint64(actor.target)));
      lineEnum(p + "behavior", actor.behavior);
      lineEnum(p + "lastIntent", actor.lastIntent);
      line(p + "cooldownTicksRemaining", unsignedText(actor.cooldownTicksRemaining));
      lineBool(p + "hasHomePosition", actor.hasHomePosition);
      line(p + "homePosition", formatVec3(actor.homePosition));
      lineString(p + "homeStableName", actor.homeStableName);
      line(p + "leashRadiusMeters", formatFloat(actor.leashRadiusMeters));
      line(p + "returnRadiusMeters", formatFloat(actor.returnRadiusMeters));
      line(p + "homeToleranceMeters", formatFloat(actor.homeToleranceMeters));
    }
  }

  void writeObjectives() {
    line("objectives.record.count", unsignedText(envelope_.objectives.objectives.size()));
    for (std::size_t index = 0; index < envelope_.objectives.objectives.size(); ++index) {
      const SaveObjectiveRecord& objective = envelope_.objectives.objectives[index];
      const std::string p = "objectives.record." + std::to_string(index) + ".";
      lineString(p + "objectiveId", objective.objectiveId);
      lineEnum(p + "status", objective.status);
      lineEnum(p + "conditionKind", objective.conditionKind);
      line(p + "conditionPlayerSlot", unsignedText(objective.conditionPlayerSlot));
      lineString(p + "conditionItemId", objective.conditionItemId);
      line(p + "conditionItemCount", unsignedText(objective.conditionItemCount));
    }
  }

  const SaveEnvelope& envelope_;
  std::ostringstream out_;
};

class Reader {
public:
  explicit Reader(std::string_view bytes) {
    std::string current;
    for (char c : bytes) {
      if (c == '\n') {
        lines_.push_back(current);
        current.clear();
      } else {
        current.push_back(c);
      }
    }
    if (!current.empty()) {
      lines_.push_back(current);
    }
  }

  SaveDecodeResult decode() {
    if (lines_.empty() || lines_[0] != kHeader) {
      return fail(SaveCodecStatus::UnsupportedVersion, "header", 1, "invalid header");
    }
    index_ = 1;
    readMetadata();
    readSession();
    readWorld();
    readAuthoredRoom();
    readPlayers();
    readClock();
    readCamera();
    readAbilities();
    readCommandLog();
    readInventory();
    readCombat();
    readAi();
    readObjectives();
    if (result_.status != SaveCodecStatus::Ok) {
      return result_;
    }
    if (index_ != lines_.size()) {
      const std::string key = keyAt(index_);
      return fail(SaveCodecStatus::UnknownKey, key, index_ + 1U, "unknown key");
    }
    result_.envelope = envelope_;
    return result_;
  }

private:
  SaveDecodeResult fail(SaveCodecStatus status,
                        const std::string& key,
                        std::size_t line,
                        std::string diagnostic) {
    if (result_.status == SaveCodecStatus::Ok) {
      result_.status = status;
      result_.diagnosticSection = sectionForKey(key);
      result_.diagnosticKey = key;
      result_.diagnosticLine = static_cast<std::uint32_t>(line);
      result_.diagnostic = std::move(diagnostic);
    }
    return result_;
  }

  std::string keyAt(std::size_t lineIndex) const {
    if (lineIndex >= lines_.size()) {
      return {};
    }
    const std::size_t equals = lines_[lineIndex].find('=');
    return equals == std::string::npos ? lines_[lineIndex] : lines_[lineIndex].substr(0, equals);
  }

  bool nextKeyIs(const std::string& expectedKey) const {
    return index_ < lines_.size() && keyAt(index_) == expectedKey;
  }

  bool nextValue(const std::string& expectedKey, std::string_view& value) {
    if (result_.status != SaveCodecStatus::Ok) {
      return false;
    }
    if (index_ >= lines_.size()) {
      result_ = fail(SaveCodecStatus::MissingField, expectedKey, index_ + 1U, "missing field");
      return false;
    }
    const std::string& line = lines_[index_];
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) {
      result_ = fail(SaveCodecStatus::DecodeFailed, expectedKey, index_ + 1U, "expected field");
      return false;
    }
    const std::string key = line.substr(0, equals);
    if (seen_.contains(key)) {
      result_ = fail(SaveCodecStatus::DuplicateKey, key, index_ + 1U, "duplicate key");
      return false;
    }
    if (key != expectedKey) {
      result_ = fail(sectionForKey(key) == sectionForKey(expectedKey)
                         ? SaveCodecStatus::UnknownKey
                         : SaveCodecStatus::InvalidSectionOrder,
                     key, index_ + 1U, "unexpected key");
      return false;
    }
    seen_.insert(key);
    value = std::string_view(line).substr(equals + 1U);
    ++index_;
    return true;
  }

  bool readString(const std::string& key, std::string& out) {
    std::string_view value;
    if (!nextValue(key, value)) {
      return false;
    }
    if (!unescapeString(value, out)) {
      result_ = fail(SaveCodecStatus::DecodeFailed, key, index_, "invalid escape");
      return false;
    }
    return true;
  }

  bool readOptionalString(const std::string& key, std::string& out) {
    if (!nextKeyIs(key)) {
      return true;
    }
    return readString(key, out);
  }

  template <typename T>
  bool readUnsigned(const std::string& key, T& out, SaveCodecStatus status = SaveCodecStatus::InvalidNumber) {
    std::string_view value;
    if (!nextValue(key, value)) {
      return false;
    }
    if (!parseUnsigned(value, out)) {
      result_ = fail(status, key, index_, "invalid number");
      return false;
    }
    return true;
  }

  bool readI32(const std::string& key, std::int32_t& out) {
    std::string_view value;
    if (!nextValue(key, value)) {
      return false;
    }
    if (!parseI32(value, out)) {
      result_ = fail(SaveCodecStatus::InvalidNumber, key, index_, "invalid integer");
      return false;
    }
    return true;
  }

  bool readOptionalI32(const std::string& key, std::int32_t& out) {
    if (!nextKeyIs(key)) {
      return true;
    }
    return readI32(key, out);
  }

  bool readFloat(const std::string& key, float& out) {
    std::string_view value;
    if (!nextValue(key, value)) {
      return false;
    }
    if (!parseFloat(value, out)) {
      result_ = fail(SaveCodecStatus::InvalidNumber, key, index_, "invalid float");
      return false;
    }
    return true;
  }

  bool readBool(const std::string& key, bool& out) {
    std::string_view value;
    if (!nextValue(key, value)) {
      return false;
    }
    if (!parseBool(value, out)) {
      result_ = fail(SaveCodecStatus::InvalidNumber, key, index_, "invalid bool");
      return false;
    }
    return true;
  }

  bool readVec3(const std::string& key, Vec3& out) {
    std::string_view value;
    if (!nextValue(key, value)) {
      return false;
    }
    if (!parseVec3(value, out)) {
      result_ = fail(SaveCodecStatus::InvalidNumber, key, index_, "invalid vector");
      return false;
    }
    return true;
  }

  template <typename Enum>
  bool readEnum(const std::string& key, Enum& out) {
    std::string_view value;
    if (!nextValue(key, value)) {
      return false;
    }
    if (!parseEnum(value, out)) {
      result_ = fail(SaveCodecStatus::InvalidEnum, key, index_, "invalid enum");
      return false;
    }
    return true;
  }

  bool readEntityId(const std::string& key, EntityId& out) {
    std::uint64_t value = 0;
    if (!readUnsigned(key, value, SaveCodecStatus::InvalidId)) {
      return false;
    }
    out = EntityId{value};
    return true;
  }

  void readMetadata() {
    readUnsigned("metadata.schemaVersion", envelope_.metadata.schemaVersion);
    readUnsigned("metadata.minimumReadableSchemaVersion", envelope_.metadata.minimumReadableSchemaVersion);
    readUnsigned("metadata.runtimeSaveVersion", envelope_.metadata.runtimeSaveVersion);
    readString("metadata.packageId", envelope_.metadata.packageId);
    readString("metadata.scenarioId", envelope_.metadata.scenarioId);
    readString("metadata.createdByToolId", envelope_.metadata.createdByToolId);
    readOptionalString("metadata.saveId", envelope_.metadata.saveId);
    readOptionalString("metadata.worldId", envelope_.metadata.worldId);
    readOptionalString("metadata.worldTitle", envelope_.metadata.worldTitle);
    readOptionalString("metadata.saveTitle", envelope_.metadata.saveTitle);
    readOptionalString("metadata.saveType", envelope_.metadata.saveType);
    readOptionalString("metadata.createdAtUtc", envelope_.metadata.createdAtUtc);
    readOptionalString("metadata.savedAtUtc", envelope_.metadata.savedAtUtc);
    readUnsigned("metadata.savedStateHash", envelope_.metadata.savedStateHash);
    readString("metadata.savedStateHashHex", envelope_.metadata.savedStateHashHex);
  }

  void readSession() {
    readEnum("session.lifecycle", envelope_.session.lifecycle);
    readEnum("session.outcome", envelope_.session.outcome);
    readUnsigned("session.currentTick", envelope_.session.currentTick);
    readUnsigned("session.nextCommandId", envelope_.session.nextCommandId, SaveCodecStatus::InvalidId);
    readUnsigned("session.sessionSeed", envelope_.session.sessionSeed);
    readUnsigned("session.sessionSchemaVersion", envelope_.session.sessionSchemaVersion);
    readUnsigned("session.fixedTickRateHz", envelope_.session.fixedTickRateHz);
    readFloat("session.interactionRangeMeters", envelope_.session.interactionRangeMeters);
    readFloat("session.movementDistanceMeters", envelope_.session.movementDistanceMeters);
    readFloat("session.slowTimeScale", envelope_.session.slowTimeScale);
    readString("session.packageId", envelope_.session.packageId);
    readString("session.scenarioId", envelope_.session.scenarioId);
  }

  void readWorld() {
    readEntityId("world.nextEntityId", envelope_.world.nextEntityId);
    std::uint64_t count = 0;
    readUnsigned("world.entity.count", count);
    envelope_.world.entities.resize(static_cast<std::size_t>(count));
    for (std::size_t index = 0; index < envelope_.world.entities.size(); ++index) {
      SaveEntityRecord& entity = envelope_.world.entities[index];
      const std::string p = "world.entity." + std::to_string(index) + ".";
      readEntityId(p + "id", entity.id);
      readString(p + "stableName", entity.stableName);
      readEnum(p + "kind", entity.kind);
      readVec3(p + "transform.position", entity.transform.position);
      readVec3(p + "transform.rotation", entity.transform.rotationEulerRadians);
      readVec3(p + "transform.scale", entity.transform.scale);
      readVec3(p + "localBounds.min", entity.localBounds.min);
      readVec3(p + "localBounds.max", entity.localBounds.max);
      readBool(p + "active", entity.active);
      readBool(p + "persistent", entity.persistent);
      readBool(p + "targetable", entity.targetable);
      std::uint64_t actionCount = 0;
      readUnsigned(p + "targetAction.count", actionCount);
      entity.targetActions.resize(static_cast<std::size_t>(actionCount));
      for (std::size_t action = 0; action < entity.targetActions.size(); ++action) {
        readEnum(p + "targetAction." + std::to_string(action), entity.targetActions[action]);
      }
      readEnum(p + "interactionKind", entity.interactionKind);
      readEnum(p + "interactionPrimaryEffect", entity.interactionPrimaryEffect);
      readString(p + "interactionItemId", entity.interactionItemId);
      readUnsigned(p + "interactionItemCount", entity.interactionItemCount);
      readString(p + "interactionObjectiveId", entity.interactionObjectiveId);
      if (nextKeyIs(p + "interactionRequiredItemId")) {
        readString(p + "interactionRequiredItemId", entity.interactionRequiredItemId);
        readUnsigned(p + "interactionRequiredItemCount", entity.interactionRequiredItemCount);
      }
      readBool(p + "interactionRepeatable", entity.interactionRepeatable);
      readBool(p + "interactionDeactivateTargetOnSuccess", entity.interactionDeactivateTargetOnSuccess);
    }
  }

  void readStringVector(const std::string& countKey,
                        const std::string& itemPrefix,
                        std::vector<std::string>& out) {
    std::uint64_t count = 0;
    readUnsigned(countKey, count);
    out.resize(static_cast<std::size_t>(count));
    for (std::size_t index = 0; index < out.size(); ++index) {
      readString(itemPrefix + std::to_string(index), out[index]);
    }
  }

  void readAuthoredRoomSemantics(const std::string& p,
                                 SaveAuthoredRoomSemanticsRecord& semantics) {
    readString(p + "materialId", semantics.materialId);
    readStringVector(p + "traversalTag.count", p + "traversalTag.",
                     semantics.traversalTags);
    readStringVector(p + "gameplayTag.count", p + "gameplayTag.", semantics.gameplayTags);
    readBool(p + "walkable", semantics.walkable);
    readBool(p + "blocksActor", semantics.blocksActor);
    readBool(p + "blocksProjectile", semantics.blocksProjectile);
  }

  void readAuthoredRoom() {
    if (!nextKeyIs("authoredRoom.present")) {
      return;
    }
    readBool("authoredRoom.present", envelope_.authoredRoom.present);
    if (!envelope_.authoredRoom.present) {
      return;
    }
    readString("authoredRoom.id", envelope_.authoredRoom.id);
    readUnsigned("authoredRoom.version", envelope_.authoredRoom.version);
    readString("authoredRoom.source", envelope_.authoredRoom.source);
    readString("authoredRoom.sourceFile", envelope_.authoredRoom.sourceFile);
    readString("authoredRoom.sourceSubset", envelope_.authoredRoom.sourceSubset);
    std::uint64_t floorCount = 0;
    readUnsigned("authoredRoom.floor.count", floorCount);
    envelope_.authoredRoom.floors.resize(static_cast<std::size_t>(floorCount));
    for (std::size_t index = 0; index < envelope_.authoredRoom.floors.size(); ++index) {
      SaveAuthoredRoomFloorRecord& floor = envelope_.authoredRoom.floors[index];
      const std::string p = "authoredRoom.floor." + std::to_string(index) + ".";
      readString(p + "id", floor.id);
      readI32(p + "storyIndex", floor.storyIndex);
      readVec3(p + "centerMeters", floor.centerMeters);
      readVec3(p + "sizeMeters", floor.sizeMeters);
      readAuthoredRoomSemantics(p + "semantics.", floor.semantics);
      readBool(p + "locked", floor.locked);
      readBool(p + "hidden", floor.hidden);
    }
    std::uint64_t wallCount = 0;
    readUnsigned("authoredRoom.wall.count", wallCount);
    envelope_.authoredRoom.walls.resize(static_cast<std::size_t>(wallCount));
    for (std::size_t index = 0; index < envelope_.authoredRoom.walls.size(); ++index) {
      SaveAuthoredRoomWallRecord& wall = envelope_.authoredRoom.walls[index];
      const std::string p = "authoredRoom.wall." + std::to_string(index) + ".";
      readString(p + "id", wall.id);
      readI32(p + "storyIndex", wall.storyIndex);
      readVec3(p + "startMeters", wall.startMeters);
      readVec3(p + "endMeters", wall.endMeters);
      readFloat(p + "bottomY", wall.bottomY);
      readFloat(p + "heightMeters", wall.heightMeters);
      readFloat(p + "thicknessMeters", wall.thicknessMeters);
      readAuthoredRoomSemantics(p + "semantics.", wall.semantics);
      readBool(p + "locked", wall.locked);
      readBool(p + "hidden", wall.hidden);
    }
    if (!nextKeyIs("authoredRoom.marker.count")) {
      return;
    }
    std::uint64_t markerCount = 0;
    readUnsigned("authoredRoom.marker.count", markerCount);
    envelope_.authoredRoom.markers.resize(static_cast<std::size_t>(markerCount));
    for (std::size_t index = 0; index < envelope_.authoredRoom.markers.size(); ++index) {
      SaveAuthoredRoomMarkerRecord& marker = envelope_.authoredRoom.markers[index];
      const std::string p = "authoredRoom.marker." + std::to_string(index) + ".";
      readString(p + "id", marker.id);
      readString(p + "tag", marker.tag);
      readString(p + "glyph", marker.glyph);
      readUnsigned(p + "row", marker.row);
      readUnsigned(p + "column", marker.column);
      readVec3(p + "positionMeters", marker.positionMeters);
      readUnsigned(p + "sourceLine", marker.sourceLine);
      readUnsigned(p + "sourceColumn", marker.sourceColumn);
    }
  }

  void readPlayers() {
    std::uint64_t count = 0;
    readUnsigned("players.slot.count", count);
    envelope_.players.slots.resize(static_cast<std::size_t>(count));
    for (std::size_t index = 0; index < envelope_.players.slots.size(); ++index) {
      SavePlayerSlotRecord& slot = envelope_.players.slots[index];
      const std::string p = "players.slot." + std::to_string(index) + ".";
      readUnsigned(p + "slotId", slot.slotId, SaveCodecStatus::InvalidId);
      readEnum(p + "kind", slot.kind);
      readEntityId(p + "controlledActor", slot.controlledActor);
      readString(p + "stableName", slot.stableName);
    }
  }

  void readClock() {
    readEnum("clock.mode", envelope_.clock.mode);
    readEnum("clock.previousUnpausedMode", envelope_.clock.previousUnpausedMode);
    readFloat("clock.previousUnpausedTimeScale", envelope_.clock.previousUnpausedTimeScale);
    readUnsigned("clock.tickIndex", envelope_.clock.tickIndex);
    readUnsigned("clock.fixedTickRateHz", envelope_.clock.fixedTickRateHz);
    readFloat("clock.timeScale", envelope_.clock.timeScale);
  }

  void readCamera() {
    readEnum("camera.activeMode", envelope_.camera.activeMode);
    readEnum("camera.previousRealtimeMode", envelope_.camera.previousRealtimeMode);
    readEntityId("camera.targetEntity", envelope_.camera.targetEntity);
    readVec3("camera.targetPoint", envelope_.camera.targetPoint);
    readBool("camera.targetHasPoint", envelope_.camera.targetHasPoint);
    readFloat("camera.yawDegrees", envelope_.camera.yawDegrees);
    readFloat("camera.pitchDegrees", envelope_.camera.pitchDegrees);
    readFloat("camera.orbitDistance", envelope_.camera.orbitDistance);
  }

  void readAbilities() {
    if (!nextKeyIs("abilities.actor.count")) {
      return;
    }
    std::uint64_t count = 0;
    readUnsigned("abilities.actor.count", count);
    envelope_.abilities.actors.resize(static_cast<std::size_t>(count));
    for (std::size_t index = 0; index < envelope_.abilities.actors.size(); ++index) {
      SaveAbilityActorRecord& actor = envelope_.abilities.actors[index];
      const std::string p = "abilities.actor." + std::to_string(index) + ".";
      readEntityId(p + "actor", actor.actor);
      readUnsigned(p + "arcaneFocus", actor.arcaneFocus);
      readUnsigned(p + "arcaneBoltReadyTick", actor.arcaneBoltReadyTick);
      if (nextKeyIs(p + "arcaneFocusNextRechargeTick")) {
        readUnsigned(p + "arcaneFocusNextRechargeTick",
                     actor.arcaneFocusNextRechargeTick);
      }
    }
  }

  void readCommandLog() {
    readEnum("commandLog.resetPolicy", envelope_.commandLog.resetPolicy);
    if (envelope_.commandLog.resetPolicy != CommandLogResetPolicy::Clear) {
      result_ = fail(SaveCodecStatus::InvalidEnum, "commandLog.resetPolicy", index_, "unsupported reset policy");
      return;
    }
    readUnsigned("commandLog.nextSequence", envelope_.commandLog.nextSequence, SaveCodecStatus::InvalidSequence);
    readUnsigned("commandLog.epoch", envelope_.commandLog.epoch);
    std::uint64_t count = 0;
    readUnsigned("commandLog.record.count", count);
    envelope_.commandLog.records.resize(static_cast<std::size_t>(count));
    for (std::size_t index = 0; index < envelope_.commandLog.records.size(); ++index) {
      SaveCommandRecord& record = envelope_.commandLog.records[index];
      const std::string p = "commandLog.record." + std::to_string(index) + ".";
      readUnsigned(p + "commandId", record.commandId, SaveCodecStatus::InvalidId);
      readUnsigned(p + "sequence", record.sequence, SaveCodecStatus::InvalidSequence);
      readEnum(p + "kind", record.kind);
      readEnum(p + "source", record.source);
      readUnsigned(p + "playerSlot", record.playerSlot, SaveCodecStatus::InvalidId);
      readEntityId(p + "actor", record.actor);
      readBool(p + "hasTargetEntity", record.hasTargetEntity);
      readEntityId(p + "targetEntity", record.targetEntity);
      readBool(p + "hasTargetPoint", record.hasTargetPoint);
      readVec3(p + "targetPoint", record.targetPoint);
      readUnsigned(p + "retrySourceCommandId", record.retrySourceCommandId, SaveCodecStatus::InvalidId);
      readOptionalI32(p + "attackDamage", record.attackDamage);
      if (nextKeyIs(p + "ability")) {
        readEnum(p + "ability", record.ability);
        readVec3(p + "abilityDirection", record.abilityDirection);
      }
      readUnsigned(p + "issuedTick", record.issuedTick);
      readUnsigned(p + "scheduledTick", record.scheduledTick);
      readEnum(p + "admission", record.admission);
      readEnum(p + "rejection", record.rejection);
    }
  }

  void readInventory() {
    std::uint64_t count = 0;
    readUnsigned("inventory.player.count", count);
    envelope_.inventory.players.resize(static_cast<std::size_t>(count));
    for (std::size_t index = 0; index < envelope_.inventory.players.size(); ++index) {
      SavePlayerInventoryRecord& player = envelope_.inventory.players[index];
      const std::string p = "inventory.player." + std::to_string(index) + ".";
      readUnsigned(p + "playerSlot", player.playerSlot, SaveCodecStatus::InvalidId);
      std::uint64_t stackCount = 0;
      readUnsigned(p + "stack.count", stackCount);
      player.stacks.resize(static_cast<std::size_t>(stackCount));
      for (std::size_t stack = 0; stack < player.stacks.size(); ++stack) {
        const std::string sp = p + "stack." + std::to_string(stack) + ".";
        readString(sp + "itemId", player.stacks[stack].itemId);
        readUnsigned(sp + "count", player.stacks[stack].count);
      }
    }
  }

  void readCombat() {
    std::uint64_t count = 0;
    readUnsigned("combat.combatant.count", count);
    envelope_.combat.combatants.resize(static_cast<std::size_t>(count));
    for (std::size_t index = 0; index < envelope_.combat.combatants.size(); ++index) {
      SaveCombatantRecord& combatant = envelope_.combat.combatants[index];
      const std::string p = "combat.combatant." + std::to_string(index) + ".";
      readEntityId(p + "entity", combatant.entity);
      readUnsigned(p + "factionId", combatant.factionId);
      readI32(p + "hitPoints", combatant.hitPoints);
      readI32(p + "maxHitPoints", combatant.maxHitPoints);
      readBool(p + "defeated", combatant.defeated);
    }
  }

  void readAi() {
    std::uint64_t count = 0;
    readUnsigned("ai.actor.count", count);
    envelope_.ai.actors.resize(static_cast<std::size_t>(count));
    for (std::size_t index = 0; index < envelope_.ai.actors.size(); ++index) {
      SaveAiActorRecord& actor = envelope_.ai.actors[index];
      const std::string p = "ai.actor." + std::to_string(index) + ".";
      readEntityId(p + "actor", actor.actor);
      readUnsigned(p + "nextDecisionTick", actor.nextDecisionTick);
      readUnsigned(p + "deterministicPolicy", actor.deterministicPolicy);
      readBool(p + "enabled", actor.enabled);
      if (nextKeyIs(p + "behavior_profile_id")) {
        readString(p + "behavior_profile_id", actor.behaviorProfileId);
      }
      if (actor.behaviorProfileId.empty()) {
        actor.behaviorProfileId = "default";
      }
      if (nextKeyIs(p + "target")) {
        readEntityId(p + "target", actor.target);
      }
      if (nextKeyIs(p + "behavior")) {
        readEnum(p + "behavior", actor.behavior);
      }
      if (nextKeyIs(p + "lastIntent")) {
        readEnum(p + "lastIntent", actor.lastIntent);
      }
      if (nextKeyIs(p + "cooldownTicksRemaining")) {
        readUnsigned(p + "cooldownTicksRemaining", actor.cooldownTicksRemaining);
      }
      if (nextKeyIs(p + "hasHomePosition")) {
        readBool(p + "hasHomePosition", actor.hasHomePosition);
      }
      if (nextKeyIs(p + "homePosition")) {
        readVec3(p + "homePosition", actor.homePosition);
      }
      if (nextKeyIs(p + "homeStableName")) {
        readString(p + "homeStableName", actor.homeStableName);
      }
      if (nextKeyIs(p + "leashRadiusMeters")) {
        readFloat(p + "leashRadiusMeters", actor.leashRadiusMeters);
      }
      if (nextKeyIs(p + "returnRadiusMeters")) {
        readFloat(p + "returnRadiusMeters", actor.returnRadiusMeters);
      }
      if (nextKeyIs(p + "homeToleranceMeters")) {
        readFloat(p + "homeToleranceMeters", actor.homeToleranceMeters);
      }
    }
  }

  void readObjectives() {
    std::uint64_t count = 0;
    readUnsigned("objectives.record.count", count);
    envelope_.objectives.objectives.resize(static_cast<std::size_t>(count));
    for (std::size_t index = 0; index < envelope_.objectives.objectives.size(); ++index) {
      SaveObjectiveRecord& objective = envelope_.objectives.objectives[index];
      const std::string p = "objectives.record." + std::to_string(index) + ".";
      readString(p + "objectiveId", objective.objectiveId);
      readEnum(p + "status", objective.status);
      readEnum(p + "conditionKind", objective.conditionKind);
      readUnsigned(p + "conditionPlayerSlot", objective.conditionPlayerSlot, SaveCodecStatus::InvalidId);
      readString(p + "conditionItemId", objective.conditionItemId);
      readUnsigned(p + "conditionItemCount", objective.conditionItemCount);
    }
  }

  SaveEnvelope envelope_;
  SaveDecodeResult result_;
  std::vector<std::string> lines_;
  std::set<std::string> seen_;
  std::size_t index_ = 0;
};

}  // namespace

SaveEncodeResult encodeSaveEnvelope(const SaveEnvelope& envelope) {
  return Writer(envelope).finish();
}

SaveDecodeResult decodeSaveEnvelope(std::string_view bytes) {
  if (bytes.size() > kMaxSaveBytes) {
    SaveDecodeResult result;
    result.status = SaveCodecStatus::SaveTooLarge;
    result.diagnostic = "save too large";
    return result;
  }
  return Reader(bytes).decode();
}

}  // namespace iggy3d

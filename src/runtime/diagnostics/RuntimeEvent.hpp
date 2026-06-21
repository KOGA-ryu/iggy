#pragma once

#include <cstdint>
#include <string>

#include "core/ids/EntityId.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/player/PlayerSlot.hpp"

namespace iggy3d {

enum class RuntimeEventKind : std::uint8_t {
  CommandAccepted,
  CommandRejected,
  Moved,
  Interacted,
  ItemAcquired,
  ObjectiveCompleted,
  ClockChanged,
  CameraChanged,
  SaveCreated,
  LoadCompleted,
  ResetCompleted,
  ReplayCompleted,
  RuntimeFailed,
};

struct RuntimeEvent {
  RuntimeEventKind kind = RuntimeEventKind::RuntimeFailed;
  CommandTick tick = kInvalidCommandTick;
  CommandId commandId = kInvalidCommandId;
  CommandSequence sequence = kInvalidCommandSequence;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  EntityId actor;
  EntityId target;
  CommandRejectionReason rejection = CommandRejectionReason::None;
  std::string stableId;
};

}  // namespace iggy3d

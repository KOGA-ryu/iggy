#pragma once

#include "config/RuntimeConfig.hpp"
#include "runtime/clock/ClockState.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/player/PlayerRoster.hpp"
#include "runtime/replay/CommandLog.hpp"
#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {

struct CommandAdmissionContext {
  const WorldState* world = nullptr;
  const PlayerRoster* players = nullptr;
  const ClockState* clock = nullptr;
  const CommandLog* commandLog = nullptr;
  const RuntimeConfig* config = nullptr;
  const CombatState* combat = nullptr;
};

struct CommandAdmissionRequest {
  CommandRecord command;
  bool allowSessionControlWhilePaused = true;
};

struct CommandAdmissionResult {
  CommandRecord command;
  CommandRejectionReason firstFailure = CommandRejectionReason::None;
};

CommandAdmissionResult admitCommand(
    const CommandAdmissionContext& context,
    const CommandAdmissionRequest& request);

CommandAdmissionResult rejectCommand(CommandRecord command, CommandRejectionReason reason);
CommandAdmissionResult acceptCommand(CommandRecord command);

}  // namespace iggy3d

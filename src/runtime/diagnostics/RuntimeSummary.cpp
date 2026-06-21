#include "runtime/diagnostics/RuntimeSummary.hpp"

#include <iomanip>
#include <sstream>

#include "runtime/replay/StateHash.hpp"

namespace iggy3d {

namespace {

std::string boolText(bool value) {
  return value ? "true" : "false";
}

std::string proofText(RuntimeProofStatus status) {
  switch (status) {
    case RuntimeProofStatus::Pass:
      return "pass";
    case RuntimeProofStatus::Fail:
      return "fail";
    case RuntimeProofStatus::Unknown:
      return "unknown";
  }
  return "unknown";
}

std::string lifecycleText(SessionLifecycle lifecycle) {
  switch (lifecycle) {
    case SessionLifecycle::Loading: return "Loading";
    case SessionLifecycle::Playing: return "Playing";
    case SessionLifecycle::Paused: return "Paused";
    case SessionLifecycle::Complete: return "Complete";
    case SessionLifecycle::Failed: return "Failed";
  }
  return "Loading";
}

std::string outcomeText(SessionOutcome outcome) {
  switch (outcome) {
    case SessionOutcome::None: return "None";
    case SessionOutcome::DemoComplete: return "DemoComplete";
    case SessionOutcome::Victory: return "Victory";
    case SessionOutcome::Defeat: return "Defeat";
    case SessionOutcome::Failed: return "Failed";
  }
  return "None";
}

std::string clockText(ClockMode mode) {
  switch (mode) {
    case ClockMode::Normal: return "Normal";
    case ClockMode::Slow: return "Slow";
    case ClockMode::Paused: return "Paused";
  }
  return "Normal";
}

std::string cameraText(CameraMode mode) {
  switch (mode) {
    case CameraMode::FirstPerson: return "FirstPerson";
    case CameraMode::ThirdPerson: return "ThirdPerson";
    case CameraMode::TacticalOverhead: return "TacticalOverhead";
  }
  return "ThirdPerson";
}

std::string rejectionText(CommandRejectionReason reason) {
  switch (reason) {
    case CommandRejectionReason::None: return "None";
    case CommandRejectionReason::OutOfRange: return "OutOfRange";
    case CommandRejectionReason::InvalidCommand: return "InvalidCommand";
    case CommandRejectionReason::InvalidPlayerSlot: return "InvalidPlayerSlot";
    case CommandRejectionReason::InvalidActor: return "InvalidActor";
    case CommandRejectionReason::ActorNotControlledBySlot: return "ActorNotControlledBySlot";
    case CommandRejectionReason::InvalidTarget: return "InvalidTarget";
    case CommandRejectionReason::TargetInactive: return "TargetInactive";
    case CommandRejectionReason::InvalidTargetPoint: return "InvalidTargetPoint";
    case CommandRejectionReason::MovementTooFar: return "MovementTooFar";
    case CommandRejectionReason::SessionPaused: return "SessionPaused";
    case CommandRejectionReason::RetrySourceMissing: return "RetrySourceMissing";
    case CommandRejectionReason::RetrySourceNotRejected: return "RetrySourceNotRejected";
    case CommandRejectionReason::RetryUnsupportedKind: return "RetryUnsupportedKind";
    default: return "Other";
  }
}

std::string formatVec3Fixed(Vec3 value) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(3) << "(" << value.x << "," << value.y << "," << value.z
      << ")";
  return out.str();
}

std::string inventoryText(const InventoryState& inventory, PlayerSlotId slotId) {
  for (const PlayerInventory& player : inventory.players) {
    if (player.playerSlot != slotId) {
      continue;
    }
    if (player.stacks.empty()) {
      return "empty";
    }
    std::ostringstream out;
    for (std::size_t index = 0; index < player.stacks.size(); ++index) {
      if (index > 0U) {
        out << ",";
      }
      out << player.stacks[index].itemId << ":" << player.stacks[index].count;
    }
    return out.str();
  }
  return "missing";
}

std::string objectiveStatusText(const ObjectiveState& objectives, const std::string& objectiveId) {
  for (const ObjectiveRecord& objective : objectives.objectives) {
    if (objective.objectiveId == objectiveId) {
      switch (objective.status) {
        case ObjectiveStatus::Inactive: return "Inactive";
        case ObjectiveStatus::Active: return "Active";
        case ObjectiveStatus::Complete: return "Complete";
        case ObjectiveStatus::Failed: return "Failed";
      }
    }
  }
  return "missing";
}

std::string entityActiveText(const WorldState& world, const std::string& stableName) {
  const EntityState* entity = world.findByStableName(stableName);
  return entity == nullptr ? "missing" : boolText(entity->active);
}

const CommandRecord* firstRejected(const CommandLog& log) {
  for (const CommandRecord& record : log.records()) {
    if (record.admission == CommandAdmissionStatus::Rejected) {
      return &record;
    }
  }
  return nullptr;
}

const CommandRecord* firstAcceptedRetry(const CommandLog& log) {
  for (const CommandRecord& record : log.records()) {
    if (record.kind == CommandKind::Retry &&
        record.admission == CommandAdmissionStatus::Accepted) {
      return &record;
    }
  }
  return nullptr;
}

std::string idText(std::uint64_t value) {
  return value == 0U ? "0" : std::to_string(value);
}

}  // namespace

RuntimeSummary buildRuntimeSummary(const RuntimeSummaryInput& input) {
  RuntimeSummary summary;
  if (input.state == nullptr) {
    summary.saveRoundtrip = RuntimeProofStatus::Fail;
    summary.resetBaseline = input.resetBaseline;
    summary.replayHash = input.replayHash;
    return summary;
  }

  const SessionState& state = *input.state;
  const CommandLogCounts counts = state.commandLog.counts();
  const CommandRecord* rejected = firstRejected(state.commandLog);
  const CommandRecord* retry = firstAcceptedRetry(state.commandLog);
  const EntityState* player = state.world.findByStableName("player");

  summary.scenario = state.identity.packageId + ":" + state.identity.scenarioId;
  summary.lifecycle = lifecycleText(state.lifecycle);
  summary.outcome = outcomeText(state.outcome);
  summary.finalTick = std::to_string(state.clock.tickIndex);
  summary.playerPosition = player == nullptr ? "missing" : formatVec3Fixed(player->transform.position);
  summary.inventoryPlayer0 = inventoryText(state.inventory, 0);
  summary.goldKeyActive = entityActiveText(state.world, "gold_key");
  summary.tacticalMarkerAlphaActive = entityActiveText(state.world, "tactical_marker_alpha");
  summary.objectiveCollectGoldKey = objectiveStatusText(state.objectives, "collect_gold_key");
  summary.clockMode = clockText(state.clock.mode);
  summary.cameraMode = cameraText(state.camera.activeMode);
  summary.cameraPreviousRealtime = cameraText(state.camera.previousRealtimeMode);
  summary.commandsSubmitted = std::to_string(counts.submitted);
  summary.commandsAccepted = std::to_string(counts.accepted);
  summary.commandsRejected = std::to_string(counts.rejected);
  summary.commandsRetry = std::to_string(counts.retry);
  summary.firstRejection = rejected == nullptr ? "None" : rejectionText(rejected->rejection);
  summary.retryOriginalRejectedCommandId =
      rejected == nullptr ? "0" : idText(rejected->commandId);
  summary.retryCommandId = retry == nullptr ? "0" : idText(retry->commandId);
  summary.retrySourceCommandId = retry == nullptr ? "0" : idText(retry->commandId);
  summary.retryRetrySourceCommandId =
      retry == nullptr ? "0" : idText(retry->payload.retrySourceCommandId);
  summary.retryExecutedCommandId = idText(input.retryExecutedCommandId);
  summary.retryExecutedSequence = idText(input.retryExecutedSequence);
  summary.saveRoundtrip = input.saveRoundtrip;
  summary.resetBaseline = input.resetBaseline;
  summary.replayHash = input.replayHash;
  summary.stateHash = formatStateHash(state.currentStateHash);
  return summary;
}

std::string formatRuntimeSummary(const RuntimeSummary& summary) {
  std::ostringstream out;
  out << "scenario=" << summary.scenario << '\n';
  out << "lifecycle=" << summary.lifecycle << '\n';
  out << "outcome=" << summary.outcome << '\n';
  out << "final_tick=" << summary.finalTick << '\n';
  out << "player.position=" << summary.playerPosition << '\n';
  out << "inventory.player0=" << summary.inventoryPlayer0 << '\n';
  out << "gold_key.active=" << summary.goldKeyActive << '\n';
  out << "tactical_marker_alpha.active=" << summary.tacticalMarkerAlphaActive << '\n';
  out << "objective.collect_gold_key=" << summary.objectiveCollectGoldKey << '\n';
  out << "clock.mode=" << summary.clockMode << '\n';
  out << "camera.mode=" << summary.cameraMode << '\n';
  out << "camera.previousRealtime=" << summary.cameraPreviousRealtime << '\n';
  out << "commands.submitted=" << summary.commandsSubmitted << '\n';
  out << "commands.accepted=" << summary.commandsAccepted << '\n';
  out << "commands.rejected=" << summary.commandsRejected << '\n';
  out << "commands.retry=" << summary.commandsRetry << '\n';
  out << "first_rejection=" << summary.firstRejection << '\n';
  out << "retry.original_rejected_command_id=" << summary.retryOriginalRejectedCommandId << '\n';
  out << "retry.retry_command_id=" << summary.retryCommandId << '\n';
  out << "retry.sourceCommandId=" << summary.retrySourceCommandId << '\n';
  out << "retry.retrySourceCommandId=" << summary.retryRetrySourceCommandId << '\n';
  out << "retry.executed.command_id=" << summary.retryExecutedCommandId << '\n';
  out << "retry.executed.sequence=" << summary.retryExecutedSequence << '\n';
  out << "save.roundtrip=" << proofText(summary.saveRoundtrip) << '\n';
  out << "reset.baseline=" << proofText(summary.resetBaseline) << '\n';
  out << "replay.hash=" << proofText(summary.replayHash) << '\n';
  out << "state_hash=" << summary.stateHash << '\n';
  return out.str();
}

}  // namespace iggy3d

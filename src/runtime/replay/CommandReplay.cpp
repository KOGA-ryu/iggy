#include "runtime/replay/CommandReplay.hpp"

#include <utility>

#include "runtime/session/SessionRunner.hpp"

namespace iggy3d {

namespace {

CommandReplayResult failure(CommandReplayStatus status,
                            const CommandRecord& expected,
                            const SessionCommandResult* actual,
                            std::string diagnostic) {
  CommandReplayResult result;
  result.status = status;
  result.commandId = expected.commandId;
  result.sequence = expected.sequence;
  result.expectedAdmission = expected.admission;
  result.expectedRejection = expected.rejection;
  if (actual != nullptr) {
    result.actualAdmission = actual->command.admission;
    result.actualRejection = actual->command.rejection;
  }
  result.diagnostic = std::move(diagnostic);
  return result;
}

bool validSequenceOrder(const std::vector<CommandRecord>& commands) {
  CommandSequence expected = 1;
  for (const CommandRecord& command : commands) {
    if (command.commandId == kInvalidCommandId || command.sequence != expected) {
      return false;
    }
    ++expected;
  }
  return true;
}

CommandRecord proposalFromSource(const CommandRecord& source) {
  CommandRecord proposal = source;
  proposal.commandId = kInvalidCommandId;
  proposal.sequence = kInvalidCommandSequence;
  proposal.issuedTick = kInvalidCommandTick;
  proposal.scheduledTick = kInvalidCommandTick;
  proposal.admission = CommandAdmissionStatus::Pending;
  proposal.rejection = CommandRejectionReason::None;
  return proposal;
}

bool queuedAcceptedKind(CommandKind kind) {
  return kind == CommandKind::Move || kind == CommandKind::Interact ||
         kind == CommandKind::Inspect || kind == CommandKind::Attack ||
         kind == CommandKind::Wait ||
         kind == CommandKind::Retry;
}

bool immediateAcceptedKind(CommandKind kind) {
  return kind == CommandKind::ToggleTacticalMode || kind == CommandKind::Pause ||
         kind == CommandKind::Resume || kind == CommandKind::StepTacticalTick;
}

CommandReplayResult invalidBaseline(std::string diagnostic) {
  CommandReplayResult result;
  result.status = CommandReplayStatus::InvalidBaseline;
  result.diagnostic = std::move(diagnostic);
  return result;
}

CommandReplayResult commandMissing(std::string diagnostic) {
  CommandReplayResult result;
  result.status = CommandReplayStatus::CommandMissing;
  result.diagnostic = std::move(diagnostic);
  return result;
}

CommandReplayResult executionFailure(const CommandRecord& expected,
                                     const SessionCommandResult& actual,
                                     const Session& session,
                                     std::string diagnostic) {
  CommandReplayResult result = failure(CommandReplayStatus::ExecutionFailed, expected, &actual,
                                       std::move(diagnostic));
  result.actualHash = session.stateHash();
  result.finalTick = session.state().clock.tickIndex;
  result.finalLifecycle = session.lifecycle();
  result.finalOutcome = session.outcome();
  return result;
}

RuntimeSummaryInput summaryInputFor(const CommandReplayRequest& request, const Session& session) {
  RuntimeSummaryInput input;
  input.state = &session.state();
  input.retryExecutedCommandId = request.retryExecutedCommandId;
  input.retryExecutedSequence = request.retryExecutedSequence;
  input.saveRoundtrip = request.saveRoundtrip;
  input.resetBaseline = request.resetBaseline;
  input.replayHash = request.replayHash;
  return input;
}

}  // namespace

CommandReplayResult replayCommands(const CommandReplayRequest& request) {
  Result<Session> created = Session::create(request.baseline);
  if (created.status != ResultStatus::Ok) {
    return invalidBaseline(created.error.code);
  }
  if (request.sourceCommands.empty() || !validSequenceOrder(request.sourceCommands)) {
    return commandMissing("invalid command sequence");
  }

  Session session = std::move(created.value);
  for (const CommandRecord& expected : request.sourceCommands) {
    const CommandRecord proposal = proposalFromSource(expected);
    SessionCommandResult actual = session.submitCommand(proposal);

    if (!actual.appendedToLog || actual.command.commandId != expected.commandId ||
        actual.command.sequence != expected.sequence) {
      return failure(CommandReplayStatus::CommandMissing, expected, &actual,
                     "command identity diverged");
    }
    if (actual.command.admission != expected.admission) {
      return failure(CommandReplayStatus::AdmissionDiverged, expected, &actual,
                     "admission diverged");
    }
    if (expected.admission == CommandAdmissionStatus::Rejected) {
      if (actual.command.rejection != expected.rejection) {
        return failure(CommandReplayStatus::RejectionReasonDiverged, expected, &actual,
                       "rejection reason diverged");
      }
      continue;
    }
    if (expected.admission != CommandAdmissionStatus::Accepted) {
      return failure(CommandReplayStatus::AdmissionDiverged, expected, &actual,
                     "invalid source admission");
    }

    if (immediateAcceptedKind(expected.kind)) {
      if (!actual.executedImmediately) {
        return executionFailure(expected, actual, session, "immediate command did not execute");
      }
      continue;
    }
    if (queuedAcceptedKind(expected.kind)) {
      const SessionRunnerRunResult run =
          runSession(SessionRunnerRunRequest{&session, 16, true, true});
      if (run.status != SessionRunnerStatus::Advanced || run.ticksAdvanced == 0U) {
        return executionFailure(expected, actual, session, run.diagnostic);
      }
      continue;
    }
    return executionFailure(expected, actual, session, "unsupported replay command");
  }

  if (request.finalizeDemo) {
    const SessionFinalizationResult finalized = session.finalizeDemoIfComplete();
    if (finalized.status != SessionFinalizationStatus::Completed &&
        finalized.status != SessionFinalizationStatus::AlreadyComplete) {
      CommandReplayResult result;
      result.status = CommandReplayStatus::ExecutionFailed;
      result.actualHash = session.stateHash();
      result.finalTick = session.state().clock.tickIndex;
      result.finalLifecycle = session.lifecycle();
      result.finalOutcome = session.outcome();
      result.diagnostic = "finalization failed";
      return result;
    }
  }

  CommandReplayResult result;
  result.expectedHash = request.expectedFinalHash;
  result.actualHash = session.stateHash();
  result.finalTick = session.state().clock.tickIndex;
  result.finalLifecycle = session.lifecycle();
  result.finalOutcome = session.outcome();
  if (request.expectedFinalHash != 0U && result.actualHash != request.expectedFinalHash) {
    result.status = CommandReplayStatus::StateHashDiverged;
    result.diagnostic = "state hash diverged";
    return result;
  }

  result.actualSummaryText = formatRuntimeSummary(buildRuntimeSummary(summaryInputFor(request, session)));
  result.expectedSummaryText = request.expectedSummaryText;
  if (!request.expectedSummaryText.empty() && result.actualSummaryText != request.expectedSummaryText) {
    result.status = CommandReplayStatus::SummaryDiverged;
    result.diagnostic = "summary diverged";
    return result;
  }

  result.status = CommandReplayStatus::Matched;
  return result;
}

}  // namespace iggy3d

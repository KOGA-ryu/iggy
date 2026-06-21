#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "runtime/command/Command.hpp"
#include "runtime/command/CommandAdmission.hpp"
#include "runtime/diagnostics/RuntimeSummary.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

enum class CommandReplayStatus : std::uint8_t {
  Matched,
  AdmissionDiverged,
  RejectionReasonDiverged,
  StateHashDiverged,
  SummaryDiverged,
  CommandMissing,
  ExecutionFailed,
  InvalidBaseline,
};

struct CommandReplayRequest {
  SessionCreateRequest baseline;
  std::vector<CommandRecord> sourceCommands;
  StateHashValue expectedFinalHash = 0;
  std::string expectedSummaryText;
  RuntimeProofStatus saveRoundtrip = RuntimeProofStatus::Unknown;
  RuntimeProofStatus resetBaseline = RuntimeProofStatus::Unknown;
  RuntimeProofStatus replayHash = RuntimeProofStatus::Pass;
  CommandId retryExecutedCommandId = kInvalidCommandId;
  CommandSequence retryExecutedSequence = kInvalidCommandSequence;
  bool finalizeDemo = true;
};

struct CommandReplayResult {
  CommandReplayStatus status = CommandReplayStatus::InvalidBaseline;
  CommandId commandId = kInvalidCommandId;
  CommandSequence sequence = kInvalidCommandSequence;
  CommandAdmissionStatus expectedAdmission = CommandAdmissionStatus::Pending;
  CommandAdmissionStatus actualAdmission = CommandAdmissionStatus::Pending;
  CommandRejectionReason expectedRejection = CommandRejectionReason::None;
  CommandRejectionReason actualRejection = CommandRejectionReason::None;
  StateHashValue expectedHash = 0;
  StateHashValue actualHash = 0;
  std::string expectedSummaryText;
  std::string actualSummaryText;
  CommandTick finalTick = kInvalidCommandTick;
  SessionLifecycle finalLifecycle = SessionLifecycle::Loading;
  SessionOutcome finalOutcome = SessionOutcome::None;
  std::string diagnostic;
};

CommandReplayResult replayCommands(const CommandReplayRequest& request);

}  // namespace iggy3d

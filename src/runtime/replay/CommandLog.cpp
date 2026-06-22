#include "runtime/replay/CommandLog.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

namespace iggy3d {

namespace {

CommandLogAppendResult appendFailure(CommandLogAppendStatus status,
                                     const CommandRecord& record,
                                     std::string diagnostic) {
  CommandLogAppendResult result;
  result.status = status;
  result.record = record;
  result.commandId = record.commandId;
  result.sequence = record.sequence;
  result.diagnostic = std::move(diagnostic);
  return result;
}

bool isValidKind(CommandKind kind) {
  return kind != CommandKind::None;
}

bool isValidAdmissionInvariant(const CommandRecord& record) {
  if (record.admission == CommandAdmissionStatus::Accepted) {
    return record.rejection == CommandRejectionReason::None;
  }
  if (record.admission == CommandAdmissionStatus::Rejected) {
    return record.rejection != CommandRejectionReason::None;
  }
  return false;
}

bool referencesEarlierRejected(const std::vector<CommandRecord>& records, CommandId commandId) {
  for (const CommandRecord& record : records) {
    if (record.commandId == commandId) {
      return record.admission == CommandAdmissionStatus::Rejected;
    }
  }
  return false;
}

CommandLogAppendStatus payloadFailureStatus(const std::vector<CommandRecord>& records,
                                            const CommandRecord& record) {
  if (!isValidPlayerSlotId(record.playerSlot)) {
    return CommandLogAppendStatus::InvalidPlayerSlot;
  }
  if (!isValidKind(record.kind)) {
    return CommandLogAppendStatus::InvalidKind;
  }
  if (record.kind == CommandKind::Retry &&
      !referencesEarlierRejected(records, record.payload.retrySourceCommandId)) {
    return CommandLogAppendStatus::InvalidRetrySource;
  }
  if (requiresActor(record.kind) && !isValid(record.actor)) {
    return CommandLogAppendStatus::InvalidCommandPayload;
  }
  if (requiresEntityTarget(record.kind) &&
      (!record.payload.target.hasEntity || !isValid(record.payload.target.entity))) {
    return CommandLogAppendStatus::InvalidCommandPayload;
  }
  if (requiresPointTarget(record.kind) &&
      (!record.payload.target.hasPoint || !isFinite(record.payload.target.point))) {
    return CommandLogAppendStatus::InvalidCommandPayload;
  }
  if (requiresAbilityPayload(record.kind) &&
      (!isValidCommandAbility(record.payload.ability) ||
       !isFinite(record.payload.abilityDirection) ||
       lengthSquared(record.payload.abilityDirection) <= 0.000001F)) {
    return CommandLogAppendStatus::InvalidCommandPayload;
  }
  if (record.kind == CommandKind::Attack &&
      record.admission == CommandAdmissionStatus::Accepted &&
      record.payload.attackDamage <= 0) {
    return CommandLogAppendStatus::InvalidCommandPayload;
  }
  return CommandLogAppendStatus::Ok;
}

CommandLogRestoreResult restoreFailure(CommandLogRestoreStatus status,
                                       std::size_t index,
                                       const CommandRecord& record) {
  return {status, index, record.commandId, record.sequence};
}

bool hasDuplicateCommandIdBefore(const std::vector<CommandRecord>& records, std::size_t index) {
  for (std::size_t prior = 0; prior < index; ++prior) {
    if (records[prior].commandId == records[index].commandId) {
      return true;
    }
  }
  return false;
}

bool restoreRetrySourceValid(const std::vector<CommandRecord>& records, std::size_t index) {
  if (records[index].kind != CommandKind::Retry) {
    return true;
  }
  for (std::size_t prior = 0; prior < index; ++prior) {
    if (records[prior].commandId == records[index].payload.retrySourceCommandId) {
      return records[prior].admission == CommandAdmissionStatus::Rejected;
    }
  }
  return false;
}

}  // namespace

CommandLog::CommandLog() : nextSequence_(1), epoch_(0) {}

const std::vector<CommandRecord>& CommandLog::records() const {
  return records_;
}

bool CommandLog::empty() const {
  return records_.empty();
}

std::size_t CommandLog::size() const {
  return records_.size();
}

CommandSequence CommandLog::nextSequence() const {
  return nextSequence_;
}

std::uint64_t CommandLog::epoch() const {
  return epoch_;
}

CommandLogAppendResult CommandLog::append(const CommandRecord& record) {
  if (record.commandId == kInvalidCommandId) {
    return appendFailure(CommandLogAppendStatus::InvalidCommandId, record, "invalid command id");
  }
  if (findById(record.commandId).record != nullptr) {
    return appendFailure(CommandLogAppendStatus::DuplicateCommandId, record, "duplicate command id");
  }
  if (record.sequence != kInvalidCommandSequence ||
      nextSequence_ == std::numeric_limits<CommandSequence>::max()) {
    return appendFailure(CommandLogAppendStatus::InvalidSequence, record, "invalid sequence");
  }
  if (!isValidAdmissionInvariant(record)) {
    return appendFailure(CommandLogAppendStatus::InvalidAdmissionState, record, "invalid admission");
  }
  const CommandLogAppendStatus payloadStatus = payloadFailureStatus(records_, record);
  if (payloadStatus != CommandLogAppendStatus::Ok) {
    return appendFailure(payloadStatus, record, "invalid payload");
  }

  CommandRecord stored = record;
  stored.sequence = nextSequence_;
  const std::size_t index = records_.size();
  records_.push_back(stored);
  nextSequence_ = records_.back().sequence + 1U;
  return {CommandLogAppendStatus::Ok, records_.back(), index, records_.back().sequence, epoch_,
          records_.back().commandId, {}};
}

CommandLogRestoreResult CommandLog::restoreForLoad(std::vector<CommandRecord> records,
                                                   CommandSequence nextSequence,
                                                   std::uint64_t epoch) {
  CommandSequence expectedSequence = 1;
  for (std::size_t index = 0; index < records.size(); ++index) {
    const CommandRecord& record = records[index];
    if (record.commandId == kInvalidCommandId) {
      return restoreFailure(CommandLogRestoreStatus::InvalidCommandId, index, record);
    }
    if (hasDuplicateCommandIdBefore(records, index)) {
      return restoreFailure(CommandLogRestoreStatus::DuplicateCommandId, index, record);
    }
    if (record.sequence == kInvalidCommandSequence) {
      return restoreFailure(CommandLogRestoreStatus::InvalidSequence, index, record);
    }
    if (record.sequence != expectedSequence) {
      return restoreFailure(CommandLogRestoreStatus::NonMonotonicSequence, index, record);
    }
    if (!isValidAdmissionInvariant(record)) {
      return restoreFailure(CommandLogRestoreStatus::InvalidAdmissionState, index, record);
    }
    if (payloadFailureStatus(records, record) != CommandLogAppendStatus::Ok) {
      return restoreFailure(CommandLogRestoreStatus::InvalidAdmissionState, index, record);
    }
    if (!restoreRetrySourceValid(records, index)) {
      return restoreFailure(CommandLogRestoreStatus::InvalidRetrySource, index, record);
    }
    ++expectedSequence;
  }
  if (nextSequence != expectedSequence) {
    CommandRecord synthetic;
    synthetic.sequence = nextSequence;
    return restoreFailure(CommandLogRestoreStatus::InvalidNextSequence, records.size(), synthetic);
  }
  records_ = std::move(records);
  nextSequence_ = nextSequence;
  epoch_ = epoch;
  return {};
}

void CommandLog::clear() {
  records_.clear();
  nextSequence_ = 1;
}

void CommandLog::reset(CommandLogResetPolicy policy) {
  if (policy == CommandLogResetPolicy::Clear) {
    clear();
    ++epoch_;
    return;
  }
  assert(policy == CommandLogResetPolicy::Clear);
}

CommandLogFindResult CommandLog::findById(CommandId commandId) const {
  if (commandId == kInvalidCommandId) {
    return {};
  }
  for (std::size_t index = 0; index < records_.size(); ++index) {
    if (records_[index].commandId == commandId) {
      return {&records_[index], index};
    }
  }
  return {};
}

CommandLogFindResult CommandLog::findRejectedById(CommandId commandId) const {
  CommandLogFindResult found = findById(commandId);
  if (found.record == nullptr || found.record->admission != CommandAdmissionStatus::Rejected) {
    return {};
  }
  return found;
}

const CommandRecord* CommandLog::latestRejected() const {
  for (auto it = records_.rbegin(); it != records_.rend(); ++it) {
    if (it->admission == CommandAdmissionStatus::Rejected) {
      return &*it;
    }
  }
  return nullptr;
}

CommandLogCounts CommandLog::counts() const {
  CommandLogCounts counts;
  for (const CommandRecord& record : records_) {
    ++counts.submitted;
    if (record.admission == CommandAdmissionStatus::Accepted) {
      ++counts.accepted;
    } else if (record.admission == CommandAdmissionStatus::Rejected) {
      ++counts.rejected;
    }
    if (record.kind == CommandKind::Retry) {
      ++counts.retry;
    } else if (record.kind == CommandKind::Move) {
      ++counts.movement;
    } else if (record.kind == CommandKind::Interact) {
      ++counts.interaction;
    } else if (record.kind == CommandKind::Attack) {
      ++counts.combat;
    } else if (record.kind == CommandKind::CastAbility) {
      ++counts.ability;
    } else if (record.kind == CommandKind::ToggleTacticalMode ||
               record.kind == CommandKind::Pause || record.kind == CommandKind::Resume ||
               record.kind == CommandKind::StepTacticalTick) {
      ++counts.control;
    }
  }
  return counts;
}

}  // namespace iggy3d

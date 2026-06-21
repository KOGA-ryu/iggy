#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "runtime/command/Command.hpp"

namespace iggy3d {

enum class CommandLogResetPolicy : std::uint8_t {
  Clear,
  NewEpoch,
};

struct CommandLogCounts {
  std::uint64_t submitted = 0;
  std::uint64_t accepted = 0;
  std::uint64_t rejected = 0;
  std::uint64_t retry = 0;
  std::uint64_t movement = 0;
  std::uint64_t interaction = 0;
  std::uint64_t combat = 0;
  std::uint64_t control = 0;
};

enum class CommandLogAppendStatus : std::uint8_t {
  Ok,
  InvalidCommandId,
  DuplicateCommandId,
  InvalidPlayerSlot,
  InvalidKind,
  InvalidSequence,
  InvalidAdmissionState,
  InvalidRetrySource,
  InvalidCommandPayload,
};

struct CommandLogAppendResult {
  CommandLogAppendStatus status = CommandLogAppendStatus::Ok;
  CommandRecord record;
  std::size_t index = 0;
  CommandSequence sequence = kInvalidCommandSequence;
  std::uint64_t epoch = 0;
  CommandId commandId = kInvalidCommandId;
  std::string diagnostic;
};

struct CommandLogFindResult {
  const CommandRecord* record = nullptr;
  std::size_t index = 0;
};

enum class CommandLogRestoreStatus : std::uint8_t {
  Restored,
  DuplicateCommandId,
  InvalidCommandId,
  InvalidSequence,
  NonMonotonicSequence,
  InvalidAdmissionState,
  InvalidRetrySource,
  InvalidNextSequence,
};

struct CommandLogRestoreResult {
  CommandLogRestoreStatus status = CommandLogRestoreStatus::Restored;
  std::size_t recordIndex = 0;
  CommandId commandId = kInvalidCommandId;
  CommandSequence sequence = kInvalidCommandSequence;
};

class CommandLog {
public:
  CommandLog();

  const std::vector<CommandRecord>& records() const;
  bool empty() const;
  std::size_t size() const;

  CommandSequence nextSequence() const;
  std::uint64_t epoch() const;

  CommandLogAppendResult append(const CommandRecord& record);
  CommandLogRestoreResult restoreForLoad(std::vector<CommandRecord> records,
                                         CommandSequence nextSequence,
                                         std::uint64_t epoch);
  void clear();
  void reset(CommandLogResetPolicy policy);

  CommandLogFindResult findById(CommandId commandId) const;
  CommandLogFindResult findRejectedById(CommandId commandId) const;
  const CommandRecord* latestRejected() const;

  CommandLogCounts counts() const;

private:
  std::vector<CommandRecord> records_;
  CommandSequence nextSequence_;
  std::uint64_t epoch_;
};

}  // namespace iggy3d

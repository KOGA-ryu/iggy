#pragma once

#include <cstdint>
#include <string>

#include "config/RuntimeConfig.hpp"
#include "content/FixtureScenarioLoader.hpp"
#include "core/result/Result.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/command/CommandAdmission.hpp"
#include "runtime/replay/CommandLog.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d {

struct SaveEnvelope;
struct RuntimeEvent;

struct SessionCreateRequest {
  RuntimeConfig config;
  FixtureScenarioSeed seed;
};

struct SessionCommandResult {
  CommandRecord command;
  CommandAdmissionResult admission;
  CommandLogAppendStatus appendStatus = CommandLogAppendStatus::Ok;
  bool appendedToLog = false;
  bool executedImmediately = false;
};

struct SessionResetResult {
  bool reset = false;
  std::uint64_t baselineHash = 0;
  std::uint64_t currentHash = 0;
};

enum class SessionLoadStatus : std::uint8_t {
  Ok,
  InvalidCandidateState,
  InvalidCommandIdCursor,
  InvalidCommandLogState,
  InvalidLifecycleState,
  ReplacementRejected,
};

struct SessionLoadResult {
  SessionLoadStatus status = SessionLoadStatus::Ok;
  StateHashValue previousHash = 0;
  StateHashValue loadedHash = 0;
  std::string diagnostic;
};

enum class SessionFinalizationStatus : std::uint8_t {
  Completed,
  NotReady,
  AlreadyComplete,
  Failed,
};

struct SessionFinalizationResult {
  SessionFinalizationStatus status = SessionFinalizationStatus::NotReady;
  SessionLifecycle previousLifecycle = SessionLifecycle::Loading;
  SessionLifecycle lifecycle = SessionLifecycle::Loading;
  SessionOutcome outcome = SessionOutcome::None;
  StateHashValue stateHash = 0;
};

class Session {
public:
  Session();
  explicit Session(SessionState state);

  static Result<Session> create(const SessionCreateRequest& request);

  const SessionState& state() const;
  SessionState& mutableStateForOwnedSystems();

  SessionLifecycle lifecycle() const;
  SessionOutcome outcome() const;
  std::uint64_t stateHash() const;

  SessionCommandResult submitCommand(const CommandRecord& command);

  StatusResult tick();
  StatusResult stepOneTick();
  StatusResult runUntilIdle(std::uint32_t maxTicks);

  SessionResetResult resetToBaseline();
  SessionLoadResult replaceStateFromLoad(SessionState loadedState);
  SessionFinalizationResult finalizeDemoIfComplete();

private:
  SessionState state_;
};

}  // namespace iggy3d

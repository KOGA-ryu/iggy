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

class SpatialSurfaceSet;
struct SaveEnvelope;
struct RuntimeEvent;

struct SessionCreateRequest {
  std::string packageId = "iggy3d.first_room";
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

struct SessionTickOptions {
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  bool usePhysicsMovePlanner = false;
};

class Session {
public:
  Session();
  explicit Session(SessionState state);

  static Result<Session> create(const SessionCreateRequest& request);

  const SessionState& state() const;
  SessionState& mutableStateForOwnedSystems();

  // Set-once L4 reasoning-graph carry (A3). The session OWNS the copy; nothing per-tick rebuilds
  // it. Built from the UNFILTERED activation-time RoomAsset by the caller (fixtures today; the
  // product activation sites in the brokered follow-up). Not hashed, not serialized.
  void setReasoningGraph(ReasoningGraph graph);

  SessionLifecycle lifecycle() const;
  SessionOutcome outcome() const;
  std::uint64_t stateHash() const;

  SessionCommandResult submitCommand(const CommandRecord& command);

  StatusResult tick(const SpatialSurfaceSet* collisionSurfaces = nullptr);
  StatusResult tickWithOptions(const SessionTickOptions& options);
  StatusResult stepOneTick(const SpatialSurfaceSet* collisionSurfaces = nullptr);
  StatusResult stepOneTickWithOptions(const SessionTickOptions& options);
  StatusResult runUntilIdle(std::uint32_t maxTicks,
                            const SpatialSurfaceSet* collisionSurfaces = nullptr);
  StatusResult runUntilIdleWithOptions(std::uint32_t maxTicks,
                                       const SessionTickOptions& options);

  SessionResetResult resetToBaseline();
  SessionLoadResult replaceStateFromLoad(SessionState loadedState);
  SessionFinalizationResult finalizeDemoIfComplete();

private:
  SessionState state_;
};

}  // namespace iggy3d

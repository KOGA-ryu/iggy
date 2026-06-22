#pragma once

#include <cstdint>
#include <string>

#include "runtime/session/Session.hpp"

namespace iggy3d {

enum class SessionRunnerStatus : std::uint8_t {
  Idle,
  Advanced,
  Complete,
  Failed,
  MaxTicksExceeded,
};

struct SessionRunnerRunRequest {
  Session* session = nullptr;
  std::uint32_t maxTicks = 0;
  bool stopWhenIdle = true;
  bool stopWhenComplete = true;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
};

struct SessionRunnerRunResult {
  SessionRunnerStatus status = SessionRunnerStatus::Idle;
  std::uint32_t ticksAttempted = 0;
  std::uint32_t ticksAdvanced = 0;
  SessionLifecycle lifecycle = SessionLifecycle::Loading;
  SessionOutcome outcome = SessionOutcome::None;
  StateHashValue stateHash = 0;
  std::string diagnostic;
};

SessionRunnerRunResult runSession(SessionRunnerRunRequest request);
SessionRunnerRunResult stepPausedOnce(Session& session);

}  // namespace iggy3d

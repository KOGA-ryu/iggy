#include "runtime/session/SessionRunner.hpp"

#include "runtime/clock/Clock.hpp"

namespace iggy3d {

namespace {

SessionRunnerRunResult finish(SessionRunnerStatus status, const Session* session, std::string diagnostic = {}) {
  SessionRunnerRunResult result;
  result.status = status;
  result.diagnostic = std::move(diagnostic);
  if (session != nullptr) {
    result.lifecycle = session->lifecycle();
    result.outcome = session->outcome();
    result.stateHash = session->stateHash();
  }
  return result;
}

bool completeOrFailed(const Session& session) {
  return session.lifecycle() == SessionLifecycle::Complete ||
         session.lifecycle() == SessionLifecycle::Failed;
}

bool pendingWork(const Session& session) {
  return !session.state().transient.pendingExecutionSequences.empty();
}

}  // namespace

SessionRunnerRunResult runSession(SessionRunnerRunRequest request) {
  if (request.session == nullptr) {
    return finish(SessionRunnerStatus::Failed, nullptr, "missing session");
  }

  Session& session = *request.session;
  if (request.stopWhenComplete && completeOrFailed(session)) {
    return finish(session.lifecycle() == SessionLifecycle::Complete ? SessionRunnerStatus::Complete
                                                                    : SessionRunnerStatus::Failed,
                  &session);
  }
  if (session.state().clock.mode == ClockMode::Paused) {
    return finish(SessionRunnerStatus::Idle, &session);
  }
  if (request.stopWhenIdle && !pendingWork(session)) {
    return finish(SessionRunnerStatus::Idle, &session);
  }
  if (request.maxTicks == 0U) {
    return finish(SessionRunnerStatus::MaxTicksExceeded, &session, "max ticks exhausted");
  }

  SessionRunnerRunResult result;
  result.status = SessionRunnerStatus::Idle;
  for (; result.ticksAttempted < request.maxTicks; ++result.ticksAttempted) {
    if (request.stopWhenComplete && completeOrFailed(session)) {
      result.status = session.lifecycle() == SessionLifecycle::Complete ? SessionRunnerStatus::Complete
                                                                        : SessionRunnerStatus::Failed;
      break;
    }
    if (session.state().clock.mode == ClockMode::Paused) {
      result.status = result.ticksAdvanced == 0U ? SessionRunnerStatus::Idle
                                                 : SessionRunnerStatus::Advanced;
      break;
    }
    if (request.stopWhenIdle && !pendingWork(session)) {
      result.status = result.ticksAdvanced == 0U ? SessionRunnerStatus::Idle
                                                 : SessionRunnerStatus::Advanced;
      break;
    }

    const CommandTick before = session.state().clock.tickIndex;
    const StatusResult tick = session.tick();
    if (tick.status != ResultStatus::Ok) {
      result.status = SessionRunnerStatus::Failed;
      result.diagnostic = tick.error.code;
      break;
    }
    const CommandTick after = session.state().clock.tickIndex;
    if (after > before) {
      ++result.ticksAdvanced;
    }
  }

  if (result.ticksAttempted == request.maxTicks && request.maxTicks > 0U &&
      (!request.stopWhenIdle || pendingWork(session)) &&
      (!request.stopWhenComplete || !completeOrFailed(session)) &&
      session.state().clock.mode != ClockMode::Paused) {
    result.status = SessionRunnerStatus::MaxTicksExceeded;
    result.diagnostic = "max ticks exhausted";
  }
  result.lifecycle = session.lifecycle();
  result.outcome = session.outcome();
  result.stateHash = session.stateHash();
  return result;
}

SessionRunnerRunResult stepPausedOnce(Session& session) {
  if (session.state().clock.mode != ClockMode::Paused) {
    return finish(SessionRunnerStatus::Failed, &session, "step requires paused clock");
  }

  SessionState& state = session.mutableStateForOwnedSystems();
  const ClockDecision requested = requestStep(state.clock);
  if (requested.status != ClockStatus::Ok) {
    return finish(SessionRunnerStatus::Failed, &session, "step request failed");
  }
  state.clock = requested.state;

  SessionRunnerRunResult result;
  result.ticksAttempted = 1;
  const CommandTick before = session.state().clock.tickIndex;
  const StatusResult stepped = session.stepOneTick();
  if (stepped.status != ResultStatus::Ok) {
    return finish(SessionRunnerStatus::Failed, &session, stepped.error.code);
  }
  result.ticksAdvanced = session.state().clock.tickIndex > before ? 1U : 0U;
  result.status = result.ticksAdvanced == 1U ? SessionRunnerStatus::Advanced
                                             : SessionRunnerStatus::Idle;
  result.lifecycle = session.lifecycle();
  result.outcome = session.outcome();
  result.stateHash = session.stateHash();
  return result;
}

}  // namespace iggy3d

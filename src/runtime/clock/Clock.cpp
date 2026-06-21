#include "runtime/clock/Clock.hpp"

#include <cmath>

namespace iggy3d {

namespace {

ClockDecision decision(ClockStatus status, ClockState state) {
  ClockDecision result;
  result.status = status;
  result.state = state;
  return result;
}

bool isPositiveFinite(float value) {
  return std::isfinite(value) && value > 0.0F;
}

bool equals(float lhs, float rhs) {
  return lhs == rhs;
}

}  // namespace

ClockState makeDefaultClockState() {
  return {};
}

ClockStatus validateClockState(const ClockState& state) {
  if (state.fixedTickRateHz == 0U) {
    return ClockStatus::InvalidTickRate;
  }
  if (!std::isfinite(state.timeScale) || state.timeScale < 0.0F) {
    return ClockStatus::InvalidTimeScale;
  }
  if (!isPositiveFinite(state.previousUnpausedTimeScale)) {
    return ClockStatus::InvalidTimeScale;
  }
  if (state.previousUnpausedMode == ClockMode::Paused) {
    return ClockStatus::InvalidTimeScale;
  }
  if (state.mode == ClockMode::Normal && !equals(state.timeScale, 1.0F)) {
    return ClockStatus::InvalidTimeScale;
  }
  if (state.mode == ClockMode::Slow &&
      (!isPositiveFinite(state.timeScale) || state.timeScale >= 1.0F)) {
    return ClockStatus::InvalidTimeScale;
  }
  if (state.mode == ClockMode::Paused && !equals(state.timeScale, 0.0F)) {
    return ClockStatus::InvalidTimeScale;
  }
  if (state.previousUnpausedMode == ClockMode::Normal &&
      !equals(state.previousUnpausedTimeScale, 1.0F)) {
    return ClockStatus::InvalidTimeScale;
  }
  if (state.previousUnpausedMode == ClockMode::Slow &&
      (!isPositiveFinite(state.previousUnpausedTimeScale) ||
       state.previousUnpausedTimeScale >= 1.0F)) {
    return ClockStatus::InvalidTimeScale;
  }
  return ClockStatus::Ok;
}

ClockDecision shouldRunAutomaticTick(const ClockState& state) {
  ClockDecision result = decision(validateClockState(state), state);
  result.shouldRunTick = result.status == ClockStatus::Ok && state.mode != ClockMode::Paused;
  return result;
}

ClockDecision enterSlow(ClockState state, float slowScale) {
  const ClockState original = state;
  if (!isPositiveFinite(slowScale) || slowScale >= 1.0F) {
    return decision(ClockStatus::InvalidTimeScale, original);
  }
  state.mode = ClockMode::Slow;
  state.previousUnpausedMode = ClockMode::Slow;
  state.previousUnpausedTimeScale = slowScale;
  state.timeScale = slowScale;
  state.stepRequested = false;
  return decision(validateClockState(state), state);
}

ClockDecision exitSlow(ClockState state) {
  state.mode = ClockMode::Normal;
  state.previousUnpausedMode = ClockMode::Normal;
  state.previousUnpausedTimeScale = 1.0F;
  state.timeScale = 1.0F;
  state.stepRequested = false;
  return decision(validateClockState(state), state);
}

ClockDecision pause(ClockState state) {
  if (state.mode != ClockMode::Paused) {
    if (state.mode != ClockMode::Normal && state.mode != ClockMode::Slow) {
      return decision(ClockStatus::InvalidTimeScale, state);
    }
    if (!isPositiveFinite(state.timeScale)) {
      return decision(ClockStatus::InvalidTimeScale, state);
    }
    state.previousUnpausedMode = state.mode;
    state.previousUnpausedTimeScale = state.timeScale;
  }
  state.mode = ClockMode::Paused;
  state.timeScale = 0.0F;
  state.stepRequested = false;
  return decision(validateClockState(state), state);
}

ClockDecision resume(ClockState state) {
  const ClockState original = state;
  if (state.previousUnpausedMode == ClockMode::Paused ||
      !isPositiveFinite(state.previousUnpausedTimeScale)) {
    return decision(ClockStatus::InvalidTimeScale, original);
  }
  state.mode = state.previousUnpausedMode;
  state.timeScale = state.previousUnpausedTimeScale;
  state.stepRequested = false;
  return decision(validateClockState(state), state);
}

ClockDecision requestStep(ClockState state) {
  if (state.mode != ClockMode::Paused) {
    return decision(ClockStatus::StepRequiresPaused, state);
  }
  state.stepRequested = true;
  return decision(validateClockState(state), state);
}

ClockDecision consumeStep(ClockState state) {
  ClockDecision result = decision(validateClockState(state), state);
  if (result.status != ClockStatus::Ok) {
    return result;
  }
  if (state.mode == ClockMode::Paused && state.stepRequested) {
    result.state.stepRequested = false;
    result.shouldRunTick = true;
    result.consumedStep = true;
  }
  return result;
}

ClockState advanceTick(ClockState state) {
  ++state.tickIndex;
  return state;
}

}  // namespace iggy3d

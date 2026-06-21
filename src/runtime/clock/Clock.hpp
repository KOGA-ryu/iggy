#pragma once

#include <cstdint>

#include "runtime/clock/ClockState.hpp"

namespace iggy3d {

enum class ClockStatus : std::uint8_t {
  Ok,
  InvalidTickRate,
  InvalidTimeScale,
  StepRequiresPaused,
};

struct ClockDecision {
  ClockStatus status = ClockStatus::Ok;
  bool shouldRunTick = false;
  bool consumedStep = false;
  ClockState state;
};

ClockState makeDefaultClockState();
ClockStatus validateClockState(const ClockState& state);

ClockDecision shouldRunAutomaticTick(const ClockState& state);
ClockDecision enterSlow(ClockState state, float slowScale);
ClockDecision exitSlow(ClockState state);
ClockDecision pause(ClockState state);
ClockDecision resume(ClockState state);
ClockDecision requestStep(ClockState state);
ClockDecision consumeStep(ClockState state);
ClockState advanceTick(ClockState state);

}  // namespace iggy3d

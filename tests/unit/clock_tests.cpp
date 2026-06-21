#include "runtime/clock/Clock.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool defaultAndValidation() {
  iggy3d::ClockState clock = iggy3d::makeDefaultClockState();
  bool ok = expect(iggy3d::validateClockState(clock) == iggy3d::ClockStatus::Ok, "default valid") &&
            expect(clock.mode == iggy3d::ClockMode::Normal, "default normal") &&
            expect(clock.previousUnpausedMode == iggy3d::ClockMode::Normal, "default previous") &&
            expect(clock.previousUnpausedTimeScale == 1.0F, "default previous scale") &&
            expect(clock.tickIndex == 0U, "default tick") &&
            expect(clock.fixedTickRateHz == 20U, "default tick rate") &&
            expect(clock.timeScale == 1.0F, "default scale") &&
            expect(!clock.stepRequested, "default no step");

  clock.fixedTickRateHz = 0;
  ok = ok && expect(iggy3d::validateClockState(clock) == iggy3d::ClockStatus::InvalidTickRate,
                    "invalid rate");
  clock = iggy3d::makeDefaultClockState();
  clock.timeScale = -1.0F;
  ok = ok && expect(iggy3d::validateClockState(clock) == iggy3d::ClockStatus::InvalidTimeScale,
                    "invalid scale");
  return ok;
}

bool acceptanceFlow() {
  iggy3d::ClockState clock = iggy3d::makeDefaultClockState();
  bool ok = expect(iggy3d::shouldRunAutomaticTick(clock).shouldRunTick, "normal auto tick");

  iggy3d::ClockDecision slow = iggy3d::enterSlow(clock, 0.250F);
  ok = ok && expect(slow.status == iggy3d::ClockStatus::Ok, "enter slow ok") &&
       expect(slow.state.mode == iggy3d::ClockMode::Slow, "slow mode") &&
       expect(slow.state.timeScale == 0.250F, "slow scale") &&
       expect(slow.state.previousUnpausedMode == iggy3d::ClockMode::Slow, "slow previous") &&
       expect(slow.state.previousUnpausedTimeScale == 0.250F, "slow previous scale");

  iggy3d::ClockDecision paused = iggy3d::pause(slow.state);
  ok = ok && expect(paused.status == iggy3d::ClockStatus::Ok, "pause ok") &&
       expect(paused.state.mode == iggy3d::ClockMode::Paused, "paused mode") &&
       expect(paused.state.previousUnpausedMode == iggy3d::ClockMode::Slow, "paused previous") &&
       expect(paused.state.previousUnpausedTimeScale == 0.250F, "paused previous scale") &&
       expect(paused.state.timeScale == 0.0F, "paused scale") &&
       expect(!iggy3d::shouldRunAutomaticTick(paused.state).shouldRunTick, "paused no auto");

  iggy3d::ClockDecision requested = iggy3d::requestStep(paused.state);
  ok = ok && expect(requested.status == iggy3d::ClockStatus::Ok, "request step ok") &&
       expect(requested.state.stepRequested, "step requested");
  iggy3d::ClockDecision consumed = iggy3d::consumeStep(requested.state);
  ok = ok && expect(consumed.status == iggy3d::ClockStatus::Ok, "consume step ok") &&
       expect(consumed.shouldRunTick, "step runs tick") &&
       expect(consumed.consumedStep, "step consumed") &&
       expect(!consumed.state.stepRequested, "step cleared") &&
       expect(consumed.state.mode == iggy3d::ClockMode::Paused, "still paused");
  clock = iggy3d::advanceTick(consumed.state);
  ok = ok && expect(clock.tickIndex == 1U, "tick advanced");

  iggy3d::ClockDecision resumed = iggy3d::resume(clock);
  ok = ok && expect(resumed.status == iggy3d::ClockStatus::Ok, "resume ok") &&
       expect(resumed.state.mode == iggy3d::ClockMode::Slow, "resume slow") &&
       expect(resumed.state.timeScale == 0.250F, "resume scale");

  iggy3d::ClockDecision normal = iggy3d::exitSlow(resumed.state);
  ok = ok && expect(normal.status == iggy3d::ClockStatus::Ok, "exit slow ok") &&
       expect(normal.state.mode == iggy3d::ClockMode::Normal, "normal mode") &&
       expect(normal.state.timeScale == 1.0F, "normal scale") &&
       expect(normal.state.previousUnpausedMode == iggy3d::ClockMode::Normal, "normal previous");
  return ok;
}

bool failuresDoNotMutateReturnedState() {
  const iggy3d::ClockState original = iggy3d::makeDefaultClockState();
  const iggy3d::ClockDecision slow = iggy3d::enterSlow(original, 1.0F);
  const iggy3d::ClockDecision step = iggy3d::requestStep(original);
  return expect(slow.status == iggy3d::ClockStatus::InvalidTimeScale, "bad slow rejected") &&
         expect(slow.state.mode == original.mode && slow.state.timeScale == original.timeScale,
                "bad slow unchanged") &&
         expect(step.status == iggy3d::ClockStatus::StepRequiresPaused, "step requires paused") &&
         expect(!step.state.stepRequested, "bad step unchanged");
}

}  // namespace

int main() {
  bool ok = true;
  ok = defaultAndValidation() && ok;
  ok = acceptanceFlow() && ok;
  ok = failuresDoNotMutateReturnedState() && ok;
  return ok ? 0 : 1;
}

# Clock

File:

- `/Users/kogaryu/iggy3d/src/runtime/clock/ClockState.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/clock/Clock.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/clock/Clock.cpp`

Verified at: `fd3abdec`

## Owns

- Runtime clock packet shape: `ClockMode` and `ClockState`.
- Clock decision/status contract.
- Clock state validation for tick rate, mode, time scale, previous unpaused mode, and step flag.
- Normal/slow/pause/resume/step/tick transition helpers.

## Does Not Own

- Session tick execution.
- Command admission for pause/resume/step commands.
- Camera policy side effects after clock changes.
- Save/load serialization of clock fields.
- App timing, SDL frame pacing, or renderer cadence.

## Reads

- `ClockState` mode, previous unpaused mode, time scales, tick index, fixed tick rate, and step request flag.
- Slow-mode time scale argument for `enterSlow`.

## Writes / Mutates

- Functions take `ClockState` by value and return updated copies in `ClockDecision` or `ClockState`.
- `advanceTick` increments the returned tick index only.
- No caller-owned state is mutated directly.

## Calls Out To / Wires Out To

- `Session` and `SessionRunner` use clock decisions to run or suppress ticks.
- `CommandAdmission` reads clock mode for paused/step command validation.
- `CameraModePolicy` reads clock mode to transition camera mode.
- Save/load and diagnostics read clock state as persisted/proof data.

## Called By / Entry Points

- `makeDefaultClockState(...)`
- `validateClockState(...)`
- `shouldRunAutomaticTick(...)`
- `enterSlow(...)`
- `exitSlow(...)`
- `pause(...)`
- `resume(...)`
- `requestStep(...)`
- `consumeStep(...)`
- `advanceTick(...)`

## Invariants

- Fixed tick rate must be nonzero.
- Normal mode has time scale `1.0`.
- Slow mode has positive finite time scale below `1.0`.
- Paused mode has time scale `0.0`.
- Previous unpaused mode is never paused and carries a valid matching time scale.
- Step requests are valid only while paused and are consumed into one tick decision.
- Entering slow, exiting slow, pausing, and resuming clear step requests.

## Tests / Proof Commands

- `rg -n "clock_tests|ClockState|ClockMode|validateClockState|requestStep|consumeStep" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `clock_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/session/Session.*`
- `/Users/kogaryu/iggy3d/src/runtime/session/SessionRunner.*`
- `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.*`
- `/Users/kogaryu/iggy3d/src/runtime/camera/CameraModePolicy.*`

## Update When

- Clock state fields, mode/time-scale validation, pause/resume/slow/step semantics, or tick-advance contract changes.

## Do Not Update When

- Only session tick work, command routing, camera transitions, save formatting, or app frame pacing changes.

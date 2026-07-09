# Camera Mode Policy

File:

- `/Users/kogaryu/iggy3d/src/runtime/camera/CameraState.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/camera/CameraModePolicy.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/camera/CameraModePolicy.cpp`

Verified at: `e5586141`

## Owns

- Runtime camera packet shape: `CameraMode`, `CameraTarget`, and `CameraState`.
- Camera mode policy request/result/status contract.
- Transitions between realtime camera modes and tactical overhead mode.
- Clock-mode-to-camera-mode policy and camera input-clear request flag.

## Does Not Own

- App viewport/controller camera rendering.
- User settings UI for camera preference.
- Session clock state mutation.
- Save/load serialization of camera state.
- Projection/render camera matrices.

## Reads

- Input `CameraState`.
- `ClockMode` from policy request.
- Active camera mode and previous realtime mode.

## Writes / Mutates

- Returns a copied `CameraState` with updated active mode, previous realtime mode, and input-clear request flag.
- Does not mutate caller-owned state directly.

## Calls Out To / Wires Out To

- `Session` applies camera policy after clock changes.
- Runtime diagnostics and projection read camera state.
- App view/controller layers consume projected camera state downstream.

## Called By / Entry Points

- `isRealtimeCameraMode(...)`
- `isTacticalCameraMode(...)`
- `makeDefaultCameraState(...)`
- `enterTacticalCamera(...)`
- `exitTacticalCamera(...)`
- `applyClockModeToCamera(...)`
- `clearCameraInputRequest(...)`

## Invariants

- Realtime modes are first-person and third-person; tactical overhead is the only tactical mode.
- Entering tactical preserves the previous realtime mode when the current mode is realtime.
- Exiting tactical returns to `previousRealtimeMode`.
- Normal clock exits tactical; slow and paused clocks enter or remain tactical.
- Mode transitions request input clear.
- Invalid camera mode or clock mode returns explicit policy status without changing ownership boundaries.

## Tests / Proof Commands

- `rg -n "camera_mode_policy_tests|enterTacticalCamera|exitTacticalCamera|applyClockModeToCamera|clearCameraInputRequest" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `camera_mode_policy_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/clock/ClockState.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/session/Session.*`
- `/Users/kogaryu/iggy3d/src/app/iggy3d/view/CameraController.*`
- `/Users/kogaryu/iggy3d/src/projection/scene/*`

## Update When

- Camera state fields, camera modes, clock-to-camera transition policy, input-clear semantics, or policy statuses change.

## Do Not Update When

- Only app camera controller math, frontend settings labels, render projection, or save formatting changes.

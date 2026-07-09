# File Spec

Files: `src/app/iggy3d/view/CameraController.hpp`, `src/app/iggy3d/view/CameraController.cpp`

Verified at: `d5abfcf8`

## Owns

- Product camera look-action application into app viewport state.
- Yaw wrapping, pitch clamping, sensitivity application, invert-look policy, and camera proof/status fields for first-person product camera control.

## Does Not Own

- Runtime camera state.
- Render camera matrices.
- Input polling or action routing.
- Movement direction math that reads camera yaw.
- Receipt field emission for camera state.

## Reads

- `ActionState` axis values for `PlayerLookX` and `PlayerLookY`.
- `FrontendSettings.lookSensitivity` and `FrontendSettings.invertLook`.
- Current `ProductViewportState.cameraYawDegrees` and `cameraPitchDegrees`.

## Writes / Mutates

- Mutates `ProductViewportState.cameraControllerActive`, `lookInputUsed`, `cameraInputSource`, `cameraMode`, `cameraController`, `cameraYawDegrees`, and `cameraPitchDegrees`.
- Does not mutate input state or runtime session state.

## Calls Out To / Wires Out To

- `actionAxisValue(...)`.
- Callers supply accepted actions and source labels.

## Called By / Entry Points

- `AppKernel.cpp` applies scripted smoke look actions.
- `InputFrame.cpp` applies accepted gameplay and creative fly look actions.
- Focused proof: `rg -n "applyProductCameraActions|camera_controller_active|camera_yaw_degrees" src/app tests/unit tests/smoke`.

## Invariants

- No look input means no viewport mutation.
- Yaw wraps into the product heading range.
- Pitch is clamped to the product pitch range.
- Look sensitivity scales both yaw and pitch.
- `invertLook` flips pitch direction only.
- Camera controller proof fields identify this path as `product_camera` and preserve the caller source string.

## Tests / Proof Commands

- `rg -n "product_camera_controller_tests|product_gameplay_controls_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "applyProductCameraActions|camera_yaw_degrees|camera_pitch_degrees" tests/unit/product_camera_controller_tests.cpp tests/smoke/product_gameplay_controls_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/camera/*` unless runtime camera contracts change.
- `src/app/iggy3d/window/InputFrame.*` unless accepted action source plumbing changes.
- `src/app/iggy3d/gameplay/ControllerKinematics.*` unless movement reads camera state differently.
- `src/app/iggy3d/receipt/GameplaySceneStateFields.*` unless camera receipt keys change.

## Update When

- Camera look-step constants, clamping/wrapping policy, viewport fields, accepted input action names, or settings usage changes.

## Do Not Update When

- Only render camera projection, movement consumption of yaw, or receipt formatting changes behind the same viewport camera fields.

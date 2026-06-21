# Camera Render Contract

This document defines how runtime camera truth becomes renderer camera data.

The renderer does not own camera truth. Runtime owns `CameraState`; renderer consumes derived `RenderCameraFrame` inside `FrameInput`.

## Source Of Truth

Runtime camera truth lives in:

```text
src/runtime/camera/CameraState.hpp
```

Runtime-owned fields:

```cpp
CameraMode activeMode;
CameraMode previousRealtimeMode;
CameraTarget target;
float yawDegrees;
float pitchDegrees;
float orbitDistance;
bool inputClearRequested;
```

Rules:

- `activeMode`, `previousRealtimeMode`, target entity/point, yaw, pitch, and orbit distance are save/replay/hash truth;
- `inputClearRequested` is transient runtime state;
- renderer cannot change any of these fields;
- renderer cannot clear `inputClearRequested`;
- renderer cannot switch camera mode;
- raw mouse/controller deltas do not enter `FrameInput`.

## Derived Render Data

Renderer consumes:

```text
FrameInput.camera
```

Expected shape from [frame_input_contract.md](frame_input_contract.md):

```cpp
struct RenderCameraFrame {
  RenderCameraMode mode;
  Vec3 worldEye;
  Vec3 worldForward;
  Vec3 worldUp;
  Mat4 viewFromWorld;
  Mat4 clipFromView;
  Mat4 clipFromWorld;
  float nearPlane;
  float farPlane;
};
```

Rules:

- `RenderCameraFrame` is derived presentation data;
- it may be regenerated every frame;
- it is excluded from save truth unless runtime later promotes exact derived values;
- it is excluded from `StateHash`;
- renderer validates it before drawing;
- renderer may not write it back into runtime.

## Mode Mapping

| Runtime `CameraMode` | Render `RenderCameraMode` | Role |
| --- | --- | --- |
| `FirstPerson` | `FirstPerson` | realtime player-eye or near-eye play |
| `ThirdPerson` | `ThirdPerson` | realtime small-third-person/orbit play |
| `TacticalOverhead` | `TacticalOverhead` | slow-time tactical overview |

Runtime policy owns transitions:

- entering tactical stores `previousRealtimeMode`;
- leaving tactical restores `previousRealtimeMode`;
- renderer only reflects the derived result.

## Coordinate And Matrix Rules

World axes:

- X right;
- Y up;
- Z forward.

Matrix rules:

- `Mat4` is row-major storage;
- math uses column-vector multiplication;
- `clipFromWorld = clipFromView * viewFromWorld`;
- Vulkan-compatible depth range is near `0`, far `1`;
- all camera vectors and matrices must be finite;
- `worldForward` and `worldUp` must be nonzero after validation.

Orientation rules:

- `yawDegrees` rotates around world Y/up;
- `pitchDegrees` tilts up/down;
- first implementation should clamp pitch in camera policy or derivation to avoid degenerate forward/up vectors;
- exact clamp values belong in file plans.

## Default Presentation Constants

These are first visual defaults, not permanent camera design.

| Mode | FOV | Near | Far | Notes |
| --- | --- | --- | --- | --- |
| `FirstPerson` | `75` degrees vertical | `0.05` | `250` | close near plane for interior play |
| `ThirdPerson` | `65` degrees vertical | `0.1` | `300` | first-room default mode |
| `TacticalOverhead` | `55` degrees vertical | `0.5` | `600` | farther view for slow-time tactics |

Rules:

- values are renderer presentation defaults derived during frame assembly;
- runtime camera truth does not depend on swapchain size;
- aspect ratio comes from `FrameInput.viewport`;
- if defaults change, diagnostics should report the active values.

Required diagnostics:

```text
camera_mode=first_person|third_person|tactical_overhead
camera_fov_degrees=
camera_near=
camera_far=
camera_eye=
camera_forward=
camera_up=
```

## Target Selection

`CameraTarget` can refer to:

- an entity;
- a point;
- both, with point available as explicit target override;
- neither, in which case frame assembly must use a documented fallback target.

Rules:

- target resolution reads runtime state but does not mutate it;
- missing target entity must produce a stable diagnostic and fallback;
- renderer does not perform target lookup from runtime;
- app/projection/frame assembly owns target resolution before `FrameInput`.

Fallback target recommendation:

```text
target = world origin or first controlled player position
reason_code=camera_target_missing
```

Exact fallback belongs in the file plan once runtime player/session APIs exist.

## First-Person Derivation

Goal: realtime player-eye or near-eye view.

Inputs:

- `CameraState.target`;
- `yawDegrees`;
- `pitchDegrees`;
- runtime/player transform resolved by frame assembly;
- first-person eye offset.

Recommended first defaults:

```text
eye_height=1.6
forward_distance=1.0 for target diagnostics only
orbit_distance ignored
```

Rules:

- `worldEye` is target position plus eye offset;
- `worldForward` comes from yaw/pitch;
- `worldUp` should stay close to world up unless camera roll is later introduced;
- no collision camera correction in first implementation;
- no head bob or animation in first implementation.

Out of scope:

- weapon viewmodel;
- camera collision;
- head bob;
- roll;
- per-weapon FOV.

## Third-Person Derivation

Goal: small-third-person play view and first-room default.

Inputs:

- target position;
- `yawDegrees`;
- `pitchDegrees`;
- `orbitDistance`.

Recommended first defaults:

```text
orbit_distance=CameraState.orbitDistance, default 8.0
target_height=1.0
min_orbit_distance=2.0
max_orbit_distance=20.0
```

Derivation:

- compute target point from target entity/point plus `target_height`;
- compute forward direction from yaw/pitch;
- place eye behind target along negative forward by `orbitDistance`;
- keep `worldUp` as world up for first implementation;
- build `viewFromWorld` from eye/target/up.

Rules:

- runtime owns orbit distance value;
- renderer does not clamp and write orbit distance back;
- if derivation clamps for presentation safety, the clamp is diagnostic-only and must not mutate runtime;
- no camera collision in first implementation.

First-room acceptance:

- `ThirdPerson` is expected initial mode;
- player proxy and room proxy are visible at startup;
- camera diagnostics print mode/eye/near/far.

## Tactical Overhead Derivation

Goal: slow-time tactical overview.

Inputs:

- runtime tactical mode;
- target entity/point or tactical focus point;
- yaw if used for orientation;
- tactical presentation constants.

Recommended first defaults:

```text
tactical_height=18.0
tactical_pitch_degrees=-60.0
tactical_orbit_distance=0.0 or ignored
tactical_far=600.0
```

Rules:

- tactical mode is runtime truth;
- renderer cannot enter or leave tactical;
- camera looks down toward target/focus point;
- tactical camera may use farther near/far defaults;
- tactical camera does not change replay result beyond runtime camera mode truth already in hash.

Out of scope:

- tactical camera pan/zoom UI;
- selection/picking;
- occlusion fading;
- grid overlays unless debug projection emits them.

## Aspect And Resize

Aspect ratio comes from `FrameInput.viewport`.

Rules:

- app/platform shell reports framebuffer size;
- frame assembly computes aspect ratio;
- `clipFromView` uses that aspect ratio;
- runtime `CameraState` does not store aspect ratio;
- renderer swapchain resize does not mutate runtime camera truth;
- minimized/zero-size viewport produces non-drawable frame handling, not camera mutation.

Diagnostics:

```text
viewport_width=
viewport_height=
camera_aspect_ratio=
drawable=true|false
```

## Interpolation

First implementation default: no visual interpolation.

Rules:

- `FrameInput.clock.interpolationAlpha = 0.0`;
- render camera is derived from the current runtime camera state;
- renderer does not advance camera using presentation delta;
- future interpolation must use immutable snapshots and cannot alter replay hash.

If interpolation is introduced later:

- runtime tick states remain authoritative;
- interpolation affects presentation only;
- diagnostics should print `camera_interpolated=true`.

## Validation Rules

Frame assembly or render validation must reject invalid camera frames.

Minimum validation:

- mode is known;
- `worldEye` is finite;
- `worldForward` is finite and nonzero;
- `worldUp` is finite and nonzero;
- `nearPlane > 0`;
- `farPlane > nearPlane`;
- matrices are finite;
- `clipFromWorld` matches `clipFromView * viewFromWorld` within tolerance;
- aspect ratio is finite and positive when drawable.

Failure behavior:

- invalid camera produces `reason_code=frame_input_invalid` or a more specific future code;
- Vulkan renderer skips drawing invalid frame;
- runtime state is not mutated.

## Replay And Save Invariance

Renderer camera data must not affect deterministic runtime results.

Required proof:

1. Run deterministic script with renderer disabled.
2. Run deterministic script with null renderer consuming `FrameInput`.
3. Run deterministic script with Vulkan renderer in strict lane.
4. Compare runtime summary and state hash.

Expected:

- runtime summary equal;
- state hash equal;
- camera mode truth equal;
- renderer diagnostics may differ;
- screenshot/frame hash may differ across platforms and is diagnostic only.

## Tests

Unit tests:

```text
tests/unit/render_projection_input_tests.cpp
tests/unit/render_replay_invariance_tests.cpp
```

Future targeted test:

```text
tests/unit/render_camera_frame_tests.cpp
```

Smoke tests:

```text
tests/smoke/vulkan_first_room_smoke.cpp
```

Required coverage:

- third-person default derives finite camera frame;
- first-person mode maps correctly;
- tactical mode maps correctly;
- invalid near/far rejected;
- invalid forward/up rejected;
- aspect ratio changes projection matrix without mutating runtime camera truth;
- replay hash unchanged by renderer camera consumption.

## Receipt Fields

Recommended receipt fields:

```text
camera_mode=first_person|third_person|tactical_overhead
camera_fov_degrees=
camera_near=
camera_far=
camera_eye=
camera_forward=
camera_up=
camera_aspect_ratio=
camera_interpolated=true|false
frame_input_valid=true|false
```

Rules:

- fields are diagnostics;
- fields do not affect save/replay truth;
- omit or print `unavailable` only when no render frame was attempted.

## Acceptance Criteria

The camera render contract is satisfied when:

- `CameraState` remains runtime truth;
- `RenderCameraFrame` is derived and finite;
- third-person first-room camera shows player and room proxy;
- first-person and tactical modes have documented derivation rules;
- renderer cannot mutate camera state;
- resize/aspect affects projection only;
- replay hash is unchanged by rendering;
- diagnostics print camera mode and projection facts.

## Open Detail Items

These belong in future file plans:

- exact camera derivation helper file path;
- exact look-at matrix helper if not already owned by math;
- exact FOV constants after gameplay feel pass;
- exact pitch clamp values;
- exact first-person eye offset from player geometry;
- exact tactical focus target rules;
- exact camera collision plan if ever added;
- exact screenshot framing checks for first-room smoke.

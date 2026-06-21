# src/render/FrameInput.hpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `src/render/FrameInput.hpp`

Purpose: Declare the backend-neutral per-frame data contract consumed by all renderer backends.

## Build Position

Packet order: 1 - Backend-neutral renderer boundary
Owner module: `frame input`
File kind: `header`
Current-build contract: this file plan is authoritative for later implementation packets, but it is not a signal to write renderer C++ before the headless runtime and projection gates are green.

Source docs read for this plan:
- `docs/vulkan/README.md`
- `docs/vulkan/renderer_file_plan_order.md`
- `docs/vulkan/vulkan_first_file_plans_index.md`
- `docs/vulkan/renderer_packet_template.md`
- `docs/vulkan/file_surface.md`
- `docs/vulkan/boundaries.md`
- `docs/vulkan/diagnostics_and_tests.md`
- `docs/vulkan/frame_input_contract.md`
- `docs/vulkan/camera_render_contract.md`
- `docs/vulkan/first_room_render_contract.md`

## Ownership

This file owns:
- Declare the backend-neutral per-frame data contract consumed by all renderer backends.
- the public or private names listed in the file shape section;
- diagnostics fields directly tied to its responsibility.

This file must never own:
- gameplay truth;
- command legality;
- save/load truth;
- replay or deterministic state-hash truth;
- content package validation truth;
- renderer fallback without diagnostics;
- legacy renderer linkage.

Runtime firewall boundaries:
- runtime, content, projection, and save code do not depend on this file unless it is a backend-neutral render contract explicitly consumed by an app layer;
- this file cannot mutate runtime state directly;
- renderer output cannot affect replay results.

## Required Include Policy

Allowed includes: standard library, core math/id value types, projection value types where the frame contract permits them, and `src/render/**` backend-neutral headers.

Forbidden includes: Vulkan SDK headers, `Vk*` types, `VK_*` constants, SDL/window headers, runtime mutation internals, content validators, save/load internals, and old repo headers.

Vulkan headers are forbidden in backend-neutral renderer, null renderer, and non-Vulkan unit test files.

SDL/window headers are forbidden outside app/platform glue and Vulkan platform smoke tests.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `RenderViewport`
- `RenderFrameClock`
- `RenderCameraMode`
- `RenderCameraFrame`
- `RenderSceneFrame`
- `FrameInput`
- `validateFrameInput`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Backend-neutral values are owned by renderer API callers for the call duration unless copied into diagnostics. Runtime/projection data remains authoritative outside the renderer. Diagnostics receipts are renderer-owned output. No GPU, window, or Vulkan object lifetime exists in this file.

## Semantics

Normal path: expose or test the backend-neutral renderer contract. Skip/fail: invalid frame/config inputs fail locally; Vulkan availability is not evaluated by this file.

Platform behavior:
macOS/MoltenVK: report `platform=macos` and `platform_lane=moltenvk` when Vulkan is attempted; MoltenVK portability details are diagnostics, not cross-platform law.
Linux: report `platform=linux` and `platform_lane=native_vulkan` for hardware/native validation; software Vulkan uses a separate lane.
Windows: report `platform=windows` and `platform_lane=native_vulkan`; multi-config shader/package paths must include the active config where relevant.
Software Vulkan: allowed for optional development evidence only; it cannot replace native macOS/Linux/Windows proof.
Strict lane: required gates fail with `result=fail`.
Optional lane: unsupported environment or missing optional Vulkan prerequisites may skip with `result=skip` before unsafe renderer work begins.

## Diagnostics And Result Policy

Stable reason names must use lowercase snake-case text. Receipts use deterministic key-value lines.

Required receipt fields:
```text
receipt_version=1
repo=iggy3d
file_plan=src/render/FrameInput.hpp
packet_order=1
allowed_to_implement_code_now=false
backend=null|vulkan|unavailable
frame_input_valid=true|false|unavailable
runtime_hash_before=
runtime_hash_after=
replay_invariant=true|false|unavailable
reason_code=
```

User-facing error message shape when this file contributes to visual startup failure:
```text
This machine cannot run the Vulkan visual renderer required by this build.
Reason: <specific renderer or platform reason>.
Action: run the headless runtime demo or use a machine/runtime that satisfies the Vulkan baseline.
```

## Fallback Policy

Fallback policy: no Vulkan fallback is needed. Null renderer fallback is explicit only when selected by app config or test harness.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=frame_input
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: constant CPU setup and no GPU work.
Per-frame cost: linear in submitted frame items for validation/diagnostics; null renderer performs no draw work.
Resize cost: backend-neutral state update only.
GPU memory cost: none.

## Tests And Verification

Unit tests:
- `tests/unit/render_projection_input_tests.cpp`
- `tests/unit/render_camera_frame_tests.cpp`

Smoke tests:
- none for this file; covered by unit tests

CTest labels:
```text
iggy3d;render
```

Expected pass behavior: required receipt fields are present and the file owns only the declared responsibility.
Expected skip behavior: optional Vulkan lanes may skip only before required Vulkan work begins and must print `result=skip` plus `reason_code`.
Expected fail behavior: strict lanes fail on missing required dependency, validation error, runtime mutation, or boundary leak.

Firewall scan:
```sh
rg -n '#include[ <"]vulkan/|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
```

Expected firewall scan result:
```text
no matches
```

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not add Vulkan or SDL includes to backend-neutral files.
- Do not hide runtime mutation inside frame validation or diagnostics.

## Completion Criteria

- File `src/render/FrameInput.hpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.

## Packet 1 Detailed Contract

Header role: define the complete backend-neutral frame payload for Packet 1. Vulkan later consumes this data, but no Vulkan vocabulary appears here.

Required type shape:
```cpp
enum class RenderCameraMode : std::uint8_t {
  FirstPerson,
  ThirdPerson,
  TacticalOverhead,
};

struct RenderViewport {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  float aspectRatio = 1.0F;
};

struct RenderFrameClock {
  std::uint64_t sourceTick = 0;
  std::uint64_t frameIndex = 0;
  float interpolationAlpha = 0.0F;
  float presentationDeltaSeconds = 0.0F;
};

struct RenderCameraFrame {
  RenderCameraMode mode = RenderCameraMode::ThirdPerson;
  Vec3 worldEye;
  Vec3 worldForward;
  Vec3 worldUp;
  Mat4 viewFromWorld;
  Mat4 clipFromView;
  Mat4 clipFromWorld;
  float nearPlane = 0.1F;
  float farPlane = 200.0F;
};

struct RenderSceneFrame {
  const SceneProjectionResult* scene = nullptr;
  const DebugProjectionResult* debug = nullptr;
};

struct FrameInput {
  RenderViewport viewport;
  RenderFrameClock clock;
  RenderCameraFrame camera;
  RenderSceneFrame projections;
};
```

Required validation API:
```cpp
enum class FrameInputStatus : std::uint8_t {
  Valid,
  NotDrawable,
  MissingSceneProjection,
  InvalidAspectRatio,
  InvalidClock,
  InvalidCameraMode,
  InvalidCameraBasis,
  InvalidCameraMatrix,
  InvalidClipPlanes,
};

FrameInputStatus validateFrameInput(const FrameInput& frame);
std::string_view frameInputReasonCode(FrameInputStatus status);
```

Validation rules:
- `width == 0` or `height == 0` returns `NotDrawable`, not a Vulkan error.
- `aspectRatio` must equal `width / height` within a fixed tolerance of `0.001F`.
- `interpolationAlpha` must be in `[0.0F, 1.0F]`.
- `presentationDeltaSeconds` must be finite and non-negative.
- all camera vectors and matrices must be finite.
- `nearPlane > 0.0F`.
- `farPlane > nearPlane`.
- scene projection pointer is required for first-room visual proof.
- debug projection pointer may be null.

Required reason codes:
```text
frame_input_valid
frame_not_drawable
frame_scene_missing
frame_aspect_invalid
frame_clock_invalid
frame_camera_mode_invalid
frame_camera_basis_invalid
frame_camera_matrix_invalid
frame_clip_planes_invalid
```

Lifetime rule:
- `FrameInput` borrows projection results for one `submitFrame` call.
- The renderer must not store `SceneProjectionResult*` or `DebugProjectionResult*` after `submitFrame` returns.

Packet 1 acceptance:
```sh
ctest --test-dir build --output-on-failure -R 'render_projection_input|render_camera_frame'
rg -n '#include[ <"](SDL3/|SDL\.h|SDL_vulkan|vulkan/)|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/render/FrameInput.hpp
```

Expected scan result:
```text
no matches
```

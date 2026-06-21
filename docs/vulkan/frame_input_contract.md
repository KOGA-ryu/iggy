# Frame Input Contract

This document defines the backend-neutral data the renderer may consume. It is the renderer/runtime treaty: runtime and projection decide what the frame means; the renderer decides how to draw it.

This is the first careful detail pass. It is detailed enough for a builder to create a file plan for `src/render/FrameInput.hpp`, but it still leaves implementation choices such as exact culling and material growth to later renderer phases.

## Local File Target

Primary future file:

```text
src/render/FrameInput.hpp
```

Expected direct consumers:

- `src/render/RenderBackend.hpp`
- `src/render/null/NullRenderer.cpp`
- `src/render/vulkan/VulkanBackend.cpp`
- `tests/unit/render_projection_input_tests.cpp`
- `tests/unit/render_replay_invariance_tests.cpp`

Allowed input producers:

- visual app frame assembly code;
- projection adapter code;
- tests that construct deterministic frame inputs.

Forbidden producers:

- Vulkan backend internals reaching into runtime directly;
- save/load code;
- package validation code;
- command admission code.

## Non-Authority Rule

`FrameInput` is derived presentation data. It is not durable truth.

It must not:

- own gameplay state;
- own command legality;
- own save/load state;
- own replay state;
- own package validation state;
- own raw input events;
- own GPU handles;
- alter deterministic state hash;
- mutate runtime state directly or indirectly.

It may:

- copy or reference projection output for the duration of one frame;
- carry camera matrices derived from `CameraState`;
- carry viewport dimensions;
- carry non-authoritative render timing;
- carry debug projection items;
- carry stable ids so renderer diagnostics and picking can refer back to runtime-owned identities through later command paths.

## Source Contracts Pulled Forward

The contract must stay consistent with existing file plans:

- `Vec3`: X right, Y up, Z forward.
- `Transform3`: position, Euler rotation in radians, scale; rotation components mean `x=pitch`, `y=yaw`, `z=roll`.
- `Mat4`: row-major storage, element index `row * 4 + column`.
- `Mat4` multiplication: column vectors, `result = matrix * vector`.
- `Mat4` composition: `A * B` applies `B` first, then `A`.
- `SceneItem`: stable entity id, kind, transform, bounds, visibility/debug flags, inert asset reference.
- `SceneProjectionResult`: stable item order and source tick/hash copied from `SessionState`.
- `DebugProjectionResult`: stable debug item order and source tick/hash.
- `CameraState`: semantic camera truth only; no GPU resources or raw input.

## Include Rules

`src/render/FrameInput.hpp` may include:

```cpp
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneItem.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "runtime/camera/CameraState.hpp"
```

The exact include set can be reduced by forward declarations if the C++ shape allows it, but the dependency meaning must stay the same.

Forbidden includes:

```cpp
#include <vulkan/vulkan.h>
```

Also forbidden:

- Vulkan SDK headers;
- platform-window headers;
- content loaders;
- package validators;
- save/load codecs;
- command admission internals;
- runtime session mutation APIs;
- renderer backend internals.

## Coordinate Contract

World axes:

- X: right;
- Y: up;
- Z: forward.

World unit:

- one world unit is one gameplay unit;
- first-room renderer plans may treat one unit as one meter for camera/projection tuning;
- gameplay distance truth remains runtime-owned and must not be reinterpreted by the renderer.

Transform convention:

- `SceneItem.transform.position` is world position;
- `SceneItem.transform.rotationEulerRadians` uses `x=pitch`, `y=yaw`, `z=roll`;
- `SceneItem.transform.scale` is presentation scale unless the owning runtime type explicitly uses it for gameplay truth;
- renderer may derive model matrices from `Transform3`, but must not write derived matrices back into runtime.

Matrix convention:

- all matrices carried by `FrameInput` use `Mat4`;
- storage is row-major;
- math uses column-vector multiplication;
- `clipFromWorld = clipFromView * viewFromWorld`;
- non-finite matrices are invalid and must cause frame rejection or diagnostic failure, not undefined rendering.

Clip-space convention:

- `FrameInput` should carry renderer-ready clip matrices using Vulkan-compatible depth range: near maps to `0`, far maps to `1`;
- X and Y normalized device coordinates are `[-1, 1]`;
- the renderer backend owns any API-specific viewport/scissor inversion or platform correction;
- if a future non-Vulkan backend is added, it must adapt from this contract rather than changing runtime/projection truth.

Near/far rules:

- near plane must be finite and greater than zero;
- far plane must be finite and greater than near plane;
- first detailed file plan should define named defaults rather than magic numbers in app code;
- tactical camera may use a farther plane than realtime camera, but the values remain frame presentation data, not save truth unless later promoted explicitly by runtime camera policy.

## Proposed Type Shape

The first implementation should keep `FrameInput` small, explicit, and value-oriented.

Recommended namespace:

```cpp
namespace iggy3d {
}
```

Recommended initial declarations:

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
  float nearPlane = 0.0F;
  float farPlane = 0.0F;
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

The exact C++ may change during file planning, but the ownership must not: `FrameInput` aggregates derived render inputs and never gives the renderer authority over runtime state.

## Viewport Contract

`RenderViewport` owns presentation dimensions for one frame.

Rules:

- `width` and `height` are framebuffer pixel dimensions, not logical UI points;
- `aspectRatio` is `width / height` when height is nonzero;
- zero width or zero height means the frame is not drawable and the renderer should skip presentation or enter minimized handling;
- resize events are reported by the app/platform shell, but swapchain recreation is renderer-owned;
- runtime camera truth must not depend on swapchain dimensions.

Needs later file-plan detail:

- whether aspect ratio is recomputed by constructor/helper or trusted as input;
- exact invalid-viewport diagnostic;
- minimized-window behavior for each platform shell.

## Frame Clock Contract

`RenderFrameClock` is presentation metadata, not simulation authority.

Fields:

- `sourceTick`: runtime tick used to build the projection;
- `frameIndex`: renderer/app presentation frame count;
- `interpolationAlpha`: optional visual interpolation between fixed runtime states;
- `presentationDeltaSeconds`: wall/display delta for visual smoothing, animation, or diagnostics only.

Rules:

- `sourceTick` may be copied from `SceneProjectionResult`/`DebugProjectionResult`;
- `frameIndex`, `interpolationAlpha`, and `presentationDeltaSeconds` are excluded from save truth and state hash;
- renderer must not use `presentationDeltaSeconds` to advance gameplay;
- if interpolation is not implemented, `interpolationAlpha` must be `0.0F`;
- `interpolationAlpha` must stay in `[0, 1]`.

Needs later file-plan detail:

- whether animation systems exist in renderer before gameplay animation exists;
- whether frame index is app-owned or renderer-owned;
- exact validation helper for clock fields.

## Camera Contract

Runtime camera truth lives in `CameraState`. `RenderCameraFrame` is derived from it for drawing.

Mode mapping:

| Runtime `CameraMode` | Render `RenderCameraMode` | Meaning |
| --- | --- | --- |
| `FirstPerson` | `FirstPerson` | realtime eye near/at the controlled actor |
| `ThirdPerson` | `ThirdPerson` | realtime follow/orbit view |
| `TacticalOverhead` | `TacticalOverhead` | slow-time tactical view |

Rules:

- renderer consumes `RenderCameraFrame`; it does not consume mutable `CameraState`;
- runtime policy owns entering/leaving tactical mode and `previousRealtimeMode`;
- renderer may use camera-specific near/far defaults only through derived frame data supplied by the app/projection assembly layer;
- renderer cannot switch camera modes;
- renderer cannot clear `inputClearRequested`;
- raw mouse/controller input never enters `FrameInput`.

Required matrix semantics:

- `viewFromWorld` transforms world coordinates into camera/view space;
- `clipFromView` transforms camera/view coordinates into clip space;
- `clipFromWorld` must equal `clipFromView * viewFromWorld` within math-test tolerance;
- all three matrices must be finite;
- `worldForward` and `worldUp` must be finite and nonzero after validation.

Needs later file-plan detail:

- exact camera derivation functions and owner file;
- first-person eye offset;
- third-person orbit offset/collision policy;
- tactical overhead height/angle;
- projection FOV defaults;
- near/far defaults for realtime and tactical modes.

## Scene Projection Contract

`SceneProjectionResult` is the only scene draw source for the renderer.

Rules:

- one renderable item comes from one `SceneItem`;
- item order is the stable order emitted by `SceneProjection`;
- renderer must not sort in a way that changes debug/replay diagnostics unless the sort is explicitly a backend-private draw optimization with stable diagnostics preserved;
- inactive/invisible items are skipped unless the projection config intentionally includes them;
- asset references are inert ids, not file paths, loaded meshes, or renderer handles;
- missing asset references must produce renderer fallback visuals and diagnostics, not runtime mutation.

Scene item fields the renderer may consume:

- stable runtime entity id;
- `SceneItemKind`;
- world transform;
- world bounds;
- active/visible flags;
- inert asset reference;
- optional item/objective/interaction ids;
- optional owning player slot.

Scene item fields the renderer may not invent:

- gameplay reach;
- objective completion;
- inventory ownership;
- command legality;
- collision truth;
- save/load truth.

Needs later file-plan detail:

- exact renderable item id alias;
- mapping from `SceneItemKind` to fallback mesh/material;
- draw ordering for opaque/debug/transparent phases;
- culling owner and first culling policy;
- asset reference format and missing-asset diagnostic.

## Debug Projection Contract

`DebugProjectionResult` is the only debug draw source for renderer-owned debug visualization.

Rules:

- debug items explain runtime state; they do not change it;
- labels are stable ids/codes, not logic-driving localized prose;
- debug item order remains deterministic;
- debug rendering can be disabled by app/build flag, but disabling it must not change runtime hash or projection output;
- debug draw failure must not fail gameplay unless the app is explicitly running a debug-render acceptance test.

Allowed debug item shapes for first renderer plans:

- point marker;
- line/ray;
- radius/sphere approximation;
- bounds/box;
- short stable label code routed to diagnostics or later text rendering.

Color contract:

- first pass should use linear float RGBA or fixed packed RGBA consistently;
- color is presentation data only;
- color must not be parsed by runtime logic.

Needs later file-plan detail:

- exact debug primitive struct if `DebugProjectionItem` is too semantic for renderer use;
- text rendering policy;
- color type;
- debug enable flag owner;
- max debug item budget.

## Copy vs Reference Policy

Recommended first implementation: `FrameInput` references projection results for one frame and copies small camera/viewport/clock values.

Rationale:

- `SceneProjectionResult` and `DebugProjectionResult` may contain vectors;
- copying large vectors every frame is unnecessary for the first renderer;
- the app/frame assembly owner can keep projection results alive until `RenderBackend::renderFrame` returns;
- the renderer must not retain raw projection pointers after the frame.

Rules:

- `FrameInput` may hold `const SceneProjectionResult*`;
- `FrameInput` may hold `const DebugProjectionResult*`;
- `FrameInput` must copy `RenderViewport`;
- `FrameInput` must copy `RenderFrameClock`;
- `FrameInput` must copy `RenderCameraFrame`;
- renderer backend must not store projection pointers past the call boundary;
- if asynchronous renderer work later requires retention, introduce an explicit immutable frame snapshot type rather than silently extending pointer lifetime.

Needs later file-plan detail:

- exact render API call signature;
- whether `FrameInput` can be default constructed;
- validation helper such as `validateFrameInput`;
- null projection behavior for device-only smoke tests.

## Threading And Lifetime

First renderer plans assume single-threaded frame assembly and render submission.

Rules:

- projection result lifetime must cover the full render call;
- renderer may copy data into GPU upload buffers during the call;
- renderer may not retain references to runtime/projection containers after the call;
- runtime cannot mutate the source `SessionState` while projection is being built;
- future multithreading must introduce explicit snapshots and ownership transfer.

Needs later file-plan detail:

- snapshot type if render thread is introduced;
- lock-free or copy queue policy if needed;
- frame allocator policy;
- teardown/wait-idle interaction.

## Validation Rules

`FrameInput` should have a validation helper before Vulkan consumes it.

Recommended future signature:

```cpp
struct FrameInputValidation {
  bool ok = false;
  std::string code;
};

FrameInputValidation validateFrameInput(const FrameInput& input);
```

Minimum validation:

- viewport width/height are either both nonzero or frame is marked not drawable;
- aspect ratio is finite and positive when drawable;
- camera matrices are finite;
- `clipFromWorld` equals `clipFromView * viewFromWorld` within tolerance;
- near/far are finite and ordered;
- `interpolationAlpha` is finite and in `[0, 1]`;
- scene/debug pointers are present for visual frames or explicitly absent for device-only smoke tests;
- projection source tick matches clock source tick when both projections are present;
- projection source hash matches between scene/debug when both carry hash and came from the same runtime state.

Failure behavior:

- null renderer can record validation failure diagnostics;
- Vulkan renderer must skip drawing invalid frames rather than using undefined data;
- validation failures must be machine-readable;
- validation failure must not mutate runtime state.

Needs later file-plan detail:

- exact error code enum;
- whether validation returns `Diagnostic`;
- tolerance constants;
- minimized-frame representation.

## Replay And Hash Invariance

Renderer presence must not affect deterministic simulation.

Required test shape:

1. Load the same first-room fixture.
2. Run a deterministic command script with renderer disabled.
3. Record final runtime summary and state hash.
4. Run the same script with null renderer consuming `FrameInput`.
5. Record final runtime summary and state hash.
6. Run the same script with Vulkan renderer when GPU smoke tests are enabled.
7. Compare runtime summary and state hash across all enabled lanes.

Expected result:

- runtime summary matches;
- state hash matches;
- command acceptance/rejection matches;
- save/load result matches;
- replay result matches;
- renderer diagnostics may differ by backend/platform, but cannot change runtime truth.

Failure diagnostics should report:

- backend name;
- source tick;
- command index if known;
- expected hash;
- actual hash;
- whether frame input validation failed;
- whether renderer was enabled, disabled, null, or Vulkan.

## First-Pass File Plan Checklist

When creating the file plan for `src/render/FrameInput.hpp`, include:

- exact includes;
- namespace;
- all public structs/enums;
- validation helper decision;
- copy/reference lifetime rule;
- null projection behavior;
- minimized viewport behavior;
- no Vulkan include scan;
- replay invariance test names;
- completion criteria tied to headless acceptance.

## Open Detail Items

These are intentionally left for the next document-specific file plans:

- exact camera matrix derivation owner;
- exact first-person/third-person/tactical camera constants;
- exact debug primitive C++ shape;
- exact renderable material/fallback mapping;
- exact validation error type;
- exact culling/sorting policy;
- exact thread-snapshot policy if renderer moves off the main thread.

# First Room Render Contract

This document defines what "the first room renders" means for the first Vulkan visual proof.

The first room render is not final art, not a full asset system, not lighting, not UI, not editor integration, and not multiplayer visualization. It is the first visible proof that runtime truth can flow through projection into `FrameInput` and be presented by the Vulkan backend without changing deterministic runtime behavior.

## Position In The Roadmap

Prerequisites:

- headless runtime acceptance is green;
- renderer boundary exists;
- `FrameInput` contract exists;
- platform shell can create a window/surface;
- Vulkan instance/device/swapchain/command/sync path exists;
- first shader pipeline can be created;
- first-room bootstrap resources can be created.

Likely roadmap phase:

- Phase 10: First visual demo app.

Related docs:

- [frame_input_contract.md](frame_input_contract.md)
- [shader_pipeline.md](shader_pipeline.md)
- [resource_model.md](resource_model.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [platform_matrix.md](platform_matrix.md)

## Input Source

The first visual proof uses the same fixture family as headless acceptance:

```text
fixtures/demos/first_room
```

Data flow:

```text
fixture -> content validation -> runtime session -> SceneProjection/DebugProjection -> FrameInput -> Vulkan renderer
```

Rules:

- runtime loads and validates the fixture through public APIs;
- runtime owns command legality, camera mode truth, save/load, replay, and state hash;
- runtime owns combat hit points, defeated truth, combat events, and `training_dummy` acceptance state;
- projection owns backend-neutral scene/debug output;
- renderer consumes `FrameInput`;
- renderer does not read fixture files directly;
- renderer does not validate package truth;
- renderer does not mutate runtime state.

## Visible Success Criteria

A strict first-room pass requires:

- a visible room/floor/wall proxy or diagnosed room fallback;
- a visible player/player marker;
- at least one visible pickup, interactable, objective marker, or diagnosed absence if the fixture/projection has none;
- deterministic initial camera view;
- non-background geometry visible in the frame;
- depth testing visibly enabled or diagnosed by depth resource/pipeline receipt;
- clear color distinguishable from room/player geometry;
- renderer diagnostics receipt printed;
- replay hash unchanged by rendering.

Minimum visual read:

```text
I can tell there is a space, a player position, and at least one gameplay-relevant projected marker or a diagnosed reason it is absent.
```

## First Geometry Policy

Use renderer-owned proxy geometry for the first room.

Reason:

- the first proof is projection-to-renderer flow;
- asset/model loading belongs later;
- proxy geometry keeps the GPU path small and debuggable;
- fallback/proxy use is acceptable only if diagnosed.

Initial proxy mapping:

| Projection input | First visual proxy | Required? | Notes |
| --- | --- | --- | --- |
| `SceneItemKind::Player` | small upright box/capsule-like proxy | yes | color should differ from room |
| `SceneItemKind::Pickup` | small cube | if projected | use stable fallback color |
| `SceneItemKind::Interactable` | highlighted box | if projected | should be distinguishable |
| `SceneItemKind::ObjectiveMarker` | marker cube/diamond/column | if projected | no special shader required |
| `SceneItemKind::TacticalMarker` | flat marker or small column | optional | useful for tactical camera proof |
| `SceneItemKind::DebugOnly` | debug color proxy | optional | only when debug projection enabled |
| room/floor/wall projection | floor/wall block proxy | preferred | if runtime projection emits room geometry |

If runtime/projection does not yet emit room/floor/wall structure, the renderer may draw a temporary first-room debug floor/grid as a diagnosed fallback:

```text
fallback_room_proxy=true
fallback_reason=no_projected_room_geometry
```

That fallback is temporary. It must not become content truth.

## Packet 8 Combat Visual Mapping Gap

Runtime Packet 8 adds `training_dummy` to the first-room acceptance state and proves combat through `CommandKind::Attack`, `TargetAction::Attack`, `CombatSystem`, combat events, combat save/load/replay/hash preservation, and final summary fields such as defeated combat state.

This renderer document does not make `training_dummy` or defeated-state rendering a Packet 1 requirement. Packet 1 is the backend-neutral renderer boundary and must not add combat rendering, combat assets, combat projection semantics, or gameplay logic.

Before a later visual packet requires combat display, the projection/visual contract must choose one explicit mapping:

- project combatants through an existing kind such as `SceneItemKind::Interactable`;
- project combat/debug state through `SceneItemKind::DebugOnly`;
- add a new explicit projected kind for combatants or defeated combatants;
- define another backend-neutral visual kind with the same ownership guarantees.

Rules for that later decision:

- renderer consumes projected combat state only;
- runtime remains authority for hit points, defeated truth, combat events, save/load, replay, and deterministic hash;
- renderer diagnostics may report visible combat proxies, but those diagnostics do not become gameplay truth;
- missing combat visual mapping cannot block Packet 1, Packet 2, or Packet 3.

## First Material Policy

First-room rendering uses vertex color only.

Required:

- no textures;
- no lighting;
- no material files;
- no descriptor sets;
- one matrix push constant: `clipFromModel`;
- deterministic fallback colors by projected item kind.

Suggested fallback colors:

| Purpose | Color intent |
| --- | --- |
| floor | muted neutral |
| wall/block | darker neutral |
| player | blue/green |
| pickup | gold/yellow |
| interactable | cyan |
| objective | magenta/red |
| debug-only | white or high-contrast debug color |
| missing resource | magenta/error color |

Exact RGB values belong in the first-room file plan. The rule here is that colors are presentation diagnostics only and never gameplay truth.

## Camera Contract

The first room uses `FrameInput.camera`.

Rules:

- initial camera mode follows runtime `CameraState`;
- expected initial acceptance mode is third person unless runtime says otherwise;
- renderer consumes `RenderCameraFrame`;
- renderer cannot switch camera modes;
- renderer cannot clear camera input flags;
- renderer cannot advance gameplay from presentation delta.

First visual camera acceptance:

- player marker is visible at startup;
- room proxy is visible at startup;
- near/far planes are finite and diagnosed;
- `clipFromWorld` is finite and valid;
- tactical camera can be tested later without changing first-room proof.

Open file-plan details:

- exact FOV;
- exact third-person eye/target derivation;
- exact first-person eye offset;
- exact tactical overhead height/angle;
- exact near/far defaults.

## First-Room Shader And Pipeline

Required shader/pipeline baseline:

```text
shader_language=glsl
pipeline_family=first_room
pipeline_layout=push_constants_only
rendering_path=dynamic
topology=triangle_list
depth_test=enabled
depth_write=enabled
blend=disabled
```

Allowed fallback:

- `rendering_path=render_pass` only if dynamic rendering fails a required platform gate and the fallback is recorded in `decisions.md`.

Required vertex inputs:

- position;
- color.

Required push constant:

- `mat4 clipFromModel`.

Forbidden in first-room shader:

- texture sampling;
- lighting;
- skeletal animation;
- material descriptor sets;
- gameplay branching;
- runtime command logic.

## First-Room Resource Contract

Required resources:

- vertex buffer;
- index buffer if geometry uses indices;
- depth image;
- shader modules;
- first-room graphics pipeline;
- command buffers and frame sync from earlier phases.

Allowed allocator state:

- `memory_allocator=manual_bootstrap` for first-room bootstrap;
- `memory_allocator=vma` if VMA is already adopted.

Forbidden:

- per-frame resource allocation in steady state;
- texture/material upload;
- renderer-owned package validation;
- runtime-visible GPU handles.

Required diagnostics:

```text
memory_allocator=manual_bootstrap|vma
vertex_buffer_count=
index_buffer_count=
depth_format=
depth_extent=
per_frame_allocation_count=0
```

## Draw Count Expectations

Minimum draw contract:

- one draw for room/floor/wall proxy or diagnosed fallback;
- one draw for player proxy;
- additional draws for projected pickup/interactable/objective/debug markers when present.

Required diagnostics:

```text
scene_item_count=
debug_item_count=
draw_count=
fallback_draw_count=
missing_resource_count=
first_room_visible=true|false
fallback_room_proxy=true|false
```

Rules:

- `draw_count` must be greater than zero for a pass;
- `first_room_visible=true` requires non-background geometry drawn;
- `missing_resource_count` may be nonzero only if fallback rendering is visible and diagnosed;
- draw order must be stable enough for diagnostics and repeatable smoke results.

## Smoke Receipt

`vulkan_first_room_smoke` must print a key-value receipt compatible with [diagnostics_and_tests.md](diagnostics_and_tests.md).

Minimum required fields:

```text
receipt_version=1
backend=vulkan
test_name=vulkan_first_room_smoke
result=pass|fail|skip
reason_code=
platform=
platform_lane=
frame_input_valid=true|false
source_tick=
scene_item_count=
debug_item_count=
draw_count=
fallback_draw_count=
missing_resource_count=
first_room_visible=true|false
fallback_room_proxy=true|false
rendering_path=dynamic|render_pass
shader_language=glsl
pipeline_family=first_room
memory_allocator=manual_bootstrap|vma
depth_format=
depth_extent=
validation=enabled|disabled|unavailable
sync_validation=enabled|disabled|unavailable
replay_invariant=true|false|unavailable
```

Strict pass requires:

- `result=pass`;
- `frame_input_valid=true`;
- `first_room_visible=true`;
- `draw_count` greater than zero;
- `replay_invariant=true`;
- validation clean in strict validation lane.

## Screenshot And Frame Hash

Screenshot is optional in the first pass.

If implemented, write:

```text
build/artifacts/render_diagnostics/vulkan_first_room_smoke/screenshot.ppm
```

Frame hash is optional and diagnostic only.

If implemented, write:

```text
build/artifacts/render_diagnostics/vulkan_first_room_smoke/frame_hash.txt
```

Rules:

- screenshot/frame hash must not become gameplay truth;
- screenshot differences do not affect replay hash;
- screenshot is useful for humans, not required until the first visual acceptance doc says so;
- frame hash may be unstable across GPUs and should not be a cross-platform strict gate without evidence.

## Failure Policy

Expected failures and reason codes:

| Failure | Reason code | Optional lane | Strict lane |
| --- | --- | --- | --- |
| Vulkan smoke disabled | `vulkan_smoke_disabled` | skip | fail if explicitly required |
| No display/window server | `no_display` or `no_window_server` | skip | fail |
| No Vulkan loader/device | `no_vulkan_loader` or `no_vulkan_device` | skip | fail |
| Shader missing | `shader_missing` | fail | fail |
| Pipeline creation failed | `pipeline_create_failed` | fail | fail |
| Depth format unsupported | `depth_format_unsupported` | fail | fail |
| Frame input invalid | `frame_input_invalid` | fail | fail |
| No drawable geometry | `no_first_room_geometry` | fail unless diagnosed fallback allowed | fail unless fallback accepted |
| Replay hash changed | `replay_hash_changed` | fail | fail |
| Validation error | `validation_error` | fail in validation lane | fail |

Rules:

- fallback room proxy is allowed only with `fallback_room_proxy=true`;
- shader/pipeline/depth failures are not runtime failures;
- renderer failure cannot mutate runtime state;
- strict lane cannot hide missing platform proof behind skip.

## Out Of Scope

Not part of first-room render:

- texture loading;
- material files;
- lighting;
- shadows;
- animation;
- skeletal meshes;
- real model loading;
- editor UI;
- HUD/UI text;
- picking;
- multiplayer-specific visuals;
- RenderDoc automation;
- shipping package polish.

## Tests

Primary smoke:

```text
tests/smoke/vulkan_first_room_smoke.cpp
```

Related tests:

```text
tests/unit/render_projection_input_tests.cpp
tests/unit/render_replay_invariance_tests.cpp
tests/smoke/vulkan_pipeline_smoke.cpp
tests/smoke/vulkan_memory_smoke.cpp
tests/smoke/vulkan_diagnostics_smoke.cpp
```

Command:

```sh
ctest --test-dir build --output-on-failure -R 'vulkan_first_room'
```

Strict command:

```sh
cmake -S . -B build -DIGGY3D_ENABLE_VISUAL_DEMO=ON -DIGGY3D_ENABLE_VULKAN=ON -DIGGY3D_ENABLE_SHADER_COMPILE=ON -DIGGY3D_ENABLE_VULKAN_SMOKE=ON -DIGGY3D_REQUIRE_VULKAN_SMOKE=ON
cmake --build build
ctest --test-dir build --output-on-failure -R 'vulkan_first_room|render_replay'
```

## Acceptance Criteria

The first-room render contract is satisfied when:

- headless runtime acceptance remains green;
- visual demo or smoke loads the first-room fixture through public APIs;
- projection builds backend-neutral scene/debug output;
- `FrameInput` validates;
- Vulkan draws visible non-background geometry;
- player proxy is visible;
- room/floor/wall proxy or diagnosed fallback is visible;
- depth is enabled and diagnosed;
- first-room shader/pipeline diagnostics are printed;
- resource diagnostics are printed;
- renderer receipt is written/printed;
- replay hash is unchanged by rendering.

## Open Detail Items

These belong in future file plans:

- exact fallback geometry vertices/indices;
- exact fallback colors;
- exact first camera constants;
- exact geometry source if room projection adds structural items;
- exact Packet 8 combat visual mapping for `training_dummy` and defeated state;
- exact screenshot policy;
- exact frame hash policy;
- exact draw ordering for debug/transparent overlays;
- exact pass/fail parser for `first_room_visible`.

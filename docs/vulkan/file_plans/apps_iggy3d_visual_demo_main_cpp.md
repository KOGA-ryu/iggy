# apps/iggy3d_visual_demo/main.cpp

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `apps/iggy3d_visual_demo/main.cpp`

Purpose: Wire runtime, projection, platform shell, renderer config, and renderer backend into the first visual demo executable.

Packet boundary note: this is Packet 3 work. Packet 1 must not create this app, SDL event handling, window code, interactive input mapping, combat visuals, shader loading, Vulkan backend startup, screenshot capture, or frame hashing.

## Build Position

Packet order: 3 - Visual app and SDL platform shell
Owner module: `visual app`
File kind: `app`
Current-build contract: this file plan is authoritative for later implementation packets, but it is not a signal to write renderer C++ before the headless runtime and projection gates are green.

Source docs read for this plan:
- `docs/vulkan/README.md`
- `docs/vulkan/renderer_file_plan_order.md`
- `docs/vulkan/vulkan_first_file_plans_index.md`
- `docs/vulkan/renderer_packet_template.md`
- `docs/vulkan/file_surface.md`
- `docs/vulkan/boundaries.md`
- `docs/vulkan/frame_input_contract.md`
- `docs/vulkan/diagnostics_and_tests.md`
- `docs/vulkan/platform_shell.md`
- `docs/vulkan/vulkan_renderer_config.md`
- `docs/vulkan/vulkan_package_runtime_lookup.md`

## Ownership

This file owns:
- Wire runtime, projection, platform shell, renderer config, and renderer backend into the first visual demo executable.
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

Allowed includes: standard library, app config helpers, diagnostics, SDL3 headers for SDL window files, and platform OS path helpers.

Forbidden includes: Vulkan headers except through `SdlVulkanSurface.*`, runtime mutation internals, content validators, save/load internals, and old repo headers.

Vulkan headers are forbidden in this file unless the exact path is `SdlVulkanSurface.*`.

SDL headers are allowed only in SDL platform implementation files and not in runtime/content/projection/save.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `main`
- `visual demo CLI flags`
- `one-frame and fixed-frame smoke modes`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

App/platform code owns window and path lookup objects. The renderer owns GPU resources. Runtime owns gameplay state. Platform events may be translated into app-level requests, but they do not mutate runtime state directly. Diagnostics receipts are emitted by app or render diagnostics helpers.

Runtime Packet 8 combat note: `training_dummy`, hit points, defeated truth, attack commands, combat events, and combat save/replay/hash behavior belong to runtime. This visual app may display combat state only after projection exposes backend-neutral scene or debug items for it; Packet 3 visual boot does not invent that mapping.

## Semantics

Normal path: resolve platform paths, window state, events, or diagnostics roots without touching runtime truth. Skip/fail: visual-only work may skip in optional graphics lanes and must fail in strict package lanes when required paths are missing.

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
file_plan=apps/iggy3d_visual_demo/main.cpp
packet_order=3
allowed_to_implement_code_now=false
window_shell=sdl3|none|unavailable
surface_provider=sdl3|none|unavailable
drawable=true|false
platform_required_instance_extensions=
resource_root=
shader_root=
diagnostics_dir=
reason_code=
```

User-facing error message shape when this file contributes to visual startup failure:
```text
This machine cannot run the Vulkan visual renderer required by this build.
Reason: <specific renderer or platform reason>.
Action: run the headless runtime demo or use a machine/runtime that satisfies the Vulkan baseline.
```

## Fallback Policy

Fallback policy: platform lookup may use documented path candidates. Runtime/content/projection behavior never changes because a graphics path is missing.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=visual_app
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: one SDL/window/path lookup setup and platform extension query.
Per-frame cost: event polling and drawable-size query only.
Resize cost: event bookkeeping; Vulkan swapchain recreate is owned elsewhere.
GPU memory cost: none in the platform shell.

## Tests And Verification

Unit tests:
- none for this file; covered by smoke or build tests

Smoke tests:
- `tests/smoke/package_visual_startup_smoke.cpp`
- `tests/smoke/vulkan_first_room_smoke.cpp`

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

## Packet 3 Detailed Contract

Current-build API and file shape:
- Entry point: `int main(int argc, char** argv)`.
- Parse only visual-demo flags and pass renderer settings through `RendererConfig`.
- Accepted flags for the first visual app packet:
  - `--renderer=null|vulkan|auto`
  - `--require-renderer`
  - `--frames <count>`
  - `--strict-vulkan`
  - `--validation=off|optional|required`
  - `--sync-validation=off|optional|required`
  - `--shader-root <path>`
  - `--diagnostics-dir <path>`
  - `--print-render-receipt`
- The app may add demo-only camera seed values through `FrameInput`, but runtime camera truth remains in runtime-owned state.
- The first visual boot may run scripted/fixed-frame only. Interactive SDL keyboard, mouse, or gamepad mapping is a separate Packet 3 acceptance choice, not part of Packet 1.
- Packet 3 must not require Packet 8 combat visual mapping; if combat items are absent from projection, the app reports projected counts and continues according to the selected smoke lane.

Startup sequence:
1. Parse app flags into app-local settings.
2. Resolve package/resource/shader/diagnostics paths through `PackageRuntimeLookup`.
3. Create a demo runtime/session through public runtime entry points only.
4. Ask projection systems for backend-neutral `SceneProjection`, `DebugProjection`, and `CameraState` data.
5. Build `FrameInput` from projection output and app timing.
6. Create `RendererApi` with either `NullRenderer` or Vulkan backend.
7. Run a bounded frame loop.
8. Print a deterministic key-value receipt when requested.
9. Shut the renderer down before runtime teardown.

Ownership:
- Owns process startup ordering, app flag parsing, visual-demo frame limit, and final process exit code.
- Does not own Vulkan handles, SDL window handles, runtime save state, runtime command legality, projection internals, or renderer resource lifetimes.
- Does not write gameplay state, save files, replay files, or deterministic runtime hashes.

Result policy:
- `result=pass` with `reason_code=visual_demo_ok` when the frame loop completes the requested frames.
- `result=fail` with `reason_code=visual_demo_config_invalid` when flags conflict or values are malformed.
- `result=fail` with `reason_code=visual_demo_package_lookup_failed` when required package paths are missing.
- `result=skip` with `reason_code=visual_demo_renderer_unavailable` only in non-strict Vulkan lanes before the renderer is required.
- `result=fail` with `reason_code=visual_demo_frame_failed` when renderer creation succeeds but frame submission fails.

Receipt fields:
```text
app=iggy3d_visual_demo
package_mode=<headless|build_tree_visual|installed_visual>
renderer_request=<null|vulkan|auto>
backend=<null|vulkan|unavailable>
frame_limit=<integer>
frames_submitted=<integer>
shader_root=<absolute-path-or-empty>
diagnostics_dir=<absolute-path-or-empty>
validation=<off|optional|required>
sync_validation=<off|optional|required>
result=<pass|skip|fail>
reason_code=<stable-reason>
```

Platform behavior:
- macOS: package lookup must support app bundle/resource-dir layouts and MoltenVK runtime placement.
- Linux: package lookup must support executable-relative and install-prefix style resource roots.
- Windows: package lookup must support executable-directory resource roots and multi-config build output.
- Software Vulkan lanes may run the app in `--renderer=vulkan` only when the Vulkan backend reports a supported software adapter.

Compute cost:
- Startup may create one window, one renderer, and one runtime demo fixture.
- Per-frame work is one projection read, one `FrameInput` build, one renderer submit, and one receipt counter update.
- The app must not perform asset scanning, shader compilation, or package discovery per frame.
- Scripted/fixed-frame boot does not require interactive input processing beyond quit/shutdown handling.

Verification:
- `package_visual_startup_smoke` proves startup, path resolution, bounded frame loop, receipt fields, and clean shutdown.
- `vulkan_first_room_smoke` later proves the same app can drive the first visible Vulkan room after backend packets land.
- Unit coverage belongs in lower-level package, config, and boundary tests; this file should remain a thin orchestration point.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not rely on current working directory for installed package lookup.
- Do not let SDL event polling mutate runtime state directly.
- Do not treat `training_dummy` or defeated combat state as visible unless projection provides a backend-neutral item for it.
- Do not make interactive input mapping a hidden prerequisite for Packet 1.

## Completion Criteria

- File `apps/iggy3d_visual_demo/main.cpp` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.

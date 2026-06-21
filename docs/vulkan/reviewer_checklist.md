# Vulkan Reviewer Checklist

This is the pass/fail gate for Vulkan file plans and future implementation packets.

Use it as a review sheet. If a packet fails any required item, the packet is not ready.

## Review Result

Use one of:

```text
PASS
PASS_WITH_NOTES
FAIL
BLOCKED
```

Definitions:

- `PASS`: all required gates pass.
- `PASS_WITH_NOTES`: all required gates pass, but non-blocking follow-up exists.
- `FAIL`: required gate failed.
- `BLOCKED`: required evidence is missing and cannot be inferred.

## Required Context

Every reviewed packet must name:

```text
packet_name=
roadmap_phase=
files_touched=
docs_read=
tests_named=
fallbacks_used=true|false
platform_lanes_touched=
```

If these are missing, review result is `FAIL`.

## 1. Runtime Authority Boundary

Required checks:

- Runtime still owns gameplay truth.
- Runtime still owns command legality.
- Runtime still owns camera mode truth.
- Runtime still owns save/load truth.
- Runtime still owns replay determinism and state hash.
- Renderer does not mutate runtime state directly.
- Renderer does not perform command admission.
- Renderer does not validate package truth.

Fail if:

- renderer reads mutable runtime internals to decide gameplay;
- renderer writes to `SessionState`, save state, command log, or state hash;
- renderer turns presentation timing into simulation authority.

## 2. Projection Boundary

Required checks:

- Renderer consumes backend-neutral projection output.
- Projection does not include Vulkan headers.
- Projection does not include SDL/window headers.
- Projection does not own GPU resources.
- Draw decisions come from `SceneProjection`, `DebugProjection`, and `FrameInput`.

Fail if:

- projection depends on renderer backend;
- projection emits `Vk*`, `VK_*`, descriptor, shader, or swapchain concepts;
- renderer bypasses projection to discover scene truth.

## 3. Include Firewall

Run or require equivalent scan:

```sh
rg -n '#include[ <"](SDL3/|SDL\\.h|SDL_vulkan|vulkan/)|\\bVk[A-Z][A-Za-z0-9_]*|\\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
```

Expected result: no matches.

Public renderer headers must also stay clean unless explicitly reviewed:

```sh
rg -n '#include[ <"](SDL3/|SDL\\.h|SDL_vulkan|vulkan/)|\\bVk[A-Z][A-Za-z0-9_]*|\\bVK_[A-Z0-9_]+' src/render/RendererApi.hpp src/render/FrameInput.hpp src/render/RenderBackend.hpp src/render/RenderDiagnostics.hpp
```

Expected result: no matches.

Allowed exceptions:

- `src/render/vulkan/**`;
- `src/app/platform/SdlVulkanSurface.*`;
- Vulkan-specific app glue;
- Vulkan-specific smoke tests.

Any exception must be named in the packet.

## 4. FrameInput Gate

Required checks:

- `FrameInput` is backend-neutral.
- `FrameInput` contains derived presentation data only.
- `FrameInput` does not own save/replay truth.
- `FrameInput` does not contain Vulkan handles.
- `FrameInput` does not contain SDL events or raw input events.
- Invalid `FrameInput` is diagnosed before Vulkan draw.

Required validation topics:

- viewport drawable or minimized;
- finite camera matrices;
- near/far valid;
- projection pointers/lifetimes valid;
- source tick/hash consistency where available.

Fail if:

- renderer stores projection pointers past the frame without an explicit snapshot design;
- renderer mutates runtime through frame input;
- invalid frame data can reach command recording silently.

## 5. Camera Gate

Required checks:

- `CameraState` remains runtime truth.
- `RenderCameraFrame` is derived.
- Renderer cannot switch camera mode.
- Renderer cannot clear `inputClearRequested`.
- Aspect ratio comes from viewport/frame assembly, not runtime camera truth.
- Presentation interpolation, if present, cannot affect replay hash.

Required diagnostics when camera is used:

```text
camera_mode=
camera_fov_degrees=
camera_near=
camera_far=
camera_aspect_ratio=
```

Fail if:

- renderer writes camera state;
- window resize changes runtime camera truth;
- renderer advances gameplay camera from frame delta.

## 6. Platform Shell Gate

Required checks:

- SDL3 remains isolated to app/platform files.
- App owns event polling.
- Runtime command path remains explicit.
- Surface creation handoff is separate from swapchain/device ownership.
- Headless builds still work with visual/Vulkan options off.

Fail if:

- SDL event handling lives in Vulkan backend;
- runtime includes SDL headers;
- platform shell owns swapchain;
- visual app directly mutates runtime state outside command APIs.

## 7. Vulkan Backend Gate

Required checks:

- Vulkan handles stay inside Vulkan backend or approved Vulkan glue.
- Instance/device/surface ownership is explicit.
- Swapchain ownership is explicit.
- Command buffer ownership is explicit.
- Sync ownership is explicit.
- Destruction order is safe.

Required diagnostics when backend is touched:

```text
device_name=
api_version=
driver_version=
enabled_instance_extensions=
enabled_device_extensions=
validation=
sync_validation=
```

Fail if:

- raw Vulkan handles leak through generic renderer API;
- device/swapchain lifetime is ambiguous;
- validation policy is absent.

## 8. Fallback Gate

Required checks:

- Fallback is allowed by [fallbacks.md](fallbacks.md).
- Fallback is visible in diagnostics.
- Fallback has `fallback_reason` or stable `reason_code`.
- Owner docs are updated if fallback changes architecture/test/platform behavior.

Required fields when fallback is used:

```text
fallback_used=true
fallback_area=
fallback_from=
fallback_to=
fallback_reason=
```

Fail if:

- fallback is silent;
- fallback on one platform becomes global default without decision update;
- strict proof is skipped because fallback hid a required failure.

## 9. Diagnostics Gate

Required checks:

- Packet names required receipt fields.
- Packet uses existing reason codes or adds new ones to diagnostics docs.
- Receipt fields are backend-neutral outside Vulkan internals.
- Failure paths are machine-readable.
- Diagnostics do not affect replay hash.

Required for Vulkan smoke attempts:

```text
receipt_version=1
backend=vulkan
result=pass|fail|skip
reason_code=
platform=
platform_lane=
```

Fail if:

- failure is prose-only;
- no receipt is printed/written for smoke attempt;
- diagnostics expose raw Vulkan handles publicly.

## 10. Test Gate

Required checks:

- Unit tests are named.
- Smoke tests are named when GPU/platform behavior is touched.
- CTest labels are named.
- Optional vs strict Vulkan behavior is defined.
- Skip exit code 77 is used only in optional lanes.

Fail if:

- strict Vulkan lane can skip required proof;
- GPU-dependent tests become required for default headless runtime lane;
- test labels are missing for Vulkan/platform work.

## 11. Platform Matrix Gate

Required checks:

- macOS/MoltenVK impact is named.
- Linux native Vulkan impact is named.
- Windows native Vulkan impact is named.
- Any unsupported lane has explicit blocker/evidence.
- MoltenVK quirks do not become renderer authority.

Fail if:

- packet validates only local macOS but claims shippable renderer;
- Linux/Windows lanes are ignored for feature choice;
- software Vulkan/lavapipe is counted as shipping proof.

## 12. Shader/Pipeline Gate

Required checks:

- Shader language remains renderer/build concern.
- Runtime/content/projection/save do not mention shader language.
- SPIR-V generation/loading path is named.
- Pipeline diagnostics are named.
- First-room pipeline uses push constants only unless a later packet opens descriptors.

Fail if:

- shader compiler becomes runtime dependency;
- shader files are loaded by content/runtime;
- descriptors/materials sneak into first-room pipeline without resource model gate.

## 13. Resource/VMA Gate

Required checks:

- First-room bootstrap resources are narrow.
- VMA gate is respected before texture/material growth.
- Allocation names exist before resource growth.
- Per-frame allocation churn is forbidden or diagnosed.
- Depth format fallback is named when depth is touched.

Fail if:

- texture/material work starts before memory diagnostics;
- GPU handles leak to runtime/projection;
- anonymous allocations are accepted for growth path;
- per-frame allocations occur silently.

## 14. First-Room Gate

Required checks:

- First room uses `fixtures/demos/first_room`.
- Fixture flows through runtime and projection.
- Renderer does not read fixture directly.
- Player proxy is visible or failure is explicit.
- Room/floor/wall proxy or diagnosed fallback is visible.
- `draw_count > 0`.
- `first_room_visible=true` for pass.
- Replay hash unchanged.

Fail if:

- renderer invents content truth;
- first room passes with no projected gameplay item visible;
- fallback room proxy is silent;
- screenshot/frame hash becomes gameplay truth.

## 15. Packaging Gate

Required checks:

- Headless package remains graphics-free.
- Shader root is renderer/package concern.
- macOS/MoltenVK runtime expectations are named if packaging is touched.
- Linux native Vulkan runtime expectations are named if packaging is touched.
- Windows native Vulkan runtime expectations are named if packaging is touched.

Fail if:

- headless tools require SDL/Vulkan/shaders;
- runtime/content/projection/save load shader artifacts;
- package cannot diagnose missing shader root.

## 16. Legacy Reference Gate

Required checks:

- Old renderer is reference only.
- No old source file is imported, moved, included, linked, or copied.
- Any concept borrowed from old renderer is rewritten against `iggy3d` ownership.

Fail if:

- old path appears outside `legacy_reference.md` or an explicitly reviewed historical note;
- old app coupling leaks into `iggy3d`;
- old render-pass baseline is used because it existed before, without platform fallback evidence.

## Final Review Template

Use this format:

```text
Review result: PASS|PASS_WITH_NOTES|FAIL|BLOCKED

Packet:
Files reviewed:
Docs checked:
Tests required:

Blocking findings:
- none

Non-blocking notes:
- none

Required commands:
- ...

Decision/fallback updates required:
- none
```

## Minimum PASS Requirements

A packet can pass only if:

- ownership boundary is intact;
- include firewall is clean or exceptions are justified;
- tests are named;
- diagnostics are named for touched backend/platform behavior;
- fallback behavior is explicit;
- headless acceptance remains independent;
- platform lanes are honestly represented;
- no legacy dependency is introduced.

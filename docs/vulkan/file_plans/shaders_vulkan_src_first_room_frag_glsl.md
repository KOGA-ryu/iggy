# shaders/vulkan/src/first_room.frag.glsl

Status: Draft file plan
Allowed to implement code now: no

## Exact File Path And Purpose

Exact path: `shaders/vulkan/src/first_room.frag.glsl`

Purpose: Define first-room fragment shader output from vertex color with no texture, lighting, or material descriptors.

## Build Position

Packet order: 6 - Shaders, pipeline, resources, descriptors
Owner module: `shader source`
File kind: `shader`
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
- `docs/vulkan/shader_interface_contract.md`
- `docs/vulkan/first_room_render_contract.md`

## Ownership

This file owns:
- Define first-room fragment shader output from vertex color with no texture, lighting, or material descriptors.
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

Allowed inputs: GLSL language constructs defined by the shader build policy and shader interface contract.

Forbidden inputs: C++ includes, runtime data files, save/replay data, package validation logic, and gameplay command logic.

Vulkan headers do not apply inside shader source; SPIR-V output is consumed by Vulkan pipeline files.

SDL headers do not apply inside shader source.

Include firewall rule:
```text
src/runtime/**, src/content/**, src/projection/**, and src/runtime/save/** must not include Vulkan headers, Vk types, VK constants, SDL headers, or window headers.
```

## Public API Or File Shape

The file must expose or define:
- `location 0 out_color`

Naming rule: these names are the current-build contract for implementation planning. Renaming requires updating this file plan and the index in the same packet.

## Data Ownership And Lifetime

Shader source is committed renderer source. Generated SPIR-V is a build or package artifact. Pipeline modules own shader modules after loading SPIR-V. Runtime and projection own no shader state. Shader interface changes are pipeline compatibility changes and require test updates.

## Semantics

Normal path: provide stable shader interface declarations consumed by the shader build and pipeline modules. Skip/fail: missing compiler/artifact is handled by shader build and pipeline smoke according to strictness.

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
file_plan=shaders/vulkan/src/first_room.frag.glsl
packet_order=6
allowed_to_implement_code_now=false
shader_language=glsl
shader_stage=
shader_interface_hash=
shader_target_env=
result=pass|fail|skip
reason_code=
```

User-facing error message shape when this file contributes to visual startup failure:
```text
This machine cannot run the Vulkan visual renderer required by this build.
Reason: <specific renderer or platform reason>.
Action: run the headless runtime demo or use a machine/runtime that satisfies the Vulkan baseline.
```

## Fallback Policy

Fallback policy: missing shader artifacts fail strict shader/pipeline lanes with `shader_missing`; build-tree shader generation is the normal development path.

Fallback receipt fields:
```text
fallback_used=true|false
fallback_area=shader_source
fallback_reason=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

## Compute Cost

Initialization cost: shader compile or shader module creation in the owning build/runtime phase.
Per-frame cost: none for source files; shader execution cost belongs to pipeline smoke and first-room draw.
Resize cost: none unless pipeline compatibility changes through formats.
GPU memory cost: shader module and pipeline state only after SPIR-V load.

## Tests And Verification

Unit tests:
- `tests/unit/render_shader_interface_tests.cpp`

Smoke tests:
- `tests/smoke/vulkan_pipeline_smoke.cpp`

CTest labels:
```text
iggy3d;render;shader
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

## Packet 6 Detailed Contract

Shader contract:
```glsl
#version 450

layout(location = 0) in vec3 in_color;
layout(location = 0) out vec4 out_color;

void main();
```

Interface metadata:
```text
shader_language=glsl
shader_stage=fragment
entry_point=main
fragment_input.0.location=0
fragment_input.0.name=in_color
fragment_output.0.location=0
fragment_output.0.name=out_color
descriptor_sets=0
```

Ownership:
- Owns vertex-color fragment output for first-room proof.
- Does not own texture sampling, lighting, material state, tone mapping, gameplay semantics, or runtime decisions.

Rules:
- Output alpha is stable and opaque for Packet 6/7 first-room path.
- No descriptor sets, samplers, textures, or material ids in first-room fragment shader.
- Interface changes require pipeline and interface tests to move with the shader.

Build output:
```text
source=shaders/vulkan/src/first_room.frag.glsl
artifact=build/generated/shaders/vulkan/<config>/first_room.frag.spv
```

Verification:
- Shader interface test proves output location zero.
- Pipeline smoke proves shader module and first-room pipeline creation with this fragment shader.

## Builder Traps

- Do not import old repo headers or paths.
- Do not make renderer output part of save or replay truth.
- Do not let runtime/content/projection/save include Vulkan or SDL headers.
- Do not encode gameplay decisions in shader code.
- Do not change shader locations without changing C++ interface tests.

## Completion Criteria

- File `shaders/vulkan/src/first_room.frag.glsl` has an implementation packet that follows this plan.
- Include scan proves the declared boundary.
- Tests listed in this plan are present or deliberately deferred by the same packet with reviewer approval.
- Receipts use deterministic key-value text and stable reason codes.
- Runtime hash/replay behavior is unchanged when runtime is involved.
- No legacy repo path, legacy renderer linkage, or graphics dependency leak appears outside the approved surface.

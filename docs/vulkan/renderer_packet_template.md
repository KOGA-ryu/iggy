# Renderer Packet Template

Use this template for Vulkan renderer file-plan packets.

The template is for planning first. Do not implement renderer code from the template unless the current task explicitly asks for implementation.

Copy the sections below into a new packet/file-plan document and fill every required field.

```md
# <Packet Name>

Status: Draft

Roadmap phase:

Packet order:

Primary objective:

Non-goals:

## Gate Check

Headless runtime acceptance status:

Dependency firewall status:

Required prior packets:

Blocked by:

Allowed to implement code now: no

## Docs Read

Required:

- [ ] docs/vulkan/boundaries.md
- [ ] docs/vulkan/renderer_file_plan_order.md
- [ ] docs/vulkan/reviewer_checklist.md

Packet-specific:

- [ ] <doc>
- [ ] <doc>

## Files Planned

| Path | Action | Owner | Notes |
| --- | --- | --- | --- |
| `<path>` | create/update | `<owner>` | `<notes>` |

## Ownership

This packet owns:

- 

This packet must never own:

- gameplay truth;
- command legality;
- save/load truth;
- replay/state hash truth;
- package validation truth;
- renderer fallback without diagnostics;
- legacy renderer linkage.

## Include Rules

Allowed includes:

- 

Forbidden includes:

- Vulkan headers outside approved Vulkan files;
- SDL headers outside approved app/platform files;
- runtime mutation APIs from renderer backend;
- old repo headers or paths.

Required scans:

```sh
rg -n '#include[ <"](SDL3/|SDL\\.h|SDL_vulkan|vulkan/)|\\bVk[A-Z][A-Za-z0-9_]*|\\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
```

Expected result:

```text
no matches
```

## Public Contract

Types/functions/config/options introduced:

- 

Inputs:

- 

Outputs:

- 

Error/diagnostic behavior:

- 

## Data Flow

Allowed flow:

```text
runtime -> projection -> FrameInput -> renderer -> diagnostics/presentation
```

Packet-specific flow:

```text

```

Forbidden flow:

```text
renderer -> runtime truth
renderer -> save truth
renderer -> replay hash
renderer -> command legality
```

## Diagnostics

Required receipt fields:

```text

```

Required reason codes:

```text

```

Artifact paths, if any:

```text

```

## Fallbacks

Fallbacks used:

```text
fallback_used=false
```

If `true`, fill:

```text
fallback_area=
fallback_from=
fallback_to=
fallback_reason=
owner_docs_updated=
```

Allowed by `fallbacks.md`:

```text
yes|no|not_applicable
```

## Platform Matrix

macOS/MoltenVK impact:

Linux native Vulkan impact:

Windows native Vulkan impact:

Optional software Vulkan impact:

Strict lane behavior:

Optional lane behavior:

## Tests

Unit tests:

- 

Smoke tests:

- 

CTest labels:

- 

Commands:

```sh

```

Expected pass behavior:

Expected skip behavior:

Expected fail behavior:

## Runtime Invariance

Replay/hash impact:

Expected result:

```text
runtime summary unchanged
state hash unchanged
```

Proof command:

```sh
ctest --test-dir build --output-on-failure -R 'render_replay|replay_state_hash'
```

## Implementation Notes

Step plan:

1. 
2. 
3. 

Destruction/lifetime order, if applicable:

1. 
2. 
3. 

Threading assumptions:

- 

Performance/cost expectations:

- 

## Blocked Scope

Do not include:

- textures before VMA/resource diagnostics;
- materials before VMA/resource diagnostics;
- asset/model streaming before first visible room;
- lighting before first visible room;
- picking before runtime command route is planned;
- RenderDoc automation before first visual proof;
- platform-specific runtime truth.

Packet-specific blocked scope:

- 

## Acceptance Criteria

This packet is ready when:

- [ ] required docs are read;
- [ ] files have owners and forbidden ownership;
- [ ] include firewall is clean;
- [ ] diagnostics fields are named;
- [ ] tests are named;
- [ ] platform lanes are accounted for;
- [ ] fallbacks are absent or diagnosed;
- [ ] headless acceptance remains independent;
- [ ] legacy renderer linkage is absent.

## Reviewer Checklist Result

Review result:

Reviewer notes:

Blocking findings:

- none

Non-blocking notes:

- none
```

## Required Packet Metadata

Every packet must include:

```text
packet_name=
packet_order=
roadmap_phase=
files_planned=
docs_read=
tests_named=
fallbacks_used=true|false
platform_lanes_touched=
review_result=
```

## Packet Type Hints

### Boundary Packet

Required docs:

- [boundaries.md](boundaries.md)
- [frame_input_contract.md](frame_input_contract.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)

Required proof:

- no Vulkan/SDL in public render headers;
- null renderer does not mutate runtime;
- replay hash unchanged.

### Platform Packet

Required docs:

- [platform_shell.md](platform_shell.md)
- [platform_matrix.md](platform_matrix.md)
- [fallbacks.md](fallbacks.md)

Required proof:

- SDL isolated;
- surface handoff isolated;
- headless build unaffected.

### Vulkan Backend Packet

Required docs:

- [manuals.md](manuals.md)
- [decisions.md](decisions.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [fallbacks.md](fallbacks.md)

Required proof:

- validation/diagnostics receipt;
- raw handles stay private;
- strict/optional lane behavior defined.

### Shader Packet

Required docs:

- [shader_pipeline.md](shader_pipeline.md)
- [platform_matrix.md](platform_matrix.md)
- [packaging.md](packaging.md)

Required proof:

- GLSL/glslang or fallback diagnosed;
- generated SPIR-V path defined;
- runtime does not know shader language.

### Resource Packet

Required docs:

- [resource_model.md](resource_model.md)
- [fallbacks.md](fallbacks.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)

Required proof:

- allocation names;
- no per-frame allocation churn;
- VMA gate respected before texture/material growth.

### First-Room Packet

Required docs:

- [first_room_render_contract.md](first_room_render_contract.md)
- [camera_render_contract.md](camera_render_contract.md)
- [frame_input_contract.md](frame_input_contract.md)

Required proof:

- visible non-background geometry;
- player proxy visible;
- `draw_count > 0`;
- replay hash unchanged.

## Common Failure Reasons

Use existing reason codes where possible:

```text
dependency_firewall_failed
frame_input_invalid
shader_missing
pipeline_create_failed
allocation_failed
depth_format_unsupported
replay_hash_changed
validation_error
unsupported_platform_lane
```

Add new reason codes only with a matching update to [diagnostics_and_tests.md](diagnostics_and_tests.md).

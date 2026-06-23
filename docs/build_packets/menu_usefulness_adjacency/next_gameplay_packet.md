# Next Gameplay Packet Plan

## Objective

Plan the next gameplay foundation after menus: movement/physics playground hardening with stable receipts and no-window proof. This follows Menu Usefulness v1 because useful pause/settings/dev/debug surfaces let Builder prove gameplay changes without visual/window churn.

## Recommended Packet Name

`Movement Physics Playground Hardening v1`

## Likely Source Files Later

- `/Users/kogaryu/iggy3d/src/runtime/movement/*`
- `/Users/kogaryu/iggy3d/src/runtime/traversal/*` if present
- `/Users/kogaryu/iggy3d/src/runtime/world/*`
- `/Users/kogaryu/iggy3d/src/content/assets/RoomAsset.*`
- `/Users/kogaryu/iggy3d/src/content/authoring/EditableRoomDocument.*` only for authored tags/surfaces if needed
- `/Users/kogaryu/iggy3d/src/projection/debug/*`
- `/Users/kogaryu/iggy3d/apps/iggy3d/main.cpp`
- `/Users/kogaryu/iggy3d/src/app/iggy3d/**`
- `/Users/kogaryu/iggy3d/tests/unit/movement_*_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_movement_playground_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_codex_control_smoke.cpp`

## Data Ownership

- Runtime movement owns player motor state, grounded/airborne phases, slope policy, jump, air control, and collision queries.
- Room spatial surfaces and traversal tags own authored affordances.
- Traversal registry/slot builder owns clamber/vault/wire-walk executable slot discovery.
- Visual app owns input/control scripting and receipt readouts.
- Debug projection/HUD owns visible diagnostics.
- Renderer does not own gameplay decisions.

## Semantics/Invariants

Grounded state:

- Ground sample uses runtime collision/spatial surfaces.
- Grounded state is deterministic and independent of frame rate in tests.
- Receipts report ground contact, surface id, normal, distance, and walkability.

Slopes:

- Slope walkability derives from configured angle/grade policy.
- Too-steep slopes block or slide according to explicit policy.
- No hidden wall-clock randomness.

Jump/air control:

- Jump starts only from grounded or explicitly allowed traversal state.
- Air control is bounded and deterministic.
- Landing restores grounded state through collision query.

Traversal:

- Clamber/vault/wire-walk use authored traversal tags/slot registry.
- Explicit tags beat legacy name fallback.
- Endpoint clamping and detach states are deterministic.

Projectile travel:

- Projectile path is deterministic, uses runtime collision/projection only for debug, and has no renderer dependency.
- Packet may include projectile travel only if movement basics stay bounded.

## Command / Action Contracts

Codex-control names:

```text
move.forward=<float>
move.right=<float>
jump=true|false
dash=true|false
stance=crouched|standing
mechanic=walk|jump|dash|clamber|vault|wire_walk|spell
mechanic.execute_frames=<csv-frames>
player.position=<x>,<y>,<z>
codex_probe.position=<x>,<y>,<z>
```

Do not add gameplay input routes that bypass runtime admission/state authority.

## Receipt Fields

```text
movement_reason=<reason>
movement_policy_band=<band>
ground_sample_valid=true|false
ground_contact=true|false
ground_surface_id=<id|none>
ground_walkable=true|false
ground_distance_meters=<float>
ground_normal_x=<float>
ground_normal_y=<float>
ground_normal_z=<float>
slope_angle_degrees=<float>
slope_travel_direction=<direction>
jump_requested=true|false
jump_started=true|false
air_control_applied=true|false
traversal_attempted=true|false
traversal_accepted=true|false
traversal_mechanic=<mechanic|none>
traversal_reason=<reason>
traversal_slot_id=<id|none>
projectile_travel_status=<status|none>
window_launch_count=0
```

## No-Go Surfaces

- No full physics engine replacement.
- No networking/multiplayer.
- No renderer/Vulkan changes.
- No fixture/schema churn unless a movement playground fixture needs a narrow authored surface tag update.
- No animation/asset work.
- No JSON.

## Builder Packet Boundaries

Packet 1: Grounded/slope/jump/air-control hardening, receipts, no-window tests.
Packet 2: Clamber/vault/wire-walk affordance tuning and debug readouts.
Packet 3: Projectile travel and collision debug after movement core is stable.

## Focused Tests

- Unit: ground contact, slope thresholds, jump start/land, air control bounds.
- Unit: traversal registry honors explicit tags and preserves endpoint constraints.
- Smoke: no-window movement playground Codex-control paths for walk, slope, jump, clamber/vault/wire-walk, receipt proof.

## Open Questions

Whether projectile travel belongs in packet 1 or packet 3. Conservative default: packet 3 unless current source already has a clean projectile seam.

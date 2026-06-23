# Product View v1

Updated: 2026-06-23

Purpose: document the current product gameplay view, its ownership boundaries,
and its receipt proof fields so future builders continue from source truth
instead of reviving older demo-only or renderer-owned assumptions.

## Current Gameplay Surface

The current product app starts on the starter/opening menu. Gameplay starts only
after an explicit launch path such as `New World`, `Load Save`,
`--auto-new-world`, or the scripted no-window gameplay smoke.

Once gameplay is active, the first product gameplay surface is the SDL product
view:

- first-person camera mode is the current product camera label;
- the view is primitive-first-person framing, not true perspective rendering;
- runtime/session state remains gameplay truth;
- scene/debug projection is derived proof from runtime state;
- product draw/framing/feedback/bridge data is derived app view data;
- renderer backends must not mutate runtime state, save state, replay state, or
  command truth.

The old product-facing phrase `FIRST PERSON PROXY VIEW` is stale. The current
user-facing label is `FIRST PERSON GAMEPLAY VIEW`. The word `proxy` may still
appear in renderer or asset-roadmap docs when it refers to diagnosed fallback
geometry, temporary renderer proof paths, or future asset migration notes.

## Source-Truth Layers

### Runtime And Session

Runtime/session owns gameplay truth:

- entity transforms and active flags;
- player roster and actor binding;
- command admission and execution;
- target discovery, reach gates, interaction, combat, reset;
- save/load/replay/hash truth.

Product view code may read runtime state through public APIs and projection, but
must not mutate runtime state directly.

### Scene And Debug Projection

`SceneProjectionResult` and `DebugProjectionResult` are read-only derived proof.
They may be regenerated from runtime state and are not saved as truth.

### ProductPrimitiveDrawList

`ProductPrimitiveDrawList` is product-view data derived from scene/debug
projection. It owns the product view's primitive draw intent:

- player marker;
- NPC, pickup, interactable, objective, tactical, and debug markers;
- player focus indicator;
- grid/room/player/objective/target visibility booleans;
- draw item counts.

It does not own runtime entities, renderer handles, GPU resources, or save data.

### ProductViewportFrame

`ProductViewportFrame` is deterministic primitive framing derived from
`ProductPrimitiveDrawList` and `ProductViewportState` camera values.

Current framing rules:

- player marker is the preferred camera anchor;
- yaw rotates marker placement around that anchor;
- pitch applies a deterministic vertical offset;
- grid remains static;
- projection mode is `primitive_first_person`;
- this is not full 3D perspective rendering.

### ProductGameplayFeedback

`ProductGameplayFeedback` is compact product HUD/action feedback derived from
`ProductAppWindowState` fields already populated by runtime command execution.

It shows the current product feedback lines:

- `TARGET`;
- `REACH`;
- `COMMAND`;
- `RESULT`;
- `REJECT` when a rejection reason exists.

It does not query runtime and does not decide command legality.

### ProductRenderBridgeFrame

`ProductRenderBridgeFrame` is renderer-safe derived handoff data built from:

- `ProductPrimitiveDrawList`;
- `ProductViewportFrame`;
- `ProductGameplayFeedback`.

It carries deterministic item metadata, counts, and visibility booleans. It is
not consumed by Vulkan yet. It is intentionally product-owned until a later
renderer packet adapts it into a backend-facing path.

## Stable Receipt Fields

These fields are stable proof surfaces for the product view. Builders may add
new fields when they prove new behavior, but must not rename these fields
without a compatibility plan.

### Gameplay View

```text
gameplay_view_visible=true|false
camera_mode=first_person
camera_yaw_degrees=<fixed-3-float>
camera_pitch_degrees=<fixed-3-float>
camera_heading_visible=true|false
```

### Product Draw List

```text
product_draw_item_count=<integer>
product_draw_grid_visible=true|false
product_draw_player_visible=true|false
product_draw_room_visible=true|false
product_draw_objective_visible=true|false
product_draw_target_indicator_visible=true|false
product_draw_debug_marker_count=<integer>
```

### Product View Framing

```text
product_view_projection=primitive_first_person
product_view_yaw_applied=true|false
product_view_pitch_applied=true|false
product_view_player_anchor_found=true|false
```

### Product Gameplay Feedback

```text
product_feedback_visible=true|false
product_feedback_target_status=<status>
product_feedback_reach_status=<pass|fail|not_attempted>
product_feedback_command_kind=<none|move|interact|attack|retry|reset|other>
product_feedback_command_status=<not_requested|accepted|rejected|no_target|missing_player>
product_feedback_rejection_reason=<reason|none>
product_feedback_attack_visible=true|false
product_feedback_interaction_visible=true|false
```

### Product Menu Transitions

```text
product_transition_last_action=<action>
product_transition_status=<status>
product_transition_returned_to_gameplay=true|false
product_transition_returned_to_title=true|false
product_transition_session_preserved=true|false
```

### Product Render Bridge

```text
product_render_bridge_ready=true|false
product_view_frame_ready=true|false
product_view_frame_item_count=<integer>
product_view_frame_on_screen_item_count=<integer>
product_view_frame_target_item_count=<integer>
product_feedback_bridge_ready=true|false
product_feedback_bridge_line_count=<integer>
```

Starter/no-world receipts must report no stale gameplay bridge:

```text
gameplay_view_visible=false
product_render_bridge_ready=false
product_view_frame_ready=false
product_view_frame_item_count=0
product_feedback_bridge_ready=false
```

## Deferred Work

The following work is intentionally not proven by Product View v1:

- full asset pipeline;
- true perspective room/object rendering in the product SDL view;
- Vulkan consumption of `ProductRenderBridgeFrame`;
- real room/object mesh rendering through the bridge;
- screenshot/frame-hash proof for the product bridge;
- tactical overhead camera rendering;
- renderer-owned gameplay feedback.

Future renderer packets may consume the product bridge, but runtime/session
state remains the authority and renderer output remains derived proof.

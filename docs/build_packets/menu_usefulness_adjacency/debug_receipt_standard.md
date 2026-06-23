# Debug and Receipt Standard Plan

## Objective

Standardize receipt/debug naming, owner/status/count fields, reason codes, and what belongs in HUD versus receipt-only output. This is adjacent to Menu Usefulness v1 because every useful row and dev tool needs stable proof fields that will not drift as gameplay/editor packets grow.

## Likely Source Files Later

- `/Users/kogaryu/iggy3d/src/app/frontend/FrontendReceipt.*`
- `/Users/kogaryu/iggy3d/src/render/RenderDiagnostics.*`
- `/Users/kogaryu/iggy3d/src/projection/debug/*`
- `/Users/kogaryu/iggy3d/apps/iggy3d/main.cpp`
- `/Users/kogaryu/iggy3d/src/app/iggy3d/*`
- `/Users/kogaryu/iggy3d/tests/unit/render_diagnostics_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/frontend_receipt_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_menu_usefulness_smoke.cpp`

## Data Ownership

- Receipts are deterministic machine-readable key-value proof for tests and builder handoffs.
- Debug HUD is human-visible live context and may be a subset of receipts.
- Runtime summary/save/replay remain gameplay truth; receipts do not become save truth.
- Renderer receipt helpers own append behavior and duplicate-key policy.

## Naming Conventions

- Lower snake case keys only.
- Boolean values: `true|false`, with `unavailable` only when tri-state is required.
- Counts end with `_count`.
- Selected ids end with `_id`.
- Status fields end with `_status`.
- Reason fields use `reason_code` or `<owner>_reason_code`.
- Owner fields use stable nouns: `menu_owner`, `input_owner`, `debug_owner`.
- Avoid ambiguous `ok`; use `result=pass|fail|skip` plus reason.

## Standard Field Groups

Common:

```text
app=<app-name>
result=pass|fail|skip
reason_code=<lower-snake-case>
window_launch_count=<integer>
```

Frontend/menu:

```text
frontend_screen=<screen>
frontend_status=<status>
menu_owner=<owner>
frontend_selected_action=<action>
frontend_launch_requested=true|false
gameplay_input_suppressed=true|false
```

Settings:

```text
settings_tab=<tab>
settings_selected_row=<row>
settings_apply_requested=true|false
settings_file_status=<status|none>
```

Editor:

```text
editor_open=true|false
editor_selected_id=<id|none>
editor_last_command=<command>
editor_last_status=<status>
editor_runtime_surface_count=<integer>
```

Movement/gameplay:

```text
movement_reason=<reason>
ground_contact=true|false
traversal_attempted=true|false
traversal_reason=<reason>
projectile_travel_status=<status|none>
```

Product View v1:

```text
gameplay_view_visible=true|false
camera_mode=first_person
camera_yaw_degrees=<fixed-3-float>
camera_pitch_degrees=<fixed-3-float>
product_draw_item_count=<integer>
product_draw_grid_visible=true|false
product_draw_player_visible=true|false
product_draw_room_visible=true|false
product_draw_objective_visible=true|false
product_draw_target_indicator_visible=true|false
product_draw_debug_marker_count=<integer>
product_view_projection=primitive_first_person
product_view_yaw_applied=true|false
product_view_pitch_applied=true|false
product_view_player_anchor_found=true|false
product_feedback_visible=true|false
product_feedback_target_status=<status>
product_feedback_reach_status=<status>
product_feedback_command_kind=<kind>
product_feedback_command_status=<status>
product_feedback_rejection_reason=<reason|none>
product_feedback_attack_visible=true|false
product_feedback_interaction_visible=true|false
product_transition_last_action=<action>
product_transition_status=<status>
product_transition_returned_to_gameplay=true|false
product_transition_returned_to_title=true|false
product_transition_session_preserved=true|false
product_render_bridge_ready=true|false
product_view_frame_ready=true|false
product_view_frame_item_count=<integer>
product_view_frame_on_screen_item_count=<integer>
product_view_frame_target_item_count=<integer>
product_feedback_bridge_ready=true|false
product_feedback_bridge_line_count=<integer>
```

`docs/product_view_v1.md` is the source-truth explainer for these fields.
Starter/no-world receipts must not report stale product view frame or bridge
readiness.

## Reason Code Rules

- Lower snake case only.
- Prefix reason codes by owner when useful: `frontend_continue_disabled`, `settings_invalid_value`, `editor_missing_selection`.
- Pass reasons must be explicit: `menu_usefulness_pass`, `settings_loaded`, not `ok`.
- Skip reasons must be deterministic: `visual_demo_unavailable`, `dependency_unavailable`.
- Do not add generic `StatusCode` wrappers.

## Duplicate Key Policy

Receipts must not emit duplicate keys. Tests should parse representative pass/fail/skip receipts and fail duplicate keys. If existing append helpers allow duplicates, each caller must avoid appending a key twice by construction.

## HUD Versus Receipt

HUD should show only fields useful while playing:

- current screen/menu owner
- selected row/category
- input source
- player position/movement status
- editor selected id/tool/status
- render/backend/frame summary

Receipt-only fields include paths, counts, compatibility metadata, detailed disabled reasons, parse diagnostics, and test proof fields.

## No-Go Surfaces

- No JSON logs.
- No raw pointer addresses.
- No raw Vulkan handles in public receipts except explicit Vulkan-private smoke diagnostics.
- No wall-clock timestamps in deterministic smoke receipts.
- No unordered-map iteration affecting field order.

## Builder Packet Boundaries

Packet 1: Add receipt standard doc/test helpers, duplicate-key tests, field naming regression tests.
Packet 2: Migrate existing frontend/editor/dev receipts to standard names while preserving compatibility aliases where required.
Packet 3: Add HUD standardization after dev tools taxonomy is implemented.

## Focused Tests

- Unit parse representative receipts and assert no duplicate keys.
- Unit assert field names match lower snake case.
- Smoke assert no-window menu receipt has standard common/frontend/settings/dev fields.

## Open Questions

No blocker. Conservative default: keep existing fields stable and add standard aliases before removing older names.

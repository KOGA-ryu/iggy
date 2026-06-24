# Dev Tools Contract v1

## Objective

Define internal dev tools as an overlay layer system, not as regular game menu
windows.

The world must remain visible. Dev tools answer questions about the running
game without covering the thing being inspected.

## Settings Toggle

Settings exposes one developer-facing toggle:

```text
Dev Tools: On / Off
```

The toggle enables or disables dev tools as a whole. V1 does not support partial
category toggles in settings.

When Dev Tools is off:

- runtime dev HUD is hidden;
- debug draw is hidden;
- dev hotkey legend is hidden;
- selection inspector is hidden;
- logs may continue collecting internally but are not shown.

When Dev Tools is on:

- the runtime dev overlay may render;
- debug draw layers may render;
- dev hotkeys may operate;
- external profiler integrations remain available when configured.

## Tool Layers

Iggy dev tools are split into three layers:

1. Runtime debug displays inside the game.
2. Engine tooling and editor-side tools.
3. External profilers and debuggers.

These layers should not be collapsed into one menu.

## Runtime Overlay Layout

The runtime overlay uses fixed screen regions:

```text
top-left:     performance and simulation
top-right:    selected object inspector
bottom-left:  logs
bottom-right: context tools / hotkey legend
center:       clear game world
world-space:  object-attached debug information
```

The center of the screen should stay almost entirely clear.

Target balance:

```text
80% world
20% tools
```

## Top-Left Performance Strip

Always visible when Dev Tools is enabled.

Tiny, pinned, non-interactive, and never attention-stealing.

V1 fields:

```text
FPS
frame_ms
runtime_tick
gameplay_active
frontend_screen
input_owner
```

Later fields:

```text
CPU_ms
GPU_ms
entities
draw_calls
triangles
physics_bodies
memory_usage
network_stats
streaming_stats
```

Frame graphs may use a thin strip near the top of the screen. They should not
be giant windows unless the user explicitly opens a deep investigation view.

## Top-Right Selection Inspector

Appears only when something is selected.

V1 fields:

```text
selected_id
selected_name
selected_kind
position
rotation
scale
```

Gameplay fields when applicable:

```text
health
state
target
current_command
reach_status
```

Later fields:

```text
components
behavior_state
inventory
physics_material
ai_blackboard
```

Selection information should appear near the selected thing or in the
top-right inspector. It should not sit in the center of the screen unless it is
attached to a world object.

## Bottom-Left Logs

Scrollable history, collapsed by default.

V1 log categories:

```text
info
warning
error
input
save_load
automation
frontend
runtime
```

Later categories:

```text
ai
physics
renderer
audio
network
asset_hot_reload
```

Logs should be useful but not become the main application surface.

## Bottom-Right Context Tools

Shows the active dev hotkey legend and context actions.

Example:

```text
F1 Performance
F2 Physics
F3 AI
F4 Rendering
F5 Input
F6 World
F7 Automation
F8 Capture
```

Only show relevant active shortcuts. The legend should be compact.

## World-Space Debug Information

Information that belongs to an object should be attached to that object.

Examples:

```text
nameplate
state label
target arrow
path line
bounding box
collision shape
contact normal
slope normal
audio radius
light radius
trigger volume
projectile arc
spawn marker
chunk boundary
occlusion volume
```

World-space debugging is the primary tool for movement, physics, AI, targeting,
projectiles, and traversal.

## Side Inspector

A collapsible side panel is allowed for deep inspection.

Preferred side:

- center-screen with right-side bias;
- does not obscure the main focal point;
- collapsible;
- summoned only when actively inspecting.

Possible sections:

```text
Scene
Entity
Components
Properties
Debug Actions
```

The side inspector is not the default dev tools experience. It is an
investigation surface.

## Runtime Debug Displays

Runtime displays should include:

- performance strip;
- frame graph strip;
- selected object inspector;
- log stream;
- tool legend;
- world-space labels;
- debug draw overlays.

Runtime displays should be safe to enable during normal gameplay testing.

## Debug Draw Systems

Debug draw is separate from debug UI.

Debug draw categories:

- collision;
- physics contacts;
- movement probes;
- slope and ground normals;
- target/reach ranges;
- projectile arcs;
- AI paths later;
- navmesh later;
- audio radii later;
- light volumes later;
- trigger regions;
- spawn points;
- chunk boundaries.

The debug draw renderer must draw extra geometry and text without mutating
runtime state.

## Engine Tooling

Engine tooling belongs to editor-side tools, not the runtime overlay.

Future engine tooling:

- asset browser;
- dependency viewer;
- hot reload monitor;
- world editor panels;
- entity/component editor;
- room/level authoring tools;
- save/package inspectors.

These tools can be more panel-heavy because they are authoring tools, not
normal runtime overlays.

## External Profilers And Debuggers

External tools are integration expectations, not in-game UI.

Target tool categories:

- graphics frame capture;
- CPU/frame profiler;
- GPU timing profiler;
- memory profiler;
- crash/log capture.

RenderDoc-style frame capture should be planned for renderer work, but the dev
tools overlay must not depend on it.

Tracy-style CPU/frame profiling may be planned later, but V1 should use simple
runtime counters and receipt proof.

## Data Ownership

Dev tools read from:

- frontend state;
- runtime debug snapshot;
- input routing state;
- player/camera state;
- physics debug state later;
- render bridge/debug stats;
- automation status;
- save/load status.

Dev tools do not own:

- runtime simulation truth;
- save file parsing;
- renderer backend behavior;
- physics solver behavior;
- AI behavior.

Dev tools may request toggles, overlays, and debug draw visibility.

## Input Semantics

Settings owns the single Dev Tools on/off toggle.

When Dev Tools is enabled:

- F1 toggles performance details;
- F2 toggles physics debug draw;
- F3 toggles AI debug draw later;
- F4 toggles rendering debug details;
- F5 toggles input debug details;
- F6 toggles world debug details;
- F7 toggles automation debug details;
- F8 triggers capture/screenshot later.

Controller equivalents can be added later. They must not conflict with gameplay
actions unless dev tools owns input.

## Receipt Fields

Recommended proof fields:

```text
dev_tools_enabled=true|false
dev_overlay_visible=true|false
dev_layout_mode=runtime_overlay
dev_top_left_visible=true|false
dev_top_right_visible=true|false
dev_bottom_left_visible=true|false
dev_bottom_right_visible=true|false
dev_world_space_debug_visible=true|false
dev_side_inspector_visible=true|false
dev_selected_entity_id=<id-or-none>
dev_log_count=<integer>
dev_active_layer=performance|physics|ai|rendering|input|world|automation|none
```

Existing fields such as `dev_tools_open`, `dev_tools_category`,
`input_owner`, and automation receipt fields should be reused where possible.

## Test Plan

Unit tests should prove:

- Dev Tools off hides overlay regions;
- Dev Tools on enables runtime overlay state;
- selected inspector appears only with selection;
- logs can collect while hidden;
- hotkey legend reflects active layer;
- debug draw visibility is state only and does not mutate runtime;
- settings toggle controls all dev tools as a group.

Receipt/no-window tests should prove:

- Dev Tools off;
- Dev Tools on;
- top-left performance strip visible;
- selection inspector hidden with no selection;
- log region state;
- tool legend state;
- `window_launch_count=0`.

Window proof can wait until the renderer/frontend surface is ready.

## No-Go

Do not implement dev tools as a pile of large draggable windows.

Do not cover the center of the screen with debug text.

Do not put runtime mutation behind dev tools before the runtime/router
contracts are clean.

Do not add external profiler dependencies to V1.

Do not mix editor-side tools into the runtime overlay.

Do not make settings contain individual dev tool category toggles in V1.

## Coding Method

Dev tools should be implemented as overlay-layer and category descriptors, not
as ordinary modal menu windows and not as scattered debug text branches in
`AppShell.cpp`.

Recommended modules:

```text
src/app/frontend/DevToolsOverlayModel.hpp
src/app/frontend/DevToolsOverlayModel.cpp
src/app/iggy3d/ProductFrontendRouter.hpp
src/app/iggy3d/ProductFrontendRouter.cpp
src/app/iggy3d/ProductFrontendViewModel.hpp
src/app/iggy3d/ProductFrontendViewModel.cpp
```

Each dev tools category should declare:

```text
id
label
region
visible_when_enabled
read_only_rows
command_rows
requires_runtime_session
requires_selection
```

## File Ownership

| File area | Owns | Must not own |
| --- | --- | --- |
| `DevToolsOverlayModel.*` / `DevToolsMenu.*` | category order, overlay regions, read-only rows, command availability | runtime truth, renderer backend behavior |
| Runtime/debug snapshot files | actual runtime facts | overlay layout |
| `ProductFrontendRouter.*` | dev toggle/category navigation and route result | debug draw implementation |
| `ProductFrontendViewModel.*` | data prepared for the view | state mutation |
| `AppShell.cpp` | call router and view builder | category-specific logic |

## Inputs And Outputs

Dev tools model input:

```text
FrontendState
FrontendSettings
RuntimeDebugSummary
InputDebugSummary
ProductRenderBridgeSummary
SelectionSummary
```

Dev tools model output:

```text
DevToolsOverlayViewModel
```

Route output should include selected category, active layer, accepted action,
requested debug toggle, and status. Runtime mutation must remain outside the dev
tools model.

## Long-Term Fit

This layout keeps dev tools useful as the editor, renderer, movement, combat,
and save systems grow. New debug rows can be added by contributing read-only
summary data, not by adding new app shell branches or covering the gameplay
view with modal windows.

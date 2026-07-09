# File Spec

Files: `src/app/iggy3d/debug/NpcBehaviorDebugHud.hpp`, `src/app/iggy3d/debug/NpcBehaviorDebugHud.cpp`, `src/app/iggy3d/debug/NpcBehaviorDebugHudState.hpp`

Verified at: `e4855124`

## Owns

- NPC behavior debug HUD packet, line model, and owned window-state mirror.
- Conversion from `DebugProjectionResult` NPC behavior lines into HUD text, tones, line count, status, and gating reason.
- Visibility/debug availability fields copied into product window state.

## Does Not Own

- Runtime AI behavior, perception, guard decisions, or debug snapshot generation.
- Debug projection construction.
- Drawing, receipt serialization, or window input.

## Reads

- Optional `DebugProjectionResult`.
- Gameplay-active, developer-tools-enabled, and debug-overlay-enabled flags.
- NPC behavior debug line/status facts.

## Writes / Mutates

- Returns `NpcBehaviorDebugHud`; no external state.
- `ProductNpcBehaviorDebugHudState` defines the window-state mirror populated elsewhere.

## Calls Out To / Wires Out To

- Uses product feedback tones for line severity.
- Output is consumed by projection refresh, debug HUD view, Vulkan frame presenter, and receipt fields.

## Called By / Entry Points

- `ProjectionRefresh.cpp` builds/copies NPC behavior HUD facts.
- `DebugHudView.cpp`, `FramePresenter.cpp`, and receipt fields consume the packet/state.
- Grep proof: `rg -n "buildNpcBehaviorDebugHud|NpcBehaviorDebugHud|ProductNpcBehaviorDebugHudState|appendNpcBehaviorDebugHudUi|drawNpcBehaviorDebugHud" src tests cmake`.

## Invariants

- Missing projection and inactive gates keep explicit reason codes.
- Visible lines inherit HUD visibility.
- Unresolved-profile observability remains a window-state/debug receipt concern, not AI policy.
- This file stays a HUD packet builder, not an AI system owner.

## Tests / Proof Commands

- `rg -n "product_npc_behavior_debug_hud_tests|product_vulkan_room_frame_tests" cmake tests`.
- `rg -n "buildNpcBehaviorDebugHud|hasUnresolvedProfile" src tests`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/*` unless projected debug facts change.
- `src/projection/debug/DebugProjection.*` unless debug payload changes.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless state copy changes.

## Update When

- NPC behavior HUD fields, state mirror, line/tone/status mapping, unresolved-profile observability, or gating changes.

## Do Not Update When

- Runtime AI behavior changes without changing projected HUD facts.

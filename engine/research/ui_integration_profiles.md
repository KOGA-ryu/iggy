# UI Integration Profiles

This note records the intended high-level UI shape for later integration. It is
not an implementation order and does not open UI/Edi mutation work.

## Principle

The UI should display the game first, not the engine. Authoring and debugging
surfaces should be available as separate workspace profiles so normal play,
scenario construction, validation, and engine inspection do not compete for the
same screen.

The existing UI direction can support multiple far-left profile tabs. Those tabs
should represent workspace modes.

## Far-Left Workspace Profiles

### Play

Purpose: play the scenario.

Primary content:
- World/level view.
- Player character, NPCs, doors, items, and interaction targets.
- Minimal HUD.
- Current objective or local context.
- Inventory quick view.
- Interaction prompts.
- Pause/menu/save controls later.

Hidden by default:
- TOML source-plan details.
- Validator diagnostics.
- Runtime frame internals.

### Build

Purpose: arrange scenario content.

Primary content:
- Level/scenario canvas.
- Terrain, wall, and floor placement.
- Player start placement.
- NPC placement.
- Item/drop placement.
- Interaction target placement.
- Region placement.

This should be spatial and visual. It should not expose raw engine registries as
the primary interaction model.

### Script

Purpose: author scenario sequence facts without becoming a generic scripting
language.

Primary content:
- Authored frame flow.
- Player commands: move, interact, pickup.
- NPC controls: wait, seek, movement mode, target.
- Interaction/pickup timing.
- Frame ids and ordering.
- Expectation authoring entry points.

Non-goal:
- Arbitrary expressions, loops, condition language, or event scripting.

### Check

Purpose: prove the authored scenario works.

Primary content:
- Lint/check/run status.
- Expected vs actual results.
- Final rows.
- Trace frame playback.
- Diagnostics with source locations.
- Summary counts.

This profile should be backed by `RuntimeGameplayAuthoringPreviewModel` and the
existing authoring facade/projection APIs.

### Actors

Purpose: inspect and author NPC/player-facing actor facts.

Primary content:
- NPC actor list.
- Profiles and trait sets.
- Actor positions.
- Actor control preview.
- Movement/debug overlays where useful.

### Items

Purpose: inspect and author inventory/drop facts.

Primary content:
- Item drops.
- Pickup targets.
- Inventory expectations.
- Item ids/counts.
- Future item definitions if a catalog is approved.

### Interactions

Purpose: inspect and author interaction target/effect facts.

Primary content:
- Interaction targets.
- Toggle/required-item state.
- Enabled/disabled preview.
- Required item ids.
- Existing effect metadata.
- Future inspect/talk/event hooks only after gates approve them.

### Package

Purpose: inspect authored package metadata and boundaries.

Primary content:
- Package title.
- Description.
- Authoring version.
- Main scenario path.
- Package validation status.
- Package-level diagnostics.

Non-goal:
- Recursive package discovery.
- Dependency management.
- Asset catalogs.

### Debug

Purpose: inspect engine/runtime state for developers.

Primary content:
- Runtime state tree.
- Command queue.
- Player intents.
- NPC actor/control registries.
- Interaction target/effect state.
- Inventory/drop state.
- Frame summaries.
- Occupancy/collision overlays.
- AI map overlay.
- Navigation/path overlay.
- Save/load snapshot status later.
- Performance counters later.

This profile is for engine inspection, not normal authoring.

### Docs / Examples

Purpose: provide example-driven authoring guidance.

Primary content:
- Canonical fixtures.
- Regression examples.
- Package examples.
- Supported TOML table/key shape.
- "Open as scenario" entry points.

## First UI Milestone

The first safe UI milestone is a read-only preview consumer:
- Accept one explicit TOML file path or package path.
- Call the runtime read-only preview model.
- Show package metadata when present.
- Show diagnostics.
- Show summary/final rows.
- Show trace frames.
- Show expectation result.

Hard stops for the first milestone:
- No editing.
- No save/load mutation.
- No file watching.
- No package scanning.
- No UI-owned parsing.
- No new scenario semantics.

## Later Milestones

1. Read-only preview panel.
2. Source-linked diagnostics.
3. Visual trace playback.
4. Build canvas for placement.
5. Structured authoring controls for existing facts.
6. Source/TOML roundtrip only after an explicit gate.
7. Product play shell.

## Open Decisions

- Should the first UI consumer live in the existing Qt shell or a separate
  authoring tool?
- Should Play and Build be separate app modes or tabs in one shell?
- Should TOML source view be always visible, optional, or debug-only?
- What is the first editable fact: terrain, actors, interactions, or frame
  commands?
- When does source mutation become safe enough to support?

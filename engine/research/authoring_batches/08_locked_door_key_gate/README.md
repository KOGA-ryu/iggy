# Batch 08: Locked Door/Key Gate

Status: complete.

## Goal
Review whether locked-door/key gameplay can be expressed with existing systems before implementation.

## Current State
Interaction toggle effects and pickup/inventory exist. Key-required door semantics may not.

## Slices
1. Grep existing interaction, inventory, pickup, and effect systems for key/requirement semantics.
2. Document whether current systems can express:
   - pickup key
   - interact door
   - door only toggles if inventory has key
3. If existing systems do not support it, stop with evidence and proposed minimal design options.
4. If existing systems do support it, produce a build order for Batch 09.

## Verification
Read-only review unless docs are added. No full build needed for read-only review.

## Hard Stops
No code implementation in this gate unless the planner explicitly converts it into Batch 09.

## Expected Result
A clear go/no-go decision for locked-door/key semantics.

## Gate Decision

No-go for locked-door/key semantics as a pure fixture-only authoring scenario.

Current systems can express:

- Key pickup: `PickupItem` effects transfer an authored drop into inventory through the existing pickup path.
- Door interaction: authored interaction targets can use `kind = "door"` and can toggle a target enabled/disabled with `ToggleTarget`.
- Item identity: item definitions support `KeyItem`, and inventory state can answer whether an item id is present.

Current systems cannot express:

- "Door only toggles if inventory has key." Interaction effect planning and application only see interaction targets/effects and the player command. They do not inspect `RuntimeInventoryState`, and `InteractionEffect2D` has no requirement/condition field.

Evidence:

- `InteractionEffect2D` supports only `None`, `InspectText`, `ToggleTarget`, `EmitEvent`, and `PickupItem`, with payload ids/text/drop only.
- `InteractionEffectPlan2D` maps a ready interaction to catalog effects without inventory input.
- `InteractionEffectPlanApplier2D` applies those effects against an interaction target registry only.
- `RuntimeInteractionEffectApplyStep` runs the interaction/effect path with session, targets, effects, command, and reach config only.
- `RuntimePlayerInputInteractionPickupFrameStep` handles pickup as a follow-up to deferred pickup effects, not as a general condition system.
- The ASCII source-plan converter can promote `kind = "door"` and `effect = "toggle_target"` or `effect = "pickup_item"`, but it has no authored requirement fields to promote.

## Proposed Batch 09 Options

Preferred minimal option:

- Add a narrow inventory requirement to interaction effects or interaction targets: required item id plus count.
- Thread `RuntimeInventoryState` into the effect planning/application point that decides whether an interaction effect is ready.
- Report a deterministic "requirement missing" outcome without mutating the door target.
- Add TOML fields only for this narrow requirement, for example `required_item_id = "item:key"` and `required_count = 1`.

Alternative smaller-but-less-general option:

- Add a dedicated `unlock_target` effect with `required_item_id` and `effect_target_id`.
- Keep it separate from generic `toggle_target`, so locked-door behavior is explicit and not a broad scripting/condition system.

Avoid for Batch 09:

- Generic scripting or arbitrary predicates.
- Path/range/line-of-sight semantics beyond the existing interaction reach checks.
- Save/load, UI/Edi, directory scanning, or runtime autorun work.

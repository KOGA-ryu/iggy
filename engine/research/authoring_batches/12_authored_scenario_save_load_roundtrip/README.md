# Batch 12: Authored Scenario Save/Load Roundtrip

## Goal
Prove final state from an authored TOML scenario can be saved and loaded through existing gameplay snapshot save/load.

## Current State
Gameplay snapshot save/load exists. TOML CLI can produce final runtime gameplay state.

## Slices
1. Add test-only acceptance: run a canonical TOML scenario to final state.
2. Save final `RuntimeGameplayState` through existing gameplay snapshot save/load.
3. Reload and compare key facts: player, NPCs, inventory, interaction targets, drops.
4. Render final rows after reload and compare with pre-save final rows.
5. Keep CLI unchanged unless a small helper is needed.

## Verification
Focused save/load and TOML fixture tests. Full verification at batch end.

## Hard Stops
No save format changes unless current save/load cannot preserve existing facts; if so, stop with evidence.

## Expected Result
Authored scenario final state roundtrips through existing persistence.

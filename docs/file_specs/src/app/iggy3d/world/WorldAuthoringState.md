# File Spec

Files: `src/app/iggy3d/world/WorldCreationState.hpp`, `src/app/iggy3d/world/WorldSetupState.hpp`

Verified at: `10135e4e`

## Owns

- Product world setup proof state packet embedded in app/window state.
- Product world creation proof state packet embedded in app/window state.
- Dungeon selection, ASCII room, draft edit, initial save, and route-after-create proof fields.

## Does Not Own

- World setup route logic.
- New-world launch orchestration.
- Durable save behavior.
- Receipt field emission.

## Reads

- No live inputs; these headers define state packets.
- Callers read the packets through `ProductAppWindowState.creativeAuthoring`.

## Writes / Mutates

- Mutated by product new-world launch, menu action handlers, automation handlers, and draft edit paths.
- Read by receipt field builders, window input diagnostics, and frame presentation for proof/UI panels.

## Calls Out To / Wires Out To

- No function calls; state packets are included by higher-level product app/window state.

## Called By / Entry Points

- `ProductAppWindowState` embeds these packets inside creative authoring state.
- Focused proof: `rg -n "ProductWorldCreationState|ProductWorldSetupState|worldCreation\\.|worldSetup\\." src/app/iggy3d tests/unit`.

## Invariants

- Setup state and creation state remain separate packets.
- Draft edit state records cursor, selected glyph, last glyph, modified flag, and status separately.
- Creation state records initial-save request/write facts without becoming durable save truth.
- Route-after-create remains proof of launch intent, not a frontend router.

## Tests / Proof Commands

- `rg -n "product_god_struct_ownership_coverage_tests|product_new_world_menu_action_tests|product_starter_menu_action_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "worldSetup\\.|worldCreation\\." tests/unit src/app/iggy3d/receipt src/app/iggy3d/world`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ProductAppWindowState.*` unless embedded state boundaries change.
- `src/app/iggy3d/receipt/WorldAuthoringFields.*` unless receipt exposure changes.
- `src/app/iggy3d/world/ProductNewWorldLaunch.*` unless creation proof writes change.

## Update When

- World setup or creation packet fields, proof semantics, or embedded ownership boundaries change.

## Do Not Update When

- Only underlying ASCII parsing, save bridge internals, or frontend labels change without changing state packet contracts.

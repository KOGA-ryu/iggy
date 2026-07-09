# File Spec

File: `src/app/iggy3d/gameplay/GameplayStore.hpp`

Verified at: `0e08703f`

## Owns

- `GameplayStore`, the product gameplay mirror aggregate embedded by `ProductAppWindowState`.
- Gameplay activity flags, input/tick proof fields, command/movement/jump/dash/reset/traversal/collision/target/outcome/tape packets, transition state, scene/debug counts, and visibility proof booleans.

## Does Not Own

- Runtime session state, gameplay command execution, movement physics, collision surface building, transition routing, receipt formatting, debug HUD rendering, or save/hash persistence.

## Reads

- Included packet definitions from `src/app/iggy3d/gameplay/*State.hpp`, `GameplayMovementInfo.hpp`, and `src/app/iggy3d/menu/ProductTransitionState.hpp`.

## Writes / Mutates

- No functions mutate state here.
- Callers mutate `window.gameplay.*` fields as frame-local/product-lifetime observability mirrors.

## Calls Out To / Wires Out To

- No calls.
- Structural wiring point for controller actions, menu transitions, automation, receipt builders, debug HUDs, window input, and frame presentation that all read or write `ProductAppWindowState::gameplay`.

## Called By / Entry Points

- Included by `src/app/iggy3d/ProductAppWindowState.hpp`.
- Grep proof: `rg -n "GameplayStore|window\\.gameplay|gameplay\\." src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- This is an app/product mirror, not simulation truth.
- Defaults must keep no-session/no-command/no-proof states explicit, usually with `false`, zero counts, or `"not_requested"` / `"none"` strings.
- Runtime, render, and save layers must not depend on this aggregate as authoritative state.
- Packet fields may be verbose for receipt/debug stability; filename/path context carries the app/gameplay ownership.

## Tests / Proof Commands

- `rg -n "GameplayStore|ProductAppWindowState|product_god_struct_ownership_coverage_tests" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake`.
- `rg -n "window\\.gameplay\\.gameplayCommand|window\\.gameplay\\.gameplayMovement|window\\.gameplay\\.gameplayCollision" tests/unit/product_gameplay_controller_tests.cpp tests/unit/product_window_input_frame_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/session/*` unless runtime command/tick APIs change.
- `src/app/iggy3d/gameplay/Controller*.{hpp,cpp}` unless writer semantics change.
- `src/app/iggy3d/receipt/*` unless receipt field projection changes.

## Update When

- `GameplayStore` fields are added, removed, renamed, or moved to a different ownership surface.
- Product gameplay mirrors stop being frame/product observability and become persistent or runtime-truth fields.

## Do Not Update When

- Only controller algorithms, HUD layout, receipt ordering, or runtime movement/collision internals change without changing the aggregate contract.

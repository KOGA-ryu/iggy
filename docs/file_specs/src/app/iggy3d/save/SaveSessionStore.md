# File Spec

Files: `src/app/iggy3d/save/SaveSessionStore.hpp`, `src/app/iggy3d/save/SaveDeleteState.hpp`, `src/app/iggy3d/save/SaveFlowState.hpp`, `src/app/iggy3d/save/SaveRecoverState.hpp`, `src/app/iggy3d/save/SelectedProductSaveState.hpp`

Verified at: `a200d76e`

## Owns

- Product save/load/delete/recover/browser state packet embedded in `ProductAppWindowState`.
- Current save write proof fields and active product save id.
- Load result proof fields and selected load id.
- Save-slot ring, action, delete confirmation, flow, deleted-browser, recover, and selected-save state.

## Does Not Own

- Save catalog scanning, durable save file mutation, soft-delete execution, or recover execution.
- Frontend model construction or draw-list rendering.
- Runtime save envelope format or state hash behavior.
- Receipt emission logic.

## Reads

- No live inputs; these headers define state packets.
- Callers read the packet through `ProductAppWindowState.saveSession`.

## Writes / Mutates

- Mutated by save current-session write code, save slot operations, product save bridge/load flows, marker binding, menu action handlers, automation, and window input paths.
- Read by receipt builders and frame presenters for proof and UI panels.

## Calls Out To / Wires Out To

- Includes save bridge result packets and saved-room marker binding result packets.
- Groups leaf state packets for selected save, save flow, delete confirmation, and recover proof.

## Called By / Entry Points

- `ProductAppWindowState` embeds `SaveSessionStore`.
- Focused proof: `rg -n "SaveSessionStore|saveSession\\.|ProductSaveDeleteState|ProductSaveFlowState|ProductSaveRecoverState|ProductSelectedProductSaveState" src/app/iggy3d tests/unit`.

## Invariants

- Empty, missing, disabled, selected, deleted, and recoverable facts must remain distinguishable.
- `activeProductSaveId` is session identity hint state, not catalog truth by itself.
- Delete and recover packets record command proof; durable mutation truth remains in save bridge results.
- New save-domain fields should prefer a nested packet over adding unrelated loose fields.
- This packet is app/window state, not durable save data.

## Tests / Proof Commands

- `rg -n "product_god_struct_ownership_coverage_tests|product_save_delete_executor_tests|product_save_bridge_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "saveSession\\.|ProductSaveDeleteState|ProductSaveFlowState|ProductSaveRecoverState" tests/unit src/app/iggy3d/receipt src/app/iggy3d/save`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ProductAppWindowState.*` unless the embedded save-session boundary changes.
- `src/app/iggy3d/save/SaveSlotOperations.*` unless delete/recover/ring mutation rules change.
- `src/app/iggy3d/receipt/SaveStateFields.*` unless receipt field exposure changes.

## Update When

- Save-session packet fields, proof semantics, embedded state packet boundaries, or save-domain ownership inside `ProductAppWindowState` changes.

## Do Not Update When

- Only lower-level save file-store behavior, UI labels, or catalog sorting change without changing the state packet contract.

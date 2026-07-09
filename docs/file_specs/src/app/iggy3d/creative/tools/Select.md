# File Spec

Files: `src/app/iggy3d/creative/tools/Select.hpp`, `src/app/iggy3d/creative/tools/Select.cpp`

Verified at: `ff909047`

## Owns

- Creative selection and hover-candidate state.
- Selection change enum and receipt packet.
- Set, clear, candidate update, and select-intent application helpers.
- Selection receipt before/after mirrors.

## Does Not Own

- Viewport picking or target discovery.
- Tool input dispatch that emits selection intents.
- Document mutation of selected objects.
- UI inspector drawing or command row enablement.
- Object lifetime cleanup beyond caller-driven selection clearing.

## Reads

- Current `CreativeSelectionState`.
- `TargetRef` values.
- `CreativeToolIntent` when applying selection tool intents.

## Writes / Mutates

- Mutates caller-owned selection state.
- Updates selected and candidate target refs.
- Writes receipts with requested/applied change, changed flag, accepted flag, and message.

## Calls Out To / Wires Out To

- Consumed by `creative::Facade` during tool-intent dispatch.
- UI model reads selection state for inspector and command availability.
- Mutation helpers clear or adjust selection when selected objects are removed.

## Called By / Entry Points

- `makeDefaultCreativeSelectionState()`.
- `clearSelection(...)`.
- `setSelectedTarget(...)`.
- `updateSelectionCandidate(...)`.
- `applySelectionToolIntent(...)`.
- Grep proof: `rg -n "CreativeSelection|clearSelection|setSelectedTarget|applySelectionToolIntent" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Re-selecting the same target is accepted but unchanged.
- Clearing an already empty selection is accepted but unchanged.
- Invalid select-intent target clears selection.
- Non-selection intents do not mutate state.
- Receipts must preserve before/after selected and candidate refs.

## Tests / Proof Commands

- `creative_select_tests`.
- `creative_facade_tests`.
- `creative_facade_mutation_tests`.
- `product_creative_pick_flow_tests`.
- `rg -n "creative_select_tests|applySelectionToolIntent|setSelectedTarget|clearSelection" cmake/iggy3d_tests.cmake tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/tools/Tools.*` unless emitted selection intent semantics change.
- `src/app/iggy3d/creative/Facade.*` unless selection routing or cleanup changes.
- `src/app/iggy3d/creative/spatial/ViewportPick.*` unless target refs from picking change.
- `src/app/iggy3d/creative/ui/UiFrame.*` unless selection UI consumption changes.

## Update When

- Selection state fields, receipt semantics, candidate behavior, invalid-target handling, or facade selection routing changes.

## Do Not Update When

- Only viewport picking, document deletion behavior, or UI inspector formatting changes without changing selection state rules.

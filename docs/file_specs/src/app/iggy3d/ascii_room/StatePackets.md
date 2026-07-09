# File Spec

Files: `src/app/iggy3d/ascii_room/AsciiRoomDraftState.hpp`, `src/app/iggy3d/ascii_room/AsciiRoomPreviewState.hpp`

Verified at: `b49a78ae`

## Owns

- Header-only product ASCII room draft and preview mirror packets embedded in creative authoring state.
- Defaults for draft source text/id/source name and preview status/count/asset text proof fields.

## Does Not Own

- Preview building, activation, automation command routing, authoring conversion, receipt emission, or save persistence.

## Reads

- No runtime data directly; these headers define packet shapes only.

## Writes / Mutates

- No functions mutate state here.
- Automation and preview/activation flows mutate these packets through `window.creativeAuthoring`.

## Calls Out To / Wires Out To

- No calls.
- Aggregated by `CreativeAuthoringStore`.
- Projected by world authoring receipt fields.

## Called By / Entry Points

- Included by `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`.
- Grep proof: `rg -n "ProductAsciiRoomDraftState|ProductAsciiRoomPreviewState|asciiRoomDraft|asciiRoomPreview" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Draft defaults represent an automation-provided ASCII preview room until overwritten.
- Preview defaults represent no preview request.
- Count fields are receipt-facing mirrors and are not room/session truth.
- Status strings are automation/receipt contracts and should not churn casually.

## Tests / Proof Commands

- `rg -n "asciiRoomDraft|asciiRoomPreview" tests/unit/product_ascii_room_activation_tests.cpp tests/smoke/product_ascii_authoring_smoke.cpp src/app/iggy3d/receipt/WorldAuthoringFields.cpp`.
- `rg -n "ProductAsciiRoomDraftState|ProductAsciiRoomPreviewState" src/app/iggy3d tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/Preview.*` unless draft/preview writer semantics change.
- `src/app/iggy3d/creative/CreativeAuthoringStore.hpp` unless state aggregation changes.
- `src/app/iggy3d/receipt/WorldAuthoringFields.*` unless receipt field projection changes.

## Update When

- Draft or preview fields, defaults, status meanings, or embedding owner changes.

## Do Not Update When

- Only authoring pipeline internals, active-room activation, or UI presentation changes without changing these packet contracts.

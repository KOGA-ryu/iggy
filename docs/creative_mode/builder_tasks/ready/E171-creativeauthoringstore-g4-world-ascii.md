# E171 — CreativeAuthoringStore G4: World Setup And ASCII Room State

**STATUS: READY.** Parent: `blocked/E161-creativeauthoringstore-bulk-move.md`.

## Goal

Move exactly these five `ProductAppWindowState` fields into the existing
`ProductAppWindowState::creativeAuthoring` store:

- `worldSetup`
- `worldCreation`
- `asciiRoomDraft`
- `asciiRoomPreview`
- `asciiRoomActivation`

This is a structural ownership move only. Receipt keys, key order, field values,
world setup behavior, ASCII room behavior, save/load behavior, and launch/menu
routing must stay byte-identical.

## Scope

Expected production files include, but are not limited to:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`
- `src/app/iggy3d/receipt/WorldAuthoringFields.cpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/ascii_room/Activation.cpp`
- `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
- `src/app/iggy3d/menu/ActionHandlers.cpp`
- `src/app/iggy3d/menu/Transitions.cpp`
- focused product world setup / world creation / ASCII room tests
- `docs/god_struct_member_ownership.tsv`

Use compiler-guided repoints. Do not use broad token replacement. In dense
blocks, a local `CreativeAuthoringStore& authoring = window.creativeAuthoring;`
or `const CreativeAuthoringStore& authoring = ...` is acceptable when it makes
the code easier to read, but keep the change mechanical.

## Required Move

1. Add the five fields to `CreativeAuthoringStore` with the exact existing
   types, defaults, and relative order from `ProductAppWindowState`.
2. Add whatever direct includes `CreativeAuthoringStore.hpp` needs for the five
   moved types.
3. Remove the five flat fields from `ProductAppWindowState`.
4. Repoint only `ProductAppWindowState` storage reads/writes to
   `window.creativeAuthoring.<field>` or the equivalent named window variable.
5. Delete the five old top-level ownership rows from
   `docs/god_struct_member_ownership.tsv`. Keep the existing
   `creativeAuthoring	CreativeAuthoringStore` row.

## Do Not Move

Do not move any of these in this card:

- `creativeDocumentRevision`
- `creativeDocumentChangedThisFrame`
- `creativeUndo`
- `creativeBakedRoomStale*`
- `creativeNavigateActive`
- `creativeUiProjection`
- `creativeUiInput`
- `creativeUiLast`
- `creativeUiCommand`
- `creativeBakedRoomAutoRefresh`
- `ProductRoomStore room`
- already-moved wireframe, viewport-pick, or room-editor fields

Do not move foreign payload/model/request fields that happen to use similar
names. Classify any remaining broad-scan hits in the completion brief.

## Required Greps

After implementation, these must produce no output:

```sh
rg -n "window\.(worldSetup|worldCreation|asciiRoomDraft|asciiRoomPreview|asciiRoomActivation)\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'

rg -n "\b(worldSetup|worldCreation|asciiRoomDraft|asciiRoomPreview|asciiRoomActivation)\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp

rg -n "^(worldSetup|worldCreation|asciiRoomDraft|asciiRoomPreview|asciiRoomActivation)\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Also run and classify remaining hits from:

```sh
rg --pcre2 -n "(?<!creativeAuthoring)\.(worldSetup|worldCreation|asciiRoomDraft|asciiRoomPreview|asciiRoomActivation)\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
```

Remaining hits must be foreign request/model/result payloads, not
`ProductAppWindowState` storage.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

`git diff -- tests/golden/product_receipt_key_order.golden` must be empty.

## Completion Brief

Report:

- exact files changed;
- the five moved fields;
- receipt golden result;
- ownership coverage result;
- required grep results;
- broad old-flat-access scan classification;
- full suite result;
- any deferred fields still untouched.

Do not stage, commit, push, or launch a window.

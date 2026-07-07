# E139: ActiveCreative G4 - Delete Window Mirror

## Objective

Final gate of the `window.activeCreative` mirror-delete migration. Remove the
mirror struct, field, funnel functions, stale divergence writes, and ownership
TSV row after E136-E138 have established identity/source readers.

## Prerequisite

E136, E137, and E138 must be completed first. If `SaveStateFields.cpp` still
reads `window.activeCreative`, or routing still depends on the mirror where
`creativeApp` is available, move this card to `blocked/` with evidence.

## Required Work

1. Resolve the null-facade pause branch in `src/app/iggy3d/save/Flow.cpp`:
   - Drop the mirror-only `saveStatus`/`saveReasonCode` writes.
   - Convert the five self-read `creativeSave` fields to explicit defaults on
     the null-facade branch.
   - Re-express the source predicate null fallback without reading the deleted
     mirror, following `docs/activecreative_mirror_delete_preflight_v0_2.md`.

2. Delete active-creative mirror writers:
   - remove `mirrorProductActiveCreativeIdentity(...)` and
     `clearProductActiveCreativeIdentity(...)` declarations/definitions.
   - remove their call sites; identity ownership remains in
     `creative::CreativeAppState::identity`.

3. Delete mirror routing:
   - remove `productCreativeWorldActiveForWindowMirror(...)`.
   - remove or rework `productCreativeWorldActiveForWindow(...)` if it has no
     non-test purpose after source routing.

4. Delete mirror storage:
   - remove `ProductActiveCreativeState activeCreative` from
     `ProductAppWindowState`.
   - delete `src/app/iggy3d/ProductActiveCreativeState.hpp` if no longer used.
   - remove any now-unused include.

5. Remove the ownership registry row:
   - delete `activeCreative` from
     `docs/god_struct_member_ownership.tsv`.

6. Fix compile breaks by moving remaining test fabricators to
   `creative::CreativeAppState::identity`.

## Do Not

- Do not remove or rename receipt keys.
- Do not alter `CreativeActiveIdentity` semantics.
- Do not widen this into unrelated `ProductAppWindowState` decomposition.
- Do not leave compatibility aliases named `activeCreative`.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
./build/product_receipt_key_order_tests
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Required greps:

```sh
rg -n "activeCreative|ProductActiveCreativeState|mirrorProductActiveCreativeIdentity|clearProductActiveCreativeIdentity|productCreativeWorldActiveForWindowMirror" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Expected result: no source/test/TSV hits. If comments in the completed task card
or preflight docs still mention the old names, report them separately; do not
rewrite historical design docs unless needed.

## Acceptance

- Full CTest suite remains green.
- `product_god_struct_ownership_coverage_tests` passes with the TSV row removed.
- `product_receipt_key_order_tests` passes; golden should remain unchanged.
- No `activeCreative` mirror references remain in source/tests/ownership TSV.
- The final diff removes a god-struct field rather than adding another store.

## Completion Brief

Append:

- Files changed/deleted:
- Flow divergence resolution:
- Mirror funnel deleted:
- Routing predicate cleanup:
- TSV row removed:
- Final grep result:
- Receipt golden changed?:
- Suite:
- Concerns/deferred:

## Completed Brief

- Files changed/deleted:
  - Deleted `src/app/iggy3d/ProductActiveCreativeState.hpp`.
  - Removed `ProductAppWindowState::activeCreative` storage from `src/app/iggy3d/ProductAppWindowState.hpp`.
  - Updated source routing/lifecycle files:
    `src/app/iggy3d/Operations.hpp`,
    `src/app/iggy3d/Operations.cpp`,
    `src/app/iggy3d/ReceiptBuilder.cpp`,
    `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`,
    `src/app/iggy3d/menu/ActionHandlers.hpp`,
    `src/app/iggy3d/menu/ActionHandlers.cpp`,
    `src/app/iggy3d/menu/FrontendRouter.hpp`,
    `src/app/iggy3d/menu/FrontendRouter.cpp`,
    `src/app/iggy3d/menu/InputRouter.cpp`,
    `src/app/iggy3d/save/Flow.cpp`,
    `src/app/iggy3d/window/InputFrame.hpp`,
    `src/app/iggy3d/window/InputFrame.cpp`.
  - Updated focused product/creative tests to use `creative::CreativeAppState::identity`
    or source-aware routing helpers instead of the deleted window mirror.
  - Removed the `activeCreative` ownership row from
    `docs/god_struct_member_ownership.tsv`.
- Flow divergence resolution:
  - The null-facade pause-save branch now records explicit `creativeSave`
    missing-facade defaults on the flow/window result without reading or writing
    a window active-creative mirror.
  - Return-to-title clears source-owned `creativeApp.identity` when available
    and also clears the product window undo mirrors.
- Mirror funnel deleted:
  - Removed `mirrorProductActiveCreativeIdentity(...)` and
    `clearProductActiveCreativeIdentity(...)` declarations, definitions, and
    call sites.
  - Creative world launch/open/save identity ownership now remains in
    `creative::CreativeAppState::identity`.
- Routing predicate cleanup:
  - Removed `productCreativeWorldActiveForWindowMirror(...)`.
  - Added/used source-aware routing helpers so product input/projection/menu
    paths prefer `CreativeAppState::identity` when a source app is available,
    while preserving conservative window-only fallbacks for legacy/no-source
    tests.
- TSV row removed:
  - `docs/god_struct_member_ownership.tsv` no longer lists `activeCreative`.
- Final grep result:
  - `rg -n "activeCreative|ProductActiveCreativeState|mirrorProductActiveCreativeIdentity|clearProductActiveCreativeIdentity|productCreativeWorldActiveForWindowMirror" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv`
    returned no hits.
- Receipt golden changed?:
  - No. `git diff -- tests/golden/product_receipt_key_order.golden` is empty,
    and `./build/product_receipt_key_order_tests` reported
    `1031 fields match golden`.
- Suite:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - Focused E139 ctest cluster passed: 15/15.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure`
    passed: 260/260.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched/new files passed.
- Concerns/deferred:
  - `Testing/Temporary/LastTest.log` remains dirty from CTest output and was
    intentionally left untouched.

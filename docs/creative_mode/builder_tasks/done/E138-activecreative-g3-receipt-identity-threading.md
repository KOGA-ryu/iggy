# E138: ActiveCreative G3 - Receipt Identity Threading

## Objective

Gate 3 of the `window.activeCreative` mirror-delete migration. Route the
`active_creative_*` receipt fields through
`creative::CreativeActiveIdentity` by const reference instead of reading the
window mirror.

## Prerequisite

E136 and E137 must be completed first. If routing is not migrated to the source
predicate, move this card to `blocked/` with evidence.

## Required Work

1. Add `const creative::CreativeActiveIdentity&` to the product receipt build
   path:
   - `src/app/iggy3d/ReceiptBuilder.hpp`
   - `src/app/iggy3d/ReceiptBuilder.cpp`
   - any receipt sub-appender declaration/definition needed for
     `appendProductSaveStateFields(...)`.

2. Repoint `src/app/iggy3d/receipt/SaveStateFields.cpp` so the twelve
   `active_creative_*` fields read from the identity:
   - saveId
   - savePath
   - worldId
   - documentId
   - objectCount
   - nextObjectId
   - saveStatus
   - saveReasonCode
   - saveDirtyFlagsBefore
   - saveDirtyFlagsDrained
   - saveDirtyFlagsAfter
   - saveSavedAtUtc

3. Production caller:
   - `src/app/iggy3d/AppKernel.cpp` passes `creativeApp.identity`.

4. Test callers:
   - most receipt fixture calls pass a default `creative::CreativeActiveIdentity{}`.
   - any test that previously set `window.activeCreative.*` to assert live
     receipt values must set/use `creativeApp.identity` instead.

## Do Not

- Do not remove any receipt key or reorder existing receipt fields.
- Do not regenerate the golden unless the test proves a value changed. The
  expected result is byte-identical golden output.
- Do not delete `window.activeCreative` or its funnel yet.
- Do not change save/pause divergence behavior in `Flow.cpp`.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
./build/product_receipt_key_order_tests
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also report whether this command is clean:

```sh
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
```

## Acceptance

- Full CTest suite remains green.
- `product_receipt_key_order_tests` passes without golden regeneration.
- `SaveStateFields.cpp` no longer reads `window.activeCreative`.
- The mirror field still exists for the final delete gate.

## Completion Brief

Append:

- Files changed:
- Receipt function signatures threaded:
- SaveStateFields source switched:
- Production caller:
- Test caller update count/files:
- Golden changed?:
- Suite:
- Concerns/deferred:

## Completion Brief - 2026-07-07

- Files changed:
  - `src/app/iggy3d/ReceiptBuilder.hpp`
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `src/app/iggy3d/receipt/ReceiptFields.hpp`
  - `src/app/iggy3d/receipt/SaveStateFields.cpp`
  - `src/app/iggy3d/AppKernel.cpp`
- Receipt function signatures threaded:
  - `buildProductAppReceipt(...)` now accepts a final defaulted `const creative::CreativeActiveIdentity&`.
  - `appendProductSaveStateFields(...)` now receives `const creative::CreativeActiveIdentity&`.
  - `defaultProductReceiptCreativeIdentity()` preserves source compatibility for existing receipt fixture callers that do not care about active creative identity values.
- SaveStateFields source switched:
  - All twelve `active_creative_*` receipt fields now read from the passed `creative::CreativeActiveIdentity`.
  - Verified `rg -n "window\\.activeCreative" src/app/iggy3d/receipt/SaveStateFields.cpp` returns no matches.
- Production caller:
  - `src/app/iggy3d/AppKernel.cpp` passes `creativeApp.identity` into `buildProductAppReceipt(...)`.
- Test caller update count/files:
  - No receipt fixture source edits were required because the receipt builder default identity preserves default-call compatibility.
  - E137 source-predicate tests remain intact in the same dirty stack.
- Golden changed?:
  - No. `./build/product_receipt_key_order_tests` passed with `1031 fields match golden (order + values)`.
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` is empty.
- Suite:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - First full CTest attempt hit transient temp-state fallout in eight product/menu tests; all eight passed when rerun directly.
  - Full CTest rerun passed: `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure`.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over E138 touched files and this card passed.
- Concerns/deferred:
  - `window.activeCreative` mirror and funnel still exist for the final G4 delete gate.
  - Flow.cpp save/pause divergence was not changed.
  - `Testing/Temporary/LastTest.log` remains dirty from CTest output and was not touched.

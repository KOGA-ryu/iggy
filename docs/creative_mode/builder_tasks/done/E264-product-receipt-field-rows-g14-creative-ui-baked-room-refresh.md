# E264: Product Receipt Field Rows G14 - Creative UI Baked Room Refresh

## Objective

Apply the proven local receipt row-table pattern to the remaining fixed
baked-room refresh diagnostic rows in
`src/app/iggy3d/receipt/CreativeUiFields.cpp`, while preserving the optional
`clearedActiveRoom` append branch as procedural.

This is the fourteenth receipt-field row slice after E251-E263. Keep it
mechanical: preserve helper signatures, key-set behavior, field order, field
keys, field values, formatting, helper call order, and receipt golden output
exactly.

## Current Context

The local receipt row-table pattern has been applied successfully through:

- E251: `GameplaySceneStateFields.cpp`
- E252: `GameplayRuntimeMovementFields.cpp`
- E253: `WorldAuthoringFields.cpp`
- E254: `FeedbackSurfaceAutomationVulkanFields.cpp`
- E255: `CreativePickWireframeFields.cpp`
- E256: `StartupProbeFields.cpp`
- E257: `ActiveRoomFields.cpp`
- E258: `DebugHudFields.cpp`
- E259: `TailFields.cpp`
- E260: `SaveStateFields.cpp`
- E261: `FrontendSettingsWindowFields.cpp`
- E262: fixed rows in `CreativeUiFields.cpp`'s public appender
- E263: fixed creative UI command diagnostic helper rows

Each completed slice used file-local row tables, did not add a shared helper or
`ProductAppReceiptContext`, and kept the receipt golden byte-identical.

After E263, the remaining procedural append rows in `CreativeUiFields.cpp` are
inside the baked-room refresh diagnostic helper:

```cpp
appendProductCreativeBakedRoomRefreshDiagnosticFields(...)
```

That helper is shared by:

- `appendProductCreativeUiCommandBakedRoomRefreshFields(...)`, whose key set
  has an empty `clearedActiveRoom` key;
- `appendProductCreativeBakedRoomAutoRefreshFields(...)`, whose key set has a
  real `creative_baked_room_auto_refresh_cleared_active_room` key.

The optional branch must keep its current policy:

```cpp
if (!keys.clearedActiveRoom.empty()) {
  appendReceiptField(receipt, keys.clearedActiveRoom, fields.clearedActiveRoom);
}
```

## Scope

Edit only:

- `src/app/iggy3d/receipt/CreativeUiFields.cpp`
- this task card when moving it to `done/`

Add file-local append-callback row tables for the fixed baked-room refresh
diagnostic rows. Preserve the existing helper signatures:

```cpp
void appendProductCreativeBakedRoomRefreshDiagnosticFields(
    RenderReceipt& receipt,
    const ProductCreativeBakedRoomRefreshDiagnostics& fields,
    const ProductCreativeBakedRoomRefreshReceiptKeySet& keys);

void appendProductCreativeUiCommandBakedRoomRefreshFields(
    RenderReceipt& receipt,
    const ProductCreativeBakedRoomRefreshDiagnostics& fields);

void appendProductCreativeBakedRoomAutoRefreshFields(
    RenderReceipt& receipt,
    const ProductCreativeBakedRoomRefreshDiagnostics& fields);
```

The shared diagnostic helper should:

1. Iterate a pre-optional table for `requested` and `accepted`.
2. Run the existing `clearedActiveRoom` optional branch unchanged.
3. Iterate a post-optional table for the remaining fixed rows.

## Required Row Shape

Use file-local append-callback rows. Do not add a shared helper yet.

Suggested shape:

```cpp
struct ProductCreativeBakedRoomRefreshReceiptFieldRow {
  std::string_view ProductCreativeBakedRoomRefreshReceiptKeySet::* key;
  void (*append)(RenderReceipt& receipt,
                 const ProductCreativeBakedRoomRefreshDiagnostics& fields,
                 std::string_view key);
};

const std::array<ProductCreativeBakedRoomRefreshReceiptFieldRow, 2>
    kProductCreativeBakedRoomRefreshPreOptionalReceiptFields{{
        {&ProductCreativeBakedRoomRefreshReceiptKeySet::requested,
         [](RenderReceipt& receipt,
            const ProductCreativeBakedRoomRefreshDiagnostics& fields,
            std::string_view key) {
           appendReceiptField(receipt, key, fields.requested);
         }},
        {&ProductCreativeBakedRoomRefreshReceiptKeySet::accepted,
         [](RenderReceipt& receipt,
            const ProductCreativeBakedRoomRefreshDiagnostics& fields,
            std::string_view key) {
           appendReceiptField(receipt, key, fields.accepted);
         }},
    }};

const std::array<ProductCreativeBakedRoomRefreshReceiptFieldRow, 10>
    kProductCreativeBakedRoomRefreshPostOptionalReceiptFields{{ /* status ... */ }};
```

When iterating, resolve the key through the key-set:

```cpp
row.append(receipt, fields, keys.*(row.key));
```

Use `const std::array` rather than forcing `constexpr` if any compiler issue
appears.

## Required Behavior Preservation

Preserve every fixed row in
`appendProductCreativeBakedRoomRefreshDiagnosticFields(...)`:

- same key source from `ProductCreativeBakedRoomRefreshReceiptKeySet`
- same order
- same value expression
- same formatter behavior and overload selection

The pre-optional table must cover exactly:

- `keys.requested` / `fields.requested`
- `keys.accepted` / `fields.accepted`

The optional branch must remain procedural and remain after `accepted` and
before `status`:

```cpp
if (!keys.clearedActiveRoom.empty()) {
  appendReceiptField(receipt,
                     keys.clearedActiveRoom,
                     fields.clearedActiveRoom);
}
```

The post-optional table must cover exactly:

- `keys.status` / `fields.status`
- `keys.reasonCode` / `fields.reasonCode`
- `keys.bakeMeasured` / `fields.bakeMeasured`
- `keys.bakeElapsedMicroseconds` / `fields.bakeElapsedMicroseconds`
- `keys.bakedDocumentRevision` / `fields.bakedDocumentRevision`
- `keys.staticMeshCount` / `fields.staticMeshCount`
- `keys.anchorCount` / `fields.anchorCount`
- `keys.spatialSurfaceCount` / `fields.spatialSurfaceCount`
- `keys.collisionReady` / `fields.collisionReady`
- `keys.collisionQuerySurfaceCount` / `fields.collisionQuerySurfaceCount`

Do not change the key literals in either wrapper key-set:

- command baked-room refresh keys, including empty `clearedActiveRoom`;
- auto-refresh keys, including
  `creative_baked_room_auto_refresh_cleared_active_room`.

Do not change the E262 public appender row tables or the E263 command
diagnostic row tables.

Do not leave any fixed baked-room refresh rows procedural unless a compile
issue forces it; if that happens, stop and report the exact row instead of
widening the design.

## Non-Goals

Do not edit:

- `ReceiptFields.hpp`
- `ReceiptFields.cpp`
- `ReceiptBuilder.cpp`
- any other `src/app/iggy3d/receipt/*Fields.cpp`
- `ProductAppReceiptContext`
- CMake
- tests
- fixtures
- `tests/golden/product_receipt_key_order.golden`

Do not add a shared receipt row helper.
Do not remove or table-drive the `clearedActiveRoom` optional branch.
Do not change receipt keys, order, values, status strings, stringifier outputs,
or formatting.
Do not split creative UI sections into multiple public appenders.
Do not perform broad include cleanup beyond what is needed to compile this file.
Do not regenerate the golden.
Do not run broad CTest.
Do not launch a window.
Do not stage, commit, or push.

## Required Grep Classification

Run:

```sh
rg -n "ProductCreativeBakedRoomRefreshReceiptFieldRow|kProductCreativeBakedRoomRefreshPreOptionalReceiptFields|kProductCreativeBakedRoomRefreshPostOptionalReceiptFields|ProductCreativeBakedRoomRefreshReceiptKeySet|appendProductCreativeBakedRoomRefreshDiagnosticFields|appendProductCreativeUiCommandBakedRoomRefreshFields|appendProductCreativeBakedRoomAutoRefreshFields|clearedActiveRoom|appendReceiptField\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/CreativeUiFields.cpp
```

Expected:

- file-local baked-room refresh row type exists;
- pre-optional and post-optional row arrays exist;
- fixed baked-room refresh `appendReceiptField(...)` calls live inside row
  callbacks;
- `clearedActiveRoom` optional branch remains procedural between the two
  row-table iterations;
- command and auto-refresh wrapper key-sets remain present.

Run:

```sh
rg -n "ProductAppReceiptContext|ReceiptFieldRow|ProductCreativeBakedRoomRefreshReceiptFieldRow" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/CreativeUiFields.cpp
```

Expected:

- no `ProductAppReceiptContext`;
- no shared `ReceiptFieldRow` in `ReceiptBuilder.cpp` or
  `ReceiptFields.hpp/.cpp`;
- baked-room refresh row type appears only in `CreativeUiFields.cpp`.

Run:

```sh
rg -c "appendReceiptField\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/CreativeUiFields.cpp
```

Expected:

- `159`

The count should remain 159 because this slice moves fixed baked-room refresh
rows into callbacks while the optional `clearedActiveRoom` append call site
stays procedural.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_receipt_key_order_tests$' --output-on-failure
(cd /Users/kogaryu/iggy3d && /Users/kogaryu/iggy3d/build/product_receipt_key_order_tests)
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over:

- `src/app/iggy3d/receipt/CreativeUiFields.cpp`
- this task card after moving it to `done/`

## Self-Blockers

Stop and report instead of widening scope if:

- the local row tables change any receipt key, order, value, or golden output
- preserving a row requires a shared helper or appender signature change
- the `clearedActiveRoom` optional-key branch needs to change or move
- command/auto-refresh key-set literals need to change
- direct bool/string/count/microsecond formatting or overload selection changes
- compile fallout expands beyond include repairs in `CreativeUiFields.cpp`
- the implementation starts to require `ProductAppReceiptContext`

## Completion Brief Checklist

Report:

- files changed
- exact row helper shape
- pre-optional and post-optional row counts
- whether the `clearedActiveRoom` branch stayed procedural
- whether command/auto-refresh wrapper key-sets stayed unchanged
- first and last receipt keys for each fixed table
- whether any fixed baked-room refresh rows were left procedural
- required grep classifications
- focused build/CTest/direct oracle results
- receipt golden diff result
- diff/whitespace check results
- confirmation that no shared helper, `ProductAppReceiptContext`, CMake, tests,
  golden, staging, commit, push, broad CTest, or window launch was performed

## Completion Brief

Status: Done.

Files changed:

- `src/app/iggy3d/receipt/CreativeUiFields.cpp`
- `docs/creative_mode/builder_tasks/done/E264-product-receipt-field-rows-g14-creative-ui-baked-room-refresh.md`

Refactor shape:

- Added file-local `ProductCreativeBakedRoomRefreshReceiptFieldRow` with:
  - `std::string_view ProductCreativeBakedRoomRefreshReceiptKeySet::* key`
  - append callback
    `void (*append)(RenderReceipt&, const ProductCreativeBakedRoomRefreshDiagnostics&, std::string_view)`
- Added ordered fixed-row tables:
  - `kProductCreativeBakedRoomRefreshPreOptionalReceiptFields`
  - `kProductCreativeBakedRoomRefreshPostOptionalReceiptFields`
- The shared diagnostic helper now resolves each fixed key through
  `keys.*(row.key)` and calls `row.append(receipt, fields, ...)`.

Row coverage:

- Pre-optional table: 2 rows,
  `requested` through `accepted`.
- Post-optional table: 10 rows,
  `status` through `collisionQuerySurfaceCount`.
- Fixed baked-room refresh rows left procedural: none.

Preserved procedural paths:

- The `clearedActiveRoom` branch stayed procedural and remains between the
  pre-optional and post-optional table iterations.
- Command baked-room refresh and auto-refresh wrapper key-sets stayed unchanged,
  including the empty command `clearedActiveRoom` key and the auto-refresh
  `creative_baked_room_auto_refresh_cleared_active_room` key.
- E262 public appender row tables and E263 command diagnostic row tables were
  not changed.

Required grep classification:

- File-local baked-room refresh row type exists in `CreativeUiFields.cpp`.
- Pre-optional and post-optional row arrays exist.
- Fixed baked-room refresh `appendReceiptField(...)` calls live inside row
  callbacks.
- `clearedActiveRoom` remains the only procedural append branch inside the
  shared diagnostic helper and remains between the two row-table iterations.
- Command and auto-refresh wrapper key-sets remain present.
- `rg -c "appendReceiptField\\(" CreativeUiFields.cpp` returned `159`.
- No `ProductAppReceiptContext` or shared `ReceiptFieldRow` was introduced in
  `ReceiptBuilder.cpp` or `ReceiptFields.hpp/.cpp`.
  The grep sees only file-local row types already present in
  `CreativeUiFields.cpp`.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d
  product_receipt_key_order_tests -j10`: passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R
  '^product_receipt_key_order_tests$' --output-on-failure`: passed.
- `(cd /Users/kogaryu/iggy3d &&
  /Users/kogaryu/iggy3d/build/product_receipt_key_order_tests)`: passed with
  `receipt key-order oracle: 1032 fields match golden (order + values)`.
- `git -C /Users/kogaryu/iggy3d diff --
  tests/golden/product_receipt_key_order.golden`: no diff.
- `git -C /Users/kogaryu/iggy3d diff --check`: passed.
- Focused trailing-whitespace scan over `CreativeUiFields.cpp` and this task
  card: passed.

Scope confirmation:

- No shared receipt helper, `ProductAppReceiptContext`, CMake, tests, fixtures,
  receipt golden files, staging, commit, push, broad CTest, or window launch
  were performed.

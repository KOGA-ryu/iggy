# E178 - ViewportStore Creative World Epoch Move

## Status

Ready.

## Objective

Move the remaining top-level `ProductAppWindowState::creativeWorldEpoch` field
into the existing viewport store. This field is the freshness token for
`window.viewport.creativeFlyAnchor`, and E177 confirmed its current TSV owner is
already `ViewportStore`.

This is a scoped structural move only.

## Scope

Expected touched files:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/view/ViewportState.hpp`
- `src/app/iggy3d/view/CreativeFlyAnchorStore.cpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
- focused tests that seed/assert `creativeWorldEpoch`
- `docs/god_struct_member_ownership.tsv`
- this task card

Use compiler-guided repoints. Do not broad replace unrelated `creativeWorldEpoch`
mentions outside `ProductAppWindowState` storage paths.

## Required Behavior

- Add `std::uint64_t creativeWorldEpoch = 0;` to `ProductViewportState`.
- Remove the top-level `ProductAppWindowState::creativeWorldEpoch` member.
- Repoint `ProductAppWindowState` storage reads/writes from
  `window.creativeWorldEpoch` / `request.window.creativeWorldEpoch` to
  `window.viewport.creativeWorldEpoch` / `request.window.viewport.creativeWorldEpoch`.
- Preserve current epoch bump/freshness behavior exactly.
- Preserve receipt golden output.
- Remove the top-level `creativeWorldEpoch	ViewportStore` row from
  `docs/god_struct_member_ownership.tsv`; keep the existing
  `viewport	ViewportStore` row.

## Do Not Touch

- Do not touch `runtimeStateHash`.
- Do not move or reshape `automationControl`.
- Do not change creative fly anchor semantics.
- Do not move `ProductCreativeFlyAnchorStore` unless compile proves the epoch
  cannot live in `ProductViewportState`.
- Do not change receipt keys/order or regenerate the receipt golden.
- Do not stage, commit, push, or launch a window.

## Required Greps

After implementation, these should produce no output:

```sh
rg -n "window\\.creativeWorldEpoch\\b|request\\.window\\.creativeWorldEpoch\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
rg -n "\\bcreativeWorldEpoch\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp
rg -n "^creativeWorldEpoch\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

These should stay non-empty:

```sh
rg -n "\\bcreativeWorldEpoch\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/view/ViewportState.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/CreativeFlyAnchorStore.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ProjectionRefresh.cpp
rg -n "\\bruntimeStateHash\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp /Users/kogaryu/iggy3d/src/app/iggy3d tests/unit --glob '*.cpp' --glob '*.hpp'
```

The second grep proves this card did not accidentally consume the separate
runtime-hash delete/rederive work.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_fly_tests|product_creative_world_launch_tests|product_vulkan_room_frame_tests|product_god_struct_ownership_coverage_tests|product_receipt_key_order_tests)$' --output-on-failure
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed.
- Exact field move.
- Grep results.
- Receipt golden result.
- Ownership coverage result.
- Focused/full test results.
- Confirmation that `runtimeStateHash` and `automationControl` were untouched.
- Any concerns/deferred work.

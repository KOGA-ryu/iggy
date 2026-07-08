# E191: Product Test Support G2 - Receipt Helpers

## Status

Ready.

## Context

E189 found repeated receipt-test helpers across product tests. E190 added the
small header-only assertion/numeric helper. This slice should add a second
small header-only helper for receipt construction and field checks, then migrate
only the highest-overlap creative UI receipt tests.

Keep this as test support, not a behavior harness. Do not hide the assertions or
expected receipt keys behind scenario methods.

## Scope

Add:

- `tests/unit/ProductReceiptTestSupport.hpp`

Migrate only these files in this slice:

- `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- `tests/unit/product_creative_ui_projection_receipt_tests.cpp`
- `tests/unit/product_creative_ui_frame_tests.cpp`
- `tests/unit/product_creative_ui_window_frame_tests.cpp`

Use the existing E190 helper where appropriate:

- `tests/unit/ProductTestSupport.hpp`

## Required Helper Shape

Use a small header-only namespace:

```cpp
namespace iggy3d::test {

struct ReceiptFieldExpectation {
  std::string_view key;
  std::string_view value;
  std::string_view message;
};

inline RenderReceipt receiptFor(const ProductAppWindowState& window);

inline bool expectReceiptField(const RenderReceipt& receipt,
                               std::string_view key,
                               std::string_view value,
                               std::string_view message);

inline bool expectReceiptCount(const RenderReceipt& receipt,
                               std::string_view key,
                               std::uint64_t value,
                               std::string_view message);

inline bool expectReceiptFields(
    const RenderReceipt& receipt,
    std::initializer_list<ReceiptFieldExpectation> expectations,
    std::string_view group);

template <std::size_t Size>
bool expectReceiptFields(
    const RenderReceipt& receipt,
    const std::array<ReceiptFieldExpectation, Size>& expectations,
    std::string_view group);

} // namespace iggy3d::test
```

The helper should build the same default receipt fixture currently repeated in
the migrated files:

- default `ProductAppOptions`;
- default `ProductWorldTemplate`;
- default `FrontendState`;
- default `FrontendSettings`;
- default `ProductSaveBridgeResult`;
- passed `ProductAppWindowState`.

Keep behavior equivalent:

- `expectReceiptField(...)` still delegates to `hasReceiptField(...)`;
- `expectReceiptCount(...)` still converts the integer with `std::to_string`;
- grouped receipt expectations still prefix the message with `group + ": "`;
- field expectation order in arrays/initializer lists is unchanged.

## Migration Policy

- Remove only duplicated receipt helpers that are replaced by
  `iggy3d::test::*`.
- It is acceptable to remove local `expect(...)` definitions in the migrated
  files when they are only used by receipt helpers and can be replaced by
  `ProductTestSupport.hpp`.
- Keep product-specific fixture/setup helpers local, for example:
  - `markCreativeAppIdentity(...)`;
  - `populateSelectedFacade(...)`;
  - `prepopulateCreativeProjection(...)`;
  - `prepopulateProductVulkanMenuUi(...)`;
  - scenario/window builders.
- Prefer explicit namespace qualification or narrow local `using`
  declarations. Avoid broad `using namespace`.
- Do not change receipt keys, expected values, test names, fixture data,
  production source, or CMake.

## Required Tests

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build \
  --target product_creative_ui_command_receipt_tests \
           product_creative_ui_projection_receipt_tests \
           product_creative_ui_frame_tests \
           product_creative_ui_window_frame_tests \
           product_receipt_key_order_tests -j10

ctest --test-dir /Users/kogaryu/iggy3d/build \
  -R '^(product_creative_ui_command_receipt_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_frame_tests|product_creative_ui_window_frame_tests|product_receipt_key_order_tests)$' \
  --output-on-failure

/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests

git -C /Users/kogaryu/iggy3d diff --check
```

Also run:

```sh
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
```

The golden diff must be empty. Also run a focused trailing-whitespace scan over
touched files.

No full CTest is required for builder on this slice unless the helper shape
widens beyond the four listed tests.

## Self-Blocking Criteria

Move this card to `blocked/` with evidence if:

- the shared receipt fixture does not match one of the migrated files;
- receipt key order or golden values change;
- migrating the four listed files requires production or CMake edits;
- the helper starts absorbing product scenario setup rather than receipt utility
  code;
- unrelated tests must be edited to make the listed targets compile.

## Non-Scope

- Do not migrate active-surface helpers.
- Do not migrate temp-root/options helpers.
- Do not migrate large harness/scenario structs.
- Do not migrate non-receipt product tests.
- Do not edit production source.
- Do not edit receipt golden.
- Do not stage, commit, push, or launch a window.

## Completion Brief Template

- Card moved to done:
- Files changed:
- Helper API added:
- Files migrated:
- Helpers intentionally left local:
- Receipt golden result:
- Tests/checks run:
- Concerns/deferred:

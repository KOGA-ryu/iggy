# E61: Product Create Panel Descriptor Palette

## Objective

Move product Creative Create rows toward a descriptor-driven palette instead of
hand-authored one-row-per-kind command catalog entries.

## Problem

E35 made command metadata centralized, but the product Create panel still gets
its object choices from explicit catalog rows:

- `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp:81` defines
  `creative.row.create.create_room`.
- `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp:88` defines
  `creative.row.create.create_crate`.
- `src/app/iggy3d/creative/ui/Ui.cpp:137` builds the Create panel by iterating
  catalog entries whose command kind is `CreateObject`.

That means a new creatable object kind still needs command-row metadata edits,
receipt/routing expectations, and tests, even when the descriptor already knows
the object's name, shape, defaults, editor/runtime visibility, and placement
support. Standalone already moved closer to descriptor-derived palette policy;
product Creative is now behind that seam.

## Dependencies

- Coordinate with E58, because E58 separates row catalog data from command-kind
  metadata. This card should not depend on ambiguous first-row kind lookups.
- Coordinate with E41 if descriptor-owned palette visibility lands first.

## Required Reads

- `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp`
- `src/app/iggy3d/creative/ui/Ui.cpp`
- `src/app/iggy3d/creative/tools/Placement.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `apps/iggy3d_creative/main.cpp` descriptor brush palette helpers, for
  precedent only
- `tests/unit/creative_ui_tests.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`

## Scope

- Add a descriptor-backed create-palette helper for product Creative UI.
- The first version can be conservative: keep the current visible rows
  (`Room`, `Crate`) plus prove the helper has an explicit filter policy.
- Generate create row id/semantic id/label from descriptor facts or a palette
  slot model, not by hand-authoring every `CreateObject` row in the command
  catalog.
- Keep `CreateObject` command execution generic: semantic row identifies a
  `CreativeObjectKind`, then `buildPlacedCreateRequest(...)` handles placement.
- Preserve current visible row order and labels unless the task explicitly
  broadens the palette.

## Acceptance

- Adding a descriptor-approved creatable kind no longer requires adding a new
  command catalog row by hand.
- The Create panel has a named descriptor/palette filter policy.
- Current `Create Room` and `Create Crate` behavior remains unchanged.
- Tests prove:
  - current visible rows are stable;
  - a palette slot maps to a valid descriptor;
  - command routing receives the intended `CreativeObjectKind`;
  - non-create command catalog metadata remains command-kind based, not
    per-object-row based.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_ui_tests product_creative_ui_command_frame_tests product_creative_ui_input_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_ui_tests|product_creative_ui_command_frame_tests|product_creative_ui_input_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not expose all 100+ descriptors in the product UI in one slice.
- Do not change create placement behavior.
- Do not remove external semantic ids for existing `Create Room` and
  `Create Crate` unless tests and input routing are updated deliberately.
- Do not combine this with E58's command-kind metadata cleanup unless the diff
  stays small.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp`
  - `src/app/iggy3d/creative/ui/Ui.cpp`
  - `tests/unit/creative_ui_tests.cpp`
  - `tests/unit/product_creative_ui_command_frame_tests.cpp`
- Implementation:
  - Moved Create-object rows out of the static command catalog into
    `productCreativeUiCreatePalette()`.
  - Added `productCreativeUiCreatePaletteEntryAllowed(...)` as the named
    product Create panel filter policy. It validates the palette slot against
    descriptor truth, keeps Room metadata as an explicit room-container create
    case, and requires non-Room slots to satisfy the authoring brush predicate
    plus transform/bounds support.
  - Updated the Create panel to iterate the create palette instead of scanning
    command catalog rows by `CreateObject`.
  - Kept existing external semantic ids and labels:
    `creative.row.create.create_room` / `Create Room` and
    `creative.row.create.create_crate` / `Create Crate`.
  - Command routing still resolves those semantic ids generically to
    `CreateObject` plus the intended `CreativeObjectKind`; placement remains
    handled by `buildPlacedCreateRequest(...)`.
- Tests:
  - `creative_ui_tests` now proves Create panel rows follow the product create
    palette and each slot maps to a valid descriptor.
  - `product_creative_ui_command_frame_tests` now proves each create palette
    row routes to and creates the descriptor-intended object kind.
  - Command-kind metadata remains independent of row order, with Create rows
    outside the command catalog.
- Verification:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing whitespace scan over touched files passed.
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_ui_tests product_creative_ui_command_frame_tests product_creative_ui_input_frame_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_ui_tests|product_creative_ui_command_frame_tests|product_creative_ui_input_frame_tests)$' --output-on-failure` passed.
- Concerns:
  - The first product Create palette remains intentionally conservative:
    Room and Crate only. Widening visible product Create rows should happen in
    a later explicit UI/product-scope slice.

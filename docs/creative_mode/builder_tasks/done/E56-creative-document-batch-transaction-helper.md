# E56: Creative Document Batch Transaction Helper

## Objective

Extract the ad hoc “copy document, apply several creates, install copy” pattern
from the Generate Room Shell command into a reusable Creative document batch
transaction helper.

## Problem

Generate Room Shell is the first multi-object product-live Creative command. Its
command handler currently stages atomicity by hand:

- copy the current document;
- loop `stagedDocument.createObject(...)`;
- stop on any rejected/no-change create;
- call `facade.installDocument(std::move(stagedDocument))`;
- translate create/install failures into shell-specific receipt status.

Evidence: `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp:276-299`.

That pattern is not Room-Shell-specific. The next multi-object command
duplicate, paste, import, generated layout, regenerate shell, grouped delete,
or template placement will need the same all-or-nothing document mutation
shape. Leaving it embedded in one UI command makes every future feature choose
between copy/paste transaction logic and unsafe partial mutation.

## Dependencies

- Avoid overlapping with E35 while `UiCommandFrame.cpp` is claimed. If E35 is
  still in progress, leave this card ready for later.

## Required Reads

- `src/app/iggy3d/creative/Facade.hpp`
- `src/app/iggy3d/creative/Facade.cpp`
- `src/app/iggy3d/creative/document/Document.hpp`
- `src/app/iggy3d/creative/document/Document.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
- `src/app/iggy3d/creative/tools/RoomShell.hpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`
- `tests/unit/creative_document_create_tests.cpp`

## Scope

- Add a focused helper for all-or-nothing batch document edits. A narrow first
  version can support a vector of `CreativeDocumentCreateRequest`s because that
  is the current repeated need.
- The helper should preserve the current behavior of applying the batch to a
  document copy and installing it only after all creates succeed.
- Return a typed receipt with:
  - requested/accepted/changed;
  - revision before/after;
  - attempted/applied create count;
  - first failed create index/status/reason;
  - install receipt/status when install is attempted.
- Use the helper from Generate Room Shell instead of open-coding the loop in
  `UiCommandFrame.cpp`.
- Keep command-specific shell receipt fields externally stable.

## Acceptance

- `UiCommandFrame.cpp` no longer owns the generic copy/loop/install transaction
  algorithm for Room Shell.
- Generate Room Shell still installs all five generated children atomically.
- A forced create rejection in a batch leaves the original facade document
  unchanged and reports the failed index/reason.
- A forced/no-change install leaves the original facade document unchanged and
  reports install failure.
- Tests prove both success and partial-failure prevention at the helper level,
  not only through shell-command receipt fields.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_command_frame_tests creative_document_create_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_frame_tests|creative_document_create_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not make batch create mutate the live document incrementally.
- Do not widen this into a full undo/redo or general command-stack rewrite.
- Do not change Room Shell generated bounds, tags, parent ids, or receipt keys.
- Do not combine this with Room Shell lifecycle/delete/regenerate work from E39.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/Facade.hpp`
  - `src/app/iggy3d/creative/Facade.cpp`
  - `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
  - `tests/unit/creative_document_create_tests.cpp`
- Added `CreativeFacadeDocumentBatchCreateStatus` and
  `CreativeFacadeDocumentBatchCreateReceipt`.
- Added `Facade::createDocumentObjectsAtomically(...)`.
- The helper stages creates on a `CreativeDocument` copy, records attempted and
  applied create counts, records the first failed create index/status/reason,
  and calls `installDocument(...)` only after all creates succeed.
- Generate Room Shell now calls the helper and maps generic create/install
  failures back to the existing shell receipt status/reason fields.
- Command-specific Room Shell receipt keys and generated bounds/tags/parent ids
  were kept stable.
- Tests added:
  - batch create applies all requests atomically;
  - staged create rejection preserves the live facade document and reports the
    failed index/reason;
  - install rejection preserves the live facade document and reports install
    failure.
- Verification:
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_command_frame_tests creative_document_create_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_frame_tests|creative_document_create_tests)$' --output-on-failure`
- Result: all checks passed.

# E51: Descriptor Hide Lock Tag Capability Truth

## Objective

Make `canBeHidden`, `canBeLocked`, and `canBeTagged` real descriptor facts, or
remove them if the intended policy is that every valid object always supports
those operations.

## Problem

`CreativeObjectDescriptor` exposes `canBeHidden`, `canBeLocked`, and
`canBeTagged`, and `CreativeDocument::createObject(...)` validates visible,
locked, and tag overrides against those fields.

But the local `descriptor(...)` helper in `ObjectDescriptor.cpp` does not accept
those capability values; it always writes `true, true, true`. That means the
fields look like an object-kind policy seam but currently cannot express any
non-default policy. Tests only pin positive defaults for Room.

This is feature-add friction: future descriptors can appear to set capability
policy while the helper silently keeps every object hideable, lockable, and
taggable.

## Required Reads

- `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
- `src/app/iggy3d/creative/document/Document.cpp`
- `tests/unit/creative_object_descriptor_tests.cpp`
- `tests/unit/creative_document_create_tests.cpp`

## Scope

- Decide and implement one policy:
  - add named capability options/flags to descriptor row construction, or
  - remove/deprecate the dead capability fields and validators if universal
    support is the intended model.
- Keep behavior unchanged unless adding a focused non-default descriptor case is
  explicitly chosen.
- Add tests that prove the chosen policy is intentional.

## Acceptance

- A reviewer can tell whether hide/lock/tag capability is real descriptor truth
  or intentionally universal.
- If the fields remain, at least one focused test proves a non-default
  capability rejects the corresponding create override.
- Descriptor row construction no longer hides these fields behind hardcoded
  `true` values.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_create_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_create_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not use raw trailing boolean expansion if E40 has not landed yet.
- Do not change user-visible UI policy without tests.
- Do not remove visibility/lock/tag mutation receipts in this slice.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`
  - `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`
  - `src/app/iggy3d/creative/document/Document.cpp`
  - `tests/unit/creative_object_descriptor_tests.cpp`
  - `tests/unit/creative_document_create_tests.cpp`
- Chosen policy: visibility, lock state, and tags are universal authored-object
  state for valid creative objects, not per-kind descriptor capabilities.
- Removed dead descriptor fields:
  - `canBeHidden`
  - `canBeLocked`
  - `canBeTagged`
- Removed unreachable create validators/reasons for unsupported visibility,
  lock, and tag overrides.
- Kept behavior unchanged for valid descriptors: create overrides for visible,
  locked, and tags still apply.
- Added a focused create test proving the universal authored-state policy across
  representative profiles:
  - `PointLight`
  - `Note`
  - `PatrolRoute` with valid path payload
- Fixed the new test to avoid holding object pointers across later document
  insertions.

## Verification

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_create_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_create_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.

All verification passed.

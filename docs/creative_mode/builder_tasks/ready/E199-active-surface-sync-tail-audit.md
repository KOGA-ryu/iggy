# E199: Active Surface Sync Tail Audit

## Status

Ready.

## Context

The old window input-owner cache fields were deleted. The remaining helper
`syncProductWindowInputOwnerFromActiveSurface(...)` appears to be a stale name:
it now resolves and returns a `ProductActiveSurfaceFrame`, but does not write
input-owner or gameplay-suppression state back into `ProductAppWindowState`.

Quick reviewer evidence before this card:

- `src/app/iggy3d/menu/FrontendRouter.cpp:384-390` currently calls
  `resolveProductActiveSurface(productActiveSurfaceContextForWindow(...))` and
  returns the frame.
- many production call sites call it as a bare statement and ignore the return;
- test helpers and a few focused tests still consume the returned frame.

This card is read-only. Do not remove calls or rename the helper in this card.

## Objective

Classify all remaining `syncProductWindowInputOwnerFromActiveSurface(...)`
callers so the next implementation can safely remove no-op call sites and/or
rename the helper without changing active-surface routing behavior.

## Scope

Inspect only:

- `src/app/iggy3d/menu/FrontendRouter.hpp`
- `src/app/iggy3d/menu/FrontendRouter.cpp`
- all production call sites under `src/app/iggy3d`
- direct test/support call sites under `tests/unit`

Do not edit source, tests, CMake, docs outside this task card, or receipt
golden.

## Required Inventory

Document:

- the current implementation of `syncProductWindowInputOwnerFromActiveSurface`;
- whether it mutates `ProductAppWindowState`;
- every production call site that ignores the return;
- every production call site that consumes the return, if any;
- every test/support call site that consumes the return;
- whether `tests/unit/ProductActiveSurfaceTestSupport.hpp::liveSurface(...)`
  should keep wrapping the helper or switch to `resolveProductActiveSurface(...)`
  directly in a follow-up;
- whether the helper name is now misleading enough to rename/delete;
- which focused tests cover active-surface routing if callers are removed.

## Required Commands

Run and summarize:

```sh
rg -n "syncProductWindowInputOwnerFromActiveSurface\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d \
  /Users/kogaryu/iggy3d/tests/unit \
  --glob '*.cpp' --glob '*.hpp'

rg -n "const .*syncProductWindowInputOwnerFromActiveSurface|=\\s*syncProductWindowInputOwnerFromActiveSurface|return syncProductWindowInputOwnerFromActiveSurface|\\(void\\)syncProductWindowInputOwnerFromActiveSurface" \
  /Users/kogaryu/iggy3d/src/app/iggy3d \
  /Users/kogaryu/iggy3d/tests/unit \
  --glob '*.cpp' --glob '*.hpp'

rg -n "inputOwner|gameplayInputSuppressed|ProductActiveSurfaceFrame|resolveProductActiveSurface" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/menu/FrontendRouter.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/menu/FrontendRouter.hpp \
  /Users/kogaryu/iggy3d/tests/unit/product_frontend_router_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_window_input_frame_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_menu_transitions_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_starter_menu_action_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/ProductActiveSurfaceTestSupport.hpp

git -C /Users/kogaryu/iggy3d diff --check
```

No build or CTest is required because this card is read-only.

## Decision Buckets

Classify the next implementation as one of:

1. **Remove Bare No-Ops, Keep Return Helper**: production bare calls are no-ops,
   but return-consuming tests/support still need the helper for now.
2. **Rename To Resolve Helper And Repoint Tests**: the helper should be renamed
   or replaced by direct `resolveProductActiveSurface(...)` usage, with bare
   production calls removed.
3. **Still Mutating Through A Hidden Seam**: stop and report evidence if any
   call still depends on mutation or side effects.

## Draft Follow-Up Card

Append a draft implementation card to this done card. The draft must include:

- exact production call sites to remove;
- exact test/support call sites to keep, rename, or repoint;
- whether the helper declaration/definition should remain;
- focused tests to run;
- self-blockers.

Do not create the follow-up card in `ready/`; reviewer will promote it after
reviewing this audit.

## Completion Brief Requirements

Report:

- call-site counts by category: ignored production, consumed production,
  consumed tests/support;
- exact file/function buckets for ignored production calls;
- implementation decision bucket;
- draft follow-up card title and scope;
- commands run;
- confirmation that no source/test/CMake files were edited.

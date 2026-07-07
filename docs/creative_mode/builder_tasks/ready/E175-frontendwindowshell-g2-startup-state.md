# E175 - FrontendWindowShell G2: Startup State

**STATUS: READY.** Parent: `blocked/E164-frontendwindowshell-ruling.md`.
Depends on completed `done/E174-frontendwindowshell-g1-scalar-menu-state.md`.

## Goal

Move only `ProductAppWindowState::startup` into the existing
`ProductAppWindowState::frontendShell`.

E174 already created `iggy3d::FrontendWindowShell` and moved the lower-risk
scalar/menu/status fields. This card should continue that same structural
FrontendWindowShell lane without touching `productVulkanMenu` or unrelated
window lifecycle fields.

## Move Exactly This Field

- `startup`

The field type and default construction must remain exactly equivalent:
`ProductStartupState startup;`

## Required Shape

1. Add `ProductStartupState startup;` to
   `src/app/iggy3d/window/FrontendWindowShell.hpp`.
2. Remove the flat `ProductAppWindowState::startup` member from
   `src/app/iggy3d/ProductAppWindowState.hpp`.
3. Repoint only `ProductAppWindowState` storage reads/writes from
   `window.startup` / `request.window.startup` / fixture-window equivalents to
   `window.frontendShell.startup`.
4. Update `docs/god_struct_member_ownership.tsv`:
   - delete the top-level `startup	FrontendWindowShell` row;
   - keep the existing `frontendShell	FrontendWindowShell` row.
5. Preserve receipt key names, receipt ordering, and values.

Use compiler-guided repoints. Do not use broad token replacement; `startup` is
too generic.

## Do Not Move

Do not move:

- `productVulkanMenu`
- `automationControl`
- `runtimeStateHash`
- `creativeWorldEpoch`
- `requested`
- `sdlAvailable`
- `created`
- `drawable`
- existing store members such as `inputDevice`, `debugHud`, `creativeAuthoring`,
  `gameplay`, `room`, `saveSession`, `viewport`, and `presentPath`

`productVulkanMenu` remains the next deferred FrontendWindowShell slice.
`automationControl` stays flat and TSV-owned by `app-global-remainder`.

## Expected Hot Files

Use the compiler and greps as truth, but expect the hottest production files to
include:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/window/FrontendWindowShell.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/window/Loop.cpp`
- `src/app/iggy3d/receipt/StartupProbeFields.cpp`
- focused product creative/world launch and wireframe tests that seed/assert
  startup receipt data

## Required Greps

After implementation, these must produce no output:

```sh
rg -n "window\\.startup\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'

rg -n "\\bstartup\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp

rg -n "^startup\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Also report these checks:

```sh
rg -n "^frontendShell\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
rg -n "^automationControl\\s+app-global-remainder$" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
rg -n "window\\.(productVulkanMenu|automationControl|runtimeStateHash|creativeWorldEpoch)\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
```

The final grep should remain non-empty because those fields are intentionally
retained/deferred.

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
- confirmation that only `startup` moved;
- confirmation that `productVulkanMenu`, `automationControl`,
  `runtimeStateHash`, and `creativeWorldEpoch` stayed flat;
- receipt golden result;
- ownership coverage result;
- required grep results;
- full suite result;
- any follow-up needed for `productVulkanMenu`.

Do not stage, commit, push, or launch a window.

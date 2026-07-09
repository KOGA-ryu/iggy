# File Spec

Files: `src/app/iggy3d/Options.hpp`, `src/app/iggy3d/Options.cpp`

Verified at: `e3d742db`

## Owns

- Product app command-line option data model, parse status model, defaults, parsing, enum display names, parse status reason strings, and help text.
- Default save-root selection from `$HOME` with local fallback.
- CLI spellings for renderer, window mode, input backend, save root, frame/hold counts, auto-new-world, scripted smoke, receipt/debug flags, dev package/scenario, automation control, and gameplay tape.

## Does Not Own

- Execution of parsed options.
- App shell exit policy for parse results.
- Window loop frame limiting behavior.
- Save root filesystem creation or save scanning.
- Automation command file parsing.

## Reads

- `argc` and `argv`.
- `$HOME` environment variable for default save root.
- Numeric option strings for unsigned integer parsing.

## Writes / Mutates

- Returns `ProductAppOptionsParseResult`.
- Mutates only local parse result state while parsing.
- Does not mutate filesystem state or global app state.

## Calls Out To / Wires Out To

- `std::getenv(...)` for default save root.
- `std::from_chars(...)` for unsigned integer parsing.
- `AppShell.cpp` consumes parse results, help text, and status reason strings.
- Many app modules consume `ProductAppOptions` fields after shell/kernel handoff.

## Called By / Entry Points

- `AppShell.cpp` calls `parseProductAppOptions(...)`, `productAppHelpText(...)`, and `productAppOptionStatusReason(...)`.
- Tests and support helpers construct `ProductAppOptions` directly.
- Focused proof: `rg -n "parseProductAppOptions|defaultProductAppOptions|productAppHelpText|ProductAppOptions" src/app tests/unit`.

## Invariants

- Defaults request Vulkan renderer, window mode, auto input backend, default scenario, no automation, no gameplay tape, no receipt printing, and no debug overlay.
- `--scripted-gameplay-smoke` also enables auto-new-world and auto input backend.
- Missing option values return `MissingOptionValue` with the option flag.
- Invalid renderer/input values return their specific invalid status with the bad value.
- Invalid numeric values currently use `UnknownOption` with the bad value.
- Help status returns immediately and does not parse later args.

## Tests / Proof Commands

- `rg -n "product_app_options_tests|parseProductAppOptions|productAppHelpText" cmake/iggy3d_tests.cmake tests/unit/product_app_options_tests.cpp`.
- `rg -n "--automation-control|--gameplay-tape|--debug-overlay|--scripted-gameplay-smoke" src/app/iggy3d/Options.* tests/unit/product_app_options_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/AppShell.*` unless parse status handling or exit policy changes.
- `src/app/iggy3d/AppKernel.*` unless parsed option execution order changes.
- `src/app/iggy3d/window/*` unless a window option changes runtime meaning.
- `src/app/iggy3d/automation/*` unless automation option semantics change.

## Update When

- CLI flags, defaults, parse statuses, help text, option enum names, or option field semantics change.

## Do Not Update When

- Only downstream behavior changes for an existing option without changing its parse/default contract.

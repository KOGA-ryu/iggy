# File Spec

Files: `src/app/iggy3d/AppShell.hpp`, `src/app/iggy3d/AppShell.cpp`

Verified at: `e3d742db`

## Owns

- Product app process entry bridge from `main(...)` into option parsing and `AppKernel`.
- CLI parse-result handling for help, parse failure receipt output, and successful kernel launch.
- Process exit code policy for help and invalid command-line options before the app kernel runs.

## Does Not Own

- Command-line option parsing rules.
- App lifetime orchestration after options are valid.
- Window creation, frame loop, save scanning, automation, gameplay, creative, or receipt construction for normal app runs.
- `main(...)` definition.

## Reads

- `argc` and `argv`.
- `ProductAppOptionsParseResult` from `parseProductAppOptions(...)`.
- Option status reason strings and help text from `Options.*`.

## Writes / Mutates

- Writes help text or parse-failure receipt text to `std::cout`.
- Constructs a local `AppKernel` for valid option runs.
- Does not mutate repo state, app state, save files, or runtime state directly.

## Calls Out To / Wires Out To

- `parseProductAppOptions(...)`.
- `productAppHelpText(...)`.
- `productAppOptionStatusReason(...)`.
- `appendReceiptField(...)` and `formatRenderReceipt(...)` for parse-failure receipts.
- `AppKernel::run(...)`.

## Called By / Entry Points

- `apps/iggy3d/main.cpp` calls `runProductApp(argc, argv)`.
- Focused proof: `rg -n "runProductApp\\(|parseProductAppOptions|productAppHelpText|AppKernel" apps src/app/iggy3d tests`.

## Invariants

- Help exits with status `0` and does not run the app kernel.
- Invalid options emit a receipt with `app`, `result`, `reason_code`, and `option`, then exit with status `2`.
- Valid options are handed to a fresh `AppKernel`.
- This file stays a thin shell; lifecycle and domain logic belong below `AppKernel` or domain modules.

## Tests / Proof Commands

- `rg -n "product_app_options_tests|parseProductAppOptions|productAppHelpText" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "runProductApp\\(" apps src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/AppKernel.*` unless app lifecycle orchestration changes.
- `src/app/iggy3d/Options.*` unless CLI parse semantics change.
- `apps/iggy3d/main.cpp` unless executable entry plumbing changes.

## Update When

- Parse failure receipt fields, app-shell exit code policy, help behavior, or handoff into `AppKernel` changes.

## Do Not Update When

- Only kernel internals, window loop behavior, save behavior, or command-line option definitions change without changing shell policy.

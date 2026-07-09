# File Spec

File: `src/app/iggy3d/ProductStartupState.hpp`

Verified at: `a48d779a`

## Owns

- Startup and first-frame measurement packet embedded in `FrontendWindowShell`.
- Package lookup/load path and status probes.
- Runtime session create timing and status probes.
- Creative world/document ID scan probes.
- Creative UI first-frame, creative wireframe first-frame, Vulkan renderer init, and Vulkan first-submit probes.

## Does Not Own

- Package loading, runtime session creation, creative world service behavior, renderer creation, UI projection, or receipt serialization.

## Reads

- No live inputs; this header defines a packet.
- Receipt and tests read its fields through `window.frontendShell.startup`.

## Writes / Mutates

- Creative launch paths write creative ID scan and blank-stage session probes.
- Product session launch writes runtime session creation probes.
- Renderer and creative frame paths write first-frame and renderer timing probes.
- Receipt field appenders read and emit the values.

## Calls Out To / Wires Out To

- No calls; embedded by `FrontendWindowShell`.
- Wired to receipt fields through `StartupProbeFields.cpp`.

## Called By / Entry Points

- Reached through `ProductAppWindowState.frontendShell.startup`.
- Grep proof: `rg -n "frontendShell\\.startup|ProductStartupState|vulkanRendererInit|creativeUiFirstFrame|creativeWireframeFirstFrame|runtimeSessionCreate|packageLookup|packageLoad" src/app/iggy3d tests/unit`.

## Invariants

- This is observability state, not launch policy.
- Timing booleans and microsecond values must stay paired with stable status strings.
- Startup probes must not become save/hash/runtime truth.
- Creative and product launch probes share the packet but keep their own status meanings.

## Tests / Proof Commands

- `rg -n "product_creative_world_launch_tests|product_creative_wireframe_frame_tests|StartupProbeFields" cmake/iggy3d_tests.cmake tests/unit src/app/iggy3d/receipt`.
- `rg -n "creativeWorldIdScan|creativeDocumentIdScan|runtimeSessionCreateStatus|vulkanFirstSubmitStatus" src/app/iggy3d tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/CreativeWorldOperations.*` unless creative launch probes change.
- `src/app/iggy3d/creative/CreativeBlankStageSession.*` unless blank-stage startup probes change.
- `src/app/iggy3d/world/ProductSessionLaunch.*` unless product runtime launch probes change.
- `src/app/iggy3d/receipt/StartupProbeFields.cpp` unless emitted receipt fields change.

## Update When

- Startup probe fields, default statuses, timing semantics, or receipt-facing meanings change.

## Do Not Update When

- Only the implementation that writes an existing probe changes without changing field meaning.

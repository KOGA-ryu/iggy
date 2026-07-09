# File Spec

Files: `src/app/iggy3d/receipt/StartupWorldBuildoutFields.cpp`

Verified at: `0fe74f9c`

## Owns

- Composition of startup probe, world authoring, and active room receipt appenders into one buildout block.

## Does Not Own

- Individual startup probe field tables.
- World authoring field tables.
- Active room field tables.
- Top-level full receipt order beyond this grouped block.

## Reads

- `FrontendState`, `ProductAppWindowState`, and `ProductSaveBridgeResult` only to pass them to sub-appenders.

## Writes / Mutates

- Appends fields to `RenderReceipt` through sub-appenders.
- Does not mutate input state.

## Calls Out To / Wires Out To

- `appendProductStartupProbeFields(...)`.
- `appendProductWorldAuthoringFields(...)`.
- `appendProductActiveRoomFields(...)`.

## Called By / Entry Points

- `buildProductAppReceipt(...)` calls `appendProductStartupWorldBuildoutFields(...)`.
- Focused proof: `rg -n "appendProductStartupWorldBuildoutFields|appendProductStartupProbeFields|appendProductWorldAuthoringFields|appendProductActiveRoomFields" src/app tests`.

## Invariants

- Buildout block order is startup probe, world authoring, then active room.
- This file remains a composition seam, not a new field-table owner.
- If one sub-appender changes its contract, update that sub-appender spec first.

## Tests / Proof Commands

- `rg -n "product_receipt_key_order_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "startup_package_lookup_status|world_setup_status|active_room_loaded" tests/unit tests/smoke src/app/iggy3d/receipt`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/receipt/StartupProbeFields.cpp`.
- `src/app/iggy3d/receipt/WorldAuthoringFields.cpp`.
- `src/app/iggy3d/receipt/ActiveRoomFields.cpp`.

## Update When

- Startup/world/active-room block composition or ordering changes.

## Do Not Update When

- Only fields inside one composed appender change without changing this grouping.

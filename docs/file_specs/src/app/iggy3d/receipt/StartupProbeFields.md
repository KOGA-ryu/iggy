# File Spec

Files: `src/app/iggy3d/receipt/StartupProbeFields.cpp`

Verified at: `0fe74f9c`

## Owns

- Receipt field emission for launch status, package lookup/load timing, runtime session creation timing, save catalog scan timing, creative id scans, creative first-frame probes, and Vulkan startup probes.
- Table-driven startup probe receipt key mapping.

## Does Not Own

- Package lookup or loading.
- Runtime session creation.
- Save catalog scanning.
- Creative UI or wireframe first-frame measurement.
- Vulkan renderer initialization.

## Reads

- `FrontendState.devToolsCategory`.
- `ProductAppWindowState.frontendShell` launch/startup probe fields.
- `ProductSaveBridgeResult` scan measurement fields.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate frontend, window, save bridge result, or startup probe state.

## Calls Out To / Wires Out To

- `appendReceiptField(...)`.
- `frontendDevToolsCategoryName(...)`.

## Called By / Entry Points

- `appendProductStartupWorldBuildoutFields(...)` calls `appendProductStartupProbeFields(...)`.
- Focused proof: `rg -n "appendProductStartupProbeFields|startup_package_lookup_status|startup_save_catalog_scan_status|launch_status" src/app tests`.

## Invariants

- Startup probe fields are observational receipts over measurements produced elsewhere.
- Save catalog scan fields read the save bridge result, not the filesystem.
- Creative and Vulkan startup probes remain separate status families.
- This appender must not perform timing or startup work.

## Tests / Proof Commands

- `rg -n "product_receipt_key_order_tests|product_menu_usefulness_smoke|product_startup_lifecycle_smoke|product_ascii_map_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "launch_status|startup_package_lookup_status|startup_vulkan_renderer_init_status" tests/unit tests/smoke src/app/iggy3d/receipt`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/RendererLifecycle.*` unless startup probe fields change.
- `src/app/iggy3d/save/SaveBridge.*` unless scan measurement fields change.
- `src/app/iggy3d/world/ProductSessionLaunch.*` unless package/session startup proof fields change.

## Update When

- Startup probe receipt keys, source measurement fields, or startup buildout grouping changes.

## Do Not Update When

- Only package load behavior, save scan internals, or Vulkan backend behavior changes without changing startup probe fields.

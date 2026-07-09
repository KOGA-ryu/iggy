# File Spec

File: `src/app/iggy3d/window/FrontendWindowShell.hpp`

Verified at: `bdb9e108`

## Owns

- Product frontend/window shell state packet embedded in `ProductAppWindowState`.
- Opening-menu draw proof, input selection proof, launch/package status mirrors, startup probe state, Vulkan menu proof, frame/event counts, row count, selected settings tab, and current shell status.

## Does Not Own

- Menu routing, session launch behavior, startup measurement implementation, Vulkan menu draw-list construction, renderer lifecycle, save/catalog state, or receipt serialization.

## Reads

- No live inputs; this header defines a packet type.
- Callers read fields to build receipts, tests, and presentation/debug assertions.

## Writes / Mutates

- Callers mutate this packet from menu actions, world/creative launch, ASCII activation, window loop, frame presenter, automation, tape runner, renderer lifecycle, and receipt/projection setup paths.
- This file itself has no behavior.

## Calls Out To / Wires Out To

- Includes `ProductStartupState` and `ProductVulkanMenuState`.
- Carries `FrontendSettingsTab` for selected settings tab proof.

## Called By / Entry Points

- Instantiated through `ProductAppWindowState`.
- Grep proof: `rg -n "FrontendWindowShell|frontendShell\\." src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- This is a state packet only; behavior belongs in the owning modules that write it.
- Launch/package status fields are product-facing proof mirrors and should use stable strings.
- Frame/event/menu draw counters are loop/presenter facts, not runtime simulation facts.
- Startup and Vulkan menu nested packets keep their own field ownership.

## Tests / Proof Commands

- `rg -n "product_god_struct_ownership_coverage_tests|product_window_input_frame_tests|product_creative_world_launch_tests|product_starter_menu_action_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "frontendShell\\.launchStatus|frontendShell\\.startup|frontendShell\\.productVulkanMenu|framesPresented|eventPollCount" src/app/iggy3d tests/unit tests/smoke`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ProductAppWindowState.hpp` unless the packet embedding changes.
- `src/app/iggy3d/ProductStartupState.hpp` unless startup probes change.
- `src/app/iggy3d/window/ProductVulkanMenuState.hpp` unless Vulkan menu proof changes.

## Update When

- Frontend shell fields, default values, nested packet ownership, or stable proof meanings change.

## Do Not Update When

- Only the behavior that writes existing fields changes without changing the packet contract or field meaning.

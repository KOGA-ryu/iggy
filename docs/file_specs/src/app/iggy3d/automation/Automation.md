# File Spec

Files: `src/app/iggy3d/automation/Automation.hpp`, `src/app/iggy3d/automation/Automation.cpp`

Verified at: `67587af2`

## Owns

- Product automation command vocabulary, categories, value kinds, command ids, canonical keys, aliases, and dispatch metadata.
- Primitive automation value parsing for bools, floats, CSV fields, menu input, frontend actions, settings tabs, dev-tools categories, save selection, dungeon draft values, room editor tool/direction/action names, and owner names.
- Reading `key=value` automation command files into `ProductAutomationCommand` rows and recording parse/load status in automation-control state.
- Common automation handlers for owner assertions, menu routing, frontend selection, settings tab selection, and dev-tools category selection.
- World setup, dungeon draft, ASCII room draft, ASCII room preview build, and ASCII room activation automation handlers.
- `markAutomationApplied(...)` as the shared write path for applied-command receipt/control facts.

## Does Not Own

- The top-level automation file execution loop.
- App-context dispatch fan-out across gameplay, save-browser, room-editing, and system handlers.
- Runtime gameplay mutation beyond world setup draft and ASCII preview activation hooks passed in by context.
- Product save/delete execution.
- Room editor command execution.

## Reads

- `ProductAutomationCommandRegistry` rows and aliases.
- `FrontendState`, `WorldSetupDraft`, `ProductAppWindowState`, settings/dev-tools enums, builtin dungeon catalog, dungeon draft helpers, ASCII room preview helpers, and room editor command parsers.
- Automation command files from a caller-provided filesystem path.

## Writes / Mutates

- `ProductAppWindowState.automationControl` load, parse, status, command, owner, and applied-count fields.
- `FrontendState.selectedAction`, `FrontendState.devToolsCategory`, and selected settings tab through common handlers.
- `WorldSetupDraft` title, dungeon id, ASCII room draft fields, dungeon draft edits, and associated `window.creativeAuthoring` proof fields.
- ASCII room draft and preview state in `window.creativeAuthoring`.

## Calls Out To / Wires Out To

- `routeInput(...)` callback for menu and world create confirmation.
- `activateAsciiRoom(...)` callback for ASCII preview activation.
- `buildProductAsciiRoomPreview(...)`, `buildProductAsciiRoomPreviewResult(...)`, `decodeProductAsciiRoomAutomationText(...)`.
- `productBuiltinDungeonCatalog(...)`, `productBuiltinDungeonIndexForRoomId(...)`, `applyProductBuiltinDungeonToDraft(...)`.
- Dungeon draft helpers such as `moveProductDungeonDraftCursor(...)`, `paintProductDungeonDraftCell(...)`, and `setProductDungeonDraftCell(...)`.

## Called By / Entry Points

- `AutomationDispatch.cpp` calls `makeProductAutomationCommandRegistry(...)`, `resolveProductAutomationCommandDispatch(...)`, `applyProductCommonAutomationCommand(...)`, and `applyProductWorldSetupAutomationCommand(...)`.
- `AutomationControl.cpp` calls `readProductAutomationCommands(...)`.
- Save-browser, gameplay, room-editing, and system handlers call parser helpers and `markAutomationApplied(...)`.
- Focused proof: `rg -n "makeProductAutomationCommandRegistry|resolveProductAutomationCommandDispatch|readProductAutomationCommands|applyProductCommonAutomationCommand|applyProductWorldSetupAutomationCommand" src/app/iggy3d/automation tests/unit`.

## Invariants

- Registry rows define all known keys and aliases; unknown keys resolve to the explicit unknown row.
- Dispatch rows are a handled subset of registry rows; registry-only metadata must not imply runtime support.
- `ignoresFalseBool` commands treat false as ignored, not failed.
- Automation command files are one command per non-empty `key=value` line; duplicate keys fail the load.
- World setup commands only mutate a New World child screen; wrong owner/screen reports owner unavailable.
- `markAutomationApplied(...)` increments applied count only for `applied`.

## Tests / Proof Commands

- `rg -n "product_automation_command_registry_tests|product_automation_dispatch_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "resolveProductAutomationBool|resolveProductGameplayAxisAutomation|resolveProductDungeonDraft|resolveProductSaveBrowserBoolAutomation" tests/unit/product_automation_command_registry_tests.cpp`.
- `rg -n "world.ascii_room_file|world.draft_cell|frontend.physics_movement|controller.input" tests/unit/product_automation_command_registry_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/automation/AutomationDispatch.*` unless handler routing order or app-context callbacks change.
- `src/app/iggy3d/automation/AutomationGameplay.*`, `AutomationSaveBrowser.*`, `AutomationRoomEditing.*`, and `AutomationSystem.*` unless those domains add or consume command ids.
- `src/app/iggy3d/world/DungeonDraft.*` unless dungeon draft semantics change.
- `src/app/iggy3d/ascii_room/*` unless ASCII preview/build semantics change.

## Update When

- Automation command keys, aliases, categories, value kinds, parser behavior, file-read contract, common handlers, or world setup/ASCII automation behavior changes.

## Do Not Update When

- Only downstream gameplay, save-browser, room-editor, or system handler internals change without changing shared registry/parser/common/world setup contracts.

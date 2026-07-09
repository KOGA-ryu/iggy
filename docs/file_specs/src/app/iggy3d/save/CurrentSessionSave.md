# File Spec

Files: `src/app/iggy3d/save/CurrentSessionSave.hpp`, `src/app/iggy3d/save/CurrentSessionSave.cpp`

Verified at: `a200d76e`

## Owns

- Current product runtime-session manual save request construction.
- Product save write proof recording into `ProductAppWindowState.saveSession`.
- Carry-forward save identity hinting for existing active product save ids.

## Does Not Own

- Pause-menu flow status selection.
- Durable save file writing or save codec behavior.
- CreativeDocument save behavior.
- Runtime session mutation.

## Reads

- `ProductAppOptions.saveRoot`.
- Optional active `Session` state.
- `ProductAppWindowState.saveSession.activeProductSaveId`.
- Active room authored-room payload through `activeRoom(window)`.

## Writes / Mutates

- Updates `window.saveSession.productSaveStatus`.
- Updates `window.saveSession.productSaveReasonCode`.
- Updates `window.saveSession.productSaveDurableReason`.
- Updates `window.saveSession.productSaveSource`.
- Updates `window.saveSession.productSaveSaveId`.
- Updates `window.saveSession.productSaveSessionSaved`.
- Updates `window.saveSession.activeProductSaveId` after successful writes with a non-empty record id.

## Calls Out To / Wires Out To

- `writeProductSessionSaveDurably(...)`.
- `productSaveTimestampNowUtc()`.
- `activeRoom(window)`.

## Called By / Entry Points

- `src/app/iggy3d/save/Flow.cpp` uses `writeProductCurrentSessionSave(...)` for pause-save product session writes.
- Focused proof: `rg -n "writeProductCurrentSessionSave|productSaveStatus|activeProductSaveId" src/app/iggy3d/save src/app/iggy3d/receipt tests/unit`.

## Invariants

- Missing active session records `product_save_session_missing` and does not request durable I/O.
- Save id hint is omitted when active product save id is `none`.
- Authored room data is attached only when the active room store has authored-room data.
- Pause/progress saves use save type `manual` and a fresh UTC timestamp label.
- Existing world identity fields are carried forward by durable save code, not duplicated here.

## Tests / Proof Commands

- `rg -n "product_save_session_missing|activeProductSaveId|writeProductCurrentSessionSave" tests/unit src/app/iggy3d/save`.
- `rg -n "product_save_bridge_tests|product_creative_world_launch_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/save/Flow.*` unless pause-flow behavior changes.
- `src/app/iggy3d/save/SaveBridge.*` unless durable write request/result contracts change.
- `src/app/iggy3d/gameplay/ProductRoomStore.*` unless active authored-room access changes.

## Update When

- Current-session save request fields, save proof fields, active save id carry-forward, or authored-room attachment rules change.

## Do Not Update When

- Only pause menu labels, save browser selection, or runtime save codec internals change without changing current-session save request/proof behavior.

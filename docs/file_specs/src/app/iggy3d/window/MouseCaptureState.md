# File Spec

Files: `src/app/iggy3d/window/MouseCaptureState.hpp`

Verified at: `a7c5f1db`

## Owns

- Product window mouse-capture state packet embedded through input device state.
- Requested, active, status, reason code, mode, and input-owner fields used for receipts and no-window proof.

## Does Not Own

- Mouse capture policy decisions.
- SDL relative mouse mode calls.
- Creative/editor pointer policy, gameplay mouselook policy, or frontend input routing.
- Receipt field formatting.

## Reads

- This header defines state only.
- Readers include input frame code, loop fallback code, receipt appenders, and tests.

## Writes / Mutates

- No functions in this file mutate state.
- `InputFrame.cpp` writes policy and platform mouse capture results into this packet.
- `Loop.cpp` writes no-window mouse capture proof.

## Calls Out To / Wires Out To

- Included by `InputDeviceStore.hpp`.
- Receipt appenders emit this packet as `mouse_capture_*` fields.

## Called By / Entry Points

- `InputDeviceStore` embeds `ProductMouseCaptureState`.
- `InputFrame.cpp` updates the packet from `ProductMouseCapturePolicy` and `SdlMouseCaptureResult`.
- Focused proof: `rg -n "ProductMouseCaptureState|mouseCapture\\.|mouse_capture_" src tests`.

## Invariants

- Defaults represent no capture requested, no active capture, gameplay-inactive reason, no mode, and no input owner.
- Status and reason code strings are receipt-facing contract values.
- Keep this file as a narrow packet; policy belongs in `MouseCapturePolicy.*` and platform calls belong in `SdlWindow.*`.

## Tests / Proof Commands

- `rg -n "product_mouse_capture_policy_tests|product_gameplay_controls_smoke|product_window_renderer_lifecycle_tests" cmake/iggy3d_tests.cmake tests`.
- `rg -n "mouse_capture_requested|mouse_capture_reason_code|mouse_capture_input_owner" src/app/iggy3d/receipt tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/MouseCapturePolicy.*` unless decision outputs change.
- `src/app/iggy3d/window/InputFrame.*` unless packet write ownership changes.
- `src/app/platform/SdlWindow.*` unless platform relative mouse mode result semantics change.
- `src/app/iggy3d/receipt/FrontendSettingsWindowFields.*` unless receipt keys change.

## Update When

- Mouse-capture packet fields, default values, writer ownership, or receipt-facing semantics change.

## Do Not Update When

- Only policy conditions or SDL platform behavior changes without changing this state packet.

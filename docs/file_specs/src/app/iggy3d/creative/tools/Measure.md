# File Spec

Files: `src/app/iggy3d/creative/tools/Measure.hpp`, `src/app/iggy3d/creative/tools/Measure.cpp`

Verified at: `7b40370f`

## Owns

- Creative measurement tool state and receipts.
- Measurement begin, update, end, cancel, clear, and intent routing.
- Pointer-to-measurement-point copying, including target refs.
- Sample count tracking for active/completed measurements.

## Does Not Own

- Tool input dispatch that emits measurement intents.
- UI drawing of measurement labels.
- Document mutation.
- World/grid snapping.
- Runtime physics or distance unit conversion beyond copied pointer coordinates.

## Reads

- Current `CreativeMeasurementState`.
- `CreativeToolPointerPacket`.
- `CreativeToolIntent` kinds emitted by `Tools.*`.

## Writes / Mutates

- Mutates caller-owned measurement state.
- Writes `CreativeMeasurementReceipt` before/after mirrors and messages.
- Clears measurement state on cancel and clear.

## Calls Out To / Wires Out To

- Consumed by `creative::Facade` during tool-intent dispatch.
- Creative UI model reads measurement state for visible measurement facts.
- Tool dispatch emits the measurement intent kinds handled here.

## Called By / Entry Points

- `makeDefaultCreativeMeasurementState()`.
- `clearMeasurement(...)`, `beginMeasurement(...)`, `updateMeasurement(...)`, `endMeasurement(...)`, `cancelMeasurement(...)`.
- `applyMeasurementToolIntent(...)`.
- Grep proof: `rg -n "CreativeMeasurement|applyMeasurementToolIntent|beginMeasurement|updateMeasurement|endMeasurement|cancelMeasurement" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Begin always starts or restarts a measurement and resets sample count to one.
- Update and end fail closed when no measurement is active.
- Unchanged active updates do not increment sample count.
- End keeps completed measurement data but clears active state.
- Cancel clears active and completed measurement data.
- Non-measurement intents do not mutate state.

## Tests / Proof Commands

- `creative_measure_tests`.
- `creative_facade_tests`.
- `product_creative_ui_command_frame_tests`.
- `rg -n "creative_measure_tests|applyMeasurementToolIntent|CreativeMeasurement" cmake/iggy3d_tests.cmake tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/tools/Tools.*` unless measurement intent emission changes.
- `src/app/iggy3d/creative/Facade.*` unless measurement routing or state reset changes.
- `src/app/iggy3d/creative/ui/UiFrame.*` unless measurement UI consumption changes.
- `src/app/iggy3d/creative/spatial/Ghost.*` unless preview and measurement state are deliberately coupled.

## Update When

- Measurement state fields, receipt semantics, sample count policy, accepted/no-op rules, or intent routing changes.

## Do Not Update When

- Only UI formatting, pointer coordinate source, or unrelated tool behavior changes without changing measurement state rules.

# 04. Frame Events To Reports

This layer answers one question: after simulation runs, how do frame-visible
facts become readable runtime reports?

The important design choice is that simulation emits events, while runtime
recorders decide how those events are stored for debugging and summaries.

```text
SimulationFrameEvents
  -> RuntimeFrameEventReportRecorder
  -> RuntimeFrameReport::frameEvents
  -> RuntimeRunSummary::lastFrameEvents

SessionEventRecorder / InventoryEventRecorder
  -> RuntimeFrameEventDeltaCollector
  -> RuntimeEventStreamDelta
  -> RuntimeFrameCompletionReportRecorder
  -> RuntimeFrameReport::sessionEvents / inventoryEvents
```

## What Each Boundary Owns

`SimulationFrameEvents` captures movement, combat, and effect requests emitted
during simulation. It can also forward movement and combat events to existing
long-lived sinks.

`RuntimeFrameEventReportRecorder` stores the current simulation event batch on
the frame report and mirrors it to the run summary as the latest-frame snapshot.

`RuntimeFrameEventDeltaCollector` remembers where long-lived session and
inventory event streams were when the frame began.

`RuntimeEventStreamDelta` owns the offset rule: given an event stream and a
starting position, copy only the events emitted after that position.

`RuntimeFrameCompletionReportRecorder` attaches those event deltas to the
finished frame report and increments the finished-frame count.

## Test Boundary

`frame_simulation_tests` covers the runtime reporting side of a simulation
frame. It verifies that the frame policy is resolved and recorded, the session
advances through the frame settings, simulation events are mirrored into frame
and run reports, and long-lived lifecycle/inventory logs are sliced into
per-frame deltas.

## Why Reports Use Deltas

Movement and combat events are naturally frame-scoped because simulation
captures a fresh batch each update. Session and inventory events are long-lived
logs, so frame reports need a cursor. Without that cursor, every frame would
repeat old lifecycle and inventory events.

```text
old events before frame -> ignored
events during frame     -> attached to this frame report
events after frame      -> captured by a later frame
```

## Lesson

Debug reports are easier to trust when the engine separates event production
from report slicing. Simulation emits facts. Runtime records the current frame's
facts and slices long-lived logs into per-frame deltas.

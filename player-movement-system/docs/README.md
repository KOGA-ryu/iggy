# Documentation Map

Read the focused lessons first. Use the longer reference docs when you need the
full system sweep.

## Movement Lessons

- [01. Input To Intent](movement/01-input-to-intent.md): how raw mouse, touch,
  and key input becomes movement intent without letting device details leak into
  simulation code.
- [02. Intent To Command](movement/02-intent-to-command.md): how gameplay intent
  becomes queued movement commands and route results.
- [03. Command To Simulation](movement/03-command-to-simulation.md): how queued
  commands cross into the simulation frame.
- [04. Frame Events To Reports](movement/04-frame-events-to-reports.md): how
  simulation and lifecycle events become per-frame debug facts.

## Reference Docs

- [Movement Flow](movement-flow.md): full running notes for the movement system.
- [Lifecycle Replay](lifecycle-replay.md): session command replay and lifecycle
  command flow.
- [Save System](save-system.md): snapshot and save-slot flow.

## Split Rule

Create a focused doc when a topic becomes a reusable lesson instead of a small
note. Keep focused docs short enough to read in one sitting, and leave the long
reference docs for historical sweep notes.

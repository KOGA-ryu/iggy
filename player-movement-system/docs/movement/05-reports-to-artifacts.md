# 05. Reports To Artifacts

This layer answers one question: how does a runtime report become a file a
developer can inspect?

The important design choice is that reporting data, trace text, and filesystem
writes are separate jobs.

```text
GameLoopResult
  -> RuntimeRunSummaryText
  -> RuntimeRunTraceFrameHeaderText
  -> RuntimeFrameTrace
  -> RuntimeTraceService
  -> RuntimeFrameTraceFileStore
  -> run.trace
```

## What Each Boundary Owns

`RuntimeRunSummaryText` formats the run-level counts at the top of the trace.

`RuntimeRunTraceFrameHeaderText` formats the per-frame marker, such as
`frame[0]`, before that frame's details.

`RuntimeFrameTrace` formats one completed frame report into deterministic text.

`RuntimeTraceService` assembles a full run trace: run summary, frame markers,
and formatted frame details.

`RuntimeFrameTraceFileStore` persists and reloads readable trace lines.

## Why The Text Is Deterministic

Debug files should be boring. A trace should be stable enough to compare in a
test, paste into notes, or diff after changing engine behavior.

```text
same report data
  -> same line order
  -> same file contents
```

The frame marker is small, but naming it matters because it is the boundary
between run-level context and frame-level details. The trace service should
assemble sections; tiny formatters should own exact text.

## Lesson

Good debug artifacts are built like the rest of the engine: data first,
deterministic translation second, filesystem output last. That keeps debugging
useful without making the simulation or runtime loop care about files.

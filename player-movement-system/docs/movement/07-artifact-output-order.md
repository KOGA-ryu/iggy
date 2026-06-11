# 07. Artifact Output Order

This layer answers one question: when a run requests multiple debug outputs,
which artifacts are written and in what order?

The important design choice is that output settings become an explicit ordered
plan before any files are written.

```text
RuntimeOutputSettings
  -> RuntimeArtifactOutputPlan
  -> RuntimeArtifactOutputRequest[]
  -> RuntimeArtifactOutputService
  -> RuntimeArtifactOutputRequestRunner
  -> RuntimeRunTraceOutputStep
  -> RuntimeDebugBundleOutputStep
```

## What Each Boundary Owns

`RuntimeOutputSettings` holds optional output paths from the app shell.

`RuntimeArtifactOutputPlan` turns those optional paths into ordered output
requests. Standalone run traces come before debug bundles.

`RuntimeArtifactOutputService` executes the plan against a `GameLoopResult`.

`RuntimeArtifactOutputRequestRunner` executes one planned artifact request by
choosing the trace or debug bundle output step.

`RuntimeRunTraceOutputStep` writes the standalone run trace and records trace
attempt/save flags.

`RuntimeDebugBundleOutputStep` writes the bundle and records bundle
attempt/save flags.

## Why Order Is Explicit

Output order affects the result snapshot passed to each writer. The standalone
trace should see its own in-progress trace attempt. The debug bundle should see
the trace result that came before it.

```text
trace path configured  -> write standalone trace first
bundle path configured -> write debug bundle second
neither configured     -> leave output flags untouched
```

Making that order a small plan keeps the service readable and gives tests a
cheap place to verify the policy without touching the filesystem.

## Lesson

Optional settings are not the same as work. Convert settings into an explicit
plan, then execute each request. That keeps artifact policy separate from
artifact writing.

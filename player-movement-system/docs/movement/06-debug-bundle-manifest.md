# 06. Debug Bundle Manifest

This layer answers one question: when a debug bundle writes multiple files, how
does the manifest know what happened?

The important design choice is that trace writing and manifest writing are
separate artifact steps. The manifest step receives the trace save result and
records it as bundle metadata.

```text
RuntimeDebugArtifactBundle
  -> RuntimeDebugArtifactLayout
  -> RuntimeDebugArtifactRootPreparer
  -> RuntimeDebugArtifactWriter
  -> RuntimeDebugTraceWriteStep
  -> RuntimeDebugManifestWriteStep
  -> RuntimeDebugManifestContextBuilder
  -> RuntimeDebugManifest
  -> manifest.txt
```

## What Each Boundary Owns

`RuntimeDebugArtifactLayout` names stable bundle paths.

`RuntimeDebugArtifactRootPreparer` makes sure the bundle root is a directory
before any files are written.

`RuntimeDebugArtifactWriter` coordinates the artifact writes and returns write
flags for the bundle result.

`RuntimeDebugTraceWriteStep` writes `run.trace` through the trace service.

`RuntimeDebugManifestWriteStep` writes `manifest.txt`. It builds the manifest
context from artifact paths plus the trace save result, formats the manifest,
and persists the readable lines.

`RuntimeDebugArtifactBundleResultBuilder` records the final bundle state:
paths, root preparation, trace saved, and manifest saved.

## Why The Manifest Receives Trace State

The manifest is the index for the bundle. It should still be useful when the
trace failed to write.

```text
trace saved   -> manifest says saved=true
trace failed  -> manifest says saved=false
```

That lets debug tooling inspect one file first and understand whether the rest
of the bundle is complete.

## Lesson

A debug bundle is not one big write. It is a small workflow: name paths, prepare
the root, write trace, write manifest, then report which steps succeeded.

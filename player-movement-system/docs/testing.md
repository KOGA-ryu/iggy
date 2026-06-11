# Testing Shape

The project started with one large `movement_tests.cpp` file because that made
early exploration simple. As the engine layers become clearer, focused test
executables should take over one cluster at a time.

## Current Pattern

`movement_tests` remains the broad regression suite.

`artifact_output_tests` owns the first extracted cluster: artifact output
settings, output flag state, output steps, output planning, and one-request
artifact execution.

`artifact_text_tests` owns pure trace and manifest formatter contracts. These
tests exercise strings produced from runtime reports without touching artifact
writers, filesystem stores, or frame execution.

```text
focused production boundary
  -> focused test executable
  -> same player_movement_system library
```

## Split Rule

Split a test cluster when it has a clear subsystem boundary and does not need a
large shared helper migration.

Good first candidates:

- input routing steps
- simulation command draining

Avoid splitting by line count alone. A smaller file is useful only when the new
test target has a clear reason to exist.

## Lesson

One large test file is acceptable while the system is forming. Once boundaries
stabilize, move tests by subsystem. That keeps build failures easier to scan
without turning test organization into a risky refactor.

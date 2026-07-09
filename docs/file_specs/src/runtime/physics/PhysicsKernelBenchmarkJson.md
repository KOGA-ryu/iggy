# File Spec

Files:

- `src/runtime/physics/PhysicsKernelBenchmarkJson.hpp`
- `src/runtime/physics/PhysicsKernelBenchmarkJson.cpp`

Verified at: `3b2f3fcb`

## Owns

- JSON string serialization for physics benchmark case and suite result packets.
- JSON schema string literals for case and suite outputs.
- Deterministic field order, indentation, string escaping, boolean/unsigned output, and compact float text.
- Convenience overload that runs a benchmark suite from config and serializes it.

## Does Not Own

- Benchmark case/suite execution.
- Physics kernel correctness or benchmark counter semantics.
- JSON parsing, file output, CLI handling, or external report storage.
- App/render/debug presentation.

## Reads

- `PhysicsKernelBenchmarkCaseResult`, `PhysicsKernelBenchmarkSuiteResult`, and `PhysicsKernelBenchmarkConfig`.
- Status-name helper from `PhysicsKernelBenchmark.*`.
- Case counters, extrema, timing, names, ok/status/reason fields, and upstream reason code.

## Writes / Mutates

- Local `std::string` output only.
- No mutation of benchmark result packets.
- No filesystem, process, app, or runtime state.

## Calls Out To / Wires Out To

- `physicsKernelBenchmarkStatusName(...)` for status fields.
- `runPhysicsKernelBenchmarkSuite(config)` for the config convenience overload.

## Called By / Entry Points

- Direct API: `physicsKernelBenchmarkCaseToJson(...)` and `physicsKernelBenchmarkSuiteToJson(...)`.
- Grep proof: `rg -n "physicsKernelBenchmarkCaseToJson|physicsKernelBenchmarkSuiteToJson" src tests`.

## Invariants

- Output is deterministic for the same input packet.
- Serializer does not mutate input result packets.
- Strings escape quotes, backslashes, common control characters, and low control bytes as JSON-safe text.
- Floats use fixed precision then trim trailing zeroes while preserving at least one decimal digit.
- Negative zero serializes as `0.0`.
- Suite serialization includes all cases in vector order.

## Tests / Proof Commands

- `rg -n "physics_kernel_benchmark_json_tests" cmake tests/unit`.
- `rg -n "exactCaseJsonMatchesStableShape|exactSuiteJsonMatchesStableShapeAndEscapesStrings|controlCharactersAreEscaped|deterministicAndDoesNotMutateSource" tests/unit/physics_kernel_benchmark_json_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsKernelBenchmark.*` unless packet fields or suite runner overloads change.
- Tools or report scripts unless they parse this JSON shape.
- Physics kernel files; JSON output should follow benchmark packets, not kernel internals.

## Update When

- JSON schema strings, field order, escaping, float formatting, emitted fields, or suite convenience behavior change.

## Do Not Update When

- Benchmark scenarios change but case/suite packet fields and JSON shape stay stable.
- Tests add parser checks for the existing serialized shape.

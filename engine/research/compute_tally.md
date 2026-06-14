# Compute Tally

Purpose: track how Iggy code shapes CPU, memory, cache, allocation, binary, and compile-time cost. This is not a style guide. It is a measurement ledger so syntax and architecture choices can be compared against observed cost.

## Core Rule

Syntax rarely costs by itself. It costs when it changes the work the machine must do:

- copies and moves
- heap allocations
- string comparisons
- branch count and branch predictability
- cache locality
- indirection and pointer chasing
- virtual dispatch or callback dispatch
- sorting and searching complexity
- template instantiation and compile time
- binary size and instruction cache pressure

The useful question is not "is this syntax fancy?" The useful question is "what extra work does this shape create, and does that work buy auditability, safety, or speed somewhere else?"

## Measurement Buckets

| Bucket | What to measure | Why it matters |
|---|---|---|
| Build time | configure time, full build time, focused target build time | Template-heavy and header-heavy code taxes iteration speed. |
| Test time | full `ctest`, focused target test time | Slow tests reduce slice velocity. |
| Runtime CPU | per-call or per-frame time for hot transforms | Decides actor count, map size, and frame budget. |
| Allocations | number and bytes allocated in hot calls | Heap churn kills throughput and makes spikes harder to reason about. |
| Copy volume | copied vectors, strings, nested reports, large structs | Value semantics are audit-friendly, but can become expensive per actor/frame. |
| Memory footprint | struct size, vector payload size, cache data size | Large actor state and reports limit scale. |
| Data locality | contiguous arrays vs pointer graphs | Cache misses often dominate simple C++ logic. |
| Branching | switches, policy checks, validation scans | Predictable branches are cheap; wide unpredictable dispatch gets costly. |
| Search/sort complexity | linear lookup, map lookup, sort size | Many current registries use deterministic vector scans by design. |
| Binary size | optimized binary and object file growth | Large generated or templated code can hurt load and instruction cache. |

## Syntax Shape Tally

| Code shape | Typical compute effect | Iggy interpretation |
|---|---|---|
| Plain structs with public fields | Low dispatch cost, easy copies, predictable layout | Good default for data packets and reports. Watch large nested report copies. |
| `std::vector` ordered storage | Contiguous, cache-friendly iteration; push/copy can allocate | Good for deterministic order. Hot paths should reserve or reuse where practical later. |
| Exact `ResourceId` values | Stable and auditable ids; string copy/compare cost | Fine for authoring/report layers. Hot runtime loops may eventually need indexed handles. |
| Pass by `const &` | Avoids input copies | Good default for transforms. |
| Return by value | Clear ownership; may copy nested vectors if not elided/moved | Good for current audit-first slices. Measure before using per-frame for many NPCs. |
| Nested copied reports | Strong debugging and acceptance coverage | Excellent for proving ownership. Candidate for lightweight runtime report mode later. |
| Builders that scan vectors | Deterministic validation; usually `O(n)` or `O(n^2)` for duplicates | Fine for authoring/build time. Avoid in every-frame loops. |
| Linear `find` over vectors | Simple, deterministic, cache-friendly for small sets | Fine for small registries. Consider sorted/indexed views for large pools/maps. |
| Stable sorting ranked candidates | Deterministic decisions; `O(n log n)` | Fine while candidate hands are small. Track once trait pools get massive. |
| Enums and switches | Cheap and explicit; can grow large | Good for objective/state/mode layers. Keep dispatch ownership narrow. |
| Virtual dispatch | Flexible, indirect, less optimizer visibility | Avoid unless a plugin/runtime boundary actually needs it. |
| Templates/generic helpers | Can remove duplication; may increase compile time | Use only when it reduces real repeated code or errors. |
| Callbacks/function objects | Useful boundary; possible indirection and capture cost | Keep for host/UI/runtime boundaries, not inner AI scoring loops. |

## Current Iggy Hotspot Candidates

These are not bugs. They are places to measure before scaling.

### NPC Trait Card Chain

Path:

```text
Trait Pools -> Draw -> Hand -> Read / MapRead -> MapPlayReport -> Play -> Tell -> Fold
```

Likely costs:

- six trait draw passes over pool entries
- `NpcHand` copies normalized Ent data and map tags
- `NpcRead` copies Hand and ranked Ents, then sorts ranked candidates
- `NpcMapRead` repeats candidate ranking with local map tag/weight scoring
- `NpcMapPlayReport` preserves raw read, map-aware read, projected read, Play, Tell, and Fold facts by value
- `NpcPlay` copies Read
- `NpcTell` copies Play and emits explanation lines
- `NpcFold` copies Tell

Why it is currently acceptable:

- the chain is audit-first
- every layer is deterministic and testable
- it gives clear training/telemetry material later

What to watch:

- many NPCs per frame
- massive trait pools
- rich `ResourceId`/tag payloads on every Ent
- always building full Tell/Fold reports in release gameplay

Future option:

- keep the full audit chain for tools/tests/debug
- add a lightweight runtime read/play path only after semantics stabilize and benchmarks show a real cost

### Map and Registry Lookup

Many registries intentionally use ordered vectors with exact id lookup.

Why it is currently acceptable:

- deterministic behavior
- simple save/build validation
- easy test coverage

What to watch:

- large maps, large AI pools, large actor registries
- repeated id lookup inside inner loops

Future option:

- keep vector source truth
- build explicit derived indexes/caches after load/build
- save source truth, rebuild indexes

### Value-Owned Runtime Packets

Runtime and scene steps often return copied state/report packets.

Why it is currently acceptable:

- prevents hidden mutation
- makes ownership boundaries auditable
- keeps tests straightforward

What to watch:

- large `RuntimeGameplayState` copies
- repeated per-frame report copies
- nested event recorder copies

Future option:

- add report levels: `None`, `Counts`, `FullTrace`
- keep full trace for tests/editor/debug
- use lighter packets for hot gameplay loops only after profiling

## First Measurement Commands

Use these as rough local smoke measurements. They are not final benchmarks.

```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
```

Focused target example:

```sh
cmake --build engine/build --target npc_fold_tests
./engine/build/npc_fold_tests
```

Count source/test footprint:

```sh
rg --files engine/src/scene/ai engine/src/scene/npc engine/src/runtime | rg 'Npc|Ai' | wc -l
rg --files engine/tests | rg 'npc|ai' | wc -l
```

Find obvious allocation/copy pressure candidates:

```sh
rg -n "std::vector|std::string|ResourceId|return .*;" engine/src/scene/ai engine/src/scene/npc engine/src/runtime
```

## Benchmark Roadmap

1. Add small focused microbench executable only after a hot path is stable.
2. Start with NPC card-chain benchmarks:
   - tiny pool / one NPC
   - large pool / one NPC
   - large pool / many NPCs
   - full Tell/Fold trace on vs off, if a light mode is later added
3. Add navigation/path benchmarks separately.
4. Add runtime queue/execution benchmarks separately.
5. Treat benchmark results as regression data, not as design law.

## Decision Rule

Do not optimize away auditability early. First prove the behavior. Then measure the exact cost. Then replace only the measured expensive part with a derived cache, index, or lightweight mode that preserves the same source-of-truth boundary.

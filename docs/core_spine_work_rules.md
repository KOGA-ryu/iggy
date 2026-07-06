# Core Spine Work Rules — the gate for load-bearing code

**Status:** standing doctrine. **Applies to:** any change touching `AppKernel`/app lifecycle, frame
scheduling, job execution, cross-thread behavior, Vulkan/GPU lifetime, resource lifetime,
`CreativeDocument` (authoring-truth) mutation, or save/load boundaries. Ordinary helpers and pure
algorithms do NOT go through this — they stay casual. Spine code needs paranoia.

> Helpers can be casual. Systems need discipline. Spine code needs paranoia — build it like a
> load-bearing beam.

## The bar

Core-spine code does not land because it sounds useful. It lands only when **all** of these hold:
the pain is real · the owner is clear · the boundary is defined · a real callsite exists · the
failure modes are known · tests prove behavior · an audit proves it did not leak chaos sideways.
No real pain + no real callsite + no real owner = architecture fan fiction. Don't merge it.

## Naming (enforced)

- **No new component named `Kernel` without explicit approval.** Reserve "kernel" for `AppKernel`
  (the runtime coordinator) and an *earned* compute kernel (hot/batched/SoA/profiled — still named
  for the concept, e.g. `RayAabbBatch`). See [[algorithms-not-kernels]].
- Prefer: `System` (domain ownership) · `Ops` (pure helper collection) · `Store` (owned data) ·
  `Registry` (handles + lookup) · `Queue` (ordered handoff) · `Scheduler` (timing/order policy) ·
  `Device`/`Backend` (platform/GPU boundary).

## Gate 0 — design preflight (before any code)

A surgical map, not a novel. Required fields:

1. **Problem** — the concrete thing that breaks today (not "engines have these").
2. **Existing path** — real file paths + symbols involved.
3. **Minimal types** — names, one-line responsibilities, ownership. Smallest possible set.
4. **Boundary** — an explicit *Owns* / *Does NOT own* table (a trash-magnet firewall).
5. **Invariants** — rules that must ALWAYS be true.
6. **Thread & lifetime rules** — who may touch what, and when; how data crosses (copy/move/snapshot).
7. **Memory rules** — long-lived vs frame vs scratch; what may allocate in hot loops.
8. **Failure behavior** — what happens on every error path.
9. **Shutdown behavior** — drain, cancel, or block; ordering vs teardown of what it mutates.
10. **Tests** — unit / integration / regression / stress / determinism / thread-safety.
11. **Non-goals** — what must NOT be built yet.
12. **Rollback / deletion path** — removal criteria if it fails to justify itself.

This kills ~80% of nonsense before it reaches a compiler.

## The sacred laws (never violate silently)

- **Main-thread law:** authoring truth (`CreativeDocument`, editor/session/window state) mutates on
  the **main thread** unless a component is explicitly, provably designed for shared access.
- **Snapshot discipline:** async work receives an **immutable snapshot** as input, produces an owned
  output, and the main thread validates the output against current state before applying. Workers do
  not read live mutating state (no raw pointers into `CreativeDocument`).
- **GPU stays on the render/main path** unless the repo *proves* otherwise (today it does not).
- **Every unit of async work must complete, fail, or be cancelled — and every failure is observable.**
- **Nothing outlives its owner:** no job/handle/reference may read state after its owner is destroyed.

## Required deliverables (per spine change)

- At least one **real production callsite** (no framework with only test consumers).
- Tests: **happy path + failure path + shutdown/cancellation** (where applicable).
- A **bypass-removal audit**: the old hardcoded/synchronous path is removed, or intentionally retained
  behind a documented fallback.
- **Instrumentation** sufficient to prove the fix worked (counters — no counters means debugging by
  incense). Fits the repo's existing receipt-field culture.
- An **architecture receipt** near the code: why it exists · what it owns · what it does not own ·
  thread rules · shutdown rules · first consumer · known limitations.

## Pre-merge audits

- **Callsite:** who calls it; who *should* but still bypasses it; any old sync path left; duplicated
  competing helpers.
- **Ownership:** who creates/destroys/borrows it; can anything outlive it; are handles generation-checked.
- **Thread:** what runs main vs worker; what crosses the boundary and *how* (copy/move/snapshot/shared).
- **Memory:** does hot code allocate; are allocations bounded; hidden copies; can cancellation leak.
- **GPU (if Vulkan):** who owns handles / records / submits / waits / recreates swapchain; can CPU
  mutation race GPU reads; are fences/semaphores respected.
- **Naming:** is this actually a kernel / system / data structure / Ops — did the name inflate scope?
- **Delete-path:** if it fails to justify itself, does it remove cleanly, or did it infect unrelated systems?

## Review gates (do NOT land as one 2,400-line "small refactor")

```txt
Gate 1  design accepted
Gate 2  minimal types added, no consumer
Gate 3  unit tests pass
Gate 4  one real consumer wired
Gate 5  old blocking path removed, or explicitly retained behind fallback
Gate 6  stress + shutdown/cancellation behavior verified
Gate 7  naming + ownership audit passed
```

## Promotion / deletion ladders (stop premature abstraction AND sediment)

Promote only as earned: one-off → local · repeated twice → `Ops` helper · needed by a domain →
`System`/`Ops` · hot + measured → optimize layout/batching · central to lifecycle → maybe core system
· coordinates runtime → maybe `AppKernel`-owned. Every new core component carries **removal criteria**
(e.g. "if it has only fake callsites, delete it"; "if the SIMD path shows no measured win, keep scalar
and remove SIMD"). A repo without deletion rules turns to sedimentary rock.

## Worked example

The first spine change gated by this doctrine is the async creative bake:
[jobsystem_bake_preflight_v0_2.md](jobsystem_bake_preflight_v0_2.md) (`JobSystem` + `MainThread
CompletionQueue`, not a kernel). Use it as the template shape for the next one.

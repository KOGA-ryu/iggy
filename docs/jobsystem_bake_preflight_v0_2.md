# JobSystem Design Preflight v0.2 — async the creative bake

**Author:** Claude (planner). **Date:** 2026-07-06. **Status:** design only — do NOT implement yet.
**Supersedes** v0.1. Gated by [core_spine_work_rules.md](core_spine_work_rules.md). Not a kernel.
All facts carry file:line from a 5-agent read-only spine recon.

---

## 1. Problem

`buildRoomAssetFromCreativeDocument` runs at **`Operations.cpp:694`** on the **main thread**, inside
`processProductWindowInputFrame` (`Loop.cpp:242-256`) which runs **before** `presentProductWindowFrame`
(`:285`). Input *and* render stall until the bake returns. Two interactive triggers hit it: explicit
"RebuildRoom" (`InputFrame.cpp:1088-1098`) and auto-on-edit (`InputFrame.cpp:1320-1329`).

**Magnitude caveat (measure before building):** the bake receipt already records
`bakeElapsedMicroseconds` (`Operations.cpp:686-697`). The freeze is structural and on the critical path
— it grows with room complexity — but if today's rooms bake in well under a frame, this is correct
architecture arriving early. Confirm it's user-perceptible on a representative room first.

## 2. Existing path (symbols)

| Stage | Symbol | File:line | Nature |
|---|---|---|---|
| trigger A/B | `processProductCreativeUiCommandPhase` / `processProductCreativeDocumentRevisionPhase` | `InputFrame.cpp:1088-1098` / `1320-1329` | interactive |
| entry | `refreshProductCreativeBakedActiveRoom` | `Operations.cpp:1590-1608` | orchestration |
| **compute** | `buildRoomAssetFromCreativeDocument` | `RoomBake.cpp:664-770` | **pure CPU, heavy, no GPU, no live-state write** |
| **install** | `installBakedRoom` (+ `activateCreativeReasoningGraph` → `Session::setReasoningGraph`) | `Operations.cpp:740-766` | **main-thread state mutation** |

Key fact: the compute/mutate boundary a job system needs **already exists** — compute returns a
self-contained `CreativeRoomBakeResult`; install is a separate main-thread step. GPU is untouched by
the bake (upload happens later on the render thread, `RenderLoop.cpp:460-473`). Repo is 100%
single-threaded today (zero `std::thread`/`mutex`/`atomic`).

## 3. Minimal types

`JobSystem` (1 worker in prod; `workerCount=0` = inline test seam) · `JobHandle` (`{id}` + atomic
state) · `JobResult` (`{status ∈ Completed|Cancelled|Failed, reasonCode}`) · `MainThreadCompletionQueue`
(mutex-guarded, drained on main thread) · optional `BakeJob` (first consumer's glue: owns a
`CreativeDocument` snapshot; Work = `buildRoomAssetFromCreativeDocument`; Completion = `installBakedRoom`).

## 4. Boundary — Owns / Does NOT own

```txt
JobSystem OWNS:                     JobSystem DOES NOT OWN:
  worker thread(s)                    editor / CreativeDocument state mutation
  the job queue                       Vulkan resource creation / upload
  job handles + their state           creative bake SEMANTICS (that stays in RoomBake)
  cancellation requests               save/load policy
  completion posting + drain          gameplay/session tick logic
  shutdown/drain behavior             render dirty-flag POLICY (it only sets a flag on apply)
  failure reporting (job ran/failed)
```

## 5. Invariants (must always hold)

1. Editor/authoring state is mutated **only on the main thread**.
2. A worker job **cannot touch** `CreativeDocument`, session/window state, or Vulkan objects.
3. Every submitted job **completes, fails, or is cancelled** — and **every failure is observable**.
4. **No job outlives the `JobSystem`**; no completion reads state after its owner is destroyed.
5. Shutdown must not destroy data while a job can still read it.
6. A stale bake (older `DocumentRevision`) is **rejected at apply**, never overwrites newer edits.

## 6. Thread, lifetime & memory rules

- **Snapshot discipline (the #1 risk):** `CreativeRoomBakeRequest.document` is a raw
  `const CreativeDocument*` (`RoomBake.hpp:22`). The job **must** capture a value **snapshot** at
  submit (main thread) and point the request at the owned copy. `CreativeDocument` is a value
  container → plain copy. Workers never read live document memory.
- **Handoff:** worker computes an owned `CreativeRoomBakeResult` → posts `{handle, result, completion}`
  to `MainThreadCompletionQueue` under lock. Main thread drains + runs completions at **`Loop.cpp:257`**
  (after input `:256`, before render prep `:259`), mirroring the existing `pendingExecutionSequences`
  drain idiom (`SessionState.hpp:68`).
- **Ownership:** one `JobSystem` owned by the window-loop/app scope beside `window_`/`session_`;
  reachable by reference from the input-frame trigger and the loop drain. No globals.
- **Memory:** the per-bake document snapshot is the one real cost (bounded, small). The drain applies
  by move; no allocation in the drain hot path beyond popping the queue.

## 7. Failure behavior

- Job `Work` throws → caught at the worker boundary → `JobResult{Failed, reasonCode}`; **old room
  stays**, editor untouched; failure surfaced via the receipt/instrumentation.
- Domain rejection (no_renderable_objects, invalid bounds) already rides in
  `CreativeRoomBakeResult.receipt` — `JobResult` stays `Completed`, the existing reject path applies.
- App closes / scene changes / project loads mid-bake → shutdown cancels or drops the pending job
  (§8); a superseded result is dropped at apply (§5.6).
- GPU device lost / swapchain resize → **not this component's concern** (bake touches no GPU).

## 8. Shutdown & cancellation

- **Cancellation = supersede.** Pending (not started) job on cancel → dropped, `JobResult{Cancelled}`.
  Running job → its result is **discarded at handoff** if superseded (v1 does not interrupt mid-compute;
  the bake is a single call). Revision-gating (§5.6) is the supersede check.
- **Shutdown:** set stop flag → notify worker → **join**. Pending jobs dropped (Cancelled). Undrained
  completions dropped. **`JobSystem::shutdown()` runs BEFORE `window_`/`session_` teardown** so no
  completion writes freed state.

## 9. Instrumentation (counters — no counters, no proof)

`jobsSubmitted`, `jobsCompleted`, `jobsFailed`, `jobsCancelled`, `avgJobMicros`, `longestJobMicros`,
`mainThreadCompletionsApplied`, `queueDepth`, `shutdownDrainMicros`. Emit through the existing
receipt-field mechanism so the fix is *provable*, not asserted.

## 10. Tests (by layer)

- **Unit:** submit→complete; result returned; cancelled job does not apply; failed job reports error;
  completion runs **only** on main-thread drain; queue drains all completed jobs; shutdown per policy.
- **Integration (real callsite):** start bake → event pump still processes; completion applies on main
  thread; render dirty flag set after apply; **no editor mutation on the worker thread**.
- **Regression:** the synchronous-bake freeze does not return; starting a bake no longer blocks the
  event pump.
- **Stress:** many small jobs; one long job; cancel during shutdown; destroy scene while bake pending;
  start bake then close window; start bake then load another project; start bake then save.
- **Determinism:** async result **bit-identical** to synchronous `buildRoomAssetFromCreativeDocument`
  (the oracle; guaranteed by `-ffp-contract=off`); scheduling order does not change the final result.
- **Thread-safety:** assert main-thread-only mutation; assert the worker cannot reach editor-mutation
  APIs (snapshot isolation test — mutate source doc after submit, result reflects the snapshot); run
  under ThreadSanitizer where supported.

## 11. Non-goals

No generic task graph · no work stealing · no fibers · no async/GPU Vulkan · no editor-state mutation
from workers · no asset-streaming rewrite · no ECS migration · no global service locator · no
thread pool (one worker) · no time/Clock system · no mid-compute cancellation token · no perf heroics
before the freeze is fixed · **not named a kernel.**

## 12. Review gates

```txt
G1 this preflight accepted        G5 old synchronous bake path removed / fallback documented
G2 JobSystem types, no consumer   G6 stress + shutdown/cancellation verified
G3 unit tests pass                G7 naming + ownership audit passed
G4 trigger A wired (one consumer)
```

Slice 1 = machine + trigger A only (one discrete click, no coalescing). Trigger B (auto-rebake) +
coalescing is **slice 2**. Do not land it all at once.

## 13. Deletion / rollback criteria

If, after slice 1, the only callers are tests (no real freeze relief measured) → **delete it**; the
sync path is retained behind a fallback until G5, so rollback is reverting one wiring point. The
`JobSystem` is isolated behind `submit`/`drain` — it must not infect `RoomBake`, `Session`, or the
render path (delete-path audit).

## 14. Architecture receipt (to sit near the code)

> `JobSystem` exists to keep the editor responsive while the creative bake computes. Owns worker/
> queue/handoff/shutdown; owns no editor/GPU/bake-semantic state. Workers read an immutable
> `CreativeDocument` snapshot and mutate nothing; the main thread applies results at the frame drain,
> rejecting stale revisions. First consumer: the "RebuildRoom" bake trigger. Limitation: v1 supersedes
> rather than interrupts; one worker; bake only.

## 15. Doctrine conformance

Passes core_spine_work_rules Gate 0 fields 1–12; adds the boundary table (§4), invariants (§5),
instrumentation (§9), the layered test matrix (§10), review gates (§12), and deletion criteria (§13)
that v0.1 lacked.

## Verdict

**Justified — yes, pending the §1 magnitude measurement.** One real freeze (`Operations.cpp:694`),
one near-ideal real callsite (pure-CPU, GPU-free, install already factored out), one bounded first
integration (one worker, trigger A, snapshot, drain at `Loop.cpp:257`, install unchanged). Adds one
small utility + two queues + one drain call; renames nothing; mirrors an existing drain idiom. Build
it when the measured bake time makes the freeze felt.

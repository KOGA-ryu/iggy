# 14 Runtime Report Duplication Audit

Status: complete.

Goal: docs-only audit/design packet for repeated count/event fields across raw,
policy, orchestrated, and runner reports.

Scope:
- No production changes.
- Identify a future smallest safe helper if any.

Anchor areas:
- Runtime frame reports.
- Policy reports.
- Orchestrated frame reports.
- Runner reports.

Verification:
- Docs diff review.
- `git diff --check`

Audit:
- Raw gameplay frame reports:
  `engine/src/runtime/RuntimeGameplayFrameReport.hpp` flattens player command
  counts, pickup/inventory counts, NPC movement counts, movement refresh flags,
  and derived events from nested frame input, inventory events, and
  `NpcActorMovementFrameReport2D`.
- Policy gameplay frame reports:
  `engine/src/runtime/RuntimePolicyGameplayFrameReport.hpp` carries the same
  flattened player, pickup/inventory, NPC movement, refresh, and event surface,
  but sources player facts through
  `RuntimePlayerInputInteractionEffectApplyFrameReport` and policy pickup
  facts through `RuntimePolicyPickupEffectFrameResult`.
- Orchestrated frame reports:
  `engine/src/runtime/RuntimeGameplayOrchestratedFrameReport.hpp` keeps a
  similar player/inventory/NPC movement projection but adds NPC control counts
  and explicit refresh booleans from the orchestrated result.
- Orchestrated runner and scenario reports:
  `engine/src/runtime/RuntimeGameplayOrchestratedFrameRunnerReport.hpp` and
  `engine/src/runtime/RuntimeGameplayScenarioRunner.hpp` aggregate repeated
  frame-level counts, especially changed frames, inventory events, NPC control,
  NPC movement, and refresh fields.

Current duplication signals:
- `RuntimeGameplayFrameReporter.cpp` and
  `RuntimePolicyGameplayFrameReporter.cpp` both define local
  `CountInventoryEvents`, `ProjectNpcMovement`, and
  `NpcMovementNeedsRefresh` helpers with the same field projection shape.
- Raw and policy frame reports intentionally use different nested source
  objects, so a shared helper would need a small destination/source adapter
  instead of direct struct reuse.
- The orchestrated runner already uses
  `RuntimeNpcOrchestrationAggregates.hpp` for NPC control, movement, and
  refresh folding, so another broad aggregate abstraction would likely overlap
  existing production helpers.
- Event enums are parallel but not identical across raw, policy, orchestrated
  frame, and orchestrated runner reports. Shared event emission would risk
  changing event order or event presence unless tests lock every event vector.

Smallest future helper candidate:
- If cleanup is still desired, start with an internal inventory event count
  projection helper for raw and policy frame reporters only. It can return a
  small count struct for `InventoryEventRecorder2D` without knowing about the
  report destination type.
- A second safe candidate is an internal NPC movement report projection helper
  that accepts `NpcActorMovementFrameReport2D` and returns flattened movement
  counts plus refresh flags. This should not emit events.
- Keep event vector construction local until event-order tests exist for raw
  and policy reports.

Recommended tests before implementation:
- Extend `runtime_gameplay_frame_report_tests` and
  `runtime_policy_gameplay_frame_report_tests` with explicit inventory event
  type-count coverage if not already exhaustive.
- Add event-order checks for raw and policy reports before extracting any
  shared event appender.
- Re-run `runtime_gameplay_orchestrated_frame_report_tests` and
  `runtime_gameplay_orchestrated_frame_runner_report_tests` if any NPC
  aggregate helper changes.

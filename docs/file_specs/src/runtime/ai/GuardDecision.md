# File Spec

Files: `src/runtime/ai/GuardDecision.hpp`, `src/runtime/ai/GuardDecision.cpp`, `src/runtime/ai/GuardDecisionReceipt.hpp`

Verified at: `9cd3a6b5`

## Owns

- Guard search-node choice kernel: `chooseSearchNode(...)`.
- Guard memory sample input, strategic value table, scoring config, decision result, and signed receipt factors.
- Deterministic candidate scoring from suspicion, strategic value, travel cost, and reserved ally contribution.

## Does Not Own

- Reasoning graph construction or persistence.
- Collider bake, route execution, route-follow state, or applying search decisions to actors.
- Session tick orchestration or AI memory recording.

## Reads

- `ReasoningGraph`, caller-supplied `PhysicsAabbCollider` span, guard position, memory sample, tick, actor id, personality weights, config, and optional excluded node id.

## Writes / Mutates

- No external state.
- Returns `GuardDecision` with optional chosen node and `GuardDecisionReceipt`.

## Calls Out To / Wires Out To

- `reasoningSegmentBlocked(...)` and `planRoute(...)` for direct/reachable travel cost.
- `GuardDecisionReceipt` is stored by `AiActorState` and consumed by tests/readouts.

## Called By / Entry Points

- `Session.cpp` calls `chooseSearchNode(...)` for search overlay decisions.
- `guard_decision_tests`, `stealth_garden_tests`, and `guard_decision_readout` call it directly.
- Grep proof: `rg -n "chooseSearchNode|GuardDecisionReceipt|GuardDecisionFactor" src tests cmake`.

## Invariants

- Graph nodes are evaluated in node-id order; strict better-score update gives lowest-id tie break.
- Blocked direct segment falls back to route planning; blocked with no route excludes the candidate.
- No candidate leaves `nodeId` empty and `receipt.hasChoice=false`.
- Receipt factors remain signed, weighted contributions in the order suspicion, strategic, travel, ally.
- This kernel never bakes colliders.

## Tests / Proof Commands

- `rg -n "guard_decision_tests|guard_decision_readout|stealth_garden_tests" cmake tests`.
- `rg -n "reasoningSegmentBlocked|planRoute|chooseSearchNode" src/runtime/ai tests`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/ReasoningGraph.*` unless graph node semantics change.
- `src/runtime/ai/ReasoningRoute.*` unless travel fallback semantics change.
- `src/runtime/session/Session.cpp` unless applying search decisions changes.

## Update When

- Search scoring, strategic table, receipt shape, candidate exclusion, route fallback, or tie-break behavior changes.

## Do Not Update When

- Caller timing or actor memory policy changes without changing the decision kernel.

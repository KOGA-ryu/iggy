# File Spec

Files: `src/runtime/ai/NpcPersonalityWeights.hpp`

Verified at: `abe51abb`

## Owns

- Header-only `NpcPersonalityWeights` data packet.
- Multiplicative guard-decision weights for suspicion, strategic value, travel cost, and ally contribution.
- Neutral default values for all factors.

## Does Not Own

- Guard decision scoring or receipt construction.
- NPC behavior profile resolution.
- Session tick actor lookup or personality assignment policy.

## Reads

- No external state.
- Consumers read the packet through behavior profiles or direct test construction.

## Writes / Mutates

- No functions and no external mutation.
- Callers construct or copy the value object.

## Calls Out To / Wires Out To

- `NpcBehaviorProfile` embeds `NpcPersonalityWeights` on each profile row.
- `GuardDecision.*` consumes weights while scoring search nodes and receipt factors.
- `Session.cpp` passes the resolved profile weights into guard decision.

## Called By / Entry Points

- Public entry point is the struct definition itself.
- Grep proof: `rg -n "NpcPersonalityWeights|personalityWeights|suspicionWeight|strategicWeight|travelWeight|allyWeight" src/runtime tests/unit`.

## Invariants

- Defaults are neutral `1.0F` for every factor.
- This header must stay data-only; scoring belongs in `GuardDecision.*`.
- Weight names must remain aligned with `GuardDecisionFactor` receipt labels.
- Ally contribution is represented even when the current scorer contributes zero.

## Tests / Proof Commands

- `rg -n "guard_decision_tests|stealth_garden_tests|guard_decision_readout" cmake tests`.
- `rg -n "travelWeight|GuardDecisionFactor|chooseSearchNode" src/runtime/ai tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/GuardDecision.*` unless scoring uses a new or renamed factor.
- `src/runtime/ai/NpcBehaviorProfile.*` unless profile storage of weights changes.
- `src/runtime/session/Session.cpp` unless profile weights stop flowing through session decisions.

## Update When

- A weight field is added, removed, renamed, or given non-neutral default semantics.

## Do Not Update When

- A profile row changes values while the packet contract remains the same.

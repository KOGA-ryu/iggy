# NPC AI Tuning Names

Purpose: Ace-facing vocabulary for tuning panels, docs, and planning conversations around the NPC AI lane.

This is not a code rename plan. The C++ type names remain the source API names for now. UI panels and tuning docs can use the friendly titles below without changing engine APIs, file names, namespaces, or test names.

## Type Titles

| Engine API | Ace-facing title | Notes |
| --- | --- | --- |
| `AiMap2D` | Tactical Map | Map-owned AI substrate: zones, tactical hints, weights, tags, and links. |
| `AiMapNode2D` | AI Zone / Tactical Node | A local tactical area or anchor that can influence nearby NPC decisions. |
| `AiMapQuery2D` | Local Situation | Read-only query of tactical zones around an NPC position. |
| `NpcAiBehaviorStore2D` | Behavior Preset Library | Reusable validated behavior presets for authoring/tuning NPC temperament values. |
| `NpcAiBehaviorPreset2D` | Behavior Preset | A named set of temperament values and instinct tags. |
| `NpcActorState2D` | NPC Actor | Scene-owned NPC identity/profile/current-goal facts. |
| `NpcActorState2DRegistry` | NPC Roster | Validated lookup table of NPC actors available to AI/runtime intake. |
| `NpcAiProfile2D` | NPC Temperament | NPC-carried bias and traits, not a behavior tree. |
| `NpcAiCurrentState2D` | NPC AI State | Small current facts for one NPC, such as id, position, and enabled state. |
| `NpcAiContextScore2D` | Instinct Scores | Weighted interpretation of local situation through temperament. |
| `NpcAiBehaviorIntent2D` | Chosen Instinct | The selected behavior category after scoring. |
| `NpcAiIntentTarget2D` | Chosen Anchor | The selected tactical node or hold-position anchor for the chosen instinct. |
| `NpcAiDecision2D` | AI Decision | Auditable composition of query, score, instinct, and anchor selection. |
| `NpcAiRouteRequest2D` | Route Request | Pre-navigation movement request from current NPC position to chosen anchor. |
| `NpcAiNavigationRequest2D` | Navigation Request | Existing navigation request shape after route validation. |
| `NpcAiPathReport2D` | Path Result | Read-only pathfinding result and diagnostics. |
| `NpcAiMovementProposal2D` | Move Proposal | The next movement point the NPC AI would ask for from the path. |
| `NpcAiMovementCommandMapper2D` | Command Mapper | Converts a move proposal into gameplay command data. |

## Tuning Field Labels

| Engine field | Ace-facing label | Meaning |
| --- | --- | --- |
| `patrolWeight` | Patrol Pull | How strongly a zone invites patrol behavior. |
| `coverWeight` | Cover Pull | How strongly a zone suggests taking cover. |
| `dangerWeight` | Danger Pressure | How strongly a zone signals danger or avoidance. |
| `interestWeight` | Interest Pull | How strongly a zone invites investigation. |
| `aggression` | Aggression | NPC tendency to favor assertive behavior. |
| `bravery` | Nerve | NPC willingness to tolerate danger. |
| `alertness` | Awareness | NPC sensitivity to interesting or risky context. |
| `preferredRange` | Comfort Range | Desired distance or engagement comfort band for future tuning. |
| `behaviorTags` | Instinct Tags | NPC temperament tags used to match map context. |
| `factionId` | Allegiance | NPC team or faction identity. |
| `links` | Lanes | Authored connections between tactical nodes. |
| `tags` | Zone Tags | Map-authored descriptors for a tactical node. |
| `enabled` | Active | Whether the node or NPC state participates in current decisions. |

## Behavior Labels

| Engine intent | Ace-facing label |
| --- | --- |
| `Patrol` | Patrol |
| `TakeCover` | Find Cover |
| `AvoidDanger` | Back Off |
| `Investigate` | Investigate |
| `HoldPosition` | Hold |
| `None` | No Action |

## Wiring Relationships

Use these relationship names when describing the AI tuning pipeline in UI plans or player-facing docs.

1. Zone Influence: Tactical Map Node -> Local Situation
2. Temperament Bias: NPC Temperament + Local Situation -> Instinct Scores
3. Instinct Choice: Instinct Scores -> Chosen Instinct
4. Anchor Choice: Chosen Instinct -> Chosen Anchor
5. Route Shape: Chosen Anchor -> Route Request -> Navigation Request -> Path Result
6. Movement Ask: Path Result -> Move Proposal
7. Runtime Action: Move Proposal -> Gameplay Command -> Runtime Queue
8. Roster Input: NPC Roster -> NPC AI State inputs for runtime queue steps

## Ownership Notes

- `scene/ai` owns map AI context, NPC AI temperament/state contracts, read-only decision reports, route/path reports, movement proposals, and command-frame mapping.
- `scene/npc` owns NPC actor identity/profile/current-goal registry data.
- Runtime owns queue intake and command execution timing.
- The Tactical Map is map-owned strategic structure.
- NPC Temperament is NPC-carried bias, not authored map truth.
- NPC Roster is actor state, not a behavior preset library and not runtime queue ownership.
- This vocabulary is allowed in UI labels and docs, but engine APIs should continue to use the existing type and field names until a dedicated rename slice exists.

# NPC Escape Target Research

Purpose: learn how reference games turn "move away from threat" into a concrete destination without making fleeing pretend to be `MoveTo`.

## Grep Commands Used

```sh
rg -n "mfndpos|flee|scared|mstrategy|dist2|monmove" NetHack-NetHack-5.0
rg -n "MonsterGoal::Retreat|AiAvoidance|RandomWalk|RoundWalk|M_FallenFear|IsTileAvailable" DevilutionX-master/Source
rg -n "AI_ESCAPE|setupEscape|_escapeAction|findReachable|getDangerous" OpenXcom-master/src/Battlescape
rg -n "SetFindPathAndFlee|SetFlee\\(|Flee\\(|OBJECTIVE_FLEE" re3-miami/src/peds
rg -n "pickATileGenThreat|AUXBITS_DANGER|dangerMap|threat|retreat" warzone2100-master/src
```

## NetHack

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:139`: live monster has `mflee`; `:141` stores flee timeout; `:170` has strategy bits.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mfndpos.h:16`: candidate info can include `NOTONL`, avoiding direct line to player; `:33` stores up to 9 candidate positions.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:458`: `monflee` starts timed or untimed fleeing; `:528` clears recent track history when made to flee.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1859`: fleeing flips approach sign to `-1`; `:1927` calls `mfndpos`; `:1969` compares candidate distance to target; `:1971` accepts non-nearer candidates while fleeing.

Shape: mostly local one-step candidate selection. Fleeing changes scoring direction; it does not require a far route target. Candidate flags carry passability/line/interaction details.

## DevilutionX

- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:123`: `MonsterGoal::Retreat`.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1732`: `RandomWalk` tries intended direction, then nearby turns through `DirOK`.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1791`: `IsTileAvailable` rejects occupied player/monster tiles and non-walkable tiles.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1836`: `RoundWalk` tries side/straight/opposite-side fallback.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1894`: `AiAvoidance` uses last-position direction, distance, same transparency area, and a short `goalVar` timer.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:2442`: retreat after close attack first tries opposite direction, then side directions.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:2497`: gargoyle retreats below half HP until enough distance, then heals or stops if blocked.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4450`: `M_FallenFear` marks nearby Fallen as retreating in radius 4, storing run distance and direction.

Shape: fleeing is a short-lived actor goal with cheap directional movement. It uses occupancy/passability at move time. It does not compute a full destination for most monsters.

## OpenXcom

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:36`: AI modes include `AI_ESCAPE`; `:47` stores escape TUs and action.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:153`: AI precomputes reachable tiles for current TUs.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:788`: `setupEscape` builds an escape action.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:812`: shuffled tile search; `:815` tries up to 150 candidates.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:885`: scores distance from aggro target; `:901` scores spotters/exposure; `:916` penalizes fire; `:920` penalizes dangerous tiles.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:902`: skips unreachable candidates; `:936` path-calculates only when a candidate can become best; `:964` fails to rethink if no tile works.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:1212`: `findReachable` flood-searches reachable tiles under TU/energy budget.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/TileEngine.cpp:3204`: danger zones are tile metadata used by AI.

Shape: turn-based escape is a scored destination search. It starts with reachable area, scores safety/exposure/distance, then routes to the chosen tile.

## re3 Miami

- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:583`: ped stores flee-from position/entity and timer; `:1100` `SetFindPathAndFlee` sets flee plus path-node seeking.
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:839`: `OBJECTIVE_FLEE_ON_FOOT_TILL_SAFE` starts fleeing from current position.
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:1031`: flee-from-character objective uses timed or indefinite flee.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:5729`: `SetFlee(position)` sets `PED_FLEE_POS`, run mode, source position, and timer.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:5758`: `SetFlee(entity)` sets `PED_FLEE_ENTITY`, source entity, run mode, and timer.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:5797`: `Flee` keeps running until timer and distance allow clearing.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:5839`: path-node flee chooses node heading from actor minus flee source.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:5871`: path graph selects next wandering node in the flee heading; fallback disables node seeking.

Shape: fleeing is stateful and timed. It can use path nodes to keep moving generally away from the source, then falls back to heading-based steering.

## Warzone 2100

- `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:193`: retreat data can hold an explicit flee-to position.
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:2812`: `pickATileGenThreat` searches expanding boxes for a passable tile with no threat in range.
- `/Users/kogaryu/iggy/warzone2100-master/src/map.cpp:2040`: danger flood fill marks accessible dangerous area.
- `/Users/kogaryu/iggy/warzone2100-master/src/map.cpp:2115`: threat update marks watched tiles from enemy weapons.
- `/Users/kogaryu/iggy/warzone2100-master/src/map.cpp:2208`: skirmish mode builds danger maps.
- `/Users/kogaryu/iggy/warzone2100-master/src/astar.cpp:134`: pathfinding context can carry `dangerMap`; `:173` exposes `isDangerous`; `:775` builds the danger map from threat bits.

Shape: larger-scale AI uses explicit retreat points plus map-owned threat/danger fields. This is useful later, but too heavy for the first Iggy flee slice.

## Iggy Anchors

- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorMovementIntent2D.hpp:10`: movement intent distinguishes `MoveTo` and `MoveAwayFrom`.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorMovementIntent2D.cpp:74`: fleeing control projects as `MoveAwayFrom` and preserves the source/threat position.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorRouteTarget2D.hpp:10`: route target statuses include `NeedsEscapeDestination`.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorRouteTarget2D.cpp:53`: `MoveAwayFrom` returns `NeedsEscapeDestination` unless config provides an escape destination.

## Recommendation: First Escape Target Slice

Add a pure/read-only `NpcActorEscapeTarget2D` projector before pathfinding.

Inputs:

- ready `NpcActorMovementIntent2D` of type `MoveAwayFrom`
- level/grid passability query only
- config: `maxCandidateRadius = 1`, `minDistanceGain = 0`, `arrivalTolerance`, deterministic tie-break order

Statuses:

- `Ready`
- `NoMovementIntent`
- `NotMoveAwayFrom`
- `ThreatAtActorPosition`
- `NoCandidates`
- `NoBetterCandidate`
- `InvalidMapQuery`

Candidate policy:

- First slice: local Moore-neighborhood candidates around actor, radius 1.
- Filter out off-map and blocked tiles.
- Do not inspect actor occupancy, LOS, cover, pathfinding, reservations, dynamic collision, or runtime state.
- Optionally expose candidate count and rejected count in the report.

Scoring policy:

- Primary: maximize squared distance from threat/source.
- Require candidate distance to be greater than current distance unless config allows lateral escape.
- Secondary: prefer lower travel distance from actor.
- Tertiary: stable deterministic order for snapshot tests.
- Do not pathfind per candidate. Pick one escape destination, then let the existing route target/path road handle routing.

Tests:

- adjacent open tile farther from threat is selected
- blocked/off-map candidates are ignored
- all candidates blocked returns `NoCandidates`
- only lateral/closer candidates returns `NoBetterCandidate`
- threat equal to actor position returns `ThreatAtActorPosition`
- tie-break is deterministic
- input intent and map data are not mutated

## Defer / Ownership Review

- Actor occupancy and reservations: needed before real execution, not for first escape target.
- LOS/visibility and cover scoring: belongs to later `AiMap2D`/map metadata scoring.
- Full reachable-area BFS: useful second version when radius 1 fails too often.
- Dynamic collision bodies: wait for collision-world ownership review.
- Multi-turn flee memory/backtracking: wait until actor control state owns previous escape facts.
- Runtime/session integration: keep this as scene/npc or scene/ai projection until executor boundaries are settled.

## Compute Notes

- Radius 1 local search is constant cost: at most 8 candidates.
- Radius `R` square scan is `O((2R + 1)^2)`.
- One cheap passability query per candidate is acceptable in unit tests and frame debug.
- Dead-end checks cost candidate count times neighbor count; keep optional.
- Pathfinding every candidate is the expensive OpenXcom-style version. Avoid it in the first slice.
- Map-owned danger/cover metadata can reduce repeated LOS/path scoring later, but should be a separate cache/query layer.

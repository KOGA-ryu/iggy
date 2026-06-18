# AI/NPC Department

## Charter

Own AI maps, profile traits, actor/NPC control and movement, navigation,
occupancy/reservation behavior, and the legacy-to-new NPC migration plan.

## Roles

- Planner: scopes AI/NPC behavior and fixture packets.
- Builder: implements AI/NPC feature slices and regression fixtures.
- Researcher: checks reference patterns, ownership, and compute costs.
- Reviewer: gates semantics, derived state, pathfinding/occupancy cost, and
  compatibility with old NPC state.
- Finisher: trims duplication around AI/NPC fixtures and movement reports.
- Apprentice/Spark: inventories role tags, movement cases, and fixture matrices.

## Bucket

1. AI-map region fixture coverage using current explicit C++ promotion config.
2. Multi-actor/profile fixture coverage.
3. Frame ordering policy tests where AI/NPC controls interleave with player
   commands.
4. Legacy NPC compatibility map updates after each migration-adjacent slice.
5. Region trigger semantics gate only if product roadmap demands it.

## Hard Stops

- Do not infer trigger/proximity/scripting semantics from regions.
- Do not make AI-map promotion default in facade/CLI without a specific gate.
- Do not change pathfinding, occupancy, or reservation semantics as fixture
  cleanup.
- Do not delete old NPC modules.

## Verification

AI-map/promoter/converter tests, movement/occupancy tests for behavior slices,
and full runtime/scene CTest lanes before integration when semantics change.

# AI Reference Studies

This folder collects compact studies of AI architecture in local reference games.
The goal is to learn ownership boundaries and reusable system shapes, not to copy
source code.

## Studies

- [AI Code Reading Playbook](ai-code-reading-playbook.md): reusable grep-driven method for turning unfamiliar AI code into an ownership map.
- [AI Code Shape and Runtime Cost](ai-code-shape-and-runtime-cost.md): how to read AI code for loops, path/LOS cost, cache rebuilds, allocation, save/load repair, and scaling pressure.
- [AI Compute Cost Ledger](ai-compute-cost-ledger.md): list of common AI/runtime compute costs, scaling shapes, per-game focus areas, and commands for finding hot spots.
- [AI Actor Movement Research](ai-actor-movement-research.md): focused learning map of objective, path, movement state, position mutation, occupancy, post-move work, and movement compute costs.
- [ASCII Dungeon Debug Projection Research](ascii-dungeon-debug-projection-research.md): NetHack/Edi/Iggy ownership map for ASCII import proof and cheap AI debug projection without making ASCII runtime truth.
- [AI Mutation, Runtime, Movement, and Queueing](ai-mutation-runtime-movement-queueing.md): how reference games separate proposals from authoritative mutation, runtime state, actor movement, and queued work.
- [NPC Actor Movement Planning Research](npc-actor-movement-planning-research.md): Iggy-specific route/path/step/executor slice plan from NPC control state to actor position mutation.
- [NPC Escape Target Research](npc-escape-target-research.md): reference map for turning `MoveAwayFrom` into an explicit escape destination without faking route targets.
- [AI Skeleton Matrix](ai-skeleton-matrix.md): cross-game map of definition data, live actor state, objectives, executor state, movement, map authority, interaction sites, director AI, and save/load boundaries.
- [AI Turn Pipeline Walkthroughs](ai-turn-pipeline-walkthroughs.md): per-game update paths showing how AI decisions become actions, movement, combat, or interaction work.
- [AI Data Structures Map](ai-data-structures-map.md): compact list of the major structs/classes/enums that hold AI-related state in each reference.
- [Interaction and Worksite AI](interaction-and-worksite-ai.md): comparison of jobs, rooms, attractors, orders, shops, doors, items, and world-owned interaction sites.
- [AI Implementation Tactics Examples](ai-implementation-tactics-examples.md): concrete examples of pointers, ids, arrays, vectors, flags, enums, switches, dispatch tables, commands, and save chunks.
- [AI Research Grep Commands](research-grep-commands.md): reusable terminal commands and search patterns used to locate the AI skeletons.
- [Programming Methods Tally](programming-methods-tally.md): cross-game tally of recurring AI programming strategies and implementation tactics used by the reference repos.
- [OpenXcom](openxcom.md): turn-based battlescape AI, action proposals, patrol nodes, pathfinding, and small persisted AI memory.
- [Warzone 2100](warzone2100.md): RTS unit AI, durable orders, current actions, movement/path control, groups, formations, and map query services.
- [NetHack](nethack.md): roguelike monster definitions, live monster state, movement candidate generation, special behavior hooks, pets, and save/restore.
- [DevilutionX](devilutionx.md): ARPG monster tables, live monster goals/modes, dungeon occupancy, path/LOS queries, and save/load rebuild boundaries.
- [KeeperFX](keeperfx.md): dungeon-sim creature jobs, rooms, creature control state, navigation, moods/needs, computer keeper tasks, and save/load chunks.
- [re3 Miami](re3-miami.md): open-world ped objectives, ped states, move modes, threat profiles, attractors, vehicle autopilot, and path graph ownership.

## Reading Lens

For each game, look for:

- what data is static definition data
- what data is live actor/session state
- what data is derived each turn or frame
- how decisions become executable actions
- what gets saved versus rebuilt
- how the same ownership boundaries repeat across different game genres

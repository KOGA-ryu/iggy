# AI Naming And Semantic Ownership

Purpose: engine-facing naming and ownership contract for the NPC AI lane. This document pauses new AI feature naming until the semantic rules below are explicit.

Use this document to review future build orders before adding new AI terms, cloning trait pools, or introducing role/profile vocabulary. Existing C++ names remain the source API names until a dedicated rename build order changes them.

## Authority

The user has direct semantic ownership of the NPC AI system. New major AI semantic layer names require user confirmation before implementation.

Current baseline:

- Existing `Npc*` vocabulary remains the baseline engine vocabulary unless explicitly renamed.
- Invented, rank-like, military, acronym, or project-language names are reserved for future role/profile layers only.
- Do not land invented/rank/acronym names in core engine AI types without explicit user approval.
- Do not introduce `Xed` naming in code for this layer.
- Trait behavior sources use `Pool`, individual records use `Ent`, and unlock/filter passes use `Draw`.

## Spatial Suffix Rule

Use `2D` only for types that directly own spatial 2D data or depend on 2D geometry, navigation, collision, map nodes, movement, path, or route data.

Drop `2D` for abstract AI semantics that do not directly own spatial data:

- traits
- trait-specific pools and draws
- behavior pools/stores where explicitly approved
- objectives
- move mode vocabulary
- behavior state vocabulary
- threat profiles
- behavior graph/profile stores

This rule is about semantic ownership, not just whether a type can eventually influence movement. If a type is abstract tuning/choice vocabulary, it should not carry the spatial suffix.

## Full Trait Names

Use full trait names in code, tests, docs, and future pool names:

- Strength
- Dexterity
- Constitution
- Intelligence
- Wisdom
- Charisma

Do not abbreviate Dexterity as Dex.

## Current Type Classification

### Keep `2D`

These types either own spatial data or depend on 2D map/navigation/path facts:

- `AiMap2D`
- `AiMapNode2D`
- `AiMapQuery2D`
- route request types that preserve `Vec2` start/target positions
- navigation request adapters that depend on navigation grids or spatial validation
- path reports that preserve path points/tiles
- movement proposal types that preserve current/target/proposed positions

### No-`2D` Abstract Names

These are abstract AI/profile/control concepts. Keep or move them toward no-`2D` names:

- `NpcTraitSet`
- `NpcStrengthPool`
- `NpcStrengthEnt`
- `NpcStrengthDraw`
- `NpcDexterityPool`
- `NpcDexterityEnt`
- `NpcDexterityDraw`
- `NpcConstitutionPool`
- `NpcConstitutionEnt`
- `NpcConstitutionDraw`
- `NpcIntelligencePool`
- `NpcIntelligenceEnt`
- `NpcIntelligenceDraw`
- `NpcWisdomPool`
- `NpcWisdomEnt`
- `NpcWisdomDraw`
- `NpcCharismaPool`
- `NpcCharismaEnt`
- `NpcCharismaDraw`
- `NpcAiBehaviorPool`
- `NpcObjective`
- `NpcMoveMode`
- `NpcBehaviorState`
- future `NpcThreatProfile`

Ambiguous current names:

- `NpcActorControlState2D` may become `NpcActorControlState` if it remains non-spatial control data.
- `NpcActorFrameState2D` is ambiguous because it copies `NpcActorState2D`, which owns actor position. Rename only after deciding whether this projection is considered a spatial frame view or an abstract control join.

## Rename Policy

Renames should be focused build slices. Each rename slice should:

- rename one narrow family at a time;
- update tests and CMake references;
- avoid behavior changes;
- avoid new feature work;
- preserve planner/reviewer ability to audit ownership.

Recommended next focused rename review:

1. Decide whether `NpcActorControlState2D` should become `NpcActorControlState`; it currently remains `2D` because the broader actor-control ownership question is still separate.
2. Decide whether `NpcActorFrameState2D` is spatial because it copies actor position, or abstract because it is a join projection.
3. Keep any future rename behavior-free and stop before runtime/session integration.

## Future Naming Gate

Before adding a new AI semantic layer, confirm the name with the user when it introduces:

- a new behavior taxonomy;
- a role/profile/pool/pool concept;
- a trait-specific concept beyond the existing Strength prototype;
- a threat, desire, utility, instinct, memory, or personality layer;
- any invented project-specific term.

Prefer conventional engine vocabulary until the user explicitly approves a richer semantic name.

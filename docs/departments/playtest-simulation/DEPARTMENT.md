# Playtest and Simulation

## Purpose

Own the separate executable and conversion boundary that turns authored Creative
content into a running gameplay world.

## Owns

- Playtest process launch, lifecycle, event protocol, and refusal behavior.
- Creative-to-runtime preparation and conversion.
- Runtime player, NPC, AI, physics, collision, movement, abilities, combat,
  interaction, inventory, objectives, projectiles, and targeting.
- Runtime reasoning and pathing derived from authored maps.
- Gameplay-specific fixtures and deterministic simulation proof.

## Does Not Own

- Creative editing UI or authoring history.
- Save-schema ownership.
- Renderer internals.
- Authoring semantics merely because runtime consumes their output.

## Dependency Direction

Consumes validated document snapshots through an explicit preparation boundary.
It may call runtime and renderer layers but must not mutate the live Creative
document. Failures return diagnostics to the editor through the playtest event
protocol.

## Primary Owners

- `apps/iggy3d_playtest/`
- `src/app/iggy3d/creative/play/`
- `src/runtime/` except persistence-specific subdirectories
- Playtest and gameplay fixtures

See [FILES.md](FILES.md) for the complete generated assignment.

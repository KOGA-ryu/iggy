# Playtest and Simulation

## Purpose

Own runtime gameplay applications and the separate executable and conversion
boundary that turns authored Creative content into a running gameplay world.

## Owns

- Playtest process launch, lifecycle, event protocol, and refusal behavior.
- Creative-to-runtime preparation and conversion.
- Runtime player, NPC, AI, physics, collision, movement, abilities, combat,
  interaction, inventory, objectives, projectiles, and targeting.
- Runtime reasoning and pathing derived from authored maps.
- Gameplay-specific fixtures and deterministic simulation proof.
- First Move's native Hunt game, reviewed challenge pack, row assessments,
  scoring, and gameplay-specific input/presentation adapters.

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
- `src/runtime/first_move/HuntSession.*` is the sole owner of First Move's
  mathematical classifications, row transitions, score, and initial attempts.
- `apps/first_move/` adapts native input and presents that state through the
  existing SDL/Vulkan/ImGui infrastructure. It does not load or mutate the
  Creative document and does not claim a durable study-history integration.
- Playtest and gameplay fixtures

See [FILES.md](FILES.md) for the complete generated assignment.

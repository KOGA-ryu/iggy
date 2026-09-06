# Paths

A native math game for exploring different ways to understand and solve the
same problem. Choose a mode from the title screen, work through questions, and
see what you answered and what help you used. Game points remain separate from
that learning record.

This folder is the standalone project root. It contains a pinned seed of First
Move's models and UI, the native helper code it needs, Dear ImGui, and reviewed
source-card plans. The independent executable/build is being implemented under
[P001](docs/P001_BUILD_PACKET.md); copying these files alone is not a completed
native migration.

- [Architecture](docs/ARCHITECTURE.md) — ownership, modes, evidence, and build boundary.
- [Workstreams](docs/WORKSTREAMS.md) — current build and following capabilities.
- [Source-card architecture](content/SOURCE_CARD_ARCHITECTURE.md) — two complete authored adaptations.
- [002: Three points, one formula](content/cards/002_guided.json) — 13 layers.
- [013: Closest point on a line](content/cards/013_guided.json) — 14 layers.
- [Planning validation](content/planning_validation.json) — exact arithmetic and provenance checks, separate from game tests.
- [Migration seed](docs/MIGRATION_SEED_MANIFEST.json) — where each borrowed file came from.

The planned build entry point is `cmake -S . -B build` from this directory,
then `cmake --build build --target paths`. SDL3 and Vulkan are external SDK
dependencies; the parent iggy3d checkout and build are not runtime dependencies.
P001's completion brief will record the actual successful build/run commands.


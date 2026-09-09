# Paths

Paths is a standalone C++ math-game project inside this checkout. Read
`docs/ARCHITECTURE.md` and the active packet in `docs/WORKSTREAMS.md` before
changing its behavior. Existing parent guidance applies to work in the parent
repository. Paths builds from its own CMake root and owns its game behavior.

## Current work allocation

The user assigns the entire textbook and application learning experience to
the textbook worker. This includes contents/curriculum, lessons, definitions,
notation, formatting, navigation, equation-solving questions, four support
levels, answer checks, progress, the authoring/import/export pipeline, and
integration of interactive figures into lessons and exercises.

The separate worker is assigned strictly to 3D assets and model construction:
geometry, animation and the model's controls, behavior and representation.
The textbook worker owns how those assets are presented, connected to a problem
and disclosed during learning. Reuse their existing contracts and preserve
concurrent asset edits. The researched textbook format is the shared baseline.
This supersedes the earlier split that limited the textbook worker to equations.
Responsibility for the full textbook does not authorize extra workers or
automatically start a queued batch.

## Working rules

- Work in `paths/`; no parent build, renderer, Creative, source-card, or Git
  changes are needed for the migration packet.
- Implement the assigned work directly in this session; do not dispatch work
  to another session or spawn workers.
- One capability per checkpoint. Keep work uncommitted and preserve unrelated
  changes. Do not initialize nested Git metadata as part of isolation.
- Use one semantic action route. Domain models own truth; UI presents it;
  statistics and scoring consume facts without rewriting attempts.
- Keep borrowed helper code and third-party sources attributed. Record the
  exact source seed in `docs/MIGRATION_SEED_MANIFEST.json`.
- Test pure models without SDL/Vulkan. Do not take screenshots or captures.
  Follow the image-free checks in the lesson workflow; the user performs visual
  confirmation after the build. Do not open a window unless explicitly asked.
- Source problem pages are read-only. Their learner Attempt/setup/solve fields
  are not game evidence. Ignore temporary authoring files.

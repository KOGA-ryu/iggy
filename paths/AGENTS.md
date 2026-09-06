# Paths

Paths is a standalone C++ math-game project inside this checkout. Read
`docs/ARCHITECTURE.md` and the active packet in `docs/WORKSTREAMS.md` before
changing its behavior. Existing parent guidance applies to work in the parent
repository. Paths builds from its own CMake root and owns its game behavior.

- Work in `paths/`; no parent build, renderer, Creative, source-card, or Git
  changes are needed for the migration packet.
- The user has taken work back from the cross-chat builder. Implement directly
  in this session; do not dispatch work to another session or spawn workers.
- One capability per checkpoint. Keep work uncommitted and preserve unrelated
  changes. Do not initialize nested Git metadata as part of isolation.
- Use one semantic action route. Domain models own truth; UI presents it;
  statistics and scoring consume facts without rewriting attempts.
- Keep borrowed helper code and third-party sources attributed. Record the
  exact source seed in `docs/MIGRATION_SEED_MANIFEST.json`.
- Test pure models without SDL/Vulkan. Use targeted offscreen native captures
  for rendering. A passing offscreen capture does not establish interactive
  swapchain or human pointer acceptance. Do not open a visible window unless
  the user requests it.
- Source problem pages are read-only. Their learner Attempt/setup/solve fields
  are not game evidence. Ignore temporary authoring files.

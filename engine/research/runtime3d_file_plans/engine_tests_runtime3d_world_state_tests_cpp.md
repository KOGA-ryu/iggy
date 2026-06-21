# `engine/tests/runtime3d_world_state_tests.cpp`

Purpose: prove runtime3d entity collection behavior.

Must test:

- Add a player entity with stable id.
- Find entity by id.
- Missing id returns null/no result.
- Upsert same id updates the existing entity instead of appending a duplicate.
- Upsert or add behavior with invalid id is deterministic.

Construction rules:

- Use small in-memory world only.
- Do not load content packages or use renderer code.

Completion:

- Test executable builds and passes.


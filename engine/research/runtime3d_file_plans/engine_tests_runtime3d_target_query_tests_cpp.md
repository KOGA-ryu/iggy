# `engine/tests/runtime3d_target_query_tests.cpp`

Purpose: prove runtime3d target discovery.

Must test:

- Pickup or door can be discovered as targetable.
- Source actor is not returned as its own target.
- Floor/wall are ignored unless explicitly targetable.
- Tie behavior is deterministic.
- No-target status is explicit.

Construction rules:

- Use in-memory world only.
- Keep target query linear until scale requires indexing.

Completion:

- Test executable builds and passes when target query is implemented.


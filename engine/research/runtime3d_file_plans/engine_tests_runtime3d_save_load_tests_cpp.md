# `engine/tests/runtime3d_save_load_tests.cpp`

Purpose: prove in-memory runtime3d save/load semantics before file IO.

Must test:

- Save copies package identity, scenario identity, clock state, camera state,
  world state, and command data into an envelope.
- Compatible load returns replacement session state.
- Incompatible version or identity returns incompatible status.
- Incompatible load does not mutate active state.
- Raw input and renderer data are absent from envelope.

Construction rules:

- No filesystem, save slots, or binary codec.

Completion:

- Test executable builds and passes when save/load is implemented.


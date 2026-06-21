# `engine/src/runtime3d/Runtime3DSaveLoad.cpp`

Purpose: implement in-memory save/load conversion.

Must contain:

- Include `runtime3d/Runtime3DSaveLoad.hpp`.
- Save method that copies package/scenario identity, clock, camera, world, and
  command data into an envelope.
- Load method that checks version and expected identity before returning active
  session state.
- Incompatible status that does not mutate active state.

Construction rules:

- No file IO, no slot store, no binary codec yet.

Completion:

- `runtime3d_save_load_tests.cpp` can prove conversion semantics.


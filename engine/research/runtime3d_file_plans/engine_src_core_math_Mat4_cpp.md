# `engine/src/core/math/Mat4.cpp`

Purpose: implement pure backend-free matrix helpers.

Must contain:

- Include `core/math/Mat4.hpp`.
- Implement identity, multiplication, translation, scale, X/Y rotation,
  perspective, and look-at helpers when declared.

Construction rules:

- Copy only pure math behavior from `engine/apps/native_play/NativePlayMath.hpp`.
- Preserve current perspective sign conventions until renderer tests require a
  change.
- Keep the file allocation-free.

Compute cost:

- `Multiply` is fixed-size `O(1)`.
- All other helpers are `O(1)`.

Completion:

- Builds through `iggy_core_sources.cmake`.


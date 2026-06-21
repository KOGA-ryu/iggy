# `engine/src/runtime3d/Runtime3DSaveLoad.hpp`

Purpose: declare conversion between active runtime3d session state and in-memory
save envelopes.

Must contain:

- `#pragma once`.
- Include `runtime3d/Runtime3DSaveEnvelope.hpp` and
  `runtime3d/Runtime3DSessionState.hpp`.
- Namespace `iggy::runtime3d`.
- Save/load status enums.
- `struct Runtime3DSaveResult`.
- `struct Runtime3DLoadInput`.
- `struct Runtime3DLoadResult`.
- `class Runtime3DSaveLoad`.

Construction rules:

- Save creates envelope from active state.
- Load validates compatibility before returning replacement state.
- No filesystem in skeleton.

Completion:

- Save/load tests can prove compatible and incompatible loads.


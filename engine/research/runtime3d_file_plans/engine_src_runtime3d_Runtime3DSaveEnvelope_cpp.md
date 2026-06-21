# `engine/src/runtime3d/Runtime3DSaveEnvelope.cpp`

Purpose: implement save-envelope helpers.

Must contain:

- Include `runtime3d/Runtime3DSaveEnvelope.hpp`.
- Optional default-version helper.
- Optional compatibility helper once load policy needs it.

Construction rules:

- Keep encoding out of this file.
- Keep helpers deterministic and pure.

Completion:

- Builds through `iggy_runtime3d_sources.cmake`.


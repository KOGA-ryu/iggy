# `engine/src/runtime3d/Runtime3DSaveEnvelope.hpp`

Purpose: declare the in-memory runtime3d save envelope shape before file
encoding.

Must contain:

- `#pragma once`.
- Include runtime3d session, clock, camera, world, and command headers as
  needed without creating cycles.
- Namespace `iggy::runtime3d`.
- `struct Runtime3DSaveEnvelope`.

Envelope fields:

- format version;
- package identity;
- scenario identity;
- lifecycle;
- clock state;
- camera mode state;
- world state;
- command log or command log summary;
- compatibility metadata placeholders.

Construction rules:

- No filesystem or binary codec here.
- No renderer draw lists, raw input events, or matrices as save truth.

Completion:

- Save/load conversion can build and validate envelopes.


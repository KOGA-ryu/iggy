# `engine/src/runtime3d/Runtime3DSession.hpp`

Purpose: declare the value-style runtime3d session operation facade.

Must contain:

- `#pragma once`.
- Include `runtime3d/Runtime3DSessionState.hpp`.
- Namespace `iggy::runtime3d`.
- `class Runtime3DSession`.

Operations:

- create empty loaded/playing session;
- set lifecycle;
- apply clock decision or return updated state;
- later admit/apply commands through command admission.

Construction rules:

- Value-in/value-out style to match current runtime patterns.
- No native input, renderer, SDL, Qt, or Vulkan.

Completion:

- Session tests can use the facade without direct field mutation if desired.


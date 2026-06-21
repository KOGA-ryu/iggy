# `engine/src/runtime3d/Runtime3DCameraModePolicy.hpp`

Purpose: declare the policy that maps runtime clock/session mode to active
camera mode.

Must contain:

- `#pragma once`.
- Include camera and clock headers.
- Namespace `iggy::runtime3d`.
- `struct Runtime3DCameraModePolicyInput`.
- `struct Runtime3DCameraModePolicyResult`.
- `class Runtime3DCameraModePolicy` with a pure update/evaluate method.

Input fields:

- current camera state;
- clock mode;
- lifecycle or tactical-planning flag;
- preferred tactical mode if needed.

Result fields:

- updated camera state;
- `bool changedMode`;
- `bool clearTransientInput`.

Construction rules:

- Avoid include cycles with session state. If needed, pass a boolean tactical
  planning flag instead of including the full session state.

Completion:

- Camera mode policy tests can exercise real-time to tactical transitions.


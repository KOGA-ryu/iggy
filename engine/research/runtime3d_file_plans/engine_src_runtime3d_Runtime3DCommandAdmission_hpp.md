# `engine/src/runtime3d/Runtime3DCommandAdmission.hpp`

Purpose: declare command validation/admission for runtime3d.

Must contain:

- `#pragma once`.
- Include `runtime3d/Runtime3DCommand.hpp` and
  `runtime3d/Runtime3DWorldState.hpp`.
- Namespace `iggy::runtime3d`.
- `struct Runtime3DCommandAdmissionInput`.
- `struct Runtime3DCommandAdmissionResult`.
- `class Runtime3DCommandAdmission`.

Input fields:

- world state or const world reference by value/result style;
- proposed command record.

Result fields:

- command record with admission status/reason filled;
- optional `bool admitted`.

Construction rules:

- This file owns validation shape, not mutation.
- Do not include session unless world plus command are insufficient.

Completion:

- Admission implementation can reject invalid actors without mutating state.


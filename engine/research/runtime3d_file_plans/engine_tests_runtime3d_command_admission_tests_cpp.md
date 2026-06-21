# `engine/tests/runtime3d_command_admission_tests.cpp`

Purpose: prove command admission accepts valid commands and rejects invalid
commands without mutation.

Must test:

- Actor command with invalid actor rejects.
- Actor command with missing actor rejects.
- Actor command with existing actor admits.
- Interact command with missing target rejects.
- Session command such as Save/Reset can admit without actor if policy allows.
- Rejected command records deterministic reason.
- World state is unchanged after rejection.

Construction rules:

- Use in-memory world and command records.
- No raw input or native app types.

Completion:

- Test executable builds and passes when admission file is implemented.


# `engine/tests/runtime3d_acceptance_demo_tests.cpp`

Purpose: prove the shippable runtime3d demo loop once all pieces exist.

Must test:

- Load or create demo state.
- Discover target.
- Reject out-of-reach interaction.
- Move or reposition into reach.
- Interact with pickup or door.
- Enter slow time and tactical camera.
- Issue a tactical command.
- Pause, step, and resume.
- Save and load.
- Retry or reset.
- Produce deterministic final state summary.

Construction rules:

- Runtime-level acceptance first; native-window proof can be separate.
- No renderer state in save assertions.

Completion:

- Acceptance test passes and demonstrates the playable loop.


# `engine/src/runtime3d/Runtime3DCommandAdmission.cpp`

Purpose: implement skeletal command validation.

Must contain:

- Include `runtime3d/Runtime3DCommandAdmission.hpp`.
- Switch on command kind.
- Actor commands require valid actor id and existing world entity.
- Interact/inspect with target entity require target exists.
- Session commands such as save/load/reset/retry can be admitted without actor
  if policy allows.
- Rejected records get deterministic rejection reason.

Construction rules:

- No world mutation in the skeleton.
- Reach/range can remain placeholder until target query and volumes are active.

Completion:

- `runtime3d_command_admission_tests.cpp` can prove accept/reject behavior.


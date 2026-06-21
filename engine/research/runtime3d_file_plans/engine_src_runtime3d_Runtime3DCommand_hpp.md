# `engine/src/runtime3d/Runtime3DCommand.hpp`

Purpose: declare serializable runtime3d player/session command records.

Must contain:

- `#pragma once`.
- Includes for `<cstdint>`, `core/math/Vec3.hpp`, and
  `runtime3d/Runtime3DEntityId.hpp`.
- Namespace `iggy::runtime3d`.
- `enum class Runtime3DCommandKind` with `Move`, `Interact`, `Inspect`, `Wait`,
  `ToggleTacticalMode`, `StepTacticalTick`, `Retry`, `Reset`, `Save`, `Load`.
- `enum class Runtime3DCommandAdmissionStatus` with at least `Pending`,
  `Accepted`, `Rejected`.
- `enum class Runtime3DCommandRejectionReason` with at least `None`,
  `InvalidActor`, `MissingActor`, `InvalidTarget`, `OutOfRange`,
  `Unsupported`.
- `struct Runtime3DCommandRecord`.

Command record fields:

- command id;
- sequence;
- player slot;
- actor entity id;
- command kind;
- target entity id plus `hasTargetEntity`;
- target point plus `hasTargetPoint`;
- issued tick;
- admission status;
- rejection reason.

Construction rules:

- Store command facts only. No raw keyboard, mouse, SDL, Qt, or renderer data.
- Keep fields save/replay ready.

Completion:

- Command admission can accept and return this record by value.


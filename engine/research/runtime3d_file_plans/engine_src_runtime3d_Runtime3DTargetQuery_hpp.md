# `engine/src/runtime3d/Runtime3DTargetQuery.hpp`

Purpose: declare target discovery over runtime3d world state.

Must contain:

- `#pragma once`.
- Include `runtime3d/Runtime3DWorldState.hpp`, `runtime3d/Runtime3DCommand.hpp`,
  and `core/math/Ray3.hpp` if ray input is supported.
- Namespace `iggy::runtime3d`.
- `enum class Runtime3DTargetQueryStatus`.
- `struct Runtime3DTargetQueryInput`.
- `struct Runtime3DTargetQueryResult`.
- `class Runtime3DTargetQuery`.

Input fields:

- world state;
- actor id or source point;
- command/action kind;
- optional ray;
- max range.

Result fields:

- status;
- target entity id;
- target point;
- rejection/no-target reason.

Construction rules:

- Start with linear scan.
- Prefer targetable kinds: pickup, door, enemy, objective, tactical marker.
- Exclude invalid ids and source actor.

Completion:

- Target query tests can prove targetability and tie behavior.


# Runtime Movement Policy

Status: implemented as hardcoded runtime policy
Primary files:

- `src/runtime/movement/MovementParams.hpp`
- `src/runtime/movement/MovementPolicy.hpp`
- `src/runtime/movement/MovementPolicy.cpp`
- `src/runtime/movement/MovementSystem.hpp`
- `src/runtime/movement/MovementSystem.cpp`
- `apps/iggy3d_collision_probe/main.cpp`

This packet keeps movement policy hardcoded but isolated. The movement solver
does not scatter slope constants through collision code. It asks the policy
table for a slope band, then applies the resolved multipliers.

## Hardcoded Policy Table

Current slope bands:

```text
flat      0-5 degrees    speed 1.00  stamina 1.00  step 1.00  careful false
easy      5-15 degrees   speed 0.92  stamina 1.10  step 1.05  careful false
moderate 15-28 degrees  speed 0.75  stamina 1.35  step 1.20  careful true
steep     28-40 degrees  speed 0.45  stamina 1.80  step 1.60  careful true
blocked   40-90 degrees  speed 0.00  stamina 0.00  step 0.00  careful true
```

The table is intentionally compiled C++ for now. Later, the same shape can move
to authored data without changing the movement solver.

## Kinematic Movement Order

The kinematic path is opt-in through `executeKinematicMovement`.

```text
actor position
-> sample current ground
-> sample slope normal and resolve slope band
-> normalize horizontal intent
-> apply speed multiplier
-> project movement onto ground plane
-> segment query against actor blockers
-> clamp before blocker
-> slide along hit normal when possible
-> snap final point to walkable ground
-> commit transform once
```

Existing direct command movement is preserved for legacy runtime/session tests.
Collision-aware movement is now available for probes and the next runtime
integration packet.

## Probe Commands

Flat movement:

```sh
./build/iggy3d_collision_probe \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --query move \
  --units feet \
  --start 10,0.05,9 \
  --intent 0,0,-1 \
  --seconds 0.5
```

Expected fields:

```text
movement_accepted=true
movement_policy_band=flat
movement_clamped=false
movement_slid=false
ground_snap_applied=false
```

Move into the north wall:

```sh
./build/iggy3d_collision_probe \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --query move \
  --units feet \
  --start 10,0.05,9 \
  --intent 0,0,-1 \
  --seconds 2.0
```

Expected behavior:

- movement remains accepted if it can clamp before the wall;
- `movement_clamped=true`;
- `hit_surface_id=north_wall_actor_blocker`;
- final `z` stays outside the blocker.

Diagonal move into the north wall:

```sh
./build/iggy3d_collision_probe \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --query move \
  --units feet \
  --start 10,0.05,9 \
  --intent 1,0,-1 \
  --seconds 1.4
```

Expected behavior:

- `movement_clamped=true`;
- `movement_slid=true`;
- `collision_sweep_count=2`;
- final position advances sideways while staying outside the wall.

If `--seconds` is pushed too high, the slide can leave the current floor
footprint and return `movement_reason=no_walkable_ground`. That is intentional
for this packet: the solver refuses to commit movement when the destination has
no walkable ground sample.

## Ownership

- Movement policy owns hardcoded slope bands.
- Movement system owns movement decisions and the one transform mutation.
- Runtime collision owns query facts.
- Content owns authored surfaces and masks.
- Render/Vulkan/save/replay are not movement authorities.

## Deferred

- stamina spending;
- actor load;
- ability overrides;
- downhill acceleration;
- forced sliding on steep downhill;
- step-up movement;
- authored external movement policy.

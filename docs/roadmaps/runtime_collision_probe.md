# Runtime Collision Probe

Status: internal test tool
Tool: `iggy3d_collision_probe`

This tool exposes the runtime collision query core from the terminal. It is for
playing with spatial seams before movement, projectiles, and tactical preview
consume the query layer.

The probe prints key/value receipts only. It does not emit object-serialized
receipts, mutate `WorldState`, create render resources, change saves, or
simulate projectiles.

## Build

```sh
cmake --build build --target iggy3d_collision_probe collision_probe_smoke -j 8
```

## Surface List

```sh
./build/iggy3d_collision_probe \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --list-surfaces
```

Use this first. It prints each authored collision surface, its role, shape,
meters bounds, normal, actor/projectile masks, blocker booleans, and whether it
is an opening.

## Default Suite

```sh
./build/iggy3d_collision_probe \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --query suite
```

The suite checks the current first-room seams:

- spawn floor height sample hits `spawn_floor_walkable`;
- spawn floor normal is world up;
- actor segment into north wall hits `north_wall_actor_blocker`;
- actor segment through east opening returns no hit;
- projectile segment into crate hits `spawn_crate_projectile_blocker`;
- actor segment into the projectile-only crate returns no hit;
- projectile point overlap on the crate hits the projectile blocker.

Expected receipt fields:

```text
surface_height_sampled=true
surface_normal_valid=true
actor_blocker_hit=true
opening_blocks_actor=false
projectile_blocker_hit=true
query_result_order=stable
result=pass
```

## Manual Probes

Height sample near player spawn, using authored feet coordinates:

```sh
./build/iggy3d_collision_probe \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --query height \
  --units feet \
  --point 10,6.5,9
```

Actor segment into north wall, using meters:

```sh
./build/iggy3d_collision_probe \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --query segment \
  --kind actor \
  --start 3.048,1,2 \
  --end 3.048,1,-1
```

Actor segment through east opening, using feet:

```sh
./build/iggy3d_collision_probe \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --query segment \
  --kind actor \
  --units feet \
  --start 18,3.28,9 \
  --end 22,3.28,9
```

Projectile segment into the crate, using feet:

```sh
./build/iggy3d_collision_probe \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --query segment \
  --kind projectile \
  --units feet \
  --start 4,1,14 \
  --end 4,1,18
```

Actor segment into the projectile-only crate should not block:

```sh
./build/iggy3d_collision_probe \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --query segment \
  --kind actor \
  --units feet \
  --start 4,1,14 \
  --end 4,1,18
```

## Edge Cases To Play With

- Move `--point` just outside floor bounds and watch height go from `hit` to
  `no_hit`.
- Raise or lower segment `y` and watch thin authored bounds stop matching.
- Compare `--kind actor` and `--kind projectile` against the crate.
- Compare `--kind actor` and `--kind opening` near the east opening.
- Change segment start/end order to inspect time-of-impact and distance.
- Use `--query aabb --kind actor --min ... --max ...` to check a candidate
  capsule-footprint box before the movement packet exists.

## Ownership

- Content owns the authored spatial surface data.
- Runtime collision owns the derived query view and query result values.
- This probe owns only command-line inputs and printed receipts.
- Render, Vulkan, save/load, and replay are not involved.

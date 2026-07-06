# PHYSICS LANE MAXIMUM — the consolidated spine

Status: v1.0, written 2026-07-06 by reconciling the six `docs/plan_bucket/physics_*_v0_1.md` contracts + `docs/roadmaps/runtime_physics_engine_scout.md` + `runtime_collision_probe.md` against ACTUAL shipped code. Docs are v0.1 intent (Jun 27, greenfield); code is truth (~28 `src/runtime/physics/*` files + `src/runtime/collision/*` + 14 `tests/unit/physics_*_tests.cpp`). Where they disagree, code wins and the drift is logged.

CUT ALL FUTURE PHYSICS SLICES FROM THIS MAP. Cite coordinates. Version-bump on corrections.

## 0. The one-sentence truth
Physics in iggy3d is TWO stacks: **collision/** — a shipped deterministic kinematic query core over authored spatial surfaces (this is the real footing substrate, per the scout's Final Recommendation) — and **physics/** — a built-but-mostly-gated bespoke AABB engine (bodies, broadphase, contacts, sequential-impulse solver, material traits, kinematic motor). Gravity=data lives in the movement lane (the lawless Controller integrator today; `PlayerMotor` is its orphaned destiny motor), not here. See `docs/movement-abilities-maximum.md` §1 for the exact home of the live gravity integrator.

## 1. Streams
- **PH1 — Authored-surface collision (collision/) [SHIPPED, LOAD-BEARING].** `RoomAsset -> buildSpatialSurfaceSet -> SpatialSurfaceSet{roles Walkable/Blocker/ProjectileBlocker/Opening, shapes Box/Plane/Opening}`; queried by `querySegment / sampleSurfaceHeight / sampleSurfaceHeightAtOrBelow / sampleSurfaceNormal / queryPointOverlap / queryAabbOverlap`. Files: `src/runtime/collision/{SpatialSurfaceSet,CollisionQuery,CollisionTypes,EntityHitQuery}`.
- **PH2 — Physics math + body data (physics/) [SHIPPED].** SoA bodies, semi-implicit Euler step. Files: `PhysicsTypes`, `PhysicsBodyStore` (`stepPhysicsBodies`/`integratePhysicsBodyVelocities`/`integratePhysicsBodyPositions`), `PhysicsShapeStore`, `PhysicsAabbCollider`.
- **PH3 — Physics collision queries (physics/) [SHIPPED].** `PhysicsCollisionQueries` (overlap/raycast/swept-AABB/ground-probe), `PhysicsBroadphase` (uniform grid), `PhysicsAabbContact`, `PhysicsAabbContactSolver` (sequential impulse).
- **PH4 — Material traits (physics/) [SHIPPED].** `PhysicsMaterialTraits`: 13 flags, friction/restitution/damping/weight-class, material-pair table. Wired to bake role-flags; solver not yet live.
- **PH5 — Bake bridge (physics<->creative) [SHIPPED].** `bakePhysicsAabbCollidersFromSpatialSurfaces`: SpatialSurfaceSet -> `PhysicsAabbCollider[]` carrying roles/shapes/source ids/owner stable names.
- **PH6 — Kinematic motor + move planner (physics<->movement) [BUILT, GATED OFF].** `PhysicsKinematicMotor.planPhysicsKinematicAabbMove` (sweep/slide/ground-snap) -> `PlayerPhysicsMovePlanner.planPlayerPhysicsMove` -> `MovementSystem.executePhysicsPlannedMovement`, entered only when `usePhysicsMovePlanner==true` (default false everywhere).
- **PH7 — Telemetry + lab (physics/) [SHIPPED benchmark; lab contract PLANNED].** `PhysicsFrameStats`, `PhysicsDebugSnapshot`, `PhysicsKernelBenchmark(+Json)` over real room assets. The scenario-receipt `PhysicsLab` from the contract is not built.
- **PH-LIVE — the one live gameplay consumer.** Session bakes colliders once + raycasts them for NPC vision/LOS + blocker checks (`Session.cpp:1096-1105, 1002, 1047`). This is the only place physics/ colliders drive shipped gameplay.

## 2. The DAG
```
content/assets/RoomAsset  [creative]
 -> buildSpatialSurfaceSet [PH1]
 -> SpatialSurfaceSet {roles,shapes}   <-- creative<->physics vocabulary seam
      |-> CollisionQuery.* [PH1]  -> MovementSystem footing (DEFAULT)  -> thief/guard walk
      |-> bake* [PH5] -> PhysicsAabbCollider[]
            |-> raycastPhysicsAabbs [PH3] -> Session NPC LOS [PH-LIVE, SHIPPED]
            |-> PhysicsKinematicMotor [PH6] -> PlayerPhysicsMovePlanner -> MovementSystem (flag-OFF)
PhysicsTypes -> PhysicsBodyStore(step) -> Broadphase -> CollisionQueries -> Contact -> Solver  [PH2/PH3, kernels only]
PhysicsMaterialTraits [PH4] -> (bake flags now; solver later)
PhysicsKernelBenchmark [PH7] = de-facto lab
```

## 3. Laws
- **L-PHY-1** Fixed timestep, never render dt; semi-implicit Euler `v+=g*h; x+=v*h`.
- **L-PHY-2** Reject work in layers: active-flag -> broadphase cell -> AABB -> narrowphase -> manifold -> solver; broadphase = uniform hash grid first.
- **L-PHY-3** No render triangles as collision truth — floors/walls become authored primitives.
- **L-PHY-4** No trig/strings/alloc in hot loops; slope = `dot(n,up)>=cosMax`; distance-squared compares; precomputed damping; expensive math needs an approval note.
- **L-PHY-5** Tables not ladders — material behavior via precomputed pair table; solver never branches on names.
- **L-PHY-6** SoA hot / AoS cold.
- **L-PHY-7** Fail-closed validation is the only sanctioned branch class; every API returns status + reasonCode (this is the doctrine's validate->repair->receipt at the type layer).

## 4. Decisions
- **D-PHY-1** Bespoke kinematic core first, Jolt later (revisit only after content-owned surfaces + deterministic query tests + kinematic proof + replay proof).
- **D-PHY-2** collision/ stays the movement/projectile truth; physics/ is additive, collision/ not rewritten.
- **D-PHY-3** Physics enters movement ONLY behind `usePhysicsMovePlanner` (default false) — the as-built relaxation of the docs' hard "must not change MovementSystem" boundary.

## 5. Shipped / Planned / Stale
- **SHIPPED:** PH1 collision core; PH2 body/math; PH3 queries/broadphase/contact/solver kernels; PH4 material traits; PH5 bake bridge; PH6 motor+planner (gated); PH7 frame-stats/snapshot/benchmark; PH-LIVE Session LOS; gravity=data consumed by movement (Controller ×18) + ProjectileSystem (9.8); 14 test suites.
- **PLANNED:** `PhysicsStaticRoomAdapter`, `PlayerPhysicsController`, `SessionPhysicsAdapter` (names reserved, roles filled ad hoc); scenario-receipt `PhysicsLab` + `ProductPhysicsLab`; orchestrated per-tick dynamic step loop; islands/warm-start/TGS/XPBD/GJK-EPA/AABB-tree (deferred); projection debug-draw of colliders/normals.
- **STALE:** doc file-names (PhysicsStep/PhysicsMath/PhysicsContact/PhysicsSolver singular) vs shipped; repo_placement "must not change MovementSystem" superseded by the flag; lab contract's PhysicsLab vs shipped KernelBenchmark; all v0.1 docs pre-date the code and read greenfield.

## 6. Vertical-slice critical path (thief -> room -> guard -> notebook -> duo)
1. **[S]** Freeze the collision/ footing path for the slice (usePhysicsMovePlanner=FALSE). Thief + guard walk on `querySegment` + `sampleMovementGroundAtOrBelow`. No new physics. OVERKILL-FLAG: do NOT light the kinematic motor.
2. **[S]** Verify the creative->physics bake produces correct collider roles for the slice room (walkable floors, wall blockers, openings) — this is what the guard's LOS rays hit.
3. **[S]** Confirm `sampleSurfaceHeightAtOrBelow` as the canonical floor-height seam the thief's jump/vault integrator lands on.
4. **[M]** One receipt-producing lab scenario (room-collision, reusing the benchmark harness): prove the slice room bakes to valid non-overlapping colliders. This is the lane's REPORT leg (doctrine minimum).
5. **[XL, DEFERRED]** Flip to the physics kinematic motor as live footing (+ PlayerPhysicsController/SessionPhysicsAdapter). NOT for the first slice — collision/ suffices; build only when a room needs dynamic/pushable props.

## 7. Seams (named concretely)
- **physics -> movement (footing):** `CollisionQuery.sampleSurfaceHeightAtOrBelow` / `querySegment(...,CollisionQueryKind::Actor)` over SpatialSurfaceSet [SHIPPED default]. Alternate: `planPlayerPhysicsMove` behind `usePhysicsMovePlanner` [gated].
- **physics -> AI (LOS):** `bakePhysicsAabbCollidersFromSpatialSurfaces` -> `raycastPhysicsAabbs` in Session [SHIPPED, live].
- **physics -> foundational (receipt):** `PhysicsFrameStats` + `PhysicsDebugSnapshot` on `MovementResult`.
- **creative -> physics (need):** `RoomAsset` surfaces with correct `CollisionSurfaceRole/Shape`; stable owner ids -> `runtimeOwnerStableNames` (so the notebook can name cover points by authored id).
- **movement -> physics (need):** `MovementParams` footprint (radius 0.30, height 1.80, slope 40deg) must match physics ground/slope thresholds or footing disagrees with traversal. NOTE: never pinned as a contract — silent-divergence hazard; pin it when the slice's step 4/5 run.
- **foundational -> physics (need):** fixed-tick cadence + bake-once-at-load lifecycle (SessionTick) for deterministic LOS + footing (L-PHY-1).

## 8. The precise physics<->movement seam sentence
Physics ENDS at "baked static colliders + deterministic queries over authored surfaces." Movement/player BEGINS at "gravity integrator + jump/dash/airborne-phase state." gravity=data is the movement lane's, not physics/'s. The two touch at exactly one function today for footing (`CollisionQuery`) and one opt-in alternate (`PlayerPhysicsMovePlanner`), and at one function for AI (`raycastPhysicsAabbs`).

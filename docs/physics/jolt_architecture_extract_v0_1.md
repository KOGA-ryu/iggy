# Jolt Physics — Architecture Extract for the iggy3d Physics Engine

**Version** v0.1 · 2026-07-12 · read-only study, no Jolt code copied
**Source** `~/game-references/JoltPhysics-master/Jolt/` — 534 files, ~114 K LOC. The rigid-body + collision library shipping in *Horizon Forbidden West* and *Death Stranding 2*.
**Purpose** Extract Jolt's architecture so we can design our **own** leaner engine for a stylized low-poly parkour-stealth game (character controller · kinematic + dynamic bodies · raycasts for line-of-sight · determinism for co-op netcode). This is a map of *why the pieces are shaped the way they are* and *what to steal vs skip*, not a tutorial.
**How to read** §0 is the payload (the ideas + the build order). §1–§4 are the architecture. §5 grounds it against `src/runtime/physics`. §6–§8 are the actionable ledger + read-first index.

> **Two corrections to common lore, verified in the source:**
> 1. Jolt's rigid-body solver is **not TGS**. It is classic **sequential-impulse / projected Gauss-Seidel** with a separate Baumgarte position pass. Catto-style *soft* constraints appear only where a `SpringSettings` frequency is set. (TGS/XPBD sub-stepping is used only by SoftBody/Hair.)
> 2. **SoftBody is CPU-only.** The GPU `Compute/` + `Shaders/` directories are owned by the separate `Physics/Hair/` strand solver, not SoftBody.

---

## 0. TL;DR — the ideas worth stealing, leverage-ranked

Ranked by *value to our engine ÷ cost to build*. The top 6 are cheap and high-leverage; the bottom 3 are the "only when we add dynamics" tier.

| # | Idea | Where in Jolt | Why it matters for us |
|---|------|---------------|-----------------------|
| 1 | **Handle = index + generation counter** (`BodyID` = 23-bit index \| 8-bit sequence) over an array with an intrusive free-list | `Body/BodyID.h`, `BodyManager.cpp:241-273` | Stable, cheap, delete-safe references. Foundation for co-op determinism and safe cross-frame refs. We already have `PhysicsBodyId` — add the generation field. |
| 2 | **Per-frame bump allocator** (`TempAllocator`: pointer-add alloc, LIFO free, one rewind at frame end) | `Core/TempAllocator.h:34-131` | ~100 LOC, near-free allocation for all transient step data (pairs, manifolds, raycast scratch). Biggest perf-per-effort win in the engine. |
| 3 | **Two-level collision filter** (coarse `BroadPhaseLayer` → one tree each; fine `ObjectLayer` → bit-table pair rules) | `Collision/ObjectLayer.h`, `BroadPhase/BroadPhaseLayer.h` | Static geometry lives in a never-rebuilt tree; LOS raycasts open only the layers they care about. The single most reusable structural idea. |
| 4 | **Support-function + GJK/EPA convex core** (one code path collides box/sphere/capsule/hull *and* casts rays *and* sweeps shapes) | `Shape/ConvexShape.h:68-102`, `Geometry/GJKClosestPoint.h`, `Geometry/EPAPenetrationDepth.h` | Highest-leverage algorithm in the codebase. Grows us past AABB-only without N² special cases. The "convex radius" trick makes spheres/capsules fall out for free. |
| 5 | **Active-edge / internal-edge removal** (ghost-collision fix on triangulated floors) | `Collision/ActiveEdges.h`, `InternalEdgeRemovingCollector.h` | **Non-negotiable for parkour.** Without it a character snags on internal mesh seams while sliding. |
| 6 | **Fixed-FP determinism toolkit** (RAII FP control word · flush denormals · own `Sin/Cos` · no FMA contraction) | `Core/FPControlWord.h`, `Math/Trigonometry.h:9` | ~100 LOC, the price of entry for lockstep co-op. Set once per worker thread. |
| 7 | **CharacterVirtual collide-and-slide** (query-only controller: move → gather contacts → plane constraints → slide-solve → validate-sweep; stairs = up/forward/down casts) | `Character/CharacterVirtual.cpp:1263,815,1611` | The blueprint for our `PhysicsKinematicMotor`. Deterministic, you own the timing, never perturbs the sim. |
| 8 | **Accumulated-impulse constraint atom** (`AxisConstraintPart`: 1-DOF, warm-started, clamped) — every joint & contact built from it | `Constraints/ConstraintPart/AxisConstraintPart.h` | *Only when we add dynamics.* The crown jewel: ~15 joint types compose from ~6 reusable 1-DOF parts. |
| 9 | **Warm-started contact cache** (two-buffer manifold cache, per-point accumulated λ keyed by body+subshape) | `Constraints/ContactConstraintManager.cpp:1131-1335` | *Only when we add dynamics.* The biggest stacking-stability / settling win. |

**The one-sentence architecture:** *A body is a stable handle into a fixed array; a step is a dependency-graph of jobs over per-frame bump-allocated scratch; collision is a two-level filter funnel feeding a support-function convex core; the solver is accumulated-impulse Gauss-Seidel over union-find islands; and determinism is bought with a fixed FP environment + stable sort-before-solve.*

---

## 1. The layering & dependency DAG

Jolt is a strict bottom-up stack. Nothing lower depends on anything higher. This is the shape to copy.

```
  ┌─────────────────────────────────────────────────────────────┐
  │  H. GAMEPLAY BODIES        Character · Ragdoll · Vehicle · SoftBody
  │     (layered via constraints + PhysicsStepListener hooks)     │
  ├─────────────────────────────────────────────────────────────┤
  │  B. THE STEP               PhysicsSystem::Update  →  job DAG   │
  │     IslandBuilder (union-find) · LargeIslandSplitter          │
  ├───────────────┬───────────────────────┬─────────────────────┤
  │ E. SOLVER     │ D. NARROWPHASE         │ C. BROADPHASE        │
  │  Constraints  │  Shape hierarchy       │  QuadTree (4-ary BVH)│
  │  ContactParts │  CollisionDispatch     │  2-level layers      │
  │  (impulse GS) │  GJK / EPA / manifold  │  ObjectLayer filter  │
  ├───────────────┴───────────────────────┴─────────────────────┤
  │  A. BODIES                 Body · MotionProperties · BodyID    │
  │     BodyManager (array + free-list) · Body locking            │
  ├─────────────────────────────────────────────────────────────┤
  │  G. MATH                   Vec3/Vec4/Mat44/Quat · SIMD abstraction
  ├─────────────────────────────────────────────────────────────┤
  │  F. CORE INFRA             JobSystem · TempAllocator · FP-det  │
  │     containers · intrusive Ref · Factory/RTTI (serialization) │
  └─────────────────────────────────────────────────────────────┘
```

**Reading the DAG:** the three middle columns (Broadphase / Narrowphase / Solver) are the classic collision-pipeline triad and are *mutually independent* — broadphase knows nothing about how two shapes collide, narrowphase knows nothing about how contacts are solved. That independence is what lets each be swapped or simplified alone. Our engine can (and does) ship the bottom three layers + broadphase + narrowphase-queries **without** the solver at all.

---

## 2. The step (the heartbeat)

`PhysicsSystem::Update(dt, collisionSteps, tempAllocator, jobSystem)` — `PhysicsSystem.cpp:176-681`.

Sub-stepping is explicit: `step_dt = dt / collisionSteps` (`:215`); each *collision step* runs its **own** narrowphase and its **own** integration. The whole thing is expressed as a **job dependency graph**, not a serial loop — there is exactly one OS-level wait (`WaitForJobs`, `:628`) per `Update()`.

**Ordered phases of one collision step** (canonical list at `PhysicsUpdateContext.h:124-147`; ∥ = parallel fan-out, → = serialization point):

```
 1. BroadPhasePrepare        build fresh BP tree in background
 2. StepListeners        ∥   user hooks (vehicles raycast wheels here)
 3. DetermineActiveConstraints ∥
 4. ApplyGravity        ∥   v += g·dt
 5. FindCollisions      ∥   broadphase pair-find + narrowphase manifolds
 6. UpdateBroadphaseFinalize →  atomically swap new tree in
 7. SetupVelocityConstraints ∥
 8. BuildIslandsFromConstraints →  union-find link
 9. FinalizeIslands      →  flatten + sort biggest-first
10. SolveVelocityConstraints ∥  warm-start once, then N velocity iters
11. IntegrateVelocity   ∥   symplectic Euler: x += v·dt
12. (Post) ResolveCCDContacts →  linear-cast for fast bodies
13. SolvePositionConstraints ∥  Baumgarte drift fixup + sleep test + BP bounds update
14. StartNextStep       →   kick step i+1
```

**The job-graph mechanism** (steal the *idea*, not the 20-node graph): a `Job` is a `function<void()>` + one atomic `mNumDependencies` (`JobSystem.h:294`). "B depends on A" = create B with count 1, and A's lambda calls `B.RemoveDependency()`; at zero, B is queued. Jobs spawn more jobs mid-flight (e.g. `FindCollisions` re-queues itself, `PhysicsSystem.cpp:866`), so it's a *dynamic* dataflow graph, not a static DAG. `JobSystemWithBarrier::Wait()` runs barrier jobs **on the calling thread** while blocked, so the main thread is never idle (`JobSystemWithBarrier.cpp:105-168`).

**Islands** (`IslandBuilder.{h,cpp}`) — a maximal set of active bodies connected by contacts/constraints. They exist for two reasons only: *parallel solve* (islands share no bodies → each solves on its own thread) and *per-island sleeping*. **They are not required for correctness.** Built by lock-free union-find that always links to the lowest body index (→ discovery-order-independent → deterministic). `LargeIslandSplitter` uses greedy graph-coloring (32 colors) so >1 thread can solve one big island; it's already behind a runtime flag with a non-split fallback.

> **For us:** at 1–2 threads, **skip islands entirely** — a single global constraint list solved serially is deterministic by construction. Keep island *sleeping* later as a cheap contact-graph heuristic if idle-CPU becomes a concern.

---

## 3. Subsystem maps

Condensed from the eight lane studies. Each entry: the idea, the key types with `file:line`, and the design rationale.

### A · World, Bodies & Locking — `Physics/Body/`

- **`Body`** (`Body.h:445-473`) — hot data first and `sizeof` pinned by `static_assert` to **128 B** (single precision): position/rotation/bounds (16-B aligned), then shape ptr, `mMotionProperties*`, then small scalars. **Static bodies have `mMotionProperties == nullptr`** — all velocity/mass state is simply absent for the common immovable case (huge for a mostly-static stealth level).
- **`MotionProperties`** (`MotionProperties.h:285-322`) — the "warm/dynamic" half, laid out *by cache line*: velocities + inverse inertia on line 1, forces/damping/island-index on line 2, rarely-touched sleep spheres on line 3. Co-allocated with its `Body` in a single `new` (`BodyWithMotionProperties`, `BodyManager.cpp:52,192`) — hot/cold split with no second heap object.
- **`BodyID`** (`BodyID.h`) — `uint32` = low 23 bits index + top 8 bits sequence/generation. `TryGetBody` compares the *full* ID, so a stale handle whose slot was reused fails the sequence check → `nullptr`. **Copy this verbatim.**
- **`BodyManager`** (`BodyManager.h:340-374`) — `Array<Body*>` reserved to max at init, **never reallocates**. Freed slots store an intrusive free-list link in place of the pointer, tagged in the low bit. A dense `mActiveBodies[]` + atomic count is published with `memory_order_release` so lock-free readers (raycasts) see only complete entries.
- **Locking model** — *no double-buffered world.* A **fixed pool of striped mutexes** (`MutexArray`); a body hashes to one, so N bodies share far fewer mutexes. `BodyLockRead/Write` take the per-body shared mutex, letting query threads read a body concurrently with the sim. `PhysicsLock` enforces a global lock *ordering* in debug to prevent deadlock. Internal `Update` code uses a `...NoLock` interface variant (it already holds everything) → zero locking cost on the hot path.
- **Sleep/activation** (`EActivation.h`, `BodyManager.cpp:489-524`) — sleep is measured per-body via 3 tracked points vs a velocity threshold over `mTimeBeforeSleep`. `ActivateBodies` **does not walk contacts to wake neighbors** — neighbors re-wake next step when the island builder rediscovers the contact. Deliberate: cascading wakes would need a graph traversal under lock and could spuriously wake whole stacks. Keeps activation O(1) and deterministic.

### B · Update pipeline & islands — covered in §2.

### C · Broadphase & filtering — `Physics/Collision/BroadPhase/`

- **The tree is a 4-ary AABB BVH** (misleadingly named `QuadTree`), not a spatial-subdivision quadtree. A `Node` (`QuadTree.h:97-139`) is 128 B holding 4 children as **SoA atomic arrays** (`mBoundsMinX[4]…`) so one SIMD op (`RayAABox4`) tests all 4 boxes. **Leaves are `BodyID`s directly** — no leaf objects. Chosen over binary-BVH/grid for: 4-wide SIMD per node, shallower trees, and no grid cell-size tuning.
- **Concurrency without locks, two mechanisms:** (1) *widen-only incremental updates* — a moving body only ever *enlarges* a box and marks the root path dirty; a concurrent reader sees a correct-or-zero-volume box, never a spuriously huge one, so a moving body is never missed. (2) *double-buffered rebuild* — a fresh tight tree is built in the background, unchanged subtrees re-grafted wholesale, then `mRootNodeIndex` atomically flipped; the old tree stays live for in-flight queries.
- **The two-level filter (steal this):**
  - **`BroadPhaseLayer`** (`uint8`, coarse) — each maps to exactly **one physical tree**. Typical set = `{MOVING, NON_MOVING}`. `ObjectVsBroadPhaseLayerFilter` prunes **entire trees** before any walk — this is why the static tree never rebuilds and a moving-only query skips it whole.
  - **`ObjectLayer`** (`uint16`, fine, per body) — per-object rejection at the leaf. Two ship-ready encodings: a **triangular bit-table** (1 bit per unordered pair) or a **Bullet-style group/mask** (`collide iff (g1&m2)&&(g2&m1)`).
- **Group / self-collision filter** (`CollisionGroup` + `GroupFilterTable`) — the narrowest broadphase filter, ANDed after the layer filter. A ragdoll disables adjacent-bone self-collision via same-group + sub-group bit-table, while world bodies use `cInvalidGroup` (always collide).
- **`BroadPhaseBruteForce`** exists as a reference implementation of the same interface — the minimal starting point before committing to the tree. The active iggy3d runtime currently performs query-driven collider scans and has no dynamic-body pair-generation stage; `AabbGridIndex` is used only by Creative picking and placement clearance.

### D · Shapes, narrowphase & geometry — `Physics/Collision/Shape/` + `Geometry/`

- **Shape hierarchy = three orthogonal mechanisms so leaves stay dumb & fast:**
  - **Convex leaves** (`ConvexShape`) know nothing about hierarchy; they expose exactly one thing — a **`Support(direction)`** function (`ConvexShape.h:68-102`).
  - **`CompoundShape`** holds an array of `{ RefConst<Shape>, compressed pos/rot }` children.
  - **`DecoratedShape`** wraps one inner shape → `Scaled` / `RotatedTranslated` / `OffsetCenterOfMass`. Transform adapters, not new geometry.
- **`SubShapeID`** (`SubShapeID.h`) — a single `uint32` encoding the *path* root→leaf (e.g. which mesh triangle). Each level pushes just enough bits; consumers pop them. Exists so a contact is addressable across frames (look up material / per-triangle userData). Invalidates when the shape structure changes.
- **Dispatch table** (`CollisionDispatch.h`) — two `NumSubShapeTypes²` function-pointer tables (`sCollideShape`, `sCastShape`), indexed by `(subtypeA, subtypeB)`. Shapes self-register their cells at startup. **Decorated/scaled shapes peel and re-dispatch** (one extra table bounce, not a combinatorial explosion); **asymmetric pairs handled once and reversed** via a `ReversedCollector`.
- **The convex core (steal this):** `Support` returns the farthest point along a direction; `Geometry/ConvexSupport.h` composes supports generically (`MinkowskiDifference`, `AddConvexRadius`, `TriangleConvexSupport`). GJK finds distance/closest-point for the common *separated/shallow* case; **EPA escalates only when deeply penetrating** (`ConvexShape.cpp:45-164`). The **convex-radius trick** means a sphere is a *point* support with radius = r, a capsule is a *segment* support — rounded shapes need no special case. The *same* core powers raycasts (`GJKClosestPoint::CastRay`) and shape sweeps (`EPAPenetrationDepth::CastShape`) — i.e. our character motor and LOS both ride it.
- **Mesh path & active edges:** a mesh is a BVH of triangle blocks; collision walks it and feeds each triangle as a `TriangleConvexSupport` to the *same* GJK/EPA. **Active edges** (`ActiveEdges.h`) mark an edge "active" only if genuinely convex; `FixNormal` replaces a spurious internal-edge normal with the face normal. `CharacterVirtual` opts into `InternalEdgeRemovingCollector` — this is the fix for characters snagging on floor seams.
- **Manifold generation** (`ManifoldBetweenTwoFaces.cpp`) — GJK/EPA give one deepest point; to get a stable patch, clip face-2 against face-1 and **prune to ≤4 points** by a (distance-to-COM × penetration) heuristic. 4 points is enough for a stable solver.

### E · Constraints & the contact solver — `Physics/Constraints/`

- **Algorithm:** sequential-impulse / projected Gauss-Seidel. Per step: warm-start once, then `GetNumVelocitySteps()` (default **10**) velocity iterations, break early when no impulse applied; then a *separate* `GetNumPositionSteps()` (default **2**) Baumgarte position pass (factor 0.2). Position pass uses the "integrate-then-discard" trick (Catto GDC'07): fixes drift without injecting momentum.
- **The atom — `AxisConstraintPart`** (`ConstraintPart/AxisConstraintPart.h`): 1 linear DOF, stores an **accumulated impulse** `mTotalLambda`. `SolveVelocityConstraint` computes `λ = effectiveMass · (Jv − bias)`, **clamps the accumulated total** to `[min,max]`, applies only the delta. That clamp is how limits, friction cones, and one-sided contacts are all expressed. `WarmStart` re-applies last frame's λ scaled by `dt_new/dt_old`.
- **Composition (the crown jewel):** ~15 joint types are assembled from ~6 reusable 1-to-3-DOF parts (`Point`, `Angle`, `HingeRotation`, `DualAxis`, `RotationEuler`, `SwingTwist`). A joint holds parts as members and forwards the four solver calls (`Setup`/`WarmStart`/`SolveVelocity`/`SolvePosition`) to each. Add a DOF = add a part member. E.g. **Hinge** = Point + HingeRotation + Angle(limit) + Angle(motor).
- **Contacts as constraints** (`ContactConstraintManager`): `ContactConstraintPart` is `AxisConstraintPart` templated on the `(motionTypeA, motionTypeB)` pair (empty-base optimization strips unused inertia). One `ContactConstraint` per body+subshape pair holds ≤4 non-penetration points + 2 tangential + 1 angular friction part. Friction is Coulomb, box-clamped to `μ·Σλ_normal` from the previous iteration. **Restitution & speculative contacts fold into the velocity bias:** `bias = max(0, −penetration/dt)` resolves not-yet-touching contacts — cheap CCD-lite that stops a fast parkour character tunneling.
- **Warm-start cache:** two `ManifoldCache` buffers swap each step; lock-free hash maps key `SubShapeIDPair → CachedManifold`, storing local-space points + accumulated λ. On rebuild, each new point copies λ from the nearest old point within a threshold. This is the single biggest stability/settling win in the solver.

### F · Core infrastructure — `Core/`

- **JobSystem** — see §2. The abstract interface (`JobSystem.h:69`) is explicitly meant to be **swapped for the host engine's scheduler**; `JobSystemThreadPool` is "an example implementation." Ship `JobSystemSingleThreaded` first (same interface, deterministic by construction), add threads behind the abstraction later.
- **`TempAllocator`** (`TempAllocator.h:34-131`) — bump/stack allocator over one up-front malloc. `Allocate` = `mTop += alignUp(size)`; `Free` = `mTop -= size` with a **LIFO assert**. No headers, no free-lists, no locks. Works because per-step data (islands, manifolds, pair arrays, solver batches) frees in exact reverse order — guaranteed by job dependencies, not mutexes. `WithMallocFallback` degrades gracefully when the arena is exhausted.
- **Determinism toolkit** — three layers, see §4.
- **Containers & refcounting** — `Array` is a `std::vector` clone that skips zero-init and takes a Jolt allocator. **`Ref`/`RefTarget`** (`Reference.h:33-157`) are *intrusive*: the atomic count lives in the object → one allocation not two (vs `shared_ptr`'s control block), and the count travels into serialization. `FixedSizeFreeList` is a lock-free ABA-tagged paged pool. `Factory`/`RTTI` + `ObjectStream` provide reflection-driven asset/settings serialization — **orthogonal** to the per-frame `StateRecorder` (§4).

### G · Math & SIMD — `Math/`

- **One type, N backends:** `Vec4` wraps one 128-bit register whose concrete type is macro-chosen — `__m128` (SSE) / `float32x4_t` (NEON) / scalar `float[4]` fallback — union-aliased with a `float[4]` so lanes stay addressable. Each op has per-backend bodies under `#if defined(JPH_USE_SSE) … #elif … #else`. Beats raw intrinsics (callers stay ISA-agnostic) and beats a generic template lib (each op hand-tuned, not left to autovectorization).
- **Vec3 stored as Vec4:** a 3-vector occupies 4 lanes; the convention is **W == Z** (not undefined), so SIMD cross/dot are free and there's no divide-by-zero under FP exceptions. Ops that shuffle must re-fix W. 16-byte alignment (8 on 32-bit ARM).
- **Single vs double (`Real.h`):** `JPH_DOUBLE_PRECISION` makes only **world-space positions** double — `DMat44` is "floats with a double translation column" — so rotation/velocity/contact math stays single. Lets huge worlds stay precise without paying double cost everywhere. *We don't need this — a stylized level fits in single-precision near one origin.*
- **Determinism in math:** ships its **own polynomial `Sin/Cos/…`** because `std::sin` is non-deterministic across libms; FMA contraction is opt-out under `JPH_CROSS_PLATFORM_DETERMINISTIC`; fixed reassociation in `ReduceSum`; true `sqrt` never `rsqrt` (fast-math `_mm_rsqrt_ps` produces NaNs).

### H · Character controller & advanced bodies — `Physics/Character/`

- **`CharacterVirtual` — the collide-and-slide controller (our blueprint).** "Doesn't use a rigid body — moves doing collision checks only." Not in the broadphase; you set velocity, apply your own gravity, call `Update()`; it queries the world and resolves its own motion. The engine is `MoveShape()` (`CharacterVirtual.cpp:1263`), a loop up to `mMaxCollisionIterations` (5):
  1. **GetContactsAtPosition** — `CollideShape` on the capsule dilated by predictive distance + padding, with `CollideOnlyWithActive` edges (ghost-edge rejection).
  2. **RemoveConflictingContacts** — drop opposing normals that would trap the character.
  3. **DetermineConstraints** — each contact → a `Plane`; penetrating contacts get a push-out velocity; **too-steep surfaces get a second vertical constraint so you can't creep up walls** (`:720-739`).
  4. **SolveConstraints** — compute time-of-impact per plane, advance to nearest, cancel velocity into its normal (`v −= v·n * n`); on a second violated plane, slide along the crease (`n1 × n2`).
  5. **ValidateMovement** — sweep-cast the resolved displacement to confirm the path is clear.
  - **Stair-stepping is *not* in the core loop** — it's `WalkStairs()` (`:1611`): sweep **up** `mStepUp`, **forward** (a full `MoveShape`), **down**, accept only if the landing normal is walkable. `StickToFloor()` snaps you down so you don't launch off small steps. `ExtendedUpdate()` is the reference glue.
  - Gains physical presence via an optional **inner kinematic body** (so raycasts/other bodies see it) and pushes dynamic props via impulses clamped to `mMaxStrength·dt`.
- **`Character` (rigid-body) vs `CharacterVirtual` (kinematic):** `Character` creates a real dynamic body in the solver — you inherit solver jitter, can't cheaply do custom stair/slide logic, and only learn your final position post-step. **A parkour-stealth game wants `CharacterVirtual`**: deterministic, you own *when* in the frame the player moves (responsiveness + netcode), explicit ledge/stair/slope logic, never perturbs the sim.
- **Ragdoll** (`Ragdoll/Ragdoll.h`) = a `Skeleton` + one body-per-bone joined by constraints. `DisableParentChildCollisions()` auto-builds the group filter. Drive modes for takedowns: `SetPose` (instant) · `DriveToPoseUsingKinematics` · `DriveToPoseUsingMotors` (constraint motors chase animation — the animation↔physics blend for a limp takedown / hit reaction).
- **Layering pattern** (skim only): `VehicleConstraint : Constraint, PhysicsStepListener` — a constraint plus an `OnStep()` hook that raycasts wheels and applies forces each step. This is the general recipe for "a system layered on the rigid-body core." SoftBody instead subclasses `MotionProperties` as a first-class `EBodyType::SoftBody` solved in a dedicated phase.

---

## 4. The determinism story (our co-op netcode enabler)

Our game vision is asymmetric co-op with the notebook as the intel packet — replicated by *replaying inputs*, which demands bit-identical simulation. Jolt buys this in three layers, all cheap:

1. **Fixed FP environment** (`Core/FPControlWord.h`, `FPFlushDenormals.h`, `FPException.h`) — RAII sets the MXCSR/FPCR control word per thread (restored on scope exit), forces flush-to-zero on denormals, and traps div-by-zero/invalid to catch NaN drift early. Every worker thread instantiates these at entry so all threads share one FP mode. **~100 LOC total.**
2. **Deterministic ordering before solve** — parallel `FindCollisions` produces contacts in nondeterministic order, so before solving each island, constraints sort by `(priority, stable insertion index)` and contacts by a stable key (`ConstraintManager.cpp:103-116`). Union-find always links to the lowest index. Cross-island order is irrelevant (independent sets commute). **This is the whole trick — a stable sort, not lockstep exotica.**
3. **State save/restore** (`Physics/StateRecorder.h`) — a combined `StreamIn`+`StreamOut` that snapshots only sim-mutated state (bodies/contacts/constraints). Validation mode compares bytes on read to *prove* a step reproduced identically. This is our rollback/resim buffer. `DeterminismLog` (opt-in, streams raw float bit-patterns to a file) lets us `diff` two runs to the first divergent bit when hunting a desync.

Plus the math-layer rules from §G (own trig, no FMA contraction, true sqrt).

---

## 5. Where iggy3d is today vs Jolt

Grounding the study against `src/runtime/physics/` (~2,850 LOC). We already run a **kinematic, AABB, query-driven** engine — deliberately the "simplify to the max" end of every lane's advice. The gap map:

| Concern | iggy3d today | Jolt equivalent | Seam status |
|---------|--------------|-----------------|-------------|
| Body handle | `PhysicsBodyId` (`PhysicsTypes.hpp:10`) | `BodyID` (index+generation) | **Add generation counter** (idea #1) — cheap, unlocks safe reuse |
| Body store | motion kinds + weight class enums | `BodyManager` array + free-list | Fine for now; adopt free-list when bodies churn |
| Shapes | **AABB only** (`PhysicsShapeStore`, `halfExtentsMeters`) | `BoxShape` + full convex hierarchy | **Biggest growth seam** → adopt support-fn convex core (idea #4) when you need capsules/slopes |
| Broadphase | No runtime pair-generation stage; Creative uses `AabbGridIndex` for picking/clearance | `BroadPhaseBruteForce` → `QuadTree` | Add layers/indexing only when dynamic body pair generation becomes real |
| Queries | raycast + AABB overlap (`PhysicsCollisionQueries`, `normalFromColliderToRay`) | `NarrowPhaseQuery::CastRay` | **This is our LOS surface — already live.** Rides the convex core once shapes grow |
| Character | `PhysicsKinematicMotor` (`desiredDisplacement → hits`) | `CharacterVirtual::MoveShape` | **Nascent collide-and-slide.** Harden toward idea #7: slide-solve, steep-slope 2nd constraint, WalkStairs |
| Creative→physics | `PhysicsSpatialSurfaceColliderBake` | (external — game builds bodies) | Our own seam; keep it |
| Instrumentation | `PhysicsFrameStats` | `NarrowPhaseStats`, `Profiler` | Matches our receipt doctrine — keep + extend |
| Solver / constraints | **none** | Constraints + ContactConstraintManager | **Absent by design.** Only needed if we add *dynamic* bodies (thrown props, ragdolls) |
| Islands / threading | **none** | IslandBuilder + job graph | **Skip until profiling demands it** |
| Determinism | (unaudited) | FP control word + stable sort | **Adopt idea #6 early** — it's ~100 LOC and retrofitting later is painful |

**The honest read:** we are a *collision-query engine*, not a *dynamics engine* — and for a parkour-stealth game that may be 80% of what we ever need. The character moves kinematically; LOS is a raycast; stealth AI reads queries. Dynamics (a real solver) is an *optional future lane*, not a prerequisite.

---

## 6. A leverage-ranked build order for our own engine

Two tracks. **Track K** hardens what we have (kinematic/query) — this is the parkour-stealth critical path. **Track D** (dynamics) is optional and only starts if we want physically-simulated props/ragdolls. Do K1–K3 before touching D.

**Track K — harden the kinematic/query engine (do this)**
1. **K1 · Determinism floor** — FP control word + flush-denormals + own `Sin/Cos` + stable-sort any query output. ~100 LOC now saves a desync hunt later. *(idea #6)*
2. **K2 · Handle hardening** — add the generation counter to `PhysicsBodyId`; free-list the body store. *(idea #1)*
3. **K3 · Two-level broadphase filter** — split bodies into `{STATIC, MOVING}` broadphase layers (static geometry in a never-rebuilt structure) + a small object-layer bit-table. Do this *before* replacing brute-force with a tree — filtering beats a fancier structure. *(idea #3)*
4. **K4 · Per-frame bump allocator** — route all per-step query scratch through it. *(idea #2)*
5. **K5 · Convex shapes via support functions** — introduce `Support(dir)` + GJK for capsule/sphere/box, with the convex-radius trick. Unlocks a capsule character + swept queries without AABB's limitations. *(idea #4)*
6. **K6 · Active-edge handling** — once we collide against meshes, add internal-edge removal so the character doesn't snag on floor seams. *(idea #5)*
7. **K7 · CharacterVirtual-grade motor** — grow `PhysicsKinematicMotor` into the full collide-and-slide loop: plane constraints, crease-sliding, the steep-slope second constraint, `WalkStairs` up/forward/down, `StickToFloor`. *(idea #7)*
8. **K8 · Broadphase tree** — only after runtime pair generation exists and profiling shows it matters, replace brute-force pairing with a 4-ary AABB BVH (SoA nodes, widen-only updates).

**Track D — dynamics (optional, only for simulated props/ragdolls)**
9. **D1 · `AxisConstraintPart` atom** + a flat contact solver (no islands): warm-start once, ~8 velocity iters, 2 position iters. *(idea #8)*
10. **D2 · Warm-started contact cache** keyed by body+subshape pair. *(idea #9)*
11. **D3 · A few joints** (Point/Fixed/Hinge) composed from parts — enough for ragdolls.
12. **D4 · Islands** — only if a single serial solve becomes a frame-time problem.

**Explicitly skip** (all lanes agreed): SoftBody, Hair/GPU-compute, Vehicle, heightfields, double-precision, CCD as a solver phase (the character does its own sweeps), the large-island splitter, and the full reflection/ObjectStream serializer (keep `StateRecorder` separately).

---

## 7. Consolidated Steal / Skip / Simplify ledger

**STEAL (high value, low cost):**
- `BodyID` index+generation handle over an array + intrusive free-list.
- `TempAllocator` bump/stack allocator for all per-frame transient data.
- Two-level broadphase filter (coarse tree-buckets + fine bit-table pairs).
- Support-function + GJK/EPA convex core (one path for collide/raycast/sweep) with the convex-radius trick.
- Active-edge / internal-edge removal for characters on meshes.
- FP-control-word + own-trig + stable-sort-before-solve determinism recipe.
- CharacterVirtual's collide-and-slide structure incl. the steep-slope second constraint and up/forward/down stairs.
- Accumulated-impulse `AxisConstraintPart` + two-buffer warm-start cache *(dynamics only)*.

**SKIP (overkill for a stylized parkour-stealth game):**
- Double precision / large-world `Real` toggle.
- SoftBody, Hair, Vehicle, heightfields, tapered shapes, buoyancy.
- CCD as a solver phase; the LargeIslandSplitter; the lock-free hash maps / ABA free-lists.
- Full `ObjectStream`/`RTTI`/`Factory` reflection serialization.
- SIMD backends beyond one target + scalar fallback (drop the AVX512→SSE cascade & the 60-specialization NEON shuffle table).

**SIMPLIFY (take the idea, not the machinery):**
- Collapse the ~20-node job DAG into a straight-line loop with one `parallel_for` per phase — or ship single-threaded first behind the `JobSystem` interface.
- Drop islands entirely at 1–2 threads (serial solve is deterministic by construction).
- Replace the `NumSubShapeTypes²` dispatch table with a small `switch` on `(typeA,typeB)` + one swap helper (~5 leaf types).
- Make `SubShapeID` a plain `{shape, triangle}` pair if compounds never nest.
- Support only uniform scale; bake decorator transforms into one matrix per body.
- ~2 broadphase layers + an N≤16 triangular filter table; full rebuild each fixed step beats incremental machinery at low body counts.

---

## 8. Read-first index — the ~20 files to open in Jolt

Base: `~/game-references/JoltPhysics-master/Jolt/`. If you read only the **bold** ones, you get 80% of the architecture.

**Core ideas (read first):**
- **`Core/TempAllocator.h:34-131`** — the bump allocator; smallest highest-value idea.
- **`Physics/Body/BodyID.h`** + `Body/BodyManager.h:340-374` — the handle scheme + storage model.
- **`Physics/Body/Body.h:445-473`** — memory-layout / hot-cold split in one struct.
- **`Core/JobSystem.h:170-305`** + `Core/JobSystem.inl` — the dependency-counter job model.

**The pipeline:**
- **`Physics/PhysicsSystem.cpp:176-681`** — the entire step as a job graph.
- `Physics/PhysicsUpdateContext.h:124-147` — canonical job order.
- `Physics/IslandBuilder.{h,cpp}` — union-find islands *(skip if going serial)*.

**Collision:**
- **`Physics/Collision/BroadPhase/QuadTree.{h,cpp}`** + `BroadPhaseLayer.h` + `ObjectLayer.h` — tree + the two-level filter.
- `Physics/Collision/BroadPhase/BroadPhaseBruteForce.{h,cpp}` — the minimal reference broadphase.
- **`Physics/Collision/CollisionDispatch.{h,cpp}`** — the dispatch model in ~100 lines.
- **`Physics/Collision/Shape/ConvexShape.h:68-102`** + `ConvexShape.cpp:45-164` — the `Support` fn + canonical GJK→EPA call.
- **`Geometry/ConvexSupport.h`** + `Geometry/GJKClosestPoint.h` + `Geometry/EPAPenetrationDepth.h` — the convex core.
- `Physics/Collision/CollideConvexVsTriangles.cpp` + **`Collision/ActiveEdges.h`** — mesh contacts + the ghost-collision fix.
- `Physics/Collision/ManifoldBetweenTwoFaces.cpp` — contact-patch generation.

**Solver *(dynamics track only)*:**
- **`Physics/Constraints/ConstraintPart/AxisConstraintPart.h`** — the accumulated-impulse atom.
- **`Physics/Constraints/ContactConstraintManager.cpp:1131-1335`** + `.h:265-430` — contacts→constraints + warm-start cache.
- `Physics/Constraints/HingeConstraint.cpp:240-359` — canonical part composition.

**Character + determinism:**
- **`Physics/Character/CharacterVirtual.cpp:1263,815,1611`** — `MoveShape` / `SolveConstraints` / `WalkStairs`.
- `Physics/Character/CharacterBase.h` — ground-state / slope model.
- **`Core/FPControlWord.h`** + `Math/Trigonometry.h` + `Physics/StateRecorder.h` — the determinism toolkit.
- `Math/Vec4.h:13-25,315-319` + `Math/Vec3.h` — the one-type/N-backend SIMD pattern.

---
*End v0.1. Corrections welcome — bump the version and note the finding. Next step if we act on this: K1 (determinism floor) + K2 (handle hardening) are the cheapest first cuts, both landing entirely inside `src/runtime/physics/` with no new dependencies.*

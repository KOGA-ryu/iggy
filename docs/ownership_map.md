<!-- Generated 2026-07-07 by a 5-agent ownership-mapping pass (verified against the tree). Update when a lane boundary or a cross-lane contract actually moves, not per commit. -->

# iggy3d Ownership Map

**Read this before scoping a builder card.** Ownership is by *convention* on one shared Mac tree — every commit authors "Ace," so the real signal is the commit-message prefix (`codex:` vs `claude:`) plus the subtree. Two agents: **Codex** owns creative + render; **claude/fleet** owns runtime ai + movement. Everything else is shared.

> **Headline ambiguity:** the taxonomy names 4 lanes but there are **5**. `src/runtime/**` (23 dirs) is a full simulation lane with no bucket — it is neither core nor render nor creative nor product-app. Cards land mis-scoped because they get force-fit into "product-app" or "core." **Fix: name `runtime/simulation` as its own lane** (details below).

---

## Lanes

### 1. core — shared foundation (co-owned, shared-write)
- **Owned paths:** `src/core/{math,spatial,grid,geom,hash,ids,result,diagnostics}/*`
- **Authoritative for:** pure stateless algorithms + POD value types — vector/matrix/transform/frustum/ray/OBB math, AABB grid index, greedy-mesh/footprint/reachability, parametric stair geom, stable hash, shared vocabulary (EntityId, Result<>, Diagnostic). **No runtime state, no jobs/time/GPU.**
- **Public surface:** `math/Vec3.hpp` (**63 includes — universal, verified**), `ids/EntityId.hpp`, `math/Aabb3.hpp`, `math/Transform3.hpp`, `result/Result.hpp`, `diagnostics/Diagnostic.hpp`.
- **Agent owner:** **SHARED-WRITE — no single owner.** Codex authored math/spatial/grid-footprint (render/creative pressure); claude authored Vec3 extensions, Snap, StairMesh, Reachability (movement/ai pressure). Courtesy rule: neither agent unilaterally reshapes a public header the other consumes (Vec3/Aabb3/EntityId/Transform3 especially).

### 2. render — Codex
- **Owned paths:** `src/render/**`; `src/app/iggy3d/view/**`; `src/app/iggy3d/window/{FramePresenter,Loop,RendererLifecycle,ProductVulkan*}.*`; `creative/render/WireframeDebugLines.*`; `creative/bridge/{WindowCoordinateSpace,UiWindowFrame}.*`.
- **Authoritative for:** Vulkan backend (device/swapchain/pipelines/sync/alloc/shaders/headless PNG capture), frame submit + present loop, world→screen projection for entities/overlays/viewport, debug HUD + wireframe overlays, null/mesh renderers.
- **Public surface:** `render/FrameInput.hpp` (**the one frame-submission contract every lane fills**), `render/RendererApi.hpp` + `RenderBackend.hpp`, `view/RenderBridge.hpp`, `view/ViewportFraming.hpp`.
- **Agent owner:** **Codex.** One cross-agent caveat: `render/vulkan/ProjectileOverlayProjection` consumes projectile data originating in the claude sim/ai lane — **FrameInput is the handoff boundary**; claude fills it upstream, Codex render consumes it, never reaches into sim state.

### 3. creative — Codex
- **Owned paths:** `src/app/iggy3d/creative/**`; `apps/iggy3d_creative/**`; `tests/unit/creative_*`; `docs/creative_mode/**`.
- **Authoritative for:** `CreativeDocument` as the authoritative editable world model; editing tools (Select/Move/Measure/Navigate/RoomShell + snap/ghost); mutation apply + undo snapshots; viewport pick / slab projection / snap math (`creative/spatial/*` — verified lives here, not render); RoomBake adapter (document → RoomAsset); creative UI draw list; standalone app + save-section serialization.
- **Public surface:** `creative/document/Document.hpp`, `Object.hpp` + `ObjectDescriptor.hpp` (incl. PatrolRoute), `DocumentMutation.hpp`, `adapters/RoomBake.hpp` (RoomAsset — **cross-lane out**), `Facade.hpp`, `CreativeAppState.hpp`.
- **Agent owner:** **Codex** (recent creative commits ~12 `codex:` vs 3 `claude:`).

### 4. product-app — SHARED (skewed, highest-contention)
- **Owned paths:** `src/app/iggy3d/*.{cpp,hpp}` (AppKernel, AppShell, Operations, ReceiptBuilder, ProductAppWindowState); `src/app/iggy3d/{receipt,save,menu,automation,gameplay,world,ascii_room,room_editor,map_maker,room,debug,input,window,view,ui,product}/**`; `src/app/{frontend,input,platform}/**`; `apps/iggy3d/main.cpp`.
- **Authoritative for:** process lifecycle (option parse → AppKernel run → window/renderer loop), frontend/menu/pause/routing, save/session flow, **receipts** (text-field state surface tests assert against), automation harness, gameplay glue over runtime, world authoring + npc-profile assignment.
- **Public surface:** `iggy3d::runProductApp(argc,argv)` (process entrypoint), `iggy3d::AppKernel` (app-lifetime coordinator), `ProductAppWindowState` (the 421/643-field god-struct being decoupled), `RenderReceipt` + `appendReceiptField`. **Verified: nothing under runtime/render/core includes `app/iggy3d/*` — product-app is a pure top-of-stack consumer.**
- **Agent owner:** **SHARED but split by file.** claude commits the runtime-facing glue (`gameplay/*`, `world/NpcProfileAssignment+PackageSessionSeed`, `debug/Npc*Hud`, `CreativeReasoningActivation`, `PatrolRouteWaypoints`, ai/movement receipt fields). Codex commits the creative-facing glue (everything reaching into `creative/*`). **The lifecycle spine (AppKernel/AppShell/Loop/Operations/ReceiptBuilder/save) is jointly touched — coordinate every commit there.**

### 5. runtime/simulation — SHARED, split by subtree ⚠️ THE MISSING LANE
- **Owned paths:** `src/runtime/{ai,movement,ability,physics,collision,player,combat,targeting,projectile,interaction,objective,inventory,camera,object,world,command,replay,session,save,clock,diagnostics,debug,multiplayer}/*` (**23 dirs, verified**).
- **Authoritative for:** deterministic per-frame simulation tick (session coordinator drives movement/ability/combat/objective/interaction/command), NPC behavior/awareness (perception, alert grading, patrol, guard decision, reasoning graph, recon intel), movement + kinematics + abilities, physics + collision, combat systems, command admission + replay/state-hash determinism, runtime save envelope.
- **Public surface:** `session/Session.hpp` (**the coordinator seam product-app drives**), `ai/AiState.hpp` (embedded in SaveEnvelope + SessionState), `ai/ReconIntel.hpp` (the intel packet — **reserved contract, consumer not built yet**), `movement/MovementSystem.hpp`, `save/SaveEnvelope.hpp` (**hard-includes `ai/AiState.hpp` — verified line 11**), `command/Command.hpp` + `replay/CommandLog.hpp`.
- **Agent owner:** **SHARED, split by subtree.** claude/fleet owns `ai/*` + `movement/*` + `ability/*` (recent ai commits all `claude:`). `session/`, `save/`, `command/replay` are **shared spine — the core-spine GATE applies** (save/session cross-thread boundaries). `physics/*` + `collision/*` are **mixed** (Codex routes math through core, claude does Vec3/consume sweeps) → shared. Remaining game systems (combat/targeting/projectile/interaction/objective/inventory/camera/object/world) are unclaimed/shared.

---

## Seam Catalogue — ranked by ambiguity pain

| # | Seam | Ownership RULING (one line) |
|---|------|------------------------------|
| **1** | **`src/runtime/**` vs the 4-lane taxonomy** | **runtime/simulation IS a distinct 5th lane — name it; do NOT fold into core or product-app.** Split by subtree: claude owns ai/movement/ability, session/save/command are shared spine (GATE), rest shared. |
| **2** | `AppKernel.{hpp,cpp}` | **product-app spine, JOINTLY owned** — it directly owns creative::CreativeAppState + runtime Session + god-struct. Keep it a thin coordinator (member add/remove only, no domain logic) so edits stay additive. |
| **3** | `CreativeReasoningActivation.{hpp,cpp}` | **claude owns it** — payload is a ReasoningGraph for the stealth guard (ai semantics; `.cpp` includes `ai/ReasoningGraph.hpp`+`Session.hpp`, verified). Codex owns the RoomAsset shape it reads. The game-master-plan #1 unblocker. |
| **4** | `receipt/**` (Creative*Fields vs Gameplay*/Physics*/ai fields) | **Own per-FILE, not per-dir** — `Creative*Fields → Codex`; `Gameplay*/Physics*/ai → claude`; `ReceiptFields.hpp`/TailFields shared. Receipts are the test contract; drift here fails the suite for BOTH agents. |
| **5** | `adapters/RoomBake.hpp` RoomAsset (creative → ai/gameplay) | **Codex owns the bake code** (walks CreativeDocument); claude owns the consumer contract. **Freeze RoomAsset as a versioned shared struct** — a lane-boundary interface, not editable by either side without a cross-lane note. Named live break: affordance/patrol wire-strings. |
| **6** | `runtime/ai/ReconIntel.hpp` (creative→AI→notebook) | **claude owns it outright** (the AI packet). Codex owns the affordance vocabulary it reads; product-app owns the future notebook consumer. **NOTE: consumer does not exist yet — verified self-contained; treat as a RESERVED published contract, not a live seam.** |
| **7** | `runtime/save/SaveEnvelope.hpp` ↔ `ai/AiState.hpp` | **Save FORMAT is shared spine (core-spine GATE); AiState payload fields are claude-owned.** Any AiState schema change is a claude commit but must pass save-lane review. |
| **8** | product-app files that #include creative/* (`window/{Loop,InputFrame}`, `menu/*`, `save/Flow`, `automation/AutomationDispatch`) | **Codex owns the creative-facing hunks** (move in lockstep with creative API); split by hunk per the awk+git-apply recipe. claude must not stage these creative include blocks. |
| **9** | `core/grid/*` (GreedyMesh, Footprint, Reachability) + `core/geom/StairMesh` | **Stay in core as shared pure algorithms** — CO-OWNED at the creative-bake ⨯ movement/ai seam. Whichever lane drives a signature change proposes it, the other reviews. Neither may privatize into its own tree. |
| **10** | `creative/bridge/*` (mixed render-frames vs input/command-frames) | **Split by suffix:** `*WindowFrame`/`*CoordinateSpace`/`WireframeFrame` → render; `*InputFrame`/`*CommandFrame`/`ViewportPickFrame` → creative. All Codex-committed — a code-lane label, not an agent handoff. |
| **11** | `gameplay/**` + `debug/Npc*Hud` | **claude, unambiguously** — product-side wiring over runtime/movement + ai. Flag: this is claude-EXCLUSIVE product glue, the exception to "product-app is shared." |
| **12** | `PatrolRouteWaypoints.{cpp,hpp}` | **claude owns it** (extractor feeding NpcPatrolSystem) despite its product-app location. Creative must not change the PatrolRoute descriptor shape without notifying AI. |

**Four contracts to FREEZE as published structs** (edited only with a cross-lane note): `FrameInput` (render), `RoomAsset` (creative→ai), `ReconIntel` (ai→notebook, reserved), `SaveEnvelope`+`AiState` (save spine).

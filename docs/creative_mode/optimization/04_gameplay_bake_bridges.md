# 04 — Gameplay Bake Bridges (Collision, Nav, Trigger, Light, Audio, Event)

> ↔ research `04_navigation_collision_triggers.md` + the lighting/audio halves
> of research `03`. Lanes: **[C] emit + [P] consume**, split per bridge below;
> every bridge is planner-brokered because it crosses the lane boundary.
>
> **The governing ruling is §7 R4:** creative mutations NEVER touch live
> gameplay. A bridge is a named, explicit EXPORT consumed on world activation —
> bake, then play. And **LAW-17**: gameplay vocabulary travels as append-only
> anchor-kind wire strings; unknown kinds are ignore-and-continue everywhere.
>
> This doc is contract-grade on purpose. It exists so the earlier packets
> (01/02/05) build toward real consumers instead of hypothetical ones — and so
> nobody invents a second vocabulary. It becomes cuttable only after
> **W3 (creation) + W5/W6 (persistence/world service)** land.

## The two transport mechanisms (both already named — reuse, never invent)

1. **The RoomEd bake** — `creative/adapters/RoomEd` (reserved 0-byte socket;
   foundation plan L5 :248-250): creative document → `RoomAsset` +
   `SpatialSurfaceSet` "via the occupancy index". Geometry-shaped consequence:
   collision, walkable surfaces, static meshes.
2. **The anchor wire strings** — `docs/affordance_vocabulary_v0_1.md` (A7
   contract): creative GameplayMarker kinds → `RoomAnchorAsset.kind` strings →
   consumed by `entityFromAnchor` (`world/PackageSessionSeed.cpp:156-175`:
   npc|monster→Npc, pickup|key|treasure→Pickup, door|secret_door→Door,
   exit→objective, else→inert Inspect marker) and by the AI reasoning graph
   (`markerToReasoningNode`, runtime/ai). Marker-shaped consequence: spawns,
   nav semantics, objectives, future triggers/audio/events.
   **Creative emission of these strings is a LATER Codex order by contract**
   (affordance_vocabulary_v0_1.md:70-72) — this doc plans for it; it does not
   pull it forward.

Every bridge below routes through one of these two. A bridge that needs a
third mechanism is a planning failure — escalate to the planner, do not build.

## Bridge table

| Bridge | Transport | Emitter [C] | Consumer [P] (exists today?) | Earliest slot |
| --- | --- | --- | --- | --- |
| Collision / walkable | RoomEd bake → `SpatialSurfaceSet` (roles Walkable/Blocker/ProjectileBlocker, `RoomAsset.hpp:54-67`) | structural kinds via occupancy index | YES — session movement + stealth LOS consume surfaces | post-W5/W6 |
| Static render meshes | RoomEd bake → `RoomAsset.staticMeshes` | box geometry first | YES — room renderer | with collision (same bake, one pass) |
| Spawns / markers | wire strings | GameplayMarker family (`ObjectDescriptor.cpp:211-221`) | YES — `entityFromAnchor` | after A7 emission order |
| Navigation semantics | wire strings | chokepoint/high_ground/etc. kinds | YES — reasoning graph builds itself from anchors (A7 landed AI-side) | after A7 emission order |
| Triggers / zones | wire strings (NEW kinds, append-only) | zone kinds (doc 01 Trigger domain) | PARTIAL — precedent: the `R` reset marker is live gameplay (Controller.cpp, test-pinned); a generic zone consumer does not exist | design packet first (below) |
| Lighting | none yet | light kinds | NO consumer (renderer has no dynamic light list for rooms) | blocked on renderer reality — SOCKET only |
| Audio | wire strings (emitter/zone kinds) | PARTIAL — the a1 sound-perception kernel (runtime/ai) consumes sound EVENTS (footsteps→hearing); no ambient/zone consumer | design packet first |
| Events | wire strings (NEW kinds) | NO generic event graph exists | SOCKET only |

The three SOCKET rows are stated so descriptors (doc 01) reserve their domains
now; they get design packets only when a consumer exists or is ordered. Do not
bake outputs nothing reads (the write-only receipt lesson from the review —
leverage lives in the READERS).

## Collision bridge spec (the first real one)

**Shape:** one pure function, [C]-side, unit-gated:

```cpp
// creative/adapters/RoomEd.hpp (socket comes alive)
struct CreativeRoomBakeResult {
    RoomAsset room;                       // staticMeshes + spatialSurfaces filled
    CreativeRoomBakeReceipt receipt;
};
CreativeRoomBakeResult bakeCreativeRoomAsset(const CreativeDocument&,
                                             const CreativeOccupancyIndex&,
                                             const CreativeRoomBakeRequest&);
```

Rules:

- Consumes descriptor `bakeDomains & Collision` objects only; per-kind shape
  policy is a descriptor-driven mapping (D6): box kinds → Blocker box surfaces,
  floor-like kinds → Walkable, wedge/ramp kinds → RECON-04b (match the ascii
  ramp bake so slopes behave identically regardless of authoring surface).
- Deterministic: stable ordering (object id), pure function of document
  content — same law as the reasoning graph build. Bake twice, byte-equal.
- `object.visible == false` policy is a DECISION, not an accident: first rule =
  hidden objects still bake collision (visibility is a render fact), receipted
  per object; revisit only with a gameplay ruling.
- Receipt: surfaces emitted per role, objects skipped per reason, source
  document revision. Round-trip pin: bake → count surfaces → matches golden
  fixture.
- Consumption [P]: the world service (W6) activates the baked RoomAsset through
  the SAME path packaged/ascii rooms use (`window.activeRoom` +
  `activeRoomCollision` rebuild). No new consumption path — the bake output is
  deliberately indistinguishable from an ascii-compiled room downstream.

Golden fixture: a 3-object document (Floor 8×8, Wall 4-long, Crate 1³) → exact
expected surface list (roles, bounds, count = RECON-04a fills exact numbers
from the ascii pipeline's equivalents so both pipelines agree on conventions).

## Trigger + audio design-packet charters (not build orders yet)

- **Triggers:** extend the wire vocabulary (append-only) with zone kinds
  carrying bounds payload; runtime consumer = a zone table on the session,
  queried via the actor's position each move (the movement code's existing
  surface-query pattern), diffed against last-frame membership for
  enter/stay/exit. The `R` reset marker's live behavior is the consumption
  precedent to generalize — RECON-04c anchors it exactly.
- **Audio:** emitter kinds bake position+radius rows; the a1 hearing kernel's
  input shape is the consumer contract — RECON-04d returns its exact struct so
  the bake emits precisely that, no adapter glue.

Both packets start with the RECON, produce a one-page contract amendment to
`affordance_vocabulary` (version bump, planner-brokered), and only then a build
slice.

## Sequencing and forbidden moves

- Nothing here starts before W3+W5/W6. The collision bridge additionally wants
  doc 02 (the index is its iteration surface).
- [C] never edits `src/runtime/**` or `src/content/**`; [P] never edits
  `creative/**`. The bake function is [C]; the activation path is [P]; the
  planner sequences the two commits.
- No bridge adds a live-mutation path. If a slice finds itself wanting
  "creative edit updates the running session", it has left the contract —
  §7 R4. Bake, activate, play.

## Metrics

Per bridge: outputs emitted per kind/role, skipped per reason, bake time
(measured), bake determinism check (hash of output, twice), consumer-side:
surfaces/anchors consumed vs emitted (must be 1:1 or receipted why not).

## RECON items

- RECON-04a: exact `SpatialSurfaceSet` surface record fields + how
  `buildRoomAssetFromAsciiRoom` fills them (counts for a known fixture, e.g.
  training_room: floors/walls/objects → surface counts) — `observed`.
- RECON-04b: ascii ramp/slope bake shape (terrain sidecar, corner points,
  normals) so creative ramps match — `observed`.
- RECON-04c: the `R` reset marker's exact consumption path in Controller.cpp
  (the live-trigger precedent) — `observed`.
- RECON-04d: a1 sound event input struct (position? radius? loudness? shape) —
  `observed`.
- RECON-04e: does session movement do any broad phase over surfaces, or full
  scan? (Determines whether baked-surface COUNT is a real budget today) —
  `observed` + `measured` if a counter exists.

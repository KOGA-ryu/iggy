# Multi-Room Connectivity — Design Stub

> The #1 gap between "AI authors a room" and "AI generates a dungeon." Today the system is single-room; this is the starting point for making it a graph of connected rooms. **Design not yet decided** — this frames the problem, the code touch-points, and a proposed approach so a session can begin designing rather than re-discovering.

## The problem, stated

A dungeon should be a **graph of rooms connected by portals** (doors/exits that lead somewhere). Today there is no room-to-room concept anywhere: you author one room, you play one room.

## Current state (facts, with touch-points)

- **Only the first room runs.** `makeProductAsciiRoomPackage` pushes exactly one room (`ascii_room/Package.cpp:26`), and the runtime consumes only `package.rooms.front()` (`world/PackageSessionSeed.cpp:309`, `Operations.cpp:112`). A package with extra `.room.*` assets silently ignores them.
- **`E` (exit) and `+`/`s` (doors) are in-room anchors, not portals.** Glyph → `markerTag` → `RoomAnchorAsset.kind` (`anchorKindForMarkerTag`) → `ScenarioEntitySeed` (`entityFromAnchor`). `E` completes an in-room objective (`SessionOutcome::Victory`); `+`/`s` open a door in place. **There is no target-room field anywhere in `RoomAsset` or the anchor structs.**
- **`.iggyroom.txt` carries no link metadata** — just the grid (+ optional `*scale` / `floorN`). Room id, materials, meshes live downstream in the generated `.room.iggy3d.toml`. `package.iggy3d.toml` lists assets but models one room.
- `packageId` is hardcoded `iggy3d.ascii_room_authoring`; `scenarioId = <roomId>.runtime_loop` is the only per-room distinguisher.
- There IS a room *catalog* concept (`BuiltinDungeon.cpp` = a list of single-room presets) — but it's a menu of independent rooms, not a connected graph.

## Requirements

1. **Author** connectivity: a portal in room A points to (room B, arrival anchor). Should be expressible from the ASCII authoring surface (a portal glyph or a portal directive) so AI can generate linked dungeons.
2. **Model** a dungeon as rooms + directed edges (portals) with a designated start room.
3. **Runtime** room transition: on reaching a portal, load the target room and spawn the player at the target arrival anchor, deterministically.
4. Preserve determinism + receipts (each room deterministic; the traversal reproducible; add `dungeon_*` / transition receipt keys).

## Proposed model (starting point — not final)

- **Portal anchor**: extend the anchor/marker with an optional `targetRoomId` + `targetAnchorId`. A door/exit with a target becomes a *portal*; without one it stays an in-room door/exit (backward compatible).
  - ASCII expression options (pick one): (a) a new directive block mapping a marker instance → target (keeps the grid clean); (b) a portal glyph whose target is supplied out-of-band (grid stays 1-char-per-cell). The grid is 1 semantic per cell (reference §7), so target data likely lives in a directive/sidecar, not the glyph.
- **Dungeon manifest**: a top-level structure = `{ startRoomId, rooms: [roomId...], portals: [{fromRoom, fromAnchor, toRoom, toAnchor}] }`. Could live in `package.iggy3d.toml` (`[[rooms]]` + `[[portals]]`) — the package already lists assets; generalize it beyond `rooms.front()`.
- **Runtime**: a dungeon-session layer above the current single-room session. On portal trigger: finalize/park the current room session, load the target `RoomAsset`, rebuild collision + entities, spawn player at `toAnchor`. Decide whether room state persists (revisiting) or resets.

## Code touch-points (where the work lands)

| Concern | File(s) | Change |
|---|---|---|
| Anchor/portal data | `content/assets/RoomAsset.hpp` (anchor struct), `ascii_room/AsciiRoomGrid.*` | add optional `targetRoomId`/`targetAnchorId` to the anchor; author it |
| Package = many rooms | `ascii_room/Package.cpp`, `world/PackageSessionSeed.cpp:309`, `Operations.cpp:112` | stop assuming `rooms.front()`; carry all rooms + the manifest |
| Manifest schema | `package.iggy3d.toml` loader (`content/`), `ascii_room/Package.*` | `[[rooms]]` + `[[portals]]` + `start_room` |
| Runtime transition | new dungeon-session module above `runtime/session/Session` + `Operations.cpp` | load target room, respawn player at arrival anchor, emit transition receipt |
| Receipts | `ReceiptBuilder.*` | `dungeon_room_count`, `dungeon_current_room`, `dungeon_transition_*` |

## Phased plan (suggested)

1. **Data model only** (no runtime): add `targetRoomId`/`targetAnchorId` to anchors + a dungeon manifest struct + loader, all receipt-verified, no behavior change. Prove multi-room packages *parse*.
2. **Runtime transition (2 rooms)**: hardcode/author a 2-room dungeon; on reaching a portal, load room B and spawn at its arrival anchor. Prove via receipts (current-room changes, player at arrival anchor).
3. **Authoring surface**: expose portals from ASCII (directive/sidecar) so `tools/ascii_room.py` can author a linked dungeon; add a `tools/ascii_dungeon.py` that compiles + boots a multi-room manifest.
4. **Generator**: AI emits a room graph (rooms + portals) — the payoff. Feeds Stream 2's closed loop for whole dungeons.

## Open decisions (need a product call)

- Portal expression in ASCII: directive block vs sidecar vs glyph-with-out-of-band-target?
- Room state on revisit: persistent (a living dungeon) or reset (a level)?
- Is a "dungeon" a new asset type, or a generalization of the existing package (`rooms.front()` → `rooms[*]` + manifest)?
- Coordinate/streaming model: one room loaded at a time (transition = swap), or adjacent rooms co-loaded (seamless)? Start with swap.

## How to verify (keep the discipline)

Each phase gated by build + full suite; new behavior proven by new receipt keys (deterministic). A 2-room manifest fixture + a smoke that activates it, walks to the portal (seed position, see the automation-move gotcha in [`ascii_dungeon_authoring_reference.md`](ascii_dungeon_authoring_reference.md) §10), and asserts the room changed + player arrived — that's the milestone-2 proof.

# Creative Object Optimization — iggy3d Build Plans

> Planner-authored 2026-07-05. This set turns the prototype research notes
> (`~/iggy/engine/research/creative_object_optimization/`, reference-only — no work
> happens there) into implementation-grade worker packets for **iggy3d**. Numbering
> mirrors the research set 1:1 where a counterpart exists.
>
> **Governing contract:** `docs/creative_mode/world_foundation_plan_v0_1.md`
> (content v0.4). Nothing in this set may contradict D1–D8, the §7 rulings, or the
> W0–W8/W1w work orders. Where this set and the foundation plan describe the same
> system, the foundation plan wins and this set adds detail.

## Documents

- `00_map_and_laws.md` — reconciliation with the foundation plan, the anchors
  master table (research name → iggy3d reality), the algorithm laws, packet DAG,
  lanes, and entry criteria. **Read first; every other doc cites it.**
- `01_descriptor_capability_columns.md` — capability/bake-domain columns on the
  EXISTING descriptor table (rides W3 / D6). ↔ research 01 (+ folds research 07's
  per-category checklists into column policy).
- `02_spatial_occupancy_index.md` — `CreativeOccupancyIndex`: sparse hash grid,
  proxies, queries, DDA ray pick (rides W8/L4). The first implementation packet.
  ↔ research 02.
- `03_render_culling_lod.md` — draw adapter, culling stack, LOD/instancing
  sockets (rides W8 `adapters/Draw` + W1w renderer-parity law). ↔ research 03.
- `04_gameplay_bake_bridges.md` — collision / nav / trigger / light / audio /
  event bridges to the runtime (the named deferred exports; post-W5/W6).
  ↔ research 04.
- `05_dirty_bake_pipeline.md` — dirty accumulator, channel map, bake DAG,
  double-buffered outputs (rides W4). ↔ research 05.
- `06_backlog_and_worker_packets.md` — the OP-order DAG, entry criteria, gates,
  and pre-filled recon packets. ↔ research 06.

## Operating rules (inherited + local)

- Object truth stays centralized: descriptor rows and document objects (D1, D6).
- A new per-kind fact is a **descriptor column, never a switch** (D6).
- Editor objects are authoring hints that **bake** cheaper runtime structures;
  creative mutations NEVER touch live gameplay (§7 R4) — bridges are explicit,
  named, and consumed on activation only.
- Simple structures first: bounds, sparse uniform grid, dirty channels, explicit
  bake outputs. Upgrades (BVH, clustered anything) require the metric named in
  the packet, measured, over threshold.
- Do not optimize from vibes. Every claim in a worker return is labeled
  `measured` / `observed` / `inferred` / `unknown`.
- Every packet ships the full DNA: source → generation → validation → REPAIR →
  decision → report (receipts). Metrics are part of done, not garnish.
- Each doc stays under 1000 lines and is independently reviewable.

## Lanes

- **[C] Codex** — `src/app/iggy3d/creative/**` + the creative window/menu frames
  + their tests/cmake rows.
- **[P] box fleet** — runtime seams: `src/runtime/**`, `src/content/**`,
  `src/app/iggy3d/{save,world,menu,window router}` non-creative files.
- The human owns git; one lane per commit, never mixed.

# Plan Bucket

This folder is the planning bucket for future iggy3d work.

## Working Rule

Codex may only create or update plan documents in this folder unless the user
explicitly changes this rule.

Codex must not:

- edit source code;
- edit tests;
- edit build files;
- commit;
- push;
- dispatch workers for implementation;
- set worker model overrides;
- set worker thinking overrides.

If a worker order is ever allowed again, the order must omit model and thinking
fields unless the user explicitly approves them.

## Document Shape

Each plan document must define:

- objective;
- owned files;
- no-go files;
- data ownership;
- control flow;
- state semantics;
- receipt or proof fields;
- test plan;
- acceptance gate;
- open questions;
- implementation stop rules.

## Current Buckets

- Frontend menu implementation strategy
- Starter menu contract
- Settings screen contract
- Dev tools contract
- Vertical faded selector contract
- Save snapshot contract
- Save load UX contract
- Product save catalog contract
- ASCII room authoring contract
- Runtime save durability contract
- NPC behavior contract (v0.2 profile baseline)
- World creation UX contract
- Selection and cursor contract
- In-game pause menu contract
- Product frontend router contract
- Product automation cleanup contract

## Implementation Strategy Index

- `frontend_menu_implementation_strategy_v1.md`
  - durable module layout, routing semantics, semantic input, receipts, tests,
    migration strategy, and stop rules;
- `starter_menu_contract_v1.md`
  - startup rows, child panels, save selectors, delete confirmation, and launch
    intent;
- `settings_screen_contract_v1.md`
  - shared starter/pause settings model, draft/apply/restore behavior, and
    persistence classes;
- `dev_tools_contract_v1.md`
  - runtime overlay layers, dev toggle, read-only/debug command boundaries, and
    receipt proof;
- `vertical_faded_selector_contract_v1.md`
  - reusable selector model for save files, chapters, and world presets;
- `save_snapshot_contract_v1.md`
  - save sidecar snapshot ownership, pause-save capture rules, selector
    fallback behavior, and no-window proof fields;
- `save_load_ux_contract_v0_1.md`
  - runtime save baseline, product save identity, world grouping, Continue
    policy, atomic write requirements, soft delete/recovery, load failure,
    autosave rules, and key-value receipt fields;
- `product_save_catalog_contract_v0_1.md`
  - rebuildable in-memory save catalog, `worldTitle` title policy, newest
    compatible Continue selection, active/deleted row semantics, coding methods,
    compute costs, receipts, and builder slices;
- `ascii_room_authoring_contract_v0_1.md`
  - ASCII map-authoring truth only, glyph vocabulary, semantic grid ownership,
    row/column to X/Z coordinate math, geometry generation algorithms, proof
    fields, compute costs, and parser-first builder slices;
- `runtime_save_durability_contract_v0_1.md`
  - current direct-write/hard-delete baseline, durable write sequence,
    same-directory temp path policy, result structs, soft delete boundary,
    receipt fields, and path/result-only first implementation slice;
- `npc_behavior_contract_v0_1.md`
  - completed runtime-owned NPC behavior v0.1 baseline plus v0.2 profile
    baseline, durable AI state and actor profile ids, hostile/passive
    engagement policy, built-in profile resolution, session tick profile
    application, scenario-authored NPC profile bindings, package validation,
    product no-window tape proof, runtime edge proofs, and deferred next NPC
    work;
- `world_creation_ux_contract_v0_1.md`
  - current New World launch baseline, world setup draft fields, initial save
    gate, world identity, product orchestration ownership, route semantics,
    receipts, and model-only first implementation slice;
- `product_frontend_router_contract_v0_1.md`
  - existing route/helper baseline, table-driven/pure routing methods, owner
    priority, parent return rules, transition vocabulary, receipt proof, and
    owner-decision-only first implementation slice;
- `selection_cursor_contract_v1.md`
  - crosshair versus mouse cursor selection authority, cursor state ownership,
    hit-test routing, and dev tools inspector wiring.

## Long-Term Codebase Fit

Frontend implementation should use durable product modules, not temporary
branches in `src/app/iggy3d/AppShell.cpp`.

The intended direction is:

- model files own rows, state, disabled reasons, and data contracts;
- router files consume semantic actions and return route results;
- view files draw already-built view models;
- receipt files report deterministic proof;
- `AppShell.cpp` composes lifecycle, input collection, routing, drawing, and
  receipts without owning per-menu behavior.

Save snapshots and selection/cursor state follow the same rule:

- save files remain gameplay truth, while snapshot images are sidecar
  presentation data;
- active camera or frontend mode decides selection source;
- frontend cursor rendering is overlay state, not gameplay logic;
- dev tools inspect shared selection summaries, not private per-tool picks.

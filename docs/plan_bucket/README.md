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
  - reusable selector model for save files, chapters, and world presets.

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

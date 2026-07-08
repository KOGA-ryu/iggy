# E209: Operations Seam Split Preflight

## Status

Ready.

## Context

`src/app/iggy3d/Operations.cpp` is still a broad product-app catch-all after
the god-struct and identity-mirror cleanup. The complexity audit calls out
`Operations.cpp` as a mixed-domain file and says to split it only after the
active-creative identity mirror work was removed. That prerequisite is now
complete.

This is a read-only preflight. Do not move code yet. The goal is to produce the
small implementation cards that can safely split `Operations.cpp` by seam.

Current rough size:

- `src/app/iggy3d/Operations.cpp`: about 1.6k lines
- `src/app/iggy3d/Operations.hpp`: about 200 lines

Known candidate seams visible from the public API and current implementation:

- save slot / save browser / delete / recover flow
- package/session bootstrap and gameplay launch
- creative new/open/save world operations
- creative baked active-room refresh service
- world-template / ASCII package helpers

## Objective

Audit `Operations.cpp` and `Operations.hpp`, classify their functions into
cohesive seams, and draft small follow-up implementation cards. The output
should let reviewer release one narrow move at a time instead of giving builder
a large refactor.

## Scope

Read-only. Do not edit source, tests, CMake, fixtures, receipt golden, or
production docs.

Allowed file move/edit:

- this task card only

Inspect at minimum:

- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/Operations.hpp`
- direct call sites for public Operations APIs under `src/app/iggy3d` and
  `tests/unit`
- `docs/complexity_audit_v0_1.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`

## Required Work

1. Inventory public declarations in `Operations.hpp` and top-level helper /
   function definitions in `Operations.cpp`.
2. Group each public API and major internal helper into one proposed owner
   seam. Use existing naming and behavior, not aspirational architecture.
3. For each seam, report:
   - candidate new file/header name;
   - public APIs it would own;
   - private helpers it would move with those APIs;
   - expected direct callers;
   - focused tests that should guard the move.
4. Identify the safest first implementation slice. Prefer a seam with:
   - cohesive public API;
   - small caller set;
   - low risk of circular includes;
   - focused tests already present.
5. Identify explicit non-slices that should not move yet.
6. Draft follow-up implementation cards, each small enough to be reviewed
   independently. At minimum, draft the first card in full.

## Suggested Commands

Run current inventories such as:

```sh
wc -l /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp
rg -n "^[A-Za-z_][A-Za-z0-9_:<>]*[&*[:space:]]+[A-Za-z_][A-Za-z0-9_:]*\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp
rg -n "writeProductCurrentSessionSave|openDeletedProductSaveBrowser|executeProductSaveRecover|openProductSaveDeleteConfirmation|executeProductSaveSoftDelete|launchProductNewWorld|launchProductCreativeNewWorld|launchProductCreativeOpenWorld|saveProductCurrentCreativeWorld|refreshProductCreativeBakedActiveRoom|launchProductContinueSave|launchProductLoadSaveSelection" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
git -C /Users/kogaryu/iggy3d diff --check
```

Add narrower `nl -ba`, `sed`, and `rg` reads as needed to map helper clusters
and call sites.

## Decision Requirements

Report one of:

- **Slice plan ready:** include ordered implementation cards with file names,
  moved APIs/helpers, expected tests, and self-blockers.
- **Blocked:** explain the exact coupling or missing tests that prevent a safe
  first move.

## Completion Brief Requirements

Report:

- card moved to done;
- files inspected;
- function/API inventory summary;
- seam grouping table;
- direct call-site hotspots by seam;
- recommended first implementation slice;
- follow-up card drafts;
- non-slices/deferred work;
- `diff --check` result;
- confirmation that no source, test, CMake, fixture, receipt golden,
  production docs, staging, commit, push, or window launch was performed.

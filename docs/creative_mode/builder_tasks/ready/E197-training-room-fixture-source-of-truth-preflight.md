# E197: Training Room Fixture Source-Of-Truth Preflight

## Status

Ready.

## Context

The shared ASCII training-room source and generated room asset currently exist
beside package-local room copies:

- `fixtures/rooms/ascii/training_room.iggyroom.txt`
- `fixtures/rooms/ascii/training_room.room.iggy3d.toml`
- `fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml`
- `fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`

Quick reviewer evidence before this card:

- shared generated room asset and `ascii_training_room` package copy are
  byte-identical;
- `npc_vision_lab` package copy has drifted from the shared generated asset;
- observed drift is the missing `clamber_candidate` traversal tag on wall
  blocker surfaces;
- both package manifests currently reference local asset paths:
  `assets/rooms/training_room.room.iggy3d.toml`.

This card is read-only. Do not edit fixtures, tests, source, CMake, or docs
outside this task card.

## Objective

Decide the safest way to make the training-room fixture single-source-of-truth,
or at least mechanically drift-proof, without guessing about package-loader path
constraints.

## Scope

Inspect only:

- `fixtures/rooms/ascii/training_room.iggyroom.txt`
- `fixtures/rooms/ascii/training_room.room.iggy3d.toml`
- `fixtures/demos/ascii_training_room/package.iggy3d.toml`
- `fixtures/demos/ascii_training_room/scenario.iggy3d.toml`
- `fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml`
- `fixtures/demos/npc_vision_lab/package.iggy3d.toml`
- `fixtures/demos/npc_vision_lab/scenario.iggy3d.toml`
- `fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`
- `tests/unit/ascii_room_asset_text_fixture_tests.cpp`
- `tests/unit/ascii_room_package_fixture_tests.cpp`
- package-loader path validation/parsing files under `src/content/Package*`
- focused package/fixture tests that load either demo package.

## Required Inventory

Document:

- all checked-in training-room source/generated/package-copy paths;
- which copies are byte-identical and which are not;
- exact diff class for any drift, not the whole file;
- every package manifest that references a package-local training-room asset;
- every existing parity/regeneration test for the shared generated room asset;
- whether existing package-loader validation allows package assets to reference a
  shared fixture outside the package directory;
- which tests would be the correct drift guards for each package.

## Required Commands

Run and summarize:

```sh
find /Users/kogaryu/iggy3d/fixtures -path '*training_room*' -print | sort

cmp -s /Users/kogaryu/iggy3d/fixtures/rooms/ascii/training_room.room.iggy3d.toml \
  /Users/kogaryu/iggy3d/fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml

cmp -s /Users/kogaryu/iggy3d/fixtures/rooms/ascii/training_room.room.iggy3d.toml \
  /Users/kogaryu/iggy3d/fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml

diff -u /Users/kogaryu/iggy3d/fixtures/rooms/ascii/training_room.room.iggy3d.toml \
  /Users/kogaryu/iggy3d/fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml | sed -n '1,220p'

rg -n "training_room\\.room\\.iggy3d\\.toml|training_room\\.iggyroom\\.txt|training_room_ascii|ascii_training_room|npc_vision_lab" \
  /Users/kogaryu/iggy3d/fixtures \
  /Users/kogaryu/iggy3d/tests \
  /Users/kogaryu/iggy3d/src \
  /Users/kogaryu/iggy3d/docs \
  --glob '!build/**' \
  --glob '!docs/creative_mode/builder_tasks/done/**' \
  --glob '!docs/creative_mode/builder_tasks/blocked/**' \
  --glob '!docs/file_plans/**'

rg -n "\\.\\.|absolute|invalid.*path|asset.*path|PackageManifest|loadPackage|path =" \
  /Users/kogaryu/iggy3d/src/content \
  /Users/kogaryu/iggy3d/tests/unit/package_loader_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/ascii_room_package_fixture_tests.cpp \
  --glob '*.cpp' --glob '*.hpp'

git -C /Users/kogaryu/iggy3d diff --check
```

No build or CTest is required unless you choose to run an existing fixture test
for additional evidence. If you do run tests, report exactly which ones.

## Decision Buckets

Classify the next implementation as one of:

1. **True Single Source**: package manifests can safely reference the shared
   generated room asset without weakening package path validation.
2. **Generated Copy With Drift Guard**: package-local copies are required by
   package-loader constraints, so the implementation should copy/regenerate from
   the shared generated asset and add/extend parity tests for every package-local
   copy.
3. **More Preflight Needed**: package identity, scenario behavior, or loader
   validation needs an owner decision before touching fixture files.

## Draft Follow-Up Card

Append a draft implementation card to this done card. The draft must include:

- exact files to edit;
- whether `npc_vision_lab` room copy should be updated to match the shared
  generated asset;
- whether package manifests change or stay package-local;
- exact drift guard tests to add or extend;
- exact verification commands.

Do not create the follow-up card in `ready/`; reviewer will promote it after
reviewing this audit.

## Completion Brief Requirements

Report:

- byte-identity results and hashes for all three generated room asset files;
- concise drift summary for non-identical copies;
- package-path validation conclusion;
- recommended decision bucket;
- draft follow-up card title and scope;
- commands run;
- confirmation that no source/test/CMake/fixture files were edited.

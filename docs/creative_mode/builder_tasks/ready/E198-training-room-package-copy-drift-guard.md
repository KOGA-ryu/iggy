# E198: Training Room Package Copy Drift Guard

## Status

Ready.

## Context

E197 chose **Generated Copy With Drift Guard** for the training-room fixture
duplication:

- package assets are package-relative by design;
- package-loader path validation rejects absolute paths and `..` components;
- shared generated room asset and the `ascii_training_room` package copy are
  byte-identical;
- the `npc_vision_lab` package copy has drifted by missing
  `clamber_candidate` traversal tags on 20 wall/blocker surfaces.

Do not attempt manifest-level true single-source routing in this card. That
would require a package asset alias/fixture-root mechanism and is broader than
this drift fix.

## Objective

Make package-local `training_room.room.iggy3d.toml` copies mechanically
drift-proof while preserving package-local asset paths.

## Scope

Edit only:

- `fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`
- `tests/unit/ascii_room_package_fixture_tests.cpp`
- this task card

Do not edit:

- `fixtures/rooms/ascii/training_room.iggyroom.txt`
- `fixtures/rooms/ascii/training_room.room.iggy3d.toml`
- package manifests;
- scenario files;
- package-loader validation;
- package schemas;
- RoomAsset writer/parser;
- CMake;
- renderer/window code;
- receipt golden.

## Required Fixture Change

Update:

- `fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`

to match exactly:

- `fixtures/rooms/ascii/training_room.room.iggy3d.toml`

The expected semantic change is the 20 missing `clamber_candidate` traversal tags
on wall/blocker surfaces. No geometry, ids, anchors, role strings, source
metadata, counts, package manifests, or scenario behavior should change.

## Required Test Change

Extend `tests/unit/ascii_room_package_fixture_tests.cpp`.

Preserve existing `ascii_training_room` parity/load assertions.

Add `npc_vision_lab` coverage:

- shared generated asset text equals
  `fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`;
- `loadPackage({"fixtures/demos/npc_vision_lab/package.iggy3d.toml"})` returns
  `PackageLoadStatus::Ok`;
- package id is `iggy3d.npc_vision_lab`;
- scenario id is `npc_vision_lab.runtime_loop`;
- package has one room and the same core room id/counts as the shared training
  room;
- loaded room has `clamber_candidate` on blocker wall surfaces if a small local
  helper can assert this cleanly.

Keep this as a fixture/package parity test. Do not move it into package-loader
behavior or product session behavior.

## Self-Blockers

Move this card to `blocked/` with evidence instead of widening scope if:

- `npc_vision_lab` intentionally requires a different room payload than shared
  `training_room_ascii`;
- the package load guard requires package-loader or manifest changes;
- the parity guard cannot be added without changing fixture generation,
  RoomAsset parsing/writing, or CMake.

## Required Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build \
  --target ascii_room_asset_text_fixture_tests ascii_room_package_fixture_tests package_loader_tests product_ascii_package_smoke \
  -j10

ctest --test-dir /Users/kogaryu/iggy3d/build \
  -R '^(ascii_room_asset_text_fixture_tests|ascii_room_package_fixture_tests|package_loader_tests|product_ascii_package_smoke)$' \
  --output-on-failure

cmp -s /Users/kogaryu/iggy3d/fixtures/rooms/ascii/training_room.room.iggy3d.toml \
  /Users/kogaryu/iggy3d/fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml

cmp -s /Users/kogaryu/iggy3d/fixtures/rooms/ascii/training_room.room.iggy3d.toml \
  /Users/kogaryu/iggy3d/fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml

git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

No full CTest is required unless the implementation widens beyond this card.

## Completion Brief Requirements

Report:

- exact files changed;
- confirmation that `npc_vision_lab` room copy now matches the shared generated
  room asset byte-for-byte;
- whether package manifests remained package-local and unchanged;
- exact parity/load guards added;
- whether loaded `npc_vision_lab` room now proves `clamber_candidate`;
- tests/checks run;
- any concerns/deferred work.

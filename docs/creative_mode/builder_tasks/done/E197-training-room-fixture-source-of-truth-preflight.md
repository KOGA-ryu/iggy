# E197: Training Room Fixture Source-Of-Truth Preflight

## Status

Done.

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

## Completion Brief

- Card moved to done: yes.
- Files inspected:
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
  - `tests/unit/package_loader_tests.cpp`
  - `tests/unit/ascii_room_runtime_collision_tests.cpp`
  - `tests/unit/product_package_session_seed_tests.cpp`
  - `tests/smoke/product_ascii_package_smoke.cpp`
  - `src/content/PackageLoader.cpp`
  - `src/content/PackageLoader.hpp`
  - `src/content/PackageManifest.hpp`
  - `src/content/PackageValidator.cpp`
- Checked-in training-room source/generated/package-copy paths:
  - Source ASCII: `fixtures/rooms/ascii/training_room.iggyroom.txt`
  - Shared generated RoomAsset: `fixtures/rooms/ascii/training_room.room.iggy3d.toml`
  - `ascii_training_room` package copy: `fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml`
  - `npc_vision_lab` package copy: `fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`
- Byte-identity and hashes:
  - `fixtures/rooms/ascii/training_room.room.iggy3d.toml`: `e1dfb031c2897325c6c3839883237c7540b9367baae1f404880bfa0bb8ba9ccb`
  - `fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml`: `e1dfb031c2897325c6c3839883237c7540b9367baae1f404880bfa0bb8ba9ccb`
  - `fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`: `0a448245db0cafce00dcda91ed3d4469d851a640291ae1cdd18a5b12445f5782`
  - `cmp` shared vs `ascii_training_room` package copy: status `0`, byte-identical.
  - `cmp` shared vs `npc_vision_lab` package copy: status `1`, not identical.
- Drift summary:
  - The three generated asset files all have 1112 lines.
  - The `npc_vision_lab` copy drift is traversal-tag payload only: 20 wall/blocker `traversal_tags` lines contain `["blocker"]` instead of the shared/generated `["blocker", "clamber_candidate"]`.
  - No geometry, ids, anchors, source metadata, role strings, counts, or file layout differences appeared in the required `diff -u ... | sed -n '1,220p'` window; the visible diff class is missing `clamber_candidate` on blocker wall surfaces.
  - Shared/generated and `ascii_training_room` package-copy `clamber_candidate` lines are at `517, 545, 573, 601, 629, 657, 685, 713, 741, 769, 797, 825, 853, 881, 909, 937, 965, 993, 1021, 1049`. The `npc_vision_lab` copy has no `clamber_candidate` hits.
- Package manifests referencing package-local training-room assets:
  - `fixtures/demos/ascii_training_room/package.iggy3d.toml:8-9`
    - `id = "room.training_room_ascii"`
    - `path = "assets/rooms/training_room.room.iggy3d.toml"`
  - `fixtures/demos/npc_vision_lab/package.iggy3d.toml:8-9`
    - `id = "room.training_room_ascii"`
    - `path = "assets/rooms/training_room.room.iggy3d.toml"`
- Existing parity/regeneration guards:
  - `tests/unit/ascii_room_asset_text_fixture_tests.cpp:16-19` names the ASCII source and shared generated asset.
  - `tests/unit/ascii_room_asset_text_fixture_tests.cpp:80-118` regenerates RoomAsset text from the ASCII source and asserts byte parity with `fixtures/rooms/ascii/training_room.room.iggy3d.toml`, plus parse/count/anchor sanity.
  - `tests/unit/ascii_room_asset_text_fixture_tests.cpp:121-137` has an env-gated regeneration path: `ASCII_ROOM_FIXTURE_REGEN=1 ./build/ascii_room_asset_text_fixture_tests`.
  - `tests/unit/ascii_room_package_fixture_tests.cpp:14-19` names the shared generated asset, the `ascii_training_room` package-local copy, and the `ascii_training_room` package.
  - `tests/unit/ascii_room_package_fixture_tests.cpp:81-131` asserts `ascii_training_room` package room text parity with the shared generated asset, package load success, package/scenario ids, asset counts, room counts, and anchor/surface sanity.
  - There is no matching package-copy parity or load guard for `fixtures/demos/npc_vision_lab/package.iggy3d.toml`; `rg` found only fixture/docs references for `npc_vision_lab`.
- Package-loader path validation conclusion:
  - Current package loading does not safely allow package assets to reference the shared fixture outside the package directory.
  - `src/content/PackageLoader.cpp:81-84` treats an asset/scenario path as invalid if it is absolute, contains a `..` path component, has an unexpected filename, or includes the forbidden legacy prefix.
  - `src/content/PackageLoader.cpp:252-258` applies that `invalidRelativePath(...)` check to scenario and asset paths during package parsing.
  - `src/content/PackageLoader.cpp:317-323` reads each asset from `packageDirectory / asset.path`, so package assets are package-relative by design.
  - `tests/unit/package_loader_tests.cpp:695-709` pins invalid absolute/legacy and parent-component scenario paths as `PackageLoadStatus::InvalidPath`. The same `invalidRelativePath(...)` helper is used for assets, though there is not currently a separate asset-path rejection test.
  - `src/content/PackageValidator.cpp:167-180` validates the manifest/scenario and old dependency strings, but the loader is the strict path containment seam for package file reads.
  - A true manifest-level single source would require relaxing/adding a package asset alias/fixture-root mechanism. That is broader than this fixture drift fix and risks weakening package containment.
- Correct drift guards by package:
  - Shared generated asset: keep `ascii_room_asset_text_fixture_tests` as the exporter/source parity and regeneration guard.
  - `ascii_training_room` package copy: keep `ascii_room_package_fixture_tests` package-copy parity and load guard.
  - `npc_vision_lab` package copy: extend `ascii_room_package_fixture_tests` with an analogous shared-vs-package-copy text parity check and `loadPackage(...)` smoke for `fixtures/demos/npc_vision_lab/package.iggy3d.toml`.
- Recommended decision bucket:
  - **Generated Copy With Drift Guard**.
  - Rationale: package-local copies are consistent with the loader’s package-relative path rules. The safe next slice is to update the stale `npc_vision_lab` generated copy from the shared generated asset and add parity/load guards so future exporter changes fail deterministically.
- Draft follow-up card:
  - Title: `E198: Training Room Package Copy Drift Guard`
  - Objective: Make package-local `training_room.room.iggy3d.toml` copies mechanically drift-proof while preserving package-local asset paths.
  - Scope:
    - Edit `fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml` by copying the current shared generated asset from `fixtures/rooms/ascii/training_room.room.iggy3d.toml`.
    - Edit `tests/unit/ascii_room_package_fixture_tests.cpp` to add `npc_vision_lab` constants and parity/load coverage.
    - Do not edit package manifests; keep both package manifests pointing at `assets/rooms/training_room.room.iggy3d.toml`.
    - Do not edit package-loader validation, schemas, RoomAsset writer/parser, scenario files, source ASCII, CMake, renderer/window code, or receipt golden.
  - Exact files to edit:
    - `fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`
    - `tests/unit/ascii_room_package_fixture_tests.cpp`
  - Fixture policy:
    - `npc_vision_lab` room copy should be updated to match the shared generated asset byte-for-byte.
    - Package manifests stay package-local because loader containment rejects absolute/parent paths and resolves assets under the package directory.
  - Drift guard tests:
    - Preserve existing `ascii_training_room` parity/load assertions.
    - Add `npc_vision_lab` package room text parity:
      - read `fixtures/rooms/ascii/training_room.room.iggy3d.toml`
      - read `fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`
      - assert byte equality
    - Add `npc_vision_lab` package load sanity:
      - load `fixtures/demos/npc_vision_lab/package.iggy3d.toml`
      - assert `PackageLoadStatus::Ok`
      - assert package id `iggy3d.npc_vision_lab`
      - assert scenario id `npc_vision_lab.runtime_loop`
      - assert one room and same core room id/counts as the shared training room
      - optionally assert the loaded room has `clamber_candidate` on blocker surfaces if a small local helper can do that without broadening the test.
  - Verification:
    - `cmake --build /Users/kogaryu/iggy3d/build --target ascii_room_asset_text_fixture_tests ascii_room_package_fixture_tests package_loader_tests product_ascii_package_smoke -j10`
    - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(ascii_room_asset_text_fixture_tests|ascii_room_package_fixture_tests|package_loader_tests|product_ascii_package_smoke)$' --output-on-failure`
    - `cmp -s /Users/kogaryu/iggy3d/fixtures/rooms/ascii/training_room.room.iggy3d.toml /Users/kogaryu/iggy3d/fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml`
    - `cmp -s /Users/kogaryu/iggy3d/fixtures/rooms/ascii/training_room.room.iggy3d.toml /Users/kogaryu/iggy3d/fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`
    - `git -C /Users/kogaryu/iggy3d diff --check`
    - Focused trailing-whitespace scan over touched files.
  - Self-blockers:
    - If `npc_vision_lab` intentionally requires a different room payload than shared `training_room_ascii`, stop and ask for owner decision.
    - If adding a load guard requires package-loader changes, stop; this card should stay fixture/test only.
- Commands run:
  - `find /Users/kogaryu/iggy3d/fixtures -path '*training_room*' -print | sort`
  - `shasum -a 256 /Users/kogaryu/iggy3d/fixtures/rooms/ascii/training_room.room.iggy3d.toml /Users/kogaryu/iggy3d/fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml /Users/kogaryu/iggy3d/fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`
  - `cmp -s /Users/kogaryu/iggy3d/fixtures/rooms/ascii/training_room.room.iggy3d.toml /Users/kogaryu/iggy3d/fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml`
  - `cmp -s /Users/kogaryu/iggy3d/fixtures/rooms/ascii/training_room.room.iggy3d.toml /Users/kogaryu/iggy3d/fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`
  - `diff -u /Users/kogaryu/iggy3d/fixtures/rooms/ascii/training_room.room.iggy3d.toml /Users/kogaryu/iggy3d/fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml | sed -n '1,220p'`
  - `rg -n "training_room\\.room\\.iggy3d\\.toml|training_room\\.iggyroom\\.txt|training_room_ascii|ascii_training_room|npc_vision_lab" /Users/kogaryu/iggy3d/fixtures /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/docs --glob '!build/**' --glob '!docs/creative_mode/builder_tasks/done/**' --glob '!docs/creative_mode/builder_tasks/blocked/**' --glob '!docs/file_plans/**'`
  - `rg -n "\\.\\.|absolute|invalid.*path|asset.*path|PackageManifest|loadPackage|path =" /Users/kogaryu/iggy3d/src/content /Users/kogaryu/iggy3d/tests/unit/package_loader_tests.cpp /Users/kogaryu/iggy3d/tests/unit/ascii_room_package_fixture_tests.cpp --glob '*.cpp' --glob '*.hpp'`
  - `rg -n 'clamber_candidate' fixtures/rooms/ascii/training_room.room.iggy3d.toml fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`
  - `rg -n "fixtures/demos/npc_vision_lab/package\\.iggy3d\\.toml|npc_vision_lab" tests/unit tests/smoke src fixtures --glob '*.cpp' --glob '*.hpp' --glob '*.toml'`
  - `rg -n "loadPackageFixture|fixtures/demos/ascii_training_room/package\\.iggy3d\\.toml|kPackagePath|ascii_training_room" tests/unit/product_package_session_seed_tests.cpp tests/unit/ascii_room_runtime_collision_tests.cpp tests/smoke/product_ascii_package_smoke.cpp tests/unit/ascii_room_package_fixture_tests.cpp`
  - `rg -n "invalid asset path|asset.path|assets/rooms|\\.\\./|absolute|PackageLoadStatus::InvalidPath" tests/unit/package_loader_tests.cpp src/content/PackageLoader.cpp src/content/PackageValidator.cpp tests/unit/ascii_room_package_fixture_tests.cpp`
  - `wc -l fixtures/rooms/ascii/training_room.room.iggy3d.toml fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml`
  - `git -C /Users/kogaryu/iggy3d diff --check`
- Tests run:
  - None. This was a read-only audit card; no build or CTest was required.
- Confirmation:
  - No source, test source, CMake, fixture, receipt golden, package manifest, or scenario files were edited.
  - Only this task card was moved/appended.

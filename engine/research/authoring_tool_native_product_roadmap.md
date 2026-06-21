# Authoring Tool To Native 3D Product Roadmap

This roadmap defines the product direction from the current engine state to a
finished authoring-tool-driven native 3D product. It supersedes any assumption
that the final product path is Qt, fixture-only, or 2D-first.

## Locked Decisions

- The product is made through an authoring tool.
- Qt is not a final product dependency. Existing Qt code can be used only as
  temporary diagnostic/reference scaffolding until deleted.
- The first authored package/demo is a test vehicle for the systems being built,
  not precious content.
- The target presentation is native 3D. 2D paths are scaffolding, regression
  harnesses, or debug projections only.
- `.igmesh` is the first runtime/package mesh contract.
- `.glb`/glTF import is a later conversion pipeline, not the first runtime
  loading contract.

## Finish Line

The project is complete when a user can:

1. Open the authoring tool.
2. Create or edit an authored package.
3. Place rooms, walls/floors, player start, NPCs, item drops, interaction
   targets, and simple progression facts.
4. Assign stable 3D assets to those authored objects.
5. Run check/lint/trace from the tool.
6. Launch the package in the native 3D runtime.
7. Play, interact, save, load, reset, and complete the authored scenario.
8. Export or hand off the package with its validated runtime assets.

The proof is not a pile of tests. The proof is a visible package made by the
tool and played in the native 3D runtime, with tests guarding the same path.

## Product Architecture

```text
Authoring Tool
  -> authored package
  -> package manifest + scenario + asset refs
  -> validation/check/trace
  -> native 3D product runtime
  -> save/load/reset/play
```

Asset path:

```text
Phase 1:
stable asset id
  -> .igmesh
  -> native 3D renderer

Later:
.glb/.gltf import
  -> validate supported subset
  -> convert to .igmesh plus material/texture records
  -> package references converted engine assets
```

## Roadmap

### Phase 0: Baseline And Worktree Control

Goal: establish a clean, trusted base before larger product work.

Tasks:
- Review the current dirty worktree and separate unrelated changes from the
  authoring/content/native path.
- Keep the first product-facing demo package under `engine/content/demos`.
- Keep acceptance tests pointed at visible demo content, not only
  `engine/tests/fixtures`.
- Run focused product checks, full build, full CTest, and `git diff --check`
  before integration.

Exit criteria:
- A clean baseline branch exists.
- The current product-loop demo package is committed or otherwise preserved as
  visible proof.
- The project can be rebuilt and tested from the baseline.

### Phase 1: Content Proof Becomes Product Proof

Goal: turn fixtures into authored product packages.

Tasks:
- Keep `engine/tests/fixtures` for edge-case and regression inputs.
- Use `engine/content/demos` for visible authored packages.
- Promote the strongest fixture behaviors into demo packages:
  - pickup;
  - required-item door;
  - target discovery;
  - NPC movement;
  - blocked movement;
  - save/load/reset;
  - completion/failure once defined.
- Every demo package must include:
  - `package.toml`;
  - scenario source;
  - README;
  - embedded expectations;
  - CLI check/trace coverage;
  - native smoke coverage;
  - product-loop acceptance coverage.

Exit criteria:
- At least one strong package proves the full current loop.
- Additional packages cover specific systems without becoming hidden test-only
  fixtures.

### Phase 2: Package Contract V1

Goal: lock the authored package boundary before building the tool around it.

Current contract:
- one explicit package directory;
- one `package.toml`;
- one main scenario file;
- display metadata;
- no discovery;
- no dependencies;
- no multi-scenario package behavior.

Add only when required:
- stable package id;
- content version;
- tags/category;
- thumbnail or preview metadata;
- asset manifest path;
- package compatibility policy.

Hard stops:
- no package manager;
- no recursive discovery;
- no dependency resolver;
- no hidden defaults;
- no runtime behavior controlled by display-only metadata.

Exit criteria:
- The authoring tool and native runtime consume the same package contract.
- Bad package errors are visible and test-covered.

### Phase 3: Asset Contract And `.igmesh` Runtime Path

Goal: make 3D assets first-class package facts without taking on full glTF
runtime complexity.

Tasks:
- Define package asset references by stable asset id.
- Map authored object kinds to asset slots:
  - floor;
  - wall;
  - player;
  - NPC;
  - item;
  - interaction target;
  - debug/fallback marker.
- Resolve asset ids to `.igmesh` files for the native runtime.
- Add asset validation:
  - missing asset;
  - invalid mesh;
  - unsupported material;
  - fallback used;
  - unused asset;
  - unknown asset id.
- Add an asset report command usable by tests and the authoring tool.

Exit criteria:
- A demo package controls which `.igmesh` assets appear in the native 3D
  runtime.
- Asset failures are visible before play.

### Phase 4: Native 3D Runtime As The Real Product Surface

Goal: move the product proof out of Qt/debug views and into native 3D.

Tasks:
- Keep `iggy_native_play` as the executable product proof path.
- Expand native scene draw-list/model-slot support from hardcoded defaults to
  package-driven asset refs.
- Render authored floor, walls, player, NPCs, items, and interaction markers.
- Add minimal product UI/HUD in native:
  - package title/status;
  - inventory summary;
  - interaction prompt;
  - save/load/reset status;
  - completion/failure result.
- Preserve deterministic scripted controls for regression.
- Add visual diagnostics only when they help authoring:
  - selected/hovered target;
  - collision/debug overlay;
  - NPC path/intent overlay;
  - asset fallback overlay.

Hard stops:
- no Qt dependency in the final product path;
- no 2D product scope expansion;
- no renderer ownership leaking into gameplay state;
- no package parsing inside renderer code.

Exit criteria:
- The demo package is playable and understandable in native 3D.
- Native scripted controls and acceptance tests exercise the same runtime path.

### Phase 5: Authoring Tool Read-Only Mode

Goal: build the tool around existing package/check/runtime truth before mutation.

Tasks:
- Open an explicit package path.
- Show package metadata and package errors.
- Show scenario validation/check/trace results.
- Show final rows or equivalent debug projection as a diagnostic, not product
  presentation.
- Show asset report and fallback status.
- Show object lists:
  - rooms/regions;
  - tiles/walls/floors;
  - player start;
  - NPCs;
  - item drops;
  - interaction targets;
  - frame/script facts.
- Launch native play from the loaded package.

Open question:
- The final native authoring shell still needs a concrete UI/windowing choice
  after Qt deletion. Candidate paths are a bespoke native editor shell, an
  SDL/native renderer tool surface, or another explicitly approved non-Qt stack.

Exit criteria:
- The authoring tool can inspect and launch a package without editing it.

### Phase 6: Authoring Tool Mutation Mode

Goal: create packages through structured controls instead of hand-editing TOML.

Tasks:
- Add package creation.
- Add grid/room editing.
- Add floor/wall placement.
- Add player start placement.
- Add NPC placement and profile assignment.
- Add item drop placement.
- Add interaction target placement.
- Add required-item interaction setup.
- Add frame/script ordering controls for current supported semantics.
- Save back to the package source format.
- Re-run check after each explicit save/build action.

Hard stops:
- no UI-owned runtime truth;
- no silent source mutation;
- no hidden package discovery;
- no broad scripting language;
- no editor-only behavior that cannot run in native product runtime.

Exit criteria:
- A user can create the demo package from the tool and run it in native 3D.

### Phase 7: Product Loop Completion

Goal: make the authored native product loop feel complete at small scale.

Tasks:
- Define scenario start, retry, reset, pause, save, load, completion, and
  failure states.
- Persist only durable gameplay truth.
- Preserve package identity/path/version where needed for load/resume.
- Add native UX for save slots.
- Add deterministic acceptance tests for save/load across authored packages.

Exit criteria:
- A package can be played, saved, loaded, reset, and completed without manual
  test harness interpretation.

### Phase 8: Gameplay Semantics By Gate

Goal: grow gameplay only through authored packages that need it.

Candidate gates:
- inspect text;
- talk target;
- region trigger;
- emit/report event;
- objective/win/fail condition;
- equipment;
- simple combat.

Rule:
- Pick one semantic at a time.
- Add source format.
- Add validation.
- Add tool controls.
- Add native runtime behavior.
- Add demo package proof.
- Add tests on the same path.

Hard stops:
- no generic event bus first;
- no dialogue tree system before a small talk proof;
- no combat/progression framework before a small authored objective proof.

Exit criteria:
- Each new gameplay feature is visible in authored content and native 3D play.

### Phase 9: `.glb` Import Pipeline

Goal: accept real external 3D assets without making glTF the runtime contract.

Tasks:
- Define supported glTF/glb subset:
  - static mesh;
  - coordinate convention;
  - scale policy;
  - material subset;
  - texture subset;
  - node transform handling;
  - unsupported animation policy.
- Import `.glb` into validated engine assets.
- Convert mesh data to `.igmesh`.
- Convert or reject materials/textures according to the supported subset.
- Record import diagnostics.
- Let the authoring tool import `.glb` and then place the converted asset.

Hard stops:
- no arbitrary glTF feature support;
- no runtime dependency on unvalidated external files;
- no silent coordinate/scale correction without diagnostics.

Exit criteria:
- A user can import a supported `.glb`, get a validated engine asset, place it
  in the authoring tool, and run it in native 3D.

### Phase 10: Qt Deletion

Goal: remove Qt once native authoring/play surfaces cover required behavior.

Prerequisites:
- Native 3D play supports package launch, gameplay, HUD, save/load/reset, and
  diagnostics needed for product proof.
- Authoring tool supports package inspect/create/edit/check/launch.
- Any remaining Qt-only diagnostic has a native/tool replacement or is declared
  obsolete.

Tasks:
- Remove Qt CMake path.
- Remove Qt shell app sources.
- Remove Qt-specific docs and tests or replace them with native/tool coverage.
- Run full build and CTest after deletion.

Exit criteria:
- The repo builds without Qt.
- No roadmap or docs describe Qt as part of the final product path.

### Phase 11: Release Packaging

Goal: produce a hand-offable product slice.

Tasks:
- Add release packaging for:
  - native runtime;
  - authoring tool;
  - demo packages;
  - validated assets;
  - minimal docs.
- Add smoke command for release verification.
- Add crash/error-report policy for bad package and bad asset cases.
- Freeze package format and asset format for the release slice.

Exit criteria:
- A clean checkout can build, verify, package, and run the authored native 3D
  product slice.

## Verification Gates

Focused slice gate:

```sh
cmake -S engine -B engine/build
cmake --build engine/build --target <focused-target>
ctest --test-dir engine/build -R <focused-test-regex> --output-on-failure
git diff --check
```

Integration gate:

```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
git diff --check
```

Content package gate:

```sh
engine/build/iggy_scenario_toml_runner --check engine/content/demos/<package>
engine/build/iggy_scenario_toml_runner --trace engine/content/demos/<package>
engine/build/iggy_native_play --play engine/content/demos/<package> --scripted-controls <controls> --quit-after-script
```

## Near-Term Order

1. Commit/preserve the current product-loop demo package.
2. Define package asset manifest shape for `.igmesh` refs.
3. Make native runtime consume package-selected assets.
4. Add asset report/check output.
5. Add a stronger 3D-native demo package using package-selected assets.
6. Define native authoring tool shell choice after Qt deletion.
7. Build read-only package inspector/launcher.
8. Add mutation mode for package creation/editing.
9. Add save/load/completion UX.
10. Begin one gated gameplay semantic.
11. Add `.glb` import after `.igmesh` package/runtime path is reliable.
12. Delete Qt when native authoring and play paths replace it.

## Current Open Questions

These do not block the next implementation packets, but they must be answered
before the named phase starts.

- Phase 5: What exact non-Qt UI/windowing stack owns the authoring tool?
- Phase 7: What is the first scenario completion condition?
- Phase 8: Which gameplay semantic is first after pickup/door/NPC movement?
- Phase 9: What glTF material/texture subset is acceptable for the first import?
- Phase 11: What platform/package format is the first release target?

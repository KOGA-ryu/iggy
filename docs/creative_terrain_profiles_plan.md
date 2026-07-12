# Terrain Profile Stamp Plan

Status: implemented; retained as research and acceptance rationale

Verified against repository HEAD `c43e9db7696ad640ab8406841fa945aeb6db1e23`
on 2026-07-12.

## Goal

Add a creator-facing **Terrain Profile** tool that stamps useful elevations from
bounded mathematical profiles. The tool must make hills, basins, rings,
craters, ridges, directional waves, and radial ripples quickly without exposing
formula syntax or permitting unbounded trigonometric behavior.

The implementation must preserve the current terrain laws:

- Authored rods remain the only durable terrain source data.
- Preview and mutation consume one shared pure plan.
- One accepted stamp is one Facade batch, one document revision, one undo
  record, and one scene-cache refresh.
- Plans use fixed-capacity storage and reject before mutation.
- Runtime planning contains no `std::sin`, `std::cos`, allocation, or platform
  dependent floating-point decisions.
- Existing Rod, Grade, and Sculpt behavior remains source-compatible.

## Research Decisions

The following references shape the design. They are research references only;
no third-party implementation is copied.

1. [Minecraft Bedrock Terrain Tool](https://learn.microsoft.com/en-us/minecraft/creator/documents/bedrockeditor/editorterraintool?view=minecraft-bedrock-stable)
   separates mode, intensity, falloff, and radius, displays the affected area,
   and supports one-shot or drag application. Terrain Profile follows the same
   creator vocabulary but starts as one-shot only.
2. [Minecraft Bedrock Brush Tool](https://learn.microsoft.com/en-us/minecraft/creator/documents/bedrockeditor/editorbrushtool?view=minecraft-bedrock-stable)
   separates brush shape from what the brush applies. This plan keeps profile,
   blend, footprint, and rod policy as independent settings.
3. [Unreal Landscape Brushes](https://dev.epicgames.com/documentation/unreal-engine/landscape-brushes-in-unreal-engine?lang=en-US)
   and [Blender brush falloff](https://docs.blender.org/manual/en/latest/sculpt_paint/brush/falloff.html)
   use named geometric shapes and visible falloff boundaries instead of asking
   creators to author equations. The UI therefore says Hill, Basin, Ring,
   Crater, Ridge, Wave, and Ripple.
4. [WorldEdit generation](https://worldedit.enginehub.org/en/latest/usage/generation/)
   normalizes coordinates before evaluating shapes. Its pinned
   [image height-map brush](https://github.com/EngineHub/WorldEdit/blob/c9884dd09de0bf38f77555f1cad9f0f90174d00d/worldedit-core/src/main/java/com/sk89q/worldedit/command/tool/brush/ImageHeightmapBrush.java)
   and [height-map kernel](https://github.com/EngineHub/WorldEdit/blob/c9884dd09de0bf38f77555f1cad9f0f90174d00d/worldedit-core/src/main/java/com/sk89q/worldedit/math/convolution/HeightMap.java)
   also demonstrate the useful sample-plan-apply boundary. The repository's
   existing `Facade::applyTerrainControlEdits` is the correct apply boundary.
5. [NIST DLMF trigonometric functions](https://dlmf.nist.gov/4.16)
   makes the poles in secant, cosecant, and cotangent explicit. Raw reciprocal
   trig is excluded from creator profiles because finite inputs can approach an
   unbounded result.
6. [Shannon's sampling paper](https://webusers.imj-prg.fr/~antoine.chambert-loir/enseignement/2018-19/shannon/shannon1949.pdf)
   motivates an explicit sampling gate. This plan uses at least four lattice
   samples per oscillation, a stricter visual rule than the theoretical minimum.
7. [libfixmath](https://github.com/PetteriAimonen/libfixmath/blob/9318ccd9428114d318eb8703a7e40c152b9059c7/libfixmath/fix16_trig.c)
   confirms that fixed-point trig can use polynomial, cache, or lookup-table
   strategies. A general dependency and its large tables are unnecessary here;
   the terrain domain needs one small checked-in quarter-wave table.

WorldEdit is GPL-licensed and libfixmath is MIT-licensed. They remain design
references. The implementation below is repository-native and independently
specified.

## Product Boundary

Terrain Profile is a separate catalog tool, not another Sculpt mode.

- Rod paints or selects individual authored controls.
- Grade connects a chosen anchor to an endpoint.
- Sculpt repeatedly modifies existing rods with local editing modes.
- Profile computes a complete mathematical stamp and commits it atomically.

Combining Profile with Sculpt would overload press/hold history semantics,
empty-field behavior, base-height ownership, and tool options. The catalog can
hold more tools than the nine-slot hotbar, so a dedicated kind is the smaller
long-term contract.

The initial catalog entry is `Terrain Profile`. It is catalog/hotbar selectable
and is not inserted into the default tool wheel. This avoids changing the
existing nine-sector layout.

## Interaction Contract

The tool reuses semantic actions already mapped for keyboard, trackpad/mouse,
and PS5 controller. It adds no physical binding.

| Semantic action | Mouse / keyboard | PS5 | Result |
|---|---|---|---|
| Accept | Right click | X | Apply one previewed stamp on press |
| Pick | Middle click | Square | Lock the currently sampled base height |
| Reject | Left click | Circle | Return a locked base to automatic sampling |
| Quick previous / next | Up / down | D-pad up / down | Increase / decrease amplitude |
| Quick decrease / increase | Left / right | D-pad left / right | Decrease / increase radius |

Accept is deliberately press-only in this batch. Holding X or right click must
not repeat every 200 ms. Large overlapping mathematical stamps can cause both
accidental terrain amplification and avoidable scene rebuilds. Continuous
profile painting is a later profiling gate, not an implicit reuse of the Rod or
Sculpt repeat loop.

### Base height

Automatic base height is sampled from the pre-edit derived terrain at the aimed
center. If no derived terrain exists there, the tool uses the current terrain
HUD height.

- Square locks that sampled value while the creator aims elsewhere.
- Circle unlocks it and restores automatic sampling.
- Circle is a no-op while the base is already automatic.
- The action hint for Circle appears only while a base is locked.
- Document replacement and tool-state reset clear the lock.

### Tool options

The option panel has a hard capacity of eight rows. Terrain Profile consumes at
most eight:

| Option | Values | Default | Visibility |
|---|---|---|---|
| PROFILE | Hill, Basin, Ring, Crater, Ridge, Wave, Ripple | Hill | Always |
| BLEND | Set, Add | Set | Always |
| ROD POLICY | Fill, Existing | Fill | Always |
| RADIUS | 2, 4, 8 cells | 4 | Always |
| AMPLITUDE | 1, 2, 4, 8, 16 cells | 4 | Always |
| SPACING | 1, 2, 4 cells | 1 | Fill only |
| DIRECTION | +X, +X+Z, +Z, -X+Z, -X, -X-Z, -Z, +X-Z | +X | Ridge or Wave |
| FREQUENCY | 1, 2 cycles | 1 | Wave or Ripple |

The existing settings-aware option-list function must perform the conditional
filtering. Do not add an editor-side parallel option table.

`Set` produces `base + profile delta` and is idempotent when repeated with the
same settings. `Add` produces `pre-edit height + profile delta` and intentionally
compounds.

`Existing` edits only authored rods already inside the footprint. `Fill` merges
those rods with a canonical lattice and creates missing rods. Fill is the
default so the tool visibly works in an empty document.

New Fill rods use influence radius `2 * spacing`, yielding radii 2, 4, or 8.
Existing rods retain their authored radius. This avoids a ninth option and
keeps profile sampling density related to terrain influence density.

## Deterministic Math Contract

### Number formats

- Normalized coordinates and phase use signed Q16 values where 65,536 is 1.0.
- Trig output and profile weights use signed Q15 values where 32,767 is 1.0.
- Intermediate products use 64-bit integers.
- Every division or downshift uses an explicitly named nearest-rounding helper.
- Signed rounding is symmetric around zero.
- Final heights clamp to the authored range 1 through 64 cells.

### Shared brush kernel

Create `TerrainBrushKernel.*` for math shared by Sculpt and Profile. It owns:

- integer square root;
- squared-distance radius admission;
- Q16 radial distance derived from `sqrt(distanceSquared << 32)`;
- Q16 linear and smoothstep falloff;
- Q15 multiply and symmetric nearest rounding;
- an independently generated 65-entry Q15 quarter-wave sine table;
- `sinTurnsQ15` using quadrant mirroring and exact integer interpolation;
- `cosTurnsQ15` as a quarter-turn phase offset;
- a cosine bell on normalized `[0, 1]`.

One turn is 65,536 phase units. A quadrant is 16,384 phase units. The 65
quarter-wave entries cover both endpoints at a step of 256 phase units.
Interpolation must use the two neighboring entries and a denominator of 256.

The table is generated from the mathematical definition, checked into this
repository, and pinned by endpoint, symmetry, periodicity, monotonic-quadrant,
and checksum tests. It is not copied from libfixmath.

`TerrainSculpt.cpp` delegates its current integer square-root and falloff work
to this kernel. Existing Sculpt expected vectors must remain bit-identical; a
single changed vector is a STOP rather than an accepted approximation.

### Normalized coordinates

For candidate offset `(dx, dz)` and footprint radius `R`:

```text
q = clamp(sqrt(dx*dx + dz*dz) / R, 0, 1)
```

For directional profiles, an eight-row Q15 direction table supplies `(vx, vz)`.
Axis rows use `(32767, 0)` or `(0, 32767)`. Diagonals use independently rounded
`1/sqrt(2)` components. Dot products produce normalized along and across
coordinates without runtime floating point.

### Creator profiles

Let `bell(t) = (1 + cos(pi*t)) / 2` for `0 <= t <= 1`, and zero outside that
range. Every expression below is evaluated with the fixed-point kernel and
clamped to `[-1, 1]` before multiplying by amplitude.

| Profile | Signed unit profile | Creator result |
|---|---|---|
| Hill | `bell(q)` | Rounded mound with a flat boundary |
| Basin | `-bell(q)` | Rounded depression |
| Ring | `sin(pi*q)` | Raised annulus, zero at center and edge |
| Crater | `-bell(q/0.70) + 0.35*bell(abs(q-0.78)/0.22)` | Depressed bowl with a raised rim |
| Ridge | `bell(abs(across)/(R/3)) * bell(abs(along)/R)` | Oriented elongated crest |
| Wave | `sin(2*pi*f*((along/R + 1)/2)) * bell(q)` | Directional signed waves in a circular envelope |
| Ripple | `cos(2*pi*f*q) * bell(q)` | Concentric signed bands fading to the edge |

The crater constants are exact checked-in Q16 constants. Tests pin center,
inner bowl, rim, and boundary samples so later tuning is an intentional contract
change.

Secant, cosecant, and cotangent do not become raw profiles. Their poles conflict
with finite bounded plans and do not provide a useful creator distinction that
cannot be expressed as a bounded sharp or spherical curve. A future cliff or
spire tool must specify a bounded profile directly and earn its own visual and
overflow tests.

### Sampling gate

Oscillatory output is valid only when the Fill lattice can represent it:

```text
Wave:   2 * frequency * spacing <= radius
Ripple: 4 * frequency * spacing <= radius
```

This guarantees at least four samples per full oscillation along the relevant
domain. Wave and Ripple with `Existing` rod policy reject with
`SamplingUnproven`; arbitrary sparse rods do not prove a sampling interval.

An unsupported combination remains visible as a red preview with a short
reason such as `NEEDS SMALLER SPACING` or `USE FILL`. The planner, not the UI,
owns the final rejection law.

## Pure Planner Contract

Create `TerrainProfile.*` with repository-native enums and a fixed-layout plan.
The public shape is:

```cpp
enum class CreativeTerrainProfileKind : std::uint8_t {
  Hill, Basin, Ring, Crater, Ridge, Wave, Ripple, Count
};

enum class CreativeTerrainProfileBlend : std::uint8_t { Set, Add, Count };
enum class CreativeTerrainProfileRodPolicy : std::uint8_t {
  Fill, Existing, Count
};

struct CreativeTerrainProfileRequest {
  const CreativeTerrainField* field;
  CreativeTerrainCoord2 center;
  std::uint16_t baseHeightCells;
  CreativeTerrainProfileKind profile;
  CreativeTerrainProfileBlend blend;
  CreativeTerrainProfileRodPolicy rodPolicy;
  CreativeTerrainProfileDirection direction;
  std::uint16_t radiusCells;
  std::uint16_t amplitudeCells;
  std::uint16_t spacingCells;
  std::uint8_t frequency;
};

struct CreativeTerrainProfilePlan {
  CreativeTerrainProfilePlanStatus status;
  std::array<CreativeTerrainControlEdit,
             kCreativeTerrainControlCapacity> edits;
  std::uint16_t candidateCount;
  std::uint16_t editCount;
  // items() exposes only the initialized prefix.
};
```

The final header may use value enums for radius, amplitude, spacing, direction,
and frequency in settings while resolving them to the request's integer fields.
Do not place UI or history state in the pure planner.

### Candidate enumeration

1. Validate every enum, range, canonical control ordering, and control point.
2. Reject coordinate arithmetic that would overflow 32-bit terrain coordinates.
3. Enumerate lattice coordinates in canonical Z-then-X order for Fill.
4. Merge the lattice stream with existing in-footprint controls in O(n + a),
   deduplicating equal coordinates. Existing policy consumes only the existing
   stream.
5. Count candidates and prospective new rods before emitting edits.
6. Reject if candidates, edits, or the resulting field exceed 256 controls.
7. Compute all outputs from the same pre-edit snapshot.
8. Emit only semantic changes. Never emit a partial prefix on rejection.

For Set, every candidate starts from the sampled center base. For Add, an
existing candidate starts from its authored height. A missing Fill candidate
starts from the pre-edit derived height at that coordinate when available, then
falls back to the center base.

The signed profile delta is amplitude multiplied by Q15 profile weight with
symmetric nearest rounding. A zero delta may still create a missing Fill rod at
its baseline height because Fill explicitly authors the sampled lattice.

### Statuses

The plan status must distinguish at least:

- `Ready`
- `NoChange`
- `NoControlsInBrush`
- `InvalidRequest`
- `CoordinateOverflow`
- `CapacityExceeded`
- `SamplingUnproven`
- `UnderSampled`

Editor reason codes map one-to-one to these statuses. Do not infer rejection
from an empty edit span.

## Preview, Apply, And History

`CreativeTerrainProfileState` lives under `CreativeEditorTerrainState`. It owns
only editor-lifetime interaction state:

- automatic or locked base-height state;
- last sampled base height;
- a revision-keyed preview cache;
- the last profile action receipt.

The preview cache key includes:

- document ID and terrain revision;
- aimed center;
- automatic/locked base and resolved base height;
- profile, blend, rod policy, radius, amplitude, spacing, direction, frequency;
- grid origin and cell size if they are not already guaranteed by the enclosing
  scene cache.

Preview construction:

1. Build the exact pure profile plan.
2. Copy the bounded terrain field.
3. Apply the plan to the copy only.
4. Build the existing terrain render plan from that copy.
5. Draw exact planned rods and surface edges through editor overlay data.

Existing authored rods remain cyan. Changed or new accepted rods are green.
A rejected plan draws its footprint and reason in red without exposing a
partial candidate prefix. The accepted preview uses the same slope-color policy
as Sculpt. Aiming and option changes never alter room geometry, document
revision, save data, or room geometry signature.

Accept rebuilds the plan from the current document, then calls
`Facade::applyTerrainControlEdits` once. The history transaction is one-shot:
begin immediately before the accepted Facade call, commit only when the receipt
changed the document, and cancel on rejection or no change. There is no profile
stroke state in this batch.

After an accepted stamp the existing scene cache observes the single terrain
revision change and rebuilds once. Do not add direct renderer uploads or manual
cache invalidation.

Preview is hidden while a catalog, tool options, clipboard preview, capture
mode, or another modal owns interaction. Switching tools clears only ephemeral
profile preview/base-lock state; it does not mutate the document.

## File Ownership

### Create

- `src/app/iggy3d/creative/tools/TerrainBrushKernel.hpp`
  - Fixed-point brush math declarations only.
- `src/app/iggy3d/creative/tools/TerrainBrushKernel.cpp`
  - Integer sqrt, falloff, Q15/Q16 operations, direction rows, and the 65-entry
    quarter-wave table.
- `src/app/iggy3d/creative/tools/TerrainProfile.hpp`
  - Profile enums, request, statuses, fixed-capacity plan, value resolvers, and
    `toString` declarations.
- `src/app/iggy3d/creative/tools/TerrainProfile.cpp`
  - Validation, canonical merge, sampling gate, profile evaluation, and plan
    construction.
- `apps/iggy3d_creative/EditorTerrainProfile.cpp`
  - Base lock, one-shot history/apply adapter, preview cache, and overlay.
- `tests/unit/creative_terrain_profile_tests.cpp`
  - Pure kernel and planner contract tests.

### Modify

- `CMakeLists.txt`
  - Register both engine sources and `EditorTerrainProfile.cpp`.
- `cmake/iggy3d_tests.cmake`
  - Register `creative_terrain_profile_tests`.
- `src/app/iggy3d/creative/tools/TerrainSculpt.cpp`
  - Delegate existing radial fixed-point math to `TerrainBrushKernel`; preserve
    every current output.
- `src/app/iggy3d/creative/tools/Tools.hpp`
  - Include profile declarations, add eight option IDs/settings fields, and
    widen `CreativeHeldItemMask` from `std::uint16_t` to `std::uint32_t`.
    The current 16 held kinds already consume every bit; Profile is kind 17.
- `src/app/iggy3d/creative/tools/Tools.cpp`
  - Add profile descriptors, conditional option filtering, validation, labels,
    cycling, equality, and 32-bit applicability-mask handling.
- `src/app/iggy3d/creative/input/Interaction.hpp`
  - Add `CreativeHeldItemKind::TerrainProfile` immediately after TerrainSculpt.
- `src/app/iggy3d/creative/input/Interaction.cpp`
  - Add the string/parse/material-use rows without changing physical bindings.
- `src/app/iggy3d/creative/input/Catalog.cpp`
  - Add the `Terrain Profile` catalog spec and search aliases. Match the other
    terrain tools' initial tool-wheel eligibility.
- `apps/iggy3d_creative/EditorTerrain.hpp`
  - Add profile state/receipt declarations under the existing terrain cluster.
- `apps/iggy3d_creative/EditorTerrain.cpp`
  - Admit TerrainProfile to the common terrain overlay and shared aim/reset
    lifecycle, then delegate profile-specific work.
- `apps/iggy3d_creative/EditorInteraction.cpp`
  - Add ordered behavior/handler rows, press-only apply, base lock/unlock,
    quick-edit labels, status text, and interruption/reset handling.
- `apps/iggy3d_creative/EditorActionHints.cpp`
  - Add Apply profile, Lock base, conditional Auto base, Amplitude, and Radius
    hints through semantic action IDs.
- `apps/iggy3d_creative/EditorToolOptions.cpp`
  - Synchronize profile quick-edit state after an option commit; do not create a
    second descriptor table.
- `tests/unit/creative_tools_tests.cpp`
  - Pin defaults, eight-row maximum, conditional rows, labels, adjustment,
    invalid enums, and the widened held-item mask.
- `tests/unit/creative_interaction_tests.cpp`
  - Pin kind string/parse and generic hotbar behavior.
- `tests/unit/creative_catalog_tests.cpp`
  - Pin catalog discovery, search aliases, and eligibility.
- `tests/unit/creative_editor_action_hints_tests.cpp`
  - Pin mouse/keyboard and PS5 semantic hints, including conditional unlock.
- `tests/unit/creative_editor_controls_tests.cpp`
  - Pin tool-wheel preference round trip for the seventeenth held-item kind.
- `tests/unit/creative_editor_terrain_tests.cpp`
  - Pin preview/apply/history/cache/base-lock/modal behavior.
- `tests/unit/creative_terrain_field_tests.cpp`
  - Keep Sculpt parity vectors and add shared-kernel parity coverage where it
    best fits the existing terrain tests.
- `docs/creative_terrain.md`
  - Replace the plan link with the implemented interaction and algorithm
    contract only after the complete feature passes.

### Boundary Notes

- `apps/iggy3d_creative/EditorPreviewFrame.cpp` changes only at the existing
  terrain-overlay call site to forward `captureMode`. The common terrain
  overlay previously had no way to honor the required capture hiding law.

The following surfaces remain unchanged:

- `render/`, `src/render/`, and Vulkan pipeline files
  - Profile preview uses existing editor overlays; no new render pipeline.
- `src/app/iggy3d/creative/document/TerrainField.*`
  - Existing field, sampler, render planner, and capacity remain authoritative.
- Save schema, replay, runtime physics, AI, and room-asset formats
  - The tool creates ordinary terrain-control edits and no new durable type.
- Physical keyboard/controller binding tables
  - Existing semantic actions are sufficient.

If implementation proves any no-change surface must change, STOP and revise
this plan before widening scope.

## Build Batches

### Batch A: deterministic kernel and complete pure planner

Create `TerrainBrushKernel.*`, `TerrainProfile.*`, and
`creative_terrain_profile_tests.cpp`. Rewire Sculpt to the shared math and
register the sources/tests. Implement all seven profiles, both blends, both rod
policies, canonical merge, capacity checks, and sampling rejection.

Batch A acceptance:

- No UI or held-item enum changes.
- Existing Sculpt vectors are bit-identical.
- Every profile has exact center/interior/boundary golden vectors.
- Axis and diagonal direction vectors are pinned.
- Set is idempotent; Add compounds from a pre-edit snapshot.
- Fill works from an empty field; Existing never creates rods.
- Capacity and coordinate failures emit zero edits.
- No runtime call to `std::sin`, `std::cos`, or heap allocation.

### Batch B: complete creator integration

Add the held kind, widen the option mask, register catalog/settings/options,
add `EditorTerrainProfile.cpp`, and wire semantic actions, preview, history,
hints, and documentation. This is one coherent integration batch because enum
order, descriptor order, handler order, and tests are compile-time coupled.

Batch B acceptance:

- The tool is discoverable from the catalog and assignable to the hotbar.
- X/right click applies once per press; held frames do not repeat.
- Square locks base and Circle restores automatic base.
- D-pad adjusts amplitude and radius without stealing flight controls because
  quick-edit admission remains context-owned.
- Tool options never exceed eight visible rows.
- Preview and applied plan are identical.
- One accepted stamp creates one undo step and one terrain revision.
- Reject/no-change/invalid stamps create no history or revision.
- Preview-only aiming causes no room scene rebuild or geometry-signature change.

## Test Matrix

### Kernel

- Sine/cosine anchors at 0, quarter, half, and three-quarter turns.
- Negative phase and multi-turn periodicity.
- Odd/even symmetry and monotonic first quadrant.
- Interpolation around every quadrant boundary.
- Quarter-wave table checksum.
- Q15 multiply and signed nearest-rounding ties.
- Q16 radial distance at axis, 3-4-5, diagonal, edge, and outside points.
- Existing Uniform, Linear, and Smooth Sculpt vectors unchanged.

### Planner

- Hill, Basin, Ring, Crater, Ridge, Wave, and Ripple sample vectors.
- Eight directional rows and diagonal mirror symmetry.
- Every radius, amplitude, spacing, and frequency value.
- Set repeat is no change; Add repeat changes again.
- Empty Fill succeeds; empty Existing reports no controls.
- Existing rods keep radius; new rods derive radius from spacing.
- Existing/lattice merge is canonical and deduplicated.
- Derived-height fallback for missing Add candidates.
- Min/max height clamping.
- Under-sampled Wave/Ripple and Existing oscillation rejection.
- Invalid enums, unsorted controls, invalid controls, coordinate overflow,
  candidate overflow, edit overflow, and final field overflow.
- All rejected plans expose an empty initialized span.

### Editor integration

- Catalog label/search/selection and held-kind parse round trip.
- Settings defaults and conditional option rows at capacity eight or lower.
- Mouse/keyboard and PS5 action hints.
- Press applies; held and release frames do not apply again.
- Automatic base follows aim; locked base remains stable; Circle unlocks.
- Preview hidden by catalog, options, capture, and tool switch.
- Preview cache hits on idle/unchanged aim and misses on every key field.
- Preview copied field equals the post-apply real field.
- Accepted stamp: one Facade batch, revision, undo entry, and scene refresh.
- No-change/rejection: zero revision, history, or scene refresh.
- Undo and redo restore the exact pre/post terrain fields.
- Document replacement clears lock/cache without mutating either document.

## Verification Commands

Run headlessly. Do not launch `i3dc` automatically.

```sh
cmake -S . -B build
cmake --build build --target \
  creative_terrain_profile_tests \
  creative_terrain_field_tests \
  creative_tools_tests \
  creative_interaction_tests \
  creative_catalog_tests \
  creative_editor_action_hints_tests \
  creative_editor_controls_tests \
  creative_editor_terrain_tests \
  iggy3d_creative
ctest --test-dir build -R \
  '^(creative_terrain_profile_tests|creative_terrain_field_tests|creative_tools_tests|creative_interaction_tests|creative_catalog_tests|creative_editor_action_hints_tests|creative_editor_controls_tests|creative_editor_terrain_tests)$' \
  --output-on-failure
git diff --check
```

Finish Batch B with one manual mouse/trackpad and PS5 check only after the
headless gate is green:

1. Empty document Hill/Fill preview and apply.
2. Lock base, move aim, apply a second Hill at the locked elevation.
3. Switch to Basin/Add and verify intentional compounding.
4. Select an invalid Ripple sampling combination and verify red non-mutating
   feedback.
5. Undo and redo each accepted stamp once.

## Stop Conditions

Stop before commit or scope expansion if any of these occur:

- Shared-kernel extraction changes an existing Sculpt expected output.
- Terrain Profile requires more than eight visible tool-option rows.
- A plan needs more than the existing 256 terrain controls or partial edits.
- Preview and mutation require separate geometry/profile calculations.
- A physical input binding must change or conflicts with flight/menu ownership.
- Held-kind insertion breaks generic string persistence or tool-wheel parsing.
- Adding the seventeenth held kind reveals another 16-bit mask or serialized
  ordinal assumption beyond the explicitly planned `CreativeHeldItemMask`.
- Runtime trig requires `std::sin`, `std::cos`, NaN handling, or an external
  fixed-point dependency.
- Renderer, Vulkan, save schema, runtime physics, AI, or room-asset formats must
  change.
- The existing terrain capacity must increase.
- Continuous profile stamping appears necessary before a profiling and user
  interaction pass proves a safe cadence.

These are plan-correction events, not invitations to improvise a broader
implementation.

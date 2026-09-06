# Prompt 01: Audit the Existing Asset and Live Material Route

Execute this prompt against the package and target asset selected in Stage 00.
This is a read-first stage. Diagnose before repairing.

## Required reading

- active package `WORKSTREAM.md`;
- package documentation, profile, patterns, scripts, tests, and manifests;
- owning department guidance;
- relevant target-asset profile and build scripts;
- `workflow/07_proof_and_acceptance/AGENTS.md` so the audit knows what later
  evidence must exist.

## Inspect the actual asset

Record:

- real dimensions and transforms;
- source geometry and evaluated geometry;
- silhouette-defining features;
- modifiers and Geometry Nodes;
- UV maps and named attributes;
- material slots and live assigned material;
- face, component, end, cut, or fracture identities;
- current coordinate source;
- behavior after resize, mirror, Boolean, or curve deformation;
- whether the target asset already exists and must not be rebuilt.

Do not identify materials only from a render. Separate wood, metal, shadow,
opening, cavity, and background through geometry and material inspection.

## Inspect the current material route

Trace:

`profile -> pattern or motif -> generator -> maps -> Blender builder -> node
group -> assigned material -> proof geometry -> saved asset -> target engine`

For each link, record:

- canonical owner;
- live consumer;
- duplicate or stale route;
- missing contract;
- current verification.

## Inspect visible quality

Open or render the latest:

- unlit base color;
- neutral geometry;
- hero material view;
- close view;
- grazing view;
- distance view;
- tiling view;
- actual-asset view.

Describe visible failures in specific language:

- pattern too regular;
- one shade per element;
- weak medium structure;
- microdetail everywhere;
- no rest areas;
- geometry hidden by color;
- repeated landmarks;
- stretched scale;
- linework reads as arbitrary cracks;
- roughness has no material story;
- shader ignores authored masks;
- damage appears in impossible locations.

Do not use "looks bad" or "needs more detail" as the diagnosis.

## Compare code claims with live behavior

Look for mismatches such as:

- map exists but shader does not consume it;
- profile says metre scale but node uses generated position or bounding box;
- material claims twenty shades but each element uses one;
- damage defaults are documented off but rendered on;
- proof claims saved asset but tests only in-memory state;
- Blender material is called engine-ready without engine parity;
- geometry semantics disappear after evaluation.

## Define the first failing observable

Record one acceptance failure that the current capability must repair. When
possible, add or identify a focused red test before implementation.

The failing observable must be visible or contract-bearing, for example:

- grain does not deflect around the approved knot;
- tool marks remain visible as distance noise;
- Boolean-created end faces inherit side grain;
- metal scale and exposed iron share one physical response;
- strand twist swims after curve deformation;
- one tile landmark repeats in every fourth block.

## Dossier update

Populate:

- existing state;
- live route;
- known visible failures;
- canonical owners and conflicts;
- first failing observable;
- candidate proving test;
- current proof paths.

Set Stage 01 to `complete` only after inspecting the live asset and current
proofs. Use `verified-existing` only when the package already documents and
proves the same audit in current artifacts.

## Exit gate

Do not advance unless:

- actual target geometry is identified;
- live material assignment is identified;
- coordinate and semantic-attribute routes are known;
- current visual failures are concrete;
- code/document claims are compared with live behavior;
- the first failing observable and proving gate are named;
- no implementation has been started merely to make the audit feel productive.

Then execute `02_reference_research.md`.

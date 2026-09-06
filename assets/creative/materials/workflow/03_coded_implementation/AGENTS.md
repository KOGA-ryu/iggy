# Stage 3: Coded Implementation Specification

This stage sits between research intent and implementation. Its purpose is to
prevent a correct-sounding plan from becoming shallow code.

Every implementation demand must document the exact texture data, geometry
semantics, Blender nodes, shader behavior, production code, tests, and proof.
The actual executable code belongs directly beneath the demand it satisfies.
Do not hide the code in a later appendix or replace it with pseudocode.

## Canonical artifact

Create or update `<material-package>/CODED_DEMANDS.md`.

Use `workflow/prompts/CODED_DEMAND_TEMPLATE.md` as the required structure. The
document becomes the reviewed implementation blueprint for the active
capability.

The code files remain canonical after implementation. `CODED_DEMANDS.md`
preserves why each code change exists and what exact demand it satisfies.

When a capability embeds complete files and an executed Blender graph, use
`workflow/scripts/render_coded_demands.py` after implementation to synchronize
the reviewed demands with the final source, node defaults, and exact links.
Use its `--check` mode at handoff. The renderer is a drift detector; it does
not replace writing and reviewing the coded demands before Prompt 05.

## Demand granularity

One coded demand should produce one reviewable visible or semantic result:

- one texture lane or coordinated texture family;
- one local-frame or semantic-attribute contract;
- one material layer;
- one distance hierarchy;
- one stylization lane;
- one damage family;
- one proof or validation boundary.

Do not create one demand called "make the complete material." Do not split one
coherent shader flow into dozens of socket-sized demands.

## Required demand structure

Every demand contains these sections in this order:

1. `Demand`: visible outcome and exact acceptance statement.
2. `Authority`: source observations, measurement IDs, proxy limits, and
   authored decisions.
3. `Textures`: complete map inventory affected by the demand.
4. `Geometry semantics`: attributes, domains, units, defaults, and
   evaluated-geometry behavior.
5. `Nodes`: exact Blender node and group inventory.
6. `Shader flow`: coordinates, sampling, decode, physical response,
   stylization, distance behavior, and output.
7. `Test code`: complete focused red/green tests.
8. `Production code`: complete executable functions or exact patch hunks.
9. `Build and validation`: exact commands and expected assertions.
10. `Proof`: isolated and final visible evidence.

Place Test code and Production code immediately beneath the demand. Do not
defer either to "implementation later".

## Texture inventory

For every affected texture, document:

- stable texture identifier and filename;
- visual or physical meaning;
- source layers and masks;
- physical span or longitudinal repeat in metres;
- resolution and metres per texel;
- bit depth;
- sRGB or Non-Color interpretation;
- channel meanings;
- range and zero encoding;
- seam and extension behavior;
- filtering;
- per-element coordinate, phase, rotation, and mirror behavior;
- distance behavior;
- Blender consumer node;
- target-engine consumer;
- focused test.

The inventory must reveal missing maps, duplicated meanings, incorrect packing,
and dormant textures before code is written.

## Node inventory

For every new or changed node, document:

- node group and version;
- exact node name;
- Blender `bl_idname`;
- operation, blend mode, interpolation, or data type;
- each non-default input and its unit;
- source node and socket for every incoming link;
- destination node and socket for every outgoing link;
- map, attribute, or profile field it represents;
- reason it exists;
- focused validation assertion.

Use socket names after verifying them against the installed Blender version.
Do not rely on numeric socket indices when a stable name exists.

## Shader contract

Document the complete material path:

`coordinate -> element transform -> texture sample -> channel decode ->
physical or optical layer -> normal and roughness combination -> stylization
and distance controls -> BSDF or material-layer blend -> material output`

Record:

- public group interface;
- coordinate units and axes;
- all named attributes;
- base-color composition;
- height decode and metre amplitude;
- normal convention and combination;
- roughness composition;
- AO use;
- metalness or physical layer identity;
- opacity when relevant;
- ink, highlight, direction, and detail-priority use;
- damage and overlay defaults;
- camera-distance or footprint behavior;
- Blender custom properties;
- target-engine function mapping;
- parity status.

A node list without this end-to-end flow is incomplete.

## Code placement

Inside each demand, use this order:

1. complete test additions or replacements;
2. profile, pattern, or data changes;
3. generator changes;
4. Blender-builder changes;
5. validation and manifest changes;
6. proof-scene changes;
7. exact commands.

Every code block names its target path and insertion or replacement location.

Acceptable:

- a complete new function with imports and called helpers already available;
- a complete replacement function;
- an exact patch hunk with enough context to apply safely;
- a complete JSON object or array fragment with its parent key;
- a complete unittest method.

Reject:

- `...`;
- `pass`;
- `TODO`;
- pseudocode;
- "add logic here";
- a function that calls undefined helpers;
- code that omits error handling or validation required by the demand;
- a node screenshot instead of builder code;
- tests described in prose but not scripted.

## Python generator quality

Production code must:

- use explicit SI-unit variable names such as `_m`, `_m2`, or
  `_per_m2`;
- load scale-bearing values from the profile or authored pattern;
- use deterministic local random generators with recorded seeds;
- keep authored pattern data separate from compilation;
- validate array shapes, finite values, and legal ranges;
- preserve local element frames;
- band-limit subpixel features;
- avoid accidental quadratic work over large images;
- avoid global mutable state;
- report manifest metrics and source hashes;
- write maps through the canonical shared image utilities;
- keep base color, physical PBR lanes, and stylization separable.

Use type hints for public helpers and structured records where they reduce
ambiguity.

## Blender builder quality

Builder code must:

- inspect the installed Blender API instead of assuming an old node layout;
- use stable named nodes and socket names;
- set color spaces explicitly;
- keep object references rather than relying on ambient selection;
- use `bpy.data` APIs when an operator is unnecessary;
- isolate any required operator context;
- assign geometry attributes on the correct domain and data type;
- fail clearly when a required map, socket, attribute, or object is absent;
- pack required images;
- validate links, custom properties, and prohibited nodes;
- save and reopen the final `.blend`;
- keep proof geometry and runtime material ownership separate.

Do not use generic Noise or Voronoi nodes as substitutes for authored material
structure.

## Test quality

Write the test code before the production code in each demand.

Tests must verify final semantics:

- physical measurements and scales;
- map formats and channel meaning;
- deterministic variation;
- local-frame and semantic-attribute continuity;
- exact node names, types, operations, socket links, and defaults;
- live consumption of authored maps;
- physical layer and stylization defaults;
- saved-file packing and hash;
- reopen behavior;
- absence of unsupported damage, noise, or parity claims.

Avoid tests that merely repeat constants from the implementation. Load the
canonical profile and manifest when they own the expected value.

## Code review before execution

Before applying any block:

1. trace every symbol and helper;
2. verify file paths and current function context;
3. inspect Blender node socket availability when relevant;
4. check units and color space;
5. check array shape and memory cost;
6. verify determinism;
7. verify the code cannot touch unrelated assets;
8. verify the test fails for the intended reason;
9. verify the proof will expose visual failure;
10. update the coded demand if implementation reality differs.

Do not let the live code silently drift away from the document. Update the
demand first, then apply the revised block.

## Stage exit gate

Do not begin the main map or shader implementation until:

- every in-scope demand has an ID;
- complete texture inventory exists;
- complete node and shader inventories exist;
- exact geometry semantics exist;
- test code sits under every demand;
- production code sits under every demand;
- no code block contains placeholders or undefined planned helpers;
- commands and proofs are specified;
- code review checks are recorded.

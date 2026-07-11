# Future Custom Object Authoring

Status: inert roadmap note. This is not an active implementation plan and must
not expand current Creative feature scope.

## Product Intent

Add a separate, approachable 3D object-authoring workspace later. It should let
players compose simple primitives, mould their dimensions, attach child shapes,
apply bounded array or pattern operations, preview the result, and save it as a
reusable Creative catalog object. Examples include pillars, stairs, dressers,
and swords.

The interaction target is closer to Minecraft creative tooling than Blender:
the common path must be discoverable with a controller, while precision options
remain contextual instead of occupying permanent bindings.

## Boundary

- The object editor is a separate editor mode/window, not another always-live
  viewport overlay.
- It may reuse selection, transform, snap, history, standard UI widgets, and
  pure pattern planners such as Linear and Radial Array.
- Editing works on a temporary object-definition document. The active map
  document changes only when a saved definition is placed as an instance.
- A saved definition is immutable from the map's perspective. Editing the
  definition later requires explicit version/update behavior; silent mutation
  of placed maps is forbidden.
- Preview and construction state never enters map save data, room geometry,
  receipts, or runtime object IDs.

## Future Data Shape

- Stable definition ID and schema version.
- Bounded primitive/child records in deterministic order.
- Per-child local transform and material reference.
- Explicit attachment/parent relationships with cycle rejection.
- Precomputed local bounds, render proxy, collision proxy, and placement anchor.
- One catalog entry that places an instance without flattening the source graph
  unless an explicit bake/export operation requests it.

## Required Decisions Before Implementation

- Instance versus baked-copy semantics when a definition changes.
- Allowed primitive set and whether boolean union/subtract is necessary.
- Collision generation, material slots, origin/pivot editing, and scale limits.
- File format, migration policy, asset portability, and missing-definition UI.
- Complexity budgets for child count, nesting depth, generated geometry, and
  pattern expansion.

## Algorithm Validation Queue

- Deterministic hierarchy evaluation and cycle detection.
- Array/pattern expansion limits and transform parity.
- Bounds and collision parity after nested non-uniform transforms.
- Mesh merge, deduplication, and index-overflow handling if baking is added.
- Save/load round trips, schema migration, stable hashing, and dependency repair.

Do not implement this feature opportunistically while extending current map
tools. Promote this note into a complete work plan only when object authoring
becomes an explicit milestone.

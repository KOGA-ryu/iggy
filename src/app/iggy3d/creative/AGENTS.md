# Creative lane — folder map

Creative code lives under one roof here. The `creative/` root holds the public
surface (`Facade`, `CreativeAppState`, `Core.hpp`, `State.hpp`); everything else
is grouped by concern:

- `document/` — the CreativeDocument data model: `Object`, `ObjectDescriptor`,
  `Block`, `Document`, `DocumentMutation`, `DocumentSnap`, `DocumentWireframe`.
- `mutation/` — the mutation grammar: `Mutation`, `MutationApply`, `Metrics`.
- `tools/` — interaction tools: `Tools`, `Select`, `Measure`, `Placement`, `Palette`.
- `spatial/` — geometry/preview: `SpatialProjection`, `ViewportPick`, `Snap`, `Ghost`.
- `ui/` — the creative UI model & draw: `Ui`, `UiDrawList`, `UiProjection`, `UiFrame`.
- `bridge/` — window-frame plumbing: `InputFrame`, `UiInputFrame`, `UiCommandFrame`,
  `UiWindowFrame`, `ViewportPickFrame`, `WireframeFrame`, `WindowCoordinateSpace`.
- `world/` — world lifecycle & save: `WorldService`, `DocumentSection`.
- `render/` — `WireframeDebugLines`. `camera/` — `Fly`. `adapters/` — reserved
  0-byte bake sockets (L5), leave dormant.

Filenames below are basenames; find them in the folder above. Includes are
full-path (`app/iggy3d/creative/<folder>/<Name>.hpp`).

# Creative Object Kind Pattern

Object kinds are DATA ROWS, not per-kind code paths. The receipted generic
create/remove/mutation pipeline is the only way objects change; the per-kind
`Commands.hpp` / `Document::create<Kind>` / `Facade::create<Kind>` pattern is
RETIRED (deleted under TD-3 — see `docs/creative_mode/creative_tooling_v1.md`).
To activate the next kind:

1. Add or expose the kind in `Object.hpp`, then update `Object.cpp` so
   `toString` and the correct category helper know about it. The shared object
   shape stays `id`, `kind`, `name`, `transform`, `bounds`, `layerId`,
   `visible`, `locked`, `tags`, and optional `parentId`.
2. Fill the kind's descriptor row in `ObjectDescriptor.cpp`: profile,
   shapeKind, projection, occupancy, creation dirty flags, defaults, and the
   capability booleans (`hasTransform`, `hasBounds`, `canHaveParent`, ...).
   A new per-kind fact is a DESCRIPTOR COLUMN, never a switch (foundation D6).
3. Verify `allowedMutations(kind)` in `Mutation.cpp` lists exactly the verbs
   that write real fields for this kind — never a verb that would return
   "no stored object field yet".
4. Creation goes through `Facade::createDocumentObject(kind)` or
   `createDocumentObject(CreativeDocumentCreateRequest)` (overrides are
   capability-gated by the descriptor). Removal goes through
   `removeDocumentObject` (refuses locked objects). Renames go through
   `renameDocumentObject`. All three are receipted; there is no other path.
5. Add focused unit coverage: descriptor row invariants
   (`creative_object_descriptor_tests`), create receipt + defaults
   (`creative_document_create_tests`), mutation/lock behavior
   (`creative_document_mutation_tests`), and wireframe projection if the kind
   is visible.

Do not generalize beyond the kind being activated. Leave future kinds dormant
until they have the same descriptor/create/mutation/test loop. UI palette
exposure is a separate UI-dex slice gated on the tooling contract (TL-7:
only real verbs reach the UI).

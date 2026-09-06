# Material Catalog Consumer Snapshot

`MATERIAL_CATALOG.json` is a generated, read-only snapshot of the authoritative
Surface Foundry registry at `~/surface-foundry/catalog/materials.json`. Do not
hand-edit the JSON or use this Markdown file as a second decision database.

Regenerate the snapshot from Surface Foundry with:

```sh
PYTHONPATH=src python3 -m surface_foundry export-workbench-snapshot \
  --iggy3d-root /Users/kogaryu/iggy3d \
  --output /Users/kogaryu/iggy3d/assets/creative/materials/MATERIAL_CATALOG.json
```

## Current presentation shelf

The snapshot exposes one current owner per independent material scope. Status
is part of the identity and may not be omitted.

| Current scope | Status |
| --- | --- |
| Structural oak growth-volume timber | user accepted |
| Structural oak clean-cut joinery | user accepted |
| Reference-derived structural oak door | not accepted |
| Layered intact forged iron | not accepted |
| Quiet intact lime-plaster finish | component donor |
| Giant-house plaster–masonry transition | production candidate |
| Measured cathedral ashlar core | production candidate |
| Cathedral chevron-voussoir trim | production candidate |
| Three-strand rope and plant fibre | production candidate |
| Actual-consumer clean arming-sword steel | production candidate |
| C04 full-length macro-polish component | component candidate |
| Master Sword cyan-steel response | component candidate |

The Master Sword colour study and its Inkblotter evidence are registered under
the Sword Steel family even though their files live in Surface Foundry.

## Workbench and archive

The JSON contains three explicit projections:

- `lineage_views.current`: the only default presentation shelf;
- `lineage_views.workbench`: active research and component evidence;
- `lineage_views.archive`: rejected, superseded, and prototype evidence.

The dark cumulative hinge, cat-face/diamond sweep, oxide experiments, rejected
oxide repair, diagnostic sword fixture, finite squiggly blade finish, and
graphic Master Sword closure live in the archive. They may be inspected as
history but may never substitute for a current entry.

Before presenting or consuming a material, resolve it through
`lineage_views.current`. Modification time and output-folder proximity carry no
authority.

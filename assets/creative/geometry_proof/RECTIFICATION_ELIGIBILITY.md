# Reference eligibility for perspective rectification

Phase 7 deliverable: which reference classes can feed the proof system
directly, which need perspective rectification first, and which cannot be
rectified at all. fSpy (or any rectification tool) stays OUT of the
production path; this document only records eligibility so future profile
work does not rediscover it per image.

## Criteria

A photographic reference is eligible for rectification only if ALL hold:

1. **Usable parallel lines** - at least two independent sets of real-world
   parallel edges long enough to fix vanishing points (frame members, shelf
   edges, panel joints). Organic or worn silhouettes alone disqualify.
2. **A known dimension or ratio** on the plane of interest, or the output is
   shape-only (usable for proportions, like Paley fig 7, never for size).
3. **The profile plane is flat and visible** - rectification recovers a
   plane; a moulding's section is only recoverable when the photo shows a
   true cut end or a straight-on raking view of the profile.
4. **Modest lens distortion** - visible barrel/pincushion on the reference
   lines disqualifies unless the tool corrects it first.

## Classification of the current reference families

| family | class | verdict |
|---|---|---|
| Paley, Manual of Gothic Mouldings plates | engraved orthographic sections | **no rectification needed** - direct spec authoring; per-figure scale only where a dimension line exists (fig 6 yes at 22.09 px/in; fig 7 none - shape only) |
| Brandon, Open Timber Roofs plates | engraved orthographic sections + described scantlings | **no rectification needed**; printed scantlings give real dimensions (pl IX tie-beam 20 x 14 in) |
| Architecture book plates (`docs/blender/reference_manifests/architecture_book_plates_v1.tsv`) | mixed engravings | **no rectification needed** for section/profile figures; perspective VIGNETTES in the same books are shape-inspiration only, NOT profile sources |
| Viollet-le-Duc article figures | mixed: sections, elevations, perspective cuts | sections/elevations: direct. His perspective cutaways: **ineligible** as measurement sources (artistic projection, inconsistent vanishing) |
| Museum object photography (Met wall harvest, weapons/instruments/furniture manifests) | photographs, mostly 3/4 views | **per-piece assessment**. Furniture with straight rails/stiles frequently passes criterion 1; carved profiles fail criterion 3 unless a true end/section view exists. Catalogued dimensions usually satisfy criterion 2 |
| Museum photography: swords, knives, tools | photographs, mostly plan-view flat lays | flat lays are already near-orthographic for the SILHOUETTE plane - **rectification unnecessary for outline work**, ineligible for cross-sections (criterion 3: sections are not visible in plan) |
| Half-timber / vernacular survey photographs (`ancienthalftimbe00habe`, `halftimberhousei00jack`, ...) | architectural photographs | facades frequently eligible (abundant parallel timber lines, published bay dimensions); moulding profiles within them **ineligible** - too small, criterion 3 fails |

## Standing rule

When a profile's only source is a photograph that fails these criteria, the
correct move is to find a drawn section (the architecture-book corpus is
deep) or author from the member vocabulary with evidence class `AUTHORED` -
never to trace a perspective image and call it measured.

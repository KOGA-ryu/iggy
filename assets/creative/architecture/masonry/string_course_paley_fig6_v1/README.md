# SINC_StringCourse_PaleyFig6

The FIRST production consumer of a SINC_GeometryProof_Profile donor - the
Phase 6 contract, exercised: this asset consumes the reference-verified
`paley_pl1_fig6_v1` spec through `profile_compiler.compile_profile()` and
authors NO geometry of its own. The moulded run (arris chamfer, 267-degree
bowtell, cavetto hollow, upper chamfer) and the stock closure are the donor's;
this package only chooses a sweep length.

Provenance is enforced, not asserted: the manifest and the asset object both
record the donor's sha256, and `tests/unit/string_course_paley_fig6_blend_tests.py`
recomputes it from the current donor spec - editing the donor without
rebuilding this consumer turns the suite red. The saved mesh is verified to be
exactly two welded rings of the compiled outline (nothing added, nothing
re-authored).

## Reproduction

```
/Applications/Blender.app/Contents/MacOS/Blender --background \
    --python assets/creative/architecture/masonry/string_course_paley_fig6_v1/build_string_course_paley_fig6_v1.py
python3 tests/unit/string_course_paley_fig6_blend_tests.py
```

`--length-m N` for other run lengths; `--skip-render` to skip the preview.

## Boundary

Geometry only: no materials (cathedral_stone_v1 and friends are the material
lane), no wear, no LODs, no mitres/returns/terminations - a straight run with
end caps. Terminations are joint-class work and stay outside until
`SINC_GeometryProof_Joint` exists.

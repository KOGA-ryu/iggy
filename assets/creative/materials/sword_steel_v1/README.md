# Sword Steel v1

`sword_steel_v1` is the smallest reusable bright worked-steel derivative of
the accepted forged-iron conductor donors. It proves a pristine ground blade,
not a complete sword and not a condition family.

## Capability

- one exposed ferrous conductor;
- body, fuller, bevel, and edge semantic regions supplied by geometry;
- one broad authored macro-polish field;
- one finite longitudinal medium-grind field;
- region-specific macro strength, grind strength, roughness, and anisotropy;
- micrometre-scale medium-finish bump with unresolved microstructure owned by
  the anisotropic BSDF;
- no oxide, patina, rust, pitting, scratches, chips, nicks, temper colour,
  engraving, blood, or other history.

The package now has two explicit consumers:

- `SM_WPN001_BladeSteelFixture` remains the analytic diagnostic implementation
  of the WPN-001 contract. Its Met envelope is only a declared proxy.
- `arming_sword_v1_clean_steel` installs the same accepted graph on the exact
  factory-generated BATMAN arming-sword blade: 0.780 m long, 0.048 m shoulder
  width, 0.006 m shoulder thickness, lenticular section, secondary bevel, and
  no fuller. Its actual-asset proof package is under
  `output/arming_sword_v1_consumer/`.

It also has one explicitly non-production geometry study:

- [`HISTORICAL_SWORDS_SET_STUDY.md`](HISTORICAL_SWORDS_SET_STUDY.md) audits a
  hash-locked CC BY 3.0 four-sword donor as construction evidence. Its 20 exact
  model cross sections expose paired taper, fuller termination, forte-plane
  transitions, and a constant-thickness negative control. No donor material or
  texture enters this package, and none of the four models is accepted as a
  production or historically measured sword.
- [`HISTORICAL_SWORDS_CLEAN_STEEL_RESPONSE.md`](HISTORICAL_SWORDS_CLEAN_STEEL_RESPONSE.md)
  applies the unchanged canonical graph to the four audited blades. The
  diagnostic proves that a real fuller and section transitions change the
  highlight response, while the LongSword negative control stays uniform and
  the ArmingSword exposes its missing edge land. It creates no new material
  candidate or accepted donor.

## Prompt chain

The reusable planning and execution chain begins at
[`prompts/README.md`](prompts/README.md). It separates consumer intake,
geometry and UV ownership, clean polish, blade-identity branches, decoration,
optional condition, engine translation, and actual-asset acceptance. The
prompts are a durable workbench and review ledger; the builder remains the
executable authority.

## Build

```bash
python3 -m unittest tests.unit.sword_steel_v1_tests
python3 -m unittest tests.unit.arming_sword_v1_clean_steel_tests
python3 -m unittest tests.unit.sword_historical_donor_study_tests
python3 -m unittest tests.unit.sword_historical_response_board_tests
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/sword_steel_v1/build_sword_steel_v1.py
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/sword_steel_v1/build_sword_steel_v1.py -- \
  --validate-only
python3 -m unittest tests.unit.sword_steel_v1_tests
```

The actual-consumer `.blend` is produced on BATMAN with Blender 5.1.1 because
the measured factory lives there. The current whole-sword factory finish
script is broken on a missing grip `zone` attribute, so the comparison
reconstructs only its documented old blade baseline and leaves other parts as
neutral context. Blender proof does not establish Unreal anisotropy parity.

The next construction gate is an owned fullered blade generator with explicit
section stations, independent width/thickness taper, fuller termination, and a
real edge land. The same clean-steel graph should then be applied unchanged.

# Prompt 02: Clean Steel and the Sword-Polish Stack

## Objective

Build the reusable intact steel foundation before construction-specific
patterns, decoration, oxidation, or damage.

## Surface decomposition

### Component A: conductor identity

- Meaning: exposed ferrous or declared blade conductor.
- Owner: one physical metal BSDF.
- Outputs: conductor base reflectance and metalness.
- Rest rule: no photographed lighting, AO, oxide colour, or diffuse-grey
  substitute.

### Component B: macro polish organization

- Meaning: broad variation left by separate polishing access, pressure, plane
  orientation, and finish grade.
- Scale: broad passages, not scratches or blobs.
- Owner: one authored low-frequency scalar field plus semantic region gains.
- Output: roughness only unless a source supports a value shift.
- Rest rule: preserve quiet passages and never form symbols or repeated
  landmarks.

### Component C: medium grinding evidence

- Meaning: finite abrasive passes visible in a close view.
- Direction: follows the documented working direction, normally
  shoulder-to-tip for the shared clean foundation.
- Owner: an authored finite-track field.
- Outputs: restrained roughness and micrometre normal response through
  different remaps.
- Rest rule: absent from silhouette and base colour; reduced on the cutting
  edge unless the reference proves otherwise.

### Component D: unresolved microstructure

- Meaning: sub-texel scratch population that creates directional reflection.
- Preferred owner: BSDF anisotropy and the blade tangent, because scratches
  below the map Nyquist limit should not become aliased stripes.
- Output: anisotropic microfacet response.
- Rest rule: it may not create visible painted lines at gameplay distance.

### Component E: regional finish grades

Body, fuller, bevel, edge, ricasso, and tip may have different roughness,
anisotropy, macro strength, and medium-track strength. The region attribute
owns the selection; the finish maps do not rediscover it from position.

## Strategy alternatives

### Macro polish

1. **Authored control lattice:** manually authored low-frequency values
   interpolated over blade-local U/V. Preferred for deterministic scripting.
2. **Hand-painted broad field:** painted on the actual blade with the same
   semantic region gains. Preferred for a hero consumer after UV lock.

### Medium grinding

1. **Finite scripted track vocabulary:** explicit pass centres, widths,
   pressures, interruptions, and envelopes.
2. **Reference-derived directional mask:** de-lit and cleaned from a licensed
   or internally authored finish sample, then scaled in metres.

### Microresponse

1. **Anisotropic BSDF:** preferred shared solution.
2. **Band-limited detail normal:** only if the target resolution and camera can
   resolve it without moire.

## Channel rules

- Base colour: conductor identity and only supported broad tint.
- Metalness: one for intact exposed metal.
- Roughness: region baseline plus independently weighted macro and medium
  fields.
- Normal: medium relief only; microresponse stays in the BRDF unless proven
  resolvable.
- AO: zero on open blade planes.
- Anisotropy: region-specific strength with the verified blade tangent.

## Adversarial proof

Render neutral, grazing, rotated-light, close, and gameplay views. Isolate the
macro field, medium field, region mask, and final roughness. Reject white-bar
lighting, black-plastic metal, painted stripes, equal-frequency noise, and a
finish that survives as texture crawl at gameplay distance.

## Exit gate

The clean foundation passes only when the conductor reads without decoration,
the macro and medium bands are separately visible in isolation, and the
microresponse is produced without an aliased texture pattern.

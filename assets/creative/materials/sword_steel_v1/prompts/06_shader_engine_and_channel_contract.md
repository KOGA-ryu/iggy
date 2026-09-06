# Prompt 06: Shader, Engine, and Channel Contract

## Objective

Assemble accepted sword components once, preserve their ownership, and define
the Blender and target-engine adapters.

## Canonical shader flow

```text
blade-local UV and semantic attributes
-> conductor identity
-> regional finish baselines
-> macro polish roughness
-> medium grind roughness and normal
-> anisotropic microresponse
-> selected blade-identity fork
-> optional decoration and stylization
-> separately switched condition
-> one physical material output
```

## Blender contract

Record exact node names, `bl_idname` values, socket names, links, color spaces,
image packing, tangent source, normal combination order, and prohibited nodes.
Every image and Attribute node must reach the material output or an explicitly
declared diagnostic material.

## Runtime material interface

Expose semantic parameters rather than raw graph internals:

- substrate preset;
- body/fuller/bevel/edge roughness;
- anisotropy and tangent rotation;
- macro and medium finish strengths;
- identity-fork enable and parameters;
- stylization strength;
- condition family switches;
- distance fade thresholds.

## Channel packing

Retain independent source maps before packing. For a BG3-style adapter:

- Base Map: intrinsic base colour, mostly grayscale when gradient recolouring
  is desired;
- Normal Map: baked and authored tangent normal;
- Physical Map R: metalness;
- Physical Map G: roughness;
- Physical Map B: AO;
- optional gradient masks for base and accent colour families.

For Unreal, document the exact material-function inputs, texture compression,
normal convention, tangent behavior, mip strategy, and per-instance controls.
Blender proof does not imply Unreal parity.

## Performance contract

Record texture count, dimensions, bit depth, packed memory, image-sample count,
BSDF count, expensive node count, and shader switches. A hero proof may use
more diagnostics than the runtime material, but dormant production routes are
forbidden.

## Exit gate

The complete graph contains only accepted components, every lane is live,
packed channels remain independently inspectable at source, and parity claims
match executed engine evidence.

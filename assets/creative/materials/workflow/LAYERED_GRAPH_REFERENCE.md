# Layered Material Graph Reference

## Source boundary

This note translates the user-supplied vertical bark-graph screenshot into
text. The supplied image is 125 by 1536 pixels. Its node labels and numeric
settings are not legible, so this is a visual-structure reading, not a claim
about exact Substance Designer node types or values.

## What is visibly established

- The target is demonstrated on both a vertical trunk and a sphere.
- Bark and weathered-wood reference strips are compared near the top.
- The graph is not one linear noise chain. It contains many bounded branches
  that are previewed as grayscale masks or height fields before convergence.
- Several branches create long vertical organization at different widths.
- Other branches break, erode, warp, or interrupt those long forms.
- High-contrast masks isolate cavities, ridges, fibres, flakes, and sparse
  events instead of applying every operation across the whole surface.
- Height-like grayscale construction precedes the late colour network.
- The late colour network blends several muted grey, tan, brown, and cool
  families through existing masks.
- The final output retains broad vertical plates and rests while fine fibres
  sit inside them. Fine detail does not replace the larger bark anatomy.

## Inferred method, clearly separated from the visible evidence

The visible graph topology is consistent with a professional layered
procedural method:

1. establish a large directional height field;
2. split it into broad bark plates or fibre families;
3. distort those families with lower-frequency directional warps;
4. derive ridge, cavity, broken-edge, and protected-rest masks;
5. add medium fibres and sparse cross-direction events through those masks;
6. add microstructure only inside eligible regions;
7. combine height bands with explicit amplitude priority;
8. derive or combine normal response from the authorized relief;
9. build colour from the same physical identities, with additional broad
   colour variation;
10. build roughness from exposed layer, fibre, porosity, and cavity cause;
11. inspect the result on both a neutral curved object and the intended
   directional form.

This inference must be tested against a readable source graph before copying a
specific node sequence.

## Workflow consequences

Every material package must keep a layer-provenance ledger in its profile.
Each visible layer records:

- physical meaning;
- source pattern, measurement, or inherited lane;
- output map, geometry, or semantic mask;
- downstream shader or geometry consumer;
- isolated proof;
- rest or absence rule;
- supporting measurement-claim IDs.

The package auditor rejects a hero or reusable material when construction,
macro, medium, edge/event, micro, cumulative colour, height/normal, response,
or stylization provenance is missing. This prevents a large node graph from
being mistaken for a layered material merely because it contains many nodes.


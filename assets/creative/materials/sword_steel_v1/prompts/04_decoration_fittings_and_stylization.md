# Prompt 04: Decoration, Fittings, and Stylization

## Objective

Add authored marks and gameplay value organization without confusing them with
blade construction or condition.

## Decoration ownership

For each engraving, rune, maker mark, fuller inscription, inlay, gilding, or
painted symbol record:

- historical, concept, or production source;
- exact face and orientation;
- physical method: cut, punched, etched, inlaid, gilded, painted, or unknown;
- depth or explicit authored translation;
- affected material lanes;
- whether the mark repeats across variants;
- whether it must survive at gameplay distance.

Choose geometry/high-to-low transfer for recesses whose sidewalls or shadows
matter. Choose an authored mask for shallow etching or inlay. Never draw a
bright symbol into base colour and call it engraving.

## Fittings boundary

Guard, habaki, pommel, wire, leather, wood, ray skin, lacquer, and grip wrap
are separate material consumers. Reuse shared material donors; do not bake
their identities into the blade shader.

## Stylized PBR lane

The BOTW/Arcane/BG3-adjacent lane may provide:

- broad cool/warm value grouping;
- one large, one medium, and one small reflection emphasis;
- restrained region-selected edge linework;
- grayscale gradient masks for engine colour variants;
- distance-aware suppression of finish detail.

It may not provide baked lighting, universal curvature whitening, black cavity
outlines, or equal emphasis on every edge.

## Strategy alternatives

1. authored vector/paint masks registered to the UV and semantic regions;
2. geometry-derived candidates corrected by hand-authored eligibility and rest
   masks.

## Proof

Show decoration by itself, cumulative material, neutral lighting, game-like
lighting, and thumbnail distance. Confirm that turning stylization off leaves
a coherent physical sword.

## Exit gate

Every mark has a source and construction method. Fittings remain separate.
Stylization improves hierarchy without becoming fake illumination.

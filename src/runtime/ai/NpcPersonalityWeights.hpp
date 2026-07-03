#pragma once

namespace iggy3d {

// Multiplicative per-factor weights for the L6 guard-decision score (A5). The A9 deck cards land
// HERE (map: "personality weight seams reserved from day one"). ALL NEUTRAL (1.0) in v1 -- the
// kernel multiplies by them, but every shipped profile passes neutral, so they are invisible to
// today's behavior. Consuming non-neutral values is A9's job.
struct NpcPersonalityWeights {
  float suspicionWeight = 1.0F;
  float strategicWeight = 1.0F;
  float travelWeight = 1.0F;
  float allyWeight = 1.0F;
};

}  // namespace iggy3d

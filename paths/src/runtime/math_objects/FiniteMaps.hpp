#pragma once
#include <array>

namespace paths {
// Total maps on nonempty labelled sets of at most four elements. Indices are
// zero-based. Inactive array entries are ignored. Weights are finite and >= 0.
struct FiniteMapsInput {
  unsigned a=4,b=4,c=4;
  std::array<unsigned,4> f{0,1,2,3},g{0,1,2,3},h{0,1,2,3};
  std::array<double,4> weights{1,1,1,1};
  unsigned fiber=0;
};
struct FiniteMapsAnalysis {
  std::array<unsigned,4> fiberMasks{},fiberSizes{},composed{},classOf{};
  unsigned imageSize=0,collisions=0,agreement=0;
  bool injective=false,surjective=false,bijective=false,commutes=false;
  bool probabilityDefined=false,conditionalDefined=false;
  double eventProbability=0;
  std::array<double,4> sourceMass{},outputMass{},composedMass{},conditionalSource{},conditionalOutput{};
};
[[nodiscard]] FiniteMapsAnalysis analyzeFiniteMaps(const FiniteMapsInput&);
} // namespace paths

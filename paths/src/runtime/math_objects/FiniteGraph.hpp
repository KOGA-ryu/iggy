#pragma once
#include <array>
#include <cstdint>

namespace paths {
// Fixed labelled nodes 0..5; bit 6*i+j is the directed relation i -> j.
using FiniteRelation=std::uint64_t;
constexpr FiniteRelation relationEdge(unsigned i,unsigned j){return FiniteRelation{1}<<(6*i+j);}
struct RelationWitness {std::array<unsigned,3> nodes{};unsigned count=0;};
struct FiniteGraphAnalysis {
  unsigned edgeCount=0,reachableCount=0,classCount=0;
  bool reflexive=true,symmetric=true,transitive=true,equivalence=false;
  std::array<RelationWitness,3> witnesses{}; // Reflexivity, symmetry, transitivity.
  std::array<int,6> distance{},parent{};
  std::array<unsigned,6> classOf{};
};
[[nodiscard]] FiniteGraphAnalysis analyzeFiniteGraph(FiniteRelation,unsigned source);
struct BipartiteMatching {
  unsigned maximumSize=0;
  // Nine bits: left i -> right j is bit 3*i+j. State 0 is empty.
  std::array<unsigned,4> states{};
  // Node IDs in alternating paths: left 0..2, right 3..5.
  std::array<std::array<unsigned,6>,3> paths{};
  std::array<unsigned,3> pathLengths{};
  unsigned deficientLeft=0,neighbors=0,deficiency=0;
};
[[nodiscard]] unsigned bipartiteEdges(FiniteRelation);
[[nodiscard]] BipartiteMatching analyzeBipartiteMatching(unsigned edges);
} // namespace paths

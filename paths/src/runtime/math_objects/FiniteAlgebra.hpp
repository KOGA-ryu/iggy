#pragma once
#include <array>
namespace paths {
// Active entries use a fixed stride of six. Inactive entries are ignored.
struct FiniteOperation {
  unsigned size=2;
  std::array<unsigned,36> values{};
  unsigned operator()(unsigned a,unsigned b) const { return values[6*a+b]; }
};
struct FiniteAlgebraAnalysis {
  bool associative=true,commutative=true,group=false;
  int identity=-1;
  std::array<int,6> inverse{};
  std::array<unsigned,3> associativityWitness{};
  std::array<unsigned,2> commutativityWitness{};
  // For each failed identity candidate e: an x and side (0=e*x, 1=x*e).
  std::array<unsigned,6> identityWitness{},identitySide{};
};
struct FiniteSubgroup {
  bool defined=false,normal=false;
  unsigned mask=0,order=0,cosetCount=0;
  std::array<unsigned,7> powers{}; // e,g,...,g^order=e
  std::array<unsigned,6> cosetOf{};
};
struct ModularRingAnalysis {
  unsigned size=2;
  std::array<int,6> inverse{},zeroDivisorWitness{};
  unsigned unitCount=0,zeroDivisorCount=0;
  bool field=false;
};
FiniteOperation modularOperation(unsigned size,bool multiplication=false);
FiniteOperation kleinFourOperation();
// Indices: identity, (12), (01), (012), (021), (02).
// Product a*b applies b first, then a, using lexicographic permutation images.
FiniteOperation trianglePermutationOperation();
FiniteAlgebraAnalysis analyzeFiniteAlgebra(const FiniteOperation&);
FiniteSubgroup generatedFiniteSubgroup(const FiniteOperation&,unsigned generator,bool rightCosets=false);
ModularRingAnalysis analyzeModularRing(unsigned size);
}

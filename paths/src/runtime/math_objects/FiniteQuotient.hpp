#pragma once
#include "runtime/math_objects/FiniteAlgebra.hpp"
namespace paths {
// A partition operation can have one class even though input groups have 2..6
// elements. -1 marks an ambiguous product; resultMasks retain ALL possibilities.
struct FinitePartitionOperation {
  unsigned classCount=0;
  std::array<unsigned,6> classOf{},members{},representative{};
  std::array<int,36> product{};
  std::array<unsigned,36> resultMasks{};
  bool wellDefined=false;
  std::array<unsigned,4> witness{}; // a,b,a',b' in the same two input classes
};
struct FiniteHomomorphism {
  bool valid=false,injective=false,surjective=false;
  unsigned kernel=0;
  std::array<unsigned,2> witness{};
  unsigned mappedProduct=0,productOfImages=0;
  std::array<unsigned,6> image{}; // image element indexed by fiber class
  FinitePartitionOperation fibers;
};
enum class FiniteSubsetFailure { None, MissingIdentity, Product, Inverse };
struct FiniteGroupQuotient {
  bool subgroup=false,normal=false;
  FiniteSubsetFailure failure=FiniteSubsetFailure::None;
  std::array<unsigned,3> subsetWitness{};
  FinitePartitionOperation quotient;
};
struct FiniteRing {
  FiniteOperation addition,multiplication;
};
struct FiniteRingQuotient {
  bool additiveSubgroup=false,ideal=false;
  FiniteSubsetFailure additiveFailure=FiniteSubsetFailure::None;
  std::array<unsigned,3> additiveWitness{},absorptionWitness{}; // r,i,r*i
  FinitePartitionOperation addition,multiplication;
};
FiniteHomomorphism analyzeFiniteHomomorphism(const FiniteOperation& source,const FiniteOperation& target,const std::array<unsigned,6>& map);
FiniteGroupQuotient analyzeFiniteGroupQuotient(const FiniteOperation&,unsigned subset);
FiniteRingQuotient analyzeFiniteRingQuotient(const FiniteRing&,unsigned subset);
// 0..4 = Z/2Z through Z/6Z; 5 = F2 x F2; 6 = F2[e]/(e^2).
FiniteRing finiteRingExample(unsigned family);
}

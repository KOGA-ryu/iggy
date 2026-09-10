#include "runtime/math_objects/FiniteQuotient.hpp"
#include <bit>
#include <stdexcept>
namespace paths {
namespace {
FiniteAlgebraAnalysis requireGroup(const FiniteOperation& t){const auto a=analyzeFiniteAlgebra(t);if(!a.group)throw std::invalid_argument("quotient input must be a group");return a;}
void subsetCheck(unsigned n,unsigned mask){if(mask>>n)throw std::invalid_argument("subset contains an inactive element");}
FinitePartitionOperation partitionOperation(const FiniteOperation& t,const std::array<unsigned,6>& masks,unsigned count){
  FinitePartitionOperation r;r.classCount=count;r.members=masks;r.classOf.fill(6);r.representative.fill(6);r.product.fill(-1);
  for(unsigned c=0;c<count;++c)for(unsigned i=0;i<t.size;++i)if(masks[c]&(1U<<i)){r.classOf[i]=c;if(r.representative[c]==6)r.representative[c]=i;}
  r.wellDefined=true;
  for(unsigned x=0;x<count;++x)for(unsigned y=0;y<count;++y){const unsigned a=r.representative[x],b=r.representative[y],expected=r.classOf[t(a,b)];
    for(unsigned i=0;i<t.size;++i)for(unsigned j=0;j<t.size;++j)if((masks[x]&(1U<<i))&&(masks[y]&(1U<<j))){const unsigned actual=r.classOf[t(i,j)];r.resultMasks[6*x+y]|=1U<<actual;if(r.wellDefined&&actual!=expected){r.wellDefined=false;r.witness={a,b,i,j};}}
    if(std::popcount(r.resultMasks[6*x+y])==1)r.product[6*x+y]=static_cast<int>(expected);
  }return r;
}
}
FiniteHomomorphism analyzeFiniteHomomorphism(const FiniteOperation& s,const FiniteOperation& t,const std::array<unsigned,6>& f){
  (void)requireGroup(s);const auto target=requireGroup(t);for(unsigned i=0;i<s.size;++i)if(f[i]>=t.size)throw std::invalid_argument("map destination outside target");
  FiniteHomomorphism r;std::array<unsigned,6> masks{};unsigned count=0;
  for(unsigned y=0;y<t.size;++y){unsigned mask=0;for(unsigned x=0;x<s.size;++x)if(f[x]==y)mask|=1U<<x;if(mask){r.image[count]=y;masks[count++]=mask;}}
  r.fibers=partitionOperation(s,masks,count);r.injective=count==s.size;r.surjective=count==t.size;r.valid=true;
  for(unsigned a=0;a<s.size;++a)for(unsigned b=0;b<s.size;++b)if(r.valid&&f[s(a,b)]!=t(f[a],f[b])){r.valid=false;r.witness={a,b};r.mappedProduct=f[s(a,b)];r.productOfImages=t(f[a],f[b]);}
  if(r.valid)for(unsigned i=0;i<s.size;++i)if(f[i]==unsigned(target.identity))r.kernel|=1U<<i;
  return r;
}
FiniteGroupQuotient analyzeFiniteGroupQuotient(const FiniteOperation& t,unsigned mask){
  const auto a=requireGroup(t);subsetCheck(t.size,mask);FiniteGroupQuotient r;
  if(!(mask&(1U<<a.identity))){r.failure=FiniteSubsetFailure::MissingIdentity;r.subsetWitness={unsigned(a.identity),unsigned(a.identity),unsigned(a.identity)};return r;}
  for(unsigned x=0;x<t.size;++x)if(mask&(1U<<x)){
    for(unsigned y=0;y<t.size;++y)if((mask&(1U<<y))&&!(mask&(1U<<t(x,y)))){r.failure=FiniteSubsetFailure::Product;r.subsetWitness={x,y,t(x,y)};return r;}
    if(!(mask&(1U<<a.inverse[x]))){r.failure=FiniteSubsetFailure::Inverse;r.subsetWitness={x,unsigned(a.identity),unsigned(a.inverse[x])};return r;}
  }
  r.subgroup=true;std::array<unsigned,6> masks{};unsigned seen=0,count=0;
  for(unsigned x=0;x<t.size;++x)if(!(seen&(1U<<x))){unsigned coset=0;for(unsigned h=0;h<t.size;++h)if(mask&(1U<<h))coset|=1U<<t(x,h);masks[count++]=coset;seen|=coset;}
  r.quotient=partitionOperation(t,masks,count);r.normal=r.quotient.wellDefined;return r;
}
FiniteRing finiteRingExample(unsigned family){
  if(family>6)throw std::invalid_argument("unknown finite ring example");
  if(family<5)return {modularOperation(family+2),modularOperation(family+2,true)};
  FiniteRing r{kleinFourOperation(),kleinFourOperation()};
  for(unsigned a=0;a<4;++a)for(unsigned b=0;b<4;++b)r.multiplication.values[6*a+b]=family==5?(a&b):((a&1)*(b&1))|((((a&1)*(b>>1))^((a>>1)*(b&1)))<<1);
  return r;
}
FiniteRingQuotient analyzeFiniteRingQuotient(const FiniteRing& ring,unsigned mask){
  const auto& add=ring.addition;const auto& mul=ring.multiplication;const auto a=requireGroup(add);const auto m=analyzeFiniteAlgebra(mul);
  if(add.size!=mul.size||!a.commutative||!m.associative||m.identity<0)throw std::invalid_argument("input must be a unital ring");
  for(unsigned x=0;x<add.size;++x)for(unsigned y=0;y<add.size;++y)for(unsigned z=0;z<add.size;++z)if(mul(x,add(y,z))!=add(mul(x,y),mul(x,z))||mul(add(x,y),z)!=add(mul(x,z),mul(y,z)))throw std::invalid_argument("ring distributivity failed");
  const auto h=analyzeFiniteGroupQuotient(add,mask);FiniteRingQuotient r;r.additiveSubgroup=h.subgroup;r.additiveFailure=h.failure;r.additiveWitness=h.subsetWitness;if(!h.subgroup)return r;
  r.addition=h.quotient;r.multiplication=partitionOperation(mul,h.quotient.members,h.quotient.classCount);r.ideal=true;
  for(unsigned x=0;x<add.size;++x)for(unsigned i=0;i<add.size;++i)if(r.ideal&&(mask&(1U<<i))&&(!(mask&(1U<<mul(x,i)))||!(mask&(1U<<mul(i,x))))){r.ideal=false;r.absorptionWitness=!(mask&(1U<<mul(x,i)))?std::array<unsigned,3>{x,i,mul(x,i)}:std::array<unsigned,3>{i,x,mul(i,x)};}
  return r;
}
}

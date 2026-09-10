#include "runtime/math_objects/FiniteAlgebra.hpp"
#include <algorithm>
#include <stdexcept>
namespace paths {
namespace {
void sizeCheck(unsigned n){if(n<2||n>6)throw std::invalid_argument("finite algebra size must be 2..6");}
void validate(const FiniteOperation& t){sizeCheck(t.size);for(unsigned i=0;i<t.size;++i)for(unsigned j=0;j<t.size;++j)if(t(i,j)>=t.size)throw std::invalid_argument("operation is not closed on the active set");}
}
FiniteOperation modularOperation(unsigned n,bool multiplication){sizeCheck(n);FiniteOperation t;t.size=n;for(unsigned a=0;a<n;++a)for(unsigned b=0;b<n;++b)t.values[6*a+b]=(multiplication?a*b:a+b)%n;return t;}
FiniteOperation kleinFourOperation(){FiniteOperation t;t.size=4;for(unsigned a=0;a<4;++a)for(unsigned b=0;b<4;++b)t.values[6*a+b]=a^b;return t;}
FiniteOperation trianglePermutationOperation(){
  constexpr std::array<std::array<unsigned,3>,6> p{{{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}}};
  FiniteOperation t;t.size=6;for(unsigned a=0;a<6;++a)for(unsigned b=0;b<6;++b){std::array<unsigned,3> image{};for(unsigned i=0;i<3;++i)image[i]=p[a][p[b][i]];t.values[6*a+b]=static_cast<unsigned>(std::find(p.begin(),p.end(),image)-p.begin());}return t;
}
FiniteAlgebraAnalysis analyzeFiniteAlgebra(const FiniteOperation& t){
  validate(t);FiniteAlgebraAnalysis r;r.inverse.fill(-1);r.identityWitness.fill(6);r.identitySide.fill(2);
  for(unsigned e=0;e<t.size;++e){bool identity=true;for(unsigned x=0;x<t.size;++x){if(t(e,x)!=x||t(x,e)!=x){identity=false;r.identityWitness[e]=x;r.identitySide[e]=t(e,x)!=x?0:1;break;}}if(identity)r.identity=static_cast<int>(e);}
  for(unsigned a=0;a<t.size;++a)for(unsigned b=0;b<t.size;++b){
    if(r.commutative&&t(a,b)!=t(b,a)){r.commutative=false;r.commutativityWitness={a,b};}
    if(r.identity>=0&&t(a,b)==static_cast<unsigned>(r.identity)&&t(b,a)==static_cast<unsigned>(r.identity)&&r.inverse[a]<0)r.inverse[a]=static_cast<int>(b);
    for(unsigned c=0;c<t.size;++c)if(r.associative&&t(t(a,b),c)!=t(a,t(b,c))){r.associative=false;r.associativityWitness={a,b,c};}
  }
  r.group=r.associative&&r.identity>=0;for(unsigned a=0;a<t.size;++a)r.group=r.group&&r.inverse[a]>=0;return r;
}
FiniteSubgroup generatedFiniteSubgroup(const FiniteOperation& t,unsigned g,bool right){
  const auto a=analyzeFiniteAlgebra(t);if(g>=t.size)throw std::invalid_argument("invalid generator");FiniteSubgroup r;r.cosetOf.fill(6);if(!a.group)return r;r.defined=true;r.powers[0]=static_cast<unsigned>(a.identity);
  unsigned x=r.powers[0];do{r.mask|=1U<<x;x=t(x,g);r.powers[++r.order]=x;}while(x!=r.powers[0]);
  r.normal=true;for(unsigned representative=0;representative<t.size;++representative){unsigned left=0,rightMask=0;for(unsigned h=0;h<t.size;++h)if(r.mask&(1U<<h)){left|=1U<<t(representative,h);rightMask|=1U<<t(h,representative);}r.normal=r.normal&&left==rightMask;
    if(r.cosetOf[representative]==6){const unsigned mask=right?rightMask:left;for(unsigned y=0;y<t.size;++y)if(mask&(1U<<y))r.cosetOf[y]=r.cosetCount;++r.cosetCount;}}
  return r;
}
ModularRingAnalysis analyzeModularRing(unsigned n){
  sizeCheck(n);ModularRingAnalysis r;r.size=n;r.inverse.fill(-1);r.zeroDivisorWitness.fill(-1);
  for(unsigned a=0;a<n;++a){for(unsigned b=0;b<n;++b){if(a*b%n==1)r.inverse[a]=static_cast<int>(b);if(a&&b&&a*b%n==0&&r.zeroDivisorWitness[a]<0)r.zeroDivisorWitness[a]=static_cast<int>(b);}r.unitCount+=r.inverse[a]>=0;r.zeroDivisorCount+=r.zeroDivisorWitness[a]>=0;}r.field=r.unitCount==n-1;return r;
}
}

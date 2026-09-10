#include "runtime/math_objects/FiniteMaps.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
FiniteMapsAnalysis analyzeFiniteMaps(const FiniteMapsInput& in) {
  if(in.a<1||in.a>4||in.b<1||in.b>4||in.c<1||in.c>4||in.fiber>=in.b)throw std::invalid_argument("finite map set size or fiber out of range");
  FiniteMapsAnalysis out;
  for(unsigned j=0;j<in.b;++j)if(in.g[j]>=in.c)throw std::invalid_argument("g leaves its codomain");
  double maximum=0;
  for(unsigned i=0;i<in.a;++i){
    if(in.f[i]>=in.b||in.h[i]>=in.c)throw std::invalid_argument("map leaves its codomain");
    if(!std::isfinite(in.weights[i])||in.weights[i]<0)throw std::invalid_argument("invalid outcome weight");
    out.fiberMasks[in.f[i]]|=1U<<i;++out.fiberSizes[in.f[i]];
    out.composed[i]=in.g[in.f[i]];out.agreement+=out.composed[i]==in.h[i];
    maximum=std::max(maximum,in.weights[i]);
  }
  for(unsigned j=0;j<in.b;++j)if(out.fiberSizes[j]){
    for(unsigned i=0;i<in.a;++i)if(in.f[i]==j)out.classOf[i]=out.imageSize;
    ++out.imageSize;
  }
  out.collisions=in.a-out.imageSize;out.injective=out.imageSize==in.a;
  out.surjective=out.imageSize==in.b;out.bijective=out.injective&&out.surjective;
  out.commutes=out.agreement==in.a;out.probabilityDefined=maximum>0;
  if(!out.probabilityDefined)return out;
  double total=0;for(unsigned i=0;i<in.a;++i)total+=in.weights[i]/maximum;
  for(unsigned i=0;i<in.a;++i){const double p=(in.weights[i]/maximum)/total;
    out.sourceMass[i]=p;out.outputMass[in.f[i]]+=p;out.composedMass[out.composed[i]]+=p;
  }
  out.eventProbability=out.outputMass[in.fiber];
  // Normalize the restricted raw weights separately: a positive event can be
  // tiny relative to the population, without making its conditional law zero.
  double eventMaximum=0;for(unsigned i=0;i<in.a;++i)if(in.f[i]==in.fiber)eventMaximum=std::max(eventMaximum,in.weights[i]);
  out.conditionalDefined=eventMaximum>0;
  if(out.conditionalDefined){double eventTotal=0;for(unsigned i=0;i<in.a;++i)if(in.f[i]==in.fiber)eventTotal+=in.weights[i]/eventMaximum;
    for(unsigned i=0;i<in.a;++i)if(in.f[i]==in.fiber){out.conditionalSource[i]=(in.weights[i]/eventMaximum)/eventTotal;out.conditionalOutput[in.fiber]+=out.conditionalSource[i];}
  }
  return out;
}
} // namespace paths

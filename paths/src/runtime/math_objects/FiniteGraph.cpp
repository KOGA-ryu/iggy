#include "runtime/math_objects/FiniteGraph.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace paths {
FiniteGraphAnalysis analyzeFiniteGraph(FiniteRelation relation,unsigned source){
  if(relation>>36||source>=6)throw std::invalid_argument("finite relation out of range");
  FiniteGraphAnalysis out;out.edgeCount=std::popcount(relation);out.distance.fill(-1);out.parent.fill(-1);
  const auto has=[&](unsigned i,unsigned j){return (relation&relationEdge(i,j))!=0;};
  for(unsigned i=0;i<6;++i){
    if(!has(i,i)&&out.reflexive){out.reflexive=false;out.witnesses[0]={{i,0,0},1};}
    for(unsigned j=0;j<6;++j){
      if(has(i,j)&&!has(j,i)&&out.symmetric){out.symmetric=false;out.witnesses[1]={{i,j,0},2};}
      for(unsigned k=0;k<6;++k)if(has(i,j)&&has(j,k)&&!has(i,k)&&out.transitive){out.transitive=false;out.witnesses[2]={{i,j,k},3};}
    }
  }
  std::array<unsigned,6> queue{};unsigned head=0,tail=1;queue[0]=source;out.distance[source]=0;
  while(head<tail){const auto i=queue[head++];for(unsigned j=0;j<6;++j)if(has(i,j)&&out.distance[j]<0){out.distance[j]=out.distance[i]+1;out.parent[j]=static_cast<int>(i);queue[tail++]=j;}}
  out.reachableCount=tail;out.equivalence=out.reflexive&&out.symmetric&&out.transitive;
  out.classOf.fill(6);
  if(out.equivalence)for(unsigned i=0;i<6;++i)if(out.classOf[i]==6){for(unsigned j=0;j<6;++j)if(has(i,j))out.classOf[j]=out.classCount;++out.classCount;}
  return out;
}
unsigned bipartiteEdges(FiniteRelation relation){
  if(relation>>36)throw std::invalid_argument("finite relation out of range");
  unsigned result=0;for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(relation&relationEdge(i,j+3))result|=1U<<(3*i+j);return result;
}
BipartiteMatching analyzeBipartiteMatching(unsigned edges){
  if(edges>>9)throw std::invalid_argument("bipartite graph out of range");
  BipartiteMatching out;std::array<int,3> left{-1,-1,-1},right{-1,-1,-1};
  for(unsigned step=0;step<3;++step){
    std::array<unsigned,3> queue{};std::array<bool,3> seenLeft{};std::array<int,3> parentRight{-1,-1,-1};unsigned head=0,tail=0;
    for(unsigned i=0;i<3;++i)if(left[i]<0){queue[tail++]=i;seenLeft[i]=true;}
    int freeRight=-1;
    while(head<tail&&freeRight<0){const auto i=queue[head++];for(unsigned j=0;j<3;++j)if((edges&(1U<<(3*i+j)))&&left[i]!=static_cast<int>(j)&&parentRight[j]<0){
      parentRight[j]=static_cast<int>(i);if(right[j]<0){freeRight=static_cast<int>(j);break;}
      const auto next=static_cast<unsigned>(right[j]);if(!seenLeft[next]){seenLeft[next]=true;queue[tail++]=next;}
    }}
    if(freeRight<0)break;
    auto& path=out.paths[step];unsigned length=0;int j=freeRight;
    while(j>=0){const auto i=parentRight[j];path[length++]=static_cast<unsigned>(j)+3;path[length++]=static_cast<unsigned>(i);j=left[i];}
    std::reverse(path.begin(),path.begin()+length);out.pathLengths[step]=length;
    j=freeRight;while(j>=0){const auto i=parentRight[j],previous=left[i];left[i]=j;right[j]=i;j=previous;}
    ++out.maximumSize;unsigned mask=0;for(unsigned i=0;i<3;++i)if(left[i]>=0)mask|=1U<<(3*i+left[i]);out.states[step+1]=mask;
  }
  for(unsigned step=out.maximumSize+1;step<4;++step)out.states[step]=out.states[out.maximumSize];
  for(unsigned subset=1;subset<8;++subset){unsigned neighbors=0;for(unsigned i=0;i<3;++i)if(subset&(1U<<i))neighbors|=(edges>>(3*i))&7U;
    const int deficit=std::popcount(subset)-std::popcount(neighbors);if(deficit>static_cast<int>(out.deficiency)){out.deficiency=static_cast<unsigned>(deficit);out.deficientLeft=subset;out.neighbors=neighbors;}
  }
  return out;
}
} // namespace paths

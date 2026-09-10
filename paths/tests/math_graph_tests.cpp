#include "runtime/math_objects/FiniteGraph.hpp"
#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include "ui/MathObjectLayout.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
namespace {
using namespace paths;using P=MathParameter;using K=MathObjectKind;
std::size_t graphCases=0,matchingCases=0,frames=0,maxVertices=0,maxIndices=0,maxParts=0;
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
bool has(FiniteRelation e,unsigned i,unsigned j){return (e&relationEdge(i,j))!=0;}
void graphOracle(FiniteRelation e){
  ++graphCases;std::array<unsigned,6> rows{};for(unsigned i=0;i<6;++i)rows[i]=static_cast<unsigned>((e>>(6*i))&63);
  bool reflexive=true,symmetric=true,transitive=true;
  for(unsigned i=0;i<6;++i){reflexive=reflexive&&((rows[i]>>i)&1);unsigned twice=0;for(unsigned j=0;j<6;++j){if(rows[i]&(1U<<j))twice|=rows[j];symmetric=symmetric&&(has(e,i,j)==has(e,j,i));}transitive=transitive&&!(twice&~rows[i]);}
  for(unsigned source=0;source<6;++source){const auto a=analyzeFiniteGraph(e,source);require(a.reflexive==reflexive&&a.symmetric==symmetric&&a.transitive==transitive,"property oracle");require(a.equivalence==(reflexive&&symmetric&&transitive),"equivalence predicate");
    unsigned reached=0,frontier=1U<<source;std::array<int,6> distance;distance.fill(-1);
    for(unsigned length=0;length<6;++length){for(unsigned i=0;i<6;++i)if((frontier&(1U<<i))&&distance[i]<0)distance[i]=static_cast<int>(length);reached|=frontier;unsigned next=0;for(unsigned i=0;i<6;++i)if(frontier&(1U<<i))next|=rows[i];frontier=next;}
    require(a.distance==distance&&a.reachableCount==std::popcount(reached),"walk enumeration disagrees with BFS");
    for(unsigned i=0;i<6;++i){if(i==source||distance[i]<0)require(a.parent[i]==-1,"root or unreachable predecessor");else require(a.parent[i]>=0&&has(e,static_cast<unsigned>(a.parent[i]),i)&&a.distance[a.parent[i]]==distance[i]-1,"shortest-path predecessor");}
    for(unsigned p=0;p<3;++p){const auto w=a.witnesses[p];const bool holds=std::array{reflexive,symmetric,transitive}[p];require((w.count==0)==holds,"witness existence");if(!holds){require(w.count==p+1,"witness arity");const auto i=w.nodes[0],j=w.nodes[1],k=w.nodes[2];if(p==0)require(!has(e,i,i),"reflexivity witness");if(p==1)require(has(e,i,j)&&!has(e,j,i),"symmetry witness");if(p==2)require(has(e,i,j)&&has(e,j,k)&&!has(e,i,k),"transitivity witness");}}
    if(a.equivalence){unsigned classes=0;for(unsigned i=0;i<6;++i){classes=std::max(classes,a.classOf[i]+1);for(unsigned j=0;j<6;++j)require((a.classOf[i]==a.classOf[j])==has(e,i,j),"equivalence partition");}require(a.classCount==classes,"class count");}
    else {require(a.classCount==0,"invented classes");for(auto c:a.classOf)require(c==6,"undefined class sentinel");}
  }
}
bool validMatching(unsigned m){unsigned left=0,right=0;for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(m&(1U<<(3*i+j))){if((left&(1U<<i))||(right&(1U<<j)))return false;left|=1U<<i;right|=1U<<j;}return true;}
void kernels(){
  for(unsigned code=0;code<512;++code){FiniteRelation e=relationEdge(3,3)|relationEdge(4,4)|relationEdge(5,5);for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(code&(1U<<(3*i+j)))e|=relationEdge(i,j);graphOracle(e);}
  constexpr auto full=(FiniteRelation{1}<<36)-1;graphOracle(full);graphOracle(0);
  for(unsigned i=0;i<36;++i){graphOracle(full^(FiniteRelation{1}<<i));graphOracle(FiniteRelation{1}<<i);}
  std::uint64_t seed=17091;for(unsigned i=0;i<2048;++i){seed=seed*6364136223846793005ULL+1442695040888963407ULL;graphOracle(seed&full);}
  for(unsigned edges=0;edges<512;++edges){++matchingCases;const auto m=analyzeBipartiteMatching(edges);unsigned best=0;
    for(unsigned candidate=0;candidate<512;++candidate)if(!(candidate&~edges)&&validMatching(candidate))best=std::max(best,static_cast<unsigned>(std::popcount(candidate)));
    require(m.maximumSize==best&&m.deficiency==3-best,"exhaustive optimum / Hall deficiency");
    FiniteRelation r=0;for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(edges&(1U<<(3*i+j)))r|=relationEdge(i,j+3);require(bipartiteEdges(r)==edges,"bipartite extraction");
    for(unsigned stage=0;stage<4;++stage){require(!(m.states[stage]&~edges)&&validMatching(m.states[stage]),"matching validity");require(std::popcount(m.states[stage])==std::min(stage,best),"matching stage size");}
    for(unsigned step=0;step<best;++step){const unsigned n=m.pathLengths[step];require(n>=2&&n<=6&&n%2==0,"augmenting path length");unsigned visited=0,toggle=0;
      for(unsigned k=0;k<n;++k){const unsigned node=m.paths[step][k];require(node<6&&!(visited&(1U<<node)),"augmenting path repeats node");visited|=1U<<node;require((node<3)==(k%2==0),"path bipartition");if(k){const auto x=m.paths[step][k-1],y=node;const unsigned bit=1U<<(3*(x<3?x:y)+(x>=3?x-3:y-3));require(edges&bit,"augmenting nonedge");require(bool(m.states[step]&bit)==(k%2==0),"path does not alternate");toggle^=bit;}}
      require((m.states[step]^toggle)==m.states[step+1],"augmenting symmetric difference");
    }
    if(best<3){unsigned neighbors=0;for(unsigned i=0;i<3;++i)if(m.deficientLeft&(1U<<i))neighbors|=(edges>>(3*i))&7U;require(neighbors==m.neighbors&&std::popcount(m.deficientLeft)>std::popcount(neighbors),"Hall witness");}
  }
  for(auto mask:{FiniteRelation{1}<<36,std::numeric_limits<FiniteRelation>::max()}){bool rejected=false;try{(void)analyzeFiniteGraph(mask,0);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"oversize relation accepted");}
  bool rejected=false;try{(void)analyzeFiniteGraph(0,6);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"bad source accepted");rejected=false;try{(void)analyzeBipartiteMatching(512);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"oversize bipartite graph accepted");
}
void act(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double value){act(m,{MathActionKind::SetParameter,{},p,value});}
void level(MathObjects& m,unsigned l){act(m,{MathActionKind::SetLevel,{},{},double(l)});}
void preset(MathObjects& m,unsigned p){act(m,{MathActionKind::ObjectPreset,{},{},0,p});}
double metric(const MathObjects& m,std::string_view name){for(unsigned i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==name)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric");}
void inspect(const MathObjects& m){
  // Fresh CPU scene for each independent branch; revision numbers alone cannot
  // distinguish snapshots copied from the same parent and edited differently.
  MathObjectScene scene;const auto& s=m.snapshot();const auto& f=scene.publish(s,{0,0,800,600});++frames;maxParts=std::max(maxParts,s.partCount);maxVertices=std::max(maxVertices,f.vertices.size());maxIndices=std::max(maxIndices,f.indices.size());
  require(s.partCount<=s.parts.size()&&s.labelCount<=s.labels.size(),"snapshot capacity");require(!f.vertices.empty()&&f.vertices.size()<=kSceneVertexCapacity&&f.indices.size()<=kSceneIndexCapacity,"mesh budget");
  std::set<unsigned> ids;std::size_t end=0;for(const auto& d:f.draws){require(ids.insert(d.objectId.value).second&&d.firstIndex==end&&d.indexCount%3==0,"draw ownership");end+=d.indexCount;require(iggy3d::isFinite(d.bounds.min)&&iggy3d::isFinite(d.bounds.max),"draw bounds");}require(end==f.indices.size(),"unowned indices");for(auto i:f.indices)require(i<f.vertices.size(),"index bound");
  for(const auto& v:f.vertices){for(auto x:v.position)require(std::isfinite(x),"vertex finite");for(auto c:v.color)require(std::isfinite(c)&&c>=0&&c<=1,"colour finite");}
  require(s.curve.active&&s.curve.count==(s.level==3?3U:6U)&&s.curve.selected<s.curve.count&&s.curve.selectionParameter==P::GraphNode,"picker contract");
  for(unsigned i=0;i<s.curve.count;++i){const auto p=scene.project(s.curve.controls[i]);require(iggy3d::isFinite(p)&&p.z>=0&&p.x>=0&&p.x<=800&&p.y>=0&&p.y<=600,"picker offscreen");}
  const auto rows=mathControlRows(m);unsigned edgeControls=0;for(unsigned i=0;i<rows.count;++i)for(unsigned j=0;j<rows.rows[i].count;++j){const auto p=rows.rows[i].parameters[j];require(m.parameterAvailable(p)&&m.parameter(p)<=mathControlRange(m,p).maximum,"control range");if(p>=P::GraphE00&&p<=P::GraphE55){++edgeControls;const auto edge=unsigned(p)-unsigned(P::GraphE00);require(edge/6==m.parameter(P::GraphNode)&&(s.level!=3||edge%6>=3),"wrong adjacency editor");}}
  require(edgeControls==(s.level==3?3U:6U),"missing edge controls");for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"nonfinite metric");
  require(s.solid.vertexCount==(s.level==3?36U:144U)&&s.solid.indexCount==(s.level==3?54U:216U),"adjacency mesh shape");
}
void scenes(){
  MathObjects m;act(m,{MathActionKind::Select,K::Graph});
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned example=0;example<9;++example){preset(m,example);inspect(m);for(const auto& p:mathParameterSpecs())if(p.owner==K::Graph&&m.parameterAvailable(p.id))for(double value:{p.minimum,m.parameterMaximum(p.id)}){auto copy=m;set(copy,p.id,value);inspect(copy);}}}
  for(unsigned l=0;l<4;++l){level(m,l);preset(m,std::array<unsigned,4>{0,0,3,6}[l]);if(l==1)set(m,P::GraphTime,6);if(l==2)set(m,P::GraphGroup,1);if(l==3)set(m,P::GraphMatchTime,3);act(m,{MathActionKind::Check});require(m.snapshot().feedback==MathFeedback::Solved,"layer challenge failed");}
  level(m,2);preset(m,4);for(unsigned missing=0;missing<36;++missing){auto copy=m;set(copy,static_cast<P>(unsigned(P::GraphE00)+missing),0);for(unsigned p=0;p<3;++p){set(copy,P::GraphProperty,p);inspect(copy);}}
  preset(m,3);const auto table=m.snapshot().table.values;for(unsigned step=0;step<=100;++step){set(m,P::GraphGroup,step/100.);require(m.snapshot().table.values==table,"grouping changed relation");inspect(m);}
  preset(m,2);require(metric(m,"Transitive")==0&&metric(m,"Witness node count")==3,"missing transitivity witness");require(!m.parameterAvailable(P::GraphGroup),"invalid equivalence grouping");
  level(m,3);preset(m,6);bool reroute=false;for(unsigned step=0;step<=60;++step){set(m,P::GraphMatchTime,step/20.);inspect(m);for(unsigned i=0;i<m.snapshot().partCount;++i)if(m.snapshot().parts[i].role=="graph_augment_tracer")reroute=true;}require(reroute,"matching tracer missing");
  set(m,P::GraphMatchTime,0);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},3});require(m.parameter(P::GraphMatchTime)==3&&!m.snapshot().playing,"matching playback end");
  preset(m,7);require(metric(m,"Maximum matching size")==2&&metric(m,"Hall deficiency")==1,"Hall obstruction scene");
  level(m,0);set(m,P::GraphNode,5);level(m,3);require(m.parameter(P::GraphNode)==2,"matching selector not clamped");const auto revision=m.snapshot().revision;
  require(!m.dispatch({MathActionKind::SetParameter,{},P::GraphNode,5}).accepted&&!m.dispatch({MathActionKind::SetParameter,{},P::GraphE00,1}).accepted&&m.snapshot().revision==revision,"invalid edits not atomic");
  act(m,mathResetControlGroup(m,MathControlGroup::Probe));require(m.parameter(P::GraphNode)==0,"selector reset");
  level(m,1);preset(m,5);set(m,P::GraphTarget,0);require(metric(m,"Shortest path length")==0,"zero-length path missing");set(m,P::GraphTarget,1);require(metric(m,"Shortest path length")==-1,"invented path");
  preset(m,0);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},6});require(!m.snapshot().playing&&metric(m,"Discovered nodes")==6,"search playback end");set(m,P::GraphTime,0);act(m,{MathActionKind::TogglePlayback});set(m,P::GraphE00,1);require(!m.snapshot().playing,"editing did not pause search");
}
}
int main(){try{kernels();scenes();std::printf("relations and graphs passed: %zu relation cases x 6 sources, %zu exhaustive bipartite graphs, %zu CPU scenes; maxima %zu parts / %zu vertices / %zu indices\n",graphCases,matchingCases,frames,maxParts,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}

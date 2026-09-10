#include "runtime/math_objects/FiniteMaps.hpp"
#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include "ui/MathObjectLayout.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using namespace paths;using P=MathParameter;using K=MathObjectKind;
std::size_t cases=0,frames=0,maxVertices=0,maxIndices=0;
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
void near(double a,double b){require(std::isfinite(a)&&std::fabs(a-b)<2e-14,"probability identity");}
unsigned power(unsigned a,unsigned n){unsigned p=1;while(n--)p*=a;return p;}
std::array<unsigned,4> decode(unsigned code,unsigned base,unsigned n){std::array<unsigned,4> v{};for(unsigned i=0;i<n;++i){v[i]=code%base;code/=base;}return v;}
void kernel(){
  // Exhaust every pair of total maps between sets of one to four elements.
  // Independently count preimages, pairwise collisions and weighted outcomes.
  for(unsigned a=1;a<=4;++a)for(unsigned b=1;b<=4;++b)for(unsigned c=1;c<=4;++c)
  for(unsigned fi=0;fi<power(b,a);++fi)for(unsigned gi=0;gi<power(c,b);++gi){
    FiniteMapsInput in;in.a=a;in.b=b;in.c=c;in.f=decode(fi,b,a);in.g=decode(gi,c,b);in.weights={1,2,3,4};
    std::array<double,4> expectedB{},expectedC{};double total=a*(a+1)/2.;bool injection=true,surjection=true;
    for(unsigned i=0;i<a;++i){in.h[i]=in.g[in.f[i]];expectedB[in.f[i]]+=(i+1)/total;expectedC[in.h[i]]+=(i+1)/total;for(unsigned j=0;j<i;++j)if(in.f[i]==in.f[j])injection=false;}
    for(unsigned j=0;j<b;++j)surjection=surjection&&std::find(in.f.begin(),in.f.begin()+a,j)!=in.f.begin()+a;
    const auto r=analyzeFiniteMaps(in);++cases;require(r.injective==injection&&r.surjective==surjection&&r.bijective==(injection&&surjection),"map classification");
    require(r.commutes&&r.agreement==a,"composition equality");unsigned all=0,classes=0;
    for(unsigned j=0;j<b;++j){unsigned mask=0,count=0;for(unsigned i=0;i<a;++i)if(in.f[i]==j){mask|=1U<<i;++count;}
      require(r.fiberMasks[j]==mask&&r.fiberSizes[j]==count,"fiber membership");require(!(all&mask),"overlapping partition");all|=mask;classes+=count>0;near(r.outputMass[j],expectedB[j]);}
    require(all==(1U<<a)-1&&r.imageSize==classes&&r.collisions==a-classes,"partition completeness");
    for(unsigned i=0;i<a;++i){require(r.composed[i]==in.h[i],"composed destination");for(unsigned j=0;j<a;++j)require((r.classOf[i]==r.classOf[j])==(in.f[i]==in.f[j]),"quotient equivalence");}
    for(unsigned j=0;j<c;++j)near(r.composedMass[j],expectedC[j]);
    if(c>1){in.h[a-1]=(in.h[a-1]+1)%c;const auto bad=analyzeFiniteMaps(in);require(!bad.commutes&&bad.agreement==a-1,"single-route counterexample");}
  }
  FiniteMapsInput in;in.f={0,0,1,2};in.weights={1,3,2,2};const auto r=analyzeFiniteMaps(in);near(r.eventProbability,.5);near(r.conditionalSource[0],.25);near(r.conditionalSource[1],.75);near(r.conditionalOutput[0],1);
  in.weights={0,0,2,2};const auto zeroEvent=analyzeFiniteMaps(in);require(zeroEvent.probabilityDefined&&!zeroEvent.conditionalDefined,"zero-weight event is undefined");
  in.weights={0,0,0,0};require(!analyzeFiniteMaps(in).probabilityDefined,"zero total must not be uniform");
  in.weights={1,3,2,2};for(unsigned fiber=0;fiber<4;++fiber){in.fiber=fiber;const auto q=analyzeFiniteMaps(in);double sum=0;for(unsigned i=0;i<4;++i){sum+=q.conditionalSource[i];if(in.f[i]!=fiber)near(q.conditionalSource[i],0);}near(sum,q.conditionalDefined?1:0);}
  in.fiber=0;in.weights={1e-300,3e-300,1e300,1e300};const auto tiny=analyzeFiniteMaps(in);require(tiny.conditionalDefined,"tiny positive event lost");near(tiny.conditionalSource[0],.25);near(tiny.conditionalSource[1],.75);
  in.weights.fill(std::numeric_limits<double>::max());const auto large=analyzeFiniteMaps(in);for(auto p:large.sourceMass)near(p,.25);
  for(unsigned which=0;which<10;++which){FiniteMapsInput bad;switch(which){case 0:bad.a=0;break;case 1:bad.b=5;break;case 2:bad.c=0;break;case 3:bad.f[0]=4;break;case 4:bad.g[0]=4;break;case 5:bad.h[0]=4;break;case 6:bad.weights[0]=-1;break;case 7:bad.weights[0]=std::numeric_limits<double>::infinity();break;case 8:bad.fiber=4;break;default:bad.weights[0]=std::numeric_limits<double>::quiet_NaN();}
    bool rejected=false;try{(void)analyzeFiniteMaps(bad);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"invalid map input accepted");}
}
void act(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double value){act(m,{MathActionKind::SetParameter,{},p,value});}
void level(MathObjects& m,unsigned v){act(m,{MathActionKind::SetLevel,{},{},double(v)});}
void preset(MathObjects& m,unsigned v){act(m,{MathActionKind::ObjectPreset,{},{},0,v});}
double metric(const MathObjects& m,std::string_view label){for(unsigned i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==label)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric");}
void inspect(MathObjects& m,MathObjectScene& scene){
  const auto& s=m.snapshot();const auto& frame=scene.publish(s,{0,0,800,600});++frames;maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());
  require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"mesh budget");require(iggy3d::isFinite(frame.clipFromWorld),"camera matrix");
  std::set<unsigned> ids;std::size_t end=0;for(const auto& draw:frame.draws){require(ids.insert(draw.objectId.value).second&&draw.firstIndex==end&&draw.indexCount%3==0,"draw ownership");end+=draw.indexCount;require(iggy3d::isFinite(draw.bounds.min)&&iggy3d::isFinite(draw.bounds.max),"finite bounds");}require(end==frame.indices.size(),"unowned geometry");
  for(const auto& v:frame.vertices){for(auto x:v.position)require(std::isfinite(x),"finite vertex");for(auto x:v.color)require(std::isfinite(x)&&x>=0&&x<=1,"colour range");}for(auto i:frame.indices)require(i<frame.vertices.size(),"index bound");
  require(s.curve.active&&s.curve.count==m.parameter(P::MapsA)&&s.curve.selected<s.curve.count&&s.curve.selectionParameter==P::MapsSource,"picker ownership");
  for(unsigned i=0;i<s.curve.count;++i){const auto p=scene.project(s.curve.controls[i]);require(iggy3d::isFinite(p)&&p.z>=0&&p.x>=0&&p.x<=800&&p.y>=0&&p.y<=600,"input picker outside viewport");}
  const auto rows=mathControlRows(m);for(unsigned i=0;i<rows.count;++i)for(unsigned j=0;j<rows.rows[i].count;++j){const auto p=rows.rows[i].parameters[j];const auto range=mathControlRange(m,p);require(m.parameterAvailable(p)&&m.parameter(p)>=range.minimum&&m.parameter(p)<=range.maximum,"control bounds");}
  for(unsigned i=0;i<s.plotCount;++i){const auto& p=s.plots[i];require(p.scrubParameter==P::Count&&p.seriesCount==1,"discrete plot contract");double sum=0;for(unsigned j=0;j<p.series[0].count;++j){const auto x=p.series[0].points[j];require(x.x==j&&x.y>=0&&x.y<=1,"mass bars");sum+=x.y;}near(sum,1);}
  for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"finite metric");
}
void scenes(){
  MathObjects m;MathObjectScene scene;act(m,{MathActionKind::Select,K::Maps});
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned example=0;example<9;++example){preset(m,example);inspect(m,scene);
    for(const auto& p:mathParameterSpecs())if(p.owner==K::Maps&&m.parameterAvailable(p.id))for(double value:{p.minimum,m.parameterMaximum(p.id)}){auto copy=m;set(copy,p.id,value);inspect(copy,scene);}
  }}
  for(unsigned l=0;l<4;++l){level(m,l);preset(m,std::array<unsigned,4>{1,3,4,6}[l]);if(l==3)set(m,P::MapsCondition,1);act(m,{MathActionKind::Check});require(m.snapshot().feedback==MathFeedback::Solved,"layer challenge");}
  level(m,2);preset(m,5);act(m,{MathActionKind::Check});require(m.snapshot().feedback==MathFeedback::TryAgain,"counterexample wrongly commutes");
  preset(m,4);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},4});near(m.parameter(P::MapsTime),2);require(!m.snapshot().playing,"playback stop");inspect(m,scene);
  set(m,P::MapsTime,0);act(m,{MathActionKind::TogglePlayback});set(m,P::MapsF0,1);require(!m.snapshot().playing,"editing did not pause traversal");
  set(m,P::MapsSource,3);set(m,P::MapsMiddle,3);set(m,P::MapsB,1);set(m,P::MapsC,1);set(m,P::MapsA,1);
  require(m.parameter(P::MapsSource)==0&&m.parameter(P::MapsMiddle)==0&&m.parameter(P::MapsFiber)==0,"resize selection normalization");
  for(auto first:{P::MapsF0,P::MapsG0,P::MapsH0})for(unsigned i=0;i<4;++i)require(m.parameter(static_cast<P>(unsigned(first)+i))==0,"resize destination normalization");inspect(m,scene);
  const auto revision=m.snapshot().revision;require(!m.dispatch({MathActionKind::SetParameter,{},P::MapsF0,1}).accepted&&!m.dispatch({MathActionKind::SetParameter,{},P::MapsF1,0}).accepted&&m.snapshot().revision==revision,"invalid edit was not atomic");
  act(m,mathResetControlGroup(m,MathControlGroup::Operation));inspect(m,scene);require(m.parameter(P::MapsF0)==0,"partial reset escaped resized codomain");
  act(m,{MathActionKind::Reset});require(m.parameter(P::MapsA)==4&&m.parameter(P::MapsF3)==3,"full reset");
  level(m,3);preset(m,7);require(m.snapshot().plotCount==0&&metric(m,"Conditional probability defined")==0,"zero-event presentation");
  set(m,P::MapsCondition,0);require(m.snapshot().plotCount==2,"unconditional mass hidden by zero event");
  preset(m,8);require(m.snapshot().plotCount==0&&metric(m,"Probability defined")==0,"zero-total presentation");inspect(m,scene);
  // Grouping changes placement, not the mapping or the computed partition.
  level(m,1);preset(m,3);set(m,P::MapsGroup,0);const auto before=m.snapshot().table;const auto original=m.snapshot().curve.controls;set(m,P::MapsGroup,1);require(before.values==m.snapshot().table.values,"grouping changed mathematics");require(original[1].y!=m.snapshot().curve.controls[1].y,"grouping did not move interleaved inputs");
  // Every A selector must expose exactly its own f field and probability weight.
  level(m,3);act(m,{MathActionKind::Reset});for(unsigned i=0;i<4;++i){set(m,P::MapsSource,i);const auto rows=mathControlRows(m);unsigned f=0,w=0;for(unsigned j=0;j<rows.count;++j){const auto p=rows.rows[j].parameters[0];if(p>=P::MapsF0&&p<=P::MapsF3){++f;require(unsigned(p)-unsigned(P::MapsF0)==i,"wrong f editor");}if(p>=P::MapsW0&&p<=P::MapsW3){++w;require(unsigned(p)-unsigned(P::MapsW0)==i,"wrong weight editor");}}require(f==1&&w==1,"selected field missing");}
}
}
int main(){try{kernel();scenes();std::printf("sets and maps passed: %zu exhaustive map pairs, %zu CPU scenes; maxima %zu vertices / %zu indices\n",cases,frames,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}

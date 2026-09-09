#include "runtime/math_objects/Simplex.hpp"
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

using namespace paths;
namespace {
using P=MathParameter;using K=MathObjectKind;using F=SimplexFunction;
unsigned checks=0,scenes=0;std::size_t maxVertices=0,maxIndices=0;double maxKlError=0,maxGapError=0;
void require(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double tolerance,const char* why){++checks;if(!std::isfinite(a)||!std::isfinite(b)||std::fabs(a-b)>tolerance){char message[300];std::snprintf(message,sizeof(message),"%s: %.17g vs %.17g, tolerance %.3g",why,a,b,tolerance);throw std::runtime_error(message);}}
template<class Fn>void rejects(Fn f){bool failed=false;try{f();}catch(const std::invalid_argument&){failed=true;}require(failed,"invalid numerical request accepted");}
long double klOracle(const SimplexPoint& p,const SimplexPoint& q){long double result=0;for(unsigned i=0;i<3;++i)if(p[i]>0){if(q[i]==0)return std::numeric_limits<long double>::infinity();result+=static_cast<long double>(p[i])*std::log(static_cast<long double>(p[i])/q[i]);}return result;}
double meanOracle(const SimplexPoint& p,const SimplexPoint& a){return p[0]*a[0]+p[1]*a[1]+p[2]*a[2];}
void mathematics(){
  const SimplexPoint values{-1,0,1},uniform{1./3,1./3,1./3};
  near(simplexFunction(F::Entropy,uniform,values),std::log(3.),1e-15,"uniform entropy");
  near(simplexFunction(F::Variance,uniform,values),2./3,1e-15,"uniform variance");
  near(simplexKl({1,0,0},{.5,.5,0}),std::log(2.),1e-15,"corner relative entropy");
  require(std::isinf(simplexKl({.5,.5,0},{1,0,0})),"missing reference support must be infinite");
  near(simplexKl({0,1,0},{0,1,0}),0,0,"zero terms in identical distributions");
  near(simplexPoint(.7,.3)[2],0,0,"closed edge became tiny positive probability");
  for(unsigned i=0;i<=24;++i)for(unsigned j=0;j<=24-i;++j){
    const auto p=simplexPoint(i/24.,j/24.);const SimplexPoint q{.17,.32,.51};
    require(isSimplexPoint(p),"grid point outside simplex");
    const double expected=static_cast<double>(klOracle(p,q));const double actual=simplexKl(p,q);maxKlError=std::max(maxKlError,std::fabs(actual-expected));near(actual,expected,2e-14,"independent log-ratio KL");require(actual>=0,"negative KL");near(simplexKl(p,p),0,0,"identity KL");
    const double mean=meanOracle(p,values),second=p[0]+p[2];near(simplexFunction(F::Mean,p,values),mean,1e-15,"independent mean");near(simplexFunction(F::Variance,p,values),second-mean*mean,2e-15,"independent second-moment variance");
    require(simplexFunction(F::Entropy,p,values)>=0&&simplexFunction(F::Entropy,p,values)<=std::log(3.)+1e-14,"entropy range");
    for(double t:{0.,.03,.25,.5,.9,1.}){
      const auto r=mixSimplex(p,q,t);require(isSimplexPoint(r),"mixture escaped simplex");
      for(unsigned k=0;k<3;++k)near(r[k],(1-t)*p[k]+t*q[k],1e-16,"mixture coordinate");
      for(unsigned kind=0;kind<4;++kind){const auto f=static_cast<F>(kind);const double gap=(1-t)*simplexFunction(f,p,values)+t*simplexFunction(f,q,values)-simplexFunction(f,r,values);if(f==F::Mean)near(gap,0,3e-15,"affine expectation");else if(f==F::NegativeEntropy)require(gap>=-3e-15,"convex negative entropy");else require(gap<=3e-15,"concave entropy or variance");}
      const double meanDifference=meanOracle(p,values)-meanOracle(q,values);
      near(simplexFunction(F::Variance,r,values)-(1-t)*simplexFunction(F::Variance,p,values)-t*simplexFunction(F::Variance,q,values),t*(1-t)*meanDifference*meanDifference,2e-15,"law of total variance for mixtures");
      const double gap=simplexFunction(F::NegativeEntropy,r,values)-simplexEntropyTangent(r,q),kl=simplexKl(r,q);maxGapError=std::max(maxGapError,std::fabs(gap-kl));near(gap,kl,2e-14,"supporting plane gap equals KL");
      const SimplexPoint permuted{r[2],r[0],r[1]},permutedQ{q[2],q[0],q[1]};near(simplexKl(permuted,permutedQ),kl,1e-14,"category permutation invariance");
    }
    for(unsigned mask=1;mask<8;++mask){SimplexPoint boundary{};unsigned count=0;for(unsigned k=0;k<3;++k)count+=(mask>>k)&1U;for(unsigned k=0;k<3;++k)if((mask>>k)&1U)boundary[k]=1./count;bool finite=true;for(unsigned k=0;k<3;++k)finite=finite&&!(p[k]>0&&boundary[k]==0);const double result=simplexKl(p,boundary);require(std::isfinite(result)==finite,"exact KL support rule");if(finite)near(result,static_cast<double>(klOracle(p,boundary)),2e-14,"finite boundary KL");}
  }
  for(double delta:{1e-5,1e-7,1e-9}){const SimplexPoint p{.5+delta,.5-delta,0},q{.5,.5,0};const double result=simplexKl(p,q);const double expected=(p[0]-.5)*(p[0]-.5)+(p[1]-.5)*(p[1]-.5);require(result>0,"near-equal KL cancelled to zero");near(result,expected,expected*1e-8,"near-equal KL quadratic limit");}
  const SimplexPoint middle{0,1,0},ends{.5,0,.5};near(simplexFunction(F::Mean,middle,values),simplexFunction(F::Mean,ends,values),0,"same-mean fixture");near(simplexFunction(F::Variance,middle,values),0,0,"certain middle variance");near(simplexFunction(F::Variance,ends,values),1,0,"split extremes variance");
  near(simplexFunction(F::Variance,uniform,{2,2,2}),0,0,"equal outcome variance");near(simplexFunction(F::Entropy,uniform,{2,2,2}),std::log(3.),1e-15,"category entropy must not merge equal values");
  for(double bad:{-1.,1.1,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){rejects([&]{simplexPoint(bad,.1);});rejects([&]{mixSimplex(uniform,uniform,bad);});}
  rejects([&]{simplexPoint(.6,.6);});rejects([&]{simplexKl({0,0,0},uniform);});rejects([&]{simplexEntropyTangent(uniform,{1,0,0});});rejects([&]{simplexFunction(F::Count,uniform,values);});rejects([&]{simplexFunction(F::Mean,uniform,{4,0,0});});
}
void act(MathObjects& m,MathAction a){const auto result=m.dispatch(a);if(!result.accepted)throw std::runtime_error(std::string(result.reason));}
void set(MathObjects& m,P p,double v){act(m,{MathActionKind::SetParameter,{},p,v});}
void preset(MathObjects& m,unsigned i){act(m,{MathActionKind::ObjectPreset,{},{},0,i});}
void level(MathObjects& m,unsigned i){act(m,{MathActionKind::SetLevel,{},{},static_cast<double>(i)});}
double metric(const MathObjects& m,std::string_view name){for(unsigned i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==name)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric "+std::string(name));}
void reject(MathObjects& m,MathAction a){const auto rev=m.snapshot().revision;std::array<double,static_cast<unsigned>(P::Count)> before{};for(auto p:mathParameterSpecs())before[static_cast<unsigned>(p.id)]=m.parameter(p.id);require(!m.dispatch(a).accepted,"invalid model action accepted");require(m.snapshot().revision==rev,"rejected action changed revision");for(auto p:mathParameterSpecs())near(m.parameter(p.id),before[static_cast<unsigned>(p.id)],0,"rejected action changed parameter");}
void challenge(MathObjects& m,bool pass){act(m,{MathActionKind::Check});require((m.snapshot().feedback==MathFeedback::Solved)==pass,"incorrect simplex challenge");}
void inspect(MathObjects& m,MathObjectScene& scene){
  const auto& s=m.snapshot();const auto& frame=scene.publish(s,{0,0,800,600});++scenes;maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());
  require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"scene capacity");require(iggy3d::isFinite(frame.clipFromWorld),"finite camera");
  std::set<unsigned> ids;unsigned end=0;for(const auto& draw:frame.draws){require(ids.insert(draw.objectId.value).second&&draw.firstIndex==end,"draw ownership");end+=draw.indexCount;}require(end==frame.indices.size(),"draw coverage");
  for(const auto& v:frame.vertices){for(float x:v.position)require(std::isfinite(x),"finite scene vertex");for(float x:v.color)require(std::isfinite(x)&&x>=0&&x<=1,"finite colour");}for(auto i:frame.indices)require(i<frame.vertices.size(),"scene index");
  for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"infinite metric sent to UI");for(unsigned i=0;i<s.table.rowCount;++i)for(unsigned j=0;j<s.table.columnCount;++j)require(std::isfinite(s.table.values[i][j]),"finite table");
  for(unsigned i=0;i<s.plotCount;++i){const auto& p=s.plots[i];require(p.seriesCount<=3,"plot series capacity");for(unsigned j=0;j<p.seriesCount;++j){require(p.series[j].count>0&&p.series[j].count<=129,"plot point count");for(unsigned k=0;k<p.series[j].count;++k)require(std::isfinite(p.series[j].points[k].x)&&std::isfinite(p.series[j].points[k].y),"infinite plot sample");}if(p.hasMarker)require(std::isfinite(p.marker.x)&&std::isfinite(p.marker.y),"finite plot marker");}
  for(unsigned j=1;j<4;++j){double sum=0;for(unsigned i=0;i<3;++i){const double p=s.table.values[i][j];require(p>=0&&p<=1,"table probability range");sum+=p;}near(sum,1,3e-15,"table distribution normalization");}
  if(s.solid.indexCount){
    require(s.solid.vertexCount==561&&s.solid.indexCount==3072,"triangular mesh count");double area=0;
    const SimplexPoint outcomes{m.parameter(P::SimplexValue0),m.parameter(P::SimplexValue1),m.parameter(P::SimplexValue2)},q=simplexPoint(m.parameter(P::SimplexQ0),m.parameter(P::SimplexQ1));
    const auto f=static_cast<F>(m.parameter(P::SimplexFunction));const double scale=s.level?metric(m,"Height / function unit"):0;
    for(unsigned i=0;i<s.solid.vertexCount;++i){const auto& v=s.solid.vertices[i];near(iggy3d::length(v.normal),1,2e-7,"unit mesh normal");require(v.normal.y>0,"upward graph normal");const double c=(v.position.z+1.1547005383792515)/3.4641016151377546,b=(1-c+v.position.x/2)/2,a=1-b-c;require(a>=-1e-7&&b>=-1e-7&&c>=-1e-7,"mesh outside probability triangle");const auto point=simplexPoint(std::clamp(a,0.,1.),std::min(std::clamp(b,0.,1.),1-std::clamp(a,0.,1.)));const double height=s.level==0?0:scale*(s.level==3?simplexKl(point,q):simplexFunction(f,point,outcomes));near(v.position.y,height,2e-6,"surface height derived from probability");}
    for(unsigned i=0;i<s.solid.indexCount;i+=3){const auto a=s.solid.vertices[s.solid.indices[i]].position,b=s.solid.vertices[s.solid.indices[i+1]].position,c=s.solid.vertices[s.solid.indices[i+2]].position;const auto normal=iggy3d::cross(b-a,c-a);require(normal.y>0,"surface winding");area+=normal.y/2;}
    near(area,4*std::sqrt(3.),2e-6,"projected triangle area without gaps/overlap");
  }
}
void model(){
  MathObjects m;MathObjectScene scene;act(m,{MathActionKind::Select,K::Simplex});require(mathLessons(K::Simplex).size()==4&&m.playbackParameter()==P::SimplexMix,"simplex registration");
  const auto specs=mathParameterSpecs();std::set<std::string_view> keys;for(unsigned i=0;i<specs.size();++i)require(static_cast<unsigned>(specs[i].id)==i&&keys.insert(specs[i].key).second,"stable parameter registry");
  require(static_cast<unsigned>(K::Truss)==30&&static_cast<unsigned>(K::Simplex)==31,"existing object ID changed");
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned e=0;e<5;++e){const auto rev=m.snapshot().revision;preset(m,e);require(m.snapshot().revision==rev+1,"preset not atomic");const auto& example=mathObjectPresets(K::Simplex,l)[e];require(example.count==10,"incomplete preset");for(unsigned j=0;j<example.count;++j)near(m.parameter(example.parameters[j]),example.values[j],0,"preset value");for(double t:{0.,.17,.5,1.}){set(m,P::SimplexMix,t);inspect(m,scene);}if(l==1||l==2)for(unsigned f=0;f<4;++f){set(m,P::SimplexFunction,f);inspect(m,scene);}set(m,P::SimplexGuides,0);inspect(m,scene);}}
  level(m,0);preset(m,0);challenge(m,true);preset(m,1);challenge(m,false);
  level(m,1);preset(m,0);challenge(m,true);preset(m,1);challenge(m,false);
  level(m,2);preset(m,1);challenge(m,true);set(m,P::SimplexMix,0);challenge(m,false);set(m,P::SimplexMix,.5);set(m,P::SimplexFunction,1);challenge(m,false);
  level(m,3);preset(m,3);challenge(m,true);preset(m,0);challenge(m,false);preset(m,4);challenge(m,false);require(m.snapshot().solid.indexCount==0&&metric(m,"Q support dimension")==1,"boundary Q invented full surface");require(metric(m,"KL(P||Q) finite")==0,"boundary infinity hidden");require(m.snapshot().plots[1].series[0].count==1,"infinite KL values plotted");
  // Both P and Q lie on Q's supported edge: a full finite curve is valid.
  preset(m,1);set(m,P::SimplexP0,.5);set(m,P::SimplexP1,.5);set(m,P::SimplexQ0,.2);set(m,P::SimplexQ1,.8);inspect(m,scene);require(m.snapshot().plots[1].series[0].count==129,"finite shared edge curve missing");
  set(m,P::SimplexQ1,0);set(m,P::SimplexQ0,1);inspect(m,scene);require(metric(m,"Q support dimension")==0&&!m.snapshot().solid.indexCount,"vertex KL domain");
  level(m,0);preset(m,0);const auto range=mathControlRange(m,P::SimplexP0);set(m,P::SimplexP0,range.maximum);near(m.parameter(P::SimplexP0)+m.parameter(P::SimplexP1),1,0,"slider endpoint cannot reach boundary");reject(m,{MathActionKind::SetParameter,{},P::SimplexP0,1});
  set(m,P::SimplexP0,.1);set(m,P::SimplexP1,.8);MathAction partial{MathActionKind::ResetParameters};partial.resetParameters.set(static_cast<unsigned>(P::SimplexP0));reject(m,partial);act(m,mathResetControlGroup(m,MathControlGroup::ShapeA));near(m.parameter(P::SimplexP0),1./3,0,"paired reset");near(m.parameter(P::SimplexP1),1./3,0,"paired reset other coordinate");
  set(m,P::SimplexMix,.23);level(m,2);set(m,P::SimplexFunction,3);level(m,0);level(m,2);near(m.parameter(P::SimplexFunction),3,0,"layer lost selected surface");near(m.parameter(P::SimplexMix),.23,1e-15,"layer lost mixture");
  set(m,P::SimplexMix,0);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},2});near(m.parameter(P::SimplexMix),.24,1e-15,"mixture playback rate");set(m,P::SimplexValue0,-2);require(!m.snapshot().playing,"edit did not pause playback");set(m,P::SimplexMix,.99);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},1});near(m.parameter(P::SimplexMix),1,0,"mixture playback endpoint");require(!m.snapshot().playing,"playback did not stop");reject(m,{MathActionKind::TogglePlayback});
  reject(m,{MathActionKind::SetParameter,{},P::SimplexP0,std::numeric_limits<double>::quiet_NaN()});reject(m,{MathActionKind::SetParameter,{},P::TrussPosition,.1});
  for(unsigned l=1;l<4;++l){level(m,l);for(unsigned mask=1;mask<8;++mask){preset(m,3);for(auto param:{P::SimplexP0,P::SimplexP1,P::SimplexQ0,P::SimplexQ1})set(m,param,0);const unsigned count=((mask&1)!=0)+((mask&2)!=0)+((mask&4)!=0);if(mask&1){set(m,P::SimplexP0,count==3?.33:1./count);set(m,P::SimplexQ0,count==3?.33:1./count);}if(mask&2){set(m,P::SimplexP1,count==3?.33:1./count);set(m,P::SimplexQ1,count==3?.33:1./count);}for(bool large:{false,true}){set(m,P::SimplexValue0,large?-3:0);set(m,P::SimplexValue1,large?3:0);set(m,P::SimplexValue2,large?-3:0);inspect(m,scene);}}}
  act(m,{MathActionKind::Select,K::Algebra});require(m.snapshot().solid.indexCount==0&&m.playbackParameter()==P::Count,"simplex state leaked");
}
}
int main(){try{mathematics();model();std::printf("simplex CPU checks: %u assertions, %u scenes; maxima %zu vertices / %zu indices; KL oracle error %.4g, supporting-gap error %.4g; no host, fonts or images\n",checks,scenes,maxVertices,maxIndices,maxKlError,maxGapError);return 0;}catch(const std::exception& e){std::fprintf(stderr,"simplex failure: %s\n",e.what());return 1;}}

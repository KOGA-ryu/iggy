#include "runtime/math_objects/Membrane.hpp"
#include "runtime/math_objects/MembraneGeometry.hpp"
#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include "ui/MathObjectLayout.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using namespace paths;using P=MathParameter;using K=MathObjectKind;
constexpr double pi=3.14159265358979323846;
unsigned assertions=0,meshes=0;std::size_t maxVertices=0,maxIndices=0;
void require(bool b,const char* why){++assertions;if(!b)throw std::runtime_error(why);}
void near(double a,double b,double tolerance,const char* why){++assertions;if(!std::isfinite(a)||!std::isfinite(b)||std::fabs(a-b)>tolerance)throw std::runtime_error(std::string(why)+": "+std::to_string(a)+" versus "+std::to_string(b));}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}require(rejected,"invalid kernel request accepted");}

// Independent numerical initial-value oracle, with no closed-form branches.
std::array<double,2> rk4(double q,double v,double omega,double gamma,double t){
  const unsigned n=std::max(1U,static_cast<unsigned>(std::ceil(t/0.0001)));const double h=t/n;
  const auto rate=[&](std::array<double,2> s){return std::array<double,2>{s[1],-omega*omega*s[0]-2*gamma*s[1]};};
  const auto add=[](std::array<double,2> a,std::array<double,2> b,double scale){return std::array<double,2>{a[0]+scale*b[0],a[1]+scale*b[1]};};
  std::array<double,2> state{q,v};
  for(unsigned i=0;i<n;++i){const auto a=rate(state),b=rate(add(state,a,h/2)),c=rate(add(state,b,h/2)),d=rate(add(state,c,h));for(unsigned k=0;k<2;++k)state[k]+=h*(a[k]+2*b[k]+2*c[k]+d[k])/6;}
  return state;
}
void motion(){
  MembraneInput input;input.width=input.depth=4;input.tension=.5;input.modes[0]={1,1,.25,-.3};
  const double omega=pi/4;
  for(double gamma:{0.,.2,omega*(1-1e-9),omega,omega*(1+1e-9),1.8})for(double t:{0.,.01,.3,2.5,12.}){
    input.damping=gamma;const auto state=prepareMembrane(input,t);const auto& mode=state.slots[0];const auto expected=rk4(.25,-.3,omega,gamma,t);
    near(mode.omega,omega,1e-14,"natural frequency");near(mode.q,expected[0],3e-12,"closed-form displacement versus RK4");near(mode.velocity,expected[1],3e-12,"closed-form velocity versus RK4");
    require(state.kinetic>=0&&state.potential>=0&&state.kinetic+state.potential<=state.initialEnergy+1e-12,"energy increased in passive motion");
    if(gamma==0)near(state.kinetic+state.potential,state.initialEnergy,2e-14,"undamped energy drift");
    if(t==0){near(mode.q,.25,1e-15,"initial displacement");near(mode.velocity,-.3,1e-15,"initial velocity");}
  }
  input.width=input.depth=1;input.tension=4;input.density=.5;input.damping=.3;input.modes[0]={6,6,.6,.6};
  const auto fast=prepareMembrane(input,.4);const auto expected=rk4(.6,.6,24*pi,.3,.4);near(fast.slots[0].q,expected[0],2e-9,"fast mode displacement");near(fast.slots[0].velocity,expected[1],2e-7,"fast mode velocity");
  // Critical damping has an independent elementary limit.
  input={};input.width=input.depth=4;input.tension=.5;input.damping=omega;input.modes[0]={1,1,.25,-.3};
  const double t=1.7;near(prepareMembrane(input,t).slots[0].q,std::exp(-omega*t)*(.25+(-.3+omega*.25)*t),2e-14,"critical damping limit");
}
void fields(){
  MembraneInput input;input.width=2.3;input.depth=3.1;input.tension=1.4;input.density=.8;input.damping=.17;
  input.modes={{{1,2,.3,.1},{3,1,-.2,.15},{2,4,.1,-.1},{1,2,.12,-.05}}};
  const double t=.71;const auto state=prepareMembrane(input,t);
  for(double u:{.13,.31,.62,.83})for(double v:{.17,.41,.72}){
    const double h=1e-5;const auto s=sampleMembrane(state,u,v),left=sampleMembrane(state,u-h,v),right=sampleMembrane(state,u+h,v),down=sampleMembrane(state,u,v-h),up=sampleMembrane(state,u,v+h);
    near(s.dx,(right.displacement-left.displacement)/(2*h*input.width),2e-8,"spatial derivative x");near(s.dy,(up.displacement-down.displacement)/(2*h*input.depth),2e-8,"spatial derivative y");
    const double laplacian=(left.displacement-2*s.displacement+right.displacement)/std::pow(h*input.width,2)+(down.displacement-2*s.displacement+up.displacement)/std::pow(h*input.depth,2);
    near(s.laplacian,laplacian,3e-6,"finite-difference Laplacian");
    const auto before=sampleMembrane(prepareMembrane(input,t-h),u,v),after=sampleMembrane(prepareMembrane(input,t+h),u,v);
    near(s.velocity,(after.displacement-before.displacement)/(2*h),2e-8,"temporal derivative");near(s.acceleration,(after.velocity-before.velocity)/(2*h),2e-7,"temporal acceleration");
    near((after.velocity-before.velocity)/(2*h)+2*input.damping*s.velocity,input.tension/input.density*laplacian,6e-6,"independent wave equation residual");
    double sum=0;for(double c:s.contributions)sum+=c;near(sum,s.displacement,1e-14,"slot superposition");
  }
  for(unsigned m=1;m<=6;++m)for(unsigned n=1;n<=6;++n){input.modes[0]={m,n,.3,.1};const auto s=prepareMembrane(input,.4);
    for(unsigned k=0;k<=m;++k)near(sampleMembrane(s,static_cast<double>(k)/m,.27).weights[0],0,4e-15,"u nodal line");
    for(unsigned k=0;k<=n;++k)near(sampleMembrane(s,.43,static_cast<double>(k)/n).weights[0],0,4e-15,"v nodal line");
    for(double f:{0.,.13,.5,.91,1.})for(auto uv:std::array<std::array<double,2>,4>{{{0,f},{1,f},{f,0},{f,1}}}){const auto p=sampleMembrane(s,uv[0],uv[1]);near(p.displacement,0,0,"fixed edge displacement");near(p.velocity,0,0,"fixed edge velocity");}
  }
}
void energies(){
  MembraneInput input;input.width=2;input.depth=3;input.tension=1.2;input.density=.7;input.damping=.23;
  for(unsigned arrangement=0;arrangement<3;++arrangement){
    input.modes={{{1,2,.3,.1},{2,1,.2,-.15},{3,4,-.1,.2},{1,2,.15,-.2}}};
    if(arrangement==1)input.modes[3]={1,2,-.3,-.1};
    if(arrangement==2){input.width=input.depth=3;input.modes[2]={2,1,-.2,.15};input.modes[3]={1,2,-.3,-.1};}
    for(double t:{0.,.3,2.,9.}){
      const auto state=prepareMembrane(input,t);double kinetic=0,potential=0;
      // Independent spatial integration retains cross terms of equal mode pairs.
      constexpr unsigned n=48;for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j){const auto p=sampleMembrane(state,(i+.5)/n,(j+.5)/n);kinetic+=p.kineticDensity;potential+=p.potentialDensity;}
      const double cell=input.width*input.depth/(n*n);near(state.kinetic,kinetic*cell,2e-12,"global kinetic versus spatial integral");near(state.potential,potential*cell,2e-12,"global strain versus spatial integral");
      if(t>0){const double h=1e-5;const auto a=prepareMembrane(input,t-h),b=prepareMembrane(input,t+h);near((b.kinetic+b.potential-a.kinetic-a.potential)/(2*h),-state.lossRate,1e-7,"dissipation law");}
      if(arrangement==2){near(state.initialEnergy,0,0,"cancelled duplicate initial energy");near(state.kinetic+state.potential,0,0,"cancelled duplicate energy");require(state.activeModes==0,"cancelled modes counted active");}
    }
  }
  input={};input.modes={{{1,2,.2,0},{2,1,.3,0},{1,1,0,0},{1,1,0,0}}};const auto degenerate=prepareMembrane(input,0);require(degenerate.activeModes==2,"equal frequencies merged distinct spatial modes");near(degenerate.slots[0].omega,degenerate.slots[1].omega,0,"square degeneracy");
  input.damping=.4;const auto initial=prepareMembrane(input,0),final=prepareMembrane(input,4);double loss=0;
  constexpr unsigned steps=4096;for(unsigned i=0;i<=steps;++i){const double weight=(i==0||i==steps)?1:(i%2?4:2);loss+=weight*prepareMembrane(input,4.*i/steps).lossRate;}
  loss*=4./steps/3;near(loss,initial.initialEnergy-final.kinetic-final.potential,2e-10,"integrated dissipated power");
  input={};input.width=input.depth=2;const auto original=prepareMembrane(input,0);input.width=input.depth=4;near(prepareMembrane(input,0).slots[0].omega,original.slots[0].omega/2,1e-14,"size frequency scaling");near(prepareMembrane(input,0).potential,original.potential,1e-14,"size strain energy scaling");
  input={};input.tension=.5;const auto tension=prepareMembrane(input,0);input.tension=2;near(prepareMembrane(input,0).slots[0].omega,2*tension.slots[0].omega,1e-14,"tension scaling");
}
void invalid(){
  for(double value:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}){
    for(unsigned field=0;field<8;++field)rejects([&]{MembraneInput input;double t=0;switch(field){case 0:input.width=value;break;case 1:input.depth=value;break;case 2:input.tension=value;break;case 3:input.density=value;break;case 4:input.damping=value;break;case 5:input.modes[0].displacement=value;break;case 6:input.modes[0].velocity=value;break;case 7:t=value;break;}(void)prepareMembrane(input,t);});
  }
  rejects([]{MembraneInput p;p.modes[0].m=0;(void)prepareMembrane(p,0);});rejects([]{MembraneInput p;p.modes[0].n=7;(void)prepareMembrane(p,0);});
  rejects([]{MembraneInput p;p.tension=0;(void)prepareMembrane(p,0);});rejects([]{MembraneInput p;p.density=0;(void)prepareMembrane(p,0);});rejects([]{(void)prepareMembrane({},12.1);});
  const auto state=prepareMembrane({},0);rejects([&]{(void)sampleMembrane(state,-.1,.5);});rejects([&]{(void)sampleMembrane(state,.5,1.1);});
  MathTriangleSurface mesh;mesh.indexCount=3;mesh.vertexCount=1;rejects([&]{buildMembraneSurface(state,{49},mesh);});require(mesh.indexCount==3&&mesh.vertexCount==1,"invalid geometry request mutated output");
}
void act(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double value){act(m,{MathActionKind::SetParameter,{},p,value});}
void level(MathObjects& m,unsigned value){act(m,{MathActionKind::SetLevel,{},{},static_cast<double>(value)});}
void preset(MathObjects& m,unsigned value){act(m,{MathActionKind::ObjectPreset,{},{},0,value});}
double metric(const MathObjects& m,std::string_view key){for(unsigned i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==key)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric "+std::string(key));}
void challenge(MathObjects& m,bool pass){act(m,{MathActionKind::Check});require((m.snapshot().feedback==MathFeedback::Solved)==pass,"incorrect membrane challenge result");}
MembraneInput input(const MathObjects& m){MembraneInput out;out.width=m.parameter(P::MembraneWidth);out.depth=m.parameter(P::MembraneDepth);out.tension=m.parameter(P::MembraneTension);out.density=m.parameter(P::MembraneDensity);out.damping=m.parameter(P::MembraneDamping);for(unsigned i=0;i<4;++i){const auto k=static_cast<unsigned>(P::MembraneM0)+4*i;out.modes[i]={static_cast<unsigned>(m.parameter(static_cast<P>(k))),static_cast<unsigned>(m.parameter(static_cast<P>(k+1))),m.parameter(static_cast<P>(k+2)),m.parameter(static_cast<P>(k+3))};}return out;}
void inspect(MathObjects& m,MathObjectScene& scene){
  const auto& s=m.snapshot();const auto& frame=scene.publish(s,{0,0,1000,700});++meshes;maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());require(frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"renderer capacity exceeded");require(!frame.vertices.empty()&&iggy3d::isFinite(frame.clipFromWorld),"empty or invalid scene");
  for(const auto& v:frame.vertices){for(float x:v.position)require(std::isfinite(x),"nonfinite vertex");for(float x:v.color)require(std::isfinite(x)&&x>=0&&x<=1,"invalid vertex colour");}for(unsigned index:frame.indices)require(index<frame.vertices.size(),"invalid renderer index");
  std::set<unsigned> ids;unsigned end=0;for(auto d:frame.draws){require(ids.insert(d.objectId.value).second&&d.firstIndex==end,"draw ownership");end+=d.indexCount;}require(end==frame.indices.size(),"unowned indices");
  const auto& mesh=s.solid;const unsigned n=static_cast<unsigned>(m.parameter(P::MembraneResolution));require(mesh.vertexCount==(n+1)*(n+1)&&mesh.indexCount==6*n*n,"membrane mesh counts");
  auto shown=input(m);if(m.parameter(P::MembraneView)==1)for(unsigned i=0;i<4;++i)if(i!=m.parameter(P::MembraneSlot))shown.modes[i].displacement=shown.modes[i].velocity=0;const auto state=prepareMembrane(shown,m.parameter(P::MembraneTime));
  for(unsigned i=0;i<=n;++i)for(unsigned j=0;j<=n;++j){const auto& vertex=mesh.vertices[i*(n+1)+j];const auto p=sampleMembrane(state,static_cast<double>(i)/n,static_cast<double>(j)/n);near(vertex.position.y,p.displacement,3e-7,"published height");near(iggy3d::length(vertex.normal),1,2e-7,"unit normal");near(vertex.normal.x+vertex.normal.y*p.dx,0,2e-6,"normal perpendicular to x tangent");near(vertex.normal.z-vertex.normal.y*p.dy,0,2e-6,"normal perpendicular to y tangent");require(vertex.normal.y>0,"normal points below sheet");if(!i||!j||i==n||j==n)near(vertex.position.y,0,0,"mesh edge moved");}
  std::map<std::pair<unsigned,unsigned>,std::pair<unsigned,int>> edges;
  for(unsigned i=0;i<mesh.indexCount;i+=3){const unsigned a=mesh.indices[i],b=mesh.indices[i+1],c=mesh.indices[i+2];require(a<mesh.vertexCount&&b<mesh.vertexCount&&c<mesh.vertexCount,"surface index");const auto cross=iggy3d::cross(mesh.vertices[b].position-mesh.vertices[a].position,mesh.vertices[c].position-mesh.vertices[a].position);require(cross.y>0,"triangle winding");for(auto e:std::array<std::pair<unsigned,unsigned>,3>{{{a,b},{b,c},{c,a}}}){auto& value=edges[std::minmax(e.first,e.second)];++value.first;value.second+=e.first<e.second?1:-1;}}
  unsigned boundary=0;for(auto [key,value]:edges){require(value.first==1||value.first==2,"nonmanifold edge");if(value.first==1)++boundary;else require(value.second==0,"inconsistent winding");}require(boundary==4*n,"boundary count");require(static_cast<int>(mesh.vertexCount)-static_cast<int>(edges.size())+static_cast<int>(mesh.indexCount/3)==1,"disk topology");
  for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"invalid metric");for(unsigned i=0;i<s.table.rowCount;++i)for(unsigned j=0;j<s.table.columnCount;++j)require(std::isfinite(s.table.values[i][j]),"invalid table");
  for(unsigned i=0;i<s.plotCount;++i){const auto& plot=s.plots[i];for(unsigned j=0;j<plot.seriesCount;++j)for(unsigned k=0;k<plot.series[j].count;++k){const auto p=plot.series[j].points[k];require(std::isfinite(p.x)&&std::isfinite(p.y),"invalid plot");if(plot.scrubParameter==P::MembraneTime)require(p.x>=0&&p.x<=12,"time plot outside domain");}}
}
void model(){
  MathObjects m;MathObjectScene scene;act(m,{MathActionKind::Select,K::Membrane});require(mathLessons(K::Membrane).size()==4&&m.playbackParameter()==P::MembraneTime,"membrane registration");
  std::set<std::string_view> keys;for(const auto& p:mathParameterSpecs())require(keys.insert(p.key).second,"duplicate parameter key");
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned p=0;p<4;++p){const auto revision=m.snapshot().revision;preset(m,p);require(m.snapshot().revision==revision+1,"non-atomic preset");const auto& example=mathObjectPresets(K::Membrane,l)[p];require(example.count==28,"incomplete preset");for(unsigned i=0;i<example.count;++i)near(m.parameter(example.parameters[i]),example.values[i],0,"preset values");
    for(unsigned n:{12U,32U,48U}){set(m,P::MembraneResolution,n);set(m,P::MembraneTime,.7);inspect(m,scene);set(m,P::MembraneView,1);inspect(m,scene);set(m,P::MembraneView,0);}
    set(m,P::MembraneGuides,0);require(m.snapshot().partCount==0,"guides remain in shape-only view");inspect(m,scene);
  }}
  level(m,0);preset(m,0);MathInspectorMemory memory;memory.visit(m);memory.rememberExample(m,"Drumhead");
  for(unsigned slot=0;slot<4;++slot){set(m,P::MembraneSlot,slot);const auto rows=mathControlRows(m);unsigned controls=0;for(unsigned i=0;i<rows.count;++i)for(unsigned j=0;j<rows.rows[i].count;++j){const auto p=rows.rows[i].parameters[j];if(p>=P::MembraneM0&&p<=P::MembraneV3){require((static_cast<unsigned>(p)-static_cast<unsigned>(P::MembraneM0))/4==slot,"wrong mode controls visible");++controls;}}require(controls==4,"selected mode controls missing");require(memory.exampleTitle(m)=="Drumhead","slot selection changed example title");}
  set(m,P::MembraneA3,.2);require(memory.exampleTitle(m)=="Custom","mode edit did not customize example");const auto rev=m.snapshot().revision;act(m,mathResetControlGroup(m,MathControlGroup::Profile));require(m.snapshot().revision==rev+1&&mathChangedControlCount(m,MathControlGroup::Profile)==0,"profile reset missed hidden slots");
  set(m,P::MembraneA2,.24);set(m,P::MembraneTime,1.23);level(m,3);level(m,0);near(m.parameter(P::MembraneA2),.24,1e-15,"layer switch lost modes");near(m.parameter(P::MembraneTime),1.23,1e-14,"layer switch lost time");
  preset(m,0);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},2});near(m.parameter(P::MembraneTime),1,0,"half-speed playback");set(m,P::MembraneU,.4);require(!m.snapshot().playing,"edit left playback running");set(m,P::MembraneTime,11.9);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},1});near(m.parameter(P::MembraneTime),12,0,"time end clamp");require(!m.snapshot().playing&&!m.dispatch({MathActionKind::TogglePlayback}).accepted,"playback passed endpoint");
  preset(m,0);challenge(m,false);set(m,P::MembraneTime,2);challenge(m,true);level(m,1);preset(m,1);challenge(m,true);set(m,P::MembraneU,0);challenge(m,false);set(m,P::MembraneU,.25);set(m,P::MembraneV,.25);challenge(m,false);
  level(m,1);preset(m,1);set(m,P::MembraneA0,0);challenge(m,false);set(m,P::MembraneV0,.01);challenge(m,true);
  level(m,2);preset(m,2);challenge(m,true);set(m,P::MembraneU,.5);challenge(m,false);level(m,3);preset(m,3);challenge(m,false);set(m,P::MembraneTime,6);challenge(m,true);
  const auto before=m.snapshot().revision;require(!m.dispatch({MathActionKind::SetParameter,{},P::MembraneDensity,0}).accepted&&!m.dispatch({MathActionKind::SetParameter,{},P::PatchU,.3}).accepted&&!m.dispatch({MathActionKind::SetParameter,{},P::MembraneTime,std::numeric_limits<double>::quiet_NaN()}).accepted,"invalid model edit accepted");require(m.snapshot().revision==before,"rejected edit mutated model");
  // Geometry/display edits do not enter the analytic energy calculation.
  const double energy=metric(m,"Total energy");set(m,P::MembraneResolution,12);set(m,P::MembraneView,1);set(m,P::MembraneGuides,0);near(metric(m,"Total energy"),energy,0,"display changed global energy");
  for(unsigned i=0;i<4;++i){set(m,static_cast<P>(static_cast<unsigned>(P::MembraneA0)+4*i),0);set(m,static_cast<P>(static_cast<unsigned>(P::MembraneV0)+4*i),0);}near(metric(m,"Total energy"),0,0,"zero membrane energy");challenge(m,false);inspect(m,scene);
  // Every public parameter limit, including maximum mode counts and damping.
  for(unsigned l=0;l<4;++l){level(m,l);preset(m,0);for(bool high:{false,true}){for(const auto& p:mathParameterSpecs())if(p.owner==K::Membrane)set(m,p.id,high?p.maximum:p.minimum);inspect(m,scene);}}
  level(m,0);preset(m,0);set(m,P::MembraneWidth,4);set(m,P::MembraneDepth,4);set(m,P::MembraneTension,.25);set(m,P::MembraneDensity,2);set(m,P::MembraneResolution,48);
  for(unsigned i=0;i<4;++i){set(m,static_cast<P>(static_cast<unsigned>(P::MembraneM0)+4*i),1);set(m,static_cast<P>(static_cast<unsigned>(P::MembraneN0)+4*i),1);set(m,static_cast<P>(static_cast<unsigned>(P::MembraneA0)+4*i),0);set(m,static_cast<P>(static_cast<unsigned>(P::MembraneV0)+4*i),.6);}set(m,P::MembraneTime,4);require(metric(m,"Combined displacement")>6,"low-frequency velocity response missing");inspect(m,scene);
  level(m,0);preset(m,3);const auto pluck=input(m);
  // Independent projection of a piecewise-linear tent; split quadrature at .5.
  for(unsigned slot=0;slot<4;++slot){const auto mode=pluck.modes[slot];const auto coefficient=[](unsigned k){constexpr unsigned n=1024;double sum=0;for(unsigned i=0;i<=n;++i){const double u=static_cast<double>(i)/n,w=(i==0||i==n)?1:(i%2?4:2);sum+=w*(1-2*std::fabs(u-.5))*std::sin(k*pi*u);}return 2*sum/(3*n);};near(mode.displacement,.5*coefficient(mode.m)*coefficient(mode.n),1e-10,"tent projection coefficient");near(mode.velocity,0,0,"pluck not released from rest");}
  act(m,{MathActionKind::Select,K::Patch});require(m.snapshot().curve.count==16,"patch picker regression");act(m,{MathActionKind::Select,K::Curve});require(m.snapshot().curve.count==4,"curve picker regression");act(m,{MathActionKind::Select,K::Algebra});require(m.snapshot().solid.indexCount==0&&m.playbackParameter()==P::Count,"membrane state leaked");
}
}
int main(){try{motion();fields();energies();invalid();model();std::printf("membrane CPU checks: %u assertions, %u mesh states; maxima %zu vertices / %zu indices; no host, fonts or images\n",assertions,meshes,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"membrane failure: %s\n",e.what());return 1;}}

#include "runtime/math_objects/BooleanSolid.hpp"
#include "runtime/math_objects/BooleanGeometry.hpp"
#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
using namespace paths;
namespace {
constexpr double pi=3.14159265358979323846;
unsigned certificates=0,meshes=0;std::size_t maxVertices=0,maxIndices=0;
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
void near(double actual,double expected,double tolerance,const char* message){if(!std::isfinite(actual)||std::fabs(actual-expected)>tolerance)throw std::runtime_error(std::string(message)+": "+std::to_string(actual)+" versus "+std::to_string(expected));}
double norm(SolidPoint x){return std::sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]);}
void kernel(){
  BooleanInput input;auto solid=prepareBoolean(input);
  near(samplePrimitive(solid.a,{0,0,0}).value,-.7,1e-14,"box center distance");near(samplePrimitive(solid.a,{1.5,1.5,1.2}).value,std::sqrt(.5),1e-14,"box corner distance");near(samplePrimitive(solid.b,{0,0,0}).value,-.55,1e-14,"cylinder center distance");near(samplePrimitive(solid.b,{1,0,2}).value,std::hypot(.45,.35),1e-14,"cylinder rim distance");
  require(!samplePrimitive(solid.a,{0,0,0}).regular,"box medial ambiguity");require(!samplePrimitive(solid.b,{0,0,0}).regular,"cylinder axis ambiguity");
  for(unsigned operation=0;operation<4;++operation)for(unsigned mask=0;mask<4;++mask){const bool a=mask&2,b=mask&1;const bool expected=operation==1?(a&&b):operation==2?(a&&!b):(a||b);require(booleanTruth(static_cast<SolidOperation>(operation),a,b)==expected,"Boolean truth table");++certificates;}
  // Independent point membership, away from exact boundaries: a rectangular box and a capped cylinder.
  for(unsigned op=0;op<3;++op){input.operation=static_cast<SolidOperation>(op);solid=prepareBoolean(input);for(unsigned i=0;i<600;++i){const SolidPoint p{1.9*std::sin(i*1.317),1.7*std::cos(i*.741),2.3*std::sin(i*1.123+.4)};const bool a=std::fabs(p[0])<1.2&&std::fabs(p[1])<1.1&&std::fabs(p[2])<.7,b=p[0]*p[0]+p[1]*p[1]<.55*.55&&std::fabs(p[2])<1.65;const bool expected=op==0?(a||b):op==1?(a&&b):(a&&!b);require((sampleBoolean(solid,p).value<0)==expected,"independent set membership");++certificates;}}
  // Compare analytic gradients with independent central differences after rigid transformations.
  for(unsigned shape=0;shape<4;++shape)for(unsigned op=0;op<4;++op){input.b.shape=static_cast<SolidShape>(shape);input.b.center={.3,-.2,.15};input.b.yaw=35;input.b.pitch=-20;input.operation=static_cast<SolidOperation>(op);solid=prepareBoolean(input);
    for(unsigned i=0;i<45;++i){const SolidPoint p{1.4*std::sin(i*.837),1.6*std::cos(i*1.142),1.1*std::sin(i*1.913+.2)};const auto s=sampleBoolean(solid,p);if(!s.regular)continue;for(unsigned j=0;j<3;++j){auto left=p,right=p;left[j]-=1e-6;right[j]+=1e-6;near(s.gradient[j],(sampleBoolean(solid,right).value-sampleBoolean(solid,left).value)/2e-6,2e-5,"finite difference gradient");}++certificates;}
    // The finite domain must contain the full negative region, including blends.
    for(unsigned axis=0;axis<3;++axis)for(unsigned side=0;side<2;++side)for(unsigned i=0;i<7;++i)for(unsigned j=0;j<7;++j){SolidPoint p{};p[axis]=side?solid.bounds.maximum[axis]:solid.bounds.minimum[axis];const auto u=(axis+1)%3,v=(axis+2)%3;p[u]=solid.bounds.minimum[u]+(solid.bounds.maximum[u]-solid.bounds.minimum[u])*i/6;p[v]=solid.bounds.minimum[v]+(solid.bounds.maximum[v]-solid.bounds.minimum[v])*j/6;require(sampleBoolean(solid,p).value>0,"surface touches domain boundary");}
  }
  input={};input.a.shape=input.b.shape=SolidShape::Sphere;input.a.center={-.6,0,0};input.b.center={.6,0,0};input.a.size=input.b.size=1;input.operation=SolidOperation::Union;solid=prepareBoolean(input);const auto sharp=sampleBoolean(solid,{0,.8,0});near(sharp.value,0,1e-14,"sphere seam");require(!sharp.regular,"hard seam normal claimed unique");input.operation=SolidOperation::SmoothUnion;input.blend=.4;solid=prepareBoolean(input);const auto smooth=sampleBoolean(solid,{0,.8,0});near(smooth.value,-.1,1e-14,"polynomial blend at equal values");require(smooth.regular,"regular blend normal missing");near(smooth.gradient[0],0,1e-14,"symmetric blend gradient");near(smooth.gradient[1],.8,1e-14,"blend is not exact distance");require(!sampleBoolean(solid,{0,0,0}).regular,"zero gradient claimed regular");
  input.blend=0;solid=prepareBoolean(input);near(sampleBoolean(solid,{0,.8,0}).value,sharp.value,0,"zero width hard-union limit");require(!sampleBoolean(solid,{0,.8,0}).regular,"zero width seam");
  input={};input.a.shape=input.b.shape=SolidShape::Sphere;input.a.size=input.b.size=1;input.operation=SolidOperation::Difference;solid=prepareBoolean(input);near(measureBooleanVolume(solid,16).volume,0,0,"identical subtraction empty");
  input={};solid=prepareBoolean(input);const double drill=8*1.2*1.1*.7-pi*.55*.55*1.4;near(measureBooleanVolume(solid,64).volume,drill,.16,"analytic drilled block volume");
  input.a.shape=SolidShape::Sphere;input.b.shape=SolidShape::Sphere;input.a.size=input.b.size=1;input.operation=SolidOperation::Union;solid=prepareBoolean(input);near(measureBooleanVolume(solid,64).volume,4*pi/3,.025,"analytic sphere volume");const auto crossing=probeBooleanSurface(solid,{.2,.3,.4});require(crossing.found&&crossing.sample.regular,"sphere surface probe");near(crossing.position[0],std::sqrt(.75),1e-10,"independent surface intersection");near(crossing.sample.value,0,1e-11,"surface root residual");require(!probeBooleanSurface(solid,{0,2,0}).found,"empty probe line fabricated crossing");
  for(auto invalid:{std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN(),-1.0,2.0}){auto bad=input;bad.b.size=invalid;bool rejected=false;try{(void)prepareBoolean(bad);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"invalid primitive accepted");}
}
void manifold(const MathTriangleSurface& m){
  std::map<std::pair<unsigned,unsigned>,std::pair<unsigned,int>> edges;
  for(unsigned i=0;i<m.indexCount;i+=3){unsigned a=m.indices[i],b=m.indices[i+1],c=m.indices[i+2];require(a<m.vertexCount&&b<m.vertexCount&&c<m.vertexCount,"mesh indices");const auto p=m.vertices[a].position,q=m.vertices[b].position,r=m.vertices[c].position;require(iggy3d::lengthSquared(iggy3d::cross(q-p,r-p))>0,"degenerate triangle");for(auto edge:{std::pair{a,b},std::pair{b,c},std::pair{c,a}}){auto& record=edges[std::minmax(edge.first,edge.second)];++record.first;record.second+=edge.first<edge.second?1:-1;}}
  for(auto [edge,count]:edges)if(count.first!=2||count.second!=0)throw std::runtime_error("open or inconsistent edge "+std::to_string(edge.first)+","+std::to_string(edge.second)+" count="+std::to_string(count.first)+" winding="+std::to_string(count.second));
  for(unsigned i=0;i<m.vertexCount;++i){const auto& v=m.vertices[i];near(iggy3d::length(v.normal),1,3e-7,"unit mesh normal");require(iggy3d::isFinite(v.position)&&iggy3d::isFinite(v.color),"finite mesh");}++certificates;
}
void geometry(){MathTriangleSurface mesh;BooleanInput input;auto solid=prepareBoolean(input);const double drill=8*1.2*1.1*.7-pi*.55*.55*1.4;
  for(unsigned n:{12U,20U,28U}){const auto full=buildBooleanSurface(solid,{n,false,0},mesh);manifold(mesh);near(full.volume,drill,.35,"closed drilled mesh volume");const auto half=buildBooleanSurface(solid,{n,true,0},mesh);manifold(mesh);near(half.volume,drill/2,.18,"analytic half-section mesh volume");for(unsigned i=0;i<mesh.vertexCount;++i)require(mesh.vertices[i].position.z<=1e-7,"section did not clip");const auto empty=buildBooleanSurface(solid,{n,true,-3},mesh);near(empty.volume,0,0,"fully hidden section volume");require(!mesh.indexCount,"hidden section mesh");}
  input.a.shape=input.b.shape=SolidShape::Sphere;input.a.size=input.b.size=1;input.operation=SolidOperation::Union;solid=prepareBoolean(input);for(unsigned n:{12U,20U,28U}){const auto report=buildBooleanSurface(solid,{n},mesh);require(report.cells==n,"sphere mesh lost requested detail");manifold(mesh);require(report.volume>4&&report.volume<4*pi/3,"inscribed sphere mesh volume");}
  for(unsigned shape=0;shape<4;++shape)for(unsigned op=0;op<4;++op){input={};input.b.shape=static_cast<SolidShape>(shape);input.b.center={.23,.13,.17};input.b.yaw=25;input.b.pitch=-15;input.operation=static_cast<SolidOperation>(op);solid=prepareBoolean(input);const auto report=buildBooleanSurface(solid,{28},mesh);manifold(mesh);require(report.volume>0&&report.cells>=4&&report.cells<=28,"valid bounded mesh");++meshes;}
  // Rotated, non-coincident pairs exercise face saddles and closed section caps.
  for(unsigned a=0;a<4;++a)for(unsigned b=0;b<4;++b)for(unsigned op=0;op<4;++op)for(bool section:{false,true}){
    input={};input.a.shape=static_cast<SolidShape>(a);input.a.yaw=15;input.a.pitch=-10;input.a.size=.8;
    input.b.shape=static_cast<SolidShape>(b);input.b.center={.47,-.13,.29};input.b.size=.63;input.b.yaw=-35;input.b.pitch=25;input.operation=static_cast<SolidOperation>(op);input.blend=.34;
    solid=prepareBoolean(input);const auto report=buildBooleanSurface(solid,{28,section,.173},mesh);try{manifold(mesh);}catch(const std::exception& e){throw std::runtime_error("pair A="+std::to_string(a)+" B="+std::to_string(b)+" op="+std::to_string(op)+" section="+std::to_string(section)+": "+e.what());}require(report.volume>=0,"stress mesh orientation");++meshes;
  }
  input={};input.a.shape=input.b.shape=SolidShape::Sphere;input.a.size=input.b.size=1;input.a.center={-1,0,0};input.b.center={1,0,0};input.operation=SolidOperation::Intersection;solid=prepareBoolean(input);(void)buildBooleanSurface(solid,{28},mesh);require(mesh.indexCount==0,"zero-thickness contact has a material mesh");

}
void action(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,MathParameter p,double v){action(m,{MathActionKind::SetParameter,{},p,v});}
void level(MathObjects& m,unsigned n){action(m,{MathActionKind::SetLevel,{},{},static_cast<double>(n)});}
void preset(MathObjects& m,unsigned p){action(m,{MathActionKind::ObjectPreset,{},{},0,p});}
double metric(const MathObjects& m,std::string_view name){for(unsigned i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==name)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric "+std::string(name));}
void checked(MathObjects& m,bool solved){action(m,{MathActionKind::Check});if((m.snapshot().feedback==MathFeedback::Solved)!=solved)throw std::runtime_error("challenge "+std::to_string(m.snapshot().level)+": "+std::string(m.snapshot().feedbackText));}
void reject(MathObjects& m,MathAction a){const auto revision=m.snapshot().revision;const auto feedback=m.snapshot().feedback;std::array<double,static_cast<unsigned>(MathParameter::Count)> values{};for(auto p:mathParameterSpecs())values[static_cast<unsigned>(p.id)]=m.parameter(p.id);require(!m.dispatch(a).accepted,"invalid action accepted");require(m.snapshot().revision==revision&&m.snapshot().feedback==feedback,"rejection mutated snapshot");for(auto p:mathParameterSpecs())near(m.parameter(p.id),values[static_cast<unsigned>(p.id)],0,"rejection mutated parameter");}
void inspect(const MathObjects& m,MathObjectScene& scene){const auto& s=m.snapshot();const auto& f=scene.publish(s,{0,0,900,650});++meshes;maxVertices=std::max(maxVertices,f.vertices.size());maxIndices=std::max(maxIndices,f.indices.size());require(!f.vertices.empty()&&f.vertices.size()<=kSceneVertexCapacity&&f.indices.size()<=kSceneIndexCapacity,"scene capacity");require(iggy3d::isFinite(f.clipFromWorld),"finite camera");std::set<unsigned> ids;unsigned covered=0;for(const auto& d:f.draws){require(d.firstIndex==covered&&d.indexCount%3==0&&ids.insert(d.objectId.value).second,"draw ownership");covered+=d.indexCount;}require(covered==f.indices.size(),"index ownership");for(auto i:f.indices)require(i<f.vertices.size(),"scene index range");for(const auto& v:f.vertices)for(auto x:v.position)require(std::isfinite(x),"finite vertex");for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"finite metric");for(unsigned i=0;i<s.plotCount;++i)for(unsigned j=0;j<s.plots[i].seriesCount;++j)for(unsigned k=0;k<s.plots[i].series[j].count;++k){const auto p=s.plots[i].series[j].points[k];require(std::isfinite(p.x)&&std::isfinite(p.y),"finite plot");}}
void model(){MathObjects m;MathObjectScene scene;action(m,{MathActionKind::Select,MathObjectKind::Boolean});require(mathObjectSpecs().size()==static_cast<std::size_t>(MathObjectKind::Count)&&mathLessons(MathObjectKind::Boolean).size()==4,"Boolean registration");
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned p=0;p<4;++p){const auto rev=m.snapshot().revision;preset(m,p);require(m.snapshot().revision==rev+1,"preset not atomic");const auto& expected=mathObjectPresets(MathObjectKind::Boolean,l)[p];for(unsigned j=0;j<expected.count;++j)near(m.parameter(expected.parameters[j]),expected.values[j],0,"preset value");inspect(m,scene);manifold(m.snapshot().solid);for(double op:{0.0,1.0,2.0,3.0}){set(m,MathParameter::BooleanOperation,op);inspect(m,scene);}preset(m,p);set(m,MathParameter::BooleanGuides,0);require(m.snapshot().partCount==0,"shape-only includes guides");inspect(m,scene);}}
  level(m,0);preset(m,0);checked(m,true);set(m,MathParameter::BooleanProbeX,2);checked(m,false);level(m,1);preset(m,0);checked(m,false);set(m,MathParameter::BooleanOperation,1);checked(m,true);require(m.snapshot().table.values[3][3]==1,"probe truth row");set(m,MathParameter::BooleanProbeX,.55);for(unsigned i=0;i<4;++i)near(m.snapshot().table.values[i][3],0,0,"boundary has binary truth row");
  level(m,2);preset(m,3);checked(m,true);set(m,MathParameter::BooleanBlend,0);checked(m,false);preset(m,3);set(m,MathParameter::BooleanX,1.1);level(m,0);near(m.parameter(MathParameter::BooleanX),1.1,1e-14,"retained shape settings");level(m,3);preset(m,0);checked(m,false);set(m,MathParameter::BooleanResolution,24);checked(m,true);const double volume=metric(m,"Midpoint full volume");set(m,MathParameter::BooleanSection,1);near(metric(m,"Midpoint full volume"),volume,0,"section changed material volume");
  // All supported parameter extrema through the same dispatcher, including empty/coincident states.
  for(unsigned l=0;l<4;++l){level(m,l);preset(m,0);for(unsigned pass=0;pass<2;++pass)for(const auto& p:mathParameterSpecs())if(m.parameterAvailable(p.id)){set(m,p.id,pass?p.maximum:p.minimum);inspect(m,scene);reject(m,{MathActionKind::SetParameter,{},p.id,std::numeric_limits<double>::quiet_NaN()});reject(m,{MathActionKind::SetParameter,{},p.id,p.maximum+1});}reject(m,{MathActionKind::ObjectPreset,{},{},0,4});reject(m,{MathActionKind::TogglePlayback});}
  // Maximum guide combination against the shared 8192-vertex scene budget.
  level(m,3);preset(m,3);set(m,MathParameter::BooleanResolution,28);for(double x:{0.0,.4,1.0,1.5}){set(m,MathParameter::BooleanX,x);inspect(m,scene);}
  const auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<8;++i)set(m,MathParameter::BooleanProbeX,i*.03);std::printf("Boolean sampling-layer rebuild mean: %.3f ms.\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/8);
  action(m,{MathActionKind::Select,MathObjectKind::Lathe});inspect(m,scene);require(m.snapshot().curve.count==7,"lathe picker regression");action(m,{MathActionKind::Select,MathObjectKind::Algebra});require(m.snapshot().solid.indexCount==0,"Boolean surface leaked");
}
}
int main(){try{kernel();geometry();model();std::printf("Boolean passed: %u numerical certificates, %u CPU meshes; max %zu vertices / %zu indices. No host, fonts or images.\n",certificates,meshes,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"Boolean tests failed: %s\n",e.what());return 1;}}

#include "runtime/math_objects/BezierCurve.hpp"
#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using namespace paths;
using V=BezierPoint;
constexpr double pi=3.14159265358979323846;
std::size_t certificates=0,meshes=0,maxVertices=0,maxIndices=0;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void near(double actual,double expected,double tolerance,const char* message){if(!std::isfinite(actual)||std::fabs(actual-expected)>tolerance)throw std::runtime_error(std::string(message)+": "+std::to_string(actual)+" vs "+std::to_string(expected));}
V sub(V a,V b){for(unsigned i=0;i<3;++i)a[i]-=b[i];return a;}
double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
double length(V a){return std::hypot(a[0],a[1],a[2]);}
V vector(iggy3d::Vec3 v){return {v.x,v.y,v.z};}
void nearVector(V actual,V expected,double tolerance,const char* message){for(unsigned i=0;i<3;++i)near(actual[i],expected[i],tolerance,message);}
// Direct Bernstein/power evaluation is independent of de Casteljau subdivision.
V position(const CubicBezier& c,double t){V r{};const double u=1-t;for(unsigned i=0;i<3;++i)r[i]=u*u*u*c.controls[0][i]+3*u*u*t*c.controls[1][i]+3*u*t*t*c.controls[2][i]+t*t*t*c.controls[3][i];return r;}
V derivative(const CubicBezier& c,double t){V r{};for(unsigned i=0;i<3;++i){const double a=-c.controls[0][i]+3*c.controls[1][i]-3*c.controls[2][i]+c.controls[3][i],b=3*c.controls[0][i]-6*c.controls[1][i]+3*c.controls[2][i],d=3*(c.controls[1][i]-c.controls[0][i]);r[i]=3*a*t*t+2*b*t+d;}return r;}
V second(const CubicBezier& c,double t){V r{};for(unsigned i=0;i<3;++i)r[i]=6*((1-t)*(c.controls[2][i]-2*c.controls[1][i]+c.controls[0][i])+t*(c.controls[3][i]-2*c.controls[2][i]+c.controls[1][i]));return r;}
double simpson(const CubicBezier& c,double a,double b){return (b-a)/6*(length(derivative(c,a))+4*length(derivative(c,(a+b)/2))+length(derivative(c,b)));}
double integral(const CubicBezier& c,double a,double b,double whole,double tolerance,unsigned depth=0){const double m=(a+b)/2,left=simpson(c,a,m),right=simpson(c,m,b),change=left+right-whole;if(depth==22||std::fabs(change)<15*tolerance)return left+right+change/15;return integral(c,a,m,left,tolerance/2,depth+1)+integral(c,m,b,right,tolerance/2,depth+1);}
double integral(const CubicBezier& c,double a,double b){return integral(c,a,b,simpson(c,a,b),1e-11);}
void frameCheck(const BezierFrame& f){require(f.defined,"frame missing");near(length(f.tangent),1,2e-13,"unit tangent");near(length(f.normal),1,2e-13,"unit normal");near(length(f.binormal),1,2e-13,"unit binormal");near(dot(f.tangent,f.normal),0,2e-13,"orthogonal TN");near(dot(f.tangent,f.binormal),0,2e-13,"orthogonal TB");near(dot(f.normal,f.binormal),0,2e-13,"orthogonal NB");nearVector(cross(f.tangent,f.normal),f.binormal,2e-13,"right-handed frame");}
void kernel() {
  const CubicBezier line{{V{-1.5,0,0},V{-.5,0,0},V{.5,0,0},V{1.5,0,0}}};
  const CubicBezier cube{{V{},V{},V{},V{1,0,0}}};
  const CubicBezier bend{{V{-1.5,0,0},V{-1,1.3,0},V{1,1.3,0},V{1.5,0,0}}};
  const CubicBezier turn{{V{},V{1,.5,.25},V{-1,-.5,-.25},V{}}};
  const CubicBezier helix{{V{-1.5,-1,-.5},V{2,-.5,1.5},V{-1.5,1,2},V{1.5,1.5,-.5}}};
  std::array<CubicBezier,17> curves{line,cube,bend,turn,helix};
  for(unsigned i=5;i<curves.size();++i)for(unsigned j=0;j<4;++j)for(unsigned k=0;k<3;++k)curves[i].controls[j][k]=1.95*std::sin((i+1)*(j+2)*(k+3)*.731);
  for(const auto& c:curves) {
    const auto arc=analyzeBezier(c);const double truth=integral(c,0,1);
    require(arc.bounds.lower<=truth+2e-10&&arc.bounds.upper>=truth-2e-10,"arc integral outside chord/polygon bounds");near(arc.length(),truth,2e-9,"arc length independent quadrature");
    near(arc.length(),arc.bounds.estimate(),2e-14,"cumulative and total bounds agree");
    double previous=-1;
    for(unsigned step=0;step<=32;++step){const double t=step/32.0;const auto s=sampleBezier(c,t);nearVector(s.position,position(c,t),3e-15,"Bernstein position");nearVector(s.first,derivative(c,t),2e-14,"first derivative");nearVector(s.second,second(c,t),2e-14,"second derivative");near(s.speed,length(derivative(c,t)),2e-14,"speed");if(s.regular){near(s.curvature,length(cross(derivative(c,t),second(c,t)))/std::pow(s.speed,3),1e-9,"curvature identity");frameCheck(seedBezierFrame(s.tangent));}for(unsigned k=0;k<3;++k){double lo=c.controls[0][k],hi=lo;for(const auto& p:c.controls){lo=std::min(lo,p[k]);hi=std::max(hi,p[k]);}require(s.position[k]>=lo-1e-14&&s.position[k]<=hi+1e-14,"convex hull bound");}nearVector(constructBezier(c,t).point,s.position,0,"construction agreement");
      const double u=bezierParameterAtFraction(arc,t);require(u>=previous&&u>=0&&u<=1,"arc inverse monotonic");previous=u;near(integral(c,0,u),t*truth,3e-8,"arc inverse independent quadrature");++certificates;}
    if(arc.regular){auto f=seedBezierFrame(sampleBezier(c,0).tangent);for(unsigned i=1;i<=128;++i){f=transportBezierFrame(f,sampleBezier(c,i/128.0).tangent);frameCheck(f);}}
  }
  const auto straight=analyzeBezier(line);require(straight.regular,"straight curve incorrectly singular");near(straight.length(),3,1e-14,"exact straight length");near(sampleBezier(line,.4).curvature,0,0,"straight curvature");
  const auto cubic=analyzeBezier(cube);require(!cubic.regular,"stationary endpoint not detected");near(cubic.length(),1,1e-14,"t-cubed length");for(double f:{.001,.125,.5,.9})near(bezierParameterAtFraction(cubic,f),std::cbrt(f),2e-9,"inverse t cubed");require(!analyzeBezier(turn).regular,"non-grid interior stationary point missed");
  const double root=.373;CubicBezier stationary;stationary.controls[0]={0,0,0};stationary.controls[1]={root*root/3,0,0};stationary.controls[2]={2*root*root/3-root/3,0,0};stationary.controls[3]={1.0/3-root+root*root,0,0};require(!analyzeBezier(stationary).regular,"double stationary root missed");
  const auto constant=analyzeBezier({});require(!constant.regular,"constant marked regular");near(constant.length(),0,0,"constant length");near(bezierParameterAtFraction(constant,1),0,0,"constant inverse convention");require(!sampleBezier({},.5).regular,"constant tangent");
  auto f=seedBezierFrame({1,0,0});const auto original=f;f=transportBezierFrame(f,{1,0,0});nearVector(f.normal,original.normal,0,"straight transport no flip");f=transportBezierFrame(f,{-1,0,0});frameCheck(f);nearVector(f.normal,original.normal,0,"180-degree normal convention");require(!seedBezierFrame({}).defined&&!transportBezierFrame(f,{}).defined,"zero tangent frame");
  for(double bad:{-1.0,2.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){bool rejected=false;try{(void)sampleBezier(line,bad);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"bad fraction accepted");}
  auto invalid=line;invalid.controls[0][0]=3;bool rejected=false;try{(void)analyzeBezier(invalid);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"out of domain control accepted");
}
void action(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,MathParameter p,double v){action(m,{MathActionKind::SetParameter,{},p,v});}
void level(MathObjects& m,unsigned n){action(m,{MathActionKind::SetLevel,{},{},static_cast<double>(n)});}
void preset(MathObjects& m,unsigned n){action(m,{MathActionKind::ObjectPreset,{},{},0,n});}
void checked(MathObjects& m,bool solved){action(m,{MathActionKind::Check});if((m.snapshot().feedback==MathFeedback::Solved)!=solved)throw std::runtime_error("curve challenge at level "+std::to_string(m.snapshot().level)+": "+std::string(m.snapshot().feedbackText));}
bool hasMetric(const MathObjects& m,std::string_view name){for(std::size_t i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==name)return true;return false;}
double metric(const MathObjects& m,std::string_view name){for(std::size_t i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==name)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric "+std::string(name));}
MathParameter coordinate(unsigned i,unsigned j){return static_cast<MathParameter>(static_cast<unsigned>(MathParameter::CurveP0X)+3*i+j);}
CubicBezier controls(const MathObjects& m){CubicBezier c;for(unsigned i=0;i<4;++i)for(unsigned j=0;j<3;++j)c.controls[i][j]=m.parameter(coordinate(i,j));return c;}
void assign(MathObjects& m,const CubicBezier& c){for(unsigned i=0;i<4;++i)for(unsigned j=0;j<3;++j)set(m,coordinate(i,j),c.controls[i][j]);}
void reject(MathObjects& m,MathAction a){const auto before=m.snapshot().revision;const auto feedback=m.snapshot().feedback;std::array<double,static_cast<std::size_t>(MathParameter::Count)> values{};for(const auto& p:mathParameterSpecs())values[static_cast<unsigned>(p.id)]=m.parameter(p.id);require(!m.dispatch(a).accepted,"invalid action accepted");require(m.snapshot().revision==before&&m.snapshot().feedback==feedback,"rejection changed snapshot");for(const auto& p:mathParameterSpecs())near(m.parameter(p.id),values[static_cast<unsigned>(p.id)],0,"rejection changed values");}
void inspect(const MathObjects& m,MathObjectScene& scene) {
  const auto& s=m.snapshot();const auto& mesh=scene.publish(s,{0,0,900,650});++meshes;maxVertices=std::max(maxVertices,mesh.vertices.size());maxIndices=std::max(maxIndices,mesh.indices.size());require(!mesh.vertices.empty()&&mesh.vertices.size()<=kSceneVertexCapacity&&mesh.indices.size()<=kSceneIndexCapacity,"mesh capacity");require(iggy3d::isFinite(mesh.clipFromWorld),"finite camera");std::set<unsigned> ids;std::size_t covered=0;for(const auto& d:mesh.draws){require(ids.insert(d.objectId.value).second&&d.firstIndex==covered&&d.indexCount%3==0,"draw ownership");covered+=d.indexCount;}require(covered==mesh.indices.size(),"draw coverage");for(auto i:mesh.indices)require(i<mesh.vertices.size(),"index range");for(const auto& v:mesh.vertices){for(auto x:v.position)require(std::isfinite(x),"finite mesh position");for(auto x:v.color)require(std::isfinite(x)&&x>=0&&x<=1,"finite mesh color");}
  for(std::size_t i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"finite metric");for(std::size_t i=0;i<s.plotCount;++i)for(std::size_t j=0;j<s.plots[i].seriesCount;++j)for(std::size_t k=0;k<s.plots[i].series[j].count;++k){const auto p=s.plots[i].series[j].points[k];require(std::isfinite(p.x)&&std::isfinite(p.y),"finite plot");}for(std::size_t i=0;i<s.partCount;++i)require(std::fabs(iggy3d::dot(s.parts[i].x,iggy3d::cross(s.parts[i].y,s.parts[i].z)))>1e-15,"singular primitive transform");
  const auto& patch=s.surface;require(patch.rows*patch.columns<=patch.vertices.size(),"patch capacity");for(unsigned i=0;i<patch.rows*patch.columns;++i){require(iggy3d::isFinite(patch.vertices[i].position),"finite patch");near(iggy3d::length(patch.vertices[i].normal),1,3e-7,"unit patch normal");}
}
void sweepCertificate(const MathObjects& m) {
  const auto& patch=m.snapshot().surface;require(patch.rows==25&&patch.columns==17,"capped sweep grid");const auto c=controls(m);const auto arc=analyzeBezier(c);
  for(unsigned row=0;row<25;++row){nearVector(vector(patch.vertices[row*17].position),vector(patch.vertices[row*17+16].position),0,"closed profile seam");nearVector(vector(patch.vertices[row*17].normal),vector(patch.vertices[row*17+16].normal),0,"seam normals");}
  for(unsigned i=0;i<21;++i){const double s=i/20.0,t=bezierParameterAtFraction(arc,s);const auto center=position(c,t),tangent=derivative(c,t);for(unsigned j=0;j<8;++j){const auto a=vector(patch.vertices[(i+2)*17+j].position),b=vector(patch.vertices[(i+2)*17+j+8].position);for(unsigned k=0;k<3;++k)near((a[k]+b[k])/2,center[k],2e-7,"opposite points centre on the curve");near(dot(sub(a,center),tangent),0,2e-6,"profile in normal plane");}}
  for(unsigned j=0;j<17;++j){nearVector(vector(patch.vertices[j].position),c.controls.front(),1e-7,"start cap centre");nearVector(vector(patch.vertices[24*17+j].position),c.controls.back(),1e-7,"end cap centre");nearVector(vector(patch.vertices[17+j].position),vector(patch.vertices[34+j].position),0,"start rim closure");nearVector(vector(patch.vertices[23*17+j].position),vector(patch.vertices[22*17+j].position),0,"end rim closure");if(m.parameter(MathParameter::CurveEndScale)==0)nearVector(vector(patch.vertices[22*17+j].position),c.controls.back(),1e-7,"collapsed horn tip");}
  ++certificates;
}
void model() {
  MathObjects m;MathObjectScene scene;action(m,{MathActionKind::Select,MathObjectKind::Curve});require(mathObjectSpecs().size()==static_cast<std::size_t>(MathObjectKind::Count)&&mathLessons(MathObjectKind::Curve).size()==4,"registered curve layers");
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned n=0;n<4;++n){const auto revision=m.snapshot().revision;preset(m,n);require(m.snapshot().revision==revision+1,"atomic preset revision");const auto& p=mathObjectPresets(MathObjectKind::Curve,l)[n];for(unsigned i=0;i<p.count;++i)near(m.parameter(p.parameters[i]),p.values[i],0,"preset values");inspect(m,scene);if(l==3)sweepCertificate(m);for(unsigned i=0;i<4;++i){auto expected=controls(m).controls[i];if(l==2)expected[0]-=2.6;nearVector(vector(m.snapshot().curve.controls[i]),expected,3e-7,"typed control handles");}}}
  level(m,0);preset(m,0);checked(m,false);set(m,MathParameter::CurveP1Z,.5);checked(m,true);set(m,MathParameter::CurveProgress,.4);require(m.snapshot().feedback==MathFeedback::None,"edit clears feedback");
  level(m,1);preset(m,0);set(m,MathParameter::CurveP1Y,.1);set(m,MathParameter::CurveP2Y,.1);checked(m,false);preset(m,0);set(m,MathParameter::CurveProgress,.5);checked(m,true);
  level(m,2);preset(m,1);checked(m,false);set(m,MathParameter::CurveTravel,1);checked(m,true);const auto c=controls(m);const double total=integral(c,0,1);for(unsigned i=0;i<8;++i){const auto& row=m.snapshot().table.values[i];const double a=i?m.snapshot().table.values[i-1][1]:0;near(row[2],integral(c,i/8.0,(i+1)/8.0),2e-9,"parameter interval integral");near(row[3],integral(c,a,row[1]),2e-9,"distance interval integral");near(row[3],total/8,2e-8,"equal distances");}++certificates;
  level(m,3);preset(m,3);checked(m,false);set(m,MathParameter::CurveTwist,90);checked(m,true);set(m,MathParameter::CurveGuides,0);require(!m.snapshot().curve.active&&m.snapshot().partCount==0,"shape only hides construction");inspect(m,scene);sweepCertificate(m);
  const auto retained=controls(m);level(m,0);near(m.parameter(MathParameter::CurveTwist),90,0,"shape settings retained across layers");require(controls(m).controls==retained.controls,"controls retained across layers");reject(m,{MathActionKind::SetParameter,{},MathParameter::CurveRadius,.2});level(m,3);near(m.parameter(MathParameter::CurveTwist),90,0,"return to sweep retains twist");
  preset(m,0);set(m,MathParameter::CurveProgress,0);near(m.parameter(MathParameter::CurveProgress),0,0,"restart position");action(m,{MathActionKind::TogglePlayback});action(m,{MathActionKind::AdvanceTime,{},{},1});near(m.parameter(MathParameter::CurveProgress),.25,0,"four second playback");set(m,MathParameter::CurveControl,2);require(!m.snapshot().playing&&m.snapshot().curve.selected==2,"select pauses playback");set(m,MathParameter::CurveP0Z,.7);near(m.parameter(MathParameter::CurveP0Z),.7,1e-14,"other point coordinates semantically available");action(m,{MathActionKind::TogglePlayback});preset(m,2);require(!m.snapshot().playing,"preset pauses playback");set(m,MathParameter::CurveProgress,0);action(m,{MathActionKind::TogglePlayback});action(m,{MathActionKind::AdvanceTime,{},{},4});near(m.parameter(MathParameter::CurveProgress),1,0,"clamped playback endpoint");require(!m.snapshot().playing,"endpoint stops playback");reject(m,{MathActionKind::TogglePlayback});
  const CubicBezier line{{V{-1.5,0,0},V{-.5,0,0},V{.5,0,0},V{1.5,0,0}}};assign(m,line);set(m,MathParameter::CurveGuides,0);
  for(unsigned profile=0;profile<3;++profile)for(double twist:{-360.0,0.0,180.0})for(double end:{0.0,.25,1.0}) {
    set(m,MathParameter::CurveProfile,profile);if(profile==2)set(m,MathParameter::CurveNormP,3);set(m,MathParameter::CurveTwist,twist);set(m,MathParameter::CurveEndScale,end);set(m,MathParameter::CurveRadius,.4);set(m,MathParameter::CurveAspect,.1);inspect(m,scene);sweepCertificate(m);
    const auto& patch=m.snapshot().surface;for(unsigned i=0;i<21;++i){const double s=i/20.0,r=.4*((1-s)+s*end),angle=twist*pi/180*s;for(unsigned j=0;j<16;++j){const auto v=patch.vertices[(i+2)*17+j].position;near(v.x,-1.5+3*s,2e-7,"straight sweep axis");if(r>0){const double u=(std::cos(angle)*v.y+std::sin(angle)*v.z)/r,w=(-std::sin(angle)*v.y+std::cos(angle)*v.z)/(r*.1);const double norm=profile==1?std::max(std::fabs(u),std::fabs(w)):std::pow(std::pow(std::fabs(u),profile==0?2:3)+std::pow(std::fabs(w),profile==0?2:3),1.0/(profile==0?2:3));near(norm,1,2e-6,"twisted profile equation");if(profile==1&&j%4==2){near(std::fabs(u),1,2e-6,"square corner u");near(std::fabs(w),1,2e-6,"square corner v");}}}}
  }
  level(m,1);set(m,MathParameter::CurveProgress,.5);near(metric(m,"Curvature"),0,0,"straight curvature readout");near(metric(m,"Tangent defined"),1,0,"straight tangent readout");checked(m,false);
  for(unsigned l=0;l<4;++l){level(m,l);assign(m,{});inspect(m,scene);checked(m,false);near(metric(m,"Tangent defined"),0,0,"constant undefined tangent");if(l==1)require(!hasMetric(m,"Curvature"),"constant got invented curvature");if(l==3)require(m.snapshot().surface.rows==0&&m.snapshot().curve.active,"invalid sweep still editable");}
  level(m,3);assign(m,{{V{},V{1,.5,.25},V{-1,-.5,-.25},V{}}});require(m.snapshot().surface.rows==0,"interior stationary sweep not suppressed");inspect(m,scene);
  for(unsigned l=0;l<4;++l){level(m,l);preset(m,0);for(unsigned pass=0;pass<2;++pass)for(const auto& p:mathParameterSpecs())if(m.parameterAvailable(p.id)){set(m,p.id,pass?p.maximum:p.minimum);inspect(m,scene);reject(m,{MathActionKind::SetParameter,{},p.id,std::numeric_limits<double>::quiet_NaN()});reject(m,{MathActionKind::SetParameter,{},p.id,p.maximum+1});}reject(m,{MathActionKind::ObjectPreset,{},{},0,4});reject(m,{MathActionKind::SetLevel,{},{},4});reject(m,{MathActionKind::SetParameter,{},MathParameter::Angle,90});}
  level(m,3);preset(m,3);for(double p:{1.0,1.25,2.0,8.0,16.0}){set(m,MathParameter::CurveNormP,p);inspect(m,scene);sweepCertificate(m);}action(m,{MathActionKind::Reset});near(m.parameter(MathParameter::CurveTwist),0,0,"reset twist");near(m.parameter(MathParameter::CurveEndScale),1,0,"reset end scale");
  const auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<30;++i)set(m,MathParameter::CurveProgress,i/30.0);const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/30;std::printf("Sweep rebuild mean: %.3f ms.\n",ms);
}
}
int main(){try{kernel();model();std::printf("Curves passed: %zu numerical certificates, %zu CPU meshes; max %zu vertices / %zu indices. Text-only; no native host, fonts or images.\n",certificates,meshes,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"Curve tests failed: %s\n",e.what());return 1;}}

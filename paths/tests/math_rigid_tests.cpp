#include "runtime/math_objects/RigidBody.hpp"
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
constexpr double pi=3.14159265358979323846;
unsigned checks=0,scenes=0,maxSteps=0;std::size_t maxVertices=0,maxIndices=0;double maxEnergyError=0,maxMomentumError=0;
void require(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
void near(double a,double b,double tol,const char* why){++checks;if(!std::isfinite(a)||!std::isfinite(b)||std::fabs(a-b)>tol){char message[256];std::snprintf(message,sizeof(message),"%s: %.17g != %.17g (difference %.4g, tolerance %.4g)",why,a,b,std::fabs(a-b),tol);throw std::runtime_error(message);}}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}require(caught,"invalid input accepted");}
double dot(const RigidVector& a,const RigidVector& b){double v=0;for(unsigned i=0;i<3;++i)v+=a[i]*b[i];return v;}
RigidVector mul(const RigidMatrix& r,const RigidVector& v){RigidVector b{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)b[i]+=r[3*i+j]*v[j];return b;}
RigidMatrix multiply(const RigidMatrix& a,const RigidMatrix& b){RigidMatrix c{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)c[3*i+j]+=a[3*i+k]*b[3*k+j];return c;}
RigidMatrix euler(const RigidVector& a){const double x=a[0]*pi/180,y=a[1]*pi/180,z=a[2]*pi/180;return multiply({std::cos(z),-std::sin(z),0,std::sin(z),std::cos(z),0,0,0,1},multiply({std::cos(y),0,std::sin(y),0,1,0,-std::sin(y),0,std::cos(y)},{1,0,0,0,std::cos(x),-std::sin(x),0,std::sin(x),std::cos(x)}));}
// Independent formulation: fixed WORLD momentum, matrix differential equation
// Rdot = skew(R I^-1 R^T L0) R. No quaternion or body-momentum integration.
RigidMatrix oracle(RigidMatrix r,const RigidVector& inertia,const RigidVector& worldL,double t){
  const auto rate=[&](const RigidMatrix& a){RigidVector bodyOmega{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)bodyOmega[i]+=a[3*j+i]*worldL[j]/inertia[i];const auto w=mul(a,bodyOmega);return multiply({0,-w[2],w[1],w[2],0,-w[0],-w[1],w[0],0},a);};
  const auto shift=[](RigidMatrix a,const RigidMatrix& b,double h){for(unsigned i=0;i<9;++i)a[i]+=b[i]*h;return a;};
  const unsigned n=std::max(1U,static_cast<unsigned>(std::ceil(t/.0002)));const double h=t/n;
  for(unsigned k=0;k<n;++k){const auto a=rate(r),b=rate(shift(r,a,h/2)),c=rate(shift(r,b,h/2)),d=rate(shift(r,c,h));for(unsigned i=0;i<9;++i)r[i]+=h*(a[i]+2*b[i]+2*c[i]+d[i])/6;}return r;
}
void mass(){
  for(unsigned kind=0;kind<4;++kind)for(double balance:{.2,.5,.8}){
    RigidInput p;p.shape=static_cast<RigidShape>(kind);p.dimensions={2.3,.7,1.2};p.mass=2.7;p.balance=balance;const auto b=makeRigidBody(p);
    double mass=0;RigidVector center{};RigidMatrix moments{};
    for(unsigned k=0;k<b.count;++k){const auto& c=b.parts[k];mass+=c.mass;for(unsigned i=0;i<3;++i)center[i]+=c.mass*c.center[i];
      // Exact quadrature for quadratic moments, independent of inertia formulas.
      const auto sample=[&](RigidVector v,double weight){for(unsigned i=0;i<3;++i)v[i]+=c.center[i];const double r2=dot(v,v);for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)moments[3*i+j]+=weight*((i==j?r2:0)-v[i]*v[j]);};
      if(c.primitive==RigidPrimitive::Box){for(unsigned mask=0;mask<8;++mask){RigidVector v{};for(unsigned i=0;i<3;++i)v[i]=(mask&(1<<i)?1:-1)*c.dimensions[i]/(2*std::sqrt(3.));sample(v,c.mass/8);}}
      else for(double u:{.5-.5/std::sqrt(3.),.5+.5/std::sqrt(3.)})for(double y:{-1.,1.})for(unsigned j=0;j<16;++j){const double phi=2*pi*(j+.5)/16;sample({.5*c.dimensions[0]*std::sqrt(u)*std::cos(phi),y*c.dimensions[1]/(2*std::sqrt(3.)),.5*c.dimensions[2]*std::sqrt(u)*std::sin(phi)},c.mass/64);}
      if(c.primitive==RigidPrimitive::Box)for(unsigned j=k+1;j<b.count;++j){bool separate=false;for(unsigned i=0;i<3;++i)separate=separate||std::fabs(c.center[i]-b.parts[j].center[i])>=(c.dimensions[i]+b.parts[j].dimensions[i])/2-1e-12;require(separate,"component interiors overlap");}
    }
    near(mass,p.mass,1e-14,"total component mass");for(double x:center)near(x,0,1e-14,"recentered COM");
    const double expected=kind==1?.315*p.dimensions[0]*(1-2*balance):kind==3?.119*p.dimensions[0]*(1-2*balance):0;near(b.center[0],expected,1e-14,"assembly COM formula");
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)near(moments[3*i+j],i==j?b.inertia[i]:0,1e-13,"inertia versus independent quadrature");
    for(unsigned i=0;i<3;++i)require(b.inertia[i]>0&&b.inertia[i]<=b.inertia[(i+1)%3]+b.inertia[(i+2)%3],"unphysical inertia");
    auto scaled=p;scaled.mass*=1.5;const auto bm=makeRigidBody(scaled);for(unsigned i=0;i<3;++i)near(bm.inertia[i],1.5*b.inertia[i],1e-13,"mass scaling");
    scaled=p;for(auto& d:scaled.dimensions)d*=1.4;const auto bs=makeRigidBody(scaled);for(unsigned i=0;i<3;++i)near(bs.inertia[i],1.96*b.inertia[i],1e-13,"dimension squared scaling");
  }
}
void invariants(const RigidMotion& m,double time){
  const auto s=m.at(time);double qnorm=0;for(double x:s.orientation)qnorm+=x*x;near(qnorm,1,8e-16,"unit quaternion");
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){double value=0;for(unsigned k=0;k<3;++k)value+=s.rotation[3*k+i]*s.rotation[3*k+j];near(value,i==j,2e-15,"rotation orthogonality");}
  const auto applied=mul(s.worldInertia,s.worldOmega);for(unsigned i=0;i<3;++i)near(applied[i],s.worldMomentum[i],2e-12,"world inertia times velocity");
  maxEnergyError=std::max(maxEnergyError,std::fabs(s.energyError));maxMomentumError=std::max(maxMomentumError,s.momentumError);require(std::fabs(s.energyError)<1e-7&&s.momentumError<1e-7,"conservation drift");
  maxSteps=std::max(maxSteps,m.integrationSteps());require(m.integrationSteps()<=RigidMotion::maxSteps,"step budget");
}
void physics(){
  RigidMotion m;rejects([&]{(void)m.at(0);});rejects([&]{(void)m.body();});
  RigidInput p;p.rotationDegrees={31,-27,63};const auto expected=euler(p.rotationDegrees),q=rigidRotationMatrix(rigidReleaseOrientation(p.rotationDegrees));for(unsigned i=0;i<9;++i)near(q[i],expected[i],4e-16,"release rotation order");
  auto quaternion=rigidReleaseOrientation(p.rotationDegrees);for(auto& x:quaternion)x=-x;const auto negative=rigidRotationMatrix(quaternion);for(unsigned i=0;i<9;++i)near(negative[i],q[i],0,"quaternion double cover");
  for(unsigned axis=0;axis<3;++axis){p.shape=RigidShape::Book;p.dimensions={2.4,.3,1.6};p.omega={};p.omega[axis]=2.4;m.configure(p);for(double t:{0.,.1,2.,12.}){invariants(m,t);auto angle=RigidVector{};angle[axis]=2.4*t*180/pi;const auto r=multiply(expected,euler(angle));const auto s=m.at(t);for(unsigned i=0;i<9;++i)near(s.rotation[i],r[i],2e-10,"principal spin analytic rotation");for(unsigned i=0;i<3;++i)near(s.bodyOmega[i],p.omega[i],1e-14,"constant principal spin");}}
  p={};p.omega={.7,2.3,-.4};m.configure(p);const auto b=m.body();const double rate=(b.inertia[1]-b.inertia[0])/b.inertia[0]*p.omega[1];for(double t:{.3,3.,12.}){const auto s=m.at(t);near(s.bodyOmega[0],.7*std::cos(rate*t)-.4*std::sin(rate*t),1e-9,"symmetric top x");near(s.bodyOmega[2],-.4*std::cos(rate*t)-.7*std::sin(rate*t),1e-9,"symmetric top z");near(s.bodyOmega[1],p.omega[1],0,"symmetric axial spin");}
  p.shape=RigidShape::Book;p.dimensions={2.4,.3,1.6};p.rotationDegrees={31,-27,63};p.omega={.02,.02,3};m.configure(p);require(m.body().distinctMoments&&m.body().order[1]==2,"book intermediate axis");
  const auto initial=m.at(0);for(double t:{.3,4.,12.}){const auto truth=oracle(expected,m.body().inertia,initial.worldMomentum,t);const auto s=m.at(t);for(unsigned i=0;i<9;++i)near(s.rotation[i],truth[i],8e-9,"world matrix ODE oracle");invariants(m,t);}
  require(m.at(4).bodyOmega[2]<-2.8,"intermediate-axis flip missing");
  const auto rev=m.revision();const auto sample=m.at(4.231);require(!m.configure(p)&&m.revision()==rev,"cache rebuilt unchanged input");for(double t:{12.,0.,7.3,1.})invariants(m,t);const auto replay=m.at(4.231);require(sample.orientation==replay.orientation&&sample.bodyMomentum==replay.bodyMomentum,"scrub depends on history");
  auto invalid=p;invalid.dimensions[0]=0;rejects([&]{m.configure(invalid);});require(m.revision()==rev&&m.at(4.231).orientation==sample.orientation,"invalid configure mutated cache");
  p.omega={};m.configure(p);for(double t:{0.,5.,12.}){const auto s=m.at(t);require(s.orientation==m.at(0).orientation&&s.energy==0&&s.energyError==0&&s.momentumError==0,"rest changed");}
  // Full shape/dimension/mass-share extrema, plus deterministic interior inputs.
  for(unsigned kind=0;kind<4;++kind)for(unsigned mask=0;mask<8;++mask)for(double f:{.2,.8}){p={};p.shape=static_cast<RigidShape>(kind);p.balance=f;for(unsigned i=0;i<3;++i)p.dimensions[i]=mask&(1<<i)?3.5:.2;p.omega={4,-4,4};p.rotationDegrees={180,-180,180};m.configure(p);for(double t:{.37,6.,12.})invariants(m,t);}
  for(unsigned i=0;i<20;++i){p.shape=static_cast<RigidShape>(i%4);for(unsigned j=0;j<3;++j){p.dimensions[j]=.2+3.3*((i*13+j*17)%37)/36.;p.omega[j]=-4+8.*((i*7+j*11)%29)/28.;p.rotationDegrees[j]=-180+360.*((i*3+j*5)%19)/18.;}m.configure(p);invariants(m,12);}
  for(double v:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()})for(unsigned field=0;field<12;++field){auto invalid=RigidInput{};switch(field){case 0:case 1:case 2:invalid.dimensions[field]=v;break;case 3:invalid.mass=v;break;case 4:invalid.balance=v;break;case 5:case 6:case 7:invalid.rotationDegrees[field-5]=v;break;case 8:case 9:case 10:invalid.omega[field-8]=v;break;case 11:rejects([&]{m.at(v);});continue;}rejects([&]{m.configure(invalid);});}
  rejects([&]{m.at(-.1);});rejects([&]{m.at(12.1);});p={};p.shape=RigidShape::Count;rejects([&]{m.configure(p);});
}
void act(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double v){act(m,{MathActionKind::SetParameter,{},p,v});}
void level(MathObjects& m,unsigned v){act(m,{MathActionKind::SetLevel,{},{},static_cast<double>(v)});}
void preset(MathObjects& m,unsigned v){act(m,{MathActionKind::ObjectPreset,{},{},0,v});}
void challenge(MathObjects& m,bool pass){act(m,{MathActionKind::Check});require((m.snapshot().feedback==MathFeedback::Solved)==pass,"incorrect rigid challenge");}
RigidInput input(const MathObjects& m){RigidInput p;p.shape=static_cast<RigidShape>(m.parameter(P::RigidShape));p.dimensions={m.parameter(P::RigidWidth),m.parameter(P::RigidHeight),m.parameter(P::RigidDepth)};p.mass=m.parameter(P::RigidMass);p.balance=m.parameter(P::RigidBalance);p.rotationDegrees={m.parameter(P::RigidRotX),m.parameter(P::RigidRotY),m.parameter(P::RigidRotZ)};p.omega={m.parameter(P::RigidSpinX),m.parameter(P::RigidSpinY),m.parameter(P::RigidSpinZ)};return p;}
void inspect(MathObjects& m,MathObjectScene& scene){
  const auto& s=m.snapshot();const auto& f=scene.publish(s,{0,0,800,600});++scenes;maxVertices=std::max(maxVertices,f.vertices.size());maxIndices=std::max(maxIndices,f.indices.size());require(!f.vertices.empty()&&f.vertices.size()<=kSceneVertexCapacity&&f.indices.size()<=kSceneIndexCapacity,"scene capacity");require(iggy3d::isFinite(f.clipFromWorld),"camera finite");
  for(const auto& v:f.vertices){for(float x:v.position)require(std::isfinite(x),"finite vertex");for(float x:v.color)require(std::isfinite(x)&&x>=0&&x<=1,"vertex colour");}for(unsigned index:f.indices)require(index<f.vertices.size(),"scene index");
  std::set<unsigned> ids;unsigned end=0;for(auto d:f.draws){require(ids.insert(d.objectId.value).second&&d.firstIndex==end,"draw ownership");end+=d.indexCount;}require(end==f.indices.size(),"unowned indices");
  RigidMotion motion;motion.configure(input(m));const auto state=motion.at(m.parameter(P::RigidTime));const auto& body=motion.body();unsigned count=0;
  for(unsigned i=0;i<s.partCount;++i){const auto& p=s.parts[i];if(p.role=="rigid_mass_component"){const auto& c=body.parts[count++];const auto center=rigidRotate(state.orientation,c.center);near(p.center.x,center[0],2e-7,"component center x");near(p.center.y,center[1],2e-7,"component center y");near(p.center.z,center[2],2e-7,"component center z");auto dimensions=c.dimensions;if(c.primitive==RigidPrimitive::Cylinder){dimensions[0]*=.5;dimensions[2]*=.5;}for(unsigned j=0;j<3;++j){RigidVector e{};e[j]=dimensions[j];const auto axis=rigidRotate(state.orientation,e);const auto actual=std::array{p.x,p.y,p.z}[j];near(actual.x,axis[0],3e-7,"body basis x");near(actual.y,axis[1],3e-7,"body basis y");near(actual.z,axis[2],3e-7,"body basis z");}}}
  require(count==body.count,"missing physical component");for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"finite metric");for(unsigned i=0;i<s.table.rowCount;++i)for(unsigned j=0;j<s.table.columnCount;++j)require(std::isfinite(s.table.values[i][j]),"finite table");
  for(unsigned i=0;i<s.plotCount;++i)for(unsigned j=0;j<s.plots[i].seriesCount;++j)for(unsigned k=0;k<s.plots[i].series[j].count;++k){const auto p=s.plots[i].series[j].points[k];require(std::isfinite(p.x)&&std::isfinite(p.y)&&p.x>=0&&p.x<=12,"finite plot domain");}
  for(unsigned i=0;i<s.matrixCount;++i)for(double x:s.matrices[i].values)require(std::isfinite(x),"finite matrix");
}
void model(){
  MathObjects m;MathObjectScene scene;act(m,{MathActionKind::Select,K::Rigid});require(mathLessons(K::Rigid).size()==4&&m.playbackParameter()==P::RigidTime,"rigid registration");
  std::set<std::string_view> keys;for(const auto& p:mathParameterSpecs())require(keys.insert(p.key).second,"duplicate key");
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned p=0;p<4;++p){const auto revision=m.snapshot().revision;preset(m,p);require(m.snapshot().revision==revision+1,"preset not atomic");const auto& example=mathObjectPresets(K::Rigid,l)[p];require(example.count==15,"incomplete preset");for(unsigned i=0;i<example.count;++i)near(m.parameter(example.parameters[i]),example.values[i],0,"preset values");for(double t:{0.,3.,4.1,12.}){set(m,P::RigidTime,t);inspect(m,scene);}set(m,P::RigidGuides,0);inspect(m,scene);for(unsigned i=0;i<m.snapshot().partCount;++i)require(m.snapshot().parts[i].role=="rigid_mass_component"||m.snapshot().parts[i].role=="rigid_surface_mark"||m.snapshot().parts[i].role=="rigid_panel_mark","guide remained");}}
  level(m,0);preset(m,0);challenge(m,false);set(m,P::RigidTime,.52);challenge(m,true);
  level(m,1);preset(m,1);challenge(m,false);set(m,P::RigidBalance,.7);challenge(m,true);
  level(m,2);preset(m,1);challenge(m,true);set(m,P::RigidSpinX,0);challenge(m,false);
  level(m,3);preset(m,2);challenge(m,false);set(m,P::RigidTime,4);challenge(m,true);set(m,P::RigidAxis,0);challenge(m,false);preset(m,0);set(m,P::RigidTime,4);challenge(m,false);
  for(auto p:{P::RigidSpinX,P::RigidSpinY,P::RigidSpinZ})set(m,p,0);inspect(m,scene);challenge(m,false);
  preset(m,2);set(m,P::RigidTime,1.23);set(m,P::RigidRotX,37);const auto saved=input(m);level(m,0);level(m,3);require(input(m)==saved,"layer lost initial state");near(m.parameter(P::RigidTime),1.23,1e-14,"layer lost time");
  preset(m,2);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},2});near(m.parameter(P::RigidTime),1,0,"half-speed playback");set(m,P::RigidAxis,0);require(!m.snapshot().playing,"edit failed to pause");set(m,P::RigidTime,11.9);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},2});near(m.parameter(P::RigidTime),12,0,"endpoint clamp");require(!m.snapshot().playing&&!m.dispatch({MathActionKind::TogglePlayback}).accepted,"endpoint playback");
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned shape=0;shape<4;++shape)for(bool high:{false,true}){preset(m,shape);for(const auto& p:mathParameterSpecs())if(p.owner==K::Rigid&&p.id!=P::RigidShape&&m.parameterAvailable(p.id))set(m,p.id,high?p.maximum:p.minimum);inspect(m,scene);}}
  level(m,0);preset(m,1);MathInspectorMemory memory;memory.visit(m);memory.rememberExample(m,"Adjustable dumbbell");set(m,P::RigidWidth,2);require(memory.exampleTitle(m)=="Custom","edit did not customize example");act(m,mathResetControlGroup(m,MathControlGroup::Shape));require(mathChangedControlCount(m,MathControlGroup::Shape)==0,"group reset failed");
  const auto revision=m.snapshot().revision;require(!m.dispatch({MathActionKind::SetParameter,{},P::RigidMass,0}).accepted&&!m.dispatch({MathActionKind::SetParameter,{},P::MembraneTime,2}).accepted,"bad action accepted");require(m.snapshot().revision==revision,"rejected action mutated model");
  act(m,{MathActionKind::Select,K::Algebra});require(m.playbackParameter()==P::Count&&m.snapshot().matrixCount==0,"rigid leaked to algebra");
}
}
int main(){try{mass();physics();model();std::printf("rigid CPU checks: %u assertions, %u scene states; maxima %zu vertices / %zu indices; integration %u steps; max relative energy %.4g / momentum %.4g; no host, fonts or images\n",checks,scenes,maxVertices,maxIndices,maxSteps,maxEnergyError,maxMomentumError);return 0;}catch(const std::exception& e){std::fprintf(stderr,"rigid failure: %s\n",e.what());return 1;}}

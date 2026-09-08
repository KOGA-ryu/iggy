#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using namespace paths;
constexpr double pi=3.14159265358979323846;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double eps,const char* why){if(!std::isfinite(a)||std::fabs(a-b)>eps)throw std::runtime_error(std::string(why)+": "+std::to_string(a)+" vs "+std::to_string(b));}
void action(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void level(MathObjects& m,unsigned n){action(m,{MathActionKind::SetLevel,{},{},static_cast<double>(n)});}
void select(MathObjects& m,MathObjectKind k,unsigned n=0){action(m,{MathActionKind::Select,k});level(m,n);action(m,{MathActionKind::Reset});}
void set(MathObjects& m,MathParameter p,double x){action(m,{MathActionKind::SetParameter,{},p,x});}
void turn(MathObjects& m,unsigned op){action(m,{MathActionKind::SymmetryTurn,{},{},0,op});}
double metric(const MathObjects& m,std::string_view name){const auto& s=m.snapshot();for(std::size_t i=0;i<s.metricCount;++i)if(s.metrics[i].label==name)return s.metrics[i].value;throw std::runtime_error("missing metric: "+std::string(name));}
void checked(MathObjects& m,bool solved){action(m,{MathActionKind::Check});require((m.snapshot().feedback==MathFeedback::Solved)==solved,"wrong challenge verdict");}
void reject(MathObjects& m,MathAction a){const auto revision=m.snapshot().revision;require(!m.dispatch(a).accepted,"invalid action accepted");require(m.snapshot().revision==revision,"rejection changed revision");}
std::size_t states=0,maxVertices=0,maxIndices=0;
void inspect(const MathObjects& m,MathObjectScene& scene) {
  const auto& s=m.snapshot();const auto& frame=scene.publish(s,{0,0,1000,700});++states;
  require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"invalid mesh capacity");
  maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());
  require(iggy3d::isFinite(frame.clipFromWorld),"nonfinite camera");
  std::set<std::uint32_t> ids;std::size_t end=0;
  for(const auto& draw:frame.draws){require(ids.insert(draw.objectId.value).second,"duplicate ID");require(draw.firstIndex==end&&draw.indexCount%3==0,"bad draw interval");end+=draw.indexCount;}
  require(end==frame.indices.size(),"unowned triangles");
  for(const auto& v:frame.vertices){for(float n:v.position)require(std::isfinite(n),"nonfinite geometry");for(float n:v.color)require(std::isfinite(n)&&n>=0&&n<=1,"invalid colour");}
  for(auto i:frame.indices)require(i<frame.vertices.size(),"index out of bounds");
  for(std::size_t i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"nonfinite measurement");
  for(std::size_t p=0;p<s.plotCount;++p){const auto& plot=s.plots[p];require(plot.seriesCount>0&&plot.seriesCount<=plot.series.size(),"bad plot count");for(std::size_t j=0;j<plot.seriesCount;++j){const auto& line=plot.series[j];require(line.count>0&&line.count<=line.points.size(),"bad line count");for(std::size_t i=0;i<line.count;++i)require(std::isfinite(line.points[i].x)&&std::isfinite(line.points[i].y),"nonfinite plot point");}}
}
using Matrix=std::array<double,9>;
Matrix multiply(const Matrix& a,const Matrix& b){Matrix r{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)r[3*i+j]+=a[3*i+k]*b[3*k+j];return r;}
constexpr Matrix identity{1,0,0,0,1,0,0,0,1};
void symmetry() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Symmetry);checked(m,false);
  turn(m,0);turn(m,0);turn(m,1);checked(m,true);inspect(m,scene);
  action(m,{MathActionKind::SymmetryUndo});checked(m,false);action(m,{MathActionKind::SymmetryIdentity});
  for(unsigned op=0;op<3;++op){turn(m,op);turn(m,op+3);require(m.snapshot().matrices[0].values==identity,"turn and inverse failed");}
  action(m,{MathActionKind::SymmetryIdentity});for(unsigned i=0;i<64;++i)turn(m,i%6);reject(m,{MathActionKind::SymmetryTurn,{},{},0,0});require(m.snapshot().symmetry.moveCount==64,"history overflow changed count");
  level(m,1);turn(m,0);turn(m,1);checked(m,true);
  const auto forward=m.snapshot().matrices[1].values,reverse=m.snapshot().matrices[2].values;
  require(forward!=reverse&&m.snapshot().matrices[0].values==forward,"composition order wrong");inspect(m,scene);
  action(m,{MathActionKind::SymmetryIdentity});turn(m,1);turn(m,0);checked(m,false);
  level(m,2);reject(m,{MathActionKind::SymmetryTurn,{},{},0,0});
  for(unsigned generator=0;generator<3;++generator) {
    set(m,MathParameter::SymmetryGenerator,generator);const unsigned order=generator==0?4:generator==1?2:3;
    for(unsigned n=0;n<=12;++n){set(m,MathParameter::SymmetryPower,n);near(metric(m,"Generator order"),order,0,"generator order");require((m.snapshot().matrices[0].values==identity)==(n%order==0),"power law failed");near(metric(m,"Orbit times stabiliser"),order,0,"cyclic orbit-stabiliser");inspect(m,scene);}
  }
  set(m,MathParameter::SymmetryPower,3);checked(m,true);set(m,MathParameter::SymmetryPower,0);checked(m,false);
  level(m,3);std::array<Matrix,24> group;std::array<std::array<unsigned,8>,24> permutations;
  for(unsigned i=0;i<24;++i) {
    set(m,MathParameter::SymmetryElement,i);group[i]=m.snapshot().matrices[0].values;permutations[i]=m.snapshot().symmetry.permutation;
    std::set<unsigned> unique(permutations[i].begin(),permutations[i].end());require(unique.size()==8,"not a permutation");
    const auto& a=group[i];for(double x:a)require(x==-1||x==0||x==1,"rotation lost exactness");
    near(a[0]*(a[4]*a[8]-a[5]*a[7])-a[1]*(a[3]*a[8]-a[5]*a[6])+a[2]*(a[3]*a[7]-a[4]*a[6]),1,0,"improper rotation");
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col){double dot=0;for(unsigned j=0;j<3;++j)dot+=a[3*row+j]*a[3*col+j];near(dot,row==col?1:0,0,"not orthogonal");}
    near(metric(m,"Group size"),24,0,"group cardinality");near(metric(m,"Orbit times stabiliser"),24,0,"full orbit-stabiliser");inspect(m,scene);
  }
  require(group[0]==identity,"identity index changed");require(std::set<Matrix>(group.begin(),group.end()).size()==24,"duplicate rotation");
  for(unsigned a=0;a<24;++a)for(unsigned b=0;b<24;++b){const auto result=multiply(group[a],group[b]);const auto found=std::find(group.begin(),group.end(),result);require(found!=group.end(),"group not closed");const auto index=static_cast<std::size_t>(found-group.begin());for(unsigned v=0;v<8;++v)require(permutations[index][v]==permutations[a][permutations[b][v]],"permutation representation not a homomorphism");}
  for(unsigned v=0;v<8;++v){unsigned fixed=0;std::set<unsigned> orbit;for(unsigned i=0;i<24;++i){fixed+=permutations[i][v]==v;orbit.insert(permutations[i][v]);}require(fixed==3&&orbit.size()==8,"vertex orbit or stabiliser wrong");set(m,MathParameter::SymmetryVertex,v);for(unsigned i=1;i<24;++i)if(permutations[i][v]==v){set(m,MathParameter::SymmetryElement,i);checked(m,true);break;}}
}
double target(unsigned rule,double t){if(rule==0)return t<pi?1:-1;if(rule==1)return t/pi-1;return t<pi/2?2*t/pi:t<3*pi/2?2-2*t/pi:2*t/pi-4;}
void harmonics() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Harmonics);
  reject(m,{MathActionKind::SetParameter,{},MathParameter::Amplitude2,1});set(m,MathParameter::HarmonicTime,1.57);checked(m,true);inspect(m,scene);
  level(m,1);set(m,MathParameter::Frequency2,1);checked(m,true);require(m.snapshot().plots[2].equalAspect,"circle projection stretches axes");
  for(const auto& p:m.snapshot().plots[2].series[0].points)near(p.x*p.x+p.y*p.y,1,1e-12,"Lissajous circle");
  set(m,MathParameter::Frequency2,2);checked(m,false);set(m,MathParameter::Frequency1,4);set(m,MathParameter::Frequency2,2);near(metric(m,"Shared period"),pi,1e-12,"shared period");inspect(m,scene);
  level(m,3);reject(m,{MathActionKind::SetParameter,{},MathParameter::Amplitude1,1});
  for(unsigned waveform=0;waveform<3;++waveform)for(unsigned n=1;n<=8;++n) {
    set(m,MathParameter::Waveform,waveform);set(m,MathParameter::HarmonicTerms,n);
    const auto& spectrum=m.snapshot().plots[1].series[0];require(spectrum.stems,"missing spectrum stems");double numericalError=0;
    for(unsigned j=0;j<16384;++j){const double t=2*pi*(j+.5)/16384;double sum=0;for(std::size_t k=1;k<spectrum.count;++k)sum+=spectrum.points[k].y*std::sin(k*t);const double error=sum-target(waveform,t);numericalError+=error*error/16384;}
    near(metric(m,"Full-period RMS error"),std::sqrt(numericalError),2e-6,"Parseval vs independent quadrature");
    for(unsigned probe=1;probe<=16;++probe){set(m,MathParameter::ProbeFrequency,probe);double integral=0;for(unsigned j=0;j<8192;++j){const double t=2*pi*(j+.5)/8192;integral+=2.0/8192*target(waveform,t)*std::sin(probe*t);}near(metric(m,"Analytic coefficient"),integral,5e-7,"analytic Fourier coefficient");near(metric(m,"Midpoint coefficient"),integral,4e-5,"midpoint Fourier coefficient");}
    inspect(m,scene);
  }
  set(m,MathParameter::Waveform,0);set(m,MathParameter::ProbeFrequency,2);checked(m,true);set(m,MathParameter::ProbeFrequency,1);checked(m,false);
  level(m,2);set(m,MathParameter::HarmonicTerms,6);checked(m,true);set(m,MathParameter::HarmonicTerms,1);checked(m,false);
  set(m,MathParameter::HarmonicTime,0);action(m,{MathActionKind::TogglePlayback});require(m.snapshot().playing,"play failed");action(m,{MathActionKind::AdvanceTime,{},{},12});near(m.parameter(MathParameter::HarmonicTime),2*pi,0,"time cap");require(!m.snapshot().playing,"play continued past window");reject(m,{MathActionKind::TogglePlayback});reject(m,{MathActionKind::AdvanceTime,{},{},.1});
  set(m,MathParameter::HarmonicTime,.5);const double before=metric(m,"Signal value");set(m,MathParameter::HarmonicTime,3);set(m,MathParameter::HarmonicTime,.5);near(metric(m,"Signal value"),before,0,"seek is not deterministic");
}
std::array<double,2> damped(double q0,double v0,double c,double t) {
  const double a=c/2;
  if(c<2){const double w=std::sqrt(1-a*a),b=(v0+a*q0)/w,e=std::exp(-a*t),q=e*(q0*std::cos(w*t)+b*std::sin(w*t));return {q,-a*q+e*w*(-q0*std::sin(w*t)+b*std::cos(w*t))};}
  if(c==2){const double b=v0+q0,q=std::exp(-t)*(q0+b*t);return {q,std::exp(-t)*b-q};}
  const double r1=-a+std::sqrt(a*a-1),r2=-a-std::sqrt(a*a-1),b=(v0-r2*q0)/(r1-r2),d=q0-b;
  return {b*std::exp(r1*t)+d*std::exp(r2*t),r1*b*std::exp(r1*t)+r2*d*std::exp(r2*t)};
}
void oscillators() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Oscillator,1);
  for(double k:{.25,1.0,4.0})for(double v:{-2.0,0.0,2.0})for(double time:{0.0,2.0,8.0,12.0}) {
    set(m,MathParameter::Stiffness,k);set(m,MathParameter::InitialVelocity,v);set(m,MathParameter::MotionTime,time);const double w=std::sqrt(k),q0=m.parameter(MathParameter::InitialPosition);
    near(metric(m,"Displacement"),q0*std::cos(w*time)+v/w*std::sin(w*time),2e-8,"undamped analytic position");near(metric(m,"Velocity"),-w*q0*std::sin(w*time)+v*std::cos(w*time),2e-8,"undamped analytic velocity");require(metric(m,"Energy balance error")<1e-7,"undamped energy drift");inspect(m,scene);
  }
  select(m,MathObjectKind::Oscillator,2);set(m,MathParameter::InitialPosition,.8);set(m,MathParameter::InitialVelocity,-.4);
  for(double c:{.5,2.0,3.0})for(double t:{0.0,2.0,8.0,12.0}) {
    set(m,MathParameter::Damping,c);set(m,MathParameter::MotionTime,t);const auto exact=damped(.8,-.4,c,t);
    near(metric(m,"Displacement"),exact[0],3e-8,"damped position");near(metric(m,"Velocity"),exact[1],3e-8,"damped velocity");require(metric(m,"Dissipated energy")>=0&&metric(m,"Energy balance error")<1e-7,"damping work balance");inspect(m,scene);
  }
  select(m,MathObjectKind::Oscillator,3);set(m,MathParameter::InitialPosition,0);set(m,MathParameter::Damping,0);
  for(double frequency:{1.0,2.0})for(double t:{2.0,8.0,12.0}) {
    set(m,MathParameter::DriveFrequency,frequency);set(m,MathParameter::MotionTime,t);
    const double exact=frequency==1?.25*t*std::sin(t):-.5/3*(std::cos(2*t)-std::cos(t));
    near(metric(m,"Displacement"),exact,3e-8,"forced analytic response");near(metric(m,"Convolution response"),exact,2e-7,"convolution vs analytic response");require(metric(m,"ODE/convolution difference")<2e-7,"ODE/convolution agreement");require(metric(m,"Energy balance error")<1e-7,"forced energy balance");inspect(m,scene);
  }
  for(double c:{.5,2.0,3.0})for(double k:{.25,1.0,4.0}) {
    set(m,MathParameter::Damping,c);set(m,MathParameter::Stiffness,k);set(m,MathParameter::InitialPosition,.8);set(m,MathParameter::InitialVelocity,-.4);set(m,MathParameter::DriveFrequency,3);set(m,MathParameter::MotionTime,12);
    require(metric(m,"ODE/convolution difference")<2e-6,"damped convolution does not match ODE");
    inspect(m,scene);
  }
  select(m,MathObjectKind::Oscillator,1);set(m,MathParameter::MotionSystem,1);set(m,MathParameter::InitialPosition,1.2);set(m,MathParameter::InitialVelocity,2);
  for(double k:{.25,1.0,4.0})for(double t:{0.0,4.0,12.0}){set(m,MathParameter::Stiffness,k);set(m,MathParameter::MotionTime,t);near(metric(m,"Mechanical energy"),2+k*(1-std::cos(1.2)),1e-7,"nonlinear pendulum conservation");inspect(m,scene);}
  set(m,MathParameter::MotionTime,3.5);const double position=metric(m,"Displacement");set(m,MathParameter::MotionTime,12);set(m,MathParameter::MotionTime,3.5);near(metric(m,"Displacement"),position,0,"motion seek is not deterministic");
  const auto& plots=m.snapshot().plots;near(plots[0].marker.y,position,0,"position marker");near(plots[1].marker.x,position,0,"phase marker");near(plots[1].marker.y,metric(m,"Velocity"),0,"phase velocity marker");
  select(m,MathObjectKind::Oscillator);set(m,MathParameter::InitialPosition,1);set(m,MathParameter::MotionTime,1.575);checked(m,true);
  level(m,1);set(m,MathParameter::MotionTime,4);checked(m,true);
  level(m,2);set(m,MathParameter::Damping,1);set(m,MathParameter::MotionTime,8);checked(m,true);
  level(m,3);set(m,MathParameter::Damping,.5);set(m,MathParameter::MotionTime,8);checked(m,true);
  set(m,MathParameter::MotionTime,0);action(m,{MathActionKind::TogglePlayback});action(m,{MathActionKind::AdvanceTime,{},{},.1});near(m.parameter(MathParameter::MotionTime),.1,1e-14,"advance duration");set(m,MathParameter::MotionTime,1);require(!m.snapshot().playing,"scrub did not pause");
  action(m,{MathActionKind::TogglePlayback});action(m,{MathActionKind::AdvanceTime,{},{},12});require(!m.snapshot().playing&&m.parameter(MathParameter::MotionTime)==12,"motion failed to stop at horizon");
}
void boundaries() {
  MathObjects m;MathObjectScene scene;const auto specs=mathParameterSpecs();require(specs.size()==static_cast<std::size_t>(MathParameter::Count),"incomplete parameter catalogue");
  for(std::size_t i=0;i<specs.size();++i)require(static_cast<std::size_t>(specs[i].id)==i&&!specs[i].key.empty(),"parameter index mismatch");
  require(mathObjectSpecs().size()==22,"wrong object count");
  for(const auto kind:{MathObjectKind::Symmetry,MathObjectKind::Harmonics,MathObjectKind::Oscillator})for(unsigned n=0;n<4;++n) {
    select(m,kind,n);require(mathLessons(kind).size()==4,"missing lessons");
    for(unsigned pass=0;pass<2;++pass)for(const auto& p:specs)if(m.parameterAvailable(p.id)){set(m,p.id,pass?p.maximum:p.minimum);inspect(m,scene);reject(m,{MathActionKind::SetParameter,{},p.id,std::numeric_limits<double>::quiet_NaN()});reject(m,{MathActionKind::SetParameter,{},p.id,p.maximum+1});}
    reject(m,{MathActionKind::SetLevel,{},{},4});reject(m,{MathActionKind::AdvanceTime,{},{},std::numeric_limits<double>::infinity()});reject(m,{MathActionKind::AdvanceTime,{},{},-1});
  }
  select(m,MathObjectKind::Oscillator,3);set(m,MathParameter::Damping,3);set(m,MathParameter::DriveAmplitude,1);level(m,0);require(!m.parameterAvailable(MathParameter::Damping)&&!m.parameterAvailable(MathParameter::DriveAmplitude),"hidden advanced parameters available");near(metric(m,"Dissipated energy"),0,0,"lower layer retained damping");near(metric(m,"Driving work"),0,0,"lower layer retained forcing");
  select(m,MathObjectKind::Algebra);reject(m,{MathActionKind::TogglePlayback});reject(m,{MathActionKind::AdvanceTime,{},{},1});reject(m,{MathActionKind::SymmetryTurn,{},{},0,0});require(!m.snapshot().symmetry.active,"symmetry diagram leaked");
}
}
int main(){try{symmetry();harmonics();oscillators();boundaries();std::printf("Exploration tests passed: 24 rotations and 576 products, Fourier quadrature, analytic oscillator solutions, challenges and %zu geometry states; maxima %zu vertices / %zu indices\n",states,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}

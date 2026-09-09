#include "RigidBody.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
constexpr double pi=3.14159265358979323846;
void bound(double v,double lo,double hi){if(!std::isfinite(v)||v<lo||v>hi)throw std::invalid_argument("rigid-body input outside finite domain");}
double dot(const RigidVector& a,const RigidVector& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
RigidVector cross(const RigidVector& a,const RigidVector& b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
RigidQuaternion product(const RigidQuaternion& a,const RigidQuaternion& b){return {a[0]*b[0]-a[1]*b[1]-a[2]*b[2]-a[3]*b[3],a[0]*b[1]+a[1]*b[0]+a[2]*b[3]-a[3]*b[2],a[0]*b[2]-a[1]*b[3]+a[2]*b[0]+a[3]*b[1],a[0]*b[3]+a[1]*b[2]-a[2]*b[1]+a[3]*b[0]};}
}
double rigidMagnitude(const RigidVector& v){return std::hypot(v[0],v[1],v[2]);}
RigidQuaternion rigidReleaseOrientation(const RigidVector& degrees){
  RigidQuaternion q{1,0,0,0};for(unsigned i=0;i<3;++i){bound(degrees[i],-180,180);const double angle=degrees[i]*pi/360;RigidQuaternion p{std::cos(angle),0,0,0};p[i+1]=std::sin(angle);q=product(p,q);}return q;
}
RigidMatrix rigidRotationMatrix(const RigidQuaternion& q){
  // q is an unmodified unit orientation returned by this module.
  const auto [w,x,y,z]=q;
  return {1-2*(y*y+z*z),2*(x*y-w*z),2*(x*z+w*y),2*(x*y+w*z),1-2*(x*x+z*z),2*(y*z-w*x),2*(x*z-w*y),2*(y*z+w*x),1-2*(x*x+y*y)};
}
RigidVector rigidRotate(const RigidQuaternion& q,const RigidVector& v){const auto r=rigidRotationMatrix(q);RigidVector out{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)out[i]+=r[3*i+j]*v[j];return out;}
RigidBody makeRigidBody(const RigidInput& input){
  if(input.shape>=RigidShape::Count)throw std::invalid_argument("unknown rigid-body shape");
  for(double x:input.dimensions)bound(x,.2,3.5);bound(input.mass,.2,5);bound(input.balance,.2,.8);
  for(double x:input.rotationDegrees)bound(x,-180,180);for(double x:input.omega)bound(x,-4,4);
  RigidBody body;body.mass=input.mass;const auto [w,h,d]=input.dimensions;const double m=input.mass,f=input.balance;
  const auto part=[&](RigidPrimitive primitive,double mass,RigidVector center,RigidVector size,unsigned material){body.parts[body.count++]={primitive,mass,center,size,material};};
  constexpr auto box=RigidPrimitive::Box;
  switch(input.shape){
    case RigidShape::Flywheel:part(RigidPrimitive::Cylinder,m,{0,0,0},{w,h,d},0);break;
    case RigidShape::Dumbbell:
      part(box,.9*m*f,{-.35*w,0,0},{.3*w,h,d},0);part(box,.9*m*(1-f),{.35*w,0,0},{.3*w,h,d},1);part(box,.1*m,{0,0,0},{.4*w,.15*h,.15*d},2);break;
    case RigidShape::Book:
      part(box,.1*m,{0,.47*h,0},{w,.06*h,d},0);part(box,.1*m,{0,-.47*h,0},{w,.06*h,d},1);part(box,.8*m,{0,0,0},{w,.88*h,d},2);break;
    case RigidShape::Satellite:
      part(box,.6*m,{0,0,0},{.28*w,h,.6*d},2);part(box,.35*m*f,{-.34*w,0,0},{.32*w,.06*h,d},0);part(box,.35*m*(1-f),{.34*w,0,0},{.32*w,.06*h,d},1);
      part(box,.025*m,{-.16*w,0,0},{.04*w,.12*h,.12*d},2);part(box,.025*m,{.16*w,0,0},{.04*w,.12*h,.12*d},2);break;
    case RigidShape::Count:throw std::invalid_argument("unknown rigid-body shape");
  }
  for(unsigned k=0;k<body.count;++k)for(unsigned i=0;i<3;++i)body.center[i]+=body.parts[k].mass*body.parts[k].center[i]/m;
  for(unsigned k=0;k<body.count;++k){auto& p=body.parts[k];for(unsigned i=0;i<3;++i)p.center[i]-=body.center[i];
    RigidVector second{};for(unsigned i=0;i<3;++i)second[i]=p.dimensions[i]*p.dimensions[i]/((p.primitive==RigidPrimitive::Cylinder&&i!=1)?16.:12.);
    for(unsigned i=0;i<3;++i){const unsigned j=(i+1)%3,l=(i+2)%3;body.inertia[i]+=p.mass*(second[j]+second[l]+p.center[j]*p.center[j]+p.center[l]*p.center[l]);}
    body.radius=std::max(body.radius,rigidMagnitude(p.center)+.5*rigidMagnitude(p.dimensions));
  }
  body.order={0,1,2};std::sort(body.order.begin(),body.order.end(),[&](unsigned a,unsigned b){return body.inertia[a]<body.inertia[b];});
  const double tolerance=1e-10*body.inertia[body.order[2]];
  body.distinctMoments=body.inertia[body.order[1]]-body.inertia[body.order[0]]>tolerance&&body.inertia[body.order[2]]-body.inertia[body.order[1]]>tolerance;
  return body;
}
RigidMotion::State RigidMotion::advance(State s,double duration) const {
  if(initialEnergy_==0)return s;
  const unsigned count=static_cast<unsigned>(std::ceil(duration/step_));if(!count)return s;
  const double h=duration/count;
  const auto rate=[&](const State& y){
    const RigidVector l{y[4],y[5],y[6]},omega{l[0]/body_.inertia[0],l[1]/body_.inertia[1],l[2]/body_.inertia[2]};
    const auto dl=cross(l,omega);const auto dq=product({y[0],y[1],y[2],y[3]},{0,omega[0],omega[1],omega[2]});
    return State{.5*dq[0],.5*dq[1],.5*dq[2],.5*dq[3],dl[0],dl[1],dl[2]};
  };
  const auto shifted=[](State a,const State& b,double scale){for(unsigned i=0;i<7;++i)a[i]+=scale*b[i];return a;};
  for(unsigned n=0;n<count;++n){const auto a=rate(s),b=rate(shifted(s,a,h/2)),c=rate(shifted(s,b,h/2)),d=rate(shifted(s,c,h));for(unsigned i=0;i<7;++i)s[i]+=h*(a[i]+2*b[i]+2*c[i]+d[i])/6;
    const double norm=std::sqrt(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]+s[3]*s[3]);for(unsigned i=0;i<4;++i)s[i]/=norm;
  }return s;
}
bool RigidMotion::configure(const RigidInput& input){
  if(revision_&&input==input_)return false;
  RigidMotion candidate;candidate.input_=input;candidate.body_=makeRigidBody(input);
  const auto q=rigidReleaseOrientation(input.rotationDegrees);RigidVector l{};for(unsigned i=0;i<3;++i)l[i]=candidate.body_.inertia[i]*input.omega[i];
  candidate.initialEnergy_=.5*dot(input.omega,l);candidate.initialWorldMomentum_=rigidRotate(q,l);
  candidate.speedBound_=std::sqrt(2*candidate.initialEnergy_/candidate.body_.inertia[candidate.body_.order[0]]);
  candidate.step_=std::min(1./480,.01/std::max(candidate.speedBound_,1e-12));
  constexpr double interval=horizon/(samples-1);
  const double steps=std::ceil(interval/candidate.step_)*(samples-1);
  if(steps>maxSteps)throw std::invalid_argument("rigid-body integration budget exceeded");
  candidate.steps_=static_cast<unsigned>(steps);candidate.cache_[0]={q[0],q[1],q[2],q[3],l[0],l[1],l[2]};
  for(unsigned i=1;i<samples;++i)candidate.cache_[i]=candidate.advance(candidate.cache_[i-1],interval);
  candidate.revision_=revision_+1;*this=candidate;return true;
}
const RigidBody& RigidMotion::body() const {if(!revision_)throw std::invalid_argument("rigid motion not configured");return body_;}
RigidState RigidMotion::describe(const State& raw,double time) const {
  RigidState s;s.time=time;std::copy_n(raw.begin(),4,s.orientation.begin());std::copy_n(raw.begin()+4,3,s.bodyMomentum.begin());
  for(unsigned i=0;i<3;++i)s.bodyOmega[i]=s.bodyMomentum[i]/body_.inertia[i];
  s.rotation=rigidRotationMatrix(s.orientation);s.worldMomentum=rigidRotate(s.orientation,s.bodyMomentum);s.worldOmega=rigidRotate(s.orientation,s.bodyOmega);s.energy=.5*dot(s.bodyOmega,s.bodyMomentum);
  RigidVector delta{};for(unsigned i=0;i<3;++i)delta[i]=s.worldMomentum[i]-initialWorldMomentum_[i];
  // Relative errors are zero for the exact stationary case. No invariant is
  // projected back to its initial value: these report actual integration drift.
  s.energyError=initialEnergy_>0?(s.energy-initialEnergy_)/initialEnergy_:0;
  const double momentum=rigidMagnitude(initialWorldMomentum_);s.momentumError=momentum>0?rigidMagnitude(delta)/momentum:0;
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)s.worldInertia[3*i+j]+=s.rotation[3*i+k]*body_.inertia[k]*s.rotation[3*j+k];
  return s;
}
RigidState RigidMotion::at(double time) const {
  if(!revision_)throw std::invalid_argument("rigid motion not configured");bound(time,0,horizon);
  constexpr double interval=horizon/(samples-1);const auto i=std::min(samples-1,static_cast<unsigned>(time/interval));
  return describe(advance(cache_[i],time-i*interval),time);
}
}

#include "Membrane.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
constexpr double pi=3.14159265358979323846;
void range(double value,double lo,double hi){if(!std::isfinite(value)||value<lo||value>hi)throw std::invalid_argument("membrane value outside finite domain");}
double sine(unsigned mode,double t){return t==0||t==1?0:std::sin(pi*mode*t);}
MembraneModeState evolve(const MembraneInput& input,MembraneMode mode,double time){
  MembraneModeState s;s.m=mode.m;s.n=mode.n;s.initialDisplacement=mode.displacement;s.initialVelocity=mode.velocity;
  const double lambda=pi*pi*(std::pow(mode.m/input.width,2)+std::pow(mode.n/input.depth,2));
  const double omega2=input.tension/input.density*lambda,gamma=input.damping;
  s.omega=std::sqrt(omega2);const double discriminant=omega2-gamma*gamma;
  double c=0,integral=0;
  if(discriminant>=0){
    const double w=std::sqrt(discriminant),angle=w*time,decay=std::exp(-gamma*time);
    c=decay*std::cos(angle);
    const double sinc=std::fabs(angle)<1e-4?1-angle*angle/6+angle*angle*angle*angle/120:std::sin(angle)/angle;
    integral=decay*time*sinc;
  }else{
    const double w=std::sqrt(-discriminant),slow=-omega2/(gamma+w),fast=-gamma-w;
    const double a=std::exp(slow*time),b=std::exp(fast*time);
    c=(a+b)/2;integral=a*(-std::expm1(-2*w*time))/(2*w);
  }
  s.q=mode.displacement*c+(mode.velocity+gamma*mode.displacement)*integral;
  s.velocity=mode.velocity*c-(omega2*mode.displacement+gamma*mode.velocity)*integral;
  s.acceleration=-2*gamma*s.velocity-omega2*s.q;
  const double massFactor=input.density*input.width*input.depth/8;
  s.kinetic=massFactor*s.velocity*s.velocity;s.potential=massFactor*omega2*s.q*s.q;
  return s;
}
}
MembraneState prepareMembrane(const MembraneInput& input,double time){
  range(input.width,1,4);range(input.depth,1,4);range(input.tension,.25,4);range(input.density,.5,2);range(input.damping,0,2);range(time,0,12);
  for(const auto& mode:input.modes){if(mode.m<1||mode.m>6||mode.n<1||mode.n>6)throw std::invalid_argument("membrane mode outside [1,6]");range(mode.displacement,-.6,.6);range(mode.velocity,-.6,.6);}
  MembraneState out;out.input=input;out.time=time;out.waveSpeed=std::sqrt(input.tension/input.density);
  std::array<MembraneMode,4> unique{};
  for(unsigned i=0;i<4;++i){const auto& mode=input.modes[i];out.slots[i]=evolve(input,mode,time);unsigned j=0;
    for(;j<out.combinedCount;++j)if(unique[j].m==mode.m&&unique[j].n==mode.n)break;
    if(j==out.combinedCount){unique[j]={mode.m,mode.n,0,0};++out.combinedCount;}
    unique[j].displacement+=mode.displacement;unique[j].velocity+=mode.velocity;
    out.maxOmega=std::max(out.maxOmega,out.slots[i].omega);
  }
  const double massFactor=input.density*input.width*input.depth/8;
  for(unsigned i=0;i<out.combinedCount;++i){const auto s=evolve(input,unique[i],time);out.combined[i]=s;
    out.kinetic+=s.kinetic;out.potential+=s.potential;
    out.initialEnergy+=massFactor*(s.initialVelocity*s.initialVelocity+s.omega*s.omega*s.initialDisplacement*s.initialDisplacement);
    if(s.initialDisplacement!=0||s.initialVelocity!=0)++out.activeModes;
  }
  out.lossRate=4*input.damping*out.kinetic;return out;
}
MembraneSample sampleMembrane(const MembraneState& state,double u,double v){
  range(u,0,1);range(v,0,1);MembraneSample out;
  for(unsigned i=0;i<4;++i){const auto& s=state.slots[i];out.weights[i]=sine(s.m,u)*sine(s.n,v);out.contributions[i]=s.q*out.weights[i];out.velocities[i]=s.velocity*out.weights[i];}
  for(unsigned i=0;i<state.combinedCount;++i){const auto& s=state.combined[i];
    const double kx=pi*s.m/state.input.width,ky=pi*s.n/state.input.depth,sx=sine(s.m,u),sy=sine(s.n,v),weight=sx*sy;
    const double h=s.q*weight;out.displacement+=h;out.velocity+=s.velocity*weight;out.acceleration+=s.acceleration*weight;
    out.dx+=s.q*kx*std::cos(pi*s.m*u)*sy;out.dy+=s.q*ky*sx*std::cos(pi*s.n*v);
    out.laplacian-=(kx*kx+ky*ky)*h;out.absoluteContributions+=std::fabs(h);
  }
  out.kineticDensity=state.input.density*out.velocity*out.velocity/2;
  out.potentialDensity=state.input.tension*(out.dx*out.dx+out.dy*out.dy)/2;return out;
}
}

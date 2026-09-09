#pragma once
#include <array>

namespace paths {
// u,v parameterize a rectangle with zero displacement on every edge.
// Uniform tension/density, linear transverse motion, and viscous damping 2*gamma.
struct MembraneMode { unsigned m=1,n=1; double displacement=0,velocity=0; };
struct MembraneInput {
  double width=3,depth=3,tension=1,density=1,damping=0;
  std::array<MembraneMode,4> modes{{{1,1,.35,0},{2,1,0,0},{1,2,0,0},{2,2,0,0}}};
};
struct MembraneModeState {
  unsigned m=1,n=1;
  double initialDisplacement=0,initialVelocity=0;
  double omega=0,q=0,velocity=0,acceleration=0,kinetic=0,potential=0;
};
struct MembraneState {
  MembraneInput input;
  double time=0,waveSpeed=0,kinetic=0,potential=0,initialEnergy=0,lossRate=0,maxOmega=0;
  std::array<MembraneModeState,4> slots{},combined{};
  unsigned combinedCount=0,activeModes=0;
};
struct MembraneSample {
  double displacement=0,velocity=0,acceleration=0,dx=0,dy=0,laplacian=0;
  double kineticDensity=0,potentialDensity=0,absoluteContributions=0;
  std::array<double,4> weights{},contributions{},velocities{};
};
// Validates before evaluation. Closed-form motion covers under-, critical and
// overdamping without timesteps. Duplicate (m,n) slots combine before energy.
// Dimensions [1,4], tension [.25,4], density [.5,2], damping [0,2], time [0,12];
// m,n in [1,6], each initial displacement/velocity in [-.6,.6]. Fixed storage.
MembraneState prepareMembrane(const MembraneInput&,double time);
// State must be an unmodified prepareMembrane result; u,v must lie in [0,1].
// Derivatives are with respect to physical coordinates x=width*u,y=depth*v.
MembraneSample sampleMembrane(const MembraneState&,double u,double v);
}

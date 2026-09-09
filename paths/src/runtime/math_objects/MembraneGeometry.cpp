#include "MembraneGeometry.hpp"
#include "MathObjects.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
void buildMembraneSurface(const MembraneState& state,MembraneDisplay display,MathTriangleSurface& out){
  if(display.subdivisions<12||display.subdivisions>48||display.selected>=4||static_cast<unsigned>(display.colour)>static_cast<unsigned>(MembraneColour::Energy))throw std::invalid_argument("invalid membrane display");
  auto shown=state;
  if(display.selectedOnly){auto input=state.input;for(unsigned i=0;i<4;++i)if(i!=display.selected)input.modes[i].displacement=input.modes[i].velocity=0;shown=prepareMembrane(input,state.time);}
  const unsigned n=display.subdivisions,stride=n+1;out.vertexCount=stride*stride;out.indexCount=0;
  const iggy3d::Vec3 neutral{.56F,.62F,.68F},blue{.23F,.46F,.76F},coral{.95F,.39F,.30F},gold{.98F,.75F,.28F};
  for(unsigned i=0;i<=n;++i)for(unsigned j=0;j<=n;++j){const double u=static_cast<double>(i)/n,v=static_cast<double>(j)/n;const auto sample=sampleMembrane(shown,u,v);
    const double length=std::hypot(sample.dx,1.0,sample.dy);iggy3d::Vec3 colour;
    if(display.colour==MembraneColour::Energy){const double e=sample.kineticDensity+sample.potentialDensity;const float t=static_cast<float>(e/(e+.25));colour=blue*(1-t)+gold*t;}
    else{const double value=display.colour==MembraneColour::SelectedBasis?sample.weights[display.selected]:sample.displacement;const float t=static_cast<float>(std::clamp(std::fabs(value)*(display.colour==MembraneColour::SelectedBasis?1:2),0.0,1.0));colour=neutral*(1-t)+(value<0?blue:coral)*t;}
    out.vertices[i*stride+j]={{static_cast<float>(state.input.width*(u-.5)),static_cast<float>(sample.displacement),static_cast<float>(state.input.depth*(.5-v))},{static_cast<float>(-sample.dx/length),static_cast<float>(1/length),static_cast<float>(sample.dy/length)},colour};
  }
  for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j){const unsigned a=i*stride+j,b=a+stride;for(unsigned index:{a,b,b+1,a,b+1,a+1})out.indices[out.indexCount++]=static_cast<std::uint16_t>(index);}
}
}

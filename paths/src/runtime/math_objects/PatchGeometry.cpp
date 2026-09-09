#include "PatchGeometry.hpp"
#include "MathObjects.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
iggy3d::Vec3 vector(BezierPoint p){return {static_cast<float>(p[0]),static_cast<float>(p[1]),static_cast<float>(p[2])};}
iggy3d::Vec3 colour(const PatchSample& s,PatchDisplay display){
  const iggy3d::Vec3 teal{.22F,.73F,.64F},gold{.98F,.75F,.28F},blue{.23F,.46F,.76F},coral{.95F,.39F,.30F},neutral{.60F,.66F,.69F};
  if(!s.regular)return neutral;
  switch(display.colour){
    case PatchColour::Material:return teal;
    case PatchColour::Influence:{const float t=static_cast<float>(std::clamp(3*s.weights[display.control],0.0,1.0));return teal*(1-t)+gold*t;}
    case PatchColour::Gaussian:{const float t=static_cast<float>(std::tanh(std::fabs(s.gaussian)*3));return neutral*(1-t)+(s.gaussian<0?coral:teal)*t;}
    case PatchColour::AreaDensity:{const float t=static_cast<float>(s.jacobian/(s.jacobian+9));return blue*(1-t)+gold*t;}
  }return neutral;
}
}
void buildPatchSurface(const BicubicPatch& p,PatchDisplay display,MathTriangleSurface& out){
  if(display.subdivisions<1||display.subdivisions>32||display.control>=16||static_cast<unsigned>(display.colour)>static_cast<unsigned>(PatchColour::AreaDensity))throw std::invalid_argument("invalid patch display");
  (void)samplePatch(p,0,0);const unsigned n=display.subdivisions,stride=n+1;out.vertexCount=stride*stride;out.indexCount=0;std::array<BezierPoint,33*33> positions{};
  for(unsigned i=0;i<=n;++i)for(unsigned j=0;j<=n;++j){const auto s=samplePatch(p,static_cast<double>(i)/n,static_cast<double>(j)/n);positions[i*stride+j]=s.position;out.vertices[i*stride+j]={vector(s.position),vector(s.normal),colour(s,display)};}
  const auto triangle=[&](unsigned a,unsigned b,unsigned c){const auto x=positions[a],y=positions[b],z=positions[c];
    const double ax=y[0]-x[0],ay=y[1]-x[1],az=y[2]-x[2],bx=z[0]-x[0],by=z[1]-x[1],bz=z[2]-x[2];
    if(std::hypot(ay*bz-az*by,az*bx-ax*bz,ax*by-ay*bx)<=1e-12)return;
    for(unsigned index:{a,b,c})out.indices[out.indexCount++]=static_cast<std::uint16_t>(index);
  };
  for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j){const unsigned a=i*stride+j,b=a+stride;triangle(a,b,b+1);triangle(a,b+1,a+1);}
}
}

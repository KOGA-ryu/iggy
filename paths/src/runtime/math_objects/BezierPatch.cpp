#include "BezierPatch.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
using V=BezierPoint;
struct Basis {std::array<double,4> value,first,second;};
Basis basis(double t){const double s=1-t;return {{s*s*s,3*t*s*s,3*t*t*s,t*t*t},{-3*s*s,3*s*s-6*t*s,6*t*s-3*t*t,3*t*t},{6*s,6*t-12*s,6*s-12*t,6*t}};}
void accumulate(V& out,const V& p,double w){for(unsigned k=0;k<3;++k)out[k]+=p[k]*w;}
V subtract(V a,V b){for(unsigned k=0;k<3;++k)a[k]-=b[k];return a;}
V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
double norm(V a){return std::hypot(a[0],a[1],a[2]);}
void validate(const BicubicPatch& p){for(auto c:p.controls)for(double x:c)if(!std::isfinite(x)||std::fabs(x)>2)throw std::invalid_argument("patch control outside finite [-2,2] domain");}
void count(unsigned n){if(n<1||n>32)throw std::invalid_argument("patch subdivisions outside [1,32]");}
PatchSample evaluate(const BicubicPatch& p,double u,double v){
  const auto a=basis(u),b=basis(v);PatchSample s;
  for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j){const auto k=4*i+j;const auto& c=p.controls[k];s.weights[k]=a.value[i]*b.value[j];
    accumulate(s.position,c,s.weights[k]);accumulate(s.du,c,a.first[i]*b.value[j]);accumulate(s.dv,c,a.value[i]*b.first[j]);
    accumulate(s.duu,c,a.second[i]*b.value[j]);accumulate(s.duv,c,a.first[i]*b.first[j]);accumulate(s.dvv,c,a.value[i]*b.second[j]);
  }
  const auto n=cross(s.du,s.dv);s.jacobian=norm(n);s.E=dot(s.du,s.du);s.F=dot(s.du,s.dv);s.G=dot(s.dv,s.dv);
  s.regular=s.jacobian>1e-12&&s.jacobian>1e-10*std::sqrt(s.E*s.G);
  if(s.regular){for(unsigned k=0;k<3;++k)s.normal[k]=n[k]/s.jacobian;s.e=dot(s.normal,s.duu);s.f=dot(s.normal,s.duv);s.g=dot(s.normal,s.dvv);
    const double determinant=s.jacobian*s.jacobian;s.gaussian=(s.e*s.g-s.f*s.f)/determinant;s.mean=(s.e*s.G-2*s.f*s.F+s.g*s.E)/(2*determinant);
    const double d=std::sqrt(std::max(0.0,s.mean*s.mean-s.gaussian));
    // Use the product for the smaller root to avoid H - sqrt(H^2-K)
    // cancellation on a surface that bends much more in one direction.
    const double large=s.mean+std::copysign(d,s.mean),small=large!=0?s.gaussian/large:0;
    s.principalMin=std::min(large,small);s.principalMax=std::max(large,small);
  }return s;
}
V position(const BicubicPatch& p,double u,double v){const auto a=basis(u),b=basis(v);V out{};for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)accumulate(out,p.controls[4*i+j],a.value[i]*b.value[j]);return out;}
}
PatchSample samplePatch(const BicubicPatch& p,double u,double v){validate(p);if(!std::isfinite(u)||!std::isfinite(v)||u<0||u>1||v<0||v>1)throw std::invalid_argument("patch probe outside [0,1]^2");return evaluate(p,u,v);}
double integratePatchArea(const BicubicPatch& p,unsigned n){
  validate(p);count(n);const double offset=1/std::sqrt(3.0);double area=0;
  for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j)for(double a:{-offset,offset})for(double b:{-offset,offset})area+=evaluate(p,(i+.5+.5*a)/n,(j+.5+.5*b)/n).jacobian;
  return area/(4.0*n*n);
}
PatchMeshMeasure measurePatchMesh(const BicubicPatch& p,unsigned n){
  validate(p);count(n);PatchMeshMeasure out;
  const auto triangle=[&](V a,V b,V c){const double twice=norm(cross(subtract(b,a),subtract(c,a)));if(twice<=1e-12)++out.skipped;else{out.area+=twice*.5;++out.triangles;}};
  std::array<V,33> previous{},next{};for(unsigned j=0;j<=n;++j)previous[j]=position(p,0,static_cast<double>(j)/n);
  for(unsigned i=0;i<n;++i){for(unsigned j=0;j<=n;++j)next[j]=position(p,static_cast<double>(i+1)/n,static_cast<double>(j)/n);
    for(unsigned j=0;j<n;++j){triangle(previous[j],next[j],next[j+1]);triangle(previous[j],next[j+1],previous[j+1]);}previous=next;
  }return out;
}
}

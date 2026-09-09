#include "runtime/math_objects/BezierCurve.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
using V=BezierPoint;
V add(V a,V b){for(unsigned i=0;i<3;++i)a[i]+=b[i];return a;}
V sub(V a,V b){for(unsigned i=0;i<3;++i)a[i]-=b[i];return a;}
V scale(V a,double s){for(auto& x:a)x*=s;return a;}
V mix(V a,V b,double t){return add(scale(a,1-t),scale(b,t));}
double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
double norm(V a){return std::hypot(a[0],a[1],a[2]);}
V unit(V a){const double n=norm(a);return n>0?scale(a,1/n):V{};}
void validate(const CubicBezier& c){for(auto p:c.controls)for(double v:p)if(!std::isfinite(v)||std::fabs(v)>2)throw std::invalid_argument("Bezier control outside finite [-2,2] domain");}
void fraction(double t){if(!std::isfinite(t)||t<0||t>1)throw std::invalid_argument("Bezier fraction outside [0,1]");}
BezierConstruction construct(const CubicBezier& c,double t) {
  BezierConstruction r;
  for(unsigned i=0;i<3;++i)r.first[i]=mix(c.controls[i],c.controls[i+1],t);
  for(unsigned i=0;i<2;++i)r.second[i]=mix(r.first[i],r.first[i+1],t);
  r.point=mix(r.second[0],r.second[1],t);return r;
}
BezierSample sample(const CubicBezier& c,double t) {
  const auto construction=construct(c,t);BezierSample r;r.position=construction.point;
  r.first=scale(sub(construction.second[1],construction.second[0]),3);
  r.second=scale(add(sub(construction.first[2],scale(construction.first[1],2)),construction.first[0]),6);
  r.speed=norm(r.first);
  double controlScale=0;for(unsigned i=0;i<3;++i)controlScale=std::max(controlScale,norm(sub(c.controls[i+1],c.controls[i])));
  r.regular=r.speed>1e-10*controlScale;
  if(r.regular){r.tangent=scale(r.first,1/r.speed);r.curvature=norm(cross(r.first,r.second))/(r.speed*r.speed*r.speed);}
  return r;
}
CubicBezier interval(const CubicBezier& c,double a,double b) {
  const auto start=sample(c,a),end=sample(c,b);const double dt=(b-a)/3;
  return {{start.position,add(start.position,scale(start.first,dt)),sub(end.position,scale(end.first,dt)),end.position}};
}
BezierLengthBounds lengthBounds(const CubicBezier& c,double tolerance,unsigned depth=0) {
  const double lower=norm(sub(c.controls[3],c.controls[0]));double upper=0;
  for(unsigned i=0;i<3;++i)upper+=norm(sub(c.controls[i+1],c.controls[i]));
  upper=std::max(lower,upper);
  if(upper-lower<=tolerance||depth==12)return {lower,upper};
  const auto m=construct(c,.5);
  const auto left=lengthBounds({{c.controls[0],m.first[0],m.second[0],m.point}},tolerance/2,depth+1);
  const auto right=lengthBounds({{m.point,m.second[1],m.first[2],c.controls[3]}},tolerance/2,depth+1);
  return {left.lower+right.lower,left.upper+right.upper};
}
bool regularCurve(const CubicBezier& c) {
  if(!sample(c,0).regular||!sample(c,1).regular)return false;
  const auto d0=scale(sub(c.controls[1],c.controls[0]),3),d1=scale(sub(c.controls[2],c.controls[1]),3),d2=scale(sub(c.controls[3],c.controls[2]),3);
  const auto a=add(sub(d2,scale(d1,2)),d0),b=scale(sub(d1,d0),2);
  // A zero vector derivative must be a root of every nonzero coordinate
  // polynomial. Solve the largest coordinate and check its candidates in 3D.
  unsigned axis=0;double largest=0;
  for(unsigned i=0;i<3;++i){const double size=std::fabs(a[i])+std::fabs(b[i])+std::fabs(d0[i]);if(size>largest){axis=i;largest=size;}}
  const double aa=a[axis],bb=b[axis],cc=d0[axis],epsilon=1e-13*largest;
  const auto stationary=[&](double t){return t>=0&&t<=1&&!sample(c,t).regular;};
  if(std::fabs(aa)<=epsilon)return std::fabs(bb)<=epsilon||!stationary(-cc/bb);
  const double discriminant=bb*bb-4*aa*cc,discTolerance=1e-13*(bb*bb+std::fabs(4*aa*cc));
  if(discriminant< -discTolerance)return true;
  const double root=std::sqrt(std::max(0.0,discriminant));
  const double q=-.5*(bb+std::copysign(root,bb));
  if(q==0)return !stationary(-bb/(2*aa));
  return !stationary(q/aa)&&!stationary(cc/q);
}
}
BezierConstruction constructBezier(const CubicBezier& c,double t){validate(c);fraction(t);return construct(c,t);}
BezierSample sampleBezier(const CubicBezier& c,double t){validate(c);fraction(t);return sample(c,t);}
BezierArcTable analyzeBezier(const CubicBezier& c) {
  validate(c);BezierArcTable table;table.curve=c;table.regular=regularCurve(c);
  for(std::size_t i=1;i<table.kKnots;++i) {
    const double a=static_cast<double>(i-1)/(table.kKnots-1),b=static_cast<double>(i)/(table.kKnots-1);
    const auto bound=lengthBounds(interval(c,a,b),1e-9/(table.kKnots-1));
    table.bounds.lower+=bound.lower;table.bounds.upper+=bound.upper;
    table.cumulative[i]=table.cumulative[i-1]+bound.estimate();
  }
  return table;
}
double bezierArcAt(const BezierArcTable& table,double t) {
  fraction(t);if(t==1)return table.length();if(t==0)return 0;
  const double scaled=t*(table.kKnots-1);const auto i=static_cast<std::size_t>(scaled);
  const double start=static_cast<double>(i)/(table.kKnots-1);
  return table.cumulative[i]+lengthBounds(interval(table.curve,start,t),1e-10).estimate();
}
double bezierParameterAtFraction(const BezierArcTable& table,double f) {
  fraction(f);if(table.length()==0||f==0)return 0;if(f==1)return 1;
  const double target=f*table.length();
  const auto upper=std::upper_bound(table.cumulative.begin(),table.cumulative.end(),target);
  const auto i=std::min<std::size_t>(table.kKnots-2,static_cast<std::size_t>(upper-table.cumulative.begin()-1));
  double lo=static_cast<double>(i)/(table.kKnots-1),hi=static_cast<double>(i+1)/(table.kKnots-1);
  for(unsigned step=0;step<22;++step){const double mid=(lo+hi)/2;if(bezierArcAt(table,mid)<target)lo=mid;else hi=mid;}
  return (lo+hi)/2;
}
BezierFrame seedBezierFrame(const BezierPoint& tangent) {
  const double length=norm(tangent);if(!std::isfinite(length)||length==0)return {};
  const auto t=unit(tangent);unsigned axis=0;for(unsigned i=1;i<3;++i)if(std::fabs(t[i])<std::fabs(t[axis]))axis=i;
  V reference{};reference[axis]=1;const auto n=unit(sub(reference,scale(t,dot(reference,t))));
  return {t,n,cross(t,n),true};
}
BezierFrame transportBezierFrame(const BezierFrame& previous,const BezierPoint& tangent) {
  if(!previous.defined)return seedBezierFrame(tangent);
  const auto next=seedBezierFrame(tangent);if(!next.defined)return {};
  const auto t=next.tangent,v=cross(previous.tangent,t);const double sine=norm(v),cosine=std::clamp(dot(previous.tangent,t),-1.0,1.0);
  V n=previous.normal;
  if(sine>1e-12){const auto axis=scale(v,1/sine);n=add(add(scale(n,cosine),scale(cross(axis,n),sine)),scale(axis,dot(axis,n)*(1-cosine)));}
  n=sub(n,scale(t,dot(n,t)));if(norm(n)<1e-12)return next;n=unit(n);
  return {t,n,cross(t,n),true};
}
}

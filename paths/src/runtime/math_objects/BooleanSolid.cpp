#include "runtime/math_objects/BooleanSolid.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
constexpr double pi=3.14159265358979323846;
double dot(SolidPoint a,SolidPoint b){double v=0;for(unsigned j=0;j<3;++j)v+=a[j]*b[j];return v;}
double length(SolidPoint a){return std::sqrt(dot(a,a));}
double sign(double a){return a<0?-1:1;}
SolidSample box(SolidPoint p,SolidPoint half){
  SolidPoint q{},outside{};for(unsigned j=0;j<3;++j){q[j]=std::fabs(p[j])-half[j];outside[j]=std::max(q[j],0.0);}
  const double norm=length(outside),largest=*std::max_element(q.begin(),q.end());SolidSample s;s.value=norm+std::min(largest,0.0);
  if(norm>1e-12){for(unsigned j=0;j<3;++j)s.gradient[j]=outside[j]*sign(p[j])/norm;s.regular=true;}
  else {unsigned selected=0,ties=0;for(unsigned j=0;j<3;++j)if(std::fabs(q[j]-largest)<1e-10){selected=j;++ties;}s.gradient[selected]=sign(p[selected]);s.regular=ties==1&&std::fabs(p[selected])>1e-12;}
  return s;
}
SolidSample sphere(SolidPoint p){const double n=length(p);SolidSample s;s.value=n-1;s.regular=n>1e-12;if(s.regular)for(unsigned j=0;j<3;++j)s.gradient[j]=p[j]/n;return s;}
SolidSample cylinder(SolidPoint p){
  const double r=std::hypot(p[0],p[1]),a=r-1,b=std::fabs(p[2])-3,oa=std::max(a,0.0),ob=std::max(b,0.0),outside=std::hypot(oa,ob);SolidSample s;s.value=outside+std::min(std::max(a,b),0.0);
  double radial=0,axial=0;if(outside>1e-12){radial=oa/outside;axial=ob/outside;s.regular=true;}else{radial=a>=b?1:0;axial=1-radial;s.regular=std::fabs(a-b)>1e-10;}
  if(radial>0){s.regular=s.regular&&r>1e-12;if(r>1e-12){s.gradient[0]=radial*p[0]/r;s.gradient[1]=radial*p[1]/r;}}
  s.gradient[2]=axial*sign(p[2]);if(axial>0&&std::fabs(p[2])<=1e-12)s.regular=false;return s;
}
SolidSample localSample(SolidShape shape,SolidPoint p){
  switch(shape){
    case SolidShape::Box:return box(p,{1.2,1.1,.7});
    case SolidShape::Sphere:return sphere(p);
    case SolidShape::Cylinder:return cylinder(p);
    case SolidShape::Arch:{auto lower=p,upper=p;lower[1]+=.9;upper[1]-=.2;return combineSolids(box(lower,{1,1.1,3}),cylinder(upper),SolidOperation::Union,0);}
    case SolidShape::Count:break;
  }throw std::invalid_argument("unknown solid shape");
}
PreparedPrimitive preparePrimitive(const SolidPrimitive& p){
  if(p.shape>=SolidShape::Count||!std::isfinite(p.size)||p.size<.2||p.size>1.5||!std::isfinite(p.yaw)||!std::isfinite(p.pitch)||std::fabs(p.yaw)>180||std::fabs(p.pitch)>180)throw std::invalid_argument("invalid solid primitive");
  for(double x:p.center)if(!std::isfinite(x)||std::fabs(x)>1.5)throw std::invalid_argument("invalid solid position");
  const double y=p.yaw*pi/180,t=p.pitch*pi/180,cy=std::cos(y),sy=std::sin(y),ct=std::cos(t),st=std::sin(t);
  PreparedPrimitive q{p,{{{cy,0,-sy},{sy*st,ct,cy*st},{sy*ct,-st,cy*ct}}},{}};
  constexpr std::array<SolidPoint,4> lower{{{-1.2,-1.1,-.7},{-1,-1,-1},{-1,-1,-3},{-1,-2,-3}}};
  constexpr std::array<SolidPoint,4> upper{{{1.2,1.1,.7},{1,1,1},{1,1,3},{1,1.2,3}}};
  q.bounds.minimum=q.bounds.maximum=p.center;
  for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k){const double lo=p.size*q.axes[k][j]*lower[static_cast<unsigned>(p.shape)][k],hi=p.size*q.axes[k][j]*upper[static_cast<unsigned>(p.shape)][k];q.bounds.minimum[j]+=std::min(lo,hi);q.bounds.maximum[j]+=std::max(lo,hi);}
  return q;
}
}
BooleanSolid prepareBoolean(const BooleanInput& input){
  if(input.operation>=SolidOperation::Count||!std::isfinite(input.blend)||input.blend<0||input.blend>.6)throw std::invalid_argument("invalid Boolean operation");
  BooleanSolid s{input,preparePrimitive(input.a),preparePrimitive(input.b),{}};
  // Common domain across all sample counts. A contains every point of A minus B.
  // A polynomial blend expands the hard union by at most blend/4 in field value.
  for(unsigned j=0;j<3;++j){s.bounds.minimum[j]=input.operation==SolidOperation::Difference?s.a.bounds.minimum[j]:std::min(s.a.bounds.minimum[j],s.b.bounds.minimum[j]);s.bounds.maximum[j]=input.operation==SolidOperation::Difference?s.a.bounds.maximum[j]:std::max(s.a.bounds.maximum[j],s.b.bounds.maximum[j]);const double margin=.17+(input.operation==SolidOperation::SmoothUnion?input.blend/4:0);s.bounds.minimum[j]-=margin;s.bounds.maximum[j]+=margin;}
  return s;
}
SolidSample samplePrimitive(const PreparedPrimitive& p,SolidPoint point){
  SolidPoint offset{},local{};for(unsigned j=0;j<3;++j)offset[j]=point[j]-p.input.center[j];for(unsigned j=0;j<3;++j)local[j]=dot(offset,p.axes[j])/p.input.size;
  auto s=localSample(p.input.shape,local);const auto gradient=s.gradient;s.value*=p.input.size;s.gradient={};for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)s.gradient[j]+=p.axes[k][j]*gradient[k];return s;
}
SolidSample combineSolids(SolidSample a,SolidSample b,SolidOperation op,double width){
  if(op==SolidOperation::SmoothUnion&&width>0){
    const double h=std::clamp(.5+(b.value-a.value)/(2*width),0.0,1.0);
    if(h==1)return a;if(h==0)return b;
    SolidSample s;s.value=h*a.value+(1-h)*b.value-width*h*(1-h);s.material=h*a.material+(1-h)*b.material;
    for(unsigned j=0;j<3;++j)s.gradient[j]=h*a.gradient[j]+(1-h)*b.gradient[j];s.regular=a.regular&&b.regular&&length(s.gradient)>1e-10;return s;
  }
  if(op==SolidOperation::Difference){b.value=-b.value;for(auto& x:b.gradient)x=-x;}
  const bool minimum=op==SolidOperation::Union||op==SolidOperation::SmoothUnion;
  auto s=(minimum?a.value<=b.value:a.value>=b.value)?a:b;
  if(std::fabs(a.value-b.value)<1e-10){SolidPoint difference{};for(unsigned j=0;j<3;++j)difference[j]=a.gradient[j]-b.gradient[j];s.regular=a.regular&&b.regular&&length(difference)<1e-9;}
  return s;
}
SolidSample sampleBoolean(const BooleanSolid& solid,SolidPoint p){auto a=samplePrimitive(solid.a,p),b=samplePrimitive(solid.b,p);b.material=1;return combineSolids(a,b,solid.input.operation,solid.input.blend);}
bool booleanTruth(SolidOperation op,bool a,bool b){switch(op){case SolidOperation::Union:case SolidOperation::SmoothUnion:return a||b;case SolidOperation::Intersection:return a&&b;case SolidOperation::Difference:return a&&!b;case SolidOperation::Count:break;}throw std::invalid_argument("unknown Boolean truth operation");}
SolidProbe probeBooleanSurface(const BooleanSolid& s,SolidPoint point){
  for(double x:point)if(!std::isfinite(x))throw std::invalid_argument("nonfinite solid probe");
  SolidProbe result;double best=1e100;const double left=s.bounds.minimum[0],right=s.bounds.maximum[0];auto p=point;p[0]=left;double previous=sampleBoolean(s,p).value;
  for(unsigned i=1;i<=128;++i){double a=left+(right-left)*(i-1)/128,b=left+(right-left)*i/128;p[0]=b;const double next=sampleBoolean(s,p).value;
    if((previous<0)!=(next<0)){const bool negative=previous<0;for(unsigned k=0;k<36;++k){const double mid=(a+b)/2;p[0]=mid;if((sampleBoolean(s,p).value<0)==negative)a=mid;else b=mid;}p[0]=(a+b)/2;const double distance=std::fabs(p[0]-point[0]);if(distance<best){best=distance;result={p,sampleBoolean(s,p),true};}}
    previous=next;
  }return result;
}
SolidVolume measureBooleanVolume(const BooleanSolid& s,unsigned n){
  if(n<4||n>64)throw std::invalid_argument("solid volume samples outside 4..64");
  SolidPoint step{};double cell=1;for(unsigned j=0;j<3;++j){step[j]=(s.bounds.maximum[j]-s.bounds.minimum[j])/n;cell*=step[j];}
  SolidVolume v{n,0,0,cell};for(unsigned z=0;z<n;++z)for(unsigned y=0;y<n;++y)for(unsigned x=0;x<n;++x){SolidPoint p{s.bounds.minimum[0]+(x+.5)*step[0],s.bounds.minimum[1]+(y+.5)*step[1],s.bounds.minimum[2]+(z+.5)*step[2]};v.inside+=sampleBoolean(s,p).value<0;}
  v.volume=v.inside*cell;return v;
}
} // namespace paths

#include "runtime/math_objects/DistanceGeometry.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
using Matrix=std::array<double,9>;
double dot(const DistancePoint& a,const DistancePoint& b){double r=0;for(unsigned i=0;i<3;++i)r+=a[i]*b[i];return r;}
void validate(const DistanceEdges& d){for(double x:d)if(!std::isfinite(x)||x<0||x>64)throw std::invalid_argument("squared distances must be finite in [0,64]");}
struct Spectrum{std::array<double,3> values{};Matrix vectors{1,0,0,0,1,0,0,0,1};double residual=0;};
Spectrum spectrum(Matrix a){
  const auto original=a;Spectrum out;
  for(unsigned sweep=0;sweep<32;++sweep){
    double largest=0;for(auto pair:{std::array<unsigned,2>{0,1},{0,2},{1,2}}){
      const unsigned p=pair[0],q=pair[1];const double apq=a[3*p+q];largest=std::max(largest,std::fabs(apq));if(std::fabs(apq)<=2e-16)continue;
      const double tau=(a[3*q+q]-a[3*p+p])/(2*apq),t=std::copysign(1.,tau)/(std::fabs(tau)+std::hypot(1.,tau)),c=1/std::hypot(1.,t),s=t*c;
      const double app=a[3*p+p],aqq=a[3*q+q];a[3*p+p]=app-t*apq;a[3*q+q]=aqq+t*apq;a[3*p+q]=a[3*q+p]=0;
      for(unsigned k=0;k<3;++k){if(k!=p&&k!=q){const double x=a[3*k+p],y=a[3*k+q];a[3*k+p]=a[3*p+k]=c*x-s*y;a[3*k+q]=a[3*q+k]=s*x+c*y;}const double x=out.vectors[3*k+p],y=out.vectors[3*k+q];out.vectors[3*k+p]=c*x-s*y;out.vectors[3*k+q]=s*x+c*y;}
    }
    if(largest<=2e-16)break;
  }
  for(unsigned i=0;i<3;++i)out.values[i]=a[3*i+i];
  for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j)if(out.values[j]>out.values[i]){std::swap(out.values[i],out.values[j]);for(unsigned k=0;k<3;++k)std::swap(out.vectors[3*k+i],out.vectors[3*k+j]);}
  for(unsigned i=0;i<3;++i)for(unsigned k=0;k<3;++k){double x=0;for(unsigned j=0;j<3;++j)x+=original[3*i+j]*out.vectors[3*j+k];out.residual=std::max(out.residual,std::fabs(x-out.values[k]*out.vectors[3*i+k]));}
  return out;
}
}
DistanceEdges squaredDistances(const DistancePoints& p){
  for(const auto& point:p)for(double x:point)if(!std::isfinite(x))throw std::invalid_argument("nonfinite distance point");
  DistanceEdges d{};for(unsigned e=0;e<6;++e){const auto pair=distancePairs[e];for(unsigned k=0;k<3;++k){const double x=p[pair[0]][k]-p[pair[1]][k];d[e]+=x*x;}}validate(d);return d;
}
DistanceEdges mixDistances(const DistanceEdges& a,const DistanceEdges& b,double t,double scale){
  validate(a);validate(b);if(!std::isfinite(t)||t<0||t>1||!std::isfinite(scale)||scale<0||scale>4)throw std::invalid_argument("invalid distance mixture");
  DistanceEdges out{};for(unsigned i=0;i<6;++i)out[i]=scale*((1-t)*a[i]+t*b[i]);validate(out);return out;
}
DistanceEdges distanceExample(unsigned n){
  switch(n){case 0:return {4,4,4,4,4,4};case 1:return {4,8,4,4,8,4};case 2:return {1,4,9,1,4,1};case 3:return {1,1,4,4,1,9};case 4:return {};default:throw std::invalid_argument("unknown distance example");}
}
DistanceAnalysis analyzeDistances(const DistanceEdges& d,bool mirror){
  validate(d);DistanceAnalysis out;for(unsigned e=0;e<6;++e){const auto ij=distancePairs[e];out.squared[4*ij[0]+ij[1]]=out.squared[4*ij[1]+ij[0]]=d[e];out.scale=std::max(out.scale,d[e]);}
  out.tolerance=1e-10*out.scale;
  if(out.scale==0){out.realizable=out.trianglesPass=true;return out;}
  out.minTriangleSlack=8;
  for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b)for(unsigned c=b+1;c<4;++c){const double x=std::sqrt(out.squared[4*a+b]),y=std::sqrt(out.squared[4*a+c]),z=std::sqrt(out.squared[4*b+c]);out.minTriangleSlack=std::min({out.minTriangleSlack,x+y-z,x+z-y,y+z-x});}
  out.trianglesPass=out.minTriangleSlack>=-1e-10*std::sqrt(out.scale);
  Matrix normalized{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){normalized[3*i+j]=.5*(out.squared[i+1]/out.scale+out.squared[j+1]/out.scale-out.squared[4*(i+1)+j+1]/out.scale);out.gram[3*i+j]=normalized[3*i+j]*out.scale;}
  const auto eig=spectrum(normalized);out.eigenResidual=eig.residual*out.scale;
  for(unsigned k=0;k<3;++k)out.eigenvalues[k]=eig.values[k]*out.scale;
  out.realizable=eig.values[2]>=-1e-10;
  if(!out.realizable){
    for(unsigned i=0;i<3;++i){out.witness[i+1]=eig.vectors[3*i+2];out.witness[0]-=out.witness[i+1];}
    double length=0;for(double x:out.witness)length+=x*x;length=std::sqrt(length);for(auto& x:out.witness)x/=length;
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)out.witnessValue+=out.witness[i]*out.squared[4*i+j]*out.witness[j];
    return out;
  }
  for(double x:eig.values)out.dimension+=x>1e-10;
  // A rank-truncated Gram square root, then a canonical point-based frame.
  // The frame removes arbitrary eigenvector signs/rotations (including repeated
  // eigenvalues). Ties choose the first point; pivot changes can change the view.
  DistancePoints raw{};for(unsigned i=0;i<3;++i)for(unsigned k=0;k<out.dimension;++k)raw[i+1][k]=eig.vectors[3*i+k]*std::sqrt(eig.values[k]);
  std::array<DistancePoint,3> axes{};
  for(unsigned k=0;k<out.dimension;++k){double longest=-1;DistancePoint best{};
    for(unsigned i=1;i<4;++i){auto v=raw[i];for(unsigned j=0;j<k;++j){const double projection=dot(v,axes[j]);for(unsigned n=0;n<3;++n)v[n]-=projection*axes[j][n];}const double norm=dot(v,v);if(norm>longest+1e-14){longest=norm;best=v;}}
    const double norm=std::sqrt(dot(best,best));if(!(norm>0))throw std::logic_error("distance reconstruction lost a numerical axis");for(unsigned n=0;n<3;++n)axes[k][n]=best[n]/norm;
  }
  for(unsigned i=1;i<4;++i)for(unsigned k=0;k<out.dimension;++k)out.points[i][k]=dot(raw[i],axes[k])*std::sqrt(out.scale)*(mirror&&k==2?-1:1);
  for(unsigned e=0;e<6;++e){const auto ij=distancePairs[e];double actual=0;for(unsigned k=0;k<3;++k){const double x=out.points[ij[0]][k]-out.points[ij[1]][k];actual+=x*x;}out.reconstructionError=std::max(out.reconstructionError,std::fabs(actual-d[e]));}
  if(out.dimension==3)out.volume=std::sqrt(eig.values[0]*eig.values[1]*eig.values[2])*out.scale*std::sqrt(out.scale)/6;
  return out;
}
}

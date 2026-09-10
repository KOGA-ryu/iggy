#include "runtime/math_objects/QrLeastSquares.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
double dot(const QrVector& a,const QrVector& b){double out=0;for(unsigned i=0;i<3;++i)out+=a[i]*b[i];return out;}
double norm(const QrVector& a){return std::hypot(a[0],a[1],a[2]);}
void validate(const QrColumns& a,const QrVector& b){for(auto col:a)for(double x:col)if(!std::isfinite(x)||std::fabs(x)>4)throw std::invalid_argument("QR entries must be finite in [-4,4]");for(double x:b)if(!std::isfinite(x)||std::fabs(x)>4)throw std::invalid_argument("QR target must be finite in [-4,4]");}
}
QrTrial evaluateQrTrial(const QrColumns& a,const QrVector& b,const QrCoefficients& x){
  validate(a,b);for(double c:x)if(!std::isfinite(c)||std::fabs(c)>1e120)throw std::invalid_argument("QR coefficients outside finite trial range");
  QrTrial out;for(unsigned i=0;i<3;++i){out.fitted[i]=a[0][i]*x[0]+a[1][i]*x[1];out.residual[i]=b[i]-out.fitted[i];}out.squaredError=dot(out.residual,out.residual);out.normalResidual=std::hypot(dot(a[0],out.residual),dot(a[1],out.residual));return out;
}
QrAnalysis analyzeQr(const QrColumns& a,const QrVector& b){
  validate(a,b);double scale=0;for(auto col:a)for(double x:col)scale=std::max(scale,std::fabs(x));
  if(scale>0&&scale<1e-100)throw std::invalid_argument("QR nonzero scale must be at least 1e-100");
  QrAnalysis out;out.residual=b;out.residualSquared=dot(b,b);
  if(scale==0){out.nullBasis={QrCoefficients{1,0},QrCoefficients{0,1}};return out;}
  QrColumns normalized=a;for(auto& col:normalized)for(double& x:col)x/=scale;
  if(norm(normalized[1])>norm(normalized[0]))out.order={1,0};
  const auto& first=normalized[out.order[0]];const auto& second=normalized[out.order[1]];
  const double r00=norm(first);for(unsigned i=0;i<3;++i)out.q[0][i]=first[i]/r00;
  auto remainder=second;double r01=0;
  for(unsigned pass=0;pass<2;++pass){const double component=dot(out.q[0],remainder);r01+=component;for(unsigned i=0;i<3;++i)remainder[i]-=component*out.q[0][i];}
  const double remainderNorm=norm(remainder);out.tolerance=1e-10*r00*scale;
  out.rank=remainderNorm>1e-10*r00?2:1;
  const double r11=out.rank==2?remainderNorm:0;
  if(out.rank==2)for(unsigned i=0;i<3;++i)out.q[1][i]=remainder[i]/r11;
  out.r={r00*scale,r01*scale,0,r11*scale};
  for(unsigned i=0;i<3;++i){out.removed[i]=r01*scale*out.q[0][i];out.remainder[i]=remainder[i]*scale;}
  const double c0=dot(out.q[0],b),c1=out.rank==2?dot(out.q[1],b):0;
  QrCoefficients permuted{};
  if(out.rank==2){permuted[1]=c1/r11;permuted[0]=(c0-r01*permuted[1])/r00;}
  else {const double denominator=r00*r00+r01*r01;permuted={r00*c0/denominator,r01*c0/denominator};const double length=std::hypot(r00,r01);out.nullBasis[0][out.order[0]]=-r01/length;out.nullBasis[0][out.order[1]]=r00/length;}
  for(unsigned i=0;i<2;++i)out.solution[out.order[i]]=permuted[i]/scale;
  for(unsigned i=0;i<3;++i){out.projected[i]=c0*out.q[0][i]+c1*out.q[1][i];out.residual[i]=b[i]-out.projected[i];}
  out.residualSquared=dot(out.residual,out.residual);out.normalResidual=std::hypot(dot(a[0],out.residual),dot(a[1],out.residual));
  double error=0,total=0;for(unsigned col=0;col<2;++col)for(unsigned i=0;i<3;++i){const double expected=normalized[out.order[col]][i],actual=col==0?r00*out.q[0][i]:r01*out.q[0][i]+r11*out.q[1][i];error=std::hypot(error,actual-expected);total=std::hypot(total,expected);}
  out.reconstructionError=error/total;
  for(unsigned i=0;i<out.rank;++i)for(unsigned j=0;j<out.rank;++j)out.orthogonalityError=std::hypot(out.orthogonalityError,dot(out.q[i],out.q[j])-(i==j?1.:0.));
  return out;
}
}

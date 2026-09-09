#include "runtime/math_objects/Simplex.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace paths {
namespace {
void requirePoint(const SimplexPoint& p) {
  if(!isSimplexPoint(p))throw std::invalid_argument("invalid probability distribution");
}
}
bool isSimplexPoint(const SimplexPoint& p) {
  double sum=0;
  for(double x:p){if(!std::isfinite(x)||x<0||x>1)return false;sum+=x;}
  return std::fabs(sum-1)<=1e-12;
}
SimplexPoint simplexPoint(double a,double b) {
  if(!std::isfinite(a)||!std::isfinite(b)||a<0||b<0||a>1||b>1||a+b>1+1e-12)
    throw std::invalid_argument("probabilities must be nonnegative with A+B <= 1");
  b=std::min(b,1-a);return {a,b,std::max(0.,1-(a+b))};
}
SimplexPoint mixSimplex(const SimplexPoint& p,const SimplexPoint& q,double t) {
  requirePoint(p);requirePoint(q);
  if(!std::isfinite(t)||t<0||t>1)throw std::invalid_argument("invalid mixture amount");
  if(t==0)return p;if(t==1)return q;
  SimplexPoint result{};for(unsigned i=0;i<3;++i)result[i]=(1-t)*p[i]+t*q[i];return result;
}
double simplexFunction(SimplexFunction f,const SimplexPoint& p,const SimplexPoint& outcomes) {
  requirePoint(p);
  for(double x:outcomes)if(!std::isfinite(x)||std::fabs(x)>3)throw std::invalid_argument("outcome outside [-3,3]");
  double mean=0,entropy=0;for(unsigned i=0;i<3;++i){mean+=p[i]*outcomes[i];if(p[i]>0)entropy-=p[i]*std::log(p[i]);}
  switch(f){
    case SimplexFunction::Mean:return mean;
    case SimplexFunction::Entropy:return entropy;
    case SimplexFunction::NegativeEntropy:return -entropy;
    case SimplexFunction::Variance:{double v=0;for(unsigned i=0;i<3;++i)v+=p[i]*(outcomes[i]-mean)*(outcomes[i]-mean);return v;}
    case SimplexFunction::Count:break;
  }
  throw std::invalid_argument("unknown simplex function");
}
double simplexKl(const SimplexPoint& p,const SimplexPoint& q) {
  requirePoint(p);requirePoint(q);double sum=0;
  for(unsigned i=0;i<3;++i){
    const double a=p[i],b=q[i];
    if(a==0){sum+=b;continue;}
    if(b==0)return std::numeric_limits<double>::infinity();
    // Sum generalized relative-entropy terms. The linear terms cancel for
    // distributions. A local series avoids catastrophic cancellation at P=Q.
    const double r=(a-b)/b;
    if(std::fabs(r)<1e-4){const double r2=r*r;sum+=b*r2*(.5+r*(-1./6+r*(1./12+r*(-1./20+r/30))));}
    else sum+=std::max(0.,a*(std::log(a)-std::log(b))-a+b);
  }
  return sum;
}
double simplexEntropyTangent(const SimplexPoint& p,const SimplexPoint& q) {
  requirePoint(p);requirePoint(q);double value=0;
  for(unsigned i=0;i<3;++i){if(q[i]==0)throw std::invalid_argument("entropy tangent requires positive reference");value+=p[i]*std::log(q[i]);}
  return value;
}
std::string_view simplexFunctionName(SimplexFunction f) {
  switch(f){case SimplexFunction::Mean:return "Expected value";case SimplexFunction::Entropy:return "Entropy H (nats)";case SimplexFunction::NegativeEntropy:return "Negative entropy (nats)";case SimplexFunction::Variance:return "Variance";case SimplexFunction::Count:break;}
  throw std::invalid_argument("unknown simplex function");
}
std::string_view simplexCurvature(SimplexFunction f) {
  switch(f){case SimplexFunction::Mean:return "Affine: chord and surface agree";case SimplexFunction::Entropy:case SimplexFunction::Variance:return "Concave: surface is above the chord";case SimplexFunction::NegativeEntropy:return "Convex: surface is below the chord";case SimplexFunction::Count:break;}
  throw std::invalid_argument("unknown simplex function");
}
}

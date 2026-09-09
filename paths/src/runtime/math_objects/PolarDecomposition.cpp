#include "runtime/math_objects/PolarDecomposition.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
constexpr PolarMatrix identity{1,0,0,0,1,0,0,0,1};
double determinant(const PolarMatrix& a){return a[0]*(a[4]*a[8]-a[5]*a[7])-a[1]*(a[3]*a[8]-a[5]*a[6])+a[2]*(a[3]*a[7]-a[4]*a[6]);}
double norm(const PolarMatrix& a){double out=0;for(double x:a)out=std::hypot(out,x);return out;}
PolarMatrix product(const PolarMatrix& a,const PolarMatrix& b){PolarMatrix out{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)out[3*i+j]+=a[3*i+k]*b[3*k+j];return out;}
PolarMatrix compose(const PolarMatrix& u,const std::array<double,3>& d,const PolarMatrix& v){PolarMatrix out{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)out[3*i+j]+=u[3*i+k]*d[k]*v[3*j+k];return out;}
double orthogonality(const PolarMatrix& a){PolarMatrix error{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){for(unsigned k=0;k<3;++k)error[3*i+j]+=a[3*k+i]*a[3*k+j];error[3*i+j]-=i==j;}return norm(error);}
}
PolarAnalysis analyzePolar(const PolarMatrix& a){
  double scale=0;for(double x:a){if(!std::isfinite(x)||std::fabs(x)>4)throw std::invalid_argument("polar entries must be finite in [-4,4]");scale=std::max(scale,std::fabs(x));}
  PolarAnalysis out;out.detA=determinant(a);PolarMatrix columns{},v=identity,u{};
  if(scale>0)for(unsigned i=0;i<9;++i)columns[i]=a[i]/scale;
  // Orthogonalize columns directly: this avoids squaring the condition number
  // by forming A^T A. The accumulated right rotations give V.
  for(unsigned sweep=0;sweep<32;++sweep){bool changed=false;
    for(unsigned p=0;p<2;++p)for(unsigned q=p+1;q<3;++q){
      double aa=0,bb=0,ab=0;for(unsigned k=0;k<3;++k){const double x=columns[3*k+p],y=columns[3*k+q];aa+=x*x;bb+=y*y;ab+=x*y;}
      if(std::fabs(ab)<=1e-15*std::sqrt(aa)*std::sqrt(bb)||ab==0)continue;
      const double tau=(bb-aa)/(2*ab),t=std::isfinite(tau)?std::copysign(1.,tau)/(std::fabs(tau)+std::hypot(1.,tau)):0;
      if(t==0)continue;
      const double c=1/std::hypot(1.,t),s=c*t;changed=true;
      for(unsigned k=0;k<3;++k){const double x=columns[3*k+p],y=columns[3*k+q];columns[3*k+p]=c*x-s*y;columns[3*k+q]=s*x+c*y;const double vx=v[3*k+p],vy=v[3*k+q];v[3*k+p]=c*vx-s*vy;v[3*k+q]=s*vx+c*vy;}
    }if(!changed)break;
  }
  std::array<double,3> lengths{};for(unsigned c=0;c<3;++c)for(unsigned r=0;r<3;++r)lengths[c]=std::hypot(lengths[c],columns[3*r+c]);
  for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j)if(lengths[j]>lengths[i]){std::swap(lengths[i],lengths[j]);for(unsigned k=0;k<3;++k){std::swap(columns[3*k+i],columns[3*k+j]);std::swap(v[3*k+i],v[3*k+j]);}}
  out.rankTolerance=1e-12*lengths[0]*scale;
  for(unsigned c=0;c<3;++c){
    if(lengths[c]>1e-12*lengths[0]){++out.rank;out.singular[c]=lengths[c]*scale;for(unsigned r=0;r<3;++r)u[3*r+c]=columns[3*r+c]/lengths[c];}
    else {double best=-1;std::array<double,3> column{};
      for(unsigned axis=0;axis<3;++axis){std::array<double,3> candidate{};candidate[axis]=1;
        for(unsigned pass=0;pass<2;++pass)for(unsigned j=0;j<c;++j){double dot=0;for(unsigned r=0;r<3;++r)dot+=candidate[r]*u[3*r+j];for(unsigned r=0;r<3;++r)candidate[r]-=dot*u[3*r+j];}
        const double n=std::hypot(candidate[0],candidate[1],candidate[2]);if(n>best){best=n;column=candidate;}}
      for(unsigned r=0;r<3;++r)u[3*r+c]=column[r]/best;
    }
  }
  out.w=compose(u,{1,1,1},v);out.p=compose(v,out.singular,v);
  out.alternateW=compose(u,{1,1,out.rank<3?-1.:1.},v);
  out.detW=determinant(out.w);out.orthogonalityError=orthogonality(out.w);
  const auto reconstructed=product(out.w,out.p);PolarMatrix error{};
  if(scale>0){PolarMatrix normalized{};for(unsigned i=0;i<9;++i){error[i]=(reconstructed[i]-a[i])/scale;normalized[i]=a[i]/scale;}out.reconstructionError=norm(error)/norm(normalized);}
  out.iterationAvailable=out.rank==3&&out.singular[2]>=1e-8;
  if(out.iterationAvailable){auto d=out.singular;
    for(unsigned step=0;step<=polarMaxSteps;++step){out.iterationSingular[step]=d;out.iterates[step]=step==0?a:compose(u,d,v);out.iterationError[step]=orthogonality(out.iterates[step]);for(double& x:d)x=.5*(x+1/x);}
  }
  return out;
}
}

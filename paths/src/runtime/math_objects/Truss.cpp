#include "Truss.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace paths {
namespace {
constexpr unsigned rows=TrussAnalysis::equations,stride=TrussAnalysis::maxUnknowns;
void bound(double x,double lo,double hi){if(!std::isfinite(x)||x<lo||x>hi)throw std::invalid_argument("truss input outside finite domain");}
double distance(TrussVector a,TrussVector b){return std::hypot(a[0]-b[0],a[1]-b[1]);}
}
std::string_view trussStatusText(TrussStatus status){
  switch(status){case TrussStatus::Determinate:return "Determinate: unique equilibrium";case TrussStatus::Indeterminate:return "Indeterminate: stiffness information needed";case TrussStatus::Mechanism:return "Mechanism: motion remains unconstrained";case TrussStatus::Degenerate:return "Degenerate: coincident joints";}return "Unknown truss state";
}
TrussAnalysis prepareTruss(const TrussInput& input){
  if(input.shape>=TrussShape::Count||input.supports>=TrussSupportMode::Count)throw std::invalid_argument("unknown truss configuration");
  bound(input.span,1,5);bound(input.height,0,3);bound(input.lean,-.15,.15);bound(input.position,0,1);for(double f:input.load)bound(f,-5,5);bound(input.tensionLimit,.25,10);bound(input.compressionLimit,.25,10);
  for(const auto& offset:input.offsets)for(double x:offset)bound(x,-.5,.5);
  TrussAnalysis out;out.input=input;const double w=input.span,h=input.height,l=input.lean;
  using Edges=std::array<std::array<unsigned,2>,9>;Edges edges{};
  switch(input.shape){
    case TrussShape::Triangle:case TrussShape::Roof:
      out.points={{{0,0},{w,0},{(.5+l)*w,h},{w/2,0},{(.25+l/2)*w,h/2},{(.75+l/2)*w,h/2}}};
      edges={{{0,3},{3,1},{1,5},{5,2},{2,4},{4,0},{3,4},{4,5},{5,3}}};
      out.loadPath={0,4,2,5,1};out.pathCount=5;out.testMember=7;
      if(input.shape==TrussShape::Roof)edges[7]={3,2};
      break;
    case TrussShape::Bridge:
      out.points={{{0,0},{w,0},{w/3,0},{2*w/3,0},{(1./6+l)*w,h},{(2./3+l)*w,h}}};
      edges={{{0,2},{2,3},{3,1},{0,4},{4,2},{2,5},{4,5},{5,3},{5,1}}};
      out.loadPath={0,2,3,1,0};out.pathCount=4;out.testMember=6;break;
    case TrussShape::Crane:
      out.points={{{0,0},{0,h},{.4*w,0},{(.4+l)*w,.85*h},{.75*w,0},{w,.45*h}}};
      edges={{{0,1},{0,2},{1,2},{1,3},{2,3},{2,4},{3,4},{3,5},{4,5}}};
      out.loadPath={1,3,5,0,0};out.pathCount=3;out.testMember=4;break;
    case TrussShape::Count:throw std::invalid_argument("unknown truss shape");
  }
  for(unsigned i=0;i<out.nodes;++i)for(unsigned axis=0;axis<2;++axis)out.points[i][axis]+=input.offsets[i][axis];
  out.restraints[0]={0,0};out.restraints[1]={0,1};out.restraints[2]={1,input.shape==TrussShape::Crane?0U:1U};out.supportCount=3;
  switch(input.supports){case TrussSupportMode::Designed:break;case TrussSupportMode::Released:out.supportCount=2;break;case TrussSupportMode::Extra:out.restraints[out.supportCount++]=input.shape==TrussShape::Crane?TrussSupport{5,1}:TrussSupport{1,0};break;case TrussSupportMode::Count:break;}
  bool coincident=false;for(unsigned i=0;i<out.nodes;++i)for(unsigned j=i+1;j<out.nodes;++j)coincident=coincident||distance(out.points[i],out.points[j])<=1e-9*w;
  for(unsigned i=0;i<edges.size();++i){auto& bar=out.bars[i];bar.a=edges[i][0];bar.b=edges[i][1];bar.active=input.braced||i!=out.testMember;bar.length=distance(out.points[bar.a],out.points[bar.b]);if(bar.length>1e-9*w)for(unsigned axis=0;axis<2;++axis)bar.direction[axis]=(out.points[bar.b][axis]-out.points[bar.a][axis])/bar.length;
    if(bar.active){const unsigned col=out.activeMembers++;out.columnMember[col]=i;for(unsigned axis=0;axis<2;++axis){out.equilibrium[(2*bar.a+axis)*stride+col]=bar.direction[axis];out.equilibrium[(2*bar.b+axis)*stride+col]=-bar.direction[axis];}}
  }
  for(unsigned i=0;i<out.supportCount;++i){const auto support=out.restraints[i];out.equilibrium[(2*support.node+support.axis)*stride+out.activeMembers+i]=1;}
  out.unknowns=out.activeMembers+out.supportCount;
  if(coincident)return out;
  auto matrix=out.equilibrium;std::iota(out.columnOrder.begin(),out.columnOrder.end(),0);for(unsigned i=0;i<rows;++i)out.rowTransform[rows*i+i]=1;
  // Full pivoting keeps all remaining coefficients under consideration; member
  // ordering and the sign of a bar's endpoint convention cannot decide rank.
  for(unsigned k=0;k<std::min(rows,out.unknowns);++k){double maximum=0;unsigned pivotRow=k,pivotColumn=k;
    for(unsigned i=k;i<rows;++i)for(unsigned j=k;j<out.unknowns;++j)if(std::fabs(matrix[i*stride+j])>maximum){maximum=std::fabs(matrix[i*stride+j]);pivotRow=i;pivotColumn=j;}
    if(maximum<=1e-10)break;
    for(unsigned j=0;j<out.unknowns;++j)std::swap(matrix[k*stride+j],matrix[pivotRow*stride+j]);for(unsigned j=0;j<rows;++j)std::swap(out.rowTransform[k*rows+j],out.rowTransform[pivotRow*rows+j]);
    for(unsigned i=0;i<rows;++i)std::swap(matrix[i*stride+k],matrix[i*stride+pivotColumn]);std::swap(out.columnOrder[k],out.columnOrder[pivotColumn]);
    const double pivot=matrix[k*stride+k];for(unsigned j=0;j<out.unknowns;++j)matrix[k*stride+j]/=pivot;for(unsigned j=0;j<rows;++j)out.rowTransform[k*rows+j]/=pivot;
    for(unsigned i=0;i<rows;++i)if(i!=k){const double scale=matrix[i*stride+k];for(unsigned j=0;j<out.unknowns;++j)matrix[i*stride+j]-=scale*matrix[k*stride+j];for(unsigned j=0;j<rows;++j)out.rowTransform[i*rows+j]-=scale*out.rowTransform[k*rows+j];}
    ++out.rank;
  }
  out.status=out.rank<rows?TrussStatus::Mechanism:out.rank<out.unknowns?TrussStatus::Indeterminate:TrussStatus::Determinate;
  if(out.status==TrussStatus::Determinate){double matrixNorm=0,inverseNorm=0;for(unsigned i=0;i<rows;++i){double a=0,b=0;for(unsigned j=0;j<rows;++j){a+=std::fabs(out.equilibrium[i*stride+j]);b+=std::fabs(out.rowTransform[i*rows+j]);}matrixNorm=std::max(matrixNorm,a);inverseNorm=std::max(inverseNorm,b);}out.reciprocalCondition=1/(matrixNorm*inverseNorm);}
  return out;
}
TrussSweep inspectTrussSweep(const TrussAnalysis& a){
  TrussSweep sweep;if(a.status!=TrussStatus::Determinate)return sweep;
  std::array<double,5> positions{};for(unsigned i=1;i<a.pathCount;++i)positions[i]=positions[i-1]+distance(a.points[a.loadPath[i-1]],a.points[a.loadPath[i]]);
  const double total=positions[a.pathCount-1];sweep.available=true;
  for(unsigned i=0;i<a.pathCount;++i){const double t=total>0?positions[i]/total:0;const auto s=sampleTruss(a,t);if(!s.forcesAvailable)return {};
    for(unsigned j=0;j<a.members;++j)if(s.utilization[j]>sweep.maxUtilization){sweep.maxUtilization=s.utilization[j];sweep.position=t;sweep.member=j;}
  }return sweep;
}
TrussSolution sampleTruss(const TrussAnalysis& a,double position){
  bound(position,0,1);TrussSolution out;out.position=position;
  std::array<double,4> lengths{};double total=0;for(unsigned i=0;i+1<a.pathCount;++i){lengths[i]=distance(a.points[a.loadPath[i]],a.points[a.loadPath[i+1]]);total+=lengths[i];}
  double remaining=position*total;unsigned segment=0;while(segment+2<a.pathCount&&remaining>lengths[segment])remaining-=lengths[segment++];
  const double t=lengths[segment]>0?std::clamp(remaining/lengths[segment],0.,1.):0;
  const auto first=a.loadPath[segment],second=a.loadPath[segment+1];for(unsigned axis=0;axis<2;++axis){out.loadPoint[axis]=(1-t)*a.points[first][axis]+t*a.points[second][axis];out.applied[first][axis]+=(1-t)*a.input.load[axis];out.applied[second][axis]+=t*a.input.load[axis];}
  if(a.status==TrussStatus::Degenerate)return out;
  std::array<double,TrussAnalysis::maxUnknowns> values{};
  for(unsigned i=0;i<a.rank;++i)for(unsigned j=0;j<rows;++j)values[a.columnOrder[i]]-=a.rowTransform[i*rows+j]*out.applied[j/2][j%2];
  for(unsigned i=0;i<rows;++i){double residual=out.applied[i/2][i%2];for(unsigned j=0;j<a.unknowns;++j)residual+=a.equilibrium[i*stride+j]*values[j];out.compatibilityResidual=std::max(out.compatibilityResidual,std::fabs(residual));}
  out.loadCompatible=out.compatibilityResidual<=1e-8*std::max(1.,std::hypot(a.input.load[0],a.input.load[1]));
  out.forcesAvailable=a.status==TrussStatus::Determinate&&out.loadCompatible;
  if(!out.forcesAvailable)return out;
  for(unsigned j=0;j<a.activeMembers;++j){const unsigned i=a.columnMember[j];out.forces[i]=values[j];out.utilization[i]=std::fabs(values[j])/(values[j]>=0?a.input.tensionLimit:a.input.compressionLimit);out.maxUtilization=std::max(out.maxUtilization,out.utilization[i]);}
  for(unsigned j=0;j<a.supportCount;++j)out.reactions[j]=values[a.activeMembers+j];
  out.jointResidual=out.applied;
  for(unsigned i=0;i<a.members;++i)if(a.bars[i].active){const auto& bar=a.bars[i];for(unsigned axis=0;axis<2;++axis){out.jointResidual[bar.a][axis]+=out.forces[i]*bar.direction[axis];out.jointResidual[bar.b][axis]-=out.forces[i]*bar.direction[axis];}}
  for(unsigned i=0;i<a.supportCount;++i)out.jointResidual[a.restraints[i].node][a.restraints[i].axis]+=out.reactions[i];
  // Global force/moment use only external loads and supports, independently of
  // the assembled member columns used for joint equilibrium.
  for(unsigned i=0;i<a.nodes;++i){for(unsigned axis=0;axis<2;++axis)out.resultant[axis]+=out.applied[i][axis];out.moment+=a.points[i][0]*out.applied[i][1]-a.points[i][1]*out.applied[i][0];out.residual=std::max(out.residual,std::hypot(out.jointResidual[i][0],out.jointResidual[i][1]));}
  for(unsigned i=0;i<a.supportCount;++i){const auto s=a.restraints[i];out.resultant[s.axis]+=out.reactions[i];out.moment+=(s.axis==1?a.points[s.node][0]:-a.points[s.node][1])*out.reactions[i];}
  return out;
}
}

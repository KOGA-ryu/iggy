#include "RowPlaneFigure.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace paths {
namespace {
using V=PlaneVector;
constexpr double tolerance=1e-10;
constexpr std::array<RowPlaneParameterSpec,10> parameters{{
  {RowPlaneParameter::Extent,"Plane extent",1,3,1.6,false},
  {RowPlaneParameter::Density,"Grid density",2,4,3,true},
  {RowPlaneParameter::FocusRow,"Highlight row",0,3,0,true},
  {RowPlaneParameter::Original,"Original planes",0,1,0,true},
  {RowPlaneParameter::Normals,"Row normals",0,1,0,true},
  {RowPlaneParameter::Solution,"Solution set",0,1,1,true},
  {RowPlaneParameter::Labels,"Labels",0,1,1,true},
  {RowPlaneParameter::ProbeS,"Solution coordinate s",-2,2,.8,false},
  {RowPlaneParameter::ProbeT,"Solution coordinate t",-2,2,.4,false},
  {RowPlaneParameter::ProbeU,"Solution coordinate u",-2,2,.2,false}
}};
double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
V scale(V a,double s){for(auto& x:a)x*=s;return a;}
V add(V a,V b){for(unsigned i=0;i<3;++i)a[i]+=b[i];return a;}
V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
double norm(V a){return std::hypot(a[0],a[1],a[2]);}
V unit(V a){return scale(a,1/norm(a));}
V canonical(V a){for(double x:a)if(std::abs(x)>tolerance)return x<0?scale(a,-1):a;return a;}
V perpendicular(V n){
  unsigned axis=0;for(unsigned i=1;i<3;++i)if(std::abs(n[i])<std::abs(n[axis]))axis=i;
  V e{};e[axis]=1;return unit(cross(n,e));
}
bool same(const BoardMatrix& a,const BoardMatrix& b){return a.rows==b.rows&&a.cols==b.cols&&a.values==b.values;}
bool supported(const BoardMatrix& a){
  return a.rows>=1&&a.rows<=3&&a.cols==3&&a.values.size()==a.rows*3&&
    std::all_of(a.values.begin(),a.values.end(),[](auto z){return std::isfinite(z.real())&&z.imag()==0;});
}
using Space=EquationSpace;
void removeComponents(V& v,const std::array<V,3>& basis,unsigned count){
  // Reorthogonalize these tiny 3D vectors; normalize rows before rank decisions.
  for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<count;++i)v=add(v,scale(basis[i],-dot(v,basis[i])));
}
Space solveSpace(const BoardMatrix& a,const BoardMatrix& rhs){
  Space out;
  const bool homogeneous=rhs.rows==0&&rhs.cols==0&&rhs.values.empty();
  if(!supported(a)||(!homogeneous&&(rhs.rows!=a.rows||rhs.cols!=1||rhs.values.size()!=a.rows||
      !std::all_of(rhs.values.begin(),rhs.values.end(),[](auto z){return std::isfinite(z.real())&&z.imag()==0;}))))return out;
  out.available=true;std::array<double,3> coordinates{};
  for(unsigned r=0;r<a.rows;++r){
    V row{};double largest=0;for(unsigned j=0;j<3;++j){row[j]=a.at(r,j).real();largest=std::max(largest,std::abs(row[j]));}
    const double right=homogeneous?0:rhs.at(r,0).real();
    if(largest==0){out.planes[r].contradiction=right!=0;if(right!=0)out.consistent=false;continue;}
    for(auto& x:row)x/=largest;
    const double length=norm(row),offset=(right/largest)/length;
    if(!std::isfinite(offset)){out.available=false;return out;}
    const V normal=unit(row),u=perpendicular(normal),v=cross(normal,u);
    out.planes[r]={normal,u,v,false,offset,false};V independent=normal;double remaining=offset,scaleRight=std::max(1.,std::abs(offset));
    for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<out.rank;++i){
      const double c=dot(independent,out.rows[i]);independent=add(independent,scale(out.rows[i],-c));
      remaining-=c*coordinates[i];scaleRight+=std::abs(c*coordinates[i]);
    }
    const double magnitude=norm(independent);
    if(magnitude>tolerance){coordinates[out.rank]=remaining/magnitude;out.rows[out.rank++]=unit(independent);}
    else if(std::abs(remaining)>tolerance*scaleRight)out.consistent=false;
  }
  for(unsigned axis=0;axis<3&&out.dimension<3-out.rank;++axis){
    V v{};v[axis]=1;removeComponents(v,out.rows,out.rank);removeComponents(v,out.null,out.dimension);
    if(norm(v)>tolerance)out.null[out.dimension++]=canonical(unit(v));
  }
  if(out.dimension!=3-out.rank)throw std::runtime_error("Could not construct the numerical null space");
  for(unsigned i=0;i<out.rank;++i)out.particular=add(out.particular,scale(out.rows[i],coordinates[i]));
  out.augmentedRank=out.rank+(out.consistent?0:1);
  for(double x:out.particular)if(!std::isfinite(x))out.available=false;
  return out;
}
double projectorDifference(const Space& a,const Space& b){
  double error=0;
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
    double x=0,y=0;for(unsigned k=0;k<a.dimension;++k)x+=a.null[k][i]*a.null[k][j];for(unsigned k=0;k<b.dimension;++k)y+=b.null[k][i]*b.null[k][j];
    error=std::max(error,std::abs(x-y));
  }
  return error;
}
iggy3d::Vec3 native(V v){return {static_cast<float>(v[0]),static_cast<float>(v[1]),static_cast<float>(v[2])};}
constexpr std::array<iggy3d::Vec3,3> rowColours{{{.30f,.79f,.94f},{.96f,.48f,.37f},{.76f,.58f,.96f}}};
constexpr iggy3d::Vec3 gold{1,.78f,.27f},muted{.30f,.36f,.44f};
struct Builder {
  MathObjectSnapshot& s;
  void part(MathShape shape,V center,V x,V y,V z,iggy3d::Vec3 colour,std::string_view role){
    if(s.partCount==s.parts.size())throw std::runtime_error("Lesson figure exceeds primitive capacity");
    auto& p=s.parts[s.partCount++];p={static_cast<std::uint32_t>(70000+s.partCount),shape,native(center),native(x),native(y),native(z),colour,role};
  }
  void rod(V a,V b,iggy3d::Vec3 colour,double radius,std::string_view role){
    const V d=add(b,scale(a,-1)),n=unit(d),u=perpendicular(n),v=cross(u,n);
    part(MathShape::Rod,scale(add(a,b),.5),scale(u,radius),d,scale(v,radius),colour,role);
  }
  void sphere(V at,double size,iggy3d::Vec3 colour,std::string_view role){part(MathShape::Sphere,at,{size,0,0},{0,size,0},{0,0,size},colour,role);}
  void label(std::string_view text,V at,iggy3d::Vec3 colour){if(s.labelCount==s.labels.size())throw std::runtime_error("Lesson figure label capacity");s.labels[s.labelCount++]={text,native(at),colour};}
  void grid(V u,V v,double extent,unsigned density,iggy3d::Vec3 colour,double radius,std::string_view role,V center={}){
    for(int k=-static_cast<int>(density);k<=static_cast<int>(density);++k){
      const double t=extent*k/density;
      rod(add(center,add(scale(u,-extent),scale(v,t))),add(center,add(scale(u,extent),scale(v,t))),colour,radius,role);
      rod(add(center,add(scale(v,-extent),scale(u,t))),add(center,add(scale(v,extent),scale(u,t))),colour,radius,role);
    }
  }
};
}
EquationSpace equationSpace(const BoardMatrix& a,const BoardMatrix& b){return solveSpace(a,b);}
std::span<const RowPlaneParameterSpec> rowPlaneParameters(){return parameters;}
iggy3d::Vec3 rowPlaneColour(unsigned row){return rowColours.at(row);}
RowPlaneFigure::RowPlaneFigure(){for(const auto& p:parameters)parameters_[static_cast<unsigned>(p.id)]=p.initial;}
double RowPlaneFigure::parameter(RowPlaneParameter p)const{return parameters_.at(static_cast<unsigned>(p));}
BoardResult RowPlaneFigure::dispatch(RowPlaneAction a){
  switch(a.kind){
    case RowPlaneActionKind::Set:{
      const auto i=static_cast<unsigned>(a.parameter);
      if(i>=parameters.size())return {false,"Unknown figure control."};
      const auto& p=parameters[i];
      if(!std::isfinite(a.value)||a.value<p.minimum||a.value>p.maximum||(p.integer&&std::floor(a.value)!=a.value))return {false,"Figure setting is outside its range."};
      if(parameters_[i]==a.value)return {true,{}};
      parameters_[i]=a.value;break;
    }
    case RowPlaneActionKind::Reset:for(const auto& p:parameters)parameters_[static_cast<unsigned>(p.id)]=p.initial;break;
    default:return {false,"Unknown figure action."};
  }
  dirty_=true;return {true,{}};
}
const RowPlaneView& RowPlaneFigure::publish(const MatrixBoardView& board){
  const auto& current=board.working?board.current:board.given;
  const auto& rhs=board.working?board.currentRhs:board.rhs;
  if(dirty_||!same(givenRhs_,board.rhs)||!same(currentRhs_,rhs)||!same(given_,board.given)||!same(current_,current)||example_!=board.example||steps_!=board.steps){
    given_=board.given;current_=current;givenRhs_=board.rhs;currentRhs_=rhs;example_=board.example;steps_=board.steps;rebuild(board);dirty_=false;
  }
  return view_;
}
void RowPlaneFigure::rebuild(const MatrixBoardView& board){
  view_={};view_.working=board.working;view_.example=board.example;view_.steps=board.steps;
  ++geometry_.revision;geometry_.kind=MathObjectKind::Linear;geometry_.level=0;geometry_.partCount=geometry_.labelCount=0;
  if(!supported(given_)||!supported(current_)||given_.rows!=current_.rows){
    view_.reason="Equation planes here require a real matrix with three columns and at most three rows. Parts (a) and (d) fit this view; every printed part remains available in the matrix exercise.";return;
  }
  const auto original=equationSpace(given_,givenRhs_),current=equationSpace(current_,currentRhs_);
  if(!original.available||!current.available){view_.reason="The explicit right-hand side has an unsupported shape or numerical range.";return;}
  view_.available=true;view_.rows=current_.rows;view_.rank=current.rank;view_.dimension=current.dimension;view_.consistent=current.consistent;view_.augmentedRank=current.augmentedRank;
  view_.planes=current.planes;view_.originalPlanes=original.planes;
  view_.agreement=projectorDifference(original,current);const double distance=norm(add(original.particular,scale(current.particular,-1)))/std::max({1.,norm(original.particular),norm(current.particular)});
  if(original.consistent&&current.consistent)view_.agreement=std::max(view_.agreement,distance);
  view_.agrees=original.consistent==current.consistent&&original.rank==current.rank&&(!current.consistent||view_.agreement<=1e-8);
  // A stable basis avoids a probe jumping just because rows were swapped.
  view_.basis=view_.agrees?original.null:current.null;
  view_.particular=view_.agrees?original.particular:current.particular;view_.probe=view_.particular;
  for(unsigned j=0;j<view_.dimension;++j)view_.probe=add(view_.probe,scale(view_.basis[j],parameter(static_cast<RowPlaneParameter>(static_cast<unsigned>(RowPlaneParameter::ProbeS)+j))));
  for(unsigned r=0;r<view_.rows;++r){
    for(unsigned j=0;j<3;++j)view_.coefficients[r*3+j]=current_.at(r,j).real();
    view_.rhs[r]=currentRhs_.rows?currentRhs_.at(r,0).real():0;
    if(!view_.planes[r].zero)view_.probeResidual=std::max(view_.probeResidual,std::abs(dot(view_.planes[r].normal,view_.probe)-view_.planes[r].offset)/std::max({1.,norm(view_.probe),std::abs(view_.planes[r].offset)}));
  }
  if(view_.consistent)view_.displayScale=std::max(1.,norm(view_.particular)/1.5);
  for(const auto& p:current.planes)view_.displayScale=std::max(view_.displayScale,std::abs(p.offset)/1.5);
  for(const auto& p:original.planes)view_.displayScale=std::max(view_.displayScale,std::abs(p.offset)/1.5);
  const auto point=[&](V v){return scale(v,1/view_.displayScale);};
  const auto center=point(view_.particular);
  Builder b{geometry_};const double extent=parameter(RowPlaneParameter::Extent);const auto density=static_cast<unsigned>(parameter(RowPlaneParameter::Density));
  const bool labels=parameter(RowPlaneParameter::Labels)!=0;const auto focus=static_cast<unsigned>(parameter(RowPlaneParameter::FocusRow));
  for(unsigned j=0;j<3;++j){V axis{};axis[j]=extent*1.3;b.rod(scale(axis,-1),axis,muted,.008,"coordinate_axis");if(labels)b.label(std::array<std::string_view,3>{"x","y","z"}[j],scale(axis,1.07),{.8f,.85f,.9f});}
  if(parameter(RowPlaneParameter::Original))for(const auto& p:original.planes)if(!p.zero)b.grid(p.u,p.v,extent,2,{.25f,.29f,.34f},.004,"original_plane",point(scale(p.normal,p.offset)));
  for(unsigned r=0;r<view_.rows;++r){const auto& p=current.planes[r];if(p.zero)continue;
    const auto colour=rowColours[r]*(focus==0||focus==r+1||focus>view_.rows?1.f:.32f);
    b.grid(p.u,p.v,extent,density,colour,.008,"equation_plane",point(scale(p.normal,p.offset)));
    if(parameter(RowPlaneParameter::Normals))b.rod(point(scale(p.normal,p.offset)),add(point(scale(p.normal,p.offset)),scale(p.normal,extent)),colour,.022,"row_normal");
    if(labels)b.label(std::array<std::string_view,3>{"Row 1 plane","Row 2 plane","Row 3 plane"}[r],add(point(scale(p.normal,p.offset)),add(scale(p.u,extent),scale(p.v,extent*.7))),colour);
  }
  if(parameter(RowPlaneParameter::Solution)&&view_.consistent){
    switch(view_.dimension){
      case 0:b.sphere(center,.075,gold,"solution_point");break;
      case 1:b.rod(add(center,scale(view_.basis[0],-extent*1.25)),add(center,scale(view_.basis[0],extent*1.25)),gold,.027,"solution_line");break;
      case 2:b.grid(view_.basis[0],view_.basis[1],extent,2,gold,.016,"solution_plane",center);break;
      case 3:
        for(unsigned axis=0;axis<3;++axis)for(int s:{-1,1})for(int t:{-1,1}){V a{},c{};a[axis]=-extent;c[axis]=extent;a[(axis+1)%3]=c[(axis+1)%3]=s*extent;a[(axis+2)%3]=c[(axis+2)%3]=t*extent;b.rod(a,c,gold,.016,"solution_space");}break;
    }
    b.sphere(point(view_.probe),.06,{1,.94f,.72f},"solution_probe");
    if(labels)b.label(view_.dimension==0?"Unique solution":"Solution probe",add(point(view_.probe),{.09,.12,.08}),gold);
  }
  b.sphere({},.025,{.9f,.9f,.9f},"origin");
}
} // namespace paths

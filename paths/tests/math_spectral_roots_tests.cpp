#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
using namespace paths;
constexpr double pi=3.14159265358979323846;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double eps,const char* why){if(!std::isfinite(a)||std::fabs(a-b)>eps)throw std::runtime_error(std::string(why)+": "+std::to_string(a)+" vs "+std::to_string(b));}
void action(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void level(MathObjects& m,unsigned n){action(m,{MathActionKind::SetLevel,{},{},static_cast<double>(n)});}
void select(MathObjects& m,MathObjectKind k,unsigned n=0){action(m,{MathActionKind::Select,k});level(m,n);action(m,{MathActionKind::Reset});}
void set(MathObjects& m,MathParameter p,double x){action(m,{MathActionKind::SetParameter,{},p,x});}
double metric(const MathObjects& m,std::string_view name){const auto& s=m.snapshot();for(std::size_t i=0;i<s.metricCount;++i)if(s.metrics[i].label==name)return s.metrics[i].value;throw std::runtime_error("missing metric: "+std::string(name));}
void checked(MathObjects& m,bool solved){action(m,{MathActionKind::Check});require((m.snapshot().feedback==MathFeedback::Solved)==solved,"wrong challenge verdict");}
void reject(MathObjects& m,MathAction a){const auto revision=m.snapshot().revision;const auto mask=m.snapshot().modularVisitedMask;require(!m.dispatch(a).accepted,"invalid action accepted");require(m.snapshot().revision==revision&&m.snapshot().modularVisitedMask==mask,"rejection changed state");}
std::size_t states=0,maxVertices=0,maxIndices=0;
void inspect(const MathObjects& m,MathObjectScene& scene) {
  const auto& s=m.snapshot();
  for(std::size_t i=0;i<s.partCount;++i){const auto& part=s.parts[i];const double det=iggy3d::dot(part.x,iggy3d::cross(part.y,part.z));if(!std::isfinite(det)||std::fabs(det)<1e-15)throw std::runtime_error("Singular primitive: object="+std::to_string(static_cast<unsigned>(s.kind))+" level="+std::to_string(s.level)+" role="+std::string(part.role));}
  const auto& frame=scene.publish(s,{0,0,950,650});++states;
  require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"mesh capacity");
  maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());
  require(s.partCount<=s.parts.size()&&s.labelCount<=s.labels.size()&&iggy3d::isFinite(frame.clipFromWorld),"invalid snapshot or camera");
  std::size_t end=0;std::set<std::uint32_t> ids;
  for(const auto& draw:frame.draws){require(ids.insert(draw.objectId.value).second&&draw.firstIndex==end&&draw.indexCount%3==0,"invalid draw");end+=draw.indexCount;}
  require(end==frame.indices.size(),"unowned geometry");for(auto i:frame.indices)require(i<frame.vertices.size(),"invalid index");
  for(const auto& vertex:frame.vertices){for(auto v:vertex.position)require(std::isfinite(v),"nonfinite position");for(auto v:vertex.color)require(std::isfinite(v)&&v>=0&&v<=1,"invalid colour");}
  for(std::size_t i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"nonfinite metric");
  for(std::size_t p=0;p<s.plotCount;++p){const auto& plot=s.plots[p];require(plot.seriesCount>0&&plot.seriesCount<=plot.series.size(),"invalid series count");for(std::size_t j=0;j<plot.seriesCount;++j){const auto& line=plot.series[j];require(line.count>0&&line.count<=line.points.size(),"invalid point count");for(std::size_t i=0;i<line.count;++i)require(std::isfinite(line.points[i].x)&&std::isfinite(line.points[i].y),"nonfinite plot");}}
  const auto& table=s.table;require(table.rowCount<=table.values.size()&&table.columnCount<=table.columns.size(),"table overflow");
  for(std::size_t i=0;i<table.rowCount;++i){require(!table.rowLabels[i].empty(),"missing row label");for(std::size_t j=0;j<table.columnCount;++j)require(!table.columns[j].empty()&&std::isfinite(table.values[i][j]),"invalid table cell");}
}
unsigned harmonicCases=0,formCases=0,rootCases=0;
// Independent explicit Legendre polynomials and symbolic differentiation.
double referenceHarmonic(unsigned index,double theta,double phi) {
  const unsigned l=static_cast<unsigned>(std::sqrt(index));const int signedM=static_cast<int>(index-l*l)-static_cast<int>(l);const unsigned m=static_cast<unsigned>(std::abs(signedM));
  constexpr std::array<std::array<double,5>,5> polynomials{{{1,0,0,0,0},{0,1,0,0,0},{-.5,0,1.5,0,0},{0,-1.5,0,2.5,0},{.375,0,-3.75,0,4.375}}};auto coefficients=polynomials[l];
  for(unsigned d=0;d<m;++d){for(unsigned j=0;j<4;++j)coefficients[j]=(j+1)*coefficients[j+1];coefficients[4]=0;}
  const double z=std::cos(theta);double polynomial=0;for(int j=4;j>=0;--j)polynomial=polynomial*z+coefficients[static_cast<unsigned>(j)];
  const double factor=std::sqrt((2*l+1)*std::tgamma(l-m+1)/(4*pi*std::tgamma(l+m+1)));
  return factor*polynomial*std::pow(std::sin(theta),m)*(m%2?-1:1)*(m?std::sqrt(2.0)*(signedM>0?std::cos(m*phi):std::sin(m*phi)):1);
}
void spherical() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Spherical,1);
  for(unsigned mode=0;mode<25;++mode){set(m,MathParameter::SphereMode,mode);for(double azimuth:{0.0,37.0,180.0,360.0}){set(m,MathParameter::SpherePhi,azimuth);const auto& line=m.snapshot().plots[0].series[0];for(std::size_t j=0;j<line.count;++j){near(line.points[j].y,referenceHarmonic(mode,line.points[j].x*pi/180,azimuth*pi/180),3e-14,"real harmonic polynomial reference");++harmonicCases;}near(metric(m,"Energy"),1,1e-15,"mode norm");near(metric(m,"Integrated energy"),1,3e-14,"independent quadrature norm");inspect(m,scene);}
    // The mesh uses the documented radius, with unit outward normals.
    const auto& patch=m.snapshot().surface;for(unsigned i=0;i<patch.rows;++i)for(unsigned j=0;j<patch.columns;++j){const auto& v=patch.vertices[i*patch.columns+j];const double y=referenceHarmonic(mode,pi*i/(patch.rows-1),2*pi*j/(patch.columns-1));near(iggy3d::length(v.position),.3+1.4*std::fabs(y),4e-7,"lobe radius");near(iggy3d::length(v.normal),1,2e-7,"surface unit normal");require(iggy3d::dot(v.position,v.normal)>-1e-7,"inward lobe normal");}
  }
  level(m,3);
  for(unsigned a=0;a<25;++a)for(unsigned b=0;b<25;++b){set(m,MathParameter::SphereMode,a);set(m,MathParameter::SphereSecond,b);set(m,MathParameter::SphereMix,a==b?-1:.6);for(double t:{0.0,.25,1.0}){set(m,MathParameter::SphereHeat,t);const unsigned l=static_cast<unsigned>(std::sqrt(a)),q=static_cast<unsigned>(std::sqrt(b));const double ca=std::exp(-static_cast<double>(l)*(l+1)*t),cb=(a==b?-1:.6)*std::exp(-static_cast<double>(q)*(q+1)*t),energy=ca*ca+cb*cb+(a==b?2*ca*cb:0);near(metric(m,"Cross inner product"),a==b?1:0,3e-14,"orthogonality for every mode pair");near(metric(m,"Energy"),energy,2e-14,"heat spectral energy");near(metric(m,"Integrated energy"),energy,5e-14,"heat quadrature energy");require(metric(m,"Laplacian residual")<5e-6,"finite difference Laplace eigenvalue");require(metric(m,"Energy lost")>=-1e-14,"heat energy increased");++harmonicCases;}
    if(a==b||b==24)inspect(m,scene);
  }
  set(m,MathParameter::SphereMode,0);set(m,MathParameter::SphereSecond,3);set(m,MathParameter::SphereMix,0);set(m,MathParameter::SphereHeat,1);near(metric(m,"Probe value"),1/std::sqrt(4*pi),2e-15,"constant survives heat");
  select(m,MathObjectKind::Spherical);checked(m,false);set(m,MathParameter::SphereTheta,90);set(m,MathParameter::SpherePhi,90);checked(m,true);level(m,1);set(m,MathParameter::SphereMode,14);checked(m,true);level(m,2);checked(m,true);set(m,MathParameter::SphereSecond,14);checked(m,false);level(m,3);set(m,MathParameter::SphereSecond,3);set(m,MathParameter::SphereHeat,.5);checked(m,true);set(m,MathParameter::SphereHeat,0);checked(m,false);
}
using V=std::array<double,3>;using M=std::array<double,9>;
double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
V times(M a,V b){return {a[0]*b[0]+a[1]*b[1]+a[2]*b[2],a[3]*b[0]+a[4]*b[1]+a[5]*b[2],a[6]*b[0]+a[7]*b[1]+a[8]*b[2]};}
void quadratic() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Quadratic,2);set(m,MathParameter::QuadX,1.2);set(m,MathParameter::QuadY,-.7);set(m,MathParameter::QuadZ,.35);const V probe{1.2,-.7,.35};
  for(double lx:{-2.0,0.0,2.0})for(double ly:{-2.0,0.0,2.0})for(double lz:{-2.0,0.0,2.0})for(double yaw:{-90.0,0.0,45.0})for(double pitch:{-45.0,0.0,60.0}) {
    ++formCases;set(m,MathParameter::QuadLambdaX,lx);set(m,MathParameter::QuadLambdaY,ly);set(m,MathParameter::QuadLambdaZ,lz);set(m,MathParameter::QuadYaw,yaw);set(m,MathParameter::QuadPitch,pitch);const auto& s=m.snapshot();const auto a=s.matrices[0].values,q=s.matrices[1].values;const V lambda{lx,ly,lz};
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){near(a[i*3+j],a[j*3+i],1e-14,"quadratic symmetry");double reconstructed=0,orthogonal=0;for(unsigned k=0;k<3;++k){reconstructed+=q[i*3+k]*lambda[k]*q[j*3+k];orthogonal+=q[k*3+i]*q[k*3+j];}near(a[i*3+j],reconstructed,1e-14,"spectral reconstruction");near(orthogonal,i==j?1:0,1e-14,"basis orthogonality");}
    near(metric(m,"Quadratic value"),dot(probe,times(a,probe)),1e-13,"direct bilinear form");near(metric(m,"Principal sum"),metric(m,"Quadratic value"),1e-13,"change of coordinates");near(metric(m,"Positive directions"),(lx>0)+(ly>0)+(lz>0),0,"positive inertia");near(metric(m,"Negative directions"),(lx<0)+(ly<0)+(lz<0),0,"negative inertia");near(metric(m,"Null directions"),(lx==0)+(ly==0)+(lz==0),0,"null inertia");
    const double det=a[0]*(a[4]*a[8]-a[5]*a[7])-a[1]*(a[3]*a[8]-a[5]*a[6])+a[2]*(a[3]*a[7]-a[4]*a[6]);near(det,metric(m,"Determinant"),2e-14,"determinant invariant");
    for(const auto& v:s.surface.vertices){const V location{v.position.x,v.position.y,probe[2]};near(v.position.z,.15*dot(location,times(a,location)),5e-7,"height map is actual slice");}
    require(s.contours.count<=s.contours.segments.size(),"contour overflow");for(std::size_t j=0;j<s.contours.count;++j)for(const auto point:{s.contours.segments[j].a,s.contours.segments[j].b}){require(std::isfinite(point.x)&&std::isfinite(point.y)&&std::fabs(point.x)<=2.000001&&std::fabs(point.y)<=2.000001,"contour bounds");const V v{point.x,point.y,probe[2]};require(std::fabs(dot(v,times(a,v)))<.021,"zero contour interpolation error");}
    inspect(m,scene);level(m,3);near(metric(m,"Rayleigh quotient"),dot(probe,times(a,probe))/dot(probe,probe),1e-13,"Rayleigh quotient");require(metric(m,"Rayleigh quotient")>=metric(m,"Smallest eigenvalue")-1e-12&&metric(m,"Rayleigh quotient")<=metric(m,"Largest eigenvalue")+1e-12,"Rayleigh bounds");for(const auto& v:m.snapshot().surface.vertices)near(iggy3d::length(v.position),1,1e-7,"Rayleigh sphere radius");level(m,2);
  }
  action(m,{MathActionKind::MoveSurfacePoint,{},{},1.234,0,-.678});near(m.parameter(MathParameter::QuadX),1.234,0,"contour horizontal input");near(m.parameter(MathParameter::QuadY),-.678,0,"contour vertical input");const auto x=m.parameter(MathParameter::QuadX),y=m.parameter(MathParameter::QuadY);reject(m,{MathActionKind::MoveSurfacePoint,{},{},.5,0,std::numeric_limits<double>::quiet_NaN()});near(m.parameter(MathParameter::QuadX),x,0,"atomic contour x");near(m.parameter(MathParameter::QuadY),y,0,"atomic contour y");reject(m,{MathActionKind::MoveSurfacePoint,{},{},2.1});
  set(m,MathParameter::QuadLambdaX,0);set(m,MathParameter::QuadLambdaY,0);set(m,MathParameter::QuadLambdaZ,0);require(metric(m,"Entire section zero")==1&&m.snapshot().contours.count==0,"zero form fabricated contour");
  select(m,MathObjectKind::Quadratic);checked(m,false);set(m,MathParameter::QuadLambdaX,-1.5);set(m,MathParameter::QuadY,0);checked(m,true);level(m,1);checked(m,true);level(m,2);set(m,MathParameter::QuadLambdaZ,0);checked(m,true);level(m,3);set(m,MathParameter::QuadLambdaX,1.5);set(m,MathParameter::QuadLambdaY,1);set(m,MathParameter::QuadLambdaZ,.5);set(m,MathParameter::QuadYaw,0);set(m,MathParameter::QuadPitch,0);set(m,MathParameter::QuadX,0);set(m,MathParameter::QuadY,0);set(m,MathParameter::QuadZ,1);checked(m,true);set(m,MathParameter::QuadZ,0);require(metric(m,"Rayleigh defined")==0,"zero probe quotient defined");checked(m,false);inspect(m,scene);reject(m,{MathActionKind::MoveSurfacePoint,{},{},.1,0,.2});
}
void roots() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Roots,3);
  constexpr std::array<std::array<int,13>,13> expected{{{0},{-1,1},{1,1},{1,1,1},{1,0,1},{1,1,1,1,1},{1,-1,1},{1,1,1,1,1,1,1},{1,0,0,0,1},{1,0,0,1,0,0,1},{1,-1,1,-1,1},{1,1,1,1,1,1,1,1,1,1,1},{1,0,-1,0,1}}};
  std::array<std::vector<long long>,13> polynomials;polynomials[1]={-1,1};polynomials[2]={1,1};
  for(unsigned n=3;n<=12;++n){set(m,MathParameter::RootN,n);unsigned totient=0;for(unsigned k=0;k<n;++k)totient+=std::gcd(k,n)==1;near(metric(m,"Primitive roots"),totient,0,"Euler totient");const auto& coeff=m.snapshot().plots[1].series[0];require(coeff.count==totient+1,"cyclotomic degree");for(std::size_t i=0;i<coeff.count;++i){near(coeff.points[i].y,expected[n][i],0,"exact cyclotomic coefficients");polynomials[n].push_back(static_cast<long long>(coeff.points[i].y));}
    for(unsigned a=1;a<=11;++a){set(m,MathParameter::RootAutomorphism,a);const bool unit=std::gcd(a,n)==1;std::set<unsigned> images;unsigned fixed=0;for(unsigned k=0;k<n;++k){const unsigned image=a*k%n;images.insert(image);fixed+=image==k;near(m.snapshot().table.values[k][1],image,0,"root exponent map");if(unit)require(std::gcd(k,n)==std::gcd(image,n),"automorphism changed element order");}near(metric(m,"Distinct images"),images.size(),0,"actual map image count");near(metric(m,"Fixed roots"),fixed,0,"fixed root count");near(metric(m,"Automorphism defined"),unit?1:0,0,"invalid Galois action accepted");if(unit){unsigned residue=1,order=0;do{residue=residue*a%n;++order;require(order<=n,"reference unit order bound");}while(residue!=1);near(metric(m,"Automorphism order"),order,0,"automorphism order");}else near(metric(m,"Automorphism order"),0,0,"nonunit has group order");require(metric(m,"Cyclotomic residual")<1e-12&&metric(m,"Root equation residual")<1e-12,"root residual");inspect(m,scene);++rootCases;}
    std::vector<long long> product{1};for(unsigned d=1;d<=n;++d)if(n%d==0){std::vector<long long> next(product.size()+polynomials[d].size()-1);for(std::size_t i=0;i<product.size();++i)for(std::size_t j=0;j<polynomials[d].size();++j)next[i+j]+=product[i]*polynomials[d][j];product=next;}require(product.size()==n+1&&product[0]==-1&&product[n]==1,"cyclotomic product degree");for(unsigned i=1;i<n;++i)require(product[i]==0,"product of divisor polynomials differs from x^n-1");
  }
  level(m,2);for(unsigned n=3;n<=12;++n)for(unsigned k=0;k<=12;++k){set(m,MathParameter::RootN,n);set(m,MathParameter::RootMultiplier,k);unsigned order=0,residue=0;std::set<unsigned> subgroup;do{subgroup.insert(residue);residue=(residue+k)%n;++order;}while(residue);near(metric(m,"Subgroup order"),order,0,"subgroup order from orbit");const auto table=m.snapshot().table;require(table.rowCount==order+1,"cycle closing point missing");for(unsigned j=0;j<=order;++j)near(table.values[j][1],k*j%n,0,"orbit table");for(unsigned power=0;power<=12;++power){set(m,MathParameter::RootPower,power);near(metric(m,"Power result exponent"),k*power%n,0,"cyclic power");++rootCases;}inspect(m,scene);}
  select(m,MathObjectKind::Roots);checked(m,false);set(m,MathParameter::RootN,8);set(m,MathParameter::RootIndex,3);checked(m,true);level(m,1);checked(m,true);level(m,2);set(m,MathParameter::RootPower,4);checked(m,true);level(m,3);set(m,MathParameter::RootAutomorphism,5);checked(m,true);set(m,MathParameter::RootAutomorphism,2);checked(m,false);
}
void boundaries() {
  MathObjects m;MathObjectScene scene;require(mathObjectSpecs().size()==static_cast<std::size_t>(MathObjectKind::Count),"wrong object count");std::set<std::string_view> keys;unsigned ordinal=0;for(const auto& p:mathParameterSpecs()){require(keys.insert(p.key).second,"duplicate CLI key");require(static_cast<unsigned>(p.id)==ordinal++,"parameter catalogue order");}
  for(const auto kind:{MathObjectKind::Spherical,MathObjectKind::Quadratic,MathObjectKind::Roots})for(unsigned l=0;l<4;++l){select(m,kind,l);require(mathLessons(kind).size()==4,"missing layer");inspect(m,scene);for(unsigned pass=0;pass<2;++pass)for(const auto& p:mathParameterSpecs())if(m.parameterAvailable(p.id)){set(m,p.id,pass?p.maximum:p.minimum);inspect(m,scene);const auto old=m.parameter(p.id);reject(m,{MathActionKind::SetParameter,{},p.id,std::numeric_limits<double>::quiet_NaN()});near(m.parameter(p.id),old,0,"invalid parameter changed value");reject(m,{MathActionKind::SetParameter,{},p.id,p.maximum+1});}reject(m,{MathActionKind::SetLevel,{},{},4});action(m,{MathActionKind::Reset});inspect(m,scene);}
  select(m,MathObjectKind::Algebra);reject(m,{MathActionKind::SetParameter,{},MathParameter::SphereMode,2});select(m,MathObjectKind::Roots);reject(m,{MathActionKind::SetParameter,{},MathParameter::RootAutomorphism,5});select(m,MathObjectKind::Spherical,3);set(m,MathParameter::SphereSecond,24);level(m,0);require(!m.parameterAvailable(MathParameter::SphereSecond),"hidden layer control available");level(m,2);near(m.parameter(MathParameter::SphereSecond),3,0,"lowering layer retained advanced state");
}
}
int main(){try{spherical();quadratic();roots();boundaries();std::printf("Spectral/forms/roots tests passed: %u harmonic checks, %u quadratic cases, %u root actions, exact cyclotomic products, atomic contours and %zu geometry states; maxima %zu vertices / %zu indices\n",harmonicCases,formCases,rootCases,states,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}

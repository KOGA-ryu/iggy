#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using namespace paths;
using V=std::array<double,3>;
constexpr double pi=3.14159265358979323846;
std::size_t meshes=0,certificates=0,maxVertices=0,maxIndices=0;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void near(double actual,double expected,double tolerance,const char* message) {
  if(!std::isfinite(actual)||std::fabs(actual-expected)>tolerance)throw std::runtime_error(std::string(message)+": "+std::to_string(actual)+" vs "+std::to_string(expected));
}
void action(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,MathParameter p,double v){action(m,{MathActionKind::SetParameter,{},p,v});}
void level(MathObjects& m,unsigned l){action(m,{MathActionKind::SetLevel,{},{},static_cast<double>(l)});}
void select(MathObjects& m,MathObjectKind k,unsigned l=0){action(m,{MathActionKind::Select,k});level(m,l);action(m,{MathActionKind::Reset});}
void preset(MathObjects& m,unsigned n){action(m,{MathActionKind::ObjectPreset,{},{},0,n});}
double metric(const MathObjects& m,std::string_view name){const auto& s=m.snapshot();for(std::size_t i=0;i<s.metricCount;++i)if(s.metrics[i].label==name)return s.metrics[i].value;throw std::runtime_error("missing metric "+std::string(name));}
const MathPart& part(const MathObjects& m,std::string_view role){const auto& s=m.snapshot();for(std::size_t i=0;i<s.partCount;++i)if(s.parts[i].role==role)return s.parts[i];throw std::runtime_error("missing part "+std::string(role));}
bool hasPart(const MathObjects& m,std::string_view role){const auto& s=m.snapshot();for(std::size_t i=0;i<s.partCount;++i)if(s.parts[i].role==role)return true;return false;}
void checked(MathObjects& m,bool expected){action(m,{MathActionKind::Check});require((m.snapshot().feedback==MathFeedback::Solved)==expected,"challenge verdict");}
void reject(MathObjects& m,MathAction a) {
  const auto revision=m.snapshot().revision;const auto feedback=m.snapshot().feedback;std::array<double,static_cast<std::size_t>(MathParameter::Count)> before{};
  for(const auto& p:mathParameterSpecs())before[static_cast<std::size_t>(p.id)]=m.parameter(p.id);
  require(!m.dispatch(a).accepted,"accepted invalid action");require(m.snapshot().revision==revision&&m.snapshot().feedback==feedback,"rejection mutated snapshot");
  for(const auto& p:mathParameterSpecs())near(m.parameter(p.id),before[static_cast<std::size_t>(p.id)],0,"rejection mutated parameter");
}
void inspect(const MathObjects& m,MathObjectScene& scene) {
  const auto& s=m.snapshot();const auto& f=scene.publish(s,{0,0,800,600});++meshes;
  maxVertices=std::max(maxVertices,f.vertices.size());maxIndices=std::max(maxIndices,f.indices.size());
  require(!f.vertices.empty()&&f.vertices.size()<=kSceneVertexCapacity&&f.indices.size()<=kSceneIndexCapacity,"scene capacity");
  require(iggy3d::isFinite(f.clipFromWorld),"camera finite");std::set<unsigned> ids;std::size_t end=0;
  for(const auto& draw:f.draws){require(ids.insert(draw.objectId.value).second&&draw.firstIndex==end&&draw.indexCount%3==0,"draw ownership");end+=draw.indexCount;}
  require(end==f.indices.size(),"unowned indices");for(auto i:f.indices)require(i<f.vertices.size(),"index range");
  for(const auto& v:f.vertices){for(double a:v.position)require(std::isfinite(a),"finite geometry");for(double c:v.color)require(std::isfinite(c)&&c>=0&&c<=1,"finite colour");}
  for(std::size_t i=0;i<s.partCount;++i){const auto& p=s.parts[i];require(std::fabs(iggy3d::dot(p.x,iggy3d::cross(p.y,p.z)))>=1e-15,"singular transform");}
  for(std::size_t i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"finite metric");
  for(std::size_t i=0;i<s.matrixCount;++i){const auto& a=s.matrices[i];require(a.rows==2&&a.columns==2,"new object matrix shape");for(unsigned j=0;j<a.rows*a.columns;++j)require(std::isfinite(a.values[j]),"finite matrix");}
  for(std::size_t i=0;i<s.plotCount;++i){const auto& p=s.plots[i];require(p.seriesCount>0&&p.seriesCount<=3,"plot count");for(std::size_t j=0;j<p.seriesCount;++j)for(std::size_t k=0;k<p.series[j].count;++k){const auto v=p.series[j].points[k];require(std::isfinite(v.x)&&std::isfinite(v.y),"finite plot");}}
  const auto& patch=s.surface;for(unsigned i=0;i<patch.rows*patch.columns;++i)near(iggy3d::length(patch.vertices[i].normal),1,3e-7,"surface unit normal");
}
V vector(iggy3d::Vec3 v){return {v.x,v.y,v.z};}
double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
// Deliberately direct powers: independent from the runtime's max-scaled kernel.
double norm(V x,double p,bool inf=false){if(inf)return std::max({std::fabs(x[0]),std::fabs(x[1]),std::fabs(x[2])});return std::pow(std::pow(std::fabs(x[0]),p)+std::pow(std::fabs(x[1]),p)+std::pow(std::fabs(x[2]),p),1/p);}
void psd() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Psd,1);
  for(double a:{-2.0,-1.0,0.0,1.0,2.0})for(double b:{-2.0,-1.0,0.0,1.0,2.0})for(double c:{-2.0,-1.0,0.0,1.0,2.0}) {
    set(m,MathParameter::PsdA,a);set(m,MathParameter::PsdB,b);set(m,MathParameter::PsdC,c);const auto& s=m.snapshot();const double det=a*c-b*b;
    const double lo=metric(m,"Smallest eigenvalue"),hi=metric(m,"Largest eigenvalue");
    near(metric(m,"PSD"),a>=0&&c>=0&&det>=0?1:0,0,"all principal minors criterion");near(lo+hi,a+c,1e-13,"trace certificate");near(lo*hi,det,1e-13,"determinant certificate");
    const auto q=s.matrices[1].values;for(unsigned col=0;col<2;++col){const double u=q[col],v=q[2+col],e=col==0?hi:lo;near(a*u+b*v,e*u,2e-14,"eigenvector first row");near(b*u+c*v,e*v,2e-14,"eigenvector second row");near(u*u+v*v,1,1e-14,"normalized eigenvector");}
    near(q[0]*q[1]+q[2]*q[3],0,1e-14,"eigenvector orthogonality");
    const auto pos=part(m,"psd_matrix_point").center;near(pos.x,(a-c)/2,1e-7,"cone x");near(pos.y,b,1e-7,"cone y");near(pos.z,(a+c)/2,1e-7,"cone t");
    const auto& patch=s.surface;for(unsigned i=0;i<patch.rows*patch.columns;++i){const auto p=patch.vertices[i].position;const double u=p.x-4.6,v=p.y;near(p.z,.3*(a*u*u+2*b*u*v+c*v*v),2e-6,"quadratic surface samples");}
    for(const auto& v:s.plots[0].series[0].points){const double u=std::cos(v.x*pi/180),w=std::sin(v.x*pi/180);near(v.y,a*u*u+2*b*u*w+c*w*w,1e-13,"quadratic plot");require(v.y>=lo-1e-12&&v.y<=hi+1e-12,"Rayleigh interval");}
    ++certificates;inspect(m,scene);
  }
  // Small matrices obey the same cone rule and retain the non-leading-minor trap.
  preset(m,3);set(m,MathParameter::PsdC,-.05);near(metric(m,"PSD"),0,0,"leading minor zero trap");near(metric(m,"Rank"),1,0,"small nonzero rank");
  level(m,2);preset(m,1);
  for(double angle:{0.0,30.0,45.0,90.0,135.0,180.0})for(double fraction:{0.0,.25,.5,.75,1.0})for(double scale:{0.0,.5,1.0,1.5}) {
    set(m,MathParameter::PsdOtherAngle,angle);set(m,MathParameter::PsdMix,fraction);set(m,MathParameter::PsdRayScale,scale);
    const auto& s=m.snapshot();const auto a=s.matrices[1].values,b=s.matrices[2].values,active=s.matrices[0].values;
    near(b[0]+b[3],2,1e-14,"rank-one B trace");near(b[0]*b[3]-b[1]*b[2],0,1e-14,"rank-one B determinant");
    for(unsigned i=0;i<4;++i)near(active[i],scale*((1-fraction)*a[i]+fraction*b[i]),1e-14,"convex combination entries");
    near(metric(m,"PSD"),1,0,"convex cone closed under mixtures and nonnegative scales");
    const bool positive=scale>0&&fraction>0&&fraction<1&&angle!=45;
    require((metric(m,"Smallest eigenvalue")>1e-10)==positive,"rank-one mixture interior");++certificates;inspect(m,scene);
  }
  preset(m,5);set(m,MathParameter::PsdMix,0);set(m,MathParameter::PsdRayScale,1);near(metric(m,"Endpoint A PSD"),0,0,"invalid endpoint premise");near(metric(m,"PSD"),0,0,"invalid mixture is not certified");
  level(m,3);
  for(double t:{0.0,.05,.5,1.0,2.0})for(double angle:{-180.0,-70.0,0.0,30.0,90.0,180.0})for(double fraction:{-1.25,-1.0,0.0,1.0,1.25}) {
    set(m,MathParameter::PsdSlice,t);set(m,MathParameter::PsdCostAngle,angle);set(m,MathParameter::PsdObjective,fraction);
    const auto& a=m.snapshot().matrices[0].values;const double x=(a[0]-a[3])/2,y=a[1],cx=std::cos(angle*pi/180),cy=std::sin(angle*pi/180);
    near(a[0]+a[3],2*t,1e-14,"fixed-trace constraint");near(x*x+y*y,t*t,1e-13,"optimum on disk boundary");near(cx*x+cy*y,t,1e-14,"objective attainment");near(metric(m,"PSD"),1,0,"optimum PSD");near(metric(m,"Rank"),t>0?1:0,0,"optimum rank");
    near(metric(m,"Plane intersects disk"),t==0||std::fabs(fraction)<=1?1:0,0,"plane feasibility");
    // Exhaust circle directions as an independent upper-bound comparison.
    for(unsigned i=0;i<96;++i){const double r=2*pi*i/96;require(t*(cx*std::cos(r)+cy*std::sin(r))<=t+1e-14,"support bound");}
    ++certificates;inspect(m,scene);
  }
}
void norms() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Norm,2);
  constexpr std::array<V,7> vectors{{{0,0,0},{1,0,0},{1,1,1},{-.8,.6,.4},{.05,0,0},{1.5,-1.5,.05},{.5,.5,0}}};
  for(double p:{1.0,1.25,1.5,2.0,3.0,8.0,16.0,32.0}) {
    set(m,MathParameter::NormP,p);
    for(const auto x:vectors) {
      set(m,MathParameter::NormX,x[0]);set(m,MathParameter::NormY,x[1]);set(m,MathParameter::NormZ,x[2]);const double expected=norm(x,p),maximum=norm(x,1,true);
      near(metric(m,"Vector norm"),expected,2e-14,"p-norm direct formula");require(expected>=maximum-1e-14&&expected<=std::pow(3.0,1/p)*maximum+1e-14,"squeeze bounds");
      const auto& patch=m.snapshot().surface;for(unsigned i=0;i<patch.rows*patch.columns;++i){const auto& v=patch.vertices[i];near(norm(vector(v.position),p),1,2e-7,"mesh lies on true unit boundary");require(iggy3d::dot(v.position,v.normal)>.01,"outward norm normal");}
      // At p=1 each nondegenerate triangle is entirely in an octahedron face.
      if(p==1)for(unsigned i=0;i+1<patch.rows;++i)for(unsigned j=0;j+1<patch.columns;++j)for(unsigned corner:{0U,1U}) {
        const unsigned a=i*patch.columns+j,b=a+patch.columns;
        const auto center=corner?(patch.vertices[a+1].position+patch.vertices[b].position+patch.vertices[b+1].position)*(1.0F/3):(patch.vertices[a].position+patch.vertices[b].position+patch.vertices[a+1].position)*(1.0F/3);
        near(norm(vector(center),1),1,2e-7,"exact octahedron face, not rounded interpolation");
      }
      const auto& plot=m.snapshot().plots[0];for(std::size_t i=0;i<plot.series[0].count;++i){const auto v=plot.series[0].points[i];near(v.y,norm(x,v.x),2e-14,"limit plot reference");if(i)require(v.y<=plot.series[0].points[i-1].y+1e-14,"p-norm decreases with p");}
      require(hasPart(m,"norm_normalized_probe")== (expected>0),"zero normalization omitted");++certificates;inspect(m,scene);
    }
  }
  preset(m,3);require(m.snapshot().surface.rows==0&&hasPart(m,"norm_exact_cube"),"exact limit uses cube");const auto cube=part(m,"norm_exact_cube");near(cube.x.x,2,0,"exact cube side");near(metric(m,"Vector norm"),.5,1e-14,"max mode exact value");require(!m.snapshot().plots[0].hasMarker,"no finite marker for infinity");inspect(m,scene);
  level(m,3);
  for(double exponent:{1.0,1.25,1.5,2.0,3.0,8.0,16.0,32.0,0.0})for(const auto w:vectors) {
    const bool inf=exponent==0;preset(m,inf?3:1);if(!inf)set(m,MathParameter::NormP,exponent);
    set(m,MathParameter::NormX,w[0]);set(m,MathParameter::NormY,w[1]);set(m,MathParameter::NormZ,w[2]);
    const double p=inf?2:exponent,q=inf?1:p==1?1:p/(p-1);const bool dualInf=!inf&&p==1;
    const double expected=norm(w,q,dualInf);near(metric(m,"Support value"),expected,2e-14,"dual norm formula");near(metric(m,"Support defined"),expected>0?1:0,0,"zero support direction");
    if(expected>0) {
      const auto point=vector(part(m,"norm_support_contact").center);near(norm(point,p,inf),1,2e-7,"contact on primal boundary");near(dot(w,point),expected,2e-7,"dual support certificate");
      const auto dual=part(m,"norm_dual_contact").center-iggy3d::Vec3{3.5F,0,0};near(norm(vector(dual),q,dualInf),1,4e-7,"dual contact lies on dual boundary");
      for(double fraction:{0.0,.7,1.0,1.4}) {
        set(m,MathParameter::NormSupport,fraction);const auto& s=m.snapshot();
        for(std::size_t i=0;i<s.partCount;++i)if(s.parts[i].role=="norm_support_plane")near(dot(w,vector(s.parts[i].center)),expected*fraction,5e-7,"plane offset uses dual support value");
      }
      // Directions independent of the runtime mesh; none can beat the certificate.
      for(unsigned i=1;i<16;++i)for(unsigned j=0;j<24;++j){V v{std::sin(pi*i/16)*std::cos(2*pi*j/24),std::sin(pi*i/16)*std::sin(2*pi*j/24),std::cos(pi*i/16)};const double length=norm(v,p,inf);for(auto& a:v)a/=length;require(dot(w,v)<=expected+1e-13,"support maximum violated");}
    } else require(!hasPart(m,"norm_support_plane")&&!hasPart(m,"norm_support_contact"),"zero support geometry omitted");
    ++certificates;inspect(m,scene);
  }
  select(m,MathObjectKind::Norm,1);require(hasPart(m,"norm_open_boundary")&&m.snapshot().surface.rows==0,"addition is not occluded by solid boundary");
  for(unsigned example:{0U,1U,2U,3U})for(const auto x:vectors)for(const auto y:vectors) {
    preset(m,example);for(unsigned i=0;i<3;++i){set(m,std::array{MathParameter::NormX,MathParameter::NormY,MathParameter::NormZ}[i],x[i]);set(m,std::array{MathParameter::NormOtherX,MathParameter::NormOtherY,MathParameter::NormOtherZ}[i],y[i]);}
    const V sum{x[0]+y[0],x[1]+y[1],x[2]+y[2]};const double p=m.parameter(MathParameter::NormP);const bool inf=m.parameter(MathParameter::NormInfinity)!=0;
    near(metric(m,"Sum norm"),norm(sum,p,inf),3e-14,"sum of vectors");near(metric(m,"Triangle slack"),norm(x,p,inf)+norm(y,p,inf)-norm(sum,p,inf),3e-14,"triangle inequality certificate");require(metric(m,"Triangle slack")>=-1e-13,"triangle inequality violated");++certificates;
    if(x==y)inspect(m,scene);
  }
}
void controls() {
  MathObjects m;MathObjectScene scene;require(mathObjectSpecs().size()==static_cast<std::size_t>(MathObjectKind::Count),"two new objects registered");
  for(const auto kind:{MathObjectKind::Psd,MathObjectKind::Norm})for(unsigned l=0;l<4;++l) {
    select(m,kind,l);require(mathLessons(kind).size()==4,"four layers");
    for(unsigned pass=0;pass<2;++pass)for(const auto& p:mathParameterSpecs())if(m.parameterAvailable(p.id)) {
      set(m,p.id,pass?p.maximum:p.minimum);inspect(m,scene);
      reject(m,{MathActionKind::SetParameter,{},p.id,std::numeric_limits<double>::quiet_NaN()});reject(m,{MathActionKind::SetParameter,{},p.id,p.maximum+1});
    }
    reject(m,{MathActionKind::ObjectPreset,{},{},0,99});reject(m,{MathActionKind::SetLevel,{},{},4});reject(m,{MathActionKind::SetParameter,{},MathParameter::Angle,90});
    select(m,kind,l);const auto presets=mathObjectPresets(kind,l);
    for(unsigned i=0;i<presets.size();++i){const auto revision=m.snapshot().revision;preset(m,i);require(m.snapshot().revision==revision+1,"preset is one semantic revision");for(unsigned j=0;j<presets[i].count;++j)near(m.parameter(presets[i].parameters[j]),presets[i].values[j],0,"preset values");inspect(m,scene);}
  }
  // Exercise both full wire bodies together at finite p: the largest part count.
  select(m,MathObjectKind::Norm,3);set(m,MathParameter::NormWire,1);
  for(double p:{1.25,2.0,32.0}){set(m,MathParameter::NormP,p);set(m,MathParameter::NormSupport,1);inspect(m,scene);require(m.snapshot().surface.rows==0,"wire view has no occluding fill");}
  select(m,MathObjectKind::Psd);checked(m,false);preset(m,1);checked(m,true);set(m,MathParameter::PsdB,.5);require(m.snapshot().feedback==MathFeedback::None,"editing clears old feedback");
  level(m,1);checked(m,false);preset(m,2);set(m,MathParameter::PsdProbeAngle,90);checked(m,true);
  level(m,2);checked(m,false);preset(m,1);checked(m,true);set(m,MathParameter::PsdRayScale,0);checked(m,false);
  level(m,3);checked(m,false);set(m,MathParameter::PsdObjective,1);checked(m,true);set(m,MathParameter::PsdSlice,0);checked(m,false);reject(m,{MathActionKind::ObjectPreset,{},{},0,0});
  level(m,0);require(!m.parameterAvailable(MathParameter::PsdObjective),"higher-level controls hidden");
  select(m,MathObjectKind::Norm);checked(m,false);preset(m,0);set(m,MathParameter::NormX,.5);set(m,MathParameter::NormY,.5);set(m,MathParameter::NormZ,0);checked(m,true);
  level(m,1);preset(m,1);checked(m,true);set(m,MathParameter::NormOtherX,0);set(m,MathParameter::NormOtherY,0);set(m,MathParameter::NormOtherZ,0);checked(m,false);
  level(m,2);checked(m,false);preset(m,2);checked(m,true);preset(m,3);checked(m,false);reject(m,{MathActionKind::SetParameter,{},MathParameter::NormP,8});
  level(m,3);checked(m,false);set(m,MathParameter::NormSupport,1);checked(m,true);for(auto p:{MathParameter::NormX,MathParameter::NormY,MathParameter::NormZ})set(m,p,0);checked(m,false);
  action(m,{MathActionKind::Reset});near(m.parameter(MathParameter::NormInfinity),0,0,"reset finite mode");require(m.snapshot().feedback==MathFeedback::None,"reset clears feedback");
}
}
int main(){try{psd();norms();controls();std::printf("Convex objects passed: %zu independent certificates; %zu CPU meshes; max %zu vertices / %zu indices. No native host, images or font initialization.\n",certificates,meshes,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"Convex objects failed: %s\n",e.what());return 1;}}

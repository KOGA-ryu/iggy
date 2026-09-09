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
void flux() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Flux,1);
  for(unsigned shape=0;shape<3;++shape)for(unsigned rule=0;rule<4;++rule)for(double r:{.5,1.0,1.5})for(double degrees:{0.0,30.0,90.0,150.0,180.0})for(unsigned reversed=0;reversed<2;++reversed) {
    set(m,MathParameter::FluxShape,shape);set(m,MathParameter::FluxField,rule);set(m,MathParameter::FluxRadius,r);set(m,MathParameter::FluxTilt,degrees);set(m,MathParameter::FluxOrientation,reversed);
    const double sign=reversed?-1:1,c=std::cos(degrees*pi/180),volume=shape==0?4*pi*r*r*r/3:8*r*r*r;
    const double exact=shape<2?sign*(rule==1?3:rule==3?1:0)*volume:sign*pi*r*r*c*(rule==0?1:rule==1||rule==3?.4:0);
    near(metric(m,"Exact flux"),exact,1e-11,"exact surface flux");
    for(unsigned n:{2U,6U,12U}) {set(m,MathParameter::FluxResolution,n);double expected=exact;if(shape==0&&rule==3)expected*=1+(1-3*c*c)/(2*n*n);near(metric(m,"Numerical flux"),expected,1e-10,"independent midpoint surface moment");near(metric(m,"Normal length"),1,1e-12,"unit surface normal");}
    inspect(m,scene);
  }
  select(m,MathObjectKind::Flux);checked(m,false);set(m,MathParameter::FluxOrientation,1);checked(m,true);
  level(m,1);set(m,MathParameter::FluxShape,2);set(m,MathParameter::FluxField,0);set(m,MathParameter::FluxOrientation,0);checked(m,true);
  level(m,2);require(metric(m,"Closed surface")==0,"open disk called closed");for(std::size_t i=0;i<m.snapshot().metricCount;++i)require(m.snapshot().metrics[i].label!="Enclosed volume","disk given enclosed volume");
  set(m,MathParameter::FluxShape,0);set(m,MathParameter::FluxField,3);set(m,MathParameter::FluxResolution,4);checked(m,false);set(m,MathParameter::FluxResolution,12);near(metric(m,"Oriented divergence integral"),4*pi/3,1e-12,"divergence integral");checked(m,true);
  level(m,3);reject(m,{MathActionKind::SetParameter,{},MathParameter::FluxShape,0});
  for(unsigned rule=0;rule<4;++rule)for(double degrees:{0.0,35.0,90.0,155.0})for(unsigned reverse=0;reverse<2;++reverse) {
    set(m,MathParameter::FluxField,rule);set(m,MathParameter::FluxTilt,degrees);set(m,MathParameter::FluxOrientation,reverse);set(m,MathParameter::FluxResolution,12);
    const double sign=reverse?-1:1,tilt=degrees*pi/180;near(metric(m,"Boundary circulation"),(rule>=2?2*pi*std::cos(tilt)*sign:0),1e-11,"closed circulation");near(metric(m,"Stokes error"),0,1e-11,"Stokes equality");
    for(double t:{0.0,.125,.375,.5,.875,1.0}) {set(m,MathParameter::FluxTime,t);const double dz=std::sin(tilt)*std::sin(sign*2*pi*t),conservative=.4*dz+.5*dz*dz;const double expected=rule==0?dz:rule==1?.4*dz:sign*2*pi*std::cos(tilt)*t+(rule==3?conservative:0);near(metric(m,"Circulation so far"),expected,1e-5,"independent partial boundary integral");inspect(m,scene);}
  }
  set(m,MathParameter::FluxTilt,0);set(m,MathParameter::FluxField,2);set(m,MathParameter::FluxOrientation,1);set(m,MathParameter::FluxTime,0);action(m,{MathActionKind::TogglePlayback});action(m,{MathActionKind::AdvanceTime,{},{},1});require(!m.snapshot().playing,"boundary did not stop");checked(m,true);reject(m,{MathActionKind::AdvanceTime,{},{},.1});
}
using V=std::array<double,3>;
constexpr std::array<MathParameter,3> up{MathParameter::TensorU0,MathParameter::TensorU1,MathParameter::TensorU2},vp{MathParameter::TensorV0,MathParameter::TensorV1,MathParameter::TensorV2},wp{MathParameter::TensorW0,MathParameter::TensorW1,MathParameter::TensorW2};
void vector(MathObjects& m,const std::array<MathParameter,3>& ids,const V& v){for(unsigned i=0;i<3;++i)set(m,ids[i],v[i]);}
double length(const V& v){return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);}
void tensors() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Tensor);
  for(int a=0;a<27;++a)for(int b=0;b<27;++b) {
    V u{double(a%3-1),double(a/3%3-1),double(a/9-1)},v{double(b%3-1),double(b/3%3-1),double(b/9-1)};vector(m,up,u);vector(m,vp,v);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)near(m.snapshot().matrices[0].values[3*i+j],u[i]*v[j],0,"outer component");near(metric(m,"Trace"),u[0]*v[0]+u[1]*v[1]+u[2]*v[2],0,"outer trace");near(metric(m,"Tensor norm"),length(u)*length(v),1e-12,"outer norm multiplicative");if(a%6==0&&b%6==0)inspect(m,scene);
  }
  vector(m,up,{1,1,0});vector(m,vp,{1,-1,0});checked(m,true);vector(m,up,{0,0,0});checked(m,false);
  constexpr std::array<V,7> vectors{{{0,0,0},{1,0,0},{0,1,0},{0,0,1},{1,1,0},{1,-1,2},{-2,2,-2}}};
  level(m,2);
  for(const auto& u:vectors)for(const auto& v:vectors)for(const auto& w:vectors) {
    vector(m,up,u);vector(m,vp,v);vector(m,wp,w);V contraction{};double norm=0;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k){const double x=u[i]*v[j]*w[k];near(m.snapshot().matrices[k].values[3*i+j],x,0,"three index component");near(m.snapshot().table.values[9*k+3*i+j][0],x,0,"table and tensor agree");norm+=x*x;if(j==k)contraction[i]+=x;}
    near(metric(m,"Tensor norm"),std::sqrt(norm),1e-11,"rank three norm");near(metric(m,"Tensor norm"),length(u)*length(v)*length(w),1e-11,"tensor norm product");near(metric(m,"Contraction x"),contraction[0],1e-12,"contraction x");near(metric(m,"Contraction y"),contraction[1],1e-12,"contraction y");near(metric(m,"Contraction z"),contraction[2],1e-12,"contraction z");inspect(m,scene);
  }
  vector(m,up,{1,0,0});vector(m,vp,{1,1,0});vector(m,wp,{1,-1,0});checked(m,true);vector(m,wp,{0,0,0});checked(m,false);
  level(m,1);vector(m,up,{1,0,0});vector(m,vp,{-1,0,0});vector(m,wp,{1,0,0});set(m,MathParameter::TensorI,0);set(m,MathParameter::TensorJ,0);set(m,MathParameter::TensorK,0);checked(m,true);
  level(m,3);reject(m,{MathActionKind::SetParameter,{},MathParameter::TensorW0,1});
  for(const auto& u:vectors)for(const auto& v:vectors)for(int degrees=-180;degrees<=180;degrees+=15) {
    vector(m,up,u);vector(m,vp,v);set(m,MathParameter::TensorBasis,degrees);const auto& s=m.snapshot();const double c=std::cos(degrees*pi/180),sn=std::sin(degrees*pi/180);const V cu{c*u[0]+sn*u[1],-sn*u[0]+c*u[1],u[2]},cv{c*v[0]+sn*v[1],-sn*v[0]+c*v[1],v[2]};
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)near(s.matrices[1].values[3*i+j],cu[i]*cv[j],1e-12,"basis via transformed vector factors");near(metric(m,"Trace error"),0,1e-12,"trace invariance");near(metric(m,"Norm error"),0,1e-12,"Frobenius invariance");if(degrees%90==0)inspect(m,scene);
  }
  vector(m,up,{1,1,0});vector(m,vp,{1,-1,1});set(m,MathParameter::TensorBasis,45);checked(m,true);set(m,MathParameter::TensorBasis,0);checked(m,false);
}
void probability() {
  MathObjects m;MathObjectScene scene;
  for(unsigned rule=0;rule<4;++rule)for(double stay:{0.0,.25,.5,.95,1.0})for(unsigned start=0;start<3;++start)for(double mix:{0.0,.5,1.0}) {
    select(m,MathObjectKind::Probability,3);set(m,MathParameter::ProbabilityRule,rule);if(rule!=2)set(m,MathParameter::ProbabilityStay,stay);set(m,MathParameter::ProbabilityStart,start);set(m,MathParameter::ProbabilityMix,mix);
    const auto matrix=m.snapshot().matrices[0].values;for(unsigned i=0;i<3;++i){double sum=0;for(unsigned j=0;j<3;++j){require(matrix[3*i+j]>=0&&matrix[3*i+j]<=1,"transition range");sum+=matrix[3*i+j];}near(sum,1,1e-14,"stochastic row");}
    const V initial{mix/3+(start==0?1-mix:0),mix/3+(start==1?1-mix:0),mix/3+(start==2?1-mix:0)};
    for(unsigned n:{0U,1U,2U,3U,12U,64U}) {
      set(m,MathParameter::ProbabilitySteps,n);V expected{};
      switch(rule) {
        case 0: {double choose=1;for(unsigned jumps=0;jumps<=n;++jumps){const double weight=choose*std::pow(1-stay,jumps)*std::pow(stay,n-jumps);for(unsigned i=0;i<3;++i)expected[(i+jumps)%3]+=initial[i]*weight;if(jumps<n)choose*=double(n-jumps)/(jumps+1);}break;}
        case 1:for(unsigned i=0;i<3;++i)expected[i]=V{.5,.3,.2}[i]+std::pow(stay,n)*(initial[i]-V{.5,.3,.2}[i]);break;
        case 2:for(unsigned i=0;i<3;++i)expected[(i+n)%3]=initial[i];break;
        case 3: {const double same=std::pow(stay,n),one=n? n*(1-stay)*std::pow(stay,n-1):0;expected={initial[0]*same,initial[1]*same+initial[0]*one,0};expected[2]=1-expected[0]-expected[1];break;}
      }
      for(unsigned i=0;i<3;++i)near(m.snapshot().table.values[i][1],expected[i],2e-13,"closed form probability evolution");near(metric(m,"Probability sum"),1,2e-13,"probability conservation");near(metric(m,"Stationary residual"),0,1e-14,"stationary fixed point");near(metric(m,"Unique stationary distribution"),(rule==2||stay<1)?1:0,0,"stationary uniqueness classification");inspect(m,scene);
    }
  }
  select(m,MathObjectKind::Probability);set(m,MathParameter::ProbabilityRule,2);checked(m,false);action(m,{MathActionKind::ProbabilityStep});action(m,{MathActionKind::ProbabilityStep});checked(m,true);
  for(unsigned i=2;i<64;++i)action(m,{MathActionKind::ProbabilityStep});require(m.snapshot().probabilityWalkCount==65,"walk cap wrong");for(unsigned i=0;i<65;++i)require(m.snapshot().probabilityWalk[i]==i%3,"periodic sampled path");reject(m,{MathActionKind::ProbabilityStep});
  for(unsigned seed:{0U,7U,65535U}) {set(m,MathParameter::ProbabilityRule,0);set(m,MathParameter::ProbabilitySeed,seed);set(m,MathParameter::ProbabilityStay,.5);for(unsigned i=0;i<64;++i)action(m,{MathActionKind::ProbabilityStep});const auto path=m.snapshot().probabilityWalk;action(m,{MathActionKind::ResetProbabilityWalk});for(unsigned i=0;i<64;++i)action(m,{MathActionKind::ProbabilityStep});require(path==m.snapshot().probabilityWalk,"seed replay differs");for(unsigned i=1;i<65;++i)require(path[i]==path[i-1]||path[i]==(path[i-1]+1)%3,"walk took impossible edge");}
  set(m,MathParameter::ProbabilityStay,1);require(m.snapshot().probabilityWalkCount==1,"changed setup retained walk");for(unsigned i=0;i<10;++i)action(m,{MathActionKind::ProbabilityStep});near(metric(m,"Visited states"),1,0,"identity walk moved");
  level(m,1);set(m,MathParameter::ProbabilityRule,1);set(m,MathParameter::ProbabilityStay,.5);set(m,MathParameter::ProbabilityRow,2);near(metric(m,"p(A)"),.25,1e-14,"row C probability A");near(metric(m,"p(B)"),.15,1e-14,"row C probability B");near(metric(m,"p(C)"),.6,1e-14,"row C probability C");near(m.snapshot().table.values[2][0],1,0,"row probe initial vector");checked(m,true);reject(m,{MathActionKind::ProbabilityStep});
  level(m,2);set(m,MathParameter::ProbabilityRule,3);set(m,MathParameter::ProbabilityStart,0);set(m,MathParameter::ProbabilitySteps,12);checked(m,true);
  level(m,3);set(m,MathParameter::ProbabilityRule,1);checked(m,true);set(m,MathParameter::ProbabilityStay,1);checked(m,false);near(metric(m,"Convergence to pi guaranteed"),0,0,"identity falsely guaranteed mixing");
  set(m,MathParameter::ProbabilityRule,2);set(m,MathParameter::ProbabilitySteps,64);near(metric(m,"Distance to pi"),2.0/3,1e-14,"periodic distribution falsely settled");near(metric(m,"Convergence to pi guaranteed"),0,0,"periodic convergence");
}
void boundaries() {
  MathObjects m;MathObjectScene scene;require(mathObjectSpecs().size()==static_cast<std::size_t>(MathObjectKind::Count),"wrong object count");std::set<std::string_view> keys;
  for(const auto& p:mathParameterSpecs())require(keys.insert(p.key).second,"duplicate CLI key");
  for(const auto kind:{MathObjectKind::Flux,MathObjectKind::Tensor,MathObjectKind::Probability})for(unsigned l=0;l<4;++l) {
    select(m,kind,l);require(mathLessons(kind).size()==4,"missing layer");inspect(m,scene);
    for(unsigned pass=0;pass<2;++pass)for(const auto& spec:mathParameterSpecs())if(m.parameterAvailable(spec.id)){set(m,spec.id,pass?spec.maximum:spec.minimum);inspect(m,scene);const auto before=m.snapshot().probabilityWalk;const auto count=m.snapshot().probabilityWalkCount;reject(m,{MathActionKind::SetParameter,{},spec.id,std::numeric_limits<double>::quiet_NaN()});require(before==m.snapshot().probabilityWalk&&count==m.snapshot().probabilityWalkCount,"rejection changed walk");reject(m,{MathActionKind::SetParameter,{},spec.id,spec.maximum+1});}
    reject(m,{MathActionKind::SetLevel,{},{},4});reject(m,{MathActionKind::AdvanceTime,{},{},-1});
  }
  select(m,MathObjectKind::Algebra);reject(m,{MathActionKind::ProbabilityStep});reject(m,{MathActionKind::ResetProbabilityWalk});
}
}
int main(){try{flux();tensors();probability();boundaries();std::printf("Shell/tensor/probability tests passed: analytic flux and Stokes, 729 outer products, 343 rank-three tensors, 1225 basis cases, exact Markov laws, walk replay, challenges and %zu geometry states; maxima %zu vertices / %zu indices\n",states,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}

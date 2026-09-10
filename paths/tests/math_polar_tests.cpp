#include "runtime/math_objects/PolarDecomposition.hpp"
#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include "ui/MathObjectLayout.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

using namespace paths;
namespace {
using M=PolarMatrix;using P=MathParameter;using K=MathObjectKind;
constexpr M identity{1,0,0,0,1,0,0,0,1};
unsigned checks=0,cases=0,scenes=0;std::size_t maxVertices=0,maxIndices=0;double maxFactorError=0,maxOrthogonalError=0;
void require(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double tolerance,const char* why){++checks;if(!std::isfinite(a)||!std::isfinite(b)||std::fabs(a-b)>tolerance){char text[300];std::snprintf(text,sizeof(text),"%s: %.17g versus %.17g, tolerance %.3g",why,a,b,tolerance);throw std::runtime_error(text);}}
M mul(const M& a,const M& b){M out{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)out[3*i+j]+=a[3*i+k]*b[3*k+j];return out;}
M trans(const M& a){M out{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)out[3*i+j]=a[3*j+i];return out;}
double det(const M& a){return a[0]*(a[4]*a[8]-a[5]*a[7])-a[1]*(a[3]*a[8]-a[5]*a[6])+a[2]*(a[3]*a[7]-a[4]*a[6]);}
double norm(const M& a){double n=0;for(double x:a)n=std::hypot(n,x);return n;}
double difference(const M& a,const M& b){M d{};for(unsigned i=0;i<9;++i)d[i]=a[i]-b[i];return norm(d);}
M rotation(unsigned axis,double angle){M r=identity;const unsigned a=(axis+1)%3,b=(axis+2)%3;r[3*a+a]=r[3*b+b]=std::cos(angle);r[3*a+b]=-std::sin(angle);r[3*b+a]=std::sin(angle);return r;}
// Independent cofactor inverse, used only as a test oracle. Production evaluates
// the inverse-transpose recurrence through the singular modes.
M inverseTranspose(const M& a){M out{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){const unsigned r=(i+1)%3,s=(i+2)%3,c=(j+1)%3,d=(j+2)%3;out[3*i+j]=(a[3*r+c]*a[3*s+d]-a[3*r+d]*a[3*s+c])/det(a);}return out;}
void certificate(const M& a,const PolarAnalysis& p){
  ++cases;const double error=norm(a)>0?difference(a,mul(p.w,p.p))/norm(a):0;maxFactorError=std::max(maxFactorError,error);maxOrthogonalError=std::max(maxOrthogonalError,p.orthogonalityError);
  near(p.reconstructionError,error,2e-15,"reported reconstruction residual");require(error<5e-12,"A != W P");require(difference(a,mul(p.alternateW,p.p))<5e-12*std::max(1.,norm(a)),"alternate extension changed product");
  near(difference(mul(trans(p.w),p.w),identity),p.orthogonalityError,3e-15,"reported orthogonality");require(p.orthogonalityError<2e-13,"W not orthogonal");require(difference(mul(trans(p.alternateW),p.alternateW),identity)<2e-13,"alternate W not orthogonal");
  require(difference(p.p,trans(p.p))<2e-14,"P not symmetric");require(difference(mul(p.p,p.p),mul(trans(a),a))<8e-11,"P squared not A transpose A");
  require(p.singular[0]>=p.singular[1]&&p.singular[1]>=p.singular[2]&&p.singular[2]>=0,"singular value order/sign");
  if(p.rank==3){M normalized=a;for(double& x:normalized)x/=norm(a);near(p.detW,det(normalized)>0?1:-1,2e-13,"orthogonal factor handedness");near(difference(p.w,p.alternateW),0,0,"alternate for invertible input");}
  else {near(difference(p.w,p.alternateW),2,2e-13,"null direction was not reversed");require(!p.iterationAvailable,"rank-deficient inverse iteration allowed");}
  // PSD certificate independent of the SVD: all principal minors.
  for(unsigned i=0;i<3;++i){require(p.p[4*i]>=-1e-13,"negative P diagonal");for(unsigned j=i+1;j<3;++j)require(p.p[4*i]*p.p[4*j]-p.p[3*i+j]*p.p[3*j+i]>=-2e-12,"negative P principal minor");}require(det(p.p)>=-1e-11,"negative determinant of P");
  if(p.iterationAvailable){
    require(p.iterates[0]==a,"X0 not exact A");
    for(unsigned k=0;k<=polarMaxSteps;++k){const auto& x=p.iterates[k];near(difference(mul(trans(x),x),identity),p.iterationError[k],1e-14*std::max(1.,p.iterationError[k]),"trace residual");
      if(k<polarMaxSteps){const auto inverse=inverseTranspose(x);M expected{};for(unsigned i=0;i<9;++i)expected[i]=.5*(x[i]+inverse[i]);require(difference(expected,p.iterates[k+1])<2e-8*std::max(1.,norm(expected)),"inverse-transpose recurrence disagreement");}
      if(k>=1)for(double d:p.iterationSingular[k])require(d>=1-2e-15,"positive mode fell below one after first iterate");
    }
    require(difference(p.iterates.back(),p.w)<2e-12,"iteration did not reach W");
  }else {for(const auto& x:p.iterates)for(double d:x)near(d,0,0,"unavailable trace contains fabricated iterate");}
}
void mathematics(){
  certificate(M{},analyzePolar(M{}));certificate(identity,analyzePolar(identity));
  const std::array<std::array<double,3>,8> spectra{{{3,2,.5},{1,1,1},{2,2,0},{1,0,0},{2,1,.05},{2,1,1e-6},{2,1,1e-11},{2,1,1e-14}}};
  for(unsigned n=0;n<64;++n)for(const auto& spectrum:spectra){auto w=mul(rotation(0,n*.17),rotation(2,n*.29));if(n%2)for(unsigned i=0;i<3;++i)w[3*i]=-w[3*i];const auto q=mul(rotation(1,n*.23),rotation(2,n*.11));M diagonal{};for(unsigned i=0;i<3;++i)diagonal[4*i]=spectrum[i];const auto p=mul(mul(q,diagonal),trans(q)),a=mul(w,p);const auto result=analyzePolar(a);certificate(a,result);require(difference(result.p,p)<1e-11,"known symmetric positive factor");if(result.rank==3&&spectrum[2]>=1e-6)require(difference(result.w,w)<2e-9,"known unique orthogonal factor");}
  for(unsigned n=0;n<320;++n){M a{};for(unsigned i=0;i<9;++i)a[i]=std::sin((n+1)*(.4+i*.137))*2.5;if(n%7==0)for(unsigned i=0;i<3;++i)a[6+i]=a[i];certificate(a,analyzePolar(a));}
  // Exact shear has a known planar polar factor and one unaffected axis.
  const M shear{1,1,0,0,1,0,0,0,1};auto s=analyzePolar(shear);const double root=std::sqrt(5.);const M expectedW{2/root,1/root,0,-1/root,2/root,0,0,0,1},expectedP{2/root,1/root,0,1/root,3/root,0,0,0,1};near(difference(s.w,expectedW),0,8e-16,"analytic shear W");near(difference(s.p,expectedP),0,1e-15,"analytic shear P");
  for(double scale:{1e-150,1e-8,.1,2.}){M a=shear;for(auto& x:a)x*=scale;const auto p=analyzePolar(a);certificate(a,p);require(p.rank==3,"rank should be relative to scale");near(difference(p.w,s.w),0,2e-15,"W should be invariant to positive scale");}
  M thin{2,0,0,0,1,0,0,0,.05};const auto t=analyzePolar(thin);require(t.iterationSingular[1][2]>10,"thin direction did not show first-step expansion");
  for(unsigned i=0;i<9;++i)for(double value:{-4.01,4.01,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){M a=identity;a[i]=value;bool caught=false;try{analyzePolar(a);}catch(const std::invalid_argument&){caught=true;}require(caught,"invalid polar input accepted");}
}
void act(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double value){act(m,{MathActionKind::SetParameter,{},p,value});}
void level(MathObjects& m,unsigned value){act(m,{MathActionKind::SetLevel,{},{},static_cast<double>(value)});}
void preset(MathObjects& m,unsigned value){act(m,{MathActionKind::ObjectPreset,{},{},0,value});}
double metric(const MathObjects& m,std::string_view label){for(unsigned i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==label)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric "+std::string(label));}
void inspect(MathObjects& m,MathObjectScene& scene){
  const auto& s=m.snapshot();const auto& frame=scene.publish(s,{0,0,800,600});++scenes;maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"scene capacity");require(iggy3d::isFinite(frame.clipFromWorld),"finite camera");
  std::set<unsigned> ids;unsigned end=0;for(const auto& d:frame.draws){require(ids.insert(d.objectId.value).second&&d.firstIndex==end,"draw coverage/identity");end+=d.indexCount;}require(end==frame.indices.size(),"draw coverage end");for(const auto& v:frame.vertices){for(float x:v.position)require(std::isfinite(x),"finite vertex");for(float x:v.color)require(std::isfinite(x)&&x>=0&&x<=1,"finite vertex colour");}for(auto i:frame.indices)require(i<frame.vertices.size(),"mesh index range");
  for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"finite metric");for(unsigned i=0;i<s.matrixCount;++i)for(double x:s.matrices[i].values)require(std::isfinite(x),"finite matrix");for(unsigned i=0;i<s.plotCount;++i)for(unsigned j=0;j<s.plots[i].seriesCount;++j)for(unsigned k=0;k<s.plots[i].series[j].count;++k){const auto& p=s.plots[i].series[j].points[k];require(std::isfinite(p.x)&&std::isfinite(p.y),"finite plot");}
  require(s.matrixCount>=1&&s.matrices[0].editable==(s.level>0),"editable matrix route");M a{};for(unsigned i=0;i<9;++i){const auto parameter=static_cast<P>(static_cast<unsigned>(P::PolarA00)+i);a[i]=m.parameter(parameter);require(s.matrices[0].parameters[i]==parameter,"matrix action bound to wrong owner");near(s.matrices[0].values[i],a[i],0,"input matrix value");}const auto p=analyzePolar(a);near(metric(m,"Numerical rank"),p.rank,0,"snapshot rank");near(metric(m,"Relative reconstruction error"),p.reconstructionError,0,"snapshot residual");
  if(s.level==1||s.level==3){require(s.matrixCount==3,"factor matrix count");require(s.matrices[1].values==p.p,"snapshot P");require(s.matrices[2].values==(s.level==3&&m.parameter(P::PolarExtension)==1?p.alternateW:p.w),"snapshot W extension");}
  if(s.level==2){near(metric(m,"Iteration available"),p.iterationAvailable,0,"iteration status");if(p.iterationAvailable){const auto step=static_cast<unsigned>(std::floor(m.parameter(P::PolarIteration)+1e-9));require(s.matrices[1].values==p.iterates[step],"snapshot iterate");near(metric(m,"Iterate orthogonality error"),p.iterationError[step],0,"iterate residual");}}
  const auto rows=mathControlRows(m);std::set<unsigned> visible;for(unsigned i=0;i<rows.count;++i)for(unsigned j=0;j<rows.rows[i].count;++j)require(visible.insert(static_cast<unsigned>(rows.rows[i].parameters[j])).second,"duplicate compact control");for(unsigned i=0;i<9;++i)require(visible.contains(static_cast<unsigned>(P::PolarA00)+i)==(s.level==0),"matrix controls duplicated or absent");
  for(unsigned i=0;i<s.solid.indexCount;i+=3){const auto& a=s.solid.vertices[s.solid.indices[i]];const auto& b=s.solid.vertices[s.solid.indices[i+1]];const auto& c=s.solid.vertices[s.solid.indices[i+2]];require(iggy3d::dot(iggy3d::cross(b.position-a.position,c.position-a.position),a.normal)>0,"triangle winding disagrees with normal");}
}
void integration(){
  require(static_cast<unsigned>(K::Distance)==32&&static_cast<unsigned>(K::Polar)==33,"appended object changed stable IDs");require(mathObjectSpecs().size()>=34,"object registry size");MathObjects m;MathObjectScene scene;act(m,{MathActionKind::Select,K::Polar});require(mathLessons(K::Polar).size()==4,"four layers");
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned p=0;p<6;++p){preset(m,p);for(unsigned shape=0;shape<2;++shape){set(m,P::PolarShape,shape);for(unsigned guides=0;guides<2;++guides){set(m,P::PolarGuides,guides);inspect(m,scene);}}}}
  level(m,2);for(unsigned p:{0U,3U,5U}){preset(m,p);for(unsigned step=0;step<=32;++step){set(m,P::PolarIteration,step);inspect(m,scene);}}
  // Every supported coefficient extreme, including reflection and rank loss.
  for(unsigned i=0;i<9;++i)for(double x:{-3.,0.,3.}){level(m,1);preset(m,0);set(m,static_cast<P>(static_cast<unsigned>(P::PolarA00)+i),x);inspect(m,scene);}
  for(unsigned l=0;l<4;++l){level(m,l);preset(m,0);for(unsigned i=0;i<9;++i)set(m,static_cast<P>(static_cast<unsigned>(P::PolarA00)+i),0);inspect(m,scene);if(l==2){require(!m.parameterAvailable(P::PolarIteration),"zero matrix iteration control");require(!m.dispatch({MathActionKind::TogglePlayback}).accepted,"zero matrix playback");}if(l==3){set(m,P::PolarExtension,1);inspect(m,scene);}}
  // The straight blend to a half-turn passes through a rank-one matrix.
  level(m,0);preset(m,2);set(m,P::PolarA00,-1);set(m,P::PolarA01,0);set(m,P::PolarA10,0);set(m,P::PolarA11,-1);set(m,P::PolarAmount,.5);inspect(m,scene);require(m.snapshot().solid.indexCount==0,"collapsed blend emitted overlapping faces");
  for(unsigned l=0;l<4;++l){level(m,l);preset(m,l==1?1:l==3?4:0);if(l==2)set(m,P::PolarIteration,7);if(l==3)set(m,P::PolarExtension,1);act(m,{MathActionKind::Check});require(m.snapshot().feedback==MathFeedback::Solved,"challenge pass case");if(l==0)set(m,P::PolarAmount,0);if(l==1)preset(m,2);if(l==2)set(m,P::PolarIteration,0);if(l==3)set(m,P::PolarExtension,0);act(m,{MathActionKind::Check});require(m.snapshot().feedback==MathFeedback::TryAgain,"challenge fail case");}
  level(m,2);preset(m,0);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},2.5});near(m.parameter(P::PolarIteration),1.25,0,"playback progress");near(metric(m,"Iteration k"),1,0,"discrete iteration index");set(m,P::PolarA01,.5);require(!m.snapshot().playing,"coefficient edit should pause playback");
  set(m,P::PolarIteration,31);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},4});near(m.parameter(P::PolarIteration),32,0,"bounded playback endpoint");require(!m.snapshot().playing,"playback endpoint should pause");
  auto revision=m.snapshot().revision;auto matrix=m.snapshot().matrices[0].values;require(!m.dispatch({MathActionKind::SetParameter,{},P::PolarA00,std::numeric_limits<double>::quiet_NaN()}).accepted,"invalid mutation accepted");require(m.snapshot().revision==revision&&m.snapshot().matrices[0].values==matrix,"invalid action mutated state");
  level(m,0);preset(m,0);set(m,P::PolarA00,2);act(m,mathResetControlGroup(m,MathControlGroup::Transform));near(m.parameter(P::PolarA00),1,0,"matrix group reset");set(m,P::PolarAmount,.3);level(m,3);level(m,0);near(m.parameter(P::PolarAmount),.3,0,"layer switch lost retained state");
}
}
int main(){try{mathematics();integration();std::printf("polar: %u assertions, %u matrix cases, %u scenes; max vertices=%zu indices=%zu; max relative reconstruction=%.3e, orthogonality=%.3e\n",checks,cases,scenes,maxVertices,maxIndices,maxFactorError,maxOrthogonalError);return 0;}catch(const std::exception& e){std::fprintf(stderr,"polar test failure: %s (after %u assertions)\n",e.what(),checks);return 1;}}

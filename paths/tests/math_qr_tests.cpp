#include "runtime/math_objects/QrLeastSquares.hpp"
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
using V=QrVector;using A=QrColumns;using X=QrCoefficients;using P=MathParameter;using K=MathObjectKind;
unsigned checks=0,cases=0,scenes=0;std::size_t maxVertices=0,maxIndices=0;double maxFactorError=0,maxOrthogonality=0;
void require(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(double a,double b,double tolerance,const char* why){++checks;if(!std::isfinite(a)||!std::isfinite(b)||std::fabs(a-b)>tolerance){char s[300];std::snprintf(s,sizeof(s),"%s: %.17g vs %.17g (tol %.3g)",why,a,b,tolerance);throw std::runtime_error(s);}}
double dot(V a,V b){double out=0;for(unsigned i=0;i<3;++i)out+=a[i]*b[i];return out;}
double norm(V a){return std::hypot(a[0],a[1],a[2]);}
V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
V apply(A a,X x){V out{};for(unsigned i=0;i<3;++i)out[i]=a[0][i]*x[0]+a[1][i]*x[1];return out;}
void certificate(A a,V b,const QrAnalysis& q,bool tight=true){
 ++cases;maxFactorError=std::max(maxFactorError,q.reconstructionError);maxOrthogonality=std::max(maxOrthogonality,q.orthogonalityError);require(q.rank<=2,"rank bound");require(q.order[0]!=q.order[1]&&q.order[0]<2&&q.order[1]<2,"pivot permutation");require(norm(a[q.order[0]])>=norm(a[q.order[1]])-1e-15,"longest-column pivot");
 near(q.r[2],0,0,"R lower triangular entry");require(q.r[0]>=0&&q.r[3]>=0,"negative R diagonal");double actualError=0,anorm=std::hypot(norm(a[0]),norm(a[1]));
 for(unsigned col=0;col<2;++col)for(unsigned i=0;i<3;++i){double value=q.q[0][i]*q.r[col]+q.q[1][i]*q.r[2+col];actualError=std::hypot(actualError,value-a[q.order[col]][i]);}
 near(q.reconstructionError,anorm?actualError/anorm:0,1e-15,"factor error readout");require(q.reconstructionError<2e-10,"QR reconstruction");require(q.orthogonalityError<1e-13,"active orthogonality");
 for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j)near(dot(q.q[i],q.q[j]),i==j&&i<q.rank?1:0,1e-13,"thin active Q contract");
 const auto fit=apply(a,q.solution);for(unsigned i=0;i<3;++i){near(q.residual[i]+q.projected[i],b[i],2e-15,"target decomposition");if(tight)near(fit[i],q.projected[i],5e-12,"solution gives projection");}
 near(dot(q.residual,q.residual),q.residualSquared,1e-15,"minimum squared residual");near(std::hypot(dot(a[0],q.residual),dot(a[1],q.residual)),q.normalResidual,2e-15,"normal residual");
 for(unsigned k=0;k<2-q.rank;++k){near(std::hypot(q.nullBasis[k][0],q.nullBasis[k][1]),1,5e-15,"unit null direction");require(norm(apply(a,q.nullBasis[k]))<=3e-10*std::max(1.,anorm),"null direction changed fit");if(tight)near(q.solution[0]*q.nullBasis[k][0]+q.solution[1]*q.nullBasis[k][1],0,2e-12*std::max(1.,std::hypot(q.solution[0],q.solution[1])),"minimum norm orthogonal to null space");}
 if(q.rank==0)near(q.nullBasis[0][0]*q.nullBasis[1][0]+q.nullBasis[0][1]*q.nullBasis[1][1],0,0,"orthogonal zero-map null basis");
}
void mathematics(){
 // Independent integer-rank and geometric-projection oracle for every 3x2
 // matrix with entries -1,0,1. No QR or normal-equation solve for projection.
 for(unsigned code=0;code<729;++code){unsigned digits=code;A a{};for(auto& col:a)for(double& x:col){x=static_cast<int>(digits%3)-1;digits/=3;}const auto n=cross(a[0],a[1]);const unsigned rank=dot(n,n)>0?2:(norm(a[0])+norm(a[1])>0?1:0);
  for(V b:std::array<V,4>{{{0,0,0},{1,2,-1},{-2,0,1},{.25,-.5,.75}}}){const auto q=analyzeQr(a,b);certificate(a,b,q);require(q.rank==rank,"exact integer rank");V expected{};X coefficients{};
   if(rank==2){for(unsigned i=0;i<3;++i)expected[i]=b[i]-n[i]*dot(b,n)/dot(n,n);const double g00=dot(a[0],a[0]),g01=dot(a[0],a[1]),g11=dot(a[1],a[1]),det=g00*g11-g01*g01;coefficients={(g11*dot(a[0],b)-g01*dot(a[1],b))/det,(g00*dot(a[1],b)-g01*dot(a[0],b))/det};}
   if(rank==1){const auto v=norm(a[0])>0?a[0]:a[1];for(unsigned i=0;i<3;++i)expected[i]=v[i]*dot(v,b)/dot(v,v);const double f=dot(a[0],a[0])+dot(a[1],a[1]);coefficients={dot(a[0],b)/f,dot(a[1],b)/f};}
   for(unsigned i=0;i<3;++i)near(q.projected[i],expected[i],3e-15,"independent geometric projection");for(unsigned i=0;i<2;++i)near(q.solution[i],coefficients[i],4e-15,"independent minimum-norm coefficients");
   for(X trial:std::array<X,3>{{{-1,2},{0,0},{.5,-.25}}}){const auto value=evaluateQrTrial(a,b,trial);const auto fitted=apply(a,trial);V diff{};for(unsigned i=0;i<3;++i){near(value.fitted[i],fitted[i],0,"trial fitted point");diff[i]=fitted[i]-q.projected[i];}near(value.squaredError,q.residualSquared+dot(diff,diff),2e-13,"orthogonal Pythagorean loss decomposition");require(value.squaredError>=q.residualSquared-2e-14,"trial beats least-squares solution");}
   auto swapped=a;std::swap(swapped[0],swapped[1]);const auto other=analyzeQr(swapped,b);for(unsigned i=0;i<3;++i)near(other.projected[i],q.projected[i],3e-15,"column-order projection invariance");for(unsigned i=0;i<2;++i)near(other.solution[i],q.solution[1-i],4e-15,"solution unpermutation");
  }
 }
 const A source{{{2,0,1},{0,1,1}}};const V target{1.5,0,3};const auto base=analyzeQr(source,target);near(base.solution[0],1,5e-16,"planted x0");near(base.solution[1],1,5e-16,"planted x1");near(base.residualSquared,2.25,1e-15,"planted residual");
 for(double scale:{1e-100,1e-12,.01,1.}){A a=source;for(auto& v:a)for(double& x:v)x*=scale;const auto q=analyzeQr(a,target);certificate(a,target,q);require(q.rank==2,"scale-invariant rank");for(unsigned i=0;i<2;++i)near(q.solution[i]*scale,1,8e-16,"solution inverse scaling");}
 for(double delta:{1e-3,1e-6,1e-9,1e-11,0.}){A a{{{2,0,0},{1,delta,0}}};const V b{1,1,1};const auto q=analyzeQr(a,b);certificate(a,b,q,false);require(q.rank==(delta>2e-10?2U:1U),"declared numerical rank threshold");near(q.residualSquared,delta>2e-10?1:2,1e-14,"rank-aware projection");}
 for(unsigned n=0;n<128;++n){const double angle=n*.071,c=std::cos(angle),s=std::sin(angle);A a=source;V b=target;for(auto& v:a){const double x=v[0],y=v[1];v[0]=c*x-s*y;v[1]=s*x+c*y;}b={c*target[0]-s*target[1],s*target[0]+c*target[1],target[2]};const auto q=analyzeQr(a,b);certificate(a,b,q);near(q.residualSquared,base.residualSquared,3e-15,"orthogonal coordinate invariance");}
 for(double bad:{5.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){A a=source;a[0][0]=bad;bool caught=false;try{analyzeQr(a,target);}catch(const std::invalid_argument&){caught=true;}require(caught,"invalid matrix accepted");}
 for(auto action:{0,1,2}){bool caught=false;try{if(action==0)analyzeQr(A{{{1e-101,0,0},{0,0,0}}},{});if(action==1)analyzeQr(source,{0,5,0});if(action==2)evaluateQrTrial(source,target,{1e121,0});}catch(const std::invalid_argument&){caught=true;}require(caught,"out-of-contract request accepted");}
}
void act(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double v){act(m,{MathActionKind::SetParameter,{},p,v});}
void level(MathObjects& m,unsigned v){act(m,{MathActionKind::SetLevel,{},{},static_cast<double>(v)});}
void preset(MathObjects& m,unsigned v){act(m,{MathActionKind::ObjectPreset,{},{},0,v});}
double metric(const MathObjects& m,std::string_view name){for(unsigned i=0;i<m.snapshot().metricCount;++i)if(m.snapshot().metrics[i].label==name)return m.snapshot().metrics[i].value;throw std::runtime_error("missing metric "+std::string(name));}
void inspect(MathObjects& m,MathObjectScene& scene){
 const auto& s=m.snapshot();const auto& frame=scene.publish(s,{0,0,800,600});++scenes;maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"scene capacity");require(iggy3d::isFinite(frame.clipFromWorld),"finite camera");
 std::set<unsigned> ids;unsigned end=0;for(auto& d:frame.draws){require(ids.insert(d.objectId.value).second&&d.firstIndex==end,"draw ownership and coverage");end+=d.indexCount;}require(end==frame.indices.size(),"draw end");for(auto& v:frame.vertices){for(float x:v.position)require(std::isfinite(x),"finite vertex");for(float x:v.color)require(std::isfinite(x)&&x>=0&&x<=1,"finite color");}for(auto i:frame.indices)require(i<frame.vertices.size(),"scene index");
 for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"finite metric");for(unsigned i=0;i<s.plotCount;++i)for(unsigned j=0;j<s.plots[i].seriesCount;++j)for(unsigned k=0;k<s.plots[i].series[j].count;++k){const auto p=s.plots[i].series[j].points[k];require(std::isfinite(p.x)&&std::isfinite(p.y),"finite plot");if(s.plots[i].scrubParameter!=P::Count){const auto& spec=mathParameterSpecs()[static_cast<unsigned>(s.plots[i].scrubParameter)];require(p.x>=spec.minimum&&p.x<=spec.maximum,"graph scrub exceeds control range");}}
 A a{};V b{};for(unsigned j=0;j<3;++j)for(unsigned i=0;i<3;++i){const double x=m.parameter(static_cast<P>(static_cast<unsigned>(P::QrA0X)+3*j+i));if(j<2)a[j][i]=x;else b[i]=x;}const auto q=analyzeQr(a,b);near(metric(m,"Numerical rank"),q.rank,0,"scene numerical rank");near(metric(m,"Relative QR residual"),q.reconstructionError,0,"scene residual");
 require(s.matrixCount==3&&s.matrices[0].rows==3&&s.matrices[0].columns==2&&s.matrices[1].rows==3&&s.matrices[1].columns==2&&s.matrices[2].rows==2&&s.matrices[2].columns==2,"rectangular matrix shapes");for(unsigned i=0;i<3;++i)for(unsigned j=0;j<2;++j){near(s.matrices[0].values[2*i+j],a[q.order[j]][i],0,"pivoted matrix ordering");near(s.matrices[1].values[2*i+j],q.q[j][i],0,"Q packing");}for(unsigned i=0;i<4;++i)near(s.matrices[2].values[i],q.r[i],0,"R packing");
 require(s.curve.active&&s.curve.count==3&&s.curve.selectionParameter==P::QrVector,"endpoint picker contract");const auto rows=mathControlRows(m);unsigned coordinates=0;for(unsigned i=0;i<rows.count;++i)for(unsigned j=0;j<rows.rows[i].count;++j){auto p=rows.rows[i].parameters[j];if(p>=P::QrA0X&&p<=P::QrBZ){++coordinates;require((static_cast<unsigned>(p)-static_cast<unsigned>(P::QrA0X))/3==static_cast<unsigned>(m.parameter(P::QrVector)),"selected vector controls");}}require(coordinates==3,"three selected coordinates");
 require(m.parameterAvailable(P::QrStage)==(s.level==0),"stage availability");require(m.parameterAvailable(P::QrNull0)==(s.level==3&&q.rank<2),"null0 availability");require(m.parameterAvailable(P::QrNull1)==(s.level==3&&q.rank==0),"null1 availability");
 if(s.level==3){require(s.surface.rows==21&&s.surface.columns==21,"error surface grid");const double height=metric(m,"Error surface height scale");for(unsigned i=0;i<21;++i)for(unsigned j=0;j<21;++j){const auto v=s.surface.vertices[i*21+j];near(v.position.y,height*evaluateQrTrial(a,{},X{-3+.3*j,-3+.3*i}).squaredError,3e-7,"error surface height");require(iggy3d::isFinite(v.normal),"surface normal");}}
}
void integration(){
 require(static_cast<unsigned>(K::Polar)==33&&static_cast<unsigned>(K::Qr)==34,"stable appended object IDs");require(mathObjectSpecs().size()==static_cast<unsigned>(K::Count),"registry coverage");MathObjects m;MathObjectScene scene;act(m,{MathActionKind::Select,K::Qr});require(mathLessons(K::Qr).size()==4,"four layers");
 for(unsigned l=0;l<4;++l){level(m,l);for(unsigned p=0;p<7;++p){preset(m,p);for(unsigned selection=0;selection<3;++selection){set(m,P::QrVector,selection);for(unsigned guide=0;guide<2;++guide){set(m,P::QrGuides,guide);inspect(m,scene);}}}}
 level(m,0);for(unsigned p:{0U,3U,5U,6U}){preset(m,p);for(unsigned stage=0;stage<=30;++stage){set(m,P::QrStage,stage/10.);inspect(m,scene);}}
 level(m,2);for(unsigned p=0;p<7;++p){preset(m,p);set(m,P::QrC0,64);set(m,P::QrC1,-64);inspect(m,scene);set(m,P::QrUseSolution,1);inspect(m,scene);}
 level(m,3);for(unsigned p:{3U,4U,5U}){preset(m,p);for(double offset:{-3.,0.,3.}){set(m,P::QrNull0,offset);if(p==5)set(m,P::QrNull1,offset);inspect(m,scene);}}
 level(m,3);for(unsigned i=0;i<9;++i)for(double value:{-3.,0.,3.}){preset(m,0);set(m,static_cast<P>(static_cast<unsigned>(P::QrA0X)+i),value);inspect(m,scene);}
 for(unsigned l=0;l<4;++l){level(m,l);preset(m,l==3?3:0);if(l==0)set(m,P::QrStage,3);if(l==2){set(m,P::QrC0,1);set(m,P::QrC1,1);}if(l==3)set(m,P::QrNull0,1);act(m,{MathActionKind::Check});require(m.snapshot().feedback==MathFeedback::Solved,"challenge pass");if(l==0)set(m,P::QrStage,0);if(l==1)preset(m,1);if(l==2)set(m,P::QrC0,0);if(l==3)set(m,P::QrNull0,0);act(m,{MathActionKind::Check});require(m.snapshot().feedback==MathFeedback::TryAgain,"challenge fail");}
 level(m,0);preset(m,0);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},1});near(m.parameter(P::QrStage),.5,0,"playback rate");set(m,P::QrA0X,1);require(!m.snapshot().playing,"editing pauses playback");set(m,P::QrStage,2.9);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},2});near(m.parameter(P::QrStage),3,0,"playback bounded end");require(!m.snapshot().playing,"playback endpoint stops");
 const auto rev=m.snapshot().revision;require(!m.dispatch({MathActionKind::SetParameter,{},P::QrA0X,std::numeric_limits<double>::quiet_NaN()}).accepted&&m.snapshot().revision==rev,"invalid mutation atomicity");
 set(m,P::QrA0X,2.5);act(m,mathResetControlGroup(m,MathControlGroup::Shape));near(m.parameter(P::QrA0X),2,0,"group reset");set(m,P::QrA0X,1.5);level(m,3);level(m,0);near(m.parameter(P::QrA0X),1.5,0,"retained column state");
}
}
int main(){try{mathematics();integration();std::printf("qr: %u assertions, %u matrix cases, %u scenes; max vertices=%zu indices=%zu; max relative QR residual=%.3e, active orthogonality=%.3e\n",checks,cases,scenes,maxVertices,maxIndices,maxFactorError,maxOrthogonality);return 0;}catch(const std::exception& e){std::fprintf(stderr,"QR failure: %s (after %u assertions)\n",e.what(),checks);return 1;}}

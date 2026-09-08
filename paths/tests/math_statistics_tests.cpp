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
unsigned binomialCases=0,bayesCases=0,cloudCases=0;
void binomial() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Binomial,3);
  for(unsigned n=1;n<=12;++n)for(unsigned percent=0;percent<=20;++percent) {
    const double p=percent/20.0;set(m,MathParameter::BinomialTrials,n);set(m,MathParameter::BinomialChance,p);++binomialCases;
    long double choose=1,total=0,mean=0,second=0;const auto table=m.snapshot().table;
    for(unsigned k=0;k<=n;++k){const long double mass=choose*std::pow(static_cast<long double>(p),k)*std::pow(static_cast<long double>(1-p),n-k);near(table.values[k][0],static_cast<double>(mass),2e-15,"independent binomial formula");total+=mass;mean+=k*mass;second+=k*k*mass;if(k<n)choose=choose*(n-k)/(k+1);}
    near(metric(m,"Probability sum"),1,2e-15,"PMF conservation");near(metric(m,"Mean"),static_cast<double>(mean),1e-12,"binomial mean");near(metric(m,"Variance"),static_cast<double>(second-mean*mean),2e-12,"binomial variance");near(static_cast<double>(total),1,2e-15,"formula mass sum");
    for(unsigned k=0;k<=12;++k){set(m,MathParameter::BinomialCut,k);double below=0,above=0;for(unsigned j=0;j<=n;++j)(j<=k?below:above)+=table.values[j][0];near(metric(m,"CDF P(X<=k)"),below,2e-15,"CDF sum");near(metric(m,"Tail P(X>k)"),above,2e-15,"tail sum");near(below+above,1,2e-15,"tail complement");}
    require((metric(m,"Normal approximation defined")==1)==(p>0&&p<1),"degenerate approximation");
    if(p>0&&p<1){const double mu=n*p,sd=std::sqrt(n*p*(1-p));const auto phi=[&](double x){return .5*(1+std::erf((x-mu)/(sd*std::sqrt(2.0))));};double maxError=phi(-.5),sum=0;for(unsigned k=0;k<=n;++k){sum+=table.values[k][0];near(table.values[k][2],phi(k+.5)-phi(double(k)-.5),1e-14,"normal bin mass");maxError=std::max(maxError,std::fabs(sum-phi(k+.5)));}near(metric(m,"Maximum CDF error"),maxError,2e-15,"normal CDF error");near(metric(m,"Normal mass outside support"),phi(-.5)+1-phi(n+.5),2e-15,"normal tail leakage");}
    inspect(m,scene);
  }
  select(m,MathObjectKind::Binomial);checked(m,false);for(unsigned i=0;i<6;++i)action(m,{MathActionKind::BernoulliStep});checked(m,true);reject(m,{MathActionKind::BernoulliStep});
  for(unsigned seed:{0U,11U,65535U})for(double chance:{0.0,.5,1.0}) {set(m,MathParameter::BinomialTrials,12);set(m,MathParameter::BinomialChance,chance);set(m,MathParameter::BinomialSeed,seed);for(unsigned i=0;i<12;++i)action(m,{MathActionKind::BernoulliStep});const auto path=m.snapshot().bernoulliPath;for(unsigned i=1;i<13;++i)require(path[i]==path[i-1]||path[i]==path[i-1]+1,"non-Bernoulli path increment");if(chance==0)require(path[12]==0,"zero chance success");if(chance==1)require(path[12]==12,"certain event failed");action(m,{MathActionKind::ResetBernoulli});for(unsigned i=0;i<12;++i)action(m,{MathActionKind::BernoulliStep});require(path==m.snapshot().bernoulliPath,"trial replay differs");inspect(m,scene);}
  set(m,MathParameter::BinomialChance,.5);require(m.snapshot().bernoulliSteps==0,"changed setup retained trials");level(m,1);set(m,MathParameter::BinomialTrials,6);set(m,MathParameter::BinomialCut,3);checked(m,true);level(m,2);set(m,MathParameter::BinomialCut,5);checked(m,true);set(m,MathParameter::BinomialCut,12);checked(m,false);level(m,3);set(m,MathParameter::BinomialTrials,12);checked(m,true);set(m,MathParameter::BinomialChance,0);checked(m,false);reject(m,{MathActionKind::BernoulliStep});
}
void bayes() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Bayes,2);
  for(double prior:{0.0,.05,.3,.5,.95,1.0})for(double hit:{0.0,.05,.5,.95,1.0})for(double other:{0.0,.05,.5,.95,1.0})for(unsigned event=0;event<2;++event) {
    ++bayesCases;set(m,MathParameter::BayesPrior,prior);set(m,MathParameter::BayesHit,hit);set(m,MathParameter::BayesFalse,other);set(m,MathParameter::BayesEvent,event);const double h=event?1-hit:hit,o=event?1-other:other,joint=prior*h,alternative=(1-prior)*o,evidence=joint+alternative;
    near(metric(m,"Evidence probability"),evidence,2e-15,"total probability");near(metric(m,"Joint total mass"),1,2e-15,"joint partition");require((metric(m,"Conditioning defined")==1)==(evidence>0),"zero evidence conditioning");
    if(evidence>0){near(m.snapshot().table.values[0][3],joint/evidence,2e-15,"Bayes H posterior");near(m.snapshot().table.values[1][3],alternative/evidence,2e-15,"Bayes alternative posterior");near(m.snapshot().table.values[0][3]+m.snapshot().table.values[1][3],1,2e-15,"posterior normalization");if(other==hit)near(metric(m,"Posterior P(H)"),prior,2e-15,"independent event updated belief");}else require(m.snapshot().table.columnCount==3,"undefined posterior exposed as number");inspect(m,scene);
  }
  level(m,3);
  for(double prior:{0.0,.3,1.0})for(double h:{0.0,.05,.8,1.0})for(double o:{0.0,.2,.95,1.0})for(unsigned yes=0;yes<=8;++yes)for(unsigned no=0;no<=8;++no) {
    ++bayesCases;set(m,MathParameter::BayesPrior,prior);set(m,MathParameter::BayesHit,h);set(m,MathParameter::BayesFalse,o);set(m,MathParameter::BayesPositive,yes);set(m,MathParameter::BayesNegative,no);
    long double left=prior,right=1-prior;for(unsigned i=0;i<no;++i){left*=1-h;right*=1-o;}for(unsigned i=0;i<yes;++i){left*=h;right*=o;}const long double evidence=left+right;
    require((metric(m,"Conditioning defined")==1)==(evidence>0),"sequential impossible evidence");if(evidence>0){near(metric(m,"Posterior P(H)"),static_cast<double>(left/evidence),2e-14,"reverse-order evidence update");near(m.snapshot().table.values[1][3],static_cast<double>(right/evidence),2e-14,"small posterior retained");require(m.snapshot().plots[0].series[0].count==yes+no+1,"valid evidence plot truncated");}else require(!m.snapshot().plots[0].hasMarker,"impossible endpoint marked valid");if((yes==0&&no==0)||(yes==8&&no==8))inspect(m,scene);
  }
  select(m,MathObjectKind::Bayes);set(m,MathParameter::BayesHit,.5);set(m,MathParameter::BayesFalse,.5);checked(m,true);level(m,1);set(m,MathParameter::BayesHit,.8);set(m,MathParameter::BayesFalse,.2);checked(m,true);level(m,2);set(m,MathParameter::BayesHit,.95);set(m,MathParameter::BayesFalse,.05);checked(m,true);level(m,3);checked(m,true);set(m,MathParameter::BayesHit,0);set(m,MathParameter::BayesFalse,0);checked(m,false);require(m.snapshot().plots[0].series[0].count==1,"impossible prefix not stopped");
}
using V=std::array<double,3>;using M=std::array<double,9>;
double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
V times(M a,V b){return {a[0]*b[0]+a[1]*b[1]+a[2]*b[2],a[3]*b[0]+a[4]*b[1]+a[5]*b[2],a[6]*b[0]+a[7]*b[1]+a[8]*b[2]};}
M empirical(const MathValueTable& table,unsigned first){V mean{};for(unsigned i=0;i<8;++i)for(unsigned j=0;j<3;++j)mean[j]+=table.values[first+i][j]/8;M cov{};for(unsigned i=0;i<8;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)cov[3*j+k]+=(table.values[first+i][j]-mean[j])*(table.values[first+i][k]-mean[k])/8;return cov;}
void covariance() {
  MathObjects m;MathObjectScene scene;select(m,MathObjectKind::Covariance,2);
  for(double sx:{0.0,.1,1.0,2.0})for(double sy:{0.0,.1,1.0,2.0})for(double sz:{0.0,.1,1.0,2.0})for(double shear:{-1.0,0.0,1.0})for(double yaw:{-90.0,0.0,45.0,180.0})for(double pitch:{-90.0,0.0,35.0}) {
    ++cloudCases;set(m,MathParameter::CloudX,sx);set(m,MathParameter::CloudY,sy);set(m,MathParameter::CloudZ,sz);set(m,MathParameter::CloudShear,shear);set(m,MathParameter::CloudYaw,yaw);set(m,MathParameter::CloudPitch,pitch);const auto& s=m.snapshot();const auto cov=empirical(s.table,0),q=s.matrices[1].values,lambda=s.matrices[2].values;
    for(unsigned i=0;i<9;++i)near(s.matrices[0].values[i],cov[i],1e-12,"covariance from displayed data");near(metric(m,"Total variance"),sx*sx+sy*sy*(1+shear*shear)+sz*sz,1e-11,"trace rotation invariant");require(metric(m,"Covariance rank")==double((sx>0)+(sy>0)+(sz>0)),"rank from independent stretch directions");
    require(lambda[0]>=lambda[4]&&lambda[4]>=lambda[8]&&lambda[8]>=0,"eigenvalue ordering");
    for(unsigned j=0;j<3;++j){const V axis{q[j],q[3+j],q[6+j]},mapped=times(cov,axis);for(unsigned i=0;i<3;++i)near(mapped[i],lambda[4*j]*axis[i],1e-10,"covariance eigenvector");for(unsigned k=0;k<3;++k)near(dot(axis,{q[k],q[3+k],q[6+k]}),j==k?1:0,1e-10,"principal orthogonality");double projected=0;for(unsigned point=0;point<8;++point){V delta{};for(unsigned i=0;i<3;++i)delta[i]=s.table.values[point][i]-s.table.values[8][i];const double z=dot(axis,delta);projected+=z*z/8;}near(projected,lambda[4*j],1e-10,"variance of principal projections");}
    if(pitch==35&&yaw==45)inspect(m,scene);
  }
  const auto saved=m.snapshot().matrices[0].values;set(m,MathParameter::CloudMeanX,.5);set(m,MathParameter::CloudMeanY,-.5);set(m,MathParameter::CloudMeanZ,.3);for(unsigned i=0;i<9;++i)near(m.snapshot().matrices[0].values[i],saved[i],1e-12,"translation changed covariance");
  level(m,3);set(m,MathParameter::CloudMeanZ,0);
  for(double sx:{.1,.7,2.0})for(double sy:{.1,1.0,2.0})for(double sz:{.1,.4,2.0})for(double shear:{-1.0,0.0,1.0})for(double yaw:{-180.0,0.0,65.0})for(double amount:{0.0,.5,1.0}) {
    set(m,MathParameter::CloudX,sx);set(m,MathParameter::CloudY,sy);set(m,MathParameter::CloudZ,sz);set(m,MathParameter::CloudShear,shear);set(m,MathParameter::CloudYaw,yaw);set(m,MathParameter::CloudWhiten,amount);require(metric(m,"Whitening defined")==1,"invertible cloud denied whitening");const auto& s=m.snapshot();const auto cov=empirical(s.table,9);for(unsigned i=0;i<9;++i)near(s.matrices[1].values[i],cov[i],2e-12,"transformed covariance from points");if(amount==1){near(metric(m,"Identity covariance error"),0,1e-9,"identity whitening");for(unsigned i=0;i<9;++i)near(cov[i],i%4==0?1:0,1e-9,"whitened covariance");}inspect(m,scene);
  }
  checked(m,true);set(m,MathParameter::CloudZ,0);require(metric(m,"Whitening defined")==0&&m.snapshot().matrixCount==1&&m.snapshot().table.rowCount==9,"singular whitening fabricated output");reject(m,{MathActionKind::SetParameter,{},MathParameter::CloudWhiten,1});checked(m,false);inspect(m,scene);
  select(m,MathObjectKind::Covariance);set(m,MathParameter::CloudMeanX,.5);set(m,MathParameter::CloudMeanY,-.5);checked(m,true);level(m,1);set(m,MathParameter::CloudX,1);set(m,MathParameter::CloudY,1);set(m,MathParameter::CloudYaw,0);set(m,MathParameter::CloudPitch,0);set(m,MathParameter::CloudShear,1);checked(m,true);set(m,MathParameter::CloudX,0);set(m,MathParameter::CloudY,0);require(metric(m,"Correlation defined")==0,"zero-variance correlation defined");checked(m,false);level(m,2);set(m,MathParameter::CloudX,1);set(m,MathParameter::CloudY,1);set(m,MathParameter::CloudZ,0);checked(m,true);
}
void boundaries() {
  MathObjects m;MathObjectScene scene;require(mathObjectSpecs().size()==22,"wrong object count");std::set<std::string_view> keys;for(const auto& p:mathParameterSpecs())require(keys.insert(p.key).second,"duplicate CLI key");
  for(const auto kind:{MathObjectKind::Binomial,MathObjectKind::Bayes,MathObjectKind::Covariance})for(unsigned l=0;l<4;++l){select(m,kind,l);require(mathLessons(kind).size()==4,"missing layer");inspect(m,scene);for(unsigned pass=0;pass<2;++pass)for(const auto& p:mathParameterSpecs())if(m.parameterAvailable(p.id)){set(m,p.id,pass?p.maximum:p.minimum);inspect(m,scene);const auto path=m.snapshot().bernoulliPath;const auto steps=m.snapshot().bernoulliSteps;reject(m,{MathActionKind::SetParameter,{},p.id,std::numeric_limits<double>::quiet_NaN()});require(path==m.snapshot().bernoulliPath&&steps==m.snapshot().bernoulliSteps,"rejection changed trial path");reject(m,{MathActionKind::SetParameter,{},p.id,p.maximum+1});}reject(m,{MathActionKind::SetLevel,{},{},4});}
  select(m,MathObjectKind::Algebra);reject(m,{MathActionKind::BernoulliStep});reject(m,{MathActionKind::ResetBernoulli});
}
}
int main(){try{binomial();bayes();covariance();boundaries();std::printf("Statistics tests passed: %u binomial models, %u Bayesian cases, %u PCA cases, whitening, trial replay, challenges and %zu geometry states; maxima %zu vertices / %zu indices\n",binomialCases,bayesCases,cloudCases,states,maxVertices,maxIndices);return 0;}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}

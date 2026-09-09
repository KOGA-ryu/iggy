#include "runtime/math_objects/Truss.hpp"
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

namespace {
using namespace paths;using P=MathParameter;using K=MathObjectKind;
unsigned checks=0,scenes=0;std::size_t maxVertices=0,maxIndices=0;double maxResidual=0,maxOracleError=0;
void require(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
void near(double a,double b,double tolerance,const char* why){++checks;if(!std::isfinite(a)||!std::isfinite(b)||std::fabs(a-b)>tolerance){char text[256];std::snprintf(text,sizeof(text),"%s: %.17g != %.17g (error %.3g, tolerance %.3g)",why,a,b,std::fabs(a-b),tolerance);throw std::runtime_error(text);}}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}require(caught,"invalid request accepted");}
// Independent displacement/stiffness formulation: unit axial stiffness, fixed
// constrained coordinates, Cholesky solve, then axial elongations. Production
// solves the full force equilibrium matrix and never builds this stiffness.
std::array<double,9> stiffnessOracle(const TrussAnalysis& a,const TrussSolution& s){
  std::array<bool,12> fixed{};for(unsigned i=0;i<a.supportCount;++i)fixed[2*a.restraints[i].node+a.restraints[i].axis]=true;
  std::array<unsigned,12> free{};unsigned count=0;for(unsigned i=0;i<12;++i)if(!fixed[i])free[count++]=i;
  std::array<double,144> stiffness{},lower{};std::array<std::array<double,12>,9> extension{};
  for(unsigned k=0;k<9;++k)if(a.bars[k].active){const auto& b=a.bars[k];const double dx=a.points[b.b][0]-a.points[b.a][0],dy=a.points[b.b][1]-a.points[b.a][1],length=std::hypot(dx,dy);extension[k][2*b.a]=-dx/length;extension[k][2*b.a+1]=-dy/length;extension[k][2*b.b]=dx/length;extension[k][2*b.b+1]=dy/length;for(unsigned i=0;i<count;++i)for(unsigned j=0;j<count;++j)stiffness[12*i+j]+=extension[k][free[i]]*extension[k][free[j]];}
  for(unsigned i=0;i<count;++i)for(unsigned j=0;j<=i;++j){double value=stiffness[12*i+j];for(unsigned k=0;k<j;++k)value-=lower[12*i+k]*lower[12*j+k];if(i==j){require(value>0,"oracle stiffness not positive definite");lower[12*i+j]=std::sqrt(value);}else lower[12*i+j]=value/lower[12*j+j];}
  std::array<double,12> y{},u{};for(unsigned i=0;i<count;++i){y[i]=s.applied[free[i]/2][free[i]%2];for(unsigned j=0;j<i;++j)y[i]-=lower[12*i+j]*y[j];y[i]/=lower[12*i+i];}
  for(unsigned i=count;i-->0;){double value=y[i];for(unsigned j=i+1;j<count;++j)value-=lower[12*j+i]*u[free[j]];u[free[i]]=value/lower[12*i+i];}
  std::array<double,9> force{};for(unsigned i=0;i<9;++i)for(unsigned j=0;j<12;++j)force[i]+=extension[i][j]*u[j];return force;
}
void equilibrium(const TrussAnalysis& a,double position,bool oracle=true){
  const auto s=sampleTruss(a,position);require(s.forcesAvailable,"determinate force solution missing");
  TrussVector force{};double moment=0;for(unsigned i=0;i<6;++i){force[0]+=s.applied[i][0];force[1]+=s.applied[i][1];moment+=a.points[i][0]*s.applied[i][1]-a.points[i][1]*s.applied[i][0];}
  for(unsigned axis=0;axis<2;++axis)near(force[axis],a.input.load[axis],2e-14,"transferred resultant");near(moment,s.loadPoint[0]*a.input.load[1]-s.loadPoint[1]*a.input.load[0],3e-14,"transferred load moment");
  for(unsigned i=0;i<6;++i){TrussVector sum=s.applied[i];for(unsigned k=0;k<9;++k){const auto& bar=a.bars[k];if(!bar.active)continue;unsigned other=6;if(bar.a==i)other=bar.b;if(bar.b==i)other=bar.a;if(other==6)continue;const double dx=a.points[other][0]-a.points[i][0],dy=a.points[other][1]-a.points[i][1],length=std::hypot(dx,dy);sum[0]+=s.forces[k]*dx/length;sum[1]+=s.forces[k]*dy/length;}
    for(unsigned k=0;k<a.supportCount;++k)if(a.restraints[k].node==i)sum[a.restraints[k].axis]+=s.reactions[k];near(sum[0],0,2e-10,"independent joint x balance");near(sum[1],0,2e-10,"independent joint y balance");}
  near(s.resultant[0],0,2e-10,"global force x");near(s.resultant[1],0,2e-10,"global force y");near(s.moment,0,1e-9,"global moment");maxResidual=std::max(maxResidual,s.residual);
  require(a.rank==12&&a.unknowns==12&&a.reciprocalCondition>0&&a.reciprocalCondition<=1,"unique-system diagnostics");
  if(oracle){const auto expected=stiffnessOracle(a,s);for(unsigned i=0;i<9;++i){maxOracleError=std::max(maxOracleError,std::fabs(s.forces[i]-expected[i]));near(s.forces[i],expected[i],2e-8*std::max(1.,std::fabs(s.forces[i])),"independent stiffness force");}}
  const auto sweep=inspectTrussSweep(a);require(sweep.available&&s.maxUtilization<=sweep.maxUtilization+1e-10,"load sweep maximum is too small");const auto peak=sampleTruss(a,sweep.position);near(peak.utilization[sweep.member],sweep.maxUtilization,0,"worst-sweep witness");
}
void physics(){
  for(unsigned kind=0;kind<4;++kind){TrussInput p;p.shape=static_cast<TrussShape>(kind);p.span=4.2;p.height=1.4;const auto a=prepareTruss(p);require(a.status==TrussStatus::Determinate,"default topology not determinate");
    for(double t:{0.,.13,.25,.5,.81,1.}){equilibrium(a,t);const auto s=sampleTruss(a,t);
      if(kind!=2){near(s.reactions[0],0,1e-13,"floor horizontal reaction");near(s.reactions[2],s.loadPoint[0]/p.span,1e-13,"floor moment reaction");near(s.reactions[1],1-s.loadPoint[0]/p.span,1e-13,"floor vertical reaction");}
      else{near(s.reactions[2],-s.loadPoint[0]/p.height,1e-13,"wall moment reaction");near(s.reactions[0],s.loadPoint[0]/p.height,1e-13,"wall horizontal pair");near(s.reactions[1],1,1e-13,"wall vertical reaction");}
    }
    if(kind==0||kind==3){const auto s=sampleTruss(a,.5);const double compression=-std::hypot(p.span/2,p.height)/(2*p.height),tension=p.span/(4*p.height);for(unsigned i=0;i<2;++i)near(s.forces[i],tension,1e-13,"triangle tie force");for(unsigned i=2;i<6;++i)near(s.forces[i],compression,1e-13,"triangle slope force");for(unsigned i=6;i<9;++i)near(s.forces[i],0,1e-13,"triangle zero-force internal member");}
    // Both signs of each independent load, superposition and scaling.
    auto x=p,y=p,sum=p;x.load={1,0};y.load={0,-1};sum.load={2,-3};const auto sx=sampleTruss(prepareTruss(x),.43),sy=sampleTruss(prepareTruss(y),.43),ss=sampleTruss(prepareTruss(sum),.43);for(unsigned i=0;i<9;++i)near(ss.forces[i],2*sx.forces[i]+3*sy.forces[i],1e-12,"load superposition");
    auto scaled=sum;scaled.span*=.75;scaled.height*=.75;const auto sc=sampleTruss(prepareTruss(scaled),.43);for(unsigned i=0;i<9;++i)near(sc.forces[i],ss.forces[i],1e-12,"uniform size force invariance");
    auto translated=sum;for(auto& offset:translated.offsets)offset={.3,-.2};const auto st=sampleTruss(prepareTruss(translated),.43);for(unsigned i=0;i<9;++i)near(st.forces[i],ss.forces[i],1e-12,"rigid translation force invariance");
    auto limits=p;limits.tensionLimit=limits.compressionLimit=.5;const auto capped=sampleTruss(prepareTruss(limits),.5),original=sampleTruss(a,.5);for(unsigned i=0;i<9;++i)near(capped.forces[i],original.forces[i],0,"force limits changed equilibrium");near(capped.maxUtilization,original.maxUtilization*4,1e-14,"force-cap scaling");
    // Losing a member exposes a mechanism even for compatible or zero load.
    p.braced=false;auto broken=prepareTruss(p);require(broken.rank==11&&broken.status==TrussStatus::Mechanism,"removed member not a mechanism");auto absent=sampleTruss(broken,.5);require(!absent.forcesAvailable&&!inspectTrussSweep(broken).available,"mechanism claimed force result");for(double f:absent.forces)near(f,0,0,"mechanism published arbitrary force");
    p.load={};broken=prepareTruss(p);absent=sampleTruss(broken,.5);require(absent.loadCompatible&&!absent.forcesAvailable,"zero-load mechanism confused with stable structure");
    p.braced=true;p.supports=TrussSupportMode::Released;auto released=prepareTruss(p);require(released.rank==11&&released.status==TrussStatus::Mechanism,"released support not a mechanism");
    p.supports=TrussSupportMode::Extra;auto extra=prepareTruss(p);require(extra.rank==12&&extra.unknowns==13&&extra.status==TrussStatus::Indeterminate,"extra support not indeterminate");require(sampleTruss(extra,.5).loadCompatible&&!sampleTruss(extra,.5).forcesAvailable,"indeterminate force invented");
    p.braced=false;extra=prepareTruss(p);if(kind==2){require(extra.status==TrussStatus::Determinate&&extra.supportCount==4,"crane fourth support did not replace missing member");p.load={.7,-1};equilibrium(prepareTruss(p),.6);}else require(extra.status==TrussStatus::Mechanism&&extra.unknowns-extra.rank==1,"combined mechanism/self-stress missing");
    p={};p.shape=static_cast<TrussShape>(kind);p.height=0;auto flat=prepareTruss(p);require(flat.status==TrussStatus::Degenerate&&!sampleTruss(flat,.5).forcesAvailable,"coincident joints accepted");
  }
  // Determinate geometry corners and interior offsets, including high aspect ratio.
  for(unsigned kind=0;kind<4;++kind)for(double w:{1.,5.})for(double h:{.05,3.})for(double lean:{-.15,.15}){TrussInput p;p.shape=static_cast<TrussShape>(kind);p.span=w;p.height=h;p.lean=lean;p.load={5,-5};auto a=prepareTruss(p);equilibrium(a,.31);equilibrium(a,.82,false);}
  for(unsigned i=0;i<40;++i){TrussInput p;p.shape=static_cast<TrussShape>(i%4);p.span=2.5;p.height=1.8;p.load={.4,-1.3};for(unsigned j=0;j<6;++j)p.offsets[j]={.3*std::sin(i*1.3+j*2.1),.3*std::cos(i*.7+j*1.9)};const auto a=prepareTruss(p);require(a.status==TrussStatus::Determinate,"perturbed fixture singular");equilibrium(a,.37);const auto sweep=inspectTrussSweep(a);for(unsigned k=0;k<=128;++k)require(sampleTruss(a,k/128.).maxUtilization<=sweep.maxUtilization+1e-10,"sweep missed an interior maximum");}
  TrussInput p;p.shape=TrussShape::Bridge;p.span=4.2;p.height=.7;p.load={0,-2};require(inspectTrussSweep(prepareTruss(p)).maxUtilization>1.4,"shallow bridge limit demand");p.height=1.4;near(inspectTrussSweep(prepareTruss(p)).maxUtilization,1,1e-14,"raised bridge full-path limit");
  // Moving a joint can create an actual geometric singularity at positive height.
  p={};p.span=1;p.height=.5;p.offsets[2]={0,-.5};require(prepareTruss(p).status==TrussStatus::Degenerate,"offset collision missed");
  for(double value:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()})for(unsigned field=0;field<20;++field){auto invalid=TrussInput{};switch(field){case 0:invalid.span=value;break;case 1:invalid.height=value;break;case 2:invalid.lean=value;break;case 3:invalid.position=value;break;case 4:invalid.load[0]=value;break;case 5:invalid.load[1]=value;break;case 6:invalid.tensionLimit=value;break;case 7:invalid.compressionLimit=value;break;default:invalid.offsets[(field-8)/2][(field-8)%2]=value;break;}rejects([&]{prepareTruss(invalid);});}
  auto invalid=TrussInput{};invalid.shape=TrussShape::Count;rejects([&]{prepareTruss(invalid);});invalid={};invalid.supports=TrussSupportMode::Count;rejects([&]{prepareTruss(invalid);});invalid={};invalid.offsets[0][0]=.51;rejects([&]{prepareTruss(invalid);});const auto a=prepareTruss({});rejects([&]{sampleTruss(a,-.1);});rejects([&]{sampleTruss(a,1.1);});rejects([&]{sampleTruss(a,std::numeric_limits<double>::quiet_NaN());});
}
void act(MathObjects& m,MathAction action){const auto r=m.dispatch(action);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double x){act(m,{MathActionKind::SetParameter,{},p,x});}
void preset(MathObjects& m,unsigned n){act(m,{MathActionKind::ObjectPreset,{},{},0,n});}
void level(MathObjects& m,unsigned n){act(m,{MathActionKind::SetLevel,{},{},static_cast<double>(n)});}
void challenge(MathObjects& m,bool pass){act(m,{MathActionKind::Check});require((m.snapshot().feedback==MathFeedback::Solved)==pass,"incorrect truss challenge");}
TrussInput input(const MathObjects& m){TrussInput p;p.shape=static_cast<TrussShape>(m.parameter(P::TrussShape));p.span=m.parameter(P::TrussSpan);p.height=m.parameter(P::TrussHeight);p.lean=m.parameter(P::TrussLean);p.position=m.parameter(P::TrussPosition);p.load={m.parameter(P::TrussLoadX),m.parameter(P::TrussLoadY)};p.braced=m.parameter(P::TrussBrace)==1;p.supports=static_cast<TrussSupportMode>(m.parameter(P::TrussSupports));p.tensionLimit=m.parameter(P::TrussTensionLimit);p.compressionLimit=m.parameter(P::TrussCompressionLimit);for(unsigned i=0;i<6;++i)for(unsigned j=0;j<2;++j)p.offsets[i][j]=m.parameter(static_cast<P>(static_cast<unsigned>(P::TrussP0X)+2*i+j));return p;}
void inspect(MathObjects& m,MathObjectScene& scene){
  const auto& s=m.snapshot();const auto& frame=scene.publish(s,{0,0,800,600});++scenes;maxVertices=std::max(maxVertices,frame.vertices.size());maxIndices=std::max(maxIndices,frame.indices.size());require(!frame.vertices.empty()&&frame.vertices.size()<=kSceneVertexCapacity&&frame.indices.size()<=kSceneIndexCapacity,"scene capacity");require(iggy3d::isFinite(frame.clipFromWorld),"finite camera");
  for(const auto& v:frame.vertices){for(float x:v.position)require(std::isfinite(x),"finite vertex");for(float x:v.color)require(std::isfinite(x)&&x>=0&&x<=1,"finite colour");}for(unsigned i:frame.indices)require(i<frame.vertices.size(),"scene index");
  unsigned end=0;std::set<unsigned> ids;for(const auto& draw:frame.draws){require(ids.insert(draw.objectId.value).second&&draw.firstIndex==end,"draw ownership");end+=draw.indexCount;}require(end==frame.indices.size(),"unowned indices");
  const auto analysis=prepareTruss(input(m));const auto solution=sampleTruss(analysis,m.parameter(P::TrussPosition));unsigned members=0,joints=0,reactions=0;
  for(unsigned i=0;i<s.partCount;++i){const auto& part=s.parts[i];if(part.role=="truss_member"){while(!analysis.bars[members].active||analysis.bars[members].length<1e-6)++members;const auto& bar=analysis.bars[members++];near(iggy3d::length(part.y),bar.length,5e-7,"member length geometry");near(part.center.x,(analysis.points[bar.a][0]+analysis.points[bar.b][0])/2-analysis.input.span/2,3e-7,"member center x");near(part.center.y,(analysis.points[bar.a][1]+analysis.points[bar.b][1])/2,3e-7,"member center y");if(!solution.forcesAvailable)near(part.color.x,part.color.y,0.2,"unavailable member colour");}joints+=part.role=="truss_joint";reactions+=part.role=="truss_reaction";}
  require(joints==6,"six joints not published");if(!solution.forcesAvailable)require(reactions==0,"unavailable reaction arrow published");
  for(unsigned i=0;i<s.metricCount;++i)require(std::isfinite(s.metrics[i].value),"finite metric");for(unsigned i=0;i<s.table.rowCount;++i)for(unsigned j=0;j<s.table.columnCount;++j)require(std::isfinite(s.table.values[i][j]),"finite table");
  for(unsigned i=0;i<s.plotCount;++i){const auto& plot=s.plots[i];require(plot.seriesCount<=3,"plot series capacity");for(unsigned j=0;j<plot.seriesCount;++j)for(unsigned k=0;k<plot.series[j].count;++k){const auto p=plot.series[j].points[k];require(std::isfinite(p.x)&&std::isfinite(p.y),"finite plot");if(plot.scrubParameter==P::TrussPosition)require(p.x>=0&&p.x<=1,"influence plot domain");}if(plot.equalAspect){const auto& line=plot.series[0];const auto last=line.points[line.count-1];near(std::hypot(last.x,last.y),0,1e-10,"force polygon did not close");}}
  for(unsigned i=0;i<s.matrixCount;++i)for(double x:s.matrices[i].values)require(std::isfinite(x),"finite matrix");
}
void model(){
  MathObjects m;MathObjectScene scene;act(m,{MathActionKind::Select,K::Truss});require(mathLessons(K::Truss).size()==4&&m.playbackParameter()==P::TrussPosition,"truss registration");std::set<std::string_view> keys;for(const auto& p:mathParameterSpecs())require(keys.insert(p.key).second,"duplicate parameter key");
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned p=0;p<4;++p){const auto revision=m.snapshot().revision;preset(m,p);require(m.snapshot().revision==revision+1,"preset not atomic");const auto& example=mathObjectPresets(K::Truss,l)[p];require(example.count==26,"incomplete preset");for(unsigned i=0;i<example.count;++i)near(m.parameter(example.parameters[i]),example.values[i],0,"preset value");for(double t:{0.,.23,.5,1.}){set(m,P::TrussPosition,t);inspect(m,scene);}for(unsigned support=0;support<3;++support)for(unsigned brace=0;brace<2;++brace){set(m,P::TrussSupports,support);set(m,P::TrussBrace,brace);inspect(m,scene);}set(m,P::TrussGuides,0);inspect(m,scene);require(!m.snapshot().curve.active,"hidden joints still pickable");}}
  level(m,0);preset(m,0);challenge(m,false);set(m,P::TrussLoadX,.5);challenge(m,true);
  level(m,1);preset(m,0);challenge(m,true);set(m,P::TrussMember,0);challenge(m,false);
  level(m,2);preset(m,0);challenge(m,false);preset(m,1);challenge(m,true);set(m,P::TrussJoint,0);challenge(m,false);
  level(m,3);preset(m,1);challenge(m,false);set(m,P::TrussHeight,1.4);challenge(m,true);set(m,P::TrussBrace,0);challenge(m,false);set(m,P::TrussBrace,1);set(m,P::TrussSupports,2);challenge(m,false);set(m,P::TrussSupports,0);set(m,P::TrussHeight,0);challenge(m,false);inspect(m,scene);
  // The selectors expose only one joint's offsets; selecting does not edit it.
  level(m,0);preset(m,0);MathInspectorMemory memory;memory.visit(m);memory.rememberExample(m,"Triangular support");
  for(unsigned selected=0;selected<6;++selected){set(m,P::TrussJoint,selected);const auto rows=mathControlRows(m);unsigned offsets=0;for(unsigned i=0;i<rows.count;++i)for(unsigned j=0;j<rows.rows[i].count;++j){const auto p=rows.rows[i].parameters[j];if(p>=P::TrussP0X&&p<=P::TrussP5Y){require((static_cast<unsigned>(p)-static_cast<unsigned>(P::TrussP0X))/2==selected,"wrong joint offset visible");++offsets;}}require(offsets==2&&m.snapshot().curve.count==6&&m.snapshot().curve.selected==selected,"selected joint controls/picker");require(memory.exampleTitle(m)=="Triangular support","joint inspection changed example name");}
  set(m,P::TrussP0X,.2);set(m,P::TrussP5Y,-.1);require(memory.exampleTitle(m)=="Custom","offset did not customize example");act(m,mathResetControlGroup(m,MathControlGroup::Profile));require(mathChangedControlCount(m,MathControlGroup::Profile)==0,"profile reset missed hidden offsets");
  set(m,P::TrussP3Y,.17);set(m,P::TrussPosition,.23);level(m,3);level(m,0);near(m.parameter(P::TrussP3Y),.17,1e-14,"layer lost geometry");near(m.parameter(P::TrussPosition),.23,1e-14,"layer lost load");
  preset(m,1);set(m,P::TrussPosition,0);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},2});near(m.parameter(P::TrussPosition),.24,1e-14,"load sweep rate");set(m,P::TrussJoint,3);require(!m.snapshot().playing,"edit failed to pause");set(m,P::TrussPosition,.98);act(m,{MathActionKind::TogglePlayback});act(m,{MathActionKind::AdvanceTime,{},{},1});near(m.parameter(P::TrussPosition),1,0,"sweep endpoint");require(!m.snapshot().playing&&!m.dispatch({MathActionKind::TogglePlayback}).accepted,"sweep passed endpoint");
  for(unsigned l=0;l<4;++l){level(m,l);for(unsigned shape=0;shape<4;++shape)for(bool high:{false,true}){preset(m,shape);for(const auto& p:mathParameterSpecs())if(p.owner==K::Truss&&p.id!=P::TrussShape)set(m,p.id,high?p.maximum:p.minimum);inspect(m,scene);}}
  level(m,0);preset(m,2);set(m,P::TrussBrace,0);set(m,P::TrussSupports,2);require(m.snapshot().plotCount==2,"four-reaction plot not split into bounded series");inspect(m,scene);
  const auto revision=m.snapshot().revision;require(!m.dispatch({MathActionKind::SetParameter,{},P::TrussP0X,.6}).accepted&&!m.dispatch({MathActionKind::SetParameter,{},P::RigidTime,1}).accepted&&!m.dispatch({MathActionKind::SetParameter,{},P::TrussLoadX,std::numeric_limits<double>::quiet_NaN()}).accepted,"invalid model action accepted");require(m.snapshot().revision==revision,"rejected edit mutated model");
  act(m,{MathActionKind::Select,K::Curve});require(m.snapshot().curve.count==4,"curve picker regression");act(m,{MathActionKind::Select,K::Patch});require(m.snapshot().curve.count==16,"patch picker regression");act(m,{MathActionKind::Select,K::Algebra});require(!m.snapshot().curve.active&&m.playbackParameter()==P::Count,"truss state leaked");
}
}
int main(){try{physics();model();std::printf("truss CPU checks: %u assertions, %u scene states; maxima %zu vertices / %zu indices; max joint residual %.4g kN / stiffness-oracle difference %.4g kN; no host, fonts or images\n",checks,scenes,maxVertices,maxIndices,maxResidual,maxOracleError);return 0;}catch(const std::exception& e){std::fprintf(stderr,"truss failure: %s\n",e.what());return 1;}}

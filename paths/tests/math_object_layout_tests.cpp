#include "ui/MathObjectLayout.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>

using namespace paths;
namespace {
using P=MathParameter;using G=MathControlGroup;using K=MathObjectKind;
unsigned checks=0,layouts=0,states=0;
void require(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
void act(MathObjects& m,MathAction a){const auto r=m.dispatch(a);if(!r.accepted)throw std::runtime_error(std::string(r.reason));}
void set(MathObjects& m,P p,double v){act(m,{MathActionKind::SetParameter,{},p,v});}
void select(MathObjects& m,K k,unsigned level=0){act(m,{MathActionKind::Select,k});if(level)act(m,{MathActionKind::SetLevel,{},{},static_cast<double>(level)});}
MathAction reset(std::initializer_list<P> params){MathAction a{MathActionKind::ResetParameters};for(auto p:params)a.resetParameters.set(static_cast<unsigned>(p));return a;}
bool inside(SceneViewport a,SceneViewport b){return a.width>0&&a.height>0&&std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.width)&&std::isfinite(a.height)&&a.x>=b.x-.01f&&a.y>=b.y-.01f&&a.x+a.width<=b.x+b.width+.01f&&a.y+a.height<=b.y+b.height+.01f;}
bool overlap(SceneViewport a,SceneViewport b){return a.width>0&&b.width>0&&a.height>0&&b.height>0&&a.x<b.x+b.width-.01f&&b.x<a.x+a.width-.01f&&a.y<b.y+b.height-.01f&&b.y<a.y+a.height-.01f;}
void layoutChecks(){
  for(float w:{320,480,600,800,960,1280,1440,1920,3840})for(float h:{320,600,720,900,2160})for(float scale:{.75f,1.f,1.5f,2.f})for(bool controls:{false,true})for(bool drawer:{false,true})for(float width:{-100.f,340.f,900.f})for(float height:{-100.f,200.f,1000.f}){
    MathLabLayoutRequest r{{13,27,w,h},scale,width,height,controls,drawer};const auto p=planMathLabLayout(r);++layouts;
    require(inside(p.toolbar,r.bounds)&&inside(p.viewport,r.bounds)&&inside(p.viewTools,r.bounds),"core pane escapes bounds");
    const std::array panes{p.toolbar,p.viewTools,p.metrics,p.viewport,p.drawer,p.drawerDivider};
    for(unsigned i=0;i<panes.size();++i)if(panes[i].height>0){if(!inside(panes[i],r.bounds))std::fprintf(stderr,"layout %.0fx%.0f scale %.2f c%d d%d pane%u: %.3f %.3f %.3f %.3f\n",w,h,scale,controls,drawer,i,panes[i].x,panes[i].y,panes[i].width,panes[i].height);require(inside(panes[i],r.bounds),"pane escapes bounds");for(unsigned j=i+1;j<panes.size();++j)require(!overlap(panes[i],panes[j]),"main panes overlap");}
    if(controls){require(inside(p.inspector,r.bounds),"inspector escapes bounds");if(!p.overlayInspector){for(auto pane:panes)require(!overlap(pane,p.inspector)&&!overlap(pane,p.inspectorDivider),"docked inspector overlaps model");require(inside(p.inspectorDivider,r.bounds),"inspector divider escapes bounds");}}
    else require(p.inspector.width==0&&!p.overlayInspector,"hidden inspector reserves space");
    if(!drawer)require(p.drawer.height==0&&p.drawerDivider.height==0,"hidden drawer reserves space");
  }
  const auto desktop=planMathLabLayout({{0,0,1440,900},1,360,200,true,true});require(desktop.toolbarRows==1&&!desktop.overlayInspector&&desktop.viewport.height>=550,"desktop workspace did not reclaim height");
  const auto closed=planMathLabLayout({{0,0,1440,900},1,360,200,false,false});require(closed.viewport.width>desktop.viewport.width&&closed.viewport.height>desktop.viewport.height,"hiding panels did not reclaim model space");
  const auto narrow=planMathLabLayout({{0,0,800,600},1,360,200,true,true});require(narrow.toolbarRows==2&&narrow.overlayInspector,"narrow inspector should overlay rather than shrink the model");
  for(auto bad:{MathLabLayoutRequest{{0,0,0,600}},MathLabLayoutRequest{{0,0,800,600},0},MathLabLayoutRequest{{0,0,800,600},std::numeric_limits<float>::quiet_NaN()}}){bool rejected=false;try{(void)planMathLabLayout(bad);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"invalid bounds accepted");}
}
void controlChecks(){
  MathObjects model;MathInspectorMemory memory;
  for(auto object:mathObjectSpecs()){
    select(model,object.id);const unsigned levels=std::max<std::size_t>(1,mathLessons(object.id).size());
    for(unsigned level=0;level<levels;++level){act(model,{MathActionKind::SetLevel,{},{},static_cast<double>(level)});memory.visit(model);++states;const auto rows=mathControlRows(model);std::bitset<static_cast<unsigned>(P::Count)> seen;bool open=false;
      for(unsigned i=0;i<rows.count;++i){const auto& row=rows.rows[i];require(row.count>=1&&row.count<=3&&!row.label.empty(),"invalid control row");open=open||memory.groupOpen(model,row.group);
        for(unsigned j=0;j<row.count;++j){const auto p=row.parameters[j];require(model.parameterAvailable(p)&&!seen.test(static_cast<unsigned>(p)),"duplicate or unavailable row parameter");seen.set(static_cast<unsigned>(p));require(mathControlMetadata(p).group==row.group,"tuple crosses groups");const auto range=mathControlRange(model,p);require(model.parameter(p)>=range.minimum-1e-9&&model.parameter(p)<=range.maximum+1e-9,"current value outside row range");if(row.count>1)require(!row.components[j].empty(),"tuple lacks component caption");}
      }
      require(rows.count==0||open,"all controls initially hidden");
      for(const auto& p:mathParameterSpecs())if(model.parameterAvailable(p.id)){
        bool expected=!(p.matrixEntry&&level>0);
        if(p.id>=P::TrussP0X&&p.id<=P::TrussP5Y)expected=expected&&(static_cast<unsigned>(p.id)-static_cast<unsigned>(P::TrussP0X))/2==static_cast<unsigned>(model.parameter(P::TrussJoint));
        if(p.id>=P::MembraneM0&&p.id<=P::MembraneV3)expected=expected&&(static_cast<unsigned>(p.id)-static_cast<unsigned>(P::MembraneM0))/4==static_cast<unsigned>(model.parameter(P::MembraneSlot));
        if(p.id>=P::PatchP00X&&p.id<=P::PatchP33Z)expected=expected&&(static_cast<unsigned>(p.id)-static_cast<unsigned>(P::PatchP00X))/3==static_cast<unsigned>(model.parameter(P::PatchControl));
        if(p.id>=P::CurveP0X&&p.id<=P::CurveP3Z)expected=expected&&(static_cast<unsigned>(p.id)-static_cast<unsigned>(P::CurveP0X))/3==static_cast<unsigned>(model.parameter(P::CurveControl));
        if(p.id>=P::LatheR0&&p.id<=P::LatheR6)expected=expected&&static_cast<unsigned>(p.id)-static_cast<unsigned>(P::LatheR0)==static_cast<unsigned>(model.parameter(P::LatheControl));
        if(p.id>=P::LatheH1&&p.id<=P::LatheH5)expected=expected&&static_cast<unsigned>(p.id)-static_cast<unsigned>(P::LatheH1)+1==static_cast<unsigned>(model.parameter(P::LatheControl));
        require(seen.test(static_cast<unsigned>(p.id))==expected,"available control lost from inspector");
      }
    }
    require(matchesMathObject(object.id,object.key),"object key not searchable");require(!matchesMathObject(object.id,"zzzzzznonexistent"),"search returned unrelated object");
  }
  require(matchesMathObject(K::Boolean," BOOL "),"case-insensitive search failed");
  select(model,K::Boolean);memory.visit(model);memory.setGroupOpen(model,G::ShapeA,false);act(model,{MathActionKind::SetLevel,{},{},1});memory.visit(model);memory.setGroupOpen(model,G::ShapeA,true);act(model,{MathActionKind::SetLevel,{},{},0});require(!memory.groupOpen(model,G::ShapeA),"layer collapse state leaked");select(model,K::Lathe);memory.visit(model);select(model,K::Boolean);memory.visit(model);require(!memory.groupOpen(model,G::ShapeA),"object switch forgot collapse state");
  act(model,{MathActionKind::ObjectPreset,{},{},0,0});memory.rememberExample(model,mathObjectPresets(K::Boolean,0)[0].name);const auto name=memory.exampleTitle(model);require(name!="Custom","selected example reported Custom");set(model,P::BooleanSizeA,model.parameter(P::BooleanSizeA)+.1);require(memory.exampleTitle(model)=="Custom","edited example name stayed stale");
  select(model,K::Curve);memory.visit(model);act(model,{MathActionKind::ObjectPreset,{},{},0,0});memory.rememberExample(model,mathObjectPresets(K::Curve,0)[0].name);const auto curve=memory.exampleTitle(model);set(model,P::CurveControl,2);require(memory.exampleTitle(model)==curve,"selecting a handle changed the example name");
  MathInspectorMemory fresh;set(model,P::CurveP0Y,model.parameter(P::CurveP0Y)+.05);fresh.visit(model);require(fresh.exampleTitle(model)=="Custom","preset detection ignored an edited parameter");
  // The shared selection table must retain the older curve/lathe control rules.
  for(auto kind:{K::Curve,K::Lathe}){select(model,kind);const bool lathe=kind==K::Lathe;for(unsigned chosen=0;chosen<(lathe?7U:4U);++chosen){set(model,lathe?P::LatheControl:P::CurveControl,chosen);const auto rows=mathControlRows(model);unsigned coordinates=0,heights=0;for(unsigned i=0;i<rows.count;++i)for(unsigned j=0;j<rows.rows[i].count;++j){const auto p=rows.rows[i].parameters[j];if(lathe&&p>=P::LatheR0&&p<=P::LatheR6){require(static_cast<unsigned>(p)-static_cast<unsigned>(P::LatheR0)==chosen,"lathe selected radius");++coordinates;}if(lathe&&p>=P::LatheH1&&p<=P::LatheH5){require(static_cast<unsigned>(p)-static_cast<unsigned>(P::LatheH1)+1==chosen,"lathe selected height");++heights;}if(!lathe&&p>=P::CurveP0X&&p<=P::CurveP3Z){require((static_cast<unsigned>(p)-static_cast<unsigned>(P::CurveP0X))/3==chosen,"curve selected coordinates");++coordinates;}}require(coordinates==(lathe?1U:3U),"selected coordinate count");require(heights==(lathe&&chosen>0&&chosen<6?1U:0U),"lathe endpoint height rule");}}
}
void resetChecks(){
  MathObjects m;select(m,K::Boolean);set(m,P::BooleanSizeA,1.2);set(m,P::BooleanSizeB,.7);const auto b=m.parameter(P::BooleanSizeB);const auto revision=m.snapshot().revision;
  act(m,mathResetControlGroup(m,G::ShapeA));require(m.snapshot().revision==revision+1&&m.parameter(P::BooleanSizeB)==b,"group reset changed another group or used multiple revisions");require(mathChangedControlCount(m,G::ShapeA)==0,"group reset left modified values");
  const auto unchanged=m.snapshot().revision;require(!m.dispatch(reset({P::BooleanSizeA,P::LatheH1})).accepted&&!m.dispatch(reset({})).accepted,"invalid reset accepted");require(m.snapshot().revision==unchanged&&m.parameter(P::BooleanSizeB)==b,"rejected reset mutated state");
  select(m,K::Lathe);set(m,P::LatheH1,.04);set(m,P::LatheH2,.10);const auto range=mathControlRange(m,P::LatheH1);require(range.maximum<=.0600001,"profile control can cross a neighbour");const auto rev=m.snapshot().revision;
  require(!m.dispatch(reset({P::LatheH1})).accepted&&m.snapshot().revision==rev&&m.parameter(P::LatheH1)==.04,"invalid individual reset was not atomic");act(m,mathResetControlGroup(m,G::Profile));require(mathChangedControlCount(m,G::Profile)==0&&m.snapshot().revision==rev+1,"coupled profile group reset failed");
  select(m,K::Curve,2);set(m,P::CurveProgress,0);act(m,{MathActionKind::TogglePlayback});act(m,reset({P::CurveRadius}));require(!m.snapshot().playing,"reset left playback running");
  select(m,K::Probability);act(m,{MathActionKind::ProbabilityStep});require(m.snapshot().probabilityWalkCount==2,"walk did not advance");act(m,reset({P::ProbabilitySeed}));require(m.snapshot().probabilityWalkCount==1,"seed reset left stale probability history");
  select(m,K::Binomial);act(m,{MathActionKind::BernoulliStep});act(m,reset({P::BinomialSeed}));require(m.snapshot().bernoulliSteps==0,"seed reset left stale trial history");
  select(m,K::Modular,1);act(m,{MathActionKind::ModularStep,{},{},1});act(m,reset({P::ModStep}));require(m.snapshot().modularWalkSteps==0,"step reset left stale modular history");
  select(m,K::Symmetry);act(m,{MathActionKind::SymmetryTurn,{},{},0,0});act(m,reset({P::SymmetryVertex}));require(m.snapshot().symmetry.moveCount==1,"probe reset erased unrelated symmetry moves");
}
}
int main(){try{layoutChecks();controlChecks();resetChecks();std::printf("compact lab CPU checks: %u assertions, %u layouts, %u object/layer states; no host, fonts, or images\n",checks,layouts,states);return 0;}catch(const std::exception& e){std::fprintf(stderr,"compact lab failure: %s\n",e.what());return 1;}}

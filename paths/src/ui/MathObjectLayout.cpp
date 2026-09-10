#include "MathObjectLayout.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <stdexcept>

namespace paths {
namespace {
using P=MathParameter;using G=MathControlGroup;
constexpr unsigned index(P p){return static_cast<unsigned>(p);}
bool selectionControl(P p){
  static const auto selectors=[] {std::bitset<index(P::Count)> result;
    for(const auto& spec:mathParameterSpecs())if(spec.control.selector!=P::Count)result.set(index(spec.control.selector));
    return result;
  }();return selectors.test(index(p));
}
bool visible(const MathObjects& m,P p){
  const auto& state=m.snapshot();const auto& spec=mathParameterSpecs()[index(p)];
  if(!m.parameterAvailable(p)||(spec.matrixEntry&&state.level>0))return false;
  if(spec.control.selector!=P::Count&&m.parameter(spec.control.selector)!=spec.control.selectedValue)return false;
  return true;
}
bool defaultOpen(G group,unsigned level){
  switch(group){case G::Display:return false;case G::Advanced:case G::Sampling:return level>=2;case G::Shape:case G::ShapeA:case G::ShapeB:case G::Profile:return level<=1;default:return true;}
}
bool contains(std::string_view hay,std::string_view needle){
  return std::search(hay.begin(),hay.end(),needle.begin(),needle.end(),[](unsigned char a,unsigned char b){return std::tolower(a)==std::tolower(b);})!=hay.end();
}
}
MathControlMetadata mathControlMetadata(P p){
  if(p>=P::Count)throw std::invalid_argument("unknown control metadata");
  const auto& spec=mathParameterSpecs()[index(p)];
  return {spec.control.group,spec.control.label.empty()?spec.label:spec.control.label};
}
std::string_view mathControlGroupName(G g){constexpr std::array<std::string_view,static_cast<unsigned>(G::Count)> names{"Shape","Shape A","Shape B","Profile","Transform","Operation","Probe","Animation","Sampling","Display","Advanced"};return names.at(static_cast<unsigned>(g));}
MathControlRows mathControlRows(const MathObjects& m){
  MathControlRows out;std::bitset<index(P::Count)> consumed;
  for(const auto& p:mathParameterSpecs())if(visible(m,p.id)&&!consumed[index(p.id)]){
    const auto meta=mathControlMetadata(p.id);MathControlRow row{meta.group,meta.label,{p.id},1};
    const auto& binding=p.control;
    if(binding.rowCount>1){
      bool complete=true;
      for(unsigned j=0;j<binding.rowCount;++j){
        const auto q=static_cast<P>(index(p.id)+j);
        complete=complete&&visible(m,q)&&mathControlMetadata(q).group==meta.group;
      }
      if(complete){
        row.count=binding.rowCount;row.label=binding.rowLabel;row.components=binding.components;
        for(unsigned j=0;j<row.count;++j)row.parameters[j]=static_cast<P>(index(p.id)+j);
      }
    }
    for(unsigned j=0;j<row.count;++j)consumed.set(index(row.parameters[j]));out.rows[out.count++]=row;
  }return out;
}
MathControlRange mathControlRange(const MathObjects& m,P p){
  if(p>=P::Count)throw std::invalid_argument("unknown control range");
  const auto& spec=mathParameterSpecs()[index(p)];MathControlRange range{spec.minimum,m.parameterMaximum(p)};
  if(p>=P::SimplexP0&&p<=P::SimplexQ1){const auto offset=index(p)-index(P::SimplexP0);range.maximum=std::max(0.,1-m.parameter(static_cast<P>(index(P::SimplexP0)+(offset^1U))));}
  if(p>=P::LatheH1&&p<=P::LatheH5){range.minimum=(p==P::LatheH1?0:m.parameter(static_cast<P>(index(p)-1)))+.04;range.maximum=(p==P::LatheH5?1:m.parameter(static_cast<P>(index(p)+1)))-.04;}return range;
}
MathAction mathResetControlGroup(const MathObjects& m,G group){MathAction action{MathActionKind::ResetParameters};for(const auto& p:mathParameterSpecs())if(p.owner==m.snapshot().kind&&mathControlMetadata(p.id).group==group)action.resetParameters.set(index(p.id));return action;}
unsigned mathChangedControlCount(const MathObjects& m,G group){unsigned count=0;for(const auto& p:mathParameterSpecs())if(p.owner==m.snapshot().kind&&mathControlMetadata(p.id).group==group&&std::fabs(m.parameter(p.id)-p.initial)>1e-9)++count;return count;}
bool matchesMathObject(MathObjectKind kind,std::string_view query){
  const auto& spec=mathObjectSpecs()[static_cast<unsigned>(kind)];
  while(!query.empty()){const auto start=query.find_first_not_of(" \t");if(start==query.npos)return true;query.remove_prefix(start);const auto end=query.find_first_of(" \t");const auto word=query.substr(0,end);if(!contains(spec.key,word)&&!contains(spec.name,word)&&!contains(spec.title,word))return false;if(end==query.npos)return true;query.remove_prefix(end);}return true;
}
void MathInspectorMemory::rememberExample(const MathObjects& m,std::string_view name){auto& object=objects[static_cast<unsigned>(m.snapshot().kind)];object.initialized=true;object.exampleName=name;for(const auto& p:mathParameterSpecs())if(p.owner==m.snapshot().kind)object.exampleValues[index(p.id)]=m.parameter(p.id);}
void MathInspectorMemory::visit(const MathObjects& m){
  auto& object=objects[static_cast<unsigned>(m.snapshot().kind)];const auto level=m.snapshot().level;
  if(!object.initialized){std::string_view name="Custom";bool defaults=true;for(const auto& p:mathParameterSpecs())if(p.owner==m.snapshot().kind&&std::fabs(m.parameter(p.id)-p.initial)>1e-9)defaults=false;if(defaults)name="Defaults";
    for(const auto& preset:mathObjectPresets(m.snapshot().kind,level)){bool match=true;for(const auto& p:mathParameterSpecs())if(p.owner==m.snapshot().kind&&!selectionControl(p.id)){double expected=p.initial;for(unsigned i=0;i<preset.count;++i)if(preset.parameters[i]==p.id)expected=preset.values[i];match=match&&std::fabs(m.parameter(p.id)-expected)<1e-9;}if(match){name=preset.name;break;}}rememberExample(m,name);}
  if(!object.visited[level]){object.visited[level]=true;const auto rows=mathControlRows(m);bool any=false;for(unsigned i=0;i<rows.count;++i){const auto group=rows.rows[i].group;const bool open=defaultOpen(group,level);object.open[level].set(static_cast<unsigned>(group),open);any=any||open;}if(!any&&rows.count)object.open[level].set(static_cast<unsigned>(rows.rows[0].group));}
}
std::string_view MathInspectorMemory::exampleTitle(const MathObjects& m) const {const auto& object=objects[static_cast<unsigned>(m.snapshot().kind)];for(const auto& p:mathParameterSpecs())if(p.owner==m.snapshot().kind&&!selectionControl(p.id)&&std::fabs(m.parameter(p.id)-object.exampleValues[index(p.id)])>1e-9)return "Custom";return object.exampleName;}
bool MathInspectorMemory::groupOpen(const MathObjects& m,G group) const {return objects[static_cast<unsigned>(m.snapshot().kind)].open[m.snapshot().level].test(static_cast<unsigned>(group));}
void MathInspectorMemory::setGroupOpen(const MathObjects& m,G group,bool open){objects[static_cast<unsigned>(m.snapshot().kind)].open[m.snapshot().level].set(static_cast<unsigned>(group),open);}
MathLabLayout planMathLabLayout(const MathLabLayoutRequest& r){
  const auto b=r.bounds;
  if(!std::isfinite(b.x)||!std::isfinite(b.y)||!std::isfinite(b.width)||!std::isfinite(b.height)||b.x<0||b.y<0||b.width<320||b.height<320||!std::isfinite(r.scale)||r.scale<.75F||r.scale>2||!std::isfinite(r.inspectorWidth)||!std::isfinite(r.drawerHeight))throw std::invalid_argument("invalid math lab layout");
  const float s=r.scale,gap=6*s,row=32*s;MathLabLayout p;
  p.toolbarRows=b.width>=960*s?1:b.width>=600*s?2:3;
  const float top=p.toolbarRows*row+12*s;
  p.toolbar={b.x,b.y,b.width,top};
  const float bodyY=b.y+top+gap,bodyH=b.height-top-gap;
  const float requested=std::clamp(r.inspectorWidth,280*s,480*s);const bool dock=r.inspector&&b.width>=requested+480*s;
  const float inspector=dock?std::min(requested,b.width*.43F):0;
  p.overlayInspector=r.inspector&&!dock;
  if(r.inspector)p.inspector={b.x+b.width-(dock?inspector:std::min(requested,b.width-2*gap)),bodyY,dock?inspector:std::min(requested,b.width-2*gap),bodyH-gap};
  const float mainWidth=b.width-(dock?inspector+gap:0)-2*gap,mainX=b.x+gap;
  if(dock)p.inspectorDivider={b.x+b.width-inspector-gap,bodyY,gap,bodyH-gap};
  const float tools=30*s;const float metrics=bodyH>140*s?48*s:0;
  p.viewTools={mainX,bodyY,mainWidth,tools};p.metrics={mainX,bodyY+tools,mainWidth,metrics};
  const float remaining=std::max(1.0F,bodyH-tools-metrics-gap);
  const float drawer=r.drawer&&remaining>120*s?std::clamp(r.drawerHeight,std::min(110*s,remaining*.4F),remaining*.48F):0;
  if(drawer>0){p.drawer={mainX,b.y+b.height-gap-drawer,mainWidth,drawer};p.drawerDivider={mainX,p.drawer.y-gap,mainWidth,gap};}
  p.viewport={mainX,bodyY+tools+metrics,mainWidth,std::max(1.0F,remaining-drawer-(drawer>0?gap:0))};return p;
}
} // namespace paths

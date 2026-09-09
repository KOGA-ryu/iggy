#include "MathObjectLayout.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <stdexcept>

namespace paths {
namespace {
using P=MathParameter;using G=MathControlGroup;
constexpr unsigned index(P p){return static_cast<unsigned>(p);}
const auto& metadata(){
  static const auto data=[] {
    std::array<MathControlMetadata,index(P::Count)> result{};
    for(const auto& p:mathParameterSpecs())result[index(p.id)]={p.minimumLevel>=2?G::Advanced:G::Shape,p.label};
    const auto range=[&](P a,P b,G g){for(unsigned i=index(a);i<=index(b);++i)result[i].group=g;};
    const auto group=[&](G g,std::initializer_list<P> params){for(auto p:params)result[index(p)].group=g;};
    group(G::Display,{P::Gap,P::SliceGap,P::Depth,P::GaussianHeight,P::TensorGap,P::NormWire,P::CurveGuides,P::LatheCut,P::LatheGuides,P::BooleanGuides,P::BooleanSection,P::PatchGuides,P::MembraneGuides,P::MembraneView,P::RigidGuides,P::TrussGuides,P::SimplexGuides,P::DistanceGuides});
    group(G::Sampling,{P::Slices,P::Sample,P::DeltaX,P::TaylorDegree,P::HarmonicTerms,P::FluxResolution,P::ProbabilitySeed,P::BinomialSeed,P::LatheSlices,P::LatheMethod,P::BooleanResolution,P::PatchResolution,P::MembraneResolution});
    group(G::Operation,{P::Shortcut,P::IntegralStart,P::ComposeAngle,P::SvdStage,P::DescentRate,P::Constraint,P::GaussianOperation,P::GaussianQuotient,P::FluxOrientation,P::TensorBasis,P::CloudWhiten,P::NormSupport,P::BooleanOperation,P::BooleanBlend});
    range(P::VectorX,P::VectorZ,G::Probe);range(P::SurfaceU,P::DirectionAngle,G::Probe);range(P::FieldX,P::FieldPitch,G::Probe);
    group(G::Probe,{P::Angle,P::FunctionX,P::TaylorCenter,P::CircleAngle,P::SymmetryVertex,P::SymmetryElement,P::ProbeFrequency,P::InverseGuess,P::CrtGuess,P::FluxProbe,P::ProbabilityRow,P::BinomialCut,P::CloudComponent,P::SphereTheta,P::SpherePhi,P::RootIndex,P::PsdProbeAngle,P::LatheProbe});
    range(P::SymmetryFirst,P::SymmetryPower,G::Operation);range(P::TensorI,P::TensorK,G::Probe);range(P::QuadX,P::QuadZ,G::Probe);range(P::NormX,P::NormOtherZ,G::Probe);range(P::BooleanProbeX,P::BooleanProbeZ,G::Probe);
    range(P::BayesEvent,P::BayesNegative,G::Operation);range(P::RootMultiplier,P::RootAutomorphism,G::Operation);range(P::PsdMix,P::PsdRayScale,G::Operation);range(P::PsdCostAngle,P::PsdObjective,G::Operation);
    group(G::Animation,{P::HarmonicTime,P::MotionTime,P::FieldTime,P::FluxTime,P::ProbabilitySteps,P::SphereHeat,P::CurveProgress,P::CurveTravel,P::LatheTurn});
    range(P::CloudYaw,P::CloudMeanZ,G::Transform);range(P::QuadYaw,P::QuadPitch,G::Transform);group(G::Transform,{P::FluxTilt});
    range(P::GaussianReal,P::GaussianImag,G::ShapeA);range(P::GaussianOtherReal,P::GaussianOtherImag,G::ShapeB);range(P::TensorU0,P::TensorU2,G::ShapeA);range(P::TensorV0,P::TensorV2,G::ShapeB);
    range(P::CurveControl,P::CurveP3Z,G::Profile);range(P::CurveProfile,P::CurveNormP,G::Shape);range(P::LatheControl,P::LatheHeight,G::Profile);range(P::LatheHollow,P::LatheFloor,G::Shape);
    group(G::ShapeA,{P::BooleanShapeA,P::BooleanSizeA});group(G::ShapeB,{P::BooleanShapeB,P::BooleanSizeB,P::BooleanX,P::BooleanY,P::BooleanZ,P::BooleanYaw,P::BooleanPitch,P::BooleanFit,P::BooleanClearance});
    range(P::PatchControl,P::PatchP33Z,G::Profile);range(P::PatchU,P::PatchV,G::Probe);
    range(P::MembraneSlot,P::MembraneV3,G::Profile);range(P::MembraneU,P::MembraneV,G::Probe);group(G::Animation,{P::MembraneDamping,P::MembraneTime});
    range(P::RigidRotX,P::RigidRotZ,G::Transform);range(P::RigidSpinX,P::RigidTime,G::Animation);group(G::Probe,{P::RigidAxis});
    range(P::TrussJoint,P::TrussP5Y,G::Profile);group(G::Animation,{P::TrussPosition});range(P::TrussLoadX,P::TrussLoadY,G::Operation);group(G::Operation,{P::TrussBrace,P::TrussSupports});group(G::Probe,{P::TrussMember});range(P::TrussTensionLimit,P::TrussCompressionLimit,G::Advanced);
    range(P::SimplexP0,P::SimplexP1,G::ShapeA);range(P::SimplexQ0,P::SimplexQ1,G::ShapeB);range(P::SimplexValue0,P::SimplexValue2,G::Shape);group(G::Operation,{P::SimplexFunction});group(G::Animation,{P::SimplexMix});
    range(P::DistanceAB,P::DistanceCD,G::Shape);group(G::Probe,{P::DistanceEdge});group(G::Transform,{P::DistanceMirror});group(G::Operation,{P::DistanceSecond,P::DistanceScale});group(G::Animation,{P::DistanceMix});
    const auto label=[&](P p,std::string_view text){result[index(p)].label=text;};
    label(P::MembraneSlot,"Mode slot");label(P::MembraneResolution,"Subdivisions");label(P::MembraneGuides,"Guides");
    label(P::PatchControl,"Control point");label(P::PatchResolution,"Subdivisions");label(P::PatchGuides,"Guides");
    label(P::BooleanShapeA,"Shape");label(P::BooleanShapeB,"Shape");label(P::BooleanSizeA,"Size");label(P::BooleanSizeB,"Size");label(P::BooleanOperation,"Combine");label(P::BooleanBlend,"Blend");label(P::BooleanResolution,"Cells / axis");label(P::BooleanGuides,"Guides");label(P::BooleanSection,"Section");label(P::BooleanFit,"Fit preview");label(P::BooleanClearance,"Clearance");
    label(P::CurveControl,"Control point");label(P::CurveProgress,"Position");label(P::CurveProfile,"Profile");label(P::CurveRadius,"Radius");label(P::CurveAspect,"Aspect");label(P::CurveEndScale,"End scale");label(P::CurveTwist,"Twist (deg)");label(P::CurveNormP,"Exponent p");label(P::CurveGuides,"Guides");label(P::CurveTravel,"Travel rule");
    label(P::LatheControl,"Profile point");for(unsigned p=index(P::LatheR0);p<=index(P::LatheR6);++p)result[p].label="Radius";for(unsigned p=index(P::LatheH1);p<=index(P::LatheH5);++p)result[p].label="Height fraction";
    label(P::LatheHeight,"Total height");label(P::LatheWall,"Wall thickness");label(P::LatheFloor,"Cavity floor");label(P::LatheTurn,"Angle (deg)");label(P::LatheCut,"Cutaway (%)");label(P::LatheProbe,"Probe");label(P::LatheSlices,"Subdivisions");label(P::LatheMethod,"Method");label(P::LatheGuides,"Guides");
    return result;
  }();return data;
}
struct Tuple { P first;unsigned count;std::string_view label;std::array<std::string_view,3> components{"X","Y","Z"}; };
constexpr std::array tuples{
  Tuple{P::DistanceAB,3,"Lengths from A",{"AB","AC","AD"}},Tuple{P::DistanceBC,3,"Other lengths",{"BC","BD","CD"}},
  Tuple{P::SimplexP0,2,"Distribution P",{"A","B",""}},Tuple{P::SimplexQ0,2,"Distribution Q",{"A","B",""}},Tuple{P::SimplexValue0,3,"Outcome values",{"A","B","C"}},
  Tuple{P::TrussSpan,2,"Dimensions (m)",{"W","H",""}},Tuple{P::TrussLoadX,2,"Applied load (kN)",{"X","Y",""}},
  Tuple{P::TrussP0X,2,"Joint offset (m)",{"X","Y",""}},
  Tuple{P::TrussP1X,2,"Joint offset (m)",{"X","Y",""}},
  Tuple{P::TrussP2X,2,"Joint offset (m)",{"X","Y",""}},
  Tuple{P::TrussP3X,2,"Joint offset (m)",{"X","Y",""}},
  Tuple{P::TrussP4X,2,"Joint offset (m)",{"X","Y",""}},
  Tuple{P::TrussP5X,2,"Joint offset (m)",{"X","Y",""}},

  Tuple{P::RigidWidth,3,"Dimensions (m)",{"W","H","D"}},Tuple{P::RigidRotX,3,"Release angles (deg)"},Tuple{P::RigidSpinX,3,"Initial spin (rad/s)"},
  Tuple{P::MembraneM0,2,"Mode numbers",{"m","n",""}},
  Tuple{P::MembraneM1,2,"Mode numbers",{"m","n",""}},
  Tuple{P::MembraneM2,2,"Mode numbers",{"m","n",""}},
  Tuple{P::MembraneM3,2,"Mode numbers",{"m","n",""}},
  Tuple{P::MembraneA0,2,"Release state",{"q0","v0",""}},Tuple{P::MembraneA1,2,"Release state",{"q0","v0",""}},
  Tuple{P::MembraneA2,2,"Release state",{"q0","v0",""}},Tuple{P::MembraneA3,2,"Release state",{"q0","v0",""}},
  Tuple{P::MembraneWidth,2,"Dimensions",{"W","D",""}},Tuple{P::MembraneU,2,"Probe UV",{"U","V",""}},
  Tuple{P::VectorX,3,"Vector"},Tuple{P::SurfaceU,2,"Surface point",{"U","V",""}},Tuple{P::GaussianReal,2,"Real / imag",{"Re","Im",""}},Tuple{P::GaussianOtherReal,2,"Real / imag",{"Re","Im",""}},
  Tuple{P::FieldX,3,"Probe position"},Tuple{P::FieldYaw,2,"Direction",{"Yaw","Pitch",""}},Tuple{P::TensorU0,3,"Vector u",{"0","1","2"}},Tuple{P::TensorV0,3,"Vector v",{"0","1","2"}},Tuple{P::TensorW0,3,"Vector w",{"0","1","2"}},Tuple{P::TensorI,3,"Indices i/j/k",{"i","j","k"}},
  Tuple{P::CloudX,3,"Spread"},Tuple{P::CloudMeanX,3,"Mean"},Tuple{P::CloudYaw,2,"Rotation",{"Yaw","Pitch",""}},Tuple{P::QuadLambdaX,3,"Eigenvalues"},Tuple{P::QuadYaw,2,"Rotation",{"Yaw","Pitch",""}},Tuple{P::QuadX,3,"Probe"},
  Tuple{P::NormX,3,"Vector x"},Tuple{P::NormOtherX,3,"Vector y"},Tuple{P::CurveP0X,3,"Position"},Tuple{P::CurveP1X,3,"Position"},Tuple{P::CurveP2X,3,"Position"},Tuple{P::CurveP3X,3,"Position"},
  Tuple{P::PatchP00X,3,"Position"},Tuple{P::PatchP01X,3,"Position"},Tuple{P::PatchP02X,3,"Position"},Tuple{P::PatchP03X,3,"Position"},
  Tuple{P::PatchP10X,3,"Position"},Tuple{P::PatchP11X,3,"Position"},Tuple{P::PatchP12X,3,"Position"},Tuple{P::PatchP13X,3,"Position"},
  Tuple{P::PatchP20X,3,"Position"},Tuple{P::PatchP21X,3,"Position"},Tuple{P::PatchP22X,3,"Position"},Tuple{P::PatchP23X,3,"Position"},
  Tuple{P::PatchP30X,3,"Position"},Tuple{P::PatchP31X,3,"Position"},Tuple{P::PatchP32X,3,"Position"},Tuple{P::PatchP33X,3,"Position"},
  Tuple{P::PatchU,2,"Probe UV",{"U","V",""}},Tuple{P::BooleanX,3,"Position"},Tuple{P::BooleanYaw,2,"Yaw / pitch",{"Yaw","Pitch",""}},Tuple{P::BooleanProbeX,3,"Probe position"}};
struct SelectedRange {P first,last,selector;unsigned stride=1,offset=0;};
constexpr std::array selectedRanges{
  SelectedRange{P::TrussP0X,P::TrussP5Y,P::TrussJoint,2},
  SelectedRange{P::CurveP0X,P::CurveP3Z,P::CurveControl,3},SelectedRange{P::PatchP00X,P::PatchP33Z,P::PatchControl,3},
  SelectedRange{P::LatheR0,P::LatheR6,P::LatheControl},SelectedRange{P::LatheH1,P::LatheH5,P::LatheControl,1,1},
  SelectedRange{P::MembraneM0,P::MembraneV3,P::MembraneSlot,4}};
bool selectionControl(P p){return std::any_of(selectedRanges.begin(),selectedRanges.end(),[&](const auto& range){return range.selector==p;});}
bool visible(const MathObjects& m,P p){
  const auto& state=m.snapshot();const auto& spec=mathParameterSpecs()[index(p)];
  if(!m.parameterAvailable(p)||(spec.matrixEntry&&state.level>0))return false;
  for(const auto& range:selectedRanges)if(p>=range.first&&p<=range.last)return (index(p)-index(range.first))/range.stride+range.offset==static_cast<unsigned>(m.parameter(range.selector));
  return true;
}
bool defaultOpen(G group,unsigned level){
  switch(group){case G::Display:return false;case G::Advanced:case G::Sampling:return level>=2;case G::Shape:case G::ShapeA:case G::ShapeB:case G::Profile:return level<=1;default:return true;}
}
bool contains(std::string_view hay,std::string_view needle){
  return std::search(hay.begin(),hay.end(),needle.begin(),needle.end(),[](unsigned char a,unsigned char b){return std::tolower(a)==std::tolower(b);})!=hay.end();
}
}
MathControlMetadata mathControlMetadata(P p){if(p>=P::Count)throw std::invalid_argument("unknown control metadata");return metadata()[index(p)];}
std::string_view mathControlGroupName(G g){constexpr std::array<std::string_view,static_cast<unsigned>(G::Count)> names{"Shape","Shape A","Shape B","Profile","Transform","Operation","Probe","Animation","Sampling","Display","Advanced"};return names.at(static_cast<unsigned>(g));}
MathControlRows mathControlRows(const MathObjects& m){
  MathControlRows out;std::bitset<index(P::Count)> consumed;
  for(const auto& p:mathParameterSpecs())if(visible(m,p.id)&&!consumed[index(p.id)]){
    const auto meta=mathControlMetadata(p.id);MathControlRow row{meta.group,meta.label,{p.id},1};
    for(const auto& tuple:tuples)if(tuple.first==p.id){bool complete=true;for(unsigned j=0;j<tuple.count;++j){const auto q=static_cast<P>(index(p.id)+j);complete=complete&&visible(m,q)&&mathControlMetadata(q).group==meta.group;}if(complete){row.count=tuple.count;row.label=tuple.label;row.components=tuple.components;for(unsigned j=0;j<tuple.count;++j)row.parameters[j]=static_cast<P>(index(p.id)+j);}break;}
    for(unsigned j=0;j<row.count;++j)consumed.set(index(row.parameters[j]));out.rows[out.count++]=row;
  }return out;
}
MathControlRange mathControlRange(const MathObjects& m,P p){
  if(p>=P::Count)throw std::invalid_argument("unknown control range");
  const auto& spec=mathParameterSpecs()[index(p)];MathControlRange range{spec.minimum,spec.maximum};
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

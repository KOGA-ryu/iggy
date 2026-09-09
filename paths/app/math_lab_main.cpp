#include "runtime/math_objects/MathObjects.hpp"
#include "ui/MatrixBoardUi.hpp"
#include "ui/MathControlUi.hpp"
#include "ui/TextbookUi.hpp"
#include "ui/NativeMath.hpp"
#include "scene/MathObjectScene.hpp"
#include "platform/NativeVulkanHost.hpp"

#include <SDL3/SDL.h>
#include "imgui.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace paths;
using Navigation = iggy3d::ProductCreativeViewportNavigationOperation;
enum class LabPage { Textbook, Objects, MatrixBoard };
struct Options {
  NativeLaunchConfig native{false,1440,900,true};
  unsigned frames=0;
  bool validate=false;
  std::filesystem::path capture;
  std::vector<MathAction> actions;
  std::vector<BoardAction> boardActions;
  bool board=false,book=false;
  LabPage page=LabPage::Textbook;
  std::vector<BookAction> bookActions;
  std::vector<SystemAction> systemActions;
  LessonPresentation bookPresentation=LessonPresentation::Together;
};
double number(std::string_view text) {
  double result=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
  if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||!std::isfinite(result))throw std::invalid_argument("Expected a finite number");return result;
}
unsigned integer(std::string_view text,unsigned minimum,unsigned maximum) {
  const double result=number(text);if(result<minimum||result>maximum||std::floor(result)!=result)throw std::invalid_argument("Integer option out of range");return static_cast<unsigned>(result);
}
enum class Flag { Offscreen,Frames,Capture,Resolution,Object,Set,Route,Check,Level,Preset,ObjectPreset,Descent,Swap,Validate,Turn,UndoTurn,Identity,Advance,ModularStep,ResetWalk,ReversePath,WalkStep,ResetProbability,TrialStep,ResetTrials,Card,BoardCase,BoardSize,BoardStep,BoardUndo,BoardCheck,Book,Section,BookNext,BookPrevious,BookExercise,BookRead,BookIndex,BookView,SystemExample,SystemRhs,SystemStep,SystemChallenge,SystemPredict,Help };
Options parse(int argc,char** argv) {
  constexpr std::array<std::pair<std::string_view,Flag>,45> flags{{
    {"--offscreen",Flag::Offscreen},{"--frames",Flag::Frames},{"--capture",Flag::Capture},
    {"--resolution",Flag::Resolution},{"--object",Flag::Object},{"--set",Flag::Set},
    {"--route",Flag::Route},{"--check",Flag::Check},{"--level",Flag::Level},
    {"--object-preset",Flag::ObjectPreset},{"--preset",Flag::Preset},{"--descent",Flag::Descent},{"--swap-bounds",Flag::Swap},
    {"--validate",Flag::Validate},{"--turn",Flag::Turn},{"--undo-turn",Flag::UndoTurn},
    {"--identity",Flag::Identity},{"--advance",Flag::Advance},{"--step",Flag::ModularStep},
    {"--reset-walk",Flag::ResetWalk},{"--reverse-path",Flag::ReversePath},{"--walk-step",Flag::WalkStep},{"--reset-probability-walk",Flag::ResetProbability},{"--trial-step",Flag::TrialStep},{"--reset-trials",Flag::ResetTrials},{"--card",Flag::Card},{"--board-case",Flag::BoardCase},{"--board-size",Flag::BoardSize},{"--board-step",Flag::BoardStep},{"--board-undo",Flag::BoardUndo},{"--board-check",Flag::BoardCheck},{"--book",Flag::Book},{"--section",Flag::Section},{"--book-next",Flag::BookNext},{"--book-previous",Flag::BookPrevious},{"--book-exercise",Flag::BookExercise},{"--book-read",Flag::BookRead},{"--book-index",Flag::BookIndex},{"--book-view",Flag::BookView},{"--system-example",Flag::SystemExample},{"--system-rhs",Flag::SystemRhs},{"--system-step",Flag::SystemStep},{"--system-challenge",Flag::SystemChallenge},{"--system-predict",Flag::SystemPredict},{"--help",Flag::Help}}};
  Options options;
  for(int i=1;i<argc;++i) {
    const std::string_view name=argv[i];const auto flag=std::find_if(flags.begin(),flags.end(),[&](const auto& item){return item.first==name;});
    if(flag==flags.end())throw std::invalid_argument("Unknown argument: "+std::string(name));
    const auto next=[&]() -> std::string_view {if(++i>=argc)throw std::invalid_argument("Missing value for "+std::string(name));return argv[i];};
    switch(flag->second) {
      case Flag::Offscreen:options.native.offscreen=true;break;
      case Flag::Frames:options.frames=integer(next(),1,100000);break;
      case Flag::Capture:options.capture=next();break;
      case Flag::Resolution: {
        const auto text=next();const auto x=text.find('x');if(x==text.npos)throw std::invalid_argument("Expected WIDTHxHEIGHT");
        options.native.width=integer(text.substr(0,x),800,3840);options.native.height=integer(text.substr(x+1),600,2160);break;
      }
      case Flag::Object: {
        const auto key=next();const auto specs=mathObjectSpecs();const auto found=std::find_if(specs.begin(),specs.end(),[&](const auto& s){return s.key==key;});
        if(found==specs.end())throw std::invalid_argument("Unknown math object");options.actions.push_back({MathActionKind::Select,found->id});break;
      }
      case Flag::Set: {
        const auto text=next();const auto eq=text.find('=');if(eq==text.npos)throw std::invalid_argument("Expected parameter=value");
        const auto specs=mathParameterSpecs();const auto found=std::find_if(specs.begin(),specs.end(),[&](const auto& s){return s.key==text.substr(0,eq);});
        if(found==specs.end())throw std::invalid_argument("Unknown math parameter");
        options.actions.push_back({MathActionKind::SetParameter,{},found->id,number(text.substr(eq+1))});break;
      }
      case Flag::Route:
        for(char c:next()) {if(c<'A'||c>'H')throw std::invalid_argument("Route must contain letters A-H");options.actions.push_back({MathActionKind::VisitVertex,{},{},0,static_cast<unsigned>(c-'A')});}break;
      case Flag::Check:options.actions.push_back({MathActionKind::Check});break;
      case Flag::Level:options.actions.push_back({MathActionKind::SetLevel,{},{},static_cast<double>(integer(next(),0,3))});break;
      case Flag::ObjectPreset:options.actions.push_back({MathActionKind::ObjectPreset,{},{},0,integer(next(),0,100)});break;
      case Flag::Preset:options.actions.push_back({MathActionKind::MatrixPreset,{},{},0,integer(next(),0,4)});break;
      case Flag::Descent:options.actions.push_back({MathActionKind::DescentStep});break;
      case Flag::Swap:options.actions.push_back({MathActionKind::SwapBounds});break;
      case Flag::Validate:options.validate=true;break;
      case Flag::Turn: {
        static constexpr std::array<std::string_view,6> turns{"x","y","z","x-inverse","y-inverse","z-inverse"};
        const auto name=next();const auto found=std::find(turns.begin(),turns.end(),name);if(found==turns.end())throw std::invalid_argument("Unknown cube turn");
        options.actions.push_back({MathActionKind::SymmetryTurn,{},{},0,static_cast<unsigned>(found-turns.begin())});break;
      }
      case Flag::UndoTurn:options.actions.push_back({MathActionKind::SymmetryUndo});break;
      case Flag::Identity:options.actions.push_back({MathActionKind::SymmetryIdentity});break;
      case Flag::Advance:options.actions.push_back({MathActionKind::AdvanceTime,{},{},number(next())});break;
      case Flag::ModularStep:options.actions.push_back({MathActionKind::ModularStep,{},{},number(next())});break;
      case Flag::ResetWalk:options.actions.push_back({MathActionKind::ResetModularWalk});break;
      case Flag::ReversePath:options.actions.push_back({MathActionKind::ReverseFieldPath});break;
      case Flag::WalkStep:options.actions.push_back({MathActionKind::ProbabilityStep});break;
      case Flag::ResetProbability:options.actions.push_back({MathActionKind::ResetProbabilityWalk});break;
      case Flag::TrialStep:options.actions.push_back({MathActionKind::BernoulliStep});break;
      case Flag::ResetTrials:options.actions.push_back({MathActionKind::ResetBernoulli});break;
      case Flag::Card:options.board=true;options.boardActions.push_back({BoardActionKind::Select,integer(next(),1,95)});break;
      case Flag::BoardCase: {
        if(options.boardActions.empty()||options.boardActions.back().kind!=BoardActionKind::Select)throw std::invalid_argument("--board-case must immediately follow --card");
        options.boardActions.back().second=integer(next(),0,5);break;
      }
      case Flag::BoardSize: {
        const auto n=integer(next(),2,32),b=integer(next(),0,31),cut=integer(next(),1,32);options.boardActions.push_back({BoardActionKind::Configure,n,b,cut});break;
      }
      case Flag::BoardStep: {const auto count=integer(next(),1,128);for(unsigned k=0;k<count;++k)options.boardActions.push_back({BoardActionKind::Step});break;}
      case Flag::BoardUndo:options.boardActions.push_back({BoardActionKind::Undo});break;
      case Flag::BoardCheck:options.boardActions.push_back({BoardActionKind::Check});break;
      case Flag::Book:options.book=true;options.bookActions.push_back({BookActionKind::Resume});break;
      case Flag::Section:options.bookActions.push_back({BookActionKind::OpenSection,integer(next(),1,static_cast<unsigned>(matrixChapter().size()))-1});break;
      case Flag::BookNext:options.bookActions.push_back({BookActionKind::Next});break;
      case Flag::BookPrevious:options.bookActions.push_back({BookActionKind::Previous});break;
      case Flag::BookExercise:options.bookActions.push_back({BookActionKind::Exercise});break;
      case Flag::BookRead:options.bookActions.push_back({BookActionKind::Read});break;
      case Flag::BookIndex:options.bookActions.push_back({BookActionKind::Index});break;
      case Flag::BookView:{
        static constexpr std::array<std::string_view,3> views{"together","reading","figure"};
        const auto name=next();const auto found=std::find(views.begin(),views.end(),name);
        if(found==views.end())throw std::invalid_argument("Use --book-view together, reading, or figure");
        options.book=true;options.bookPresentation=static_cast<LessonPresentation>(found-views.begin());break;
      }
      case Flag::SystemExample:options.systemActions.push_back({SystemActionKind::SelectExample,integer(next(),1,5)-1});break;
      case Flag::SystemRhs:{const auto row=integer(next(),1,3)-1;options.systemActions.push_back({SystemActionKind::SetRightHandSide,row,0,number(next())});break;}
      case Flag::SystemStep:options.systemActions.push_back({SystemActionKind::RowOperation,0,0,0,{BoardActionKind::Step}});break;
      case Flag::SystemChallenge:options.systemActions.push_back({SystemActionKind::SelectChallenge,integer(next(),1,3)-1});break;
      case Flag::SystemPredict:{const auto outcome=integer(next(),0,2);const auto reason=integer(next(),0,2);options.systemActions.push_back({SystemActionKind::Predict,outcome,reason});break;}
      case Flag::Help:
        std::printf("Textbook (default): [--book] [--section 1..%zu] [--book-next] [--book-previous] [--book-exercise] [--book-read] [--book-index] [--book-view together|reading|figure] [--validate]\n--validate never reads or writes a reading bookmark.\n",matrixChapter().size());
        std::puts("Systems lesson (section 3): --system-example 1..5; --system-rhs ROW VALUE; --system-step; --system-challenge 1..3; --system-predict OUTCOME REASON. Outcomes: 0 none, 1 one, 2 infinite. Reasons: 0 contradiction, 1 full pivots, 2 free variables.");
        std::puts("Matrix board: --card 001|004|018|031|044|059 [--board-case 0..5] [--board-size SIZE BAND CUT] [--board-step N] [--board-undo] [--board-check] [--validate]");
        std::puts("math_lab [--object algebra|trig|calculus|linear|discrete|function|surface|symmetry|harmonics|oscillator|modular|gaussian|field|flux|tensor|probability|binomial|bayes|covariance|spherical|quadratic|roots|psd|norm|curve|lathe|boolean|patch|membrane|rigid|truss|simplex|distance|polar] [--level 0..3]\n         [--set key=value] [--object-preset N] [--preset 0..4] [--descent] [--swap-bounds] [--route BDH] [--check]\n         [--turn x|y|z|x-inverse|y-inverse|z-inverse] [--undo-turn] [--identity] [--advance duration]\n         [--step 1|-1] [--reset-walk] [--reverse-path] [--walk-step] [--reset-probability-walk] [--trial-step] [--reset-trials]\n         [--validate] [--offscreen] [--frames N] [--resolution 1440x900] [--capture /path/view.png]\n--validate computes geometry and prints measurements without creating a native host or images.\nArguments apply in order: select the object and level before setting its parameters.\nObject presets: PSD 0 bowl, 1 rank-one, 2 saddle, 3 zero, 4 positive-entry indefinite, 5 negative bowl; norm 0 octahedron, 1 sphere, 2 rounded cube, 3 exact cube; curve 0 pipe, 1 cable, 2 ribbon, 3 horn; lathe 0 vase, 1 bottle, 2 goblet, 3 pawn; boolean 0 drilled block, 1 archway, 2 ball-and-socket, 3 blended stones; patch 0 canopy, 1 sail, 2 curved ramp, 3 saddle terrain; membrane 0 drumhead, 1 divided membrane, 2 interference, 3 damped pluck; rigid 0 flywheel, 1 adjustable dumbbell, 2 tumbling book, 3 satellite; truss 0 triangular support, 1 bridge, 2 crane boom, 3 roof truss; simplex 0 balanced uncertainty, 1 mix two certainties, 2 same mean/different spread, 3 asymmetric information, 4 missing outcome; distance 0 tetrahedron, 1 square, 2 line, 3 coincident points, 4 impossible tetrahedron, 5 dimension from mixing; polar 0 sheared block, 1 quarter-turn and stretch, 2 pure rotation, 3 mirror and stretch, 4 collapsed sheet, 5 thin direction.\nMatrix presets: 0 identity, 1 shear, 2 xy projection, 3 stretch/reflection, 4 z rotation.");
        for(const auto& p:mathParameterSpecs())std::printf("  %s [%g,%g]  %s (level %u+)\n",p.key.data(),p.minimum,p.maximum,p.label.data(),p.minimumLevel);
        std::exit(0);
    }
  }
  options.book=options.book||!options.bookActions.empty();
  if(options.book&&(options.board||!options.actions.empty()))throw std::invalid_argument("Use textbook, card, or object arguments in one invocation");
  if(options.board)options.page=LabPage::MatrixBoard;
  if(!options.actions.empty())options.page=LabPage::Objects;
  if(!options.boardActions.empty()&&!options.board)throw std::invalid_argument("Board actions require --card");
  if(options.board&&!options.actions.empty())throw std::invalid_argument("Use either card actions or object actions in one invocation");
  if(options.native.offscreen&&!options.frames)options.frames=3;
  if(!options.capture.empty()&&!options.frames)options.frames=3;
  if(options.validate&&!options.capture.empty())throw std::invalid_argument("Text validation cannot capture images");
  return options;
}
struct UiState {
  bool quit=false,hasPending=false,inspector=true,drawer=true,rememberDefaults=false;
  MathAction pending{};MathInspectorMemory memory;
  float inspectorWidth=360,drawerHeight=200;
  unsigned labels=1;int requestTab=-1;
  std::array<char,96> search{};std::string message;
};
constexpr auto windowFlags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBringToFrontOnFocus;
void window(const char* name,ImVec2 position,ImVec2 size,ImGuiWindowFlags extra=0) {
  ImGui::SetNextWindowPos(position);ImGui::SetNextWindowSize(size);ImGui::Begin(name,nullptr,windowFlags|extra);
}
void apply(MathObjects& objects,MathAction action) {
  const auto result=objects.dispatch(action);if(!result.accepted)throw std::runtime_error(std::string(result.reason));
}
void queue(UiState& ui,MathAction action) { ui.pending=action;ui.hasPending=true;ui.rememberDefaults=action.kind==MathActionKind::Reset; }
ImU32 colour(iggy3d::Vec3 c,float alpha=1) {return ImGui::ColorConvertFloat4ToU32({c.x,c.y,c.z,alpha});}
void plotView(const MathPlot& plot,const MathObjects& model,UiState& ui) {
  ImGui::PushID(plot.title.data());ImGui::TextUnformatted(plot.title.data());
  const ImVec2 origin=ImGui::GetCursorScreenPos(),size{std::max(60.0F,ImGui::GetContentRegionAvail().x),std::max(55.0F,ImGui::GetContentRegionAvail().y-45)};
  ImGui::InvisibleButton("plot",size);auto* draw=ImGui::GetWindowDrawList();
  double minX=0,maxX=0,minY=0,maxY=0;
  for(std::size_t s=0;s<plot.seriesCount;++s)for(std::size_t i=0;i<plot.series[s].count;++i) {
    const auto p=plot.series[s].points[i];minX=std::min(minX,p.x);maxX=std::max(maxX,p.x);minY=std::min(minY,p.y);maxY=std::max(maxY,p.y);
  }
  if(plot.hasMarker){minX=std::min(minX,plot.marker.x);maxX=std::max(maxX,plot.marker.x);minY=std::min(minY,plot.marker.y);maxY=std::max(maxY,plot.marker.y);}
  const double pad=std::max(.25,(maxY-minY)*.08);minY-=pad;maxY+=pad;if(maxX-minX<1e-9)maxX=minX+1;
  if(plot.equalAspect) {
    const double aspect=size.x/size.y,dx=maxX-minX,dy=maxY-minY;
    if(dx<aspect*dy){const double extra=(aspect*dy-dx)/2;minX-=extra;maxX+=extra;}
    else {const double extra=(dx/aspect-dy)/2;minY-=extra;maxY+=extra;}
  }
  const auto project=[&](MathPlotPoint p){return ImVec2{origin.x+static_cast<float>((p.x-minX)/(maxX-minX))*size.x,origin.y+size.y-static_cast<float>((p.y-minY)/(maxY-minY))*size.y};};
  draw->AddRectFilled(origin,{origin.x+size.x,origin.y+size.y},IM_COL32(16,28,39,255),3);
  draw->PushClipRect(origin,{origin.x+size.x,origin.y+size.y},true);
  draw->AddLine(project({minX,0}),project({maxX,0}),IM_COL32(85,102,117,255));draw->AddLine(project({0,minY}),project({0,maxY}),IM_COL32(85,102,117,255));
  // Diagrams forward an input change for the next frame, before any view is
  // drawn, so every representation in a frame reads one model revision.
  if(ImGui::IsItemActive()&&model.parameterAvailable(plot.scrubParameter)) {
    const auto& parameter=mathParameterSpecs()[static_cast<std::size_t>(plot.scrubParameter)];
    const double value=std::clamp(minX+(ImGui::GetIO().MousePos.x-origin.x)/size.x*(maxX-minX),parameter.minimum,parameter.maximum);
    queue(ui,{MathActionKind::SetParameter,{},parameter.id,value});
  }
  for(std::size_t s=0;s<plot.seriesCount;++s) {
    const auto& series=plot.series[s];
    const auto fill=[&](MathPlotPoint a,MathPlotPoint b) {
      std::array<ImVec2,4> quad{project(a),project(b),project({b.x,0}),project({a.x,0})};
      const bool negative=a.y+b.y<0;if(negative)std::reverse(quad.begin(),quad.end());
      draw->AddConvexPolyFilled(quad.data(),4,negative?IM_COL32(242,143,99,75):colour(series.color,.28F));
    };
    if(series.stems) {
      for(std::size_t i=0;i<series.count;++i){const auto p=series.points[i];draw->AddLine(project({p.x,0}),project(p),colour(series.color),2);draw->AddCircleFilled(project(p),3,colour(series.color));}
      continue;
    }
    for(std::size_t i=1;i<series.count;++i) {
      const auto a=series.points[i-1],b=series.points[i];
      if(series.signedFill) {
        if(a.y*b.y<0) {const MathPlotPoint zero{a.x+(b.x-a.x)*(-a.y)/(b.y-a.y),0};fill(a,zero);fill(zero,b);}
        else fill(a,b);
      } else draw->AddLine(project(a),project(b),colour(series.color),2);
    }
  }
  if(plot.hasMarker) {draw->AddLine(project({plot.marker.x,minY}),project({plot.marker.x,maxY}),IM_COL32(245,201,90,100));draw->AddCircleFilled(project(plot.marker),4,IM_COL32(245,201,90,255));}
  draw->PopClipRect();
  ImGui::TextDisabled("x: %.2g .. %.2g   y: %.2g .. %.2g",minX,maxX,minY,maxY);
  for(std::size_t s=0;s<plot.seriesCount;++s) {if(s)ImGui::SameLine();ImGui::TextColored({plot.series[s].color.x,plot.series[s].color.y,plot.series[s].color.z,1},"%s",plot.series[s].name.data());}
  ImGui::PopID();
}
void matrixView(const MathMatrixView& matrix,const MathObjects& model,UiState& ui) {
  ImGui::PushID(matrix.name.data());ImGui::TextUnformatted(matrix.name.data());
  if(ImGui::BeginTable("matrix",static_cast<int>(matrix.columns),ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_BordersInner)) {
    for(unsigned i=0;i<matrix.rows*matrix.columns;++i) {
      ImGui::TableNextColumn();ImGui::PushID(static_cast<int>(i));
      if(matrix.editable&&model.parameterAvailable(matrix.parameters[i])) {
        const auto& p=mathParameterSpecs()[static_cast<std::size_t>(matrix.parameters[i])];float value=static_cast<float>(matrix.values[i]);ImGui::SetNextItemWidth(-1);
        if(ImGui::DragFloat("##entry",&value,.025F,static_cast<float>(p.minimum),static_cast<float>(p.maximum),"%.2f",ImGuiSliderFlags_AlwaysClamp))queue(ui,{MathActionKind::SetParameter,{},p.id,value});
        if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",p.label.data());
      } else ImGui::Text("% .3g",matrix.values[i]);
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  if(matrix.editable)ImGui::TextWrapped("Drag entries to change the same A used by the object and measurements.");
  ImGui::PopID();
}
void contourView(const MathContourMap& map,UiState& ui,bool quadratic=false) {
  ImGui::TextUnformatted(quadratic?"Zero set q(x,y,z0)=0 in the input plane":"Contours in the input plane (u,v)");
  const ImVec2 available=ImGui::GetContentRegionAvail();const float side=std::max(50.0F,std::min(available.x,available.y-25));
  const ImVec2 origin=ImGui::GetCursorScreenPos();ImGui::InvisibleButton("contour map",{side,side});auto* draw=ImGui::GetWindowDrawList();
  const auto project=[&](MathPlotPoint p){return ImVec2{origin.x+static_cast<float>((p.x+2)/4)*side,origin.y+static_cast<float>((2-p.y)/4)*side};};
  draw->AddRectFilled(origin,{origin.x+side,origin.y+side},IM_COL32(16,28,39,255),3);draw->PushClipRect(origin,{origin.x+side,origin.y+side},true);
  draw->AddLine(project({-2,0}),project({2,0}),IM_COL32(85,102,117,255));draw->AddLine(project({0,-2}),project({0,2}),IM_COL32(85,102,117,255));
  for(std::size_t i=0;i<map.count;++i) {
    const auto& line=map.segments[i];draw->AddLine(project(line.a),project(line.b),line.height<0?IM_COL32(243,145,100,255):line.height==0?IM_COL32(210,226,238,255):IM_COL32(72,186,172,255),1.3F);
  }
  if(map.constrained)draw->AddCircle(project({0,0}),side/4,IM_COL32(245,201,90,255),64,2);
  draw->AddLine(project(map.point),project({map.point.x+map.gradient.x*.25,map.point.y+map.gradient.y*.25}),IM_COL32(245,201,90,255),2);
  draw->AddCircleFilled(project(map.point),4,IM_COL32(245,201,90,255));draw->PopClipRect();
  if(ImGui::IsItemActive()&&!map.constrained) {
    const auto mouse=ImGui::GetIO().MousePos;
    queue(ui,{MathActionKind::MoveSurfacePoint,{},{},std::clamp(-2+4.0*(mouse.x-origin.x)/side,-2.0,2.0),0,std::clamp(2-4.0*(mouse.y-origin.y)/side,-2.0,2.0)});
  }
  ImGui::TextDisabled(quadratic?"x/y: -2 .. 2; drag to move the probe; gradient at 1/4 scale":"u/v: -2 .. 2; gradient shown at 1/4 scale");
}
void symmetryView(const MathSymmetryView& view) {
  ImGui::TextUnformatted("Label -> fixed destination slot");
  if(ImGui::BeginTable("permutation",4,ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_BordersInner)) {
    for(unsigned i=0;i<8;++i){ImGui::TableNextColumn();ImGui::Text("%c -> %c",'A'+i,'A'+view.permutation[i]);}ImGui::EndTable();
  }
  std::string orbit;for(unsigned i=0;i<8;++i)if(view.orbit[i]){if(!orbit.empty())orbit+=" ";orbit+=static_cast<char>('A'+i);}
  if(!orbit.empty())ImGui::TextWrapped("Orbit destinations (gold slots): %s",orbit.c_str());
  if(view.moveCount) {
    static constexpr std::array<const char*,6> moves{"X+","Y+","Z+","X-","Y-","Z-"};std::string word;
    for(std::size_t i=0;i<view.moveCount;++i){if(i)word+=" ";word+=moves[view.moves[i]];}ImGui::TextWrapped("Applied in order: %s",word.c_str());
  }
  ImGui::TextWrapped("Slots A-H have sign bits x=1, y=2, z=4; A=(-,-,-), H=(+,+,+). Labels move with the cube.");
}
void explorationControls(const MathObjects& model,UiState& ui) {
  const auto& s=model.snapshot();
  if(s.kind==MathObjectKind::Symmetry&&s.level<2) {
    static constexpr std::array<const char*,6> labels{"X +90","Y +90","Z +90","X -90","Y -90","Z -90"};
    ImGui::TextUnformatted("Turn about a fixed world axis");ImGui::BeginDisabled(s.symmetry.moveCount==s.symmetry.moves.size());
    for(unsigned i=0;i<6;++i){if(i%3)ImGui::SameLine();if(ImGui::Button(labels[i]))queue(ui,{MathActionKind::SymmetryTurn,{},{},0,i});}
    if(s.level==1){if(ImGui::Button("Apply first"))queue(ui,{MathActionKind::SymmetryTurn,{},{},0,static_cast<unsigned>(model.parameter(MathParameter::SymmetryFirst))});ImGui::SameLine();if(ImGui::Button("Apply second"))queue(ui,{MathActionKind::SymmetryTurn,{},{},0,static_cast<unsigned>(model.parameter(MathParameter::SymmetrySecond))});}
    ImGui::EndDisabled();ImGui::BeginDisabled(s.symmetry.moveCount==0);if(ImGui::Button("Undo turn"))queue(ui,{MathActionKind::SymmetryUndo});ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Return to identity"))queue(ui,{MathActionKind::SymmetryIdentity});
  }
  if(s.kind==MathObjectKind::Modular&&s.level==1) {
    ImGui::BeginDisabled(s.modularWalkSteps==64);
    if(ImGui::Button("Step forward"))queue(ui,{MathActionKind::ModularStep,{},{},1});ImGui::SameLine();if(ImGui::Button("Step backward"))queue(ui,{MathActionKind::ModularStep,{},{},-1});
    ImGui::EndDisabled();if(ImGui::Button("Reset walk"))queue(ui,{MathActionKind::ResetModularWalk});
    if(s.modularWalkSteps==64)ImGui::TextWrapped("Walk limit reached. Reset to explore another cycle.");
  }
  if(s.kind==MathObjectKind::VectorField&&s.level>0) {
    if(ImGui::Button("Reverse path"))queue(ui,{MathActionKind::ReverseFieldPath});ImGui::SameLine();ImGui::TextUnformatted(s.fieldPathReversed?"Reversed":"Forward");
  }
  if(s.kind==MathObjectKind::Probability&&s.level==0) {
    ImGui::BeginDisabled(s.probabilityWalkCount==s.probabilityWalk.size());if(ImGui::Button("Next walk step"))queue(ui,{MathActionKind::ProbabilityStep});ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Reset walk"))queue(ui,{MathActionKind::ResetProbabilityWalk});
    if(s.probabilityWalkCount==s.probabilityWalk.size())ImGui::TextWrapped("Walk limit reached. Reset to start again.");
  }
  if(s.kind==MathObjectKind::Probability&&s.level>=2) {
    const auto p=MathParameter::ProbabilitySteps;ImGui::BeginDisabled(model.parameter(p)>=64);if(ImGui::Button("Next distribution"))queue(ui,{MathActionKind::SetParameter,{},p,model.parameter(p)+1});ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Restart distribution"))queue(ui,{MathActionKind::SetParameter,{},p,0});
  }
  if(s.kind==MathObjectKind::Binomial&&s.level==0) {
    ImGui::BeginDisabled(s.bernoulliSteps>=model.parameter(MathParameter::BinomialTrials));if(ImGui::Button("Next trial"))queue(ui,{MathActionKind::BernoulliStep});ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Reset trials"))queue(ui,{MathActionKind::ResetBernoulli});
    if(s.bernoulliSteps>=model.parameter(MathParameter::BinomialTrials))ImGui::TextWrapped("Trial path complete. Reset to replay it, or change the seed for another path.");
  }
  const auto time=model.playbackParameter();
  if(model.parameterAvailable(time)) {
    const bool atEnd=model.parameter(time)>=mathParameterSpecs()[static_cast<std::size_t>(time)].maximum;
    ImGui::BeginDisabled(atEnd);if(ImGui::Button(s.playing?"Pause":"Play"))queue(ui,{MathActionKind::TogglePlayback});ImGui::SameLine();if(ImGui::Button("Advance 0.1"))queue(ui,{MathActionKind::AdvanceTime,{},{},.1});ImGui::EndDisabled();ImGui::SameLine();
    if(ImGui::Button(mathObjectSpecs()[static_cast<unsigned>(s.kind)].playbackRestart.data()))queue(ui,{MathActionKind::SetParameter,{},time,0});
    if(atEnd)ImGui::TextWrapped("%s",mathObjectSpecs()[static_cast<unsigned>(s.kind)].playbackEnd.data());
  }
}
void valueTable(const MathValueTable& table) {
  ImGui::TextWrapped("%s",table.title.data());
  if(ImGui::BeginTable("values",static_cast<int>(table.columnCount+1),ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_BordersInner|ImGuiTableFlags_RowBg|ImGuiTableFlags_ScrollY,{0,std::max(45.0F,ImGui::GetContentRegionAvail().y)})) {
    ImGui::TableSetupColumn("Value");for(std::size_t c=0;c<table.columnCount;++c)ImGui::TableSetupColumn(table.columns[c].data());ImGui::TableSetupScrollFreeze(0,1);ImGui::TableHeadersRow();
    for(std::size_t r=0;r<table.rowCount;++r){ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::TextUnformatted(table.rowLabels[r].data());for(std::size_t c=0;c<table.columnCount;++c){ImGui::TableNextColumn();ImGui::Text("%.5g",table.values[r][c]);}}
    ImGui::EndTable();
  }
}
void linkedViews(const MathObjects& model,UiState& ui) {
  const auto& snapshot=model.snapshot();
  const auto panels=[&](auto count,auto display) {
    if(ImGui::GetContentRegionAvail().x<720&&count>1) {
      if(ImGui::BeginTabBar("views")) {for(std::size_t i=0;i<count;++i){const auto name=display(i,false);if(ImGui::BeginTabItem(name.data())){display(i,true);ImGui::EndTabItem();}}ImGui::EndTabBar();}
    } else if(ImGui::BeginTable("views",static_cast<int>(count),ImGuiTableFlags_SizingStretchSame)) {
      for(std::size_t i=0;i<count;++i){ImGui::TableNextColumn();display(i,true);}ImGui::EndTable();
    }
  };
  const unsigned types=(snapshot.plotCount?1:0)+(snapshot.matrixCount?1:0)+(snapshot.contours.active?1:0)+(snapshot.symmetry.active?1:0);
  const auto plots=[&]{panels(snapshot.plotCount,[&](std::size_t i,bool show){if(show)plotView(snapshot.plots[i],model,ui);return snapshot.plots[i].title;});};
  const auto matrices=[&]{panels(snapshot.matrixCount,[&](std::size_t i,bool show){if(show)matrixView(snapshot.matrices[i],model,ui);return snapshot.matrices[i].name;});};
  const auto contours=[&]{contourView(snapshot.contours,ui,snapshot.kind==MathObjectKind::Quadratic);};
  if(types>1) {
    if(ImGui::BeginTabBar("Representations")) {
      if(snapshot.plotCount&&ImGui::BeginTabItem("Plots")){plots();ImGui::EndTabItem();}
      if(snapshot.matrixCount&&ImGui::BeginTabItem("Matrices")){matrices();ImGui::EndTabItem();}
      if(snapshot.contours.active&&ImGui::BeginTabItem("Contours")){contours();ImGui::EndTabItem();}
      if(snapshot.symmetry.active&&ImGui::BeginTabItem("Permutation")){symmetryView(snapshot.symmetry);ImGui::EndTabItem();}
      ImGui::EndTabBar();
    }
  }else if(snapshot.plotCount)plots();
  else if(snapshot.matrixCount)matrices();
  else if(snapshot.contours.active)contours();
  else if(snapshot.symmetry.active)symmetryView(snapshot.symmetry);
  else ImGui::TextWrapped("Explore the 3D model above. Measurements are in Values; the relationship is in Math.");
}
void objectPicker(MathObjects& model,UiState& ui,float width) {
  ImGui::SetNextItemWidth(width);
  if(ImGui::BeginCombo("##object",mathObjectSpecs()[static_cast<unsigned>(model.snapshot().kind)].name.data(),ImGuiComboFlags_HeightLarge)) {
    ImGui::SetNextItemWidth(-1);
    if(ImGui::IsWindowAppearing())ImGui::SetKeyboardFocusHere();
    ImGui::InputTextWithHint("##search","Search objects...",ui.search.data(),ui.search.size(),ImGuiInputTextFlags_AutoSelectAll);
    unsigned matches=0;
    for(const auto& object:mathObjectSpecs())if(matchesMathObject(object.id,ui.search.data())) {
      ++matches;
      if(ImGui::Selectable(object.name.data(),object.id==model.snapshot().kind)) {
        apply(model,{MathActionKind::Select,object.id});ui.memory.visit(model);ui.message.clear();
      }
    }
    if(!matches)ImGui::TextDisabled("No matching objects");
    ImGui::EndCombo();
  }
  if(ImGui::IsItemHovered())ImGui::SetTooltip("Choose or search all math objects");
}
void examplePicker(MathObjects& model,UiState& ui,float width) {
  ImGui::SetNextItemWidth(width);
  if(ImGui::BeginCombo("##example",ui.memory.exampleTitle(model).data())) {
    if(ImGui::Selectable("Defaults")) {apply(model,{MathActionKind::Reset});ui.memory.rememberExample(model,"Defaults");}
    const auto presets=mathObjectPresets(model.snapshot().kind,model.snapshot().level);
    for(unsigned i=0;i<presets.size();++i)if(ImGui::Selectable(presets[i].name.data())) {
      apply(model,{MathActionKind::ObjectPreset,{},{},0,i});ui.memory.rememberExample(model,presets[i].name);
    }
    if(model.snapshot().kind==MathObjectKind::Linear&&model.snapshot().level>0) {
      const auto names=mathMatrixPresetNames();
      for(unsigned i=0;i<names.size();++i)if(ImGui::Selectable(names[i].data())) {
        apply(model,{MathActionKind::MatrixPreset,{},{},0,i});ui.memory.rememberExample(model,names[i]);
      }
    }
    ImGui::EndCombo();
  }
  if(ImGui::IsItemHovered())ImGui::SetTooltip("Example; edits are shown as Custom");
}
void layerPicker(MathObjects& model,UiState& ui,float width) {
  const auto lessons=mathLessons(model.snapshot().kind);ImGui::SetNextItemWidth(width);
  ImGui::BeginDisabled(lessons.empty());
  if(ImGui::BeginCombo("##layer",lessons.empty()?"Explore":lessons[model.snapshot().level].name.data())) {
    for(unsigned i=0;i<lessons.size();++i)if(ImGui::Selectable(lessons[i].name.data(),model.snapshot().level==i)) {
      apply(model,{MathActionKind::SetLevel,{},{},static_cast<double>(i)});ui.memory.visit(model);
    }
    ImGui::EndCombo();
  }
  ImGui::EndDisabled();if(ImGui::IsItemHovered())ImGui::SetTooltip("Learning layer");
}
void toolbar(MathObjects& model,UiState& ui,LabPage& page,const MathLabLayout& layout,float scale) {
  const auto r=layout.toolbar;window("Math lab toolbar",{r.x,r.y},{r.width,r.height},ImGuiWindowFlags_NoScrollbar);
  const float width=ImGui::GetContentRegionAvail().x,gap=ImGui::GetStyle().ItemSpacing.x,nav=48*scale,actions=2*ImGui::GetFrameHeight()+ImGui::CalcTextSize("Controls").x+ImGui::CalcTextSize("Analysis").x+2*ImGui::GetStyle().ItemInnerSpacing.x+gap;
  if(ImGui::Button("Lab",{nav,0}))ImGui::OpenPopup("Lab pages");
  if(ImGui::BeginPopup("Lab pages")) {
    if(ImGui::MenuItem("Textbook"))page=LabPage::Textbook;
    if(ImGui::MenuItem("Exercise matrix board"))page=LabPage::MatrixBoard;
    ImGui::EndPopup();
  }
  ImGui::SameLine();
  const auto toggles=[&] {ImGui::Checkbox("Controls",&ui.inspector);ImGui::SameLine();ImGui::Checkbox("Analysis",&ui.drawer);};
  if(layout.toolbarRows==1) {
    const float room=width-nav-actions-4*gap;
    objectPicker(model,ui,room*.40F);ImGui::SameLine();examplePicker(model,ui,room*.26F);ImGui::SameLine();layerPicker(model,ui,room*.34F);ImGui::SameLine();toggles();
  } else {
    objectPicker(model,ui,width-nav-gap);
    if(layout.toolbarRows==2) {
      const float room=width-actions-2*gap;
      examplePicker(model,ui,room*.44F);ImGui::SameLine();layerPicker(model,ui,room*.56F);ImGui::SameLine();toggles();
    } else {
      examplePicker(model,ui,(width-gap)*.44F);ImGui::SameLine();layerPicker(model,ui,(width-gap)*.56F);toggles();
    }
  }
  ImGui::End();
}
void controlGroups(const MathObjects& model,UiState& ui) {
  const auto rows=mathControlRows(model);
  ImGui::PushID(static_cast<int>(model.snapshot().kind));ImGui::PushID(static_cast<int>(model.snapshot().level));
  for(unsigned g=0;g<static_cast<unsigned>(MathControlGroup::Count);++g) {
    const auto group=static_cast<MathControlGroup>(g);
    bool present=false;for(unsigned i=0;i<rows.count;++i)present=present||rows.rows[i].group==group;
    if(!present)continue;
    ImGui::PushID(static_cast<int>(g));const auto changed=mathChangedControlCount(model,group);
    std::string title(mathControlGroupName(group));if(changed)title+=" ("+std::to_string(changed)+" changed)";
    ImGui::SetNextItemOpen(ui.memory.groupOpen(model,group),ImGuiCond_Always);
    const float resetWidth=28*ImGui::GetFontSize()/16.f;
    ImGui::SetNextItemWidth(std::max(40.f,ImGui::GetContentRegionAvail().x-resetWidth));
    const bool open=ImGui::TreeNodeEx("group",ImGuiTreeNodeFlags_Framed|ImGuiTreeNodeFlags_NoTreePushOnOpen|ImGuiTreeNodeFlags_AllowOverlap,"%s",title.c_str());
    ui.memory.setGroupOpen(model,group,open);
    // Tree headers span the row; place a distinct reset action at their right edge.
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x-resetWidth);
    ImGui::BeginDisabled(!changed);if(ImGui::SmallButton("R"))queue(ui,mathResetControlGroup(model,group));ImGui::EndDisabled();
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))ImGui::SetTooltip("Reset every %s parameter to defaults, including hidden controls",mathControlGroupName(group).data());
    if(open)for(unsigned i=0;i<rows.count;++i)if(rows.rows[i].group==group) {
      const auto& row=rows.rows[i];MathControlEdit edit;
      if(row.count==1) {
        const auto p=row.parameters[0];edit=compactMathControl(mathParameterSpecs()[static_cast<unsigned>(p)],row.label,model.parameter(p),mathControlRange(model,p),true);
      } else {
        std::array<double,3> values{};std::array<MathControlRange,3> ranges{};
        for(unsigned j=0;j<row.count;++j){values[j]=model.parameter(row.parameters[j]);ranges[j]=mathControlRange(model,row.parameters[j]);}
        edit=compactMathTuple(row,values,ranges);
      }
      if(edit.reset) {MathAction action{MathActionKind::ResetParameters};for(unsigned j=0;j<row.count;++j)action.resetParameters.set(static_cast<unsigned>(row.parameters[j]));queue(ui,action);}
      else if(edit.changed)queue(ui,{MathActionKind::SetParameter,{},row.parameters[edit.component],edit.value});
    }
    ImGui::PopID();
  }
  ImGui::PopID();ImGui::PopID();
}
void routeControls(const MathObjects& model,UiState& ui) {
  const auto& snapshot=model.snapshot();if(snapshot.kind!=MathObjectKind::Discrete)return;
  std::string route;for(unsigned i=0;i<snapshot.routeCount;++i){if(i)route+=" > ";route+=static_cast<char>('A'+snapshot.route[i]);}
  ImGui::TextWrapped("Route: %s",route.c_str());
  for(unsigned i=0;i<8;++i) {
    if(i%4)ImGui::SameLine();const char label[]{static_cast<char>('A'+i),'\0'};
    ImGui::BeginDisabled(!snapshot.allowedVertices[i]||snapshot.routeCount==snapshot.route.size());
    if(ImGui::Button(label,{34,0}))queue(ui,{MathActionKind::VisitVertex,{},{},0,i});ImGui::EndDisabled();
  }
  ImGui::BeginDisabled(snapshot.routeCount<=1);if(ImGui::Button("Undo step"))queue(ui,{MathActionKind::UndoRoute});ImGui::EndDisabled();ImGui::SameLine();
  if(ImGui::Button("Restart route"))queue(ui,{MathActionKind::ResetRoute});
}
void inspector(MathObjects& model,UiState& ui,const MathLabLayout& layout) {
  const auto r=layout.inspector;if(r.width<=0)return;
  window("Object controls",{r.x,r.y},{r.width,r.height});
  ImGui::TextUnformatted("CONTROLS");ImGui::SameLine();
  if(ImGui::SmallButton("Reset all")){queue(ui,{MathActionKind::Reset});ui.rememberDefaults=true;}
  ImGui::SameLine();if(ImGui::SmallButton("Hide"))ui.inspector=false;
  ImGui::TextWrapped("%s",mathObjectSpecs()[static_cast<unsigned>(model.snapshot().kind)].title.data());
  controlGroups(model,ui);
  if(model.snapshot().kind==MathObjectKind::Linear&&model.snapshot().level>0) {
    if(ImGui::Button("Edit matrix in Analysis")){ui.drawer=true;ui.requestTab=0;}
  }
  ImGui::Separator();explorationControls(model,ui);routeControls(model,ui);
  if(model.snapshot().kind==MathObjectKind::Function&&model.snapshot().level>=2&&ImGui::Button("Swap bounds"))queue(ui,{MathActionKind::SwapBounds});
  if(model.snapshot().kind==MathObjectKind::Surface&&model.parameterAvailable(MathParameter::DescentRate)&&ImGui::Button("Take a descent step"))queue(ui,{MathActionKind::DescentStep});
  if(ImGui::Button("Exercise")){ui.drawer=true;ui.requestTab=3;}ImGui::SameLine();if(ImGui::Button("Read the math")){ui.drawer=true;ui.requestTab=2;}
  if(!ui.message.empty())ImGui::TextWrapped("%s",ui.message.c_str());
  ImGui::End();
}
void viewTools(const MathObjects& model,MathObjectScene& scene,UiState& ui,const SceneViewport& r) {
  window("View tools",{r.x,r.y},{r.width,r.height},ImGuiWindowFlags_NoScrollbar);
  if(ImGui::SmallButton("Reset camera"))scene.resetView();ImGui::SameLine();
  if(ImGui::SmallButton("View"))ImGui::OpenPopup("View settings");
  if(ImGui::BeginPopup("View settings")) {
    ImGui::TextDisabled("Labels");const std::array<const char*,3> labels{"All labels","Hover / selected","Hide labels"};
    for(unsigned i=0;i<labels.size();++i)if(ImGui::MenuItem(labels[i],nullptr,ui.labels==i))ui.labels=i;
    ImGui::Separator();ImGui::TextUnformatted("Left drag: orbit\nRight drag: pan\nScroll: zoom");ImGui::EndPopup();
  }
  constexpr std::array guides{MathParameter::NormWire,MathParameter::CurveGuides,MathParameter::LatheGuides,MathParameter::BooleanGuides,MathParameter::PatchGuides,MathParameter::MembraneGuides,MathParameter::RigidGuides,MathParameter::TrussGuides,MathParameter::SimplexGuides,MathParameter::DistanceGuides,MathParameter::PolarGuides};
  for(auto p:guides)if(model.parameterAvailable(p)) {ImGui::SameLine();bool enabled=model.parameter(p)!=0;if(ImGui::Checkbox(p==MathParameter::NormWire?"Wire":"Guides",&enabled))queue(ui,{MathActionKind::SetParameter,{},p,enabled?1.:0.});}
  if(model.parameterAvailable(MathParameter::BooleanSection)) {ImGui::SameLine();bool enabled=model.parameter(MathParameter::BooleanSection)!=0;if(ImGui::Checkbox("Section",&enabled))queue(ui,{MathActionKind::SetParameter,{},MathParameter::BooleanSection,enabled?1.:0.});}
  if(model.parameterAvailable(MathParameter::LatheCut)) {ImGui::SameLine();if(ImGui::SmallButton("Cutaway"))ImGui::OpenPopup("Cutaway amount");if(ImGui::BeginPopup("Cutaway amount")){const auto p=MathParameter::LatheCut;const auto edit=compactMathControl(mathParameterSpecs()[static_cast<unsigned>(p)],"Cutaway (%)",model.parameter(p),mathControlRange(model,p));if(edit.changed)queue(ui,{MathActionKind::SetParameter,{},p,edit.value});ImGui::EndPopup();}}
  ImGui::End();
}
void measurements(const MathObjectSnapshot& snapshot,const SceneViewport& r) {
  if(r.height<=0)return;
  window("Key measurements",{r.x,r.y},{r.width,r.height},ImGuiWindowFlags_NoScrollbar);
  const auto count=std::min<std::size_t>(snapshot.metricCount,r.width<440?2:3);
  if(count&&ImGui::BeginTable("key values",static_cast<int>(count),ImGuiTableFlags_SizingStretchSame)) {
    for(unsigned i=0;i<count;++i){ImGui::TableNextColumn();const auto& m=snapshot.metrics[i];ImGui::TextDisabled("%s",m.label.data());if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",m.label.data());ImGui::Text("%.4g %s",m.value,m.suffix.empty()?"":m.suffix.data());}ImGui::EndTable();
  }
  ImGui::End();
}
void analysisDrawer(const MathObjects& model,UiState& ui,const SceneViewport& r) {
  if(r.height<=0)return;window("Object analysis",{r.x,r.y},{r.width,r.height});
  const auto& snapshot=model.snapshot();const auto& spec=mathObjectSpecs()[static_cast<unsigned>(snapshot.kind)];
  const auto lessons=mathLessons(snapshot.kind);const auto* lesson=lessons.empty()?nullptr:&lessons[snapshot.level];
  if(ImGui::BeginTabBar("Analysis tabs")) {
    const auto flags=[&](unsigned i){return ui.requestTab==static_cast<int>(i)?ImGuiTabItemFlags_SetSelected:ImGuiTabItemFlags_None;};
    if(ImGui::BeginTabItem("Graph",nullptr,flags(0))){linkedViews(model,ui);ImGui::EndTabItem();}
    if(ImGui::BeginTabItem("Values",nullptr,flags(1))) {
      for(unsigned i=0;i<snapshot.metricCount;++i){const auto& m=snapshot.metrics[i];ImGui::TextWrapped("%s: %.6g %s",m.label.data(),m.value,m.suffix.empty()?"":m.suffix.data());}
      if(snapshot.table.rowCount){ImGui::Separator();valueTable(snapshot.table);}ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Math",nullptr,flags(2))) {
      ImGui::TextWrapped("%s",lesson?lesson->relationship.data():spec.relationship.data());
      ImGui::Separator();if(lesson)ImGui::TextWrapped("%s",lesson->explanation.data());else for(auto text:spec.progression)ImGui::BulletText("%s",text.data());
      ImGui::Spacing();ImGui::TextWrapped("%s",spec.convention.data());ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Exercise",nullptr,flags(3))) {
      ImGui::TextWrapped("%s",lesson?lesson->challenge.data():spec.challenge.data());
      if(ImGui::Button("Check my solution"))queue(ui,{MathActionKind::Check});
      if(snapshot.feedback!=MathFeedback::None) {ImGui::PushStyleColor(ImGuiCol_Text,snapshot.feedback==MathFeedback::Solved?ImVec4{.35F,.9F,.66F,1}:ImVec4{.98F,.77F,.35F,1});ImGui::TextWrapped("%s",snapshot.feedbackText.data());ImGui::PopStyleColor();}
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();ui.requestTab=-1;
  }
  ImGui::End();
}
void resizeHandle(const char* name,SceneViewport r,float& preference,float extent,bool vertical) {
  if(r.width<=0||r.height<=0)return;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});window(name,{r.x,r.y},{r.width,r.height},ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
  ImGui::InvisibleButton("resize",{r.width,r.height});
  if(ImGui::IsItemHovered()||ImGui::IsItemActive()) {
    ImGui::SetMouseCursor(vertical?ImGuiMouseCursor_ResizeEW:ImGuiMouseCursor_ResizeNS);
    ImGui::GetWindowDrawList()->AddRectFilled({r.x,r.y},{r.x+r.width,r.y+r.height},IM_COL32(72,186,172,120));
    if(ImGui::IsItemActive())preference=extent-(vertical?ImGui::GetIO().MouseDelta.x:ImGui::GetIO().MouseDelta.y);
  }
  ImGui::End();ImGui::PopStyleVar();
}
bool inside(ImVec2 point,SceneViewport r) {return r.width>0&&r.height>0&&point.x>=r.x&&point.y>=r.y&&point.x<r.x+r.width&&point.y<r.y+r.height;}
void draw(MathObjects& model,MathObjectScene& scene,UiState& ui,LabPage& page) {
  if(ui.hasPending){const auto result=model.dispatch(ui.pending);ui.message=result.accepted?"":std::string(result.reason);ui.hasPending=false;if(result.accepted&&ui.rememberDefaults)ui.memory.rememberExample(model,"Defaults");ui.rememberDefaults=false;}
  if(model.snapshot().playing)apply(model,{MathActionKind::AdvanceTime,{},{},std::clamp(static_cast<double>(ImGui::GetIO().DeltaTime),.001,.1)});
  const auto& io=ImGui::GetIO();if(io.DisplaySize.x<320||io.DisplaySize.y<320)return;
  const float scale=std::clamp(ImGui::GetFontSize()/16.f,.75f,2.f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize,{1,1});ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{8*scale,6*scale});
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{4*scale,3*scale});ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{6*scale,4*scale});
  const auto plan=[&]{return planMathLabLayout({{0,0,io.DisplaySize.x,io.DisplaySize.y},scale,ui.inspectorWidth,ui.drawerHeight,ui.inspector,ui.drawer});};
  ui.memory.visit(model);toolbar(model,ui,page,plan(),scale);const auto layout=plan();const auto& snapshot=model.snapshot();
  viewTools(model,scene,ui,layout.viewTools);measurements(snapshot,layout.metrics);
  const auto viewport=layout.viewport;static_cast<void>(scene.publish(snapshot,viewport));
  window("Object viewport",{viewport.x,viewport.y},{viewport.width,viewport.height},ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
  ImGui::SetCursorPos({0,0});const bool covered=layout.overlayInspector&&inside(io.MousePos,layout.inspector);
  ImGui::BeginDisabled(covered);ImGui::InvisibleButton("3D object",{viewport.width,viewport.height},ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonRight);
  if(!covered&&snapshot.curve.active&&ImGui::IsItemHovered()&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    float nearest=14*14*scale*scale;unsigned selected=snapshot.curve.selected;
    for(unsigned i=0;i<snapshot.curve.count;++i){const auto at=scene.project(snapshot.curve.controls[i]);if(at.z<0)continue;const float dx=at.x-io.MousePos.x,dy=at.y-io.MousePos.y,distance=dx*dx+dy*dy;if(distance<nearest){nearest=distance;selected=i;}}
    if(selected!=snapshot.curve.selected)queue(ui,{MathActionKind::SetParameter,{},snapshot.curve.selectionParameter,static_cast<double>(selected)});
  }
  if(!covered&&ImGui::IsItemActive()) {
    if(ImGui::IsMouseDragging(ImGuiMouseButton_Left))static_cast<void>(scene.navigate(Navigation::Orbit,io.MouseDelta.x,io.MouseDelta.y));
    if(ImGui::IsMouseDragging(ImGuiMouseButton_Right))static_cast<void>(scene.navigate(Navigation::Pan,io.MouseDelta.x,io.MouseDelta.y));
  }
  if(!covered&&ImGui::IsItemHovered()&&io.MouseWheel!=0)static_cast<void>(scene.navigate(Navigation::Dolly,0,io.MouseWheel));
  const bool hovered=!covered&&ImGui::IsItemHovered();ImGui::EndDisabled();static_cast<void>(scene.publish(snapshot,viewport));
  auto* overlay=ImGui::GetWindowDrawList();overlay->PushClipRect({viewport.x,viewport.y},{viewport.x+viewport.width,viewport.y+viewport.height},true);
  if(ui.labels!=2)for(unsigned i=0;i<snapshot.labelCount;++i) {
    const auto& label=snapshot.labels[i];const auto p=scene.project(label.position);if(p.z<0)continue;
    bool selected=false;if(snapshot.curve.active){const auto q=scene.project(snapshot.curve.controls[snapshot.curve.selected]);selected=std::hypot(p.x-q.x,p.y-q.y)<24*scale;}
    if(ui.labels==1&&!selected&&(!hovered||std::hypot(p.x-io.MousePos.x,p.y-io.MousePos.y)>32*scale))continue;
    const auto extent=ImGui::CalcTextSize(label.text.data());const ImVec2 at{p.x-extent.x*.5F,p.y-extent.y*.5F};
    overlay->AddRectFilled({at.x-4,at.y-2},{at.x+extent.x+4,at.y+extent.y+2},IM_COL32(12,21,31,210),3);overlay->AddText(at,colour(label.color),label.text.data());
  }
  overlay->PopClipRect();ImGui::End();
  analysisDrawer(model,ui,layout.drawer);
  resizeHandle("Resize controls",layout.inspectorDivider,ui.inspectorWidth,layout.inspector.width,true);
  resizeHandle("Resize analysis",layout.drawerDivider,ui.drawerHeight,layout.drawer.height,false);
  inspector(model,ui,layout);ImGui::PopStyleVar(4);
}
} // namespace
int main(int argc,char** argv) {
  try {
    const auto options=parse(argc,argv);MathObjects model;for(const auto& action:options.actions)apply(model,action);
    MathObjectScene scene;UiState ui;
    MatrixBoard board;MatrixBoardUiState boardUi;LabPage page=options.page;
    Textbook book;TextbookUiState bookUi;bookUi.presentation=options.bookPresentation;
    const auto applyBookActions=[&]{for(const auto action:options.bookActions){const auto result=book.dispatch(action);if(!result.accepted)throw std::invalid_argument(result.reason);}};
    if(options.validate&&page==LabPage::Textbook){
      applyBookActions();for(const auto& action:options.systemActions){const auto result=book.systems().dispatch(action);if(!result.accepted)throw std::invalid_argument(result.reason);}
      const auto v=book.view();const auto& section=matrixChapter()[v.section];
      const auto matrix=book.hasBoard()?book.board().view():MatrixBoardView{};
      std::size_t terms=0,openHelp=0;for(const auto& section:matrixChapter())terms+=section.terms.size();
      const auto lesson=book.lessonView();for(const auto& block:lesson)for(const auto& help:block.help)openHelp+=help.open;
      std::printf("textbook text validation: sections=%zu terms=%zu section=%u id=%s page=%u mode=%u card=%03u working=%d checked=%d blocks=%zu open_help=%zu bookmark_io=0\n",matrixChapter().size(),terms,v.section+1,section.id,static_cast<unsigned>(v.page),static_cast<unsigned>(v.mode),section.card,matrix.working,matrix.checked,lesson.size(),openHelp);
      const bool objectExercise=v.mode==BookMode::Exercise&&section.exercise==BookExerciseKind::Object;
      const bool hasFigure=v.page==BookPage::Section&&(v.mode==BookMode::Reading||objectExercise)&&section.figure.kind!=BookFigureKind::None;
      const auto spread=planLessonSpread({static_cast<float>(options.native.width),static_cast<float>(options.native.height),true,hasFigure,objectExercise?LessonPresentation::Figure:bookUi.presentation});
      if(matrixChapter()[v.section].figure.kind==BookFigureKind::AffinePlanes){
        const auto state=book.systems().view(v.mode==BookMode::Exercise);
        std::printf("systems lesson text validation: practice=%d revealed=%d attempts=%zu",state.practice,state.revealed,state.attempts.size());
        if(state.solution)std::printf(" outcome=%u rank=%u augmented_rank=%u nullity=%u",static_cast<unsigned>(systemOutcome(*state.solution)),state.solution->rank,state.solution->augmentedRank,state.solution->dimension);
        std::putchar('\n');
      }
      if(hasFigure&&section.figure.kind==BookFigureKind::Object){
        const auto& state=book.objectLesson().snapshot(objectExercise);std::size_t vertices=0,indices=0;
        if(spread.showFigure){const auto regions=planLessonFigureRegions(spread.figure);const auto& frame=bookUi.figure.scene.publish(state,regions.viewport);vertices=frame.vertices.size();indices=frame.indices.size();}
        std::printf("object lesson text validation: available=1 practice=%d feedback=%u split=%d visible=%d vertices=%zu indices=%zu\n",objectExercise,static_cast<unsigned>(state.feedback),spread.split,spread.showFigure,vertices,indices);
        for(unsigned i=0;i<state.metricCount;++i)std::printf("  %s = %.12g\n",state.metrics[i].label.data(),state.metrics[i].value);
      }else if(hasFigure){
        const auto& figure=bookUi.figure.model.publish(matrix);std::size_t vertices=0,indices=0;
        if(figure.available&&spread.showFigure){const auto regions=planLessonFigureRegions(spread.figure);const auto& frame=bookUi.figure.scene.publish(bookUi.figure.model.geometry(),regions.viewport);vertices=frame.vertices.size();indices=frame.indices.size();}
        std::printf("lesson figure text validation: available=%d rows=%u rank=%u nullity=%u agrees=%d split=%d visible=%d vertices=%zu indices=%zu consistent=%d augmented_rank=%u\n",figure.available,figure.rows,figure.rank,figure.dimension,figure.agrees,spread.split,spread.showFigure,vertices,indices,figure.consistent,figure.augmentedRank);
      }
      return 0;
    }
    for(const auto& action:options.systemActions){const auto result=book.systems().dispatch(action);if(!result.accepted)throw std::invalid_argument(result.reason);}
    for(const auto& action:options.boardActions){const auto result=board.dispatch(action);if(!result.accepted)throw std::invalid_argument(result.reason);}
    if(options.validate&&page==LabPage::MatrixBoard){
      const auto v=board.view();std::printf("matrix_board text validation: card=%03u case=%u rows=%u cols=%u steps=%u working=%d complete=%d blocked=%d checked=%d passed=%d residual=%.12g tolerance=%.12g\n",v.card,v.example,v.given.rows,v.given.cols,v.steps,v.working,v.complete,v.blocked,v.checked,v.passed,v.residual,v.tolerance);
      std::printf("%s\n%s\n",v.status.c_str(),v.evidence.c_str());return v.checked&&!v.passed?2:0;
    }
    if(options.validate) {
      const auto layout=planMathLabLayout({{0,0,static_cast<float>(options.native.width),static_cast<float>(options.native.height)},1,360,200,true,true});
      const auto rows=mathControlRows(model);
      std::printf("compact layout text validation: toolbar_rows=%u overlay_controls=%d control_rows=%u viewport=%.0fx%.0f drawer=%.0f\n",layout.toolbarRows,layout.overlayInspector,rows.count,layout.viewport.width,layout.viewport.height,layout.drawer.height);
      const auto& frame=scene.publish(model.snapshot(),{0,0,static_cast<float>(options.native.width),static_cast<float>(options.native.height)});
      std::printf("math_lab text validation: object=%s level=%u vertices=%zu indices=%zu plots=%zu matrices=%zu contours=%zu feedback=%u\n",mathObjectSpecs()[static_cast<std::size_t>(model.snapshot().kind)].key.data(),model.snapshot().level,frame.vertices.size(),frame.indices.size(),model.snapshot().plotCount,model.snapshot().matrixCount,model.snapshot().contours.count,static_cast<unsigned>(model.snapshot().feedback));
      for(std::size_t i=0;i<model.snapshot().metricCount;++i){const auto& metric=model.snapshot().metrics[i];std::printf("  %s = %.12g\n",metric.label.data(),metric.value);}
      const auto& table=model.snapshot().table;
      if(table.rowCount){std::printf("  Table: %s\n",table.title.data());for(std::size_t r=0;r<table.rowCount;++r){std::printf("    %s:",table.rowLabels[r].data());for(std::size_t c=0;c<table.columnCount;++c)std::printf(" %s=%.12g",table.columns[c].data(),table.values[r][c]);std::putchar('\n');}}
      return 0;
    }
    std::filesystem::path bookmarkPath;
    if(char* folder=SDL_GetPrefPath("Paths","MathLab")){bookmarkPath=std::filesystem::path(folder)/"reading-v1.txt";SDL_free(folder);}
    if(!bookmarkPath.empty()){const auto result=readTextbookBookmark(bookmarkPath,book);if(!result.accepted)bookUi.message=result.reason;}
    std::string lastSavedBookmark=book.bookmark();applyBookActions();
    auto lastBookmarkSave=std::chrono::steady_clock::now();
    const auto saveBookmark=[&]{if(bookmarkPath.empty()||book.bookmark()==lastSavedBookmark)return;const auto result=writeTextbookBookmark(bookmarkPath,book);if(result.accepted)lastSavedBookmark=book.bookmark();else bookUi.message=result.reason;};
    NativeVulkanHost host(options.native);
    NativeMath math(std::filesystem::path(SDL_GetBasePath())/"math_typesetter");
    auto& style=ImGui::GetStyle();style.WindowPadding={15,13};style.FramePadding={8,5};style.ItemSpacing={7,8};style.FrameRounding=4;
    style.Colors[ImGuiCol_WindowBg]={.035F,.055F,.08F,1};style.Colors[ImGuiCol_Button]={.12F,.26F,.30F,1};
    // The host reads this stable packet after the UI callback chooses the active pane.
    SceneFrame presented;presented.vertices.reserve(kSceneVertexCapacity);presented.indices.reserve(kSceneIndexCapacity);presented.draws.reserve(MathObjectSnapshot::kPartCapacity);
    unsigned frames=0,skipped=0;
    while(!ui.quit&&(!options.frames||frames<options.frames)) {
      const auto result=host.frame([&](const SDL_Event& event){if(event.type==SDL_EVENT_QUIT)ui.quit=true;},[&]{
        presented.vertices.clear();presented.indices.clear();presented.draws.clear();
        switch(page){
        case LabPage::Textbook:if(drawTextbook(book,bookUi,math,presented))page=LabPage::Objects;break;
        case LabPage::Objects:draw(model,scene,ui,page);presented=scene.frame();break;
        case LabPage::MatrixBoard:{bool stay=true;drawMatrixBoard(board,boardUi,stay,"Textbook");if(!stay)page=LabPage::Textbook;break;}
      }},&presented);
      if(result.status==FrameStatus::Failed)throw std::runtime_error(result.error);if(result.status==FrameStatus::Closed)break;
      if(result.status==FrameStatus::Skipped){if(++skipped>1000)throw std::runtime_error("Too many skipped native frames");continue;}
      skipped=0;++frames;
      const auto now=std::chrono::steady_clock::now();if(now-lastBookmarkSave>std::chrono::seconds(2)){saveBookmark();lastBookmarkSave=now;}
    }
    saveBookmark();
    if(!options.capture.empty()) {
      if(!options.capture.parent_path().empty())std::filesystem::create_directories(options.capture.parent_path());
      std::string error;if(!host.capture(capturePaths(options.capture),error))throw std::runtime_error(error);
    }
    std::printf("math_lab object=%s vertices=%zu indices=%zu feedback=%u\n",mathObjectSpecs()[static_cast<std::size_t>(model.snapshot().kind)].key.data(),scene.frame().vertices.size(),scene.frame().indices.size(),static_cast<unsigned>(model.snapshot().feedback));
    return 0;
  }catch(const std::exception& error){std::fprintf(stderr,"math_lab: %s\n",error.what());return 1;}
}

#include "runtime/math_objects/MathObjects.hpp"
#include "scene/MathObjectScene.hpp"
#include "platform/NativeVulkanHost.hpp"

#include <SDL3/SDL.h>
#include "imgui.h"

#include <algorithm>
#include <charconv>
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
struct Options {
  NativeLaunchConfig native{false,1440,900,true};
  unsigned frames=0;
  bool validate=false;
  std::filesystem::path capture;
  std::vector<MathAction> actions;
};
double number(std::string_view text) {
  double result=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
  if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||!std::isfinite(result))throw std::invalid_argument("Expected a finite number");return result;
}
unsigned integer(std::string_view text,unsigned minimum,unsigned maximum) {
  const double result=number(text);if(result<minimum||result>maximum||std::floor(result)!=result)throw std::invalid_argument("Integer option out of range");return static_cast<unsigned>(result);
}
enum class Flag { Offscreen,Frames,Capture,Resolution,Object,Set,Route,Check,Level,Preset,Descent,Swap,Validate,Turn,UndoTurn,Identity,Advance,ModularStep,ResetWalk,ReversePath,WalkStep,ResetProbability,TrialStep,ResetTrials,Help };
Options parse(int argc,char** argv) {
  constexpr std::array<std::pair<std::string_view,Flag>,25> flags{{
    {"--offscreen",Flag::Offscreen},{"--frames",Flag::Frames},{"--capture",Flag::Capture},
    {"--resolution",Flag::Resolution},{"--object",Flag::Object},{"--set",Flag::Set},
    {"--route",Flag::Route},{"--check",Flag::Check},{"--level",Flag::Level},
    {"--preset",Flag::Preset},{"--descent",Flag::Descent},{"--swap-bounds",Flag::Swap},
    {"--validate",Flag::Validate},{"--turn",Flag::Turn},{"--undo-turn",Flag::UndoTurn},
    {"--identity",Flag::Identity},{"--advance",Flag::Advance},{"--step",Flag::ModularStep},
    {"--reset-walk",Flag::ResetWalk},{"--reverse-path",Flag::ReversePath},{"--walk-step",Flag::WalkStep},{"--reset-probability-walk",Flag::ResetProbability},{"--trial-step",Flag::TrialStep},{"--reset-trials",Flag::ResetTrials},{"--help",Flag::Help}}};
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
      case Flag::Help:
        std::puts("math_lab [--object algebra|trig|calculus|linear|discrete|function|surface|symmetry|harmonics|oscillator|modular|gaussian|field|flux|tensor|probability|binomial|bayes|covariance|spherical|quadratic|roots] [--level 0..3]\n         [--set key=value] [--preset 0..4] [--descent] [--swap-bounds] [--route BDH] [--check]\n         [--turn x|y|z|x-inverse|y-inverse|z-inverse] [--undo-turn] [--identity] [--advance duration]\n         [--step 1|-1] [--reset-walk] [--reverse-path] [--walk-step] [--reset-probability-walk] [--trial-step] [--reset-trials]\n         [--validate] [--offscreen] [--frames N] [--resolution 1440x900] [--capture /path/view.png]\n--validate computes geometry and prints measurements without creating a native host or images.\nArguments apply in order: select the object and level before setting its parameters.\nMatrix presets: 0 identity, 1 shear, 2 xy projection, 3 stretch/reflection, 4 z rotation.");
        for(const auto& p:mathParameterSpecs())std::printf("  %s [%g,%g]  %s (level %u+)\n",p.key.data(),p.minimum,p.maximum,p.label.data(),p.minimumLevel);
        std::exit(0);
    }
  }
  if(options.native.offscreen&&!options.frames)options.frames=3;
  if(!options.capture.empty()&&!options.frames)options.frames=3;
  if(options.validate&&!options.capture.empty())throw std::invalid_argument("Text validation cannot capture images");
  return options;
}
struct UiState { bool quit=false,hasPending=false;MathAction pending{}; };
constexpr auto windowFlags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBringToFrontOnFocus;
void window(const char* name,ImVec2 position,ImVec2 size,ImGuiWindowFlags extra=0) {
  ImGui::SetNextWindowPos(position);ImGui::SetNextWindowSize(size);ImGui::Begin(name,nullptr,windowFlags|extra);
}
void apply(MathObjects& objects,MathAction action) {
  const auto result=objects.dispatch(action);if(!result.accepted)throw std::runtime_error(std::string(result.reason));
}
void queue(UiState& ui,MathAction action) { ui.pending=action;ui.hasPending=true; }
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
  if(ImGui::BeginTable("matrix",3,ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_BordersInner)) {
    for(unsigned i=0;i<9;++i) {
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
void explorationControls(MathObjects& model) {
  const auto& s=model.snapshot();
  if(s.kind==MathObjectKind::Symmetry&&s.level<2) {
    static constexpr std::array<const char*,6> labels{"X +90","Y +90","Z +90","X -90","Y -90","Z -90"};
    ImGui::TextUnformatted("Turn about a fixed world axis");ImGui::BeginDisabled(s.symmetry.moveCount==s.symmetry.moves.size());
    for(unsigned i=0;i<6;++i){if(i%3)ImGui::SameLine();if(ImGui::Button(labels[i]))apply(model,{MathActionKind::SymmetryTurn,{},{},0,i});}
    if(s.level==1){if(ImGui::Button("Apply first"))apply(model,{MathActionKind::SymmetryTurn,{},{},0,static_cast<unsigned>(model.parameter(MathParameter::SymmetryFirst))});ImGui::SameLine();if(ImGui::Button("Apply second"))apply(model,{MathActionKind::SymmetryTurn,{},{},0,static_cast<unsigned>(model.parameter(MathParameter::SymmetrySecond))});}
    ImGui::EndDisabled();ImGui::BeginDisabled(s.symmetry.moveCount==0);if(ImGui::Button("Undo turn"))apply(model,{MathActionKind::SymmetryUndo});ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Return to identity"))apply(model,{MathActionKind::SymmetryIdentity});
  }
  if(s.kind==MathObjectKind::Modular&&s.level==1) {
    ImGui::BeginDisabled(s.modularWalkSteps==64);
    if(ImGui::Button("Step forward"))apply(model,{MathActionKind::ModularStep,{},{},1});ImGui::SameLine();if(ImGui::Button("Step backward"))apply(model,{MathActionKind::ModularStep,{},{},-1});
    ImGui::EndDisabled();if(ImGui::Button("Reset walk"))apply(model,{MathActionKind::ResetModularWalk});
    if(s.modularWalkSteps==64)ImGui::TextWrapped("Walk limit reached. Reset to explore another cycle.");
  }
  if(s.kind==MathObjectKind::VectorField&&s.level>0) {
    if(ImGui::Button("Reverse path"))apply(model,{MathActionKind::ReverseFieldPath});ImGui::SameLine();ImGui::TextUnformatted(s.fieldPathReversed?"Reversed":"Forward");
  }
  if(s.kind==MathObjectKind::Probability&&s.level==0) {
    ImGui::BeginDisabled(s.probabilityWalkCount==s.probabilityWalk.size());if(ImGui::Button("Next walk step"))apply(model,{MathActionKind::ProbabilityStep});ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Reset walk"))apply(model,{MathActionKind::ResetProbabilityWalk});
    if(s.probabilityWalkCount==s.probabilityWalk.size())ImGui::TextWrapped("Walk limit reached. Reset to start again.");
  }
  if(s.kind==MathObjectKind::Probability&&s.level>=2) {
    const auto p=MathParameter::ProbabilitySteps;ImGui::BeginDisabled(model.parameter(p)>=64);if(ImGui::Button("Next distribution"))apply(model,{MathActionKind::SetParameter,{},p,model.parameter(p)+1});ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Restart distribution"))apply(model,{MathActionKind::SetParameter,{},p,0});
  }
  if(s.kind==MathObjectKind::Binomial&&s.level==0) {
    ImGui::BeginDisabled(s.bernoulliSteps>=model.parameter(MathParameter::BinomialTrials));if(ImGui::Button("Next trial"))apply(model,{MathActionKind::BernoulliStep});ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Reset trials"))apply(model,{MathActionKind::ResetBernoulli});
    if(s.bernoulliSteps>=model.parameter(MathParameter::BinomialTrials))ImGui::TextWrapped("Trial path complete. Reset to replay it, or change the seed for another path.");
  }
  const auto time=model.playbackParameter();
  if(model.parameterAvailable(time)) {
    const bool atEnd=model.parameter(time)>=mathParameterSpecs()[static_cast<std::size_t>(time)].maximum;
    ImGui::BeginDisabled(atEnd);if(ImGui::Button(s.playing?"Pause":"Play"))apply(model,{MathActionKind::TogglePlayback});ImGui::SameLine();if(ImGui::Button("Advance 0.1"))apply(model,{MathActionKind::AdvanceTime,{},{},.1});ImGui::EndDisabled();ImGui::SameLine();
    if(ImGui::Button("Restart time"))apply(model,{MathActionKind::SetParameter,{},time,0});
    if(atEnd)ImGui::TextWrapped("Time window complete. Restart or scrub time to explore again.");
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
  if(snapshot.table.rowCount) {
    if((snapshot.plotCount||snapshot.matrixCount||snapshot.contours.active)&&ImGui::BeginTabBar("number and field views")) {
      if(ImGui::BeginTabItem("Values")){valueTable(snapshot.table);ImGui::EndTabItem();}
      if(snapshot.contours.active&&ImGui::BeginTabItem("Zero set")){contourView(snapshot.contours,ui,snapshot.kind==MathObjectKind::Quadratic);ImGui::EndTabItem();}
      if(snapshot.matrixCount&&ImGui::BeginTabItem("Matrices")){panels(snapshot.matrixCount,[&](std::size_t i,bool show){if(show)matrixView(snapshot.matrices[i],model,ui);return snapshot.matrices[i].name;});ImGui::EndTabItem();}
      if(snapshot.plotCount&&ImGui::BeginTabItem("Linked plots")){panels(snapshot.plotCount,[&](std::size_t i,bool show){if(show)plotView(snapshot.plots[i],model,ui);return snapshot.plots[i].title;});ImGui::EndTabItem();}
      ImGui::EndTabBar();
    } else if(!snapshot.plotCount&&!snapshot.matrixCount&&!snapshot.contours.active)valueTable(snapshot.table);
  } else if(snapshot.symmetry.active) {
    if(ImGui::BeginTabBar("symmetry views")) {
      if(ImGui::BeginTabItem("Vertex permutation")){symmetryView(snapshot.symmetry);ImGui::EndTabItem();}
      if(ImGui::BeginTabItem("Rotation matrices")){panels(snapshot.matrixCount,[&](std::size_t i,bool show){if(show)matrixView(snapshot.matrices[i],model,ui);return snapshot.matrices[i].name;});ImGui::EndTabItem();}
      ImGui::EndTabBar();
    }
  } else if(snapshot.contours.active) {
    if(ImGui::BeginTabBar("surface views")) {
      if(ImGui::BeginTabItem("Contour map")){contourView(snapshot.contours,ui);ImGui::EndTabItem();}
      if(ImGui::BeginTabItem("Linked sections")){panels(snapshot.plotCount,[&](std::size_t i,bool show){if(show)plotView(snapshot.plots[i],model,ui);return snapshot.plots[i].title;});ImGui::EndTabItem();}
      ImGui::EndTabBar();
    }
  } else if(snapshot.plotCount)panels(snapshot.plotCount,[&](std::size_t i,bool show){if(show)plotView(snapshot.plots[i],model,ui);return snapshot.plots[i].title;});
  else if(snapshot.matrixCount)panels(snapshot.matrixCount,[&](std::size_t i,bool show){if(show)matrixView(snapshot.matrices[i],model,ui);return snapshot.matrices[i].name;});
}
void draw(MathObjects& model,MathObjectScene& scene,UiState& ui) {
  if(ui.hasPending){apply(model,ui.pending);ui.hasPending=false;}
  if(model.snapshot().playing)apply(model,{MathActionKind::AdvanceTime,{},{},std::clamp(static_cast<double>(ImGui::GetIO().DeltaTime),.001,.1)});
  const auto size=ImGui::GetIO().DisplaySize;
  const unsigned columns=std::clamp(static_cast<unsigned>(std::max(1.0F,size.x/145)),1U,static_cast<unsigned>(mathObjectSpecs().size()));
  const unsigned rows=(static_cast<unsigned>(mathObjectSpecs().size())+columns-1)/columns;
  const bool compactSubjects=size.y<720;
  const float sidebar=std::clamp(size.x*.29F,280.0F,380.0F),top=compactSubjects?90:50+42.0F*rows,footer=148;
  window("Subjects",{0,0},{size.x,top-8});
  ImGui::TextUnformatted("PATHS / MATH OBJECTS");ImGui::SameLine();ImGui::TextDisabled("Explore a relationship in three dimensions");
  ImGui::Spacing();
  if(compactSubjects) {
    ImGui::SetNextItemWidth(-1);if(ImGui::BeginCombo("##subject",mathObjectSpecs()[static_cast<std::size_t>(model.snapshot().kind)].name.data())) {
      for(const auto& item:mathObjectSpecs())if(ImGui::Selectable(item.name.data(),model.snapshot().kind==item.id))apply(model,{MathActionKind::Select,item.id});ImGui::EndCombo();
    }
  } else for(const auto& spec:mathObjectSpecs()) {
    if(static_cast<unsigned>(spec.id)%columns)ImGui::SameLine();
    const bool active=model.snapshot().kind==spec.id;
    if(active)ImGui::PushStyleColor(ImGuiCol_Button,{.14F,.45F,.40F,1});
    if(ImGui::Button(spec.name.data(),{(size.x-30-(columns-1)*7)/columns,35}))apply(model,{MathActionKind::Select,spec.id});
    if(active)ImGui::PopStyleColor();
  }
  ImGui::End();
  const auto& spec=mathObjectSpecs()[static_cast<std::size_t>(model.snapshot().kind)];
  window("Object controls",{size.x-sidebar-10,top},{sidebar,size.y-top-12});
  ImGui::TextWrapped("%s",spec.title.data());ImGui::Spacing();
  const auto lessons=mathLessons(model.snapshot().kind);
  if(!lessons.empty()) {
    ImGui::TextUnformatted("Learning layer");ImGui::SetNextItemWidth(-1);
    if(ImGui::BeginCombo("##level",lessons[model.snapshot().level].name.data())) {
      for(unsigned i=0;i<lessons.size();++i)if(ImGui::Selectable(lessons[i].name.data(),model.snapshot().level==i))apply(model,{MathActionKind::SetLevel,{},{},static_cast<double>(i)});
      ImGui::EndCombo();
    }
  }
  const MathLesson* lesson=lessons.empty()?nullptr:&lessons[model.snapshot().level];
  if(model.snapshot().kind==MathObjectKind::Linear&&model.snapshot().level>0) {
    ImGui::SetNextItemWidth(-1);
    if(ImGui::BeginCombo("##preset","Choose a matrix example")) {
      const auto names=mathMatrixPresetNames();for(unsigned i=0;i<names.size();++i)if(ImGui::Selectable(names[i].data()))apply(model,{MathActionKind::MatrixPreset,{},{},0,i});ImGui::EndCombo();
    }
    ImGui::TextWrapped("Edit A in the linked matrix diagram below the object.");
  }
  for(const auto& p:mathParameterSpecs())if(model.parameterAvailable(p.id)&&!(p.matrixEntry&&model.snapshot().level>0)) {
    ImGui::PushID(static_cast<int>(p.id));
    float value=static_cast<float>(model.parameter(p.id));bool changed=false;
    if(!p.choices.empty()) {
      int selection=static_cast<int>(value);ImGui::TextUnformatted(p.label.data());ImGui::SetNextItemWidth(-1);
      changed=ImGui::Combo("##choice",&selection,p.choices.data());value=static_cast<float>(selection);
    } else switch(p.id) {
      case MathParameter::Sample: {
        int selection=static_cast<int>(value);ImGui::SetNextItemWidth(-1);
        changed=ImGui::Combo("##sample",&selection,"Left sample\0Midpoint sample\0Right sample\0");value=static_cast<float>(selection);break;
      }
      case MathParameter::Shortcut: {bool enabled=value!=0;changed=ImGui::Checkbox(p.label.data(),&enabled);value=enabled?1.0F:0.0F;break;}
      default:ImGui::TextUnformatted(p.label.data());ImGui::SetNextItemWidth(-1);changed=ImGui::SliderFloat("##value",&value,static_cast<float>(p.minimum),static_cast<float>(p.maximum),p.step>=1?"%.0f":p.step<.01?"%.3f":"%.2f",ImGuiSliderFlags_AlwaysClamp);break;
    }
    if(changed)apply(model,{MathActionKind::SetParameter,{},p.id,value});ImGui::PopID();ImGui::Spacing();
  }
  explorationControls(model);
  if(ImGui::Button("Reset values"))apply(model,{MathActionKind::Reset});ImGui::SameLine();if(ImGui::Button("Reset view"))scene.resetView();
  if(model.snapshot().kind==MathObjectKind::Function&&model.snapshot().level>=2&&ImGui::Button("Swap bounds"))apply(model,{MathActionKind::SwapBounds});
  if(model.snapshot().kind==MathObjectKind::Surface&&model.parameterAvailable(MathParameter::DescentRate)&&ImGui::Button("Take a descent step")) {
    // A stationary point or domain boundary is an ordinary mathematical result.
    const auto result=model.dispatch({MathActionKind::DescentStep});if(!result.accepted)ImGui::TextWrapped("%s",result.reason.data());
  }
  ImGui::Separator();
  const auto& snapshot=model.snapshot();
  for(std::size_t i=0;i<snapshot.metricCount;++i) {const auto& metric=snapshot.metrics[i];ImGui::Text("%s: %.4g %s",metric.label.data(),metric.value,metric.suffix.data()?metric.suffix.data():"");}
  if(snapshot.kind==MathObjectKind::Discrete) {
    ImGui::Spacing();ImGui::TextUnformatted("Your route");
    std::array<char,MathObjectSnapshot::kRouteCapacity*4> route{};std::size_t length=0;
    for(std::size_t i=0;i<snapshot.routeCount;++i){if(i){route[length++]=' ';route[length++]='>';route[length++]=' ';}route[length++]=static_cast<char>('A'+snapshot.route[i]);}
    ImGui::TextWrapped("%s",route.data());
    for(unsigned i=0;i<8;++i) {
      if(i%4)ImGui::SameLine();const char label[]{static_cast<char>('A'+i),'\0'};
      ImGui::BeginDisabled(!snapshot.allowedVertices[i] || snapshot.routeCount==snapshot.route.size());
      if(ImGui::Button(label,{46,28}))apply(model,{MathActionKind::VisitVertex,{},{},0,i});ImGui::EndDisabled();
    }
    ImGui::BeginDisabled(snapshot.routeCount<=1);if(ImGui::Button("Undo step"))apply(model,{MathActionKind::UndoRoute});ImGui::EndDisabled();ImGui::SameLine();
    if(ImGui::Button("Start route again"))apply(model,{MathActionKind::ResetRoute});
  }
  ImGui::Separator();ImGui::TextUnformatted("CHALLENGE");ImGui::TextWrapped("%s",lesson?lesson->challenge.data():spec.challenge.data());
  if(ImGui::Button("Check my solution",{-1,32}))apply(model,{MathActionKind::Check});
  if(snapshot.feedback!=MathFeedback::None) {
    ImGui::PushStyleColor(ImGuiCol_Text,snapshot.feedback==MathFeedback::Solved?ImVec4{.35F,.9F,.66F,1}:ImVec4{.98F,.77F,.35F,1});
    ImGui::TextWrapped("%s",snapshot.feedbackText.data());ImGui::PopStyleColor();
  }
  ImGui::Spacing();
  if(lesson)ImGui::TextWrapped("%s",lesson->explanation.data());
  else if(ImGui::CollapsingHeader("Learning connections",ImGuiTreeNodeFlags_DefaultOpen))for(const auto text:spec.progression)ImGui::BulletText("%s",text.data());
  ImGui::End();

  const bool linked=snapshot.plotCount||snapshot.matrixCount||snapshot.contours.active||snapshot.symmetry.active||snapshot.table.rowCount;
  const float diagramHeight=linked?std::clamp(size.y*.26F,170.0F,250.0F):0;
  const SceneViewport viewport{16,top,std::max(80.0F,size.x-sidebar-42),std::max(80.0F,size.y-top-footer-diagramHeight-24)};
  static_cast<void>(scene.publish(model.snapshot(),viewport));
  window("Object viewport",{viewport.x,viewport.y},{viewport.width,viewport.height},ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
  ImGui::SetCursorPos({0,0});ImGui::InvisibleButton("3D object",{viewport.width,viewport.height},ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonRight);
  const auto& io=ImGui::GetIO();
  if(ImGui::IsItemActive()) {
    if(ImGui::IsMouseDragging(ImGuiMouseButton_Left))static_cast<void>(scene.navigate(Navigation::Orbit,io.MouseDelta.x,io.MouseDelta.y));
    if(ImGui::IsMouseDragging(ImGuiMouseButton_Right))static_cast<void>(scene.navigate(Navigation::Pan,io.MouseDelta.x,io.MouseDelta.y));
  }
  if(ImGui::IsItemHovered()&&io.MouseWheel!=0)static_cast<void>(scene.navigate(Navigation::Dolly,0,io.MouseWheel));
  ImGui::End();static_cast<void>(scene.publish(model.snapshot(),viewport));
  auto* overlay=ImGui::GetForegroundDrawList();overlay->PushClipRect({viewport.x,viewport.y},{viewport.x+viewport.width,viewport.y+viewport.height},true);
  for(std::size_t i=0;i<snapshot.labelCount;++i) {
    const auto& label=snapshot.labels[i];const auto p=scene.project(label.position);if(p.z<0)continue;
    const auto extent=ImGui::CalcTextSize(label.text.data());const ImVec2 at{p.x-extent.x*.5F,p.y-extent.y*.5F};
    overlay->AddRectFilled({at.x-4,at.y-2},{at.x+extent.x+4,at.y+extent.y+2},IM_COL32(12,21,31,210),3);
    overlay->AddText(at,ImGui::ColorConvertFloat4ToU32({label.color.x,label.color.y,label.color.z,1}),label.text.data());
  }
  overlay->PopClipRect();
  if(linked) {
    window("Linked mathematical diagrams",{viewport.x,size.y-footer-diagramHeight-8},{viewport.width,diagramHeight});linkedViews(model,ui);ImGui::End();
  }
  window("Relationship",{viewport.x,size.y-footer},{viewport.width,footer-12});
  ImGui::TextUnformatted("THE RELATIONSHIP");ImGui::TextWrapped("%s",lesson?lesson->relationship.data():spec.relationship.data());
  ImGui::Spacing();ImGui::TextWrapped("%s",spec.convention.data());ImGui::Spacing();ImGui::TextDisabled("Left drag: orbit   |   Right drag: pan   |   Scroll: zoom");ImGui::End();
}
} // namespace
int main(int argc,char** argv) {
  try {
    const auto options=parse(argc,argv);MathObjects model;for(const auto& action:options.actions)apply(model,action);
    MathObjectScene scene;UiState ui;
    if(options.validate) {
      const auto& frame=scene.publish(model.snapshot(),{0,0,static_cast<float>(options.native.width),static_cast<float>(options.native.height)});
      std::printf("math_lab text validation: object=%s level=%u vertices=%zu indices=%zu plots=%zu matrices=%zu contours=%zu feedback=%u\n",mathObjectSpecs()[static_cast<std::size_t>(model.snapshot().kind)].key.data(),model.snapshot().level,frame.vertices.size(),frame.indices.size(),model.snapshot().plotCount,model.snapshot().matrixCount,model.snapshot().contours.count,static_cast<unsigned>(model.snapshot().feedback));
      for(std::size_t i=0;i<model.snapshot().metricCount;++i){const auto& metric=model.snapshot().metrics[i];std::printf("  %s = %.12g\n",metric.label.data(),metric.value);}
      const auto& table=model.snapshot().table;
      if(table.rowCount){std::printf("  Table: %s\n",table.title.data());for(std::size_t r=0;r<table.rowCount;++r){std::printf("    %s:",table.rowLabels[r].data());for(std::size_t c=0;c<table.columnCount;++c)std::printf(" %s=%.12g",table.columns[c].data(),table.values[r][c]);std::putchar('\n');}}
      return 0;
    }
    NativeVulkanHost host(options.native);
    auto& style=ImGui::GetStyle();style.WindowPadding={15,13};style.FramePadding={8,5};style.ItemSpacing={7,8};style.FrameRounding=4;
    style.Colors[ImGuiCol_WindowBg]={.035F,.055F,.08F,1};style.Colors[ImGuiCol_Button]={.12F,.26F,.30F,1};
    unsigned frames=0,skipped=0;
    while(!ui.quit&&(!options.frames||frames<options.frames)) {
      const auto result=host.frame([&](const SDL_Event& event){if(event.type==SDL_EVENT_QUIT)ui.quit=true;},[&]{draw(model,scene,ui);},&scene.frame());
      if(result.status==FrameStatus::Failed)throw std::runtime_error(result.error);if(result.status==FrameStatus::Closed)break;
      if(result.status==FrameStatus::Skipped){if(++skipped>1000)throw std::runtime_error("Too many skipped native frames");continue;}
      skipped=0;++frames;
    }
    if(!options.capture.empty()) {
      if(!options.capture.parent_path().empty())std::filesystem::create_directories(options.capture.parent_path());
      std::string error;if(!host.capture(capturePaths(options.capture),error))throw std::runtime_error(error);
    }
    std::printf("math_lab object=%s vertices=%zu indices=%zu feedback=%u\n",mathObjectSpecs()[static_cast<std::size_t>(model.snapshot().kind)].key.data(),scene.frame().vertices.size(),scene.frame().indices.size(),static_cast<unsigned>(model.snapshot().feedback));
    return 0;
  }catch(const std::exception& error){std::fprintf(stderr,"math_lab: %s\n",error.what());return 1;}
}

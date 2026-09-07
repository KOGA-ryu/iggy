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
enum class Flag { Offscreen,Frames,Capture,Resolution,Object,Set,Route,Check,Help };
Options parse(int argc,char** argv) {
  constexpr std::array<std::pair<std::string_view,Flag>,9> flags{{
    {"--offscreen",Flag::Offscreen},{"--frames",Flag::Frames},{"--capture",Flag::Capture},
    {"--resolution",Flag::Resolution},{"--object",Flag::Object},{"--set",Flag::Set},
    {"--route",Flag::Route},{"--check",Flag::Check},{"--help",Flag::Help}}};
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
      case Flag::Help:
        std::puts("math_lab [--object algebra|trig|calculus|linear|discrete] [--set key=value] [--route BDH] [--check]\n         [--offscreen] [--frames N] [--resolution 1440x900] [--capture /path/view.png]\nParameters: x, gap, angle, slices, slice_gap, sample (0=left,1=midpoint,2=right), shear, scale, depth, shortcut.");
        std::exit(0);
    }
  }
  if(options.native.offscreen&&!options.frames)options.frames=3;
  if(!options.capture.empty()&&!options.frames)options.frames=3;
  return options;
}
struct UiState { int level=0;bool quit=false; };
constexpr auto windowFlags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBringToFrontOnFocus;
void window(const char* name,ImVec2 position,ImVec2 size,ImGuiWindowFlags extra=0) {
  ImGui::SetNextWindowPos(position);ImGui::SetNextWindowSize(size);ImGui::Begin(name,nullptr,windowFlags|extra);
}
void apply(MathObjects& objects,MathAction action) {
  const auto result=objects.dispatch(action);if(!result.accepted)throw std::runtime_error(std::string(result.reason));
}
void draw(MathObjects& model,MathObjectScene& scene,UiState& ui) {
  const auto size=ImGui::GetIO().DisplaySize;
  const float sidebar=std::clamp(size.x*.29F,280.0F,380.0F),top=120,footer=148;
  const SceneViewport viewport{16,top,size.x-sidebar-42,std::max(100.0F,size.y-top-footer-16)};
  window("Subjects",{0,0},{size.x,top-8});
  ImGui::TextUnformatted("PATHS / MATH OBJECTS");ImGui::SameLine();ImGui::TextDisabled("Explore a relationship in three dimensions");
  ImGui::Spacing();
  for(const auto& spec:mathObjectSpecs()) {
    if(spec.id!=MathObjectKind::Algebra)ImGui::SameLine();
    const bool active=model.snapshot().kind==spec.id;
    if(active)ImGui::PushStyleColor(ImGuiCol_Button,{.14F,.45F,.40F,1});
    if(ImGui::Button(spec.name.data(),{(size.x-80)/5,35}))apply(model,{MathActionKind::Select,spec.id});
    if(active)ImGui::PopStyleColor();
  }
  ImGui::End();
  const auto& spec=mathObjectSpecs()[static_cast<std::size_t>(model.snapshot().kind)];
  window("Object controls",{size.x-sidebar-10,top},{sidebar,size.y-top-12});
  ImGui::TextWrapped("%s",spec.title.data());ImGui::Spacing();
  for(const auto& p:mathParameterSpecs())if(p.owner==model.snapshot().kind) {
    ImGui::PushID(static_cast<int>(p.id));
    float value=static_cast<float>(model.parameter(p.id));bool changed=false;
    switch(p.id) {
      case MathParameter::Sample: {
        int selection=static_cast<int>(value);ImGui::SetNextItemWidth(-1);
        changed=ImGui::Combo("##sample",&selection,"Left sample\0Midpoint sample\0Right sample\0");value=static_cast<float>(selection);break;
      }
      case MathParameter::Shortcut: {bool enabled=value!=0;changed=ImGui::Checkbox(p.label.data(),&enabled);value=enabled?1.0F:0.0F;break;}
      default:ImGui::TextUnformatted(p.label.data());ImGui::SetNextItemWidth(-1);changed=ImGui::SliderFloat("##value",&value,static_cast<float>(p.minimum),static_cast<float>(p.maximum),p.step>=1?"%.0f":"%.2f",ImGuiSliderFlags_AlwaysClamp);break;
    }
    if(changed)apply(model,{MathActionKind::SetParameter,{},p.id,value});ImGui::PopID();ImGui::Spacing();
  }
  if(ImGui::Button("Reset values"))apply(model,{MathActionKind::Reset});ImGui::SameLine();if(ImGui::Button("Reset view"))scene.resetView();
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
  ImGui::Separator();ImGui::TextUnformatted("CHALLENGE");ImGui::TextWrapped("%s",spec.challenge.data());
  if(ImGui::Button("Check my solution",{-1,32}))apply(model,{MathActionKind::Check});
  if(snapshot.feedback!=MathFeedback::None) {
    ImGui::PushStyleColor(ImGuiCol_Text,snapshot.feedback==MathFeedback::Solved?ImVec4{.35F,.9F,.66F,1}:ImVec4{.98F,.77F,.35F,1});
    ImGui::TextWrapped("%s",snapshot.feedbackText.data());ImGui::PopStyleColor();
  }
  ImGui::Spacing();ImGui::SetNextItemWidth(-1);ImGui::Combo("##level",&ui.level,"School\0Bridge\0Introductory university\0");
  ImGui::TextWrapped("%s",spec.progression[static_cast<std::size_t>(ui.level)].data());ImGui::End();

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
  window("Relationship",{viewport.x,size.y-footer},{viewport.width,footer-12});
  ImGui::TextUnformatted("THE RELATIONSHIP");ImGui::TextWrapped("%s",spec.relationship.data());
  ImGui::Spacing();ImGui::TextWrapped("%s",spec.convention.data());ImGui::Spacing();ImGui::TextDisabled("Left drag: orbit   |   Right drag: pan   |   Scroll: zoom");ImGui::End();
}
} // namespace
int main(int argc,char** argv) {
  try {
    const auto options=parse(argc,argv);MathObjects model;for(const auto& action:options.actions)apply(model,action);
    MathObjectScene scene;UiState ui;NativeVulkanHost host(options.native);
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

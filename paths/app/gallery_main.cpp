#include "platform/NativeVulkanHost.hpp"
#include "scene/GalleryScene.hpp"
#include "runtime/gallery/GallerySession.hpp"
#include "ui/GalleryMenu.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include "imgui.h"

namespace {
using namespace paths;
using Kind = GalleryActionKind;
enum class Arguments { None, Amount, Vector, Pair, Index, IndexedVector, IndexedAmount, Fly };
struct Command { std::string_view name; Kind kind; Arguments args; };
constexpr std::array commands{
  Command{"frame",Kind::Tick,Arguments::None},
  Command{"tick",Kind::Tick,Arguments::Amount},
  Command{"pause",Kind::Pause,Arguments::Amount},
  Command{"reset_camera",Kind::ResetCamera,Arguments::None},
  Command{"frame_all",Kind::FrameAll,Arguments::None},
  Command{"fly",Kind::Fly,Arguments::Fly},
  Command{"look",Kind::Look,Arguments::Pair},
  Command{"orbit",Kind::Orbit,Arguments::Pair},
  Command{"pan",Kind::Pan,Arguments::Pair},
  Command{"dolly",Kind::Dolly,Arguments::Amount},
  Command{"pick",Kind::Pick,Arguments::Pair},
  Command{"shoot",Kind::Pick,Arguments::Pair},
  Command{"select",Kind::Select,Arguments::Index},
  Command{"add_box",Kind::AddBox,Arguments::None},
  Command{"add_ramp",Kind::AddRamp,Arguments::None},
  Command{"add_frame",Kind::AddFrame,Arguments::None},
  Command{"add_sphere",Kind::AddSphere,Arguments::None},
  Command{"route",Kind::Route,Arguments::Index},
  Command{"extent",Kind::Extent,Arguments::Vector},
  Command{"route_dwell",Kind::RouteDwell,Arguments::Amount},
  Command{"route_seed",Kind::RouteSeed,Arguments::Index},
  Command{"delete",Kind::Delete,Arguments::None},
  Command{"position",Kind::Position,Arguments::Vector},
  Command{"size",Kind::Size,Arguments::Vector},
  Command{"yaw",Kind::Yaw,Arguments::Amount},
  Command{"color",Kind::Color,Arguments::Vector},
  Command{"patrol",Kind::Patrol,Arguments::Amount},
  Command{"speed",Kind::Speed,Arguments::Amount},
  Command{"loop",Kind::Loop,Arguments::Amount},
  Command{"waypoint",Kind::Waypoint,Arguments::IndexedVector},
  Command{"dwell",Kind::Dwell,Arguments::IndexedAmount},
  Command{"segment_speed",Kind::SegmentSpeed,Arguments::IndexedAmount},
  Command{"scrub",Kind::Scrub,Arguments::Amount},
};
using MenuKind=GalleryMenuActionKind;
struct ScriptStep { GalleryAction action; std::size_t line; std::optional<GalleryMenuAction> navigation; };
struct Options {
  NativeLaunchConfig native{false,1440,900,true};
  std::uint64_t frames=0;
  std::filesystem::path capture,report,scriptPath,contentPack;
  std::vector<ScriptStep> script;
  GalleryScreen startup=GalleryScreen::ChooseQuestions;
  GalleryConfig game;
};
std::uint32_t integer(std::string_view s) {
  std::uint32_t n=0;
  const auto result=std::from_chars(s.data(),s.data()+s.size(),n);
  if(result.ec!=std::errc{} || result.ptr!=s.data()+s.size() || !n)
    throw std::invalid_argument("Expected a positive integer");
  return n;
}
void readScript(Options& options) {
  if(options.scriptPath.empty())return;
  std::ifstream input(options.scriptPath);
  if(!input)throw std::runtime_error("Cannot open gallery script");
  std::string line;
  std::size_t number=0;
  while(std::getline(input,line)) {
    ++number;
    if(const auto hash=line.find('#');hash!=std::string::npos)line.resize(hash);
    std::istringstream tokens(line);
    std::string name,extra;
    if(!(tokens>>name))continue;
    static constexpr std::array menuCommands{
      std::pair{"choose_pack",MenuKind::SelectPack},std::pair{"choose_mode",MenuKind::SelectMode},
      std::pair{"choose_motion",MenuKind::SelectMotion},std::pair{"set_pace",MenuKind::SetPace},
      std::pair{"new_game",MenuKind::NewGame},std::pair{"cancel_new_game",MenuKind::CancelNewGame},
      std::pair{"start_new_game",MenuKind::StartNewGame},
      std::pair{"review",MenuKind::OpenReview},std::pair{"back_to_game",MenuKind::CloseReview},
      std::pair{"review_run",MenuKind::SelectReviewRun},std::pair{"review_step",MenuKind::ToggleReviewStep},
      std::pair{"play",MenuKind::Play},std::pair{"questions",MenuKind::ChooseQuestions},std::pair{"workshop",MenuKind::Workshop}};
    const auto menu=std::find_if(menuCommands.begin(),menuCommands.end(),[&](const auto& c){return c.first==name;});
    if(menu!=menuCommands.end()) {
      GalleryMenuAction action{menu->second};
      switch(action.kind) {
        case MenuKind::SelectReviewRun:case MenuKind::ToggleReviewStep:
        case MenuKind::SelectPack:case MenuKind::SelectMode:tokens>>action.index;break;
        case MenuKind::SetPace:tokens>>action.amount;break;
        case MenuKind::SelectMotion: {
          std::string id;tokens>>id;
          const auto routes=targetRouteDescriptors();
          const auto route=std::find_if(routes.begin(),routes.end(),[&](const auto& r){return r.id==id;});
          if(route==routes.end())throw std::runtime_error("Unknown movement on line "+std::to_string(number));
          action.index=static_cast<std::size_t>(route-routes.begin());break;
        }
        case MenuKind::Play:case MenuKind::ChooseQuestions:case MenuKind::Workshop:
        case MenuKind::OpenReview:case MenuKind::CloseReview:
        case MenuKind::NewGame:case MenuKind::CancelNewGame:case MenuKind::StartNewGame:break;
      }
      if(tokens.fail() || !std::isfinite(action.amount) || (tokens>>extra))
        throw std::runtime_error("Invalid menu arguments on line "+std::to_string(number));
      options.script.push_back({{},number,action});
      if(options.script.size()>10000)throw std::runtime_error("Gallery script exceeds 10000 actions");
      continue;
    }
    const auto found=std::find_if(commands.begin(),commands.end(),[&](auto c){return c.name==name;});
    if(found==commands.end())throw std::runtime_error("Unknown gallery command on line "+std::to_string(number));
    GalleryAction a{found->kind};
    switch(found->args) {
      case Arguments::None:break;
      case Arguments::Amount:tokens>>a.amount;break;
      case Arguments::Vector:tokens>>a.value.x>>a.value.y>>a.value.z;break;
      case Arguments::Pair:tokens>>a.value.x>>a.value.y;break;
      case Arguments::Index:tokens>>a.index;break;
      case Arguments::IndexedVector:tokens>>a.index>>a.value.x>>a.value.y>>a.value.z;break;
      case Arguments::IndexedAmount:tokens>>a.index>>a.amount;break;
      case Arguments::Fly:tokens>>a.value.x>>a.value.y>>a.value.z>>a.amount>>a.index;break;
    }
    if(tokens.fail() || !iggy3d::isFinite(a.value) || !std::isfinite(a.amount) || (tokens>>extra))
      throw std::runtime_error("Invalid gallery arguments on line "+std::to_string(number));
    if(a.kind==Kind::Dolly)a.value.y=a.amount;
    options.script.push_back({a,number});
    if(options.script.size()>10000)throw std::runtime_error("Gallery script exceeds 10000 actions");
  }
  if(!input.eof())throw std::runtime_error("Gallery script read failed");
}
Options parse(int argc,char** argv) {
  Options o;
  for(int i=1;i<argc;++i) {
    const std::string_view key=argv[i];
    if(key=="--offscreen") {o.native.offscreen=true;continue;}
    if(key=="--help") {
      std::cout<<"gallery [--start-mode questions|workshop";
      for(const auto& mode:galleryVariations())std::cout<<'|'<<mode.id;
      std::cout<<"] [--content-pack pack.json] [--seed N] [--motion ROUTE] [--pace N] [--offscreen] [--frames N] [--resolution WxH] [--capture image.png] [--report run.json] [--script commands.txt]\n";
      std::exit(0);
    }
    if(i+1>=argc)throw std::invalid_argument("Missing launch option value");
    const std::string_view value=argv[++i];
    enum class Option { Frames,Resolution,Capture,Report,Script,StartMode,Seed,Motion,Pace,ContentPack };
    static constexpr std::array table{
      std::pair{"--frames",Option::Frames},std::pair{"--resolution",Option::Resolution},
      std::pair{"--start-mode",Option::StartMode},std::pair{"--seed",Option::Seed},
      std::pair{"--content-pack",Option::ContentPack},
      std::pair{"--motion",Option::Motion},std::pair{"--pace",Option::Pace},std::pair{"--capture",Option::Capture},std::pair{"--report",Option::Report},std::pair{"--script",Option::Script}};
    const auto found=std::find_if(table.begin(),table.end(),[&](auto p){return key==p.first;});
    if(found==table.end())throw std::invalid_argument("Unknown launch option: "+std::string(key));
    switch(found->second) {
      case Option::StartMode: {
        if(value=="questions") {o.startup=GalleryScreen::ChooseQuestions;break;}
        if(value=="workshop") {o.startup=GalleryScreen::Workshop;break;}
        const auto modes=galleryVariations();
        const auto mode=std::find_if(modes.begin(),modes.end(),[&](const auto& d){return d.id==value;});
        if(mode==modes.end())throw std::invalid_argument("Unknown gallery startup mode");
        o.startup=GalleryScreen::Playing;o.game.variation=mode->variation;break;
      }
      case Option::Seed: {
        const auto parsed=std::from_chars(value.data(),value.data()+value.size(),o.game.seed);
        if(parsed.ec!=std::errc{} || parsed.ptr!=value.data()+value.size())throw std::invalid_argument("Invalid seed");
        break;
      }
      case Option::Motion: {
        const auto routes=targetRouteDescriptors();
        const auto route=std::find_if(routes.begin(),routes.end(),[&](const auto& d){return d.id==value;});
        if(route==routes.end())throw std::invalid_argument("Unknown target route");
        o.game.motion=route->kind;break;
      }
      case Option::Pace: {
        const auto parsed=std::from_chars(value.data(),value.data()+value.size(),o.game.pace);
        if(parsed.ec!=std::errc{} || parsed.ptr!=value.data()+value.size() || !std::isfinite(o.game.pace))
          throw std::invalid_argument("Invalid movement pace");
        break;
      }
      case Option::Frames:o.frames=integer(value);break;
      case Option::Resolution: {
        const auto x=value.find('x');
        if(x==std::string_view::npos)throw std::invalid_argument("Resolution must be WxH");
        o.native.width=integer(value.substr(0,x));o.native.height=integer(value.substr(x+1));
        if(o.native.width<800 || o.native.height<600 || o.native.width>3840 || o.native.height>2160)
          throw std::invalid_argument("Resolution range is 800x600 through 3840x2160");
        break;
      }
      case Option::Capture:o.capture=value;break;
      case Option::Report:o.report=value;break;
      case Option::Script:o.scriptPath=value;break;
      case Option::ContentPack:
        if(value.empty())throw std::invalid_argument("--content-pack requires a file path");
        o.contentPack=value;break;
    }
  }
  if(o.startup!=GalleryScreen::Playing && !o.contentPack.empty())throw std::invalid_argument("--content-pack requires a question startup mode; workshop has no questions");
  readScript(o);
  if(!o.frames && (!o.scriptPath.empty() || !o.capture.empty()))o.frames=o.script.size()+3;
  if(o.frames && o.frames<o.script.size())throw std::invalid_argument("Frame limit truncates script");
  if(o.native.offscreen && !o.frames)throw std::invalid_argument("Offscreen requires --frames, --capture, or --script");
  std::vector<std::filesystem::path> outputs;
  if(!o.capture.empty()) {
    const auto c=capturePaths(o.capture);
    outputs={c.screenshotPath,c.rawPath,c.metaPath,c.hashPath};
  }
  if(!o.report.empty())outputs.push_back(o.report);
  std::vector<std::filesystem::path> normalized;
  for(const auto& output:outputs) {
    const auto path=std::filesystem::weakly_canonical(std::filesystem::absolute(output));
    if(std::find(normalized.begin(),normalized.end(),path)!=normalized.end() ||
       (!o.scriptPath.empty() && path==std::filesystem::weakly_canonical(std::filesystem::absolute(o.scriptPath))))
      throw std::invalid_argument("Output paths alias another output or the input script");
    if(std::filesystem::is_directory(path))throw std::invalid_argument("Output path is a directory");
    normalized.push_back(path);
  }
  return o;
}

struct Ui {
  GalleryResult last{true,"ready"};
  bool quit=false,looking=false,panning=false,orbiting=false;
  float scrub=0;
  std::optional<GalleryMenuAction> navigation;
  void navigate(GalleryMenuAction action) {if(!navigation)navigation=action;}
  void act(GalleryScene& scene,GalleryAction action) { last=scene.dispatch(action); }
  void act(GallerySession& game,const GalleryCommand& action) {last=game.dispatch(action);}
  void script(GallerySession& game,GalleryAction action) {
    switch(action.kind) {
      case Kind::Tick:act(game,GalleryTick{action.amount});break;
      case Kind::Pause:act(game,GalleryPause{action.amount!=0});break;
      case Kind::Pick:act(game,Shoot{game.scene().frame().id,game.view().challenge,action.value.x,action.value.y});break;
      default:last={false,"workshop_action_unavailable_in_game"};break;
    }
  }
};
SceneViewport viewport(float w,float h) {return {0,78,std::floor(w-std::clamp(w*0.27F,320.0F,410.0F)),h-78};}
void drawUi(GalleryScene& scene,Ui& ui,bool scripted,float dt) {
  auto& io=ImGui::GetIO();
  const auto v=scene.frame().viewport;
  const bool inside=io.MousePos.x>=v.x && io.MousePos.x<v.x+v.width && io.MousePos.y>=v.y && io.MousePos.y<v.y+v.height;
  if(!scripted) {
    if(ImGui::IsKeyPressed(ImGuiKey_Escape,false))ui.act(scene,{Kind::Pause,{},scene.paused()?0.0F:1.0F});
    const bool canInput=!scene.paused() && !io.WantTextInput && !ImGui::IsAnyItemActive();
    if(canInput && inside && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
      ui.act(scene,{Kind::Pick,{(io.MousePos.x-v.x)/v.width,(io.MousePos.y-v.y)/v.height,0}});
    if(canInput && inside) {
      if(ImGui::IsMouseClicked(ImGuiMouseButton_Right))ui.looking=true;
      if(ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {ui.orbiting=io.KeyAlt;ui.panning=!io.KeyAlt;}
      if(io.MouseWheel)ui.act(scene,{Kind::Dolly,{0,io.MouseWheel,0}});
      const auto down=[](ImGuiKey k){return ImGui::IsKeyDown(k)?1.0F:0.0F;};
      const iggy3d::Vec3 move{down(ImGuiKey_D)-down(ImGuiKey_A),down(ImGuiKey_W)-down(ImGuiKey_S),down(ImGuiKey_E)-down(ImGuiKey_Q)};
      if(iggy3d::lengthSquared(move)>0)ui.act(scene,{Kind::Fly,move,dt,io.KeyShift?1U:0U});
      if(ImGui::IsKeyPressed(ImGuiKey_F,false))ui.act(scene,{Kind::FrameAll});
    }
    ui.looking=ui.looking && canInput && ImGui::IsMouseDown(ImGuiMouseButton_Right);
    ui.panning=ui.panning && canInput && ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    ui.orbiting=ui.orbiting && canInput && ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    if(io.MouseDelta.x || io.MouseDelta.y) {
      if(ui.looking)ui.act(scene,{Kind::Look,{io.MouseDelta.x*0.15F,-io.MouseDelta.y*0.15F,0}});
      if(ui.panning)ui.act(scene,{Kind::Pan,{io.MouseDelta.x,io.MouseDelta.y,0}});
      if(ui.orbiting)ui.act(scene,{Kind::Orbit,{io.MouseDelta.x,io.MouseDelta.y,0}});
    }
  }
  const auto flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
    ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings;
  ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({v.width,78});
  ImGui::Begin("Gallery header",nullptr,flags);
  ImGui::TextColored({0.32F,0.88F,0.82F,1},"PATHS / GALLERY WORKSHOP");
  ImGui::TextDisabled("Engine port preview  |  Click an object to select it");
  ImGui::End();
  ImGui::SetNextWindowPos({v.width,0});ImGui::SetNextWindowSize({io.DisplaySize.x-v.width,io.DisplaySize.y});
  ImGui::Begin("Gallery controls",nullptr,flags);
  ImGui::Text("SCENE CONTROLS");ImGui::Separator();
  if(ImGui::Button("Choose questions"))ui.navigate({MenuKind::ChooseQuestions});
  ImGui::TextWrapped("Right-drag: look. WASD: move. Q/E: height. Shift: faster. Middle-drag: pan. Alt + middle-drag: orbit. Wheel: zoom.");
  if(ImGui::Button(scene.paused()?"Resume patrols":"Pause / Esc"))ui.act(scene,{Kind::Pause,{},scene.paused()?0.0F:1.0F});
  ImGui::SameLine();if(ImGui::Button("Reset view"))ui.act(scene,{Kind::ResetCamera});
  if(ImGui::Button("Frame room / F"))ui.act(scene,{Kind::FrameAll});
  ImGui::Spacing();ImGui::SeparatorText("Create");
  constexpr std::array creates{std::pair{"Box",Kind::AddBox},std::pair{"Ramp",Kind::AddRamp},std::pair{"Frame",Kind::AddFrame},std::pair{"Sphere",Kind::AddSphere}};
  for(std::size_t i=0;i<creates.size();++i) {
    if(i)ImGui::SameLine();
    if(ImGui::Button(creates[i].first))ui.act(scene,{creates[i].second});
  }
  ImGui::TextDisabled("%zu / %zu objects",scene.objects().size(),kGalleryObjectCapacity);
  ImGui::SeparatorText("Selection");
  if(const auto* o=scene.selected()) {
    ImGui::Text("Object %u",o->id.value);
    float pos[3]{o->origin.x,o->origin.y,o->origin.z};
    if(ImGui::DragFloat3("Position",pos,0.05F,-100,100,"%.2f"))ui.act(scene,{Kind::Position,{pos[0],pos[1],pos[2]}});
    float size[3]{o->size.x,o->size.y,o->size.z};
    if(o->primitive==GalleryPrimitive::Sphere) {
      if(ImGui::SliderFloat("Diameter",&size[0],0.1F,5,"%.2f m"))ui.act(scene,{Kind::Size,{size[0],size[0],size[0]}});
    } else if(ImGui::DragFloat3("Size",size,0.05F,0.1F,20,"%.2f"))ui.act(scene,{Kind::Size,{size[0],size[1],size[2]}});
    float yaw=o->yawDegrees;
    if(ImGui::SliderFloat("Yaw",&yaw,-180,180,"%.0f deg"))ui.act(scene,{Kind::Yaw,{},yaw});
    float color[3]{o->color.x,o->color.y,o->color.z};
    if(ImGui::ColorEdit3("Colour",color))ui.act(scene,{Kind::Color,{color[0],color[1],color[2]}});
    ImGui::SeparatorText("Target motion");
    const auto routes=targetRouteDescriptors();
    const auto current=std::find_if(routes.begin(),routes.end(),[&](const auto& r){return r.kind==o->route.spec.kind;});
    if(ImGui::BeginCombo("Pattern",current->label.data())) {
      for(const auto& route:routes)if(ImGui::Selectable(route.label.data(),route.kind==o->route.spec.kind))
        ui.act(scene,{Kind::Route,{},0,static_cast<std::uint32_t>(route.kind)});
      ImGui::EndCombo();
    }
    float extent[3]{o->route.spec.extent.x,o->route.spec.extent.y,o->route.spec.extent.z};
    if(ImGui::DragFloat3("Extent",extent,0.02F,0.05F,5,"%.2f m"))
      ui.act(scene,{Kind::Extent,{extent[0],extent[1],extent[2]}});
    bool patrol=o->route.spec.settings.startsActive;
    if(ImGui::Checkbox("Moving",&patrol))ui.act(scene,{Kind::Patrol,{},patrol?1.0F:0.0F});
    bool loop=o->route.spec.settings.traversalMode==iggy3d::creative::CreativeMovingPlatformTraversalMode::Loop;
    ImGui::SameLine();ImGui::BeginDisabled(!o->route.usesWaypoints);
    if(ImGui::Checkbox("Loop",&loop))ui.act(scene,{Kind::Loop,{},loop?1.0F:0.0F});
    ImGui::EndDisabled();
    float speed=static_cast<float>(o->route.spec.settings.speedMetersPerSecond);
    if(ImGui::SliderFloat("Pace",&speed,0.1F,10,"%.2f"))ui.act(scene,{Kind::Speed,{},speed});
    if(o->route.spec.kind==RouteKind::StopAndGo || o->route.spec.kind==RouteKind::SeededRoam) {
      float dwell=static_cast<float>(o->route.spec.dwellSeconds);
      if(ImGui::SliderFloat("Route wait",&dwell,0,5,"%.2f s"))ui.act(scene,{Kind::RouteDwell,{},dwell});
    }
    if(o->route.spec.kind==RouteKind::SeededRoam) {
      auto seed=o->route.spec.seed;
      if(ImGui::InputScalar("Roam seed",ImGuiDataType_U32,&seed))ui.act(scene,{Kind::RouteSeed,{},0,seed});
    }
    ImGui::TextDisabled("Loop off = ping-pong. Points are local offsets.");
    if(o->route.spec.kind==RouteKind::Waypoints)for(std::size_t i=0;i<o->route.spec.pointCount;++i) {
      ImGui::PushID(static_cast<int>(i));
      if(ImGui::TreeNode("Waypoint","Waypoint %zu",i+1)) {
        const auto point=o->route.spec.points[i];
        float p[3]{point.position.x,point.position.y,point.position.z};
        if(ImGui::DragFloat3("Offset",p,0.05F,-20,20,"%.2f"))ui.act(scene,{Kind::Waypoint,{p[0],p[1],p[2]},0,static_cast<std::uint32_t>(i)});
        float dwell=static_cast<float>(point.dwellSeconds), multiplier=static_cast<float>(point.outgoingSpeedMultiplier);
        if(ImGui::SliderFloat("Wait",&dwell,0,5,"%.2f s"))ui.act(scene,{Kind::Dwell,{},dwell,static_cast<std::uint32_t>(i)});
        if(ImGui::SliderFloat("Segment speed",&multiplier,0.25F,4,"%.2fx"))ui.act(scene,{Kind::SegmentSpeed,{},multiplier,static_cast<std::uint32_t>(i)});
        ImGui::TreePop();
      }
      ImGui::PopID();
    }
    ImGui::BeginDisabled(!scene.paused());
    if(ImGui::SliderFloat("Preview +seconds",&ui.scrub,0,60,"%.2f s"))ui.act(scene,{Kind::Scrub,{},ui.scrub/60});
    ImGui::EndDisabled();
    ImGui::TextDisabled("Pause to preview up to 60 s from the pause point. Edits restart this route.");
    if(ImGui::Button("Delete selected"))ui.act(scene,{Kind::Delete});
  } else ImGui::TextWrapped("Select a coloured block, or create a primitive. The room is fixed scenery.");
  ImGui::Separator();
  ImGui::TextColored(ui.last.accepted?ImVec4{0.4F,0.8F,0.7F,1}:ImVec4{1,0.6F,0.4F,1},"%.*s",static_cast<int>(ui.last.reason.size()),ui.last.reason.data());
  ImGui::TextDisabled("Simulation ticks: %llu",static_cast<unsigned long long>(scene.tickCount()));
  ImGui::TextWrapped("Arrange objects and preview their movement. Changes last for this session.");
  if(ImGui::Button("Quit"))ui.quit=true;
  ImGui::End();
}
void gameInput(GallerySession& game,Ui& ui,bool scripted) {
  if(scripted)return;
  const auto& io=ImGui::GetIO();
  if(ImGui::IsKeyPressed(ImGuiKey_Escape,false))ui.act(game,GalleryPause{!game.scene().paused()});
  const auto v=game.scene().frame().viewport;
  const bool inside=io.MousePos.x>=v.x && io.MousePos.x<v.x+v.width && io.MousePos.y>=v.y && io.MousePos.y<v.y+v.height;
  if(inside && !io.WantTextInput && !ImGui::IsAnyItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    ui.act(game,Shoot{game.scene().frame().id,game.view().challenge,(io.MousePos.x-v.x)/v.width,(io.MousePos.y-v.y)/v.height});
}
const char* feedbackLabel(GalleryFeedback feedback) {
  switch(feedback) {
    case GalleryFeedback::None:return "";
    case GalleryFeedback::Correct:return "Correct";
    case GalleryFeedback::Incorrect:return "Wrong";
    case GalleryFeedback::Miss:return "Miss";
  }
  return "";
}
void drawGameUi(GallerySession& game,Ui& ui,bool) {
  auto data=game.view();
  const auto& io=ImGui::GetIO();
  const auto v=game.scene().frame().viewport;
  const auto flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
    ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings;
  ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({v.width,78});
  ImGui::Begin("Gallery header",nullptr,flags);
  ImGui::TextColored({0.35F,0.9F,0.8F,1},"PATHS / %.*s",static_cast<int>(data.title.size()),data.title.data());
  ImGui::SameLine();ImGui::TextDisabled(data.paused?"Stopped":"Endless");
  if(data.paused)ImGui::Text("Correct %zu   Wrong %zu   Misses %zu   Questions %zu",data.correctHits,data.wrongHits,data.aimMisses,data.completedQuestions);
  else if(data.feedback!=GalleryFeedback::None)
    ImGui::TextColored(data.feedback==GalleryFeedback::Correct?ImVec4{0.35F,0.9F,0.8F,1}:ImVec4{1,0.5F,0.45F,1},
      "%s",feedbackLabel(data.feedback));
  ImGui::End();
  ImGui::SetNextWindowPos({v.width,0});ImGui::SetNextWindowSize({io.DisplaySize.x-v.width,io.DisplaySize.y});
  ImGui::Begin("Answer board",nullptr,flags);
  ImGui::SetWindowFontScale(1.15F);
  if(ImGui::Button(data.paused?"Resume":"Stop / Esc")) {
    ui.act(game,GalleryPause{!data.paused});data=game.view();
  }
  ImGui::SameLine();ImGui::TextDisabled("Step %zu / %zu",data.step,data.stepCount);
  if(data.paused && ImGui::Button("Answer review"))ui.navigate({MenuKind::OpenReview});
  if(data.paused && ImGui::Button("Choose questions"))ui.navigate({MenuKind::ChooseQuestions});
  ImGui::Spacing();
  ImGui::TextColored({0.55F,0.86F,1,1},"%.*s",static_cast<int>(data.equation.size()),data.equation.data());
  ImGui::TextWrapped("%.*s",static_cast<int>(data.prompt.size()),data.prompt.data());
  if(!data.working.empty() && data.working!=data.equation) {
    ImGui::SeparatorText("Working so far");
    ImGui::TextWrapped("%.*s",static_cast<int>(data.working.size()),data.working.data());
  }
  ImGui::Spacing();
  ImGui::TextDisabled("Collected %zu / %zu",data.collected,data.required);
  ImGui::Separator();
  const auto colours=galleryDisplayColours();
  for(std::size_t i=0;i<data.choiceCount;++i) {
    const auto row=data.answers[i];const auto& colour=colours[row.binding.token.value];
    ImGui::PushID(static_cast<int>(i));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,ImVec4{0.07F,0.10F,0.14F,1});
    const float textHeight=ImGui::CalcTextSize(row.text.data(),row.text.data()+row.text.size(),false,
      ImGui::GetContentRegionAvail().x-24).y;
    const float height=std::max(76.0F,textHeight+47);
    ImGui::BeginChild("Answer",{0,height},ImGuiChildFlags_Borders,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::TextColored({colour.rgb.x,colour.rgb.y,colour.rgb.z,1},"%s / %s%s",colour.marker.data(),colour.name.data(),row.collected?"  - collected":"");
    ImGui::TextWrapped("%.*s",static_cast<int>(row.text.size()),row.text.data());
    ImGui::EndChild();ImGui::PopStyleColor();ImGui::PopID();
  }
  ImGui::Separator();
  if(data.paused) {
    if(data.priorExposure)ImGui::TextDisabled("Repeated question");
    if(ImGui::Button("Quit"))ui.quit=true;
  }
  ImGui::End();
}
const char* reviewOutcome(iggy3d::first_move::QuestionReviewOutcome outcome) {
  using Outcome=iggy3d::first_move::QuestionReviewOutcome;
  switch(outcome) {
    case Outcome::InProgress:return "In progress";
    case Outcome::CorrectFirstTry:return "Correct first try";
    case Outcome::CorrectAfterRetry:return "Correct after retry";
    case Outcome::AnswerShown:return "Answer shown";
  }
  return "Unknown";
}
void drawReview(const GalleryMenu& menu,Ui& ui,bool scripted) {
  const auto& io=ImGui::GetIO();
  const auto& question=menu.activeGame()->question();
  const auto review=*question.review(menu.reviewRun());
  const auto text=[](std::string_view value){ImGui::TextWrapped("%.*s",static_cast<int>(value.size()),value.data());};
  ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::Begin("Answer review",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
    ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
  const float width=std::min(900.0F,io.DisplaySize.x-48);
  ImGui::SetCursorPos({(io.DisplaySize.x-width)*0.5F,20});
  ImGui::BeginChild("Review panel",{width,io.DisplaySize.y-40});
  ImGui::SetWindowFontScale(1.2F);
  ImGui::TextColored({0.35F,0.9F,0.8F,1},"PATHS / ANSWER REVIEW");
  if(ImGui::Button("Back to game") || (!scripted && ImGui::IsKeyPressed(ImGuiKey_Escape)))ui.navigate({MenuKind::CloseReview});
  ImGui::SameLine();ImGui::TextDisabled("Paused - Resume when ready");
  const auto runLabel=[&](std::size_t index) {
    if(index==0)return std::string("Current question");
    return "Question "+std::to_string(question.archivedRuns()[index-1].runNumber)+" (completed)";
  };
  ImGui::SetNextItemWidth(-1);
  if(ImGui::BeginCombo("##Question run",runLabel(menu.reviewRun()).c_str())) {
    for(std::size_t i=0;i<=question.archivedRuns().size();++i)
      if(ImGui::Selectable(runLabel(i).c_str(),menu.reviewRun()==i))ui.navigate({MenuKind::SelectReviewRun,i});
    ImGui::EndCombo();
  }
  ImGui::Text("Wrong clicks: %zu   Steps needing another try: %zu",review.wrongAttempts,review.stepsNeedingRetry);
  ImGui::Separator();
  ImGui::BeginChild("Reached steps",{0,0});
  text(review.equation);
  ImGui::TextDisabled(review.completed?"Question completed":"Current question - reached steps only");
  for(std::size_t i=0;i<review.steps.size();++i) {
    const auto& step=review.steps[i];
    const bool expanded=menu.expandedReviewStep()==i;
    ImGui::PushID(static_cast<int>(i));
    const auto label=std::string(expanded?"- ":"+ ")+"Step "+std::to_string(i+1)+" / "+reviewOutcome(step.outcome);
    if(ImGui::Selectable(label.c_str(),expanded))ui.navigate({MenuKind::ToggleReviewStep,i});
    if(!step.name.empty())text(step.name);
    if(expanded) {
      text(step.prompt);
      if(step.required>1)ImGui::Text("%zu of %zu found",step.collected,step.required);
      if(!step.working.empty()){ImGui::TextDisabled("Working shown at this step");text(step.working);}
      ImGui::TextDisabled("Your attempts, in order");
      if(step.attempts.empty())ImGui::TextUnformatted("No answers submitted yet.");
      for(std::size_t j=0;j<step.attempts.size();++j) {
        const auto& attempt=step.attempts[j];
        ImGui::TextColored(attempt.correct?ImVec4{0.35F,0.9F,0.8F,1}:ImVec4{1,0.66F,0.45F,1},
          "%zu. %s",j+1,attempt.correct?"Correct":"Incorrect");
        text(attempt.label);
      }
      if(!step.explanation.empty()){ImGui::TextDisabled("Explanation");text(step.explanation);}
    }
    ImGui::Separator();ImGui::PopID();
  }
  ImGui::EndChild();ImGui::EndChild();ImGui::End();
}
void drawMenu(const GalleryMenu& menu,Ui& ui,bool scripted) {
  const auto& io=ImGui::GetIO();
  ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
  const auto flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings;
  ImGui::Begin("Choose questions",nullptr,flags);
  const float width=std::min(720.0F,io.DisplaySize.x-48);
  ImGui::SetCursorPos({(io.DisplaySize.x-width)*0.5F,24});
  ImGui::BeginChild("Question choices",{width,io.DisplaySize.y-48});
  ImGui::SetWindowFontScale(1.35F);
  const bool preparing=menu.preparingNewGame();
  ImGui::TextColored({0.35F,0.9F,0.8F,1},"PATHS");
  ImGui::TextUnformatted(preparing?"New game":"Choose questions");
  ImGui::TextWrapped(preparing?"Start again with a different setup.":"Read each step, then click the ball matching your answer.");
  ImGui::Spacing();
  if(!preparing)for(std::size_t i=0;i<menu.packs().size();++i) {
    const auto& pack=menu.packs()[i];
    ImGui::PushID(pack.id.c_str());
    if(ImGui::Selectable(pack.title.c_str(),menu.selectedPack()==i,0,{0,30}))ui.navigate({MenuKind::SelectPack,i});
    ImGui::Indent(12);ImGui::TextWrapped("%s",pack.description.c_str());ImGui::Unindent(12);
    ImGui::Spacing();ImGui::PopID();
  }
  const auto modes=menu.modes();
  const auto mode=std::find_if(modes.begin(),modes.end(),[&](const auto& m){return m.variation==menu.selectedMode();});
  if(preparing) {
    ImGui::TextWrapped("%s",menu.packs()[menu.selectedPack()].title.c_str());
    const auto variations=galleryVariations();
    const auto saved=std::find_if(variations.begin(),variations.end(),[&](const auto& m){return m.variation==menu.selectedMode();});
    ImGui::TextUnformatted(saved->title.data());
    ImGui::Spacing();
    ImGui::TextWrapped("Starting replaces this practice type's current game, including its progress and answer history. Other games are kept.");
    ImGui::Spacing();
  }
  if(!preparing && modes.size()>1 && ImGui::BeginCombo("Practice type",mode==modes.end()?"Choose":mode->title.data())) {
    for(std::size_t i=0;i<modes.size();++i)if(ImGui::Selectable(modes[i].title.data(),modes[i].variation==menu.selectedMode()))
      ui.navigate({MenuKind::SelectMode,i});
    ImGui::EndCombo();
  }
  const bool resume=menu.canResume();
  auto setup=menu.selectedConfig();
  const auto routes=targetRouteDescriptors();
  const auto motion=std::find_if(routes.begin(),routes.end(),[&](const auto& route){return route.kind==setup.motion;});
  ImGui::BeginDisabled(resume && !preparing);
  if(ImGui::BeginCombo("Movement",motion==routes.end()?"Choose":motion->label.data())) {
    for(std::size_t i=0;i<routes.size();++i) {
      if(routes[i].kind==RouteKind::Waypoints)continue;
      if(ImGui::Selectable(routes[i].label.data(),routes[i].kind==setup.motion))ui.navigate({MenuKind::SelectMotion,i});
    }
    ImGui::EndCombo();
  }
  if(ImGui::SliderFloat("Speed",&setup.pace,0.01F,100.0F,"%.3g m/s",ImGuiSliderFlags_Logarithmic|ImGuiSliderFlags_AlwaysClamp))
    ui.navigate({MenuKind::SetPace,0,setup.pace});
  ImGui::EndDisabled();
  ImGui::TextWrapped(resume && !preparing?"Resume keeps this game's movement settings.":"Movement and speed stay fixed after Play.");
  const bool playable=!modes.empty() || menu.canResume();
  if(preparing) {
    if(ImGui::Button("Start new game",{200,40}))ui.navigate({MenuKind::StartNewGame});
    ImGui::SameLine();if(ImGui::Button("Cancel",{110,40}))ui.navigate({MenuKind::CancelNewGame});
    ImGui::TextWrapped("Cancel keeps your existing game exactly where you left it.");
  } else {
    ImGui::BeginDisabled(!playable);
    if(ImGui::Button(resume?"Resume":"Play",{120,40}))ui.navigate({MenuKind::Play});
    ImGui::EndDisabled();
    if(resume){ImGui::SameLine();if(ImGui::Button("New game",{130,40}))ui.navigate({MenuKind::NewGame});}
    ImGui::SameLine();if(ImGui::Button("Developer workshop",{220,40}))ui.navigate({MenuKind::Workshop});
    ImGui::SameLine();if(ImGui::Button("Quit",{90,40}))ui.quit=true;
    ImGui::TextWrapped("Pause during play to return here. Started sets keep their progress until you close the game.");
  }
  if(!menu.error().empty()) {
    ImGui::TextColored({1,0.65F,0.4F,1},"Could not apply that choice.");
    ImGui::TextWrapped("%s",menu.error().c_str());
    if(!preparing && ImGui::Button("Try loading again"))ui.navigate({MenuKind::SelectPack,menu.selectedPack()});
  }
  if(!scripted && !ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId) && !ImGui::IsAnyItemActive() && !io.WantTextInput) {
    if(preparing && ImGui::IsKeyPressed(ImGuiKey_Escape,false))ui.navigate({MenuKind::CancelNewGame});
    if(!preparing && ImGui::IsKeyPressed(ImGuiKey_UpArrow,false))
      ui.navigate({MenuKind::SelectPack,(menu.selectedPack()+menu.packs().size()-1)%menu.packs().size()});
    if(!preparing && ImGui::IsKeyPressed(ImGuiKey_DownArrow,false))ui.navigate({MenuKind::SelectPack,(menu.selectedPack()+1)%menu.packs().size()});
    if(!preparing && playable && ImGui::IsKeyPressed(ImGuiKey_Enter,false))ui.navigate({MenuKind::Play});
  }
  ImGui::EndChild();ImGui::End();
}
void writeReport(const Options& options,const GalleryScene& scene,const GalleryMenu& menu,std::size_t actions,std::uint64_t frames) {
  const auto* game=menu.activeGame();
  if(options.report.empty())return;
  if(!options.report.parent_path().empty())std::filesystem::create_directories(options.report.parent_path());
  std::ofstream f(options.report);
  f.precision(9);
  f<<"{\n  \"product\": \"paths_gallery\",\n  \"schema_version\": 2,\n  \"frames\": "<<frames
   <<",\n  \"script_actions\": "<<actions<<",\n  \"selected_id\": "<<scene.selectedId().value
   <<",\n  \"paused\": "<<(scene.paused()?"true":"false")<<",\n  \"simulation_ticks\": "<<scene.tickCount();
  const auto vec=[&](iggy3d::Vec3 v){f<<'['<<v.x<<','<<v.y<<','<<v.z<<']';};
  const auto writeMotion=[&](const GalleryConfig& config) {
    const auto routes=targetRouteDescriptors();
    const auto route=std::find_if(routes.begin(),routes.end(),[&](const auto& r){return r.kind==config.motion;});
    f<<", \"motion\": "<<std::quoted(std::string(route==routes.end()?"unknown":route->id))<<", \"pace\": "<<config.pace;
  };
  f<<",\n  \"camera_anchor\": ";vec(scene.camera().anchorPositionMeters);
  f<<",\n  \"camera_yaw\": "<<scene.camera().yawDegrees<<",\n  \"camera_pitch\": "<<scene.camera().pitchDegrees;
  f<<",\n  \"vertices\": "<<scene.frame().vertices.size()<<",\n  \"indices\": "<<scene.frame().indices.size()<<",\n  \"objects\": [";
  bool first=true;
  for(const auto& o:scene.objects()) {
    if(!first)f<<',';first=false;
    f<<"\n    {\"id\": "<<o.id.value<<", \"primitive\": "<<static_cast<int>(o.primitive)<<", \"position\": ";vec(o.position);
    f<<", \"size\": ";vec(o.size);f<<", \"colour\": ";vec(o.color);
    f<<", \"patrol\": "<<(o.route.spec.settings.startsActive?"true":"false")<<", \"route_ticks\": "<<o.motion.tick<<", \"visual_phase\": "<<static_cast<int>(o.phase)<<'}';
  }
  f<<"\n  ],\n  \"menu\": {\"screen\": "
   <<std::quoted(menu.screen()==GalleryScreen::ChooseQuestions?"questions":menu.screen()==GalleryScreen::Playing?"playing":menu.screen()==GalleryScreen::Review?"review":"workshop")
   <<", \"selected_pack\": "<<std::quoted(menu.packs()[menu.selectedPack()].id);
  writeMotion(menu.selectedConfig());
  f<<", \"settings_locked\": "<<(menu.canResume() && !menu.preparingNewGame()?"true":"false")
   <<", \"new_game_setup\": "<<(menu.preparingNewGame()?"true":"false")
   <<", \"started_games\": "<<menu.games().size()<<", \"sessions\": [";
  for(std::size_t i=0;i<menu.games().size();++i) {
    if(i)f<<',';
    const auto& started=menu.games()[i];const auto view=started.session->view();
    const auto modes=galleryVariations();
    const auto mode=std::find_if(modes.begin(),modes.end(),[&](const auto& m){return m.variation==started.variation;});
    f<<"{\"pack\": "<<std::quoted(menu.packs()[started.pack].id)<<", \"mode\": "<<std::quoted(std::string(mode->id));
    writeMotion(started.session->config());
    f<<", \"question\": "<<std::quoted(started.session->question().content().id)<<", \"step\": "<<view.step
     <<", \"correct_hits\": "<<view.correctHits<<", \"wrong_hits\": "<<view.wrongHits
     <<", \"completed_questions\": "<<view.completedQuestions<<", \"paused\": "<<(view.paused?"true":"false")<<'}';
  }
  f<<"]}";
  if(menu.screen()==GalleryScreen::Review) {
    const auto review=*game->question().review(menu.reviewRun());
    f<<",\n  \"review\": {\"selection\": "<<menu.reviewRun()<<", \"run\": "<<review.runNumber
     <<", \"wrong_clicks\": "<<review.wrongAttempts<<", \"steps_needing_retry\": "<<review.stepsNeedingRetry
     <<", \"steps\": [";
    for(std::size_t i=0;i<review.steps.size();++i) {
      if(i)f<<',';
      const auto& step=review.steps[i];
      f<<"{\"id\": "<<step.id.value<<", \"outcome\": "<<std::quoted(reviewOutcome(step.outcome))
       <<", \"collected\": "<<step.collected<<", \"required\": "<<step.required
       <<", \"explanation_available\": "<<(!step.explanation.empty()?"true":"false")
       <<", \"attempts\": "<<step.attempts.size()<<", \"expanded\": "<<(menu.expandedReviewStep()==i?"true":"false")<<'}';
    }
    f<<"]}";
  }
  if(game) {
    const auto view=game->view();
    const auto variations=galleryVariations();
    const auto variation=std::find_if(variations.begin(),variations.end(),[&](const auto& d){return d.variation==game->config().variation;});
    f<<",\n  \"gallery\": {\n    \"variation\": "<<std::quoted(std::string(variation->id));
    writeMotion(game->config());
    f<<", \"seed\": "<<game->config().seed
     <<", \"change_correct_colour\": "<<(game->config().changeCorrectColour?"true":"false")
     <<", \"challenge\": "<<view.challenge.value<<", \"ready\": "<<(view.ready?"true":"false")
     <<", \"correct_hits\": "<<view.correctHits<<", \"wrong_hits\": "<<view.wrongHits
     <<", \"aim_misses\": "<<view.aimMisses<<", \"completed_questions\": "<<view.completedQuestions
     <<", \"feedback\": "<<std::quoted(feedbackLabel(view.feedback))
     <<",\n    \"assignments\": [";
    bool firstChallenge=true;
    for(const auto& challenge:game->challenges()) {
      if(!firstChallenge)f<<',';firstChallenge=false;
      f<<"\n      {\"challenge\": "<<challenge.id.value<<", \"question\": "<<std::quoted(challenge.questionId)
       <<", \"version\": "<<challenge.contentVersion<<", \"run\": "<<challenge.runNumber
       <<", \"step\": "<<challenge.step.value<<", \"assignment_rule_version\": "<<challenge.assignmentRuleVersion
       <<", \"bindings\": [";
      for(std::size_t i=0;i<challenge.count;++i) {
        if(i)f<<',';
        const auto b=challenge.bindings[i];
        f<<"{\"object\": "<<b.object.value<<", \"option\": "<<b.option.value<<", \"token\": "<<static_cast<unsigned>(b.token.value)<<'}';
      }
      f<<"]}";
    }
    f<<"\n    ],\n    \"question_runs\": [";
    bool firstRun=true;
    const auto writeRun=[&](const iggy3d::first_move::LayeredQuestionRunRecord& run) {
      if(!firstRun)f<<',';firstRun=false;
      f<<"\n      {\"question\": "<<std::quoted(run.questionId)<<", \"version\": "<<run.contentVersion
       <<", \"run\": "<<run.runNumber<<", \"prior_exposure\": "<<(run.priorExposure?"true":"false")
       <<", \"completed\": "<<(run.completed?"true":"false")<<", \"steps\": [";
      for(std::size_t i=0;i<run.steps.size();++i) {
        if(i)f<<',';
        const auto& step=run.steps[i];
        f<<"{\"step\": "<<step.id.value<<", \"collected_mask\": "<<static_cast<unsigned>(step.collectedOptions)<<", \"attempts\": [";
        for(std::size_t j=0;j<step.attempts.size();++j) {
          if(j)f<<',';
          const auto a=step.attempts[j];
          f<<"{\"option\": "<<a.option.value<<", \"correct\": "<<(a.correct?"true":"false")<<'}';
        }
        f<<"]}";
      }
      f<<"]}";
    };
    for(const auto& run:game->question().archivedRuns())writeRun(run);
    writeRun(game->question().currentRun());
    f<<"\n    ]\n  }";
  }
  f<<"\n}\n";f.flush();
  if(!f)throw std::runtime_error("Cannot write gallery report");
}
}

int main(int argc,char** argv) {
  try {
    const auto options=parse(argc,argv);
    const char* base=SDL_GetBasePath();
    if(!base)throw std::runtime_error(std::string("Cannot locate bundled content: ")+SDL_GetError());
    GalleryMenu menu(std::filesystem::path(base)/"content/packs",options.game,options.contentPack);
    GalleryScene workshop;
    if(options.startup==GalleryScreen::Playing) {
      if(!menu.launch(options.game.variation))throw std::runtime_error(menu.error());
    } else if(options.startup==GalleryScreen::Workshop) {
      static_cast<void>(menu.dispatch({MenuKind::Workshop}));
    } else static_cast<void>(menu.dispatch({MenuKind::SelectPack,menu.selectedPack()}));
    const auto showsGame=[&] {return menu.screen()==GalleryScreen::Playing || menu.screen()==GalleryScreen::Review;};
    const auto sceneFor=[&]() -> const GalleryScene& {
      return showsGame()?menu.activeGame()->scene():workshop;
    };
    const auto setViewport=[&](SceneViewport v) {
      if(showsGame()) {
        const auto r=menu.activeGame()->dispatch(GalleryViewport{v});
        if(!r.accepted)throw std::runtime_error(std::string(r.reason));
      } else workshop.setViewport(v);
    };
    const auto publish=[&] {
      if(showsGame())static_cast<void>(menu.activeGame()->publishFrame());
      else static_cast<void>(workshop.publishFrame());
    };
    setViewport(viewport(static_cast<float>(options.native.width),static_cast<float>(options.native.height)));publish();
    paths::NativeVulkanHost host(options.native);
    auto& style=ImGui::GetStyle();
    style.WindowPadding={18,14};style.FramePadding={8,5};style.ItemSpacing={8,9};
    style.FrameRounding=4;style.Colors[ImGuiCol_WindowBg]={0.035F,0.05F,0.07F,1};
    style.Colors[ImGuiCol_Button]={0.10F,0.25F,0.28F,1};
    Ui ui;
    std::uint64_t frames=0;
    std::size_t scriptIndex=0,skipped=0;
    const auto navigate=[&](GalleryMenuAction action) {
      const bool accepted=menu.dispatch(action);
      if(accepted) {
        ui.looking=ui.panning=ui.orbiting=false;
        static_cast<void>(workshop.dispatch({Kind::Pause,{},menu.screen()==GalleryScreen::Workshop?0.0F:1.0F}));
      }
      return accepted;
    };
    while(!ui.quit && (!options.frames || frames<options.frames)) {
      // Change scene ownership between frames. The host's frame pointer and
      // this frame's board must refer to the same session for their lifetime.
      if(ui.navigation) {static_cast<void>(navigate(*ui.navigation));ui.navigation.reset();}
      std::optional<ScriptStep> sceneStep;
      if(scriptIndex<options.script.size()) {
        const auto step=options.script[scriptIndex];
        if(step.navigation) {
          if(!navigate(*step.navigation))throw std::runtime_error("Script line "+std::to_string(step.line)+" rejected: "+menu.error());
          ++scriptIndex;
        } else sceneStep=step;
      }
      auto* game=menu.screen()==GalleryScreen::Playing?menu.activeGame():nullptr;
      const auto& scene=sceneFor();
      const auto result=host.frame([&](const SDL_Event& event) {
        if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST) {
          if(game)ui.act(*game,GalleryPause{true});else ui.act(workshop,{Kind::Pause,{},1});
          ui.navigation.reset();ui.looking=ui.panning=ui.orbiting=false;
        }
      },[&] {
        const auto size=ImGui::GetIO().DisplaySize;
        setViewport(viewport(size.x,size.y));
        if(sceneStep) {
          const auto step=*sceneStep;
          if(game)ui.script(*game,step.action);
          else if(menu.screen()==GalleryScreen::Workshop)ui.act(workshop,step.action);
          else ui.last={step.action.kind==Kind::Tick && step.action.amount>=0 && step.action.amount<=0.25F,"only_frame_or_tick_available_in_menu"};
          if(!ui.last.accepted)throw std::runtime_error("Script line "+std::to_string(step.line)+" rejected: "+std::string(ui.last.reason));
          ++scriptIndex;
        }
        const bool scripted=!options.scriptPath.empty();
        const float dt=std::clamp(ImGui::GetIO().DeltaTime,0.0002F,0.25F);
        switch(menu.screen()) {
          case GalleryScreen::Review:drawReview(menu,ui,scripted);break;
          case GalleryScreen::ChooseQuestions:drawMenu(menu,ui,scripted);break;
          case GalleryScreen::Playing:gameInput(*game,ui,scripted);break;
          case GalleryScreen::Workshop:drawUi(workshop,ui,scripted,dt);break;
        }
        if(!scripted && (game || menu.screen()==GalleryScreen::Workshop)) {
          const auto tick=game?game->dispatch(GalleryTick{dt}):workshop.dispatch({Kind::Tick,{},dt});
          if(!tick.accepted)throw std::runtime_error(std::string(tick.reason));
        }
        // Present the question only after input and simulation have committed;
        // the board and the mesh below always describe the same challenge.
        if(game)drawGameUi(*game,ui,scripted);
        publish();
      },&scene.frame());
      if(result.status==paths::FrameStatus::Failed)throw std::runtime_error(result.error);
      if(result.status==paths::FrameStatus::Closed)break;
      if(result.status==paths::FrameStatus::Rendered) {++frames;skipped=0;}
      else if(options.frames && ++skipped>300)throw std::runtime_error("No drawable gallery frames");
    }
    if(scriptIndex!=options.script.size())throw std::runtime_error("Run ended before script completion");
    if(!options.capture.empty()) {
      std::string error;
      if(!host.capture(paths::capturePaths(options.capture),error))throw std::runtime_error(error);
    }
    const auto& scene=sceneFor();
    writeReport(options,scene,menu,scriptIndex,frames);
    std::cerr<<"gallery: frames="<<frames<<" objects="<<scene.objects().size()<<" ticks="<<scene.tickCount()<<'\n';
    return 0;
  } catch(const std::exception& e) {std::cerr<<"gallery: "<<e.what()<<'\n';return 1;}
}

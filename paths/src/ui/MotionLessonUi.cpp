#include "ui/MotionLessonUi.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>

namespace paths {
namespace {
constexpr auto gold=IM_COL32(255,199,77,255),cyan=IM_COL32(89,217,255,255),green=IM_COL32(102,230,166,255),violet=IM_COL32(170,125,230,255);
constexpr auto orange=IM_COL32(242,147,98,255),muted=IM_COL32(157,169,187,255);
constexpr auto flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBringToFrontOnFocus;
NotationBounds bounds(bool enabled=true) {
  const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
  return {a.x,a.y,b.x-a.x,b.y-a.y,enabled && ImGui::IsItemVisible() && !ImGui::GetIO().AppFocusLost};
}
void window(const char* name,SceneViewport r,ImGuiWindowFlags extra=0) {
  ImGui::SetNextWindowPos({r.x,r.y});ImGui::SetNextWindowSize({r.width,r.height});ImGui::Begin(name,nullptr,flags|extra);
}
void formula(MotionLessonUiState& ui,std::string_view text,ImU32 colour,float pixels=14) {
  if(!ui.math){ImGui::TextUnformatted(text.data(),text.data()+text.size());return;}
  auto e=ui.math->layout(text,pixels);const auto available=ImGui::GetContentRegionAvail().x;
  if(e.width>available && e.width>0)e=ui.math->layout(text,std::max(10.0F,pixels*available/e.width));
  if(!e.error.empty()){++ui.formulaErrors;ImGui::TextWrapped("%.*s",static_cast<int>(text.size()),text.data());return;}
  const auto p=ImGui::GetCursorScreenPos();ui.math->draw(e,p.x,p.y,colour);ImGui::Dummy({e.width,e.height});
}
bool button(MotionLessonUiState& ui,MotionControl control,const char* label,float width,bool enabled=true) {
  ImGui::BeginDisabled(!enabled);const bool pressed=ImGui::Button(label,{width,22});
  ui.controls[static_cast<std::size_t>(control)]=bounds(enabled);ImGui::EndDisabled();return pressed;
}
void graph(MotionLessonUiState& ui,SceneViewport rect) {
  auto& lesson=*ui.lesson;const auto index=lesson.progress().selected;const auto& c=lesson.chapter();
  const bool position=c.profile==MotionProfile::Position;
  const double minimum=position?-2:-4,maximum=position?14:8;
  window("Motion graph",rect,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
  auto* draw=ImGui::GetWindowDrawList();
  const SceneViewport plot{rect.x+38,rect.y+16,std::max(1.0F,rect.width-51),std::max(1.0F,rect.height-45)};
  ui.graph={plot.x,plot.y,plot.width,plot.height,true};
  const auto point=[&](double time,double value){return ImVec2{plot.x+static_cast<float>(time/c.duration)*plot.width,plot.y+static_cast<float>((maximum-value)/(maximum-minimum))*plot.height};};
  char label[80];
  draw->AddText({rect.x+8,rect.y+1},muted,position?"x (m)":"v (m/s)");
  for(int i=0;i<=4;++i) {
    const double value=minimum+(maximum-minimum)*i/4;const auto p=point(0,value);
    draw->AddLine(p,point(c.duration,value),IM_COL32(49,61,77,255));
    std::snprintf(label,sizeof(label),"%g",value);draw->AddText({rect.x+8,p.y-5},muted,label);
  }
  for(int i=0;i<=2;++i) {
    const double t=c.duration*i/2;const auto p=point(t,minimum);
    draw->AddLine(p,point(t,maximum),IM_COL32(49,61,77,255));
    std::snprintf(label,sizeof(label),"%g",t);draw->AddText({p.x-3,p.y+2},muted,label);
  }
  draw->AddText({rect.x+rect.width-27,plot.y+plot.height+2},muted,"s");
  draw->AddLine(point(0,0),point(c.duration,0),IM_COL32(119,133,150,255));
  const auto curve=[&](const MotionPlan& plan,ImU32 colour,bool fill) {
    const auto journey=motionJourney(index,plan);
    for(std::size_t i=0;i<journey.count;++i) {
      const auto& s=journey.segments[i];const auto a=point(s.begin,position?c.start:s.firstVelocity);
      const auto b=point(s.end,position?plan.values[0]:s.lastVelocity);
      if(fill && !position)draw->AddQuadFilled(point(s.begin,0),a,b,point(s.end,0),s.firstVelocity+s.lastVelocity<0?IM_COL32(242,147,98,48):IM_COL32(89,217,255,45));
      draw->AddLine(a,b,colour,fill?2.5F:1.5F);
      if(i && c.profile==MotionProfile::Sections) {
        const auto previous=point(s.begin,journey.segments[i-1].lastVelocity);
        for(int n=0;n<10;n+=2)draw->AddLine({a.x,previous.y+(a.y-previous.y)*n/10},{a.x,previous.y+(a.y-previous.y)*(n+1)/10},colour);
      }
    }
  };
  if(const auto* previous=lesson.ghost())curve(previous->plan,violet,false);
  curve(lesson.shownPlan(),cyan,true);
  const auto sample=lesson.sample();const auto cursor=point(sample.time,position?sample.position:sample.velocity);
  draw->AddLine({cursor.x,plot.y},{cursor.x,plot.y+plot.height},IM_COL32(241,241,233,140));draw->AddCircleFilled(cursor,3,IM_COL32_WHITE);
  std::snprintf(label,sizeof(label),"t %.2f s   x %.2f m   v %.2f m/s",sample.time,sample.position,sample.velocity);
  draw->AddText({rect.x+8,rect.y+rect.height-14},cyan,label);
  ui.handles={};
  const bool editable=!lesson.run().solved && !lesson.progress().inspected;
  ImGui::BeginDisabled(!editable);
  const auto journey=motionJourney(index,lesson.shownPlan());
  const std::size_t count=c.profile==MotionProfile::Sections?2:1;
  for(std::size_t i=0;i<count;++i) {
    ImGui::PushID(static_cast<int>(i));
    double t=position?c.duration:(journey.segments[i].begin+journey.segments[i].end)*.5;
    if(c.profile==MotionProfile::Ramp)t=lesson.shownPlan().values[1];
    const auto p=point(t,lesson.shownPlan().values[i]);
    ImGui::SetCursorScreenPos({p.x-10,p.y-10});
    ImGui::InvisibleButton("Graph handle",{20,20});ui.handles[i]=bounds(editable);
    draw->AddCircle(p,6,editable?cyan:green,16,2);draw->AddCircleFilled(p,2,cyan);
    if(ImGui::IsItemActivated())(void)lesson.dispatch({MotionActionKind::BeginEdit});
    if(ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      const auto mouse=ImGui::GetIO().MousePos;
      (void)lesson.dispatch({MotionActionKind::SetParameter,i,maximum-(mouse.y-plot.y)/plot.height*(maximum-minimum)});
      if(c.profile==MotionProfile::Ramp)(void)lesson.dispatch({MotionActionKind::SetParameter,1,(mouse.x-plot.x)/plot.width*c.duration});
    }
    if(ImGui::IsItemDeactivated())(void)lesson.dispatch({MotionActionKind::EndEdit});
    if(ImGui::IsItemHovered())ImGui::SetTooltip(c.profile==MotionProfile::Ramp?"Drag the cyan peak. Left/right changes time; up/down changes velocity.":"Drag vertically to adjust the cyan plan.");
    ImGui::PopID();
  }
  ImGui::EndDisabled();ImGui::End();
}
void working(MotionLessonUiState& ui,SceneViewport rect) {
  auto& lesson=*ui.lesson;const auto& c=lesson.chapter();const auto& run=lesson.run();
  window("Motion working",rect,ImGuiWindowFlags_HorizontalScrollbar);
  ui.working={rect.x,rect.y,rect.width,rect.height,true};
  ImGui::TextColored({.35F,.85F,1,1},"Working / attempts");
  if(lesson.progress().inspected) {
    ImGui::TextColored({.67F,.49F,.9F,1},"Inspecting run %zu",*lesson.progress().inspected+1);
    if(button(ui,MotionControl::ReturnToPlan,"My plan",72))(void)lesson.dispatch({MotionActionKind::ReturnToPlan});
  }
  for(const auto& line:motionWorking(lesson.progress().selected,lesson.shownPlan()))formula(ui,line,cyan);
  const auto sample=lesson.sample();
  if(c.profile!=MotionProfile::Position) {
    if(sample.acceleration)ImGui::TextDisabled("At cursor: a = %.2f m/s^2",*sample.acceleration);
    else ImGui::TextDisabled("At this corner, acceleration is undefined.");
  }
  const bool judged=lesson.progress().inspected.has_value() || (!run.attempts.empty() && run.attempts.back().plan==run.plan && !lesson.playing());
  if(judged) {
    const auto evaluation=evaluateMotion(lesson.progress().selected,lesson.shownPlan());
    for(const auto& check:evaluation.checks)ImGui::TextColored(check.passed?ImVec4{.4F,.9F,.65F,1}:ImVec4{.95F,.58F,.38F,1},"%s %.*s: %.2f",check.passed?"[ok]":"[try]",static_cast<int>(check.goal.label.size()),check.goal.label.data(),check.measured);
  }
  if(!lesson.message().empty())ImGui::TextWrapped("%.*s",static_cast<int>(lesson.message().size()),lesson.message().data());
  if(run.solved && !lesson.progress().inspected) {
    ImGui::TextColored({.4F,.9F,.65F,1},"Complete. Your result stays here.");
    if(button(ui,MotionControl::Again,"Again",58))(void)lesson.dispatch({MotionActionKind::Again});
    if(lesson.progress().selected+1==motionChapterCount)ImGui::TextWrapped("Five chapters available. Return to Contents or explore a different peak time with Again.");
  }
  if(ImGui::CollapsingHeader("Symbols and meaning")) {
    ImGui::TextWrapped("x: position (m). t: elapsed time (s). v: signed velocity (m/s). a: acceleration (m/s^2). Delta x: displacement. d: total distance.");
    ImGui::TextWrapped("%.*s",static_cast<int>(c.explanation.size()),c.explanation.data());
  }
  const bool opened=ImGui::CollapsingHeader("Choose a symbolic plan",ui.symbols?ImGuiTreeNodeFlags_DefaultOpen:0);
  ui.controls[static_cast<std::size_t>(MotionControl::Symbols)]=bounds();ui.symbols=opened;
  ui.choices={};
  if(opened) {
    ImGui::TextWrapped("Tiles set the same controls. Run the plan to test it.");
    ImGui::BeginDisabled(run.solved || lesson.progress().inspected.has_value());
    for(std::size_t i=0;i<c.choices.size();++i) {
      ImGui::PushID(static_cast<int>(i));const auto p=ImGui::GetCursorScreenPos();
      const auto e=ui.math?ui.math->layout(c.choices[i].latex,14):NativeMath::Equation{};
      const bool pressed=ImGui::Button("##plan",{std::max(80.0F,ImGui::GetContentRegionAvail().x),std::max(26.0F,e.height+8)});
      ui.choices[i]=bounds(!run.solved && !lesson.progress().inspected);
      if(ui.math && e.error.empty())ui.math->draw(e,p.x+8,p.y+4,cyan);
      if(pressed)(void)lesson.dispatch({MotionActionKind::ChoosePlan,i});ImGui::PopID();
    }
    ImGui::EndDisabled();
  }
  ui.attempts.clear();
  if(!run.attempts.empty() && ImGui::CollapsingHeader("Run history",ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::TextWrapped("Violet compares the previous different plan. Inspecting a run keeps your current plan intact.");
    for(std::size_t i=0;i<run.attempts.size();++i) {
      const auto& attempt=run.attempts[i];char label[80];std::snprintf(label,sizeof(label),"%zu  %s  [%.1f, %.1f]",i+1,attempt.passed?"Complete":"Try again",attempt.plan.values[0],attempt.plan.values[1]);
      if(ImGui::Selectable(label,lesson.progress().inspected==i))(void)lesson.dispatch({MotionActionKind::Review,i});ui.attempts.push_back(bounds());
    }
  }
  if(!ui.progressMessage.empty())ImGui::TextWrapped("%s",ui.progressMessage.c_str());
  ImGui::End();
}
}
void drawMotionLesson(MotionLessonUiState& ui) {
  if(!ui.lesson)return;
  auto& lesson=*ui.lesson;auto& io=ImGui::GetIO();ui.presented=true;ui.controls={};ui.parameters={};ui.formulaErrors=0;
  const bool narrow=io.DisplaySize.x<680;const float gap=4,header=narrow?108:114,transport=30;
  const float body=io.DisplaySize.y-header-transport,left=narrow?io.DisplaySize.x:io.DisplaySize.x*.64F;
  const float trackHeight=narrow?98:body*.43F,graphHeight=narrow?114:body*.36F;
  const float parameterHeight=lesson.chapter().parameters.size()*26+8;
  ImGui::PushFont(nullptr,13);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{8,4});ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{5,3});
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{5,3});ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,3);
  ImGui::PushStyleColor(ImGuiCol_WindowBg,{.055F,.065F,.085F,1});
  ImGui::PushStyleColor(ImGuiCol_Button,{.13F,.19F,.24F,1});ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{.20F,.32F,.39F,1});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,{.22F,.40F,.48F,1});ImGui::PushStyleColor(ImGuiCol_SliderGrab,{.35F,.85F,1,1});
  ImGui::BeginDisabled(io.AppFocusLost);
  window("Motion question",{0,0,io.DisplaySize.x,header},ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
  if(button(ui,MotionControl::Back,"Contents",66)){if(lesson.playing())(void)lesson.dispatch({MotionActionKind::Pause});ui.open=false;}
  ImGui::SameLine();ImGui::TextColored({.35F,.85F,1,1},"Drive the velocity graph");
  const auto specs=motionChapters();ui.chapters={};
  if(narrow) {
    ImGui::SetNextItemWidth(-1);const auto& selected=lesson.chapter();
    const auto label=std::to_string(lesson.progress().selected+1)+". "+std::string(selected.title);
    const bool open=ImGui::BeginCombo("##Motion chapter",label.c_str());ui.controls[static_cast<std::size_t>(MotionControl::Chapter)]=bounds();
    if(open) {
      for(std::size_t i=0;i<specs.size();++i) {
        const auto& r=lesson.progress().runs[i];const auto item=std::to_string(i+1)+". "+std::string(specs[i].title)+(r.solved?" [done]":r.started?" [started]":"");
        if(ImGui::Selectable(item.c_str(),lesson.progress().selected==i))(void)lesson.dispatch({MotionActionKind::Select,i});ui.chapters[i]=bounds();
      }
      ImGui::EndCombo();
    }
  } else {
    for(std::size_t i=0;i<specs.size();++i) {
      if(i)ImGui::SameLine();ImGui::PushID(static_cast<int>(i));const auto& r=lesson.progress().runs[i];
      const auto label=std::to_string(i+1)+(r.solved?" +":r.started?" *":"");
      if(ImGui::Button(label.c_str(),{35,22}))(void)lesson.dispatch({MotionActionKind::Select,i});ui.chapters[i]=bounds();
      if(ImGui::IsItemHovered())ImGui::SetTooltip("%.*s",static_cast<int>(specs[i].title.size()),specs[i].title.data());ImGui::PopID();
    }
    ImGui::SameLine();ImGui::TextUnformatted(lesson.chapter().title.data());
  }
  ImGui::PushTextWrapPos(0);ImGui::TextColored({1,.78F,.3F,1},"%.*s",static_cast<int>(lesson.chapter().prompt.size()),lesson.chapter().prompt.data());ImGui::PopTextWrapPos();ui.question=bounds();
  if(!narrow)formula(ui,lesson.chapter().given,gold,16);
  ImGui::End();
  window("Motion transport",{0,header,io.DisplaySize.x,transport},ImGuiWindowFlags_NoScrollbar);
  ImGui::PushStyleColor(ImGuiCol_Button,{.11F,.35F,.39F,1});
  if(button(ui,MotionControl::Run,(lesson.run().solved || lesson.progress().inspected)?"Replay":"Run",48))(void)lesson.dispatch({MotionActionKind::Run});ImGui::PopStyleColor();
  ImGui::SameLine();if(button(ui,MotionControl::Pause,lesson.playing()?"Pause":"Resume",52,lesson.playing() || lesson.progress().recording || (lesson.progress().time>0 && lesson.progress().time<lesson.chapter().duration)))(void)lesson.dispatch({MotionActionKind::Pause});
  ImGui::SameLine();if(button(ui,MotionControl::Rewind,"|<",28))(void)lesson.dispatch({MotionActionKind::Rewind});
  ImGui::SameLine();ImGui::SetNextItemWidth(58);
  constexpr std::array<double,4> speeds{.25,.5,1,2};constexpr const char* names[]={"0.25x","0.5x","1x","2x"};
  const auto selected=static_cast<int>(std::find(speeds.begin(),speeds.end(),lesson.progress().speed)-speeds.begin());int speed=selected;
  if(ImGui::Combo("##Speed",&speed,names,4))(void)lesson.dispatch({MotionActionKind::Speed,0,speeds[speed]});ui.controls[static_cast<std::size_t>(MotionControl::Speed)]=bounds();
  ImGui::SameLine();if(button(ui,MotionControl::Undo,"Undo",40,!lesson.run().solved && !lesson.run().undo.empty() && !lesson.progress().inspected))(void)lesson.dispatch({MotionActionKind::Undo});
  ImGui::SameLine();ImGui::PushStyleColor(ImGuiCol_Button,{.16F,.38F,.28F,1});
  if(button(ui,MotionControl::Next,"Next",42,lesson.run().solved && lesson.progress().selected+1<motionChapterCount))(void)lesson.dispatch({MotionActionKind::Next});ImGui::PopStyleColor();
  if(!narrow){ImGui::SameLine();ImGui::TextDisabled("Cyan: your plan   Gold: targets   Violet: previous plan");}
  ImGui::End();
  const float top=header+transport;
  const SceneViewport track{0,top,left,trackHeight};
  window("Motion track",track,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
  (void)ui.scene.publish(lesson,{track.x,track.y+24,track.width,track.height-24});ui.track={track.x,track.y+24,track.width,track.height-24,true};
  auto* draw=ImGui::GetWindowDrawList();
  for(int x=static_cast<int>(ui.scene.minimum());x<=static_cast<int>(ui.scene.maximum());x+=(ui.scene.maximum()-ui.scene.minimum()>25?4:2)) {
    const auto p=ui.scene.project({static_cast<float>(x),.02F,1.2F});char text[20];std::snprintf(text,sizeof(text),"%d m",x);draw->AddText({p.x-8,p.y},muted,text);
  }
  ImGui::TextDisabled("3D track");ImGui::SameLine();ImGui::SetNextItemWidth(-1);
  float time=static_cast<float>(lesson.progress().time);
  if(ImGui::SliderFloat("##time",&time,0,static_cast<float>(lesson.chapter().duration),"t = %.2f s",ImGuiSliderFlags_NoInput))(void)lesson.dispatch({MotionActionKind::Scrub,0,time});ui.controls[static_cast<std::size_t>(MotionControl::Timeline)]=bounds();
  ImGui::End();
  graph(ui,{0,top+trackHeight+gap,left,graphHeight});
  const float parameterTop=top+trackHeight+graphHeight+2*gap;
  window("Motion controls",{0,parameterTop,left,narrow?parameterHeight:std::max(parameterHeight,io.DisplaySize.y-parameterTop)},ImGuiWindowFlags_NoScrollbar);
  const bool editable=!lesson.run().solved && !lesson.progress().inspected;
  ImGui::BeginDisabled(!editable);
  for(std::size_t i=0;i<lesson.chapter().parameters.size();++i) {
    const auto& p=lesson.chapter().parameters[i];float value=static_cast<float>(lesson.shownPlan().values[i]);
    ImGui::PushID(static_cast<int>(i));ImGui::TextUnformatted(p.label.data());ImGui::SameLine(narrow?142:std::min(180.0F,left*.38F));ImGui::SetNextItemWidth(-1);
    const bool changed=ImGui::SliderFloat("##value",&value,static_cast<float>(p.minimum),static_cast<float>(p.maximum),"%.1f",ImGuiSliderFlags_NoInput);
    ui.parameters[i]=bounds(editable);
    if(ImGui::IsItemActivated())(void)lesson.dispatch({MotionActionKind::BeginEdit});
    if(changed)(void)lesson.dispatch({MotionActionKind::SetParameter,i,value});
    if(ImGui::IsItemDeactivated())(void)lesson.dispatch({MotionActionKind::EndEdit});ImGui::PopID();
  }
  ImGui::EndDisabled();
  if(!narrow) {
    ImGui::TextDisabled("Drag cyan handles or sliders. Run records a full attempt.");
  }
  ImGui::End();
  // A click on the graph timeline scrubs without manufacturing a completed run.
  if(!io.AppFocusLost && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && io.MousePos.x>=ui.graph.x && io.MousePos.x<=ui.graph.x+ui.graph.width && io.MousePos.y>=ui.graph.y && io.MousePos.y<=ui.graph.y+ui.graph.height)
    (void)lesson.dispatch({MotionActionKind::Scrub,0,(io.MousePos.x-ui.graph.x)/ui.graph.width*lesson.chapter().duration});
  const float workingTop=narrow?parameterTop+parameterHeight:top;
  working(ui,narrow?SceneViewport{0,workingTop,io.DisplaySize.x,std::max(20.0F,io.DisplaySize.y-workingTop)}:SceneViewport{left+gap,top,io.DisplaySize.x-left-gap,body});
  ImGui::EndDisabled();ImGui::PopStyleColor(5);ImGui::PopStyleVar(4);ImGui::PopFont();
}
} // namespace paths

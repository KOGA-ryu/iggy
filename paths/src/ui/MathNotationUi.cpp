#include "ui/MathNotationUi.hpp"

#include <algorithm>
#include <imgui.h>

namespace paths {
namespace {
NotationBounds bounds(bool available=true) {
  const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
  return {a.x,a.y,b.x-a.x,b.y-a.y,available && ImGui::IsItemVisible()};
}
constexpr ImVec4 violet{.76F,.65F,1,1},cyan{.3F,.85F,.95F,1},green{.4F,.9F,.65F,1};
}
void syncMathNotation(MathNotationUiState& ui,std::string_view question,std::uint32_t run) {
  if(ui.question!=question || ui.run!=run) {ui={};ui.question=question;ui.run=run;}
  ui.toggle={};ui.panel={};ui.body={};ui.detail={};ui.controls={};ui.tokens={};
}
void drawMathNotationToggle(MathNotationUiState& ui,bool available,bool blocked) {
  if(!available)return;
  const auto cursor=ImGui::GetCursorPos();
  ImGui::SameLine(ImGui::GetWindowWidth()-70);
  ImGui::PushFont(nullptr,12);ImGui::BeginDisabled(blocked);
  ImGui::PushStyleColor(ImGuiCol_Button,{.30F,.20F,.46F,1});
  if(ImGui::Button("Symbols",{58,16})) {ui.open=!ui.open;ui.top=true;}
  ui.toggle=bounds(!blocked);ImGui::PopStyleColor();ImGui::EndDisabled();ImGui::PopFont();
  ImGui::SetCursorPos(cursor);
}
void drawMathNotation(MathNotationUiState& ui,std::span<const iggy3d::first_move::NotationLesson> lessons,bool blocked,bool paused) {
  namespace fm=iggy3d::first_move;
  if(lessons.empty()) {ui.open=false;return;}
  ui.lesson=std::min(ui.lesson,lessons.size()-1);
  const auto at=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();ui.panel={at.x,at.y,size.x,size.y,true};
  ImGui::PushFont(nullptr,13);ImGui::PushID("Math notation");ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat,false);
  ImGui::BeginDisabled(blocked);
  const std::array labels{"Close","<",">",ui.practice?"Learn###mode":"Try###mode","Up","Dn"};
  const std::array enabled{true,ui.lesson>0,ui.lesson+1<lessons.size(),lessons[ui.lesson].check.has_value(),ui.scroll>0,ui.scroll<ui.scrollMax};
  std::optional<float> scroll;
  for(std::size_t i=0;i<labels.size();++i) {
    if(i)ImGui::SameLine(0,4);
    ImGui::BeginDisabled(!enabled[i]);
    if(ImGui::Button(labels[i],{i==0?40.0F:i==3?42.0F:24.0F,18})) {
      switch(static_cast<NotationControl>(i)) {
        case NotationControl::Close:ui.open=false;break;
        case NotationControl::Previous:--ui.lesson;ui.token=0;ui.top=true;ui.verdict=fm::NotationVerdict::Unavailable;break;
        case NotationControl::Next:++ui.lesson;ui.token=0;ui.top=true;ui.verdict=fm::NotationVerdict::Unavailable;break;
        case NotationControl::Mode:ui.practice=!ui.practice;ui.top=true;ui.verdict=fm::NotationVerdict::Unavailable;break;
        case NotationControl::Up:scroll=std::max(0.0F,ui.scroll-ui.pageHeight*.8F);break;
        case NotationControl::Down:scroll=std::min(ui.scrollMax,ui.scroll+ui.pageHeight*.8F);break;
        case NotationControl::Count:break;
      }
    }
    ui.controls[i]=bounds(!blocked && enabled[i]);ImGui::EndDisabled();
  }
  ImGui::SameLine(0,6);ImGui::TextColored(violet,"%zu/%zu",ui.lesson+1,lessons.size());
  if(ui.top)scroll=0;
  if(scroll)ImGui::SetNextWindowScroll({0,*scroll});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
  ImGui::BeginChild("Notation body",{0,0},ImGuiChildFlags_NavFlattened,ImGuiWindowFlags_NoSavedSettings);
  ImGui::PopStyleVar();
  const auto bodyAt=ImGui::GetWindowPos(),bodySize=ImGui::GetWindowSize();ui.body={bodyAt.x,bodyAt.y,bodySize.x,bodySize.y,true};
  const auto& lesson=lessons[ui.lesson];ui.token=std::min(ui.token,lesson.tokens.size()-1);
  if(!lesson.check)ui.practice=false;
  ImGui::PushID(lesson.id.c_str());
  ImGui::TextColored(violet,"%s",lesson.title.c_str());ImGui::TextWrapped("%s",lesson.context.c_str());
  ImGui::TextWrapped("%s",ui.practice?lesson.check->prompt.c_str():"Select a symbol to read its meaning here.");
  ImGui::BeginDisabled(paused && ui.practice);
  for(std::size_t i=0;i<lesson.tokens.size();++i) {
    const auto& token=lesson.tokens[i];
    const float width=std::min(ImGui::GetWindowWidth()-16,std::max(28.0F,ImGui::CalcTextSize(token.text.c_str()).x+16));
    if(i && ImGui::GetItemRectMax().x+6+width<ImGui::GetWindowPos().x+ImGui::GetWindowWidth()-8)ImGui::SameLine(0,6);
    ImGui::PushID(static_cast<int>(i));
    ImGui::PushStyleColor(ImGuiCol_Button,ui.token==i?ImVec4{.08F,.39F,.46F,1}:ImVec4{.09F,.14F,.18F,1});
    if(ImGui::Button(token.text.c_str(),{width,24})) {
      ui.token=i;ui.follow=true;
      if(ui.practice)ui.verdict=fm::checkNotation(lesson,i);
    }
    ui.tokens[i]=bounds(!blocked && !(paused && ui.practice));ImGui::PopStyleColor();ImGui::PopID();
  }
  ImGui::EndDisabled();ImGui::Separator();ImGui::BeginGroup();
  if(ui.practice) {
    const bool correct=ui.verdict==fm::NotationVerdict::Correct;
    ImGui::TextColored(correct?green:violet,"%s",correct?"Read correctly":ui.verdict==fm::NotationVerdict::Retry?"Try another symbol":"Reading practice");
    if(ui.follow)ImGui::SetScrollHereY(0);
    if(ui.verdict!=fm::NotationVerdict::Unavailable)ImGui::TextWrapped("%s",(correct?lesson.check->correct:lesson.check->retry).c_str());
    ImGui::TextWrapped("This separate example leaves your problem and progress unchanged.");
  } else {
    const auto& token=lesson.tokens[ui.token];const auto& definition=token.definition;
    ImGui::TextColored(cyan,"%s",definition.title.c_str());if(ui.follow)ImGui::SetScrollHereY(0);
    ImGui::TextWrapped("%s",definition.meaning.c_str());
    ImGui::TextDisabled("HERE");ImGui::TextWrapped("%s",token.role.c_str());
    ImGui::TextDisabled("DEFINITION");ImGui::TextWrapped("%s",definition.definition.c_str());
    ImGui::TextDisabled("EXAMPLE");ImGui::TextWrapped("%s",definition.example.c_str());
    ImGui::TextDisabled("READ THE WHOLE EXPRESSION");ImGui::TextWrapped("%s",lesson.reading.c_str());
  }
  ImGui::EndGroup();ui.detail=bounds();ImGui::PopID();
  ui.scroll=ImGui::GetScrollY();ui.scrollMax=ImGui::GetScrollMaxY();ui.pageHeight=bodySize.y;
  ui.top=false;ui.follow=false;
  ImGui::EndChild();ImGui::EndDisabled();ImGui::PopItemFlag();ImGui::PopID();ImGui::PopFont();
}
} // namespace paths

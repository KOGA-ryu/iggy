#include "DocumentLessonUi.hpp"
#include <algorithm>
#include <utility>
#include <imgui.h>

namespace paths {
namespace {
NotationBounds item(bool enabled=true){const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();return {a.x,a.y,b.x-a.x,b.y-a.y,enabled && ImGui::IsItemVisible()};}
}
bool applyDocumentReadingAction(DocumentReadingUiState& ui,const CorpusEntry& e,const BookAction& action) {
  const auto found=std::find_if(e.lesson.begin(),e.lesson.end(),[&](const auto& b){return b.id==action.target;});
  if(ui.entry!=e.id || found==e.lesson.end())return false;
  switch(action.kind) {
    case BookActionKind::OpenBlock:ui.anchor=found->id;return true;
    case BookActionKind::ToggleHelp: {
      const auto h=static_cast<unsigned>(action.help);
      if(h>=found->help.size() || found->help[h].empty())return false;
      ui.helpMasks.resize(e.lesson.size());ui.helpMasks[found-e.lesson.begin()]^=static_cast<std::uint8_t>(1u<<h);return true;
    }
    default:return false;
  }
}
std::optional<BookAction> drawDocumentReading(const CorpusEntry& e,NativeMath& math,DocumentReadingUiState& ui,bool blocked) {
  if(ui.entry!=e.id){ui.entry=e.id;ui.book={};ui.helpMasks.assign(e.lesson.size(),0);ui.anchor.clear();ImGui::SetScrollY(0);}
  ui.fallbacks=0;ui.reading={};std::optional<BookAction> intent;
  const auto top=ImGui::GetCursorScreenPos();
  if(e.lesson.empty()) {
    const auto& document=math.layoutDocument(e.body,std::max(1.0F,ImGui::GetContentRegionAvail().x));
    math.draw(document,top.x,top.y,IM_COL32(221,221,226,255),IM_COL32(89,217,255,255),IM_COL32(255,199,77,255));
    ImGui::Dummy({document.width,document.height});ui.reading=item();ui.fallbacks=document.fallbacks;return intent;
  }
  ImGui::BeginDisabled(blocked || ui.textScale<=.9001f);if(ImGui::Button("A-"))ui.textScale=std::max(.9f,ui.textScale-.1f);ImGui::EndDisabled();ImGui::SameLine();
  ImGui::BeginDisabled(blocked || ui.textScale>=1.9999f);if(ImGui::Button("A+"))ui.textScale=std::min(2.f,ui.textScale+.1f);ImGui::EndDisabled();ImGui::SameLine();
  ImGui::TextDisabled("%.0f%%",100*ui.textScale);
  ImGui::SetNextItemWidth(-1);
  if(ImGui::BeginCombo("##reading-sections","Jump to a definition or example")) {
    for(const auto& b:e.lesson)if(ImGui::Selectable((b.number+" / "+b.title).c_str()) && !blocked)
      ui.anchor=b.id;
    ImGui::EndCombo();
  }
  ImGui::PushFont(nullptr,ImGui::GetFontSize()*ui.textScale);ui.book.fallbacks=0;
  const float available=ImGui::GetContentRegionAvail().x;
  const float width=std::max(1.f,std::min(available,ImGui::CalcTextSize("abcdefghijklmnopqrstuvwxyz").x/26*72));
  const float inset=std::max(0.f,(available-width)*.5f);
  if(inset>0)ImGui::Indent(inset);
  ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+width);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{ImGui::GetStyle().ItemSpacing.x,8*ui.textScale});
  const auto anchor=std::exchange(ui.anchor,{});
  for(const auto& b:bookLessonView(e.lesson,ui.helpMasks)) {
    if(anchor==b.id)ImGui::SetScrollFromPosY(ImGui::GetCursorScreenPos().y-ImGui::GetWindowPos().y,0.f);
    const auto action=drawBookBlock(b,ui.book,math,width);
    if(action && !blocked && applyDocumentReadingAction(ui,e,*action))intent=action;
  }
  ImGui::PopStyleVar();ImGui::PopTextWrapPos();if(inset>0)ImGui::Unindent(inset);ImGui::PopFont();
  ui.fallbacks=ui.book.fallbacks;const auto end=ImGui::GetCursorScreenPos();ui.reading={top.x,top.y,available,end.y-top.y,!blocked};
  return intent;
}
std::optional<std::string> drawDocumentLesson(const CorpusEntry& e,NativeMath& math,DocumentLessonUiState& ui,bool blocked) {
  ui.presented=false;ui.viewport={};ui.reading={};ui.questionLinks.clear();ui.parameterControls.clear();ui.fallbacks=0;
  if(ui.entry!=e.id) {
    ui.object=e.figure?std::make_unique<MathObjects>(instantiateDocumentFigure(*e.figure)):nullptr;
    ui.scene=MathObjectScene{};
  }
  ImGui::TextColored({1,.78F,.3F,1},"%s",e.title.c_str());
  std::optional<std::string> chosen;
  if(!e.questions.empty()) {
    ImGui::PushStyleColor(ImGuiCol_Button,{.15F,.27F,.46F,1});
    float used=0;const auto width=ImGui::GetContentRegionAvail().x;
    for(std::size_t i=0;i<e.questions.size();++i) {
      if(used && used+94<=width)ImGui::SameLine();else used=0;
      ImGui::PushID(e.questions[i].c_str());const auto label="Question "+std::to_string(i+1);
      if(ImGui::Button(label.c_str(),{88,24}))chosen=e.questions[i];
      ui.questionLinks.emplace_back(e.questions[i],item(!blocked));ImGui::PopID();used+=94;
    }
    ImGui::PopStyleColor();
  }
  const auto available=ImGui::GetContentRegionAvail();const bool figure=e.figure && ui.object;
  const bool wide=available.x>=700;const float gap=8;
  const ImVec2 readingSize{figure && wide?(available.x-gap)*.52F:available.x,figure && !wide?std::max(110.0F,available.y*.45F):available.y};
  ImGui::BeginChild("Document reading",readingSize,ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar);
  (void)drawDocumentReading(e,math,ui,blocked);
  if(figure) {
    ImGui::TextWrapped("%s",e.figure->caption.c_str());
    if(!e.figure->parameters.empty() && ImGui::CollapsingHeader("Diagram controls")) {
      for(const auto& [key,initial]:e.figure->parameters) {
        const auto specs=mathParameterSpecs();const auto p=std::find_if(specs.begin(),specs.end(),[&](const auto& p){return p.key==key && p.owner==ui.object->snapshot().kind;});
        if(p==specs.end())continue;
        auto value=ui.object->parameter(p->id);ImGui::SetNextItemWidth(-1);
        if(ImGui::SliderScalar(std::string(p->label).c_str(),ImGuiDataType_Double,&value,&p->minimum,&p->maximum,"%.3f"))
          (void)ui.object->dispatch({MathActionKind::SetParameter,p->owner,p->id,value});
        ui.parameterControls.emplace_back(key,item(!blocked));
      }
      if(ImGui::Button("Reset diagram")){*ui.object=instantiateDocumentFigure(*e.figure);ui.scene=MathObjectScene{};}
    }
  }
  ImGui::EndChild();
  if(figure) {
    if(wide)ImGui::SameLine(0,gap);
    const auto left=ImGui::GetContentRegionAvail();
    ImGui::BeginChild("Document 3D",{left.x,left.y},ImGuiChildFlags_None,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    const auto p=ImGui::GetCursorScreenPos(),space=ImGui::GetContentRegionAvail();
    if(space.x>1 && space.y>1) {
      ImGui::InvisibleButton("Orbit document diagram",space,ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonRight);ui.viewport=item(!blocked);
      const SceneViewport area{p.x,p.y,space.x,space.y};(void)ui.scene.publish(ui.object->snapshot(),area);
      using Nav=iggy3d::ProductCreativeViewportNavigationOperation;const auto& io=ImGui::GetIO();
      if(!blocked && ImGui::IsItemActive()) {
        if(ImGui::IsMouseDragging(0))(void)ui.scene.navigate(Nav::Orbit,io.MouseDelta.x,io.MouseDelta.y);
        if(ImGui::IsMouseDragging(1))(void)ui.scene.navigate(Nav::Pan,io.MouseDelta.x,io.MouseDelta.y);
      }
      if(!blocked && ImGui::IsItemHovered() && io.MouseWheel)(void)ui.scene.navigate(Nav::Dolly,0,io.MouseWheel);
      (void)ui.scene.publish(ui.object->snapshot(),area);ui.presented=true;
      auto* overlay=ImGui::GetForegroundDrawList();overlay->PushClipRect(p,{p.x+space.x,p.y+space.y},true);
      const auto& state=ui.object->snapshot();
      for(std::size_t i=0;i<state.labelCount;++i) {
        const auto& label=state.labels[i];const auto at=ui.scene.project(label.position);
        if(at.z>=0)overlay->AddText({at.x,at.y},IM_COL32(89,217,255,255),label.text.data(),label.text.data()+label.text.size());
      }
      overlay->AddText({p.x+4,p.y+space.y-18},IM_COL32(190,202,215,255),"Drag: orbit   Wheel: zoom");overlay->PopClipRect();
    }
    ImGui::EndChild();
  }
  return chosen;
}
}

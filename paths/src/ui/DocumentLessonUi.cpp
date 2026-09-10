#include "DocumentLessonUi.hpp"
#include <algorithm>
#include <imgui.h>

namespace paths {
namespace {
constexpr auto flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings;
void panel(const char* name,SceneViewport area,ImGuiWindowFlags extra=0){
  ImGui::SetNextWindowPos({area.x,area.y});ImGui::SetNextWindowSize({area.width,area.height});ImGui::Begin(name,nullptr,flags|extra);
}
NotationBounds item(bool enabled=true){const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();return {a.x,a.y,b.x-a.x,b.y-a.y,enabled && ImGui::IsItemVisible()};}
}
void drawDocumentFigure(const CorpusEntry& e,DocumentLessonUiState& ui,SceneViewport area,float textScale,bool blocked) {
  ui.presented=false;ui.viewport={};ui.parameterControls.clear();
  if(!e.figure)return;
  if(ui.entry!=e.id){ui.entry=e.id;ui.object=std::make_unique<MathObjects>(instantiateDocumentFigure(*e.figure));ui.scene=MathObjectScene{};}
  const auto regions=planLessonFigureRegions(area);
  panel("Textbook figure title",regions.title);
  ImGui::PushFont(nullptr,ImGui::GetFontSize()*textScale);
  ImGui::TextColored({.45f,.82f,.78f,1},"%s",e.title.c_str());
  ImGui::PopFont();ImGui::End();
  panel("Textbook figure controls",regions.controls);
  ImGui::PushFont(nullptr,ImGui::GetFontSize()*textScale);ImGui::BeginDisabled(blocked);
  ImGui::TextWrapped("%s",e.figure->caption.c_str());
  for(const auto& [key,initial]:e.figure->parameters) {
    const auto specs=mathParameterSpecs();const auto p=std::find_if(specs.begin(),specs.end(),[&](const auto& p){return p.key==key && p.owner==ui.object->snapshot().kind;});
    if(p==specs.end())continue;
    auto value=ui.object->parameter(p->id);ImGui::SetNextItemWidth(-1);
    if(ImGui::SliderScalar(std::string(p->label).c_str(),ImGuiDataType_Double,&value,&p->minimum,&p->maximum,"%.3f"))
      (void)ui.object->dispatch({MathActionKind::SetParameter,p->owner,p->id,value});
    ui.parameterControls.emplace_back(key,item(!blocked));
  }
  if(ImGui::Button("Reset diagram")){*ui.object=instantiateDocumentFigure(*e.figure);ui.scene=MathObjectScene{};}
  ImGui::EndDisabled();ImGui::PopFont();ImGui::End();
  panel("Textbook figure viewport",regions.viewport,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
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
  ImGui::End();
}
}

#include "MathControlUi.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace paths {
namespace {
const char* format(const MathParameterSpec& p){return p.step>=1?"%.0f":p.step<.01?"%.3f":"%.2f";}
void hint(std::string_view text){if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))ImGui::SetTooltip("%s",text.data());}
bool number(const MathParameterSpec& p,double& value,MathControlRange range,const char* id){return ImGui::DragScalar(id,ImGuiDataType_Double,&value,static_cast<float>(p.step),&range.minimum,&range.maximum,format(p),ImGuiSliderFlags_AlwaysClamp);}
void labelCell(std::string_view label,std::string_view help){ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::AlignTextToFramePadding();ImGui::TextWrapped("%.*s",static_cast<int>(label.size()),label.data());hint(help);ImGui::TableNextColumn();}
bool table(){const float available=ImGui::GetContentRegionAvail().x;const bool open=ImGui::BeginTable("compact row",2,ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_NoSavedSettings);if(open){ImGui::TableSetupColumn("Label",ImGuiTableColumnFlags_WidthFixed,std::min(115.f*ImGui::GetFontSize()/16.f,available*.32f));ImGui::TableSetupColumn("Control",ImGuiTableColumnFlags_WidthStretch);}return open;}
}
MathControlEdit compactMathControl(const MathParameterSpec& p,std::string_view label,double value,MathControlRange range,bool allowReset){
  MathControlEdit edit;edit.value=value;ImGui::PushID(static_cast<int>(p.id));
  if(table()){
    labelCell(label,p.label);const float scale=ImGui::GetFontSize()/16.f,gap=ImGui::GetStyle().ItemSpacing.x,available=ImGui::GetContentRegionAvail().x,resetWidth=allowReset?22*scale+gap:0,control=std::max(24.f,available-resetWidth);
    if(!p.choices.empty()){
      const char* selected=p.choices.data();for(int i=0;i<static_cast<int>(value)&&*selected;++i)selected+=std::char_traits<char>::length(selected)+1;
      ImGui::SetNextItemWidth(control);
      if(ImGui::BeginCombo("##choice",selected)){
        int choice=0;for(const char* name=p.choices.data();*name;name+=std::char_traits<char>::length(name)+1,++choice){
          if(choice<range.minimum||choice>range.maximum)continue;
          if(ImGui::Selectable(name,choice==static_cast<int>(value))){edit.changed=true;edit.value=choice;}
          if(choice==static_cast<int>(value))ImGui::SetItemDefaultFocus();
        }ImGui::EndCombo();
      }
    }
    else if(p.minimum==0&&p.maximum==1&&p.step==1){bool checked=value!=0;edit.changed=ImGui::Checkbox("##enabled",&checked);edit.value=checked?1:0;}
    else{const float numeric=64*scale;const bool slider=control>=numeric+70*scale;
      if(slider){ImGui::SetNextItemWidth(control-numeric-gap);edit.changed=ImGui::SliderScalar("##slider",ImGuiDataType_Double,&edit.value,&range.minimum,&range.maximum,"",ImGuiSliderFlags_AlwaysClamp);hint(p.label);ImGui::SameLine();}
      ImGui::SetNextItemWidth(slider?numeric:control);edit.changed=number(p,edit.value,range,"##number")||edit.changed;if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))ImGui::SetTooltip("%s\nDrag to change; Ctrl-click to type. Range %.4g to %.4g.",p.label.data(),range.minimum,range.maximum);
    }
    if(allowReset){ImGui::SameLine();ImGui::BeginDisabled(std::fabs(value-p.initial)<1e-9||p.initial<range.minimum-1e-9||p.initial>range.maximum+1e-9);edit.reset=ImGui::SmallButton("R");ImGui::EndDisabled();if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled|ImGuiHoveredFlags_DelayShort))ImGui::SetTooltip("Reset to default: %.5g. Ordered profile heights must still fit their neighbours.",p.initial);}
    ImGui::EndTable();
  }ImGui::PopID();return edit;
}
MathControlEdit compactMathTuple(const MathControlRow& row,const std::array<double,3>& values,const std::array<MathControlRange,3>& ranges){
  MathControlEdit edit;ImGui::PushID(static_cast<int>(row.parameters[0]));
  if(table()){
    labelCell(row.label,row.label);const float gap=ImGui::GetStyle().ItemSpacing.x,available=ImGui::GetContentRegionAvail().x,scale=ImGui::GetFontSize()/16.f;
    const float width=std::max(24.f,(available-22*scale-gap*row.count)/row.count);bool changed=false;
    for(unsigned i=0;i<row.count;++i){if(i)ImGui::SameLine();ImGui::PushID(static_cast<int>(i));const auto& p=mathParameterSpecs()[static_cast<unsigned>(row.parameters[i])];double value=values[i];ImGui::SetNextItemWidth(width);
      const std::string componentFormat=std::string(row.components[i])+(p.step>=1?" %.0f":" %.3g");
      if(ImGui::DragScalar("##component",ImGuiDataType_Double,&value,static_cast<float>(p.step),&ranges[i].minimum,&ranges[i].maximum,componentFormat.c_str(),ImGuiSliderFlags_AlwaysClamp)){edit.changed=true;edit.component=i;edit.value=value;}if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))ImGui::SetTooltip("%s\nDrag to change; Ctrl-click to type.",p.label.data());changed=changed||std::fabs(values[i]-p.initial)>1e-9;ImGui::PopID();
    }
    ImGui::SameLine();ImGui::BeginDisabled(!changed);edit.reset=ImGui::SmallButton("R");ImGui::EndDisabled();hint("Reset this row to defaults");ImGui::EndTable();
  }ImGui::PopID();return edit;
}
} // namespace paths

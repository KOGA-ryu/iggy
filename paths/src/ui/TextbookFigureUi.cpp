#include "MathControlUi.hpp"
#include "TextbookFigureUi.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace paths {
namespace {
constexpr auto panelFlags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings;
void panel(const char* name,SceneViewport p,ImGuiWindowFlags extra=0){
  ImGui::SetNextWindowPos({p.x,p.y});ImGui::SetNextWindowSize({p.width,p.height});ImGui::Begin(name,nullptr,panelFlags|extra);
}
ImVec4 colour(iggy3d::Vec3 c){return {c.x,c.y,c.z,1};}
void setting(TextbookFigureUiState& ui,RowPlaneParameter p,double value){if(!ui.hasPending){ui.pending={RowPlaneActionKind::Set,p,value};ui.hasPending=true;}}
void systemAction(TextbookFigureUiState& ui,SystemAction a){if(!ui.hasSystemPending){ui.systemPending=a;ui.hasSystemPending=true;}}
void objectAction(TextbookFigureUiState& ui,unsigned section,ObjectLessonAction a){if(!ui.hasObjectPending){ui.objectPending=a;ui.objectSection=section;ui.hasObjectPending=true;}}
void action(MatrixBoardUiState& ui,BoardAction a){if(!ui.hasPending){ui.pending=a;ui.hasPending=true;}}
void beside(const char* label){
  if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+ImGui::CalcTextSize(label).x+2*ImGui::GetStyle().FramePadding.x<ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x)ImGui::SameLine();
}
std::string scalar(double x){
  char value[64];std::snprintf(value,sizeof(value),"%.5g",x);std::string s=value;const auto e=s.find('e');
  if(e!=s.npos)s=s.substr(0,e)+"\\times 10^{"+std::to_string(std::stoi(s.substr(e+1)))+"}";
  return s;
}
std::string equation(const RowPlaneView& v,unsigned row){
  std::string text;
  for(unsigned j=0;j<3;++j){const double c=v.coefficients[row*3+j];if(c==0)continue;
    if(!text.empty())text+=c<0?"-":"+";else if(c<0)text+="-";
    if(std::abs(c)!=1)text+=scalar(std::abs(c));text+=std::array<const char*,3>{"x","y","z"}[j];
  }
  if(text.empty())text="0";return text+"="+scalar(v.rhs[row]);
}
void equations(TextbookFigureUiState& ui,NativeMath& math,const RowPlaneView& v){
 const float pixels=ImGui::GetFontSize()*1.1f;
  for(unsigned r=0;r<v.rows;++r){
    ImGui::PushID(static_cast<int>(r));const auto c=rowPlaneColour(r);char label[32];std::snprintf(label,sizeof(label),"Row %u",r+1);
    ImGui::PushStyleColor(ImGuiCol_Text,colour(c));
    if(ImGui::Selectable(label,ui.model.parameter(RowPlaneParameter::FocusRow)==r+1))setting(ui,RowPlaneParameter::FocusRow,r+1);
    ImGui::PopStyleColor();
    auto& entry=ui.equations[r];const auto source=equation(v,r);
    if(entry.latex!=source||entry.pixels!=pixels){entry.latex=source;entry.pixels=pixels;entry.layout=math.layout(source,pixels,true);}
    if(entry.layout.error.empty()){
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{5,5});
      const float overflow=entry.layout.width+10>ImGui::GetContentRegionAvail().x?ImGui::GetStyle().ScrollbarSize+2:0;
      ImGui::BeginChild("Equation",{0,entry.layout.height+10+overflow},ImGuiChildFlags_AlwaysUseWindowPadding,ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoSavedSettings);
      const auto at=ImGui::GetCursorScreenPos();math.draw(entry.layout,at.x,at.y,ImGui::GetColorU32(colour(c)));ImGui::Dummy({entry.layout.width,entry.layout.height});ImGui::EndChild();ImGui::PopStyleVar();
    }else ImGui::TextWrapped("%s",source.c_str());
    if(v.planes[r].zero)ImGui::TextWrapped("%s",v.planes[r].contradiction?"Contradiction: a zero left-hand side cannot equal this nonzero right-hand side. No plane represents this row.":"This zero row imposes no constraint; it is not drawn as a plane.");
    ImGui::PopID();
  }
  ImGui::TextDisabled("Displayed coefficients rounded to 5 significant digits.");
}
void rowChoice(const char* label,int& row,unsigned count){
  char preview[24];std::snprintf(preview,sizeof(preview),"Row %d",row+1);
  ImGui::SetNextItemWidth(125);
  if(ImGui::BeginCombo(label,preview)){for(unsigned r=0;r<count;++r){char value[24];std::snprintf(value,sizeof(value),"Row %u",r+1);if(ImGui::Selectable(value,static_cast<unsigned>(row)==r))row=static_cast<int>(r);}ImGui::EndCombo();}
}
void toggle(TextbookFigureUiState& ui,RowPlaneParameter p){
  const auto& spec=rowPlaneParameters()[static_cast<unsigned>(p)];bool value=ui.model.parameter(p)!=0;
  if(ImGui::Checkbox(spec.label,&value))setting(ui,p,value?1:0);
}
void rowPlaneControls(MatrixBoard& board,MatrixBoardUiState& boardUi,TextbookFigureUiState& ui,NativeMath& math,SystemLesson* systems){
  const auto rowAction=[&](BoardAction a){if(systems)systemAction(ui,{SystemActionKind::RowOperation,0,0,0,a});else action(boardUi,a);};
  const auto state=board.view();const auto& v=ui.model.view();
  ImGui::BeginDisabled(state.complete||state.blocked);if(ImGui::Button("Next pivot"))rowAction({BoardActionKind::Step});ImGui::EndDisabled();
  beside("Undo");ImGui::BeginDisabled(!state.steps);if(ImGui::Button("Undo"))rowAction({BoardActionKind::Undo});ImGui::EndDisabled();
  beside("Reset example");if(ImGui::Button("Reset example")){if(systems)systemAction(ui,{SystemActionKind::ResetExploration});else rowAction({BoardActionKind::Reset});}
  beside("Reset camera");if(ImGui::Button("Reset camera"))ui.resetCamera=true;
  if(v.available){
    static constexpr std::array<const char*,4> solutions{"one point","a line","a plane","all of 3D space"};
    if(v.consistent)ImGui::TextWrapped("Rank %u / nullity %u: the solution set is %s.",v.rank,v.dimension,solutions[v.dimension]);
    else ImGui::TextWrapped("No solution. Rank(A) = %u, rank([A | b]) = %u. Nullity %u describes Ax = 0, not a solution family for this inconsistent system.",v.rank,v.augmentedRank,v.dimension);
    if(v.displayScale>1.0001)ImGui::TextWrapped("Display scale: 1 scene unit = %.4g coordinate units.",v.displayScale);
    if(v.agrees)ImGui::TextWrapped("Original and current solution sets agree within the numerical tolerance.");
    else ImGui::TextWrapped("The numerical comparison no longer agrees; undo an operation or reset to inspect the example.");
    ImGui::TextWrapped("%s",state.status.c_str());
    if(v.consistent)for(unsigned i=0;i<v.dimension;++i){
      const auto p=static_cast<RowPlaneParameter>(static_cast<unsigned>(RowPlaneParameter::ProbeS)+i);const auto& spec=rowPlaneParameters()[static_cast<unsigned>(p)];float value=static_cast<float>(ui.model.parameter(p));
      ImGui::SetNextItemWidth(-1);if(ImGui::SliderFloat(spec.label,&value,-2,2,"%.2f"))setting(ui,p,value);
    }
    if(v.consistent)ImGui::TextWrapped("Probe (%.4g, %.4g, %.4g) / normalized equation residual %.2g",v.probe[0],v.probe[1],v.probe[2],v.probeResidual);
    if(ImGui::CollapsingHeader("Equations / select a row",ImGuiTreeNodeFlags_DefaultOpen)){
      if(systems)ImGui::Checkbox("Show original equations",&ui.originalEquations);
      auto shown=v;
      if(systems&&ui.originalEquations){shown.planes=v.originalPlanes;for(unsigned r=0;r<v.rows;++r){shown.rhs[r]=state.rhs.at(r,0).real();for(unsigned c=0;c<3;++c)shown.coefficients[r*3+c]=state.given.at(r,c).real();}}
      equations(ui,math,shown);
    }
    if(ImGui::CollapsingHeader("Choose a row operation")){
      ui.target=std::clamp(ui.target,0,static_cast<int>(v.rows)-1);ui.other=std::clamp(ui.other,0,static_cast<int>(v.rows)-1);
      rowChoice("Target",ui.target,v.rows);rowChoice("Other",ui.other,v.rows);
      float factor=static_cast<float>(ui.multiplier);ImGui::SetNextItemWidth(-1);
      if(ImGui::SliderFloat("Real multiplier",&factor,-3,3,"%.2f"))ui.multiplier=factor;
      ImGui::TextWrapped("Apply R%d <- R%d + (%.2f) R%d",ui.target+1,ui.target+1,ui.multiplier,ui.other+1);
      const auto r=static_cast<unsigned>(ui.target),s=static_cast<unsigned>(ui.other);
      ImGui::BeginDisabled(r==s);if(ImGui::Button("Add to target"))rowAction({BoardActionKind::AddRow,r,s,0,ui.multiplier});beside("Swap rows");if(ImGui::Button("Swap rows"))rowAction({BoardActionKind::SwapRows,r,s});ImGui::EndDisabled();
      ImGui::BeginDisabled(std::abs(ui.multiplier)<=1e-12);if(ImGui::Button("Scale target"))rowAction({BoardActionKind::ScaleRow,r,0,0,ui.multiplier});ImGui::EndDisabled();
    }
    if(ImGui::CollapsingHeader("Figure settings")){
      for(const auto p:{RowPlaneParameter::Original,RowPlaneParameter::Normals,RowPlaneParameter::Solution,RowPlaneParameter::Labels})toggle(ui,p);
      float extent=static_cast<float>(ui.model.parameter(RowPlaneParameter::Extent));if(ImGui::SliderFloat("Plane extent",&extent,1,3,"%.1f"))setting(ui,RowPlaneParameter::Extent,extent);
      int density=static_cast<int>(ui.model.parameter(RowPlaneParameter::Density));if(ImGui::SliderInt("Grid density",&density,2,4))setting(ui,RowPlaneParameter::Density,density);
      if(ImGui::Button("Highlight all rows"))setting(ui,RowPlaneParameter::FocusRow,0);
      if(ImGui::Button("Reset figure settings")){ui.pending={RowPlaneActionKind::Reset};ui.hasPending=true;}
      ImGui::TextWrapped("Plane grids show bounded portions of infinite planes. Rank uses a relative tolerance of 1e-10; the solution-set comparison uses 1e-8. This illustration does not grade a proof.");
      ImGui::TextWrapped("Solution projector difference: %.3g",v.agreement);
    }
  }else ImGui::TextWrapped("%s",v.reason.c_str());
  if(!systems&&(ImGui::CollapsingHeader("Choose a printed example")||!v.available)){
    ImGui::TextWrapped("Choosing a part restarts this card's working matrix.");
    for(unsigned example:{0u,3u}){
      const char* label=example==0?"Part (a) / two planes":"Part (d) / three planes";
      ImGui::BeginDisabled(state.example==example);if(ImGui::Button(label)){rowAction({BoardActionKind::Select,4,example});ui.resetCamera=true;}ImGui::EndDisabled();
    }
  }
  if(!boardUi.message.empty())ImGui::TextWrapped("%s",boardUi.message.c_str());
  if(!ui.message.empty())ImGui::TextWrapped("%s",ui.message.c_str());
}
void systemControls(SystemLesson& lesson,TextbookFigureUiState& ui){
  const auto state=lesson.view();
  ImGui::SetNextItemWidth(-1);
  if(ImGui::BeginCombo("Teaching example",systemExamples()[state.example].title)){
    for(unsigned i=0;i<systemExamples().size();++i)if(ImGui::Selectable(systemExamples()[i].title,state.example==i))systemAction(ui,{SystemActionKind::SelectExample,i});
    ImGui::EndCombo();
  }
  ImGui::TextWrapped("%s",systemExamples()[state.example].prompt);
  if(ImGui::CollapsingHeader("Move the planes / edit givens")){
    ImGui::TextWrapped("Right-hand sides move the planes. Editing a given begins a new reduction; Undo below reverses row operations.");
    for(unsigned r=0;r<3;++r){
      ImGui::PushID(static_cast<int>(r));int value=static_cast<int>(std::lround(state.board.rhs.at(r,0).real()*4));
      const auto label="b"+std::to_string(r+1)+" / row "+std::to_string(r+1);
      ImGui::TextColored(colour(rowPlaneColour(r)),"%s = %.2f",label.c_str(),value/4.);
      ImGui::SetNextItemWidth(-1);
      if(ImGui::SliderInt("##rhs",&value,-12,12,""))systemAction(ui,{SystemActionKind::SetRightHandSide,r,0,value/4.});
      ImGui::PopID();
    }
    if(ImGui::TreeNode("Change plane orientations")){
      for(unsigned r=0;r<3;++r){ImGui::PushID(static_cast<int>(r));ImGui::TextColored(colour(rowPlaneColour(r)),"Row %u coefficients",r+1);
        for(unsigned c=0;c<3;++c){int value=static_cast<int>(std::lround(state.board.given.at(r,c).real()*4));
          const auto label=std::string(std::array<const char*,3>{"x","y","z"}[c])+" coefficient";
          ImGui::Text("%s = %.2f",label.c_str(),value/4.);ImGui::SetNextItemWidth(-1);
          if(ImGui::SliderInt(("##"+label).c_str(),&value,-12,12,""))systemAction(ui,{SystemActionKind::SetCoefficient,r,c,value/4.});
        }ImGui::PopID();
      }ImGui::TreePop();
    }
    ImGui::Separator();
  }
}
RowPlaneView equationView(const MatrixBoardView& state){
  RowPlaneView v;const auto& a=state.working?state.current:state.given;const auto& b=state.working?state.currentRhs:state.rhs;v.rows=a.rows;
  for(unsigned r=0;r<a.rows;++r){v.rhs[r]=b.at(r,0).real();bool zero=true;for(unsigned c=0;c<3;++c){v.coefficients[r*3+c]=a.at(r,c).real();zero=zero&&a.at(r,c)==MatrixScalar{};}v.planes[r].zero=zero;v.planes[r].contradiction=zero&&v.rhs[r]!=0;}
  return v;
}
void choice(const char* label,int& selected,const std::array<const char*,3>& values){
  ImGui::SetNextItemWidth(-1);
  if(ImGui::BeginCombo(label,selected<0?"Choose...":values[static_cast<unsigned>(selected)])){
    for(unsigned i=0;i<values.size();++i)if(ImGui::Selectable(values[i],selected==static_cast<int>(i)))selected=static_cast<int>(i);ImGui::EndCombo();
  }
}
void viewport(TextbookFigureUiState& ui,const MathObjectSnapshot& geometry,SceneViewport p){
  static_cast<void>(ui.scene.publish(geometry,p));
  if(ui.resetCamera){ui.scene.resetView();ui.resetCamera=false;}
  panel("Textbook 3D viewport",p,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
  ImGui::SetCursorPos({0,0});ImGui::InvisibleButton("Explore figure",{p.width,p.height},ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonRight);
  using Navigation=iggy3d::ProductCreativeViewportNavigationOperation;const auto& io=ImGui::GetIO();
  if(ImGui::IsItemActive()){
    if(ImGui::IsMouseDragging(ImGuiMouseButton_Left))static_cast<void>(ui.scene.navigate(Navigation::Orbit,io.MouseDelta.x,io.MouseDelta.y));
    if(ImGui::IsMouseDragging(ImGuiMouseButton_Right))static_cast<void>(ui.scene.navigate(Navigation::Pan,io.MouseDelta.x,io.MouseDelta.y));
  }
  if(ImGui::IsItemHovered()&&io.MouseWheel!=0)static_cast<void>(ui.scene.navigate(Navigation::Dolly,0,io.MouseWheel));
  ImGui::End();static_cast<void>(ui.scene.publish(geometry,p));
  auto* overlay=ImGui::GetForegroundDrawList();overlay->PushClipRect({p.x,p.y},{p.x+p.width,p.y+p.height},true);
  const auto& g=geometry;
  for(unsigned i=0;i<g.labelCount;++i){const auto& label=g.labels[i];const auto at=ui.scene.project(label.position);if(at.z<0)continue;
    const auto size=ImGui::CalcTextSize(label.text.data());const ImVec2 text{at.x-size.x*.5f,at.y-size.y*.5f};
    overlay->AddRectFilled({text.x-4,text.y-2},{text.x+size.x+4,text.y+size.y+2},IM_COL32(10,18,27,220),3);
    overlay->AddText(text,ImGui::GetColorU32(colour(label.color)),label.text.data());
  }
  overlay->AddText({p.x+10,p.y+p.height-25},IM_COL32(190,202,215,255),"Drag: orbit  /  Right drag: pan  /  Wheel: zoom");overlay->PopClipRect();
}
void objectMatrices(const MathObjectSnapshot& state,const ObjectLessonSpec& spec,TextbookFigureUiState& ui,NativeMath& math){
  if(!ImGui::CollapsingHeader("Matrix values",ImGuiTreeNodeFlags_DefaultOpen))return;
  for(unsigned i=0;i<state.matrixCount;++i){
    const auto& matrix=state.matrices[i];
    if(!spec.matrices.empty()&&std::find(spec.matrices.begin(),spec.matrices.end(),matrix.name)==spec.matrices.end())continue;
    std::string source="\\text{"+std::string(matrix.name)+"}=\\begin{bmatrix}";
    for(unsigned r=0;r<matrix.rows;++r){if(r)source+="\\\\";for(unsigned c=0;c<matrix.columns;++c){if(c)source+='&';source+=scalar(matrix.values[r*matrix.columns+c]);}}
    source+="\\end{bmatrix}";
    auto& entry=ui.equations[i];const auto pixels=ImGui::GetFontSize()*1.1f;
    if(entry.latex!=source||entry.pixels!=pixels){entry.latex=source;entry.pixels=pixels;entry.layout=math.layout(source,pixels,true);}
    ImGui::PushID(static_cast<int>(i));
    if(entry.layout.error.empty()){
      const float overflow=entry.layout.width+10>ImGui::GetContentRegionAvail().x?ImGui::GetStyle().ScrollbarSize+2:0;
      ImGui::BeginChild("Object matrix",{0,entry.layout.height+10+overflow},0,ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground);
      const auto at=ImGui::GetCursorScreenPos();math.draw(entry.layout,at.x,at.y,ImGui::GetColorU32(ImGuiCol_Text));ImGui::Dummy({entry.layout.width,entry.layout.height});ImGui::EndChild();
    }else ImGui::TextWrapped("%s",source.c_str());
    ImGui::PopID();
  }
}
void objectControls(Textbook& book,TextbookFigureUiState& ui,NativeMath& math){
  auto& model=book.objectLesson();const auto& spec=model.spec();const auto view=book.view();const bool practice=view.mode==BookMode::Exercise;
  const auto send=[&](ObjectLessonAction a){a.practice=practice;objectAction(ui,view.section,a);};
  if(practice){
    ImGui::TextWrapped("%s",spec.challenge);
    if(ImGui::Button("Check"))send({ObjectLessonActionKind::Check});beside("Reset practice");
    if(ImGui::Button("Reset practice"))send({ObjectLessonActionKind::Reset});
    const auto& state=model.snapshot(true);
    if(state.feedback!=MathFeedback::None)ImGui::TextWrapped("%s",state.feedbackText.data());
    ImGui::TextWrapped("This practice example is separate from the teaching examples. Feedback is created only by Check and lasts for this session.");
  }else {
    ImGui::SetNextItemWidth(-1);
    if(ImGui::BeginCombo("Teaching example",model.customized()?"Custom parameters":spec.examples[model.example()].title)){
      for(unsigned i=0;i<spec.examples.size();++i)if(ImGui::Selectable(spec.examples[i].title,!model.customized()&&model.example()==i))send({ObjectLessonActionKind::SelectExample,i});
      ImGui::EndCombo();
    }
    if(!model.customized())ImGui::TextWrapped("%s",spec.examples[model.example()].explanation);
    if(ImGui::Button("Reset example"))send({ObjectLessonActionKind::Reset});
  }
  beside("Reset camera");if(ImGui::Button("Reset camera"))ui.resetCamera=true;
  ImGui::Separator();
  const auto parameters=mathParameterSpecs();
  for(const auto& control:spec.controls){
    const auto& p=parameters[static_cast<unsigned>(control.parameter)];
    const auto edit=compactMathControl(p,control.label,model.parameter(control.parameter,practice),{p.minimum,p.maximum});
    if(edit.changed)send({ObjectLessonActionKind::SetParameter,0,control.parameter,edit.value});
  }
  ImGui::Separator();ImGui::TextWrapped("%s",spec.relationship);
  const auto& state=model.snapshot(practice);
  for(unsigned i=0;i<state.metricCount;++i){const auto& value=state.metrics[i];
    if(!spec.metrics.empty()&&std::find(spec.metrics.begin(),spec.metrics.end(),value.label)==spec.metrics.end())continue;
    ImGui::TextWrapped("%s: %.5g %s",value.label.data(),value.value,value.suffix.data()?value.suffix.data():"");
  }
  objectMatrices(state,spec,ui,math);
  ImGui::TextWrapped("%s",spec.convention);
  if(!ui.message.empty())ImGui::TextWrapped("%s",ui.message.c_str());
}
}
void applyObjectLessonPending(Textbook& book,TextbookFigureUiState& ui){
  if(!ui.hasObjectPending)return;
  if(book.view().section!=ui.objectSection||book.exerciseKind()!=BookExerciseKind::Object)ui.message="The object lesson changed before this action.";
  else {const auto result=book.objectLesson().dispatch(ui.objectPending);ui.message=result.accepted?"":result.reason;}
  ui.hasObjectPending=false;
}
void applySystemLessonPending(SystemLesson& model,TextbookFigureUiState& ui){
  if(!ui.hasSystemPending)return;
  const auto result=model.dispatch(ui.systemPending);ui.message=result.accepted?"":result.reason;
  if(result.accepted)switch(ui.systemPending.kind){
    case SystemActionKind::SelectChallenge:ui.prediction=ui.reason=-1;break;
    case SystemActionKind::SelectExample:case SystemActionKind::ResetExploration:ui.resetCamera=true;ui.originalEquations=false;break;
    default:break;
  }
  ui.hasSystemPending=false;
}
void drawSystemExercise(SystemLesson& model,TextbookFigureUiState& ui,MatrixBoardUiState& boardUi,NativeMath& math){
  const auto v=model.view(true);
  ImGui::TextWrapped("Practice / three new systems. Predict before opening the explanation.");
  for(unsigned n=0;n<3;++n){const auto label="System "+std::string(1,static_cast<char>('A'+n));
    if(n)beside(label.c_str());ImGui::BeginDisabled(v.challenge==n);if(ImGui::Button(label.c_str()))systemAction(ui,{SystemActionKind::SelectChallenge,n});ImGui::EndDisabled();
  }
  beside("Restart this system");if(ImGui::Button("Restart this system"))systemAction(ui,{SystemActionKind::SelectChallenge,v.challenge});
  ImGui::Separator();
  ImGui::TextUnformatted(v.board.working?"Current equations":"Given equations");
  equations(ui,math,equationView(v.board));
  ImGui::Separator();
  choice("How many solutions?",ui.prediction,{"No solution","One solution","Infinitely many solutions"});
  choice("Why?",ui.reason,{"Reduction produces a contradiction, 0 = c with c nonzero","Consistent, with a pivot for every variable","Consistent, with at least one free variable"});
  ImGui::BeginDisabled(ui.prediction<0||ui.reason<0);
  if(ImGui::Button("Check prediction"))systemAction(ui,{SystemActionKind::Predict,static_cast<unsigned>(ui.prediction),static_cast<unsigned>(ui.reason)});
  ImGui::EndDisabled();
  if(!v.revealed){beside("Reveal explanation");if(ImGui::Button("Reveal explanation"))systemAction(ui,{SystemActionKind::Reveal});}
  if(v.hasPrediction)ImGui::TextWrapped("%s",v.predictionCorrect?"Correct prediction and reason.":"The prediction or its reason needs revision. Compare the explanation with the equations.");
  if(v.solution){const auto& result=*v.solution;
    ImGui::Separator();ImGui::TextColored({1,.78f,.27f,1},"%s",systemOutcomeName(systemOutcome(result)));
    ImGui::TextWrapped("Rank(A) = %u; rank([A | b]) = %u; nullity(A) = %u.",result.rank,result.augmentedRank,result.dimension);
    if(!result.consistent)ImGui::TextWrapped("An extra augmented pivot records a contradiction. Step through the reduction to find a row with zero coefficients and a nonzero right-hand side. There is no point satisfying all three equations.");
    else if(result.dimension)ImGui::TextWrapped("The system is consistent and has %u free variable(s). A particular solution plus any null-space vector gives another solution.",result.dimension);
    else ImGui::TextWrapped("Every variable has a coefficient pivot, and the system is consistent. Exactly one point satisfies all three equations.");
    const auto rowAction=[&](BoardAction action){systemAction(ui,{SystemActionKind::RowOperation,0,0,0,action,true});};
    ImGui::BeginDisabled(v.board.complete||v.board.blocked);if(ImGui::Button("Next pivot"))rowAction({BoardActionKind::Step});ImGui::EndDisabled();
    beside("Undo");ImGui::BeginDisabled(!v.board.steps);if(ImGui::Button("Undo"))rowAction({BoardActionKind::Undo});ImGui::EndDisabled();
    beside("Reset reduction");if(ImGui::Button("Reset reduction"))rowAction({BoardActionKind::Reset});
    ImGui::TextWrapped("%s",v.board.status.c_str());
    if(ImGui::CollapsingHeader("Augmented matrix / coefficients and right-hand side")){
      drawMatrixBoardGrid("A",v.board.working?v.board.current:v.board.given,v.board,boardUi);
      drawMatrixBoardGrid("b",v.board.working?v.board.currentRhs:v.board.rhs,v.board,boardUi);
    }
    if(result.consistent){
      ImGui::Separator();ImGui::TextUnformatted("Find a solution point");
      ImGui::TextWrapped("Enter coordinates, then test them against every original equation.");
      for(unsigned c=0;c<3;++c){float x=static_cast<float>(v.point[c]);ImGui::SetNextItemWidth(-1);
        if(ImGui::SliderFloat(std::array<const char*,3>{"x","y","z"}[c],&x,-3,3,"%.2f"))systemAction(ui,{SystemActionKind::SetPoint,c,0,std::round(x*4)/4});
      }
      if(ImGui::Button("Check point"))systemAction(ui,{SystemActionKind::CheckPoint});
      if(v.pointChecked)ImGui::TextWrapped("%s Normalized residual: %.3g.",v.pointCorrect?"This point satisfies every equation.":"This point fails at least one equation.",v.pointResidual);
    }
  }
  if(!v.attempts.empty()&&ImGui::CollapsingHeader("Your predictions"))for(unsigned i=0;i<v.attempts.size();++i){const auto& a=v.attempts[i];
    ImGui::TextWrapped("%u. System %c / %s / %s%s",i+1,'A'+a.challenge,systemOutcomeName(a.prediction),a.correct?"correct":"revise",a.assisted?" / after explanation":"");
  }
  if(!ui.message.empty())ImGui::TextWrapped("%s",ui.message.c_str());
}
bool drawTextbookFigure(const BookFigureSpec& spec,Textbook& book,MatrixBoardUiState* boardUi,
                       TextbookFigureUiState& ui,NativeMath& math,SceneViewport area,float textScale){
  const bool practice=book.view().mode==BookMode::Exercise;
  if(ui.sceneId!=spec.id||ui.scenePractice!=practice){
    // Different owners can have the same local revision. Never reuse their mesh.
    ui.scene=MathObjectScene{};ui.sceneId=spec.id;ui.scenePractice=practice;
  }
  if(spec.kind==BookFigureKind::Object){
    const auto regions=planLessonFigureRegions(area);const auto& model=book.objectLesson();
    panel("Textbook figure title",regions.title);ImGui::TextColored({.45f,.82f,.78f,1},"%s",spec.title);
    ImGui::TextWrapped("%s",practice?"Exercise / your independent example":(model.customized()?"Teaching figure / custom parameters":model.spec().examples[model.example()].title));ImGui::End();
    viewport(ui,model.snapshot(practice),regions.viewport);
    panel("Textbook figure controls",regions.controls);ImGui::PushFont(nullptr,ImGui::GetFontSize()*textScale);
    if(!practice){ImGui::TextWrapped("%s",spec.caption);ImGui::Separator();}
    objectControls(book,ui,math);ImGui::PopFont();ImGui::End();return true;
  }
  SystemLesson* systems=nullptr;
  switch(spec.kind){case BookFigureKind::None:case BookFigureKind::Object:return false;case BookFigureKind::RowPlanes:break;case BookFigureKind::AffinePlanes:systems=&book.systems();break;}
  if(!boardUi)return false;
  auto& board=book.board();
  if(ui.hasPending){const auto result=ui.model.dispatch(ui.pending);ui.message=result.accepted?"":result.reason;ui.hasPending=false;}
  const auto& view=ui.model.publish(board.view());const auto regions=planLessonFigureRegions(area);
  panel("Textbook figure title",regions.title);
  ImGui::PushStyleColor(ImGuiCol_Text,{.45f,.82f,.78f,1});ImGui::TextWrapped("%s",spec.title);ImGui::PopStyleColor();
  if(systems)ImGui::TextWrapped("Authored system / %s / %u operations",systems->view().customized?"edited givens":systemExamples()[systems->view().example].title,view.steps);
  else ImGui::TextWrapped("Card 004 / part (%c) / %s / %u operations",'a'+view.example,view.working?"current matrix":"original matrix",view.steps);
  ImGui::End();
  if(view.available)viewport(ui,ui.model.geometry(),regions.viewport);
  else {panel("Textbook 3D viewport",regions.viewport);ImGui::TextWrapped("%s",view.reason.c_str());ImGui::End();}
  panel("Textbook figure controls",regions.controls);
  ImGui::PushFont(nullptr,ImGui::GetFontSize()*textScale);
  ImGui::TextWrapped("%s",spec.caption);ImGui::Separator();
  if(systems)systemControls(*systems,ui);
  rowPlaneControls(board,*boardUi,ui,math,systems);
  ImGui::PopFont();ImGui::End();return view.available;
}
} // namespace paths

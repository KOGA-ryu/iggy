#include "ui/MathCorpusUi.hpp"
#include "runtime/textbook/Textbook.hpp"
#include <algorithm>
#include <cstring>
#include <imgui.h>

namespace paths {
namespace {
namespace fm=iggy3d::first_move;
constexpr auto gold=IM_COL32(255,199,77,255),cyan=IM_COL32(89,217,255,255),green=IM_COL32(102,230,166,255);
// Reflow only authored separators between givens. Mathematical expressions
// themselves retain their layout and can scroll when they exceed the viewport.
NativeMath::Equation equation(NativeMath& math,std::string_view source,float width,float pixels=16) {
  auto result=math.layout(source,pixels);
  if(result.width<=width || source.find("\\begin{")!=std::string_view::npos)return result;
  std::string wrapped(source);std::size_t at=0;bool changed=false;
  while((at=wrapped.find(",\\quad",at))!=std::string::npos) {
    wrapped.replace(at,6,"\\\\");at+=2;changed=true;
  }
  return changed?math.layout("\\begin{gathered}"+wrapped+"\\end{gathered}",pixels):result;
}
void ink(NativeMath& math,const NativeMath::Equation& e,std::string_view source,ImU32 colour,std::string_view question={}) {
  if(!e.error.empty())ImGui::TextColored({1,.65F,.3F,1},"%.*s",static_cast<int>(source.size()),source.data());
  else {const auto p=ImGui::GetCursorScreenPos();math.draw(e,p.x,p.y,colour);ImGui::Dummy({e.width,e.height});}
  if(question.empty())drawTextCopyMenu("equation-copy",{{"Copy equation (LaTeX)",source}});
  else drawTextCopyMenu("equation-copy",{{"Copy equation (LaTeX)",source},{"Copy question + working",question}});
}
std::optional<std::size_t> drawSupported(MathCorpusUiState& ui,bool blocked,std::optional<std::size_t> next) {
  auto& practice=*ui.practice;auto& math=*ui.math;const auto v=*practice.active()->supportView();
  ImGui::PushID(v.command.questionId.c_str());ImGui::PushID(static_cast<int>(v.command.runNumber));
  const auto send=[&](fm::SupportAction action,std::uint32_t value=0,std::string text={}) {
    if(blocked)return false;
    fm::LayeredQuestionCommand c{fm::LayeredQuestionCommandKind::Support};c.support=v.command;
    c.support.action=action;c.support.value=value;c.support.text=std::move(text);return practice.dispatch(c);
  };
  const float width=ImGui::GetContentRegionAvail().x;
  const auto copied=questionCopyText(practice.questions()[*practice.selected()],*practice.active());
  ImGui::PushTextWrapPos(0);ImGui::TextDisabled("%s",practice.questions()[*practice.selected()].title.c_str());drawTextCopyMenu("question-copy",{{"Copy question + working",copied}});ImGui::PopTextWrapPos();
  constexpr std::array labels{"1 Learn","2 Practice","3 Solve","4 Write"};
  for(std::size_t i=0;i<labels.size();++i) {
    if(i)ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,i==static_cast<std::size_t>(v.level)?ImVec4{.12F,.4F,.53F,1}:ImVec4{.12F,.16F,.23F,1});
    if(ImGui::Button(labels[i],{std::min(90.0F,(width-18)/4),23}))send(fm::SupportAction::SelectLevel,i);
    ImGui::PopStyleColor();
  }
  ImGui::PushTextWrapPos(0);ImGui::TextColored({1,.78F,.3F,1},"%s %s",v.goal.c_str(),v.domain.c_str());ImGui::PopTextWrapPos();
  const auto given=equation(math,v.given,(width-8)*.5F,17),working=equation(math,v.working,(width-8)*.5F,17);
  const float height=std::clamp(std::max(given.height,working.height)+26,58.0F,86.0F);
  ImGui::BeginChild("Given",{(width-8)*.5F,height},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::TextDisabled("Given");ink(math,given,v.given,gold,copied);ImGui::EndChild();ImGui::SameLine();
  ImGui::BeginChild("Checked working",{0,height},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::TextDisabled("Working");if(!v.working.empty())ink(math,working,v.working,v.completed?green:cyan,copied);
  else ImGui::Dummy({1,20});ImGui::EndChild();
  bool advance=false,restart=false;
  ImGui::BeginDisabled(!v.completed || !next);advance=ImGui::Button("Next",{48,22});ImGui::EndDisabled();ImGui::SameLine();
  restart=ImGui::Button("Again",{48,22});
  if(ImGui::IsItemHovered())ImGui::SetTooltip("Start a fresh attempt. This working, draft and help use remain in the earlier run.");
  ImGui::SameLine();ImGui::BeginDisabled(!v.canUndo);
  if(ImGui::Button("Undo",{48,22}))send(fm::SupportAction::Undo);ImGui::EndDisabled();ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Button,{.30F,.20F,.46F,1});
  if(ImGui::Button(v.help==fm::SupportHelp::None?"Help":"Close help",{76,22})) {
    send(fm::SupportAction::ReadHelp,v.help==fm::SupportHelp::None?1:0);
  }
  ImGui::PopStyleColor();
  if(!v.prompt.empty()){ImGui::PushTextWrapPos(0);ImGui::TextUnformatted(v.prompt.c_str());drawTextCopyMenu("prompt-copy",{{"Copy paragraph",v.prompt},{"Copy question + working",copied}});ImGui::PopTextWrapPos();}
  if(!v.responseCue.empty())ink(math,equation(math,v.responseCue,width,17),v.responseCue,cyan);
  // A reload retires active widget buffers; the canonical draft supplies the
  // next widgets, including when the same question ID now has new mathematics.
  ImGui::PushID(static_cast<int>(ui.previewRevision));
  if(!v.completed) {
    ImGui::BeginDisabled(!v.canRespond);
    if(!v.choices.empty() && v.canRespond) {
      float used=0;
      for(const auto& option:v.choices) {
        const auto e=equation(math,option.label,width,17);const auto w=std::max(64.0F,e.width+22),h=std::max(34.0F,e.height+12);
        if(used && used+w<=width)ImGui::SameLine();else used=0;
        ImGui::PushID(static_cast<int>(option.id.value));const auto at=ImGui::GetCursorScreenPos();
        if(ImGui::Button("##symbol",{w,h}))send(fm::SupportAction::Choose,option.id.value);
        math.draw(e,at.x+(w-e.width)/2,at.y+(h-e.height)/2,cyan);
        drawTextCopyMenu("choice-copy",{{"Copy equation (LaTeX)",option.label}});
        ImGui::PopID();used+=w+6;
      }
    }
    bool typing=false;
    if(v.level==fm::SupportLevel::Practice) {
      typing=ImGui::CollapsingHeader("Type an answer (optional)",v.draft.empty()?ImGuiTreeNodeFlags_None:ImGuiTreeNodeFlags_DefaultOpen);

    }
    if(typing || v.level>=fm::SupportLevel::Solve) {
      std::memcpy(ui.supportDraft.data(),v.draft.data(),v.draft.size());ui.supportDraft[v.draft.size()]='\0';
      bool submit=false,edited=false;
      if(v.level==fm::SupportLevel::Practice) {
        ImGui::SetNextItemWidth(std::max(80.0F,width-64));
        submit=ImGui::InputTextWithHint("##blank","value, e.g. -2/3",ui.supportDraft.data(),ui.supportDraft.size(),ImGuiInputTextFlags_EnterReturnsTrue);
        edited=ImGui::IsItemEdited();ImGui::SameLine();
        submit=ImGui::Button("Check",{56,22}) || submit;
      } else {
        const auto remaining=ImGui::GetContentRegionAvail().y;
        const float editorHeight=std::max(64.0F,remaining*(v.reading.empty()?.72F:.45F)-30);
        edited=ImGui::InputTextMultiline("##written",ui.supportDraft.data(),ui.supportDraft.size(),{width,editorHeight},ImGuiInputTextFlags_AllowTabInput);

        submit=ImGui::Button("Check work",{86,23});
        if(width>=420)ImGui::SameLine();ImGui::TextDisabled("%s",v.inputLabel.c_str());
        if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",v.inputHelp.c_str());
      }
      // ImGui's Escape reverts to the activation text; our draft has already
      // saved each edit. Leaving the editor must retain that canonical draft.
      if(edited && !ImGui::IsKeyPressed(ImGuiKey_Escape,false))send(fm::SupportAction::EditDraft,0,ui.supportDraft.data());
      if(submit)send(v.level==fm::SupportLevel::Practice?fm::SupportAction::SubmitBlank:fm::SupportAction::CheckWork,0,ui.supportDraft.data());
    }
    ImGui::EndDisabled();
  }
  ImGui::PopID();
  ImGui::BeginChild("Reading and submissions",{0,0},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
  if(v.completed) {
    ImGui::TextColored({.4F,.9F,.65F,1},"Complete");ImGui::TextWrapped("%s",v.verification.c_str());
  }
  if(!v.feedback.empty()) {
    const bool wrong=v.status==fm::WrittenCheckStatus::Incorrect;
    if(wrong)ImGui::PushStyleColor(ImGuiCol_Text,{1,.6F,.4F,1});
    ImGui::TextWrapped("%s",v.feedback.c_str());drawTextCopyMenu("feedback-copy",{{"Copy feedback",v.feedback}});if(wrong)ImGui::PopStyleColor();
  }
  if(v.assisted || v.seenBefore)ImGui::TextDisabled("%s%s",v.assisted?"Guidance used":"",v.seenBefore?"  Seen before":"");
  if(v.help!=fm::SupportHelp::None) {
    constexpr std::array helpLabels{"Terms","Hint","Next line","Solution"};
    for(std::size_t i=0;i<helpLabels.size();++i) {
      if(i)ImGui::SameLine();
      if(ImGui::Button(helpLabels[i],{std::min(92.0F,(width-18)/4),22})) {
        send(fm::SupportAction::ReadHelp,i+1);
      }

    }
  }
  if(!v.reading.empty()) {
    const auto& document=math.layoutDocument(v.reading,std::max(1.0F,ImGui::GetContentRegionAvail().x));
    const auto p=ImGui::GetCursorScreenPos();math.draw(document,p.x,p.y,IM_COL32(221,221,226,255),cyan,gold);
    ImGui::Dummy({document.width,document.height});drawDocumentCopyMenu(document,p.x,p.y);
  }
  if(!v.history.empty() && ImGui::TreeNode("Checked steps")) {
    for(std::size_t i=0;i<v.history.size();++i){ImGui::PushID(static_cast<int>(i));ink(math,equation(math,v.history[i],width),v.history[i],cyan);ImGui::TextWrapped("%s",v.historyNotes[i].c_str());drawTextCopyMenu("note-copy",{{"Copy paragraph",v.historyNotes[i]}});ImGui::PopID();}ImGui::TreePop();
  }
  const auto& owner=*practice.active();
  if((!owner.currentRun().support->submissions.empty() || !owner.archivedRuns().empty()) && ImGui::TreeNode("Attempts and earlier runs")) {
    const auto show=[&](const fm::LayeredQuestionRunRecord& run) {
      if(!run.support)return;
      ImGui::TextDisabled("Run %u%s",run.runNumber,run.completed?" / complete":"");
      for(const auto& attempt:run.support->submissions) {
        ImGui::TextWrapped("%s",attempt.text.c_str());ImGui::TextWrapped("%s",attempt.feedback.c_str());
      }
      if(!run.support->draft.empty())ImGui::TextWrapped("Draft: %s",run.support->draft.c_str());
    };
    show(owner.currentRun());for(const auto& old:owner.archivedRuns())show(old);ImGui::TreePop();
  }
  if(!practice.message().empty())ImGui::TextWrapped("%s",practice.message().c_str());
  ImGui::EndChild();ImGui::PopID();ImGui::PopID();
  if(restart && !blocked){fm::LayeredQuestionCommand c{fm::LayeredQuestionCommandKind::RestartQuestion};c.archiveUnfinished=true;(void)practice.dispatch(c);}
  return advance && !blocked?next:std::nullopt;
}
}
std::string questionCopyText(const CorpusStarter& q,const fm::LayeredQuestionSession& session){
  std::string text=q.title+"\nQuestion: "+q.id+"\n\n"+q.question.description;
  const auto prose=[&](std::string_view heading,std::string_view value){if(!value.empty())text+="\n\n"+std::string(heading)+"\n"+std::string(value);};
  const auto math=[&](std::string_view heading,std::string_view value){if(!value.empty())prose(heading,"$$\n"+std::string(value)+"\n$$");};
  const auto choices=[&](const auto& options){
    if(options.empty())return;
    text+="\n\nChoices";
    for(std::size_t i=0;i<options.size();++i)math(std::to_string(i+1)+".",options[i].label);
  };
  if(const auto v=session.supportView()){
    prose("Goal",v->goal);prose("Domain",v->domain);math("Given",v->given);
    math(v->completed?"Working / complete":"Working",v->working);
    if(!v->completed){prose("Current step",v->prompt);math("Response",v->responseCue);choices(v->choices);}
    prose("Feedback",v->feedback);if(v->completed)prose("Verification",v->verification);
  }else {
    math("Given",q.question.equation);math(session.currentRun().completed?"Working / complete":"Working",session.visibleWorking());
    if(!session.currentRun().completed){
      const auto& step=q.question.steps.at(session.currentRun().currentStep);prose("Current step",step.prompt);choices(step.options);
      const auto review=session.review();const auto& attempts=review->steps.back().attempts;
      if(!attempts.empty() && !attempts.back().correct)prose("Feedback",attempts.back().feedback);
    }
  }
  return text+"\n";
}
bool recordQuestionReadingHelp(MathCorpusUiState& ui,BookHelp opened) {
  constexpr std::array help{fm::SupportHelp::Definitions,fm::SupportHelp::Hint,fm::SupportHelp::Solution,fm::SupportHelp::Solution};
  if(static_cast<unsigned>(opened)>=help.size() || !ui.practice || !ui.practice->active() || !ui.practice->active()->supportView())return false;
  fm::LayeredQuestionCommand command{fm::LayeredQuestionCommandKind::Support};command.support=ui.practice->active()->supportView()->command;
  command.support.action=fm::SupportAction::ReadReference;command.support.value=static_cast<unsigned>(help[static_cast<unsigned>(opened)]);
  return ui.practice->dispatch(command);
}
std::optional<std::size_t> drawCorpusQuestions(MathCorpusUiState& ui,const MathCorpus& corpus,bool blocked) {
  auto& practice=*ui.practice;auto& math=*ui.math;
  const auto& questions=practice.questions();
  if(ui.questionMatches.empty() || !practice.active())return {};
  const bool narrow=ImGui::GetContentRegionAvail().x<700;
  auto& session=*practice.active();const auto selected=*practice.selected();const auto& q=questions[selected];
  if(q.question.support) {
    const auto position=std::find(ui.questionMatches.begin(),ui.questionMatches.end(),selected);
    const auto next=position!=ui.questionMatches.end() && position+1!=ui.questionMatches.end()?std::optional<std::size_t>(*(position+1)):std::nullopt;
    return drawSupported(ui,blocked,next);
  }
  const auto& run=session.currentRun();const auto stepIndex=run.currentStep;const auto& step=q.question.steps[stepIndex];
  const bool complete=run.completed;
  const float width=ImGui::GetContentRegionAvail().x;
  const auto copied=questionCopyText(q,session);
  ImGui::TextDisabled("%s",q.title.c_str());
  if(ImGui::IsItemHovered())ImGui::SetTooltip("%s / %s · %s",corpus.subjects[q.subject].title.c_str(),q.title.c_str(),q.level.c_str());
  drawTextCopyMenu("question-copy",{{"Copy question + working",copied}});
  ImGui::PushTextWrapPos(0);ImGui::TextColored({1,.78F,.3F,1},"%s",q.question.description.c_str());drawTextCopyMenu("description-copy",{{"Copy paragraph",q.question.description},{"Copy question + working",copied}});ImGui::PopTextWrapPos();
  const auto problem=equation(math,q.question.equation,width,18);
  if(q.level=="practice") {
    const float column=(width-8)*.5F;float height=problem.height;
    for(const auto& state:q.question.workingStates)height=std::max(height,equation(math,state.display,column,16).height);
    height=std::clamp(height+30,64.0F,120.0F);
    ImGui::BeginChild("Given matrix",{column,height},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextDisabled("Given");ink(math,equation(math,q.question.equation,column,16),q.question.equation,gold,copied);ImGui::EndChild();ImGui::SameLine();
    ImGui::BeginChild("Current matrix",{0,height},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextDisabled("Working");const auto working=session.visibleWorking();ink(math,equation(math,working,column,16),working,complete?green:cyan,copied);ImGui::EndChild();
  } else {
    ImGui::BeginChild("Pinned problem",{0,std::clamp(problem.height+12,36.0F,narrow?82.0F:116.0F)},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
    ink(math,problem,q.question.equation,gold,copied);ImGui::EndChild();
  }
  const auto position=std::find(ui.questionMatches.begin(),ui.questionMatches.end(),selected);
  const bool hasNext=position!=ui.questionMatches.end() && position+1!=ui.questionMatches.end();
  ImGui::PushStyleColor(ImGuiCol_Button,{.15F,.27F,.46F,1});
  ImGui::BeginDisabled(!complete || !hasNext);
  bool next=ImGui::Button("Next",{50,22});ImGui::EndDisabled();ImGui::SameLine();
  ImGui::BeginDisabled(!complete);bool replay=ImGui::Button("Again",{50,22});ImGui::EndDisabled();
  ImGui::PopStyleColor();ImGui::SameLine();
  if(complete)ImGui::TextColored({.4F,.9F,.65F,1},"Complete");else ImGui::TextDisabled("%zu / %zu · %s",stepIndex+1,q.question.steps.size(),step.layerName.c_str());
  std::optional<fm::OptionId> choice;
  if(!complete) {
    ImGui::TextWrapped("%s",step.prompt.c_str());drawTextCopyMenu("prompt-copy",{{"Copy paragraph",step.prompt},{"Copy question + working",copied}});
    std::vector<NativeMath::Equation> tiles;float height=0,rowHeight=0,x=0;
    for(const auto& option:step.options) {
      auto e=equation(math,option.label,width-24,16);const float w=std::max(70.0F,e.width+20),h=std::max(32.0F,e.height+12);
      if(x>0 && x+w>width-16){height+=rowHeight+5;x=0;rowHeight=0;}
      x+=w+6;rowHeight=std::max(rowHeight,h);tiles.push_back(std::move(e));
    }
    height+=rowHeight+18;
    // Fixed problem and choices precede scrolling history. Wide notation has a
    // horizontal scrollbar instead of clipped text or unreadably small glyphs.
    ImGui::BeginChild("Answer tiles",{0,std::min(height,std::max(60.0F,ImGui::GetContentRegionAvail().y-28))},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
    x=0;
    for(std::size_t i=0;i<tiles.size();++i) {
      const auto& e=tiles[i];const float w=std::max(70.0F,e.width+20),h=std::max(32.0F,e.height+12);
      if(x>0 && x+w<=width-16)ImGui::SameLine();else x=0;
      ImGui::PushID(static_cast<int>(i));const auto p=ImGui::GetCursorScreenPos();
      if(ImGui::Button("##answer",{w,h}))choice=step.options[i].id;

      if(e.error.empty())math.draw(e,p.x+(w-e.width)/2,p.y+(h-e.height)/2,cyan);
      else ImGui::GetWindowDrawList()->AddText({p.x+4,p.y+4},gold,step.options[i].label.c_str());
      drawTextCopyMenu("choice-copy",{{"Copy equation (LaTeX)",step.options[i].label}});
      ImGui::PopID();x+=w+6;
    }
    ImGui::EndChild();
  }
  ImGui::BeginChild("Starter working",{0,0},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
  const auto review=session.review();
  if(!complete && !review->steps[stepIndex].attempts.empty() && !review->steps[stepIndex].attempts.back().correct) {
    const auto feedback=review->steps[stepIndex].attempts.back().feedback;
    ImGui::PushStyleColor(ImGuiCol_Text,{1,.6F,.4F,1});
    ImGui::TextWrapped("%.*s",static_cast<int>(feedback.size()),feedback.data());drawTextCopyMenu("feedback-copy",{{"Copy feedback",feedback}});ImGui::PopStyleColor();
    ImGui::TextDisabled("Working retained. Choose another tile.");
  }
  if((stepIndex || complete) && q.level!="practice") {
    const auto working=session.visibleWorking();ink(math,equation(math,working,width),working,complete?green:cyan,copied);
  }
  for(std::size_t i=0;i<review->steps.size();++i)if(!review->steps[i].explanation.empty()) {
    ImGui::PushID(static_cast<int>(i));
    ImGui::TextDisabled("%zu · %s",i+1,q.question.steps[i].layerName.c_str());
    if(complete && i+1<q.question.steps.size()) {
      const auto& state=q.question.workingStates[i+1].display;ink(math,equation(math,state,width),state,cyan);
    }
    ImGui::TextWrapped("%.*s",static_cast<int>(review->steps[i].explanation.size()),review->steps[i].explanation.data());
    drawTextCopyMenu("explanation-copy",{{"Copy paragraph",review->steps[i].explanation}});ImGui::PopID();
  }
  if(!practice.message().empty())ImGui::TextWrapped("%s",practice.message().c_str());
  ImGui::EndChild();
  if(choice && !blocked) {
    (void)practice.dispatch(fm::LayeredQuestionCommand::submitOption(*choice));
    if(fm::layeredQuestionStepResolved(session.currentRun().steps[stepIndex]))
      (void)practice.dispatch({fm::LayeredQuestionCommandKind::Continue});
  }
  if(replay && !blocked)(void)practice.dispatch({fm::LayeredQuestionCommandKind::RestartQuestion});
  return next && hasNext && !blocked?std::optional<std::size_t>(*(position+1)):std::nullopt;
}
}

#include "ui/MathCorpusUi.hpp"
#include "runtime/textbook/Textbook.hpp"
#include <algorithm>
#include <imgui.h>

namespace paths {
namespace {
namespace fm=iggy3d::first_move;
constexpr auto gold=IM_COL32(255,199,77,255),cyan=IM_COL32(89,217,255,255),green=IM_COL32(102,230,166,255);
NotationBounds item(bool enabled=true) {
  const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
  return {a.x,a.y,b.x-a.x,b.y-a.y,enabled && ImGui::IsItemVisible()};
}
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
void ink(NativeMath& math,const NativeMath::Equation& e,std::string_view source,ImU32 colour) {
  if(!e.error.empty()){ImGui::TextColored({1,.65F,.3F,1},"%.*s",static_cast<int>(source.size()),source.data());return;}
  const auto p=ImGui::GetCursorScreenPos();math.draw(e,p.x,p.y,colour);ImGui::Dummy({e.width,e.height});
}
// A reading projection only. Exercise answers, solutions, board state and
// automatic figure disclosures never cross into this question-help surface.
std::pair<std::string,std::string> reading(std::string_view id) {
  for(const auto& block:rrefLesson())if(id==block.id &&
      (block.kind==BookBlockKind::Definition || block.kind==BookBlockKind::Proposition)) {
    std::string source;
    for(const auto& p:block.body)source+=(p.kind==BookPassage::Kind::DisplayMath?"$$"+p.text+"$$":p.text)+"\n\n";
    return {std::string(block.number)+" / "+block.title,std::move(source)};
  }
  for(const auto& section:matrixChapter())if(id==section.id && !section.explanation.empty()) {
    std::string source;for(const auto* text:section.explanation)source+=std::string(text)+"\n\n";
    return {section.title,std::move(source)};
  }
  return {std::string(id),"This reading reference is unavailable. Your question and working are retained."};
}
void drawReading(MathCorpusUiState& ui,const CorpusStarter& question,bool blocked) {
  if(ui.readingIndex>=question.readingRefs.size())ui.readingIndex=0;
  ui.readingChoices.clear();const auto selected=reading(question.readingRefs[ui.readingIndex]);
  ImGui::SetNextItemWidth(-1);
  if(ImGui::BeginCombo("##question-reading",selected.first.c_str())) {
    for(std::size_t i=0;i<question.readingRefs.size();++i) {
      const auto reference=reading(question.readingRefs[i]);
      if(ImGui::Selectable(reference.first.c_str(),ui.readingIndex==i))ui.readingIndex=i;
      ui.readingChoices.push_back(item(!blocked));
    }
    ImGui::EndCombo();
  }
  ui.readingSource=reading(question.readingRefs[ui.readingIndex]).second;
  const auto& document=ui.math->layoutDocument(ui.readingSource,std::max(1.0F,ImGui::GetContentRegionAvail().x));
  const auto p=ImGui::GetCursorScreenPos();ui.math->draw(document,p.x,p.y,IM_COL32(221,221,226,255),cyan,gold);
  ImGui::Dummy({document.width,document.height});ui.readingBody=item();ui.readingFallbacks=document.fallbacks;
}
}
void drawCorpusQuestions(MathCorpusUiState& ui,const MathCorpus& corpus,bool blocked) {
  auto& practice=*ui.practice;auto& math=*ui.math;
  const auto& questions=practice.questions();ui.answerTiles.clear();ui.questionInk={};ui.currentWorking={};ui.readingBody={};ui.readingSource.clear();ui.readingFallbacks=0;
  if(ui.refresh) {
    ui.questionMatches=practice.find(ui.subject,ui.topic,ui.query.data());ui.refresh=false;
    if(!ui.questionMatches.empty() && (!practice.selected() ||
        std::find(ui.questionMatches.begin(),ui.questionMatches.end(),*practice.selected())==ui.questionMatches.end()))
      practice.open(ui.questionMatches.front());
  }
  if(!ui.focusQuestion)ImGui::TextDisabled("%zu questions",ui.questionMatches.size());
  const bool narrow=ImGui::GetIO().DisplaySize.x<700;
  const float available=ImGui::GetContentRegionAvail().x;
  const auto label=[&](std::size_t index) {
    const auto& q=questions[index];const auto* attempt=practice.attempt(index);
    return std::string(attempt && attempt->currentRun().completed?"[done] ":attempt && attempt->progress()!=fm::QuestionProgress::NotStarted?"[started] ":"")+
      (q.level=="subcategory"?"  ":"")+q.title;
  };
  if(!ui.focusQuestion && narrow) {
    ImGui::SetNextItemWidth(available);
    if(ImGui::BeginCombo("##starter",practice.selected()?label(*practice.selected()).c_str():"Choose a question")) {
      for(const auto i:ui.questionMatches) {
        ImGui::PushID(questions[i].id.c_str());
        if(ImGui::Selectable(label(i).c_str(),practice.selected()==i))practice.open(i);
        ui.rows.emplace_back(i,item(!blocked));ImGui::PopID();
      }
      ImGui::EndCombo();
    }
  } else if(!ui.focusQuestion) {
    ImGui::BeginChild("Starting question list",{std::min(300.0F,available*.3F),0},ImGuiChildFlags_Borders);
    ImGuiListClipper clipper;clipper.Begin(static_cast<int>(ui.questionMatches.size()),41);
    while(clipper.Step())for(int row=clipper.DisplayStart;row<clipper.DisplayEnd;++row) {
      const auto i=ui.questionMatches[row];const auto& q=questions[i];ImGui::PushID(q.id.c_str());
      const auto text=label(i);
      if(ImGui::Selectable("##question",practice.selected()==i,0,{0,36}))practice.open(i);
      const auto bounds=item(!blocked);ui.rows.emplace_back(i,bounds);
      auto* draw=ImGui::GetWindowDrawList();draw->PushClipRect({bounds.x,bounds.y},{bounds.x+bounds.width,bounds.y+bounds.height},true);
      const auto* attempt=practice.attempt(i);
      draw->AddText(ImGui::GetFont(),13,{bounds.x+3,bounds.y+2},attempt && attempt->currentRun().completed?green:IM_COL32(213,213,226,255),text.c_str(),nullptr,bounds.width-8);
      draw->PopClipRect();if(ImGui::IsItemHovered())ImGui::SetTooltip("%s / %s",corpus.subjects[q.subject].title.c_str(),text.c_str());
      ImGui::PopID();
    }
    ImGui::EndChild();ImGui::SameLine();
  }
  ImGui::BeginChild("Starter workspace",{0,0},ImGuiChildFlags_Borders);
  if(ui.questionMatches.empty() || !practice.active()) {
    ImGui::TextWrapped("No matching questions. Clear the search or choose another chapter.");ImGui::EndChild();return;
  }
  auto& session=*practice.active();const auto selected=*practice.selected();const auto& q=questions[selected];
  if(ui.readingQuestion!=q.id){ui.readingQuestion=q.id;ui.readingOpen=false;ui.readingIndex=0;}
  const auto& run=session.currentRun();const auto stepIndex=run.currentStep;const auto& step=q.question.steps[stepIndex];
  const bool complete=run.completed;
  const float width=ImGui::GetContentRegionAvail().x;
  ImGui::TextDisabled("%s",q.title.c_str());
  if(ImGui::IsItemHovered())ImGui::SetTooltip("%s / %s · %s",corpus.subjects[q.subject].title.c_str(),q.title.c_str(),q.level.c_str());
  ImGui::PushTextWrapPos(0);ImGui::TextColored({1,.78F,.3F,1},"%s",q.question.description.c_str());ImGui::PopTextWrapPos();
  const auto problem=equation(math,q.question.equation,width,18);
  if(q.level=="practice") {
    const float column=(width-8)*.5F;float height=problem.height;
    for(const auto& state:q.question.workingStates)height=std::max(height,equation(math,state.display,column,16).height);
    height=std::clamp(height+30,64.0F,120.0F);
    ImGui::BeginChild("Given matrix",{column,height},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextDisabled("Given");ink(math,equation(math,q.question.equation,column,16),q.question.equation,gold);ui.questionInk=item();ImGui::EndChild();ImGui::SameLine();
    ImGui::BeginChild("Current matrix",{0,height},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextDisabled("Working");const auto working=session.visibleWorking();ink(math,equation(math,working,column,16),working,complete?green:cyan);ui.currentWorking=item();ImGui::EndChild();
  } else {
    ImGui::BeginChild("Pinned problem",{0,std::clamp(problem.height+12,36.0F,narrow?82.0F:116.0F)},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
    ink(math,problem,q.question.equation,gold);ui.questionInk=item();ImGui::EndChild();
  }
  const auto record=[&](CorpusControl c,bool enabled){ui.controls[static_cast<std::size_t>(c)]=item(enabled && !blocked);};
  const auto position=std::find(ui.questionMatches.begin(),ui.questionMatches.end(),selected);
  const bool hasNext=position!=ui.questionMatches.end() && position+1!=ui.questionMatches.end();
  ImGui::PushStyleColor(ImGuiCol_Button,{.15F,.27F,.46F,1});
  ImGui::BeginDisabled(!complete || !hasNext);
  bool next=ImGui::Button("Next",{50,22});record(CorpusControl::NextStarter,complete && hasNext);ImGui::EndDisabled();ImGui::SameLine();
  ImGui::BeginDisabled(!complete);bool replay=ImGui::Button("Again",{50,22});record(CorpusControl::ReplayStarter,complete);ImGui::EndDisabled();
  ImGui::PopStyleColor();ImGui::SameLine();
  if(!q.readingRefs.empty()) {
    ImGui::PushStyleColor(ImGuiCol_Button,{.30F,.20F,.46F,1});
    if(ImGui::Button(ui.readingOpen?"Working###method":"Method###method",{66,22}))ui.readingOpen=!ui.readingOpen;
    record(CorpusControl::Method,true);ImGui::PopStyleColor();ImGui::SameLine();
  }
  if(complete)ImGui::TextColored({.4F,.9F,.65F,1},"Complete");else ImGui::TextDisabled("%zu / %zu · %s",stepIndex+1,q.question.steps.size(),step.layerName.c_str());
  std::optional<fm::OptionId> choice;
  if(!complete) {
    ImGui::TextUnformatted(step.prompt.c_str());
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
    const auto p=ImGui::GetWindowPos(),s=ImGui::GetWindowSize();ui.choiceArea={p.x,p.y,s.x,s.y,true};x=0;
    for(std::size_t i=0;i<tiles.size();++i) {
      const auto& e=tiles[i];const float w=std::max(70.0F,e.width+20),h=std::max(32.0F,e.height+12);
      if(x>0 && x+w<=width-16)ImGui::SameLine();else x=0;
      ImGui::PushID(static_cast<int>(i));const auto p=ImGui::GetCursorScreenPos();
      if(ImGui::Button("##answer",{w,h}))choice=step.options[i].id;
      ui.answerTiles.push_back(item(!blocked));
      if(e.error.empty())math.draw(e,p.x+(w-e.width)/2,p.y+(h-e.height)/2,cyan);
      else ImGui::GetWindowDrawList()->AddText({p.x+4,p.y+4},gold,step.options[i].label.c_str());
      ImGui::PopID();x+=w+6;
    }
    ImGui::EndChild();
  }
  ImGui::BeginChild("Starter working",{0,0},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
  if(ui.readingOpen && !q.readingRefs.empty())drawReading(ui,q,blocked);
  else {
  if(!complete && !run.steps[stepIndex].attempts.empty() && !run.steps[stepIndex].attempts.back().correct)
    ImGui::TextColored({1,.6F,.4F,1},"Try another tile. Working retained.");
  if((stepIndex || complete) && q.level!="practice") {
    const auto working=session.visibleWorking();ink(math,equation(math,working,width),working,complete?green:cyan);
  }
  const auto review=session.review();
  for(std::size_t i=0;i<review->steps.size();++i)if(!review->steps[i].explanation.empty()) {
    ImGui::TextDisabled("%zu · %s",i+1,q.question.steps[i].layerName.c_str());
    if(complete && i+1<q.question.steps.size()) {
      const auto& state=q.question.workingStates[i+1].display;ink(math,equation(math,state,width),state,cyan);
    }
    ImGui::TextWrapped("%.*s",static_cast<int>(review->steps[i].explanation.size()),review->steps[i].explanation.data());
  }
  if(!practice.message().empty())ImGui::TextWrapped("%s",practice.message().c_str());
  }
  ImGui::EndChild();ImGui::EndChild();
  if(choice) {
    (void)practice.dispatch(fm::LayeredQuestionCommand::submitOption(*choice));
    if(fm::layeredQuestionStepResolved(session.currentRun().steps[stepIndex]))
      (void)practice.dispatch({fm::LayeredQuestionCommandKind::Continue});
  }
  if(next && hasNext)practice.open(*(position+1));
  if(replay)(void)practice.dispatch({fm::LayeredQuestionCommandKind::RestartQuestion});
}
}

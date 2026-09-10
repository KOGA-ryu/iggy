#include "ui/MathCorpusUi.hpp"

#include <algorithm>
#include <imgui.h>

namespace paths {
namespace {
NotationBounds item(bool enabled=true) {
  const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
  return {a.x,a.y,b.x-a.x,b.y-a.y,enabled && ImGui::IsItemVisible()};
}
NotationBounds panel() {
  const auto p=ImGui::GetWindowPos(),s=ImGui::GetWindowSize();return {p.x,p.y,s.x,s.y,true};
}
constexpr ImVec4 violet{.76F,.65F,1,1},gold{1,.78F,.3F,1};
void drawEquations(NativeMathPanelState& ui,NativeMath& math,bool blocked) {
  ui.samples={};ui.controls={};ui.viewport={};ui.ink={};
  const auto size=ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos({size.x*.5F,size.y*.5F},ImGuiCond_Always,{.5F,.5F});
  ImGui::SetNextWindowSize({std::min(720.0F,size.x-24),std::min(500.0F,size.y-24)});
  if(!ImGui::BeginPopupModal("Native equations",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove))return;
  ImGui::PushFont(nullptr,13);ImGui::BeginDisabled(blocked);
  constexpr std::array colours{IM_COL32(89,217,255,255),IM_COL32(255,199,77,255),IM_COL32(102,230,166,255),IM_COL32(194,166,255,255)};
  for(std::size_t i=0;i<nativeMathSamples.size();++i) {
    if(i)ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text,colours[i]);
    if(ImGui::Button(nativeMathSamples[i].label,{i==0?66.0F:54.0F,22}))ui.sample=i;
    ui.samples[i]=item(!blocked);ImGui::PopStyleColor();
  }
  const auto record=[&](MathPanelControl c,bool enabled=true){ui.controls[static_cast<std::size_t>(c)]=item(enabled && !blocked);};
  ImGui::BeginDisabled(ui.pixels==16);
  if(ImGui::Button("A-",{30,22}))ui.pixels-=2;
  record(MathPanelControl::Smaller,ui.pixels>16);ImGui::EndDisabled();ImGui::SameLine();
  ImGui::Text("%d",ui.pixels);ImGui::SameLine();ImGui::BeginDisabled(ui.pixels==40);
  if(ImGui::Button("A+",{30,22}))ui.pixels+=2;
  record(MathPanelControl::Larger,ui.pixels<40);ImGui::EndDisabled();ImGui::SameLine();
  if(ImGui::Button(ui.source?"Hide source###equation_source":"Show source###equation_source",{98,22}))ui.source=!ui.source;
  record(MathPanelControl::Source);
  const auto& sample=nativeMathSamples[ui.sample];
  ImGui::TextWrapped("%s",sample.title);
  const auto& equation=math.layout(sample.latex,static_cast<float>(ui.pixels));ui.error=equation.error;
  ImGui::BeginChild("Equation viewport",{0,std::max(120.0F,ImGui::GetContentRegionAvail().y*.6F)},ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar);
  ui.viewport=panel();
  if(equation.error.empty()) {
    const auto available=ImGui::GetContentRegionAvail();
    ImGui::SetCursorPos({std::max(8.0F,(available.x-equation.width)*.5F),std::max(8.0F,(available.y-equation.height)*.5F)});
    const auto p=ImGui::GetCursorScreenPos();math.draw(equation,p.x,p.y,colours[ui.sample]);
    ui.ink={p.x,p.y,equation.width,equation.height,true};
    ImGui::Dummy({equation.width+8,equation.height+8});
  } else {
    ImGui::TextColored({1,.5F,.5F,1},"Unable to typeset this equation.");
    ImGui::TextWrapped("%s",equation.error.c_str());ImGui::TextWrapped("%s",sample.latex);
  }
  ui.horizontalScrollMax=ImGui::GetScrollMaxX();ImGui::EndChild();
  if(ui.source) {
    ImGui::BeginChild("Equation source",{0,std::max(30.0F,ImGui::GetContentRegionAvail().y-29)},ImGuiChildFlags_Borders);
    ImGui::TextWrapped("%s",sample.latex);ImGui::EndChild();
  } else {
    if(ui.sample==3)ImGui::TextWrapped("Trefethen · Spectral Methods in MATLAB · 11.3");
    ImGui::Dummy({0,std::max(0.0F,ImGui::GetContentRegionAvail().y-27)});
  }
  if(ImGui::Button("Close",{58,22}) || (!blocked && ImGui::IsKeyPressed(ImGuiKey_Escape,false)))ImGui::CloseCurrentPopup();
  record(MathPanelControl::Close);
  ImGui::EndDisabled();ImGui::PopFont();ImGui::EndPopup();
}
}
void refreshMathCorpusPreview(MathCorpusUiState& ui,const MathCorpus& corpus,const DocumentRemap& remap) {
  const auto mapped=[](std::optional<std::size_t> i,const auto& indices)->std::optional<std::size_t> {
    return i && *i<indices.size()?indices[*i]:std::nullopt;
  };
  ui.subject=mapped(ui.subject,remap.subjects);ui.topic=mapped(ui.topic,remap.topics);
  ui.entry=mapped(ui.entry,remap.entries);ui.shown=mapped(ui.shown,remap.entries);
  std::erase_if(ui.trail,[&](auto& bookmark){const auto i=mapped(bookmark.entry,remap.entries);if(i)bookmark.entry=*i;return !i;});
  ui.matches=corpus.find(ui.subject,ui.topic,ui.query.data());
  if(ui.entry){ui.restoreScroll=ui.scroll;ui.restoreHorizontal=ui.horizontal;}
  else {ui.focusDocument=false;ui.entry=ui.matches.empty()?std::nullopt:std::optional{ui.matches.front()};ui.top=true;ui.restoreScroll.reset();ui.restoreHorizontal.reset();}
  if(ui.practice) {
    ui.questionMatches=ui.practice->find(ui.subject,ui.topic,ui.query.data());
    if(ui.focusQuestion && ui.practice->selected() && std::find(ui.questionMatches.begin(),ui.questionMatches.end(),*ui.practice->selected())==ui.questionMatches.end())
      ui.questionMatches.push_back(*ui.practice->selected());
    if(!ui.practice->active())ui.focusQuestion=false;
  }
  ui.document.entry.clear();ui.document.presented=false;ui.document.questionLinks.clear();ui.document.parameterControls.clear();
  ui.document.helpMasks.clear();ui.document.anchor.clear();ui.readingChoices.clear();ui.readingSource.clear();
  ui.questionReading.entry.clear();ui.questionReading.helpMasks.clear();ui.questionReading.anchor.clear();
  ui.rows.clear();ui.subjectRows.clear();ui.topicRows.clear();ui.relatedRows.clear();ui.answerTiles.clear();ui.controls={};
  ui.supportLevels={};ui.supportHelp={};ui.supportEditor={};ui.refresh=false;ui.follow=true;++ui.previewRevision;
}
void drawMathCorpus(MathCorpusUiState& ui,const MathCorpus& corpus,bool blocked) {
  auto& io=ImGui::GetIO();blocked=blocked || io.AppFocusLost;
  const bool narrow=io.DisplaySize.x<700;
  const bool editing=io.WantTextInput; // Escape may deactivate an editor later in this frame.
  ui.document.presented=false;
  ui.controls={};ui.rows.clear();ui.subjectRows.clear();ui.topicRows.clear();ui.relatedRows.clear();
  std::optional<std::size_t> linkedEntry;bool back=false;
  ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::PushStyleColor(ImGuiCol_WindowBg,{.055F,.065F,.085F,1});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{12,10});
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{5,3});
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{6,5});
  const bool document=ui.focusDocument && !ui.questions && ui.entry && corpus.entries[*ui.entry].document;
  ImGui::Begin("Math library",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|(document?ImGuiWindowFlags_NoBackground:0));
  ImGui::PushFont(nullptr,13);ImGui::BeginDisabled(blocked);
  if(ui.livePreview || ui.importFailed) {
    if(ui.importFailed)ImGui::BeginChild("Document errors",{0,66},ImGuiChildFlags_Borders);
    ImGui::PushTextWrapPos(0);
    ImGui::TextColored(ui.importFailed?ImVec4{1,.5F,.4F,1}:ImVec4{.35F,.85F,1,1},"%s",ui.importMessage.c_str());
    ImGui::PopTextWrapPos();
    if(ui.importFailed)ImGui::EndChild();
  }
  const auto record=[&](CorpusControl control,bool enabled=true){ui.controls[static_cast<std::size_t>(control)]=item(enabled && !blocked);};
  if(document) {
    if(ImGui::Button("Browse",{66,22}))ui.focusDocument=false;record(CorpusControl::OpenDocument);
    ImGui::SameLine();ImGui::TextColored(violet,"Lesson workspace");
    if(auto id=drawDocumentLesson(corpus.entries[*ui.entry],*ui.math,ui.document,blocked);id && ui.practice) {
      const auto& questions=ui.practice->questions();
      const auto q=std::find_if(questions.begin(),questions.end(),[&](const auto& q){return q.id==*id;});
      if(q!=questions.end()) {
        ui.practice->open(q-questions.begin());ui.subject=q->subject;ui.topic=q->topic;ui.query={};
        ui.questionMatches=ui.practice->find(ui.subject,ui.topic,"");ui.refresh=false;
        ui.questions=true;ui.focusQuestion=true;
      }
    }
    if(!blocked && !editing && ImGui::IsKeyPressed(ImGuiKey_Escape,false) && !ImGui::IsAnyItemActive())ui.focusDocument=false;
    ImGui::EndDisabled();ImGui::PopFont();ImGui::End();ImGui::PopStyleVar(3);ImGui::PopStyleColor();return;
  }
  if(ui.questions && ui.focusQuestion && ui.practice && ui.math) {
    if(ImGui::Button("Browse",{66,22}))ui.focusQuestion=false;
    record(CorpusControl::FocusQuestion);ImGui::SameLine();ImGui::TextColored(violet,"Question workspace");
    drawCorpusQuestions(ui,corpus,blocked);
    if(!blocked && !editing && ImGui::IsKeyPressed(ImGuiKey_Escape) && !ImGui::IsAnyItemActive())ui.focusQuestion=false;
    ImGui::EndDisabled();ImGui::PopFont();ImGui::End();ImGui::PopStyleVar(3);ImGui::PopStyleColor();return;
  }
  ImGui::PushStyleColor(ImGuiCol_Button,{.15F,.27F,.46F,1});
  if(ImGui::Button("Practice",{68,22}))ui.open=false;
  record(CorpusControl::Practice);ImGui::PopStyleColor();ImGui::SameLine();
  ImGui::TextColored(violet,"Math library");
  if(ui.math) {
    ImGui::SameLine();ImGui::PushStyleColor(ImGuiCol_Button,{.15F,.27F,.46F,1});
    if(ImGui::Button("Equations",{78,22}))ImGui::OpenPopup("Native equations");
    record(CorpusControl::Equations);ImGui::PopStyleColor();
  }
  if(ui.practice && ui.math) {
    if(!narrow)ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,{.15F,.27F,.46F,1});
    if(ImGui::Button(ui.questions?"Definitions###starters":"Questions###starters",{88,22})) {
      ui.questions=!ui.questions;ui.refresh=true;ui.query={};
    }
    record(CorpusControl::Questions);ImGui::PopStyleColor();
    if(ui.questions) {
      ImGui::SameLine();if(ImGui::Button("Focus",{56,22}))ui.focusQuestion=true;
      record(CorpusControl::FocusQuestion);
    }
  }
  ui.equations.open=ImGui::IsPopupOpen("Native equations");
  const float available=ImGui::GetContentRegionAvail().x,half=(available-6)*.5F;
  ImGui::SetNextItemWidth(narrow?available:half);
  if(ImGui::BeginCombo("##subject",ui.subject?corpus.subjects[*ui.subject].title.c_str():"All subjects")) {
    if(ImGui::Selectable("All subjects",!ui.subject)){ui.subject.reset();ui.topic.reset();ui.refresh=true;}
    ui.subjectRows.push_back({corpus.subjects.size(),item(!blocked)});
    for(std::size_t i=0;i<corpus.subjects.size();++i) {
      if(ImGui::Selectable(corpus.subjects[i].title.c_str(),ui.subject==i)){ui.subject=i;ui.topic.reset();ui.refresh=true;}
      ui.subjectRows.push_back({i,item(!blocked)});
    }
    ImGui::EndCombo();
  }
  record(CorpusControl::Subject);
  if(!narrow)ImGui::SameLine();
  ImGui::SetNextItemWidth(narrow?available:half);
  if(ImGui::BeginCombo("##topic",ui.topic?corpus.topics[*ui.topic].title.c_str():"All topics")) {
    if(ImGui::Selectable("All topics",!ui.topic)){ui.topic.reset();ui.refresh=true;}
    ui.topicRows.push_back({corpus.topics.size(),item(!blocked)});
    for(std::size_t i=0;i<corpus.topics.size();++i)if(!ui.subject || corpus.topics[i].subject==*ui.subject) {
      ImGui::PushID(corpus.topics[i].id.c_str());
      const auto label=ui.subject?corpus.topics[i].title:corpus.subjects[corpus.topics[i].subject].title+" / "+corpus.topics[i].title;
      if(ImGui::Selectable(label.c_str(),ui.topic==i)){ui.topic=i;ui.refresh=true;}
      ui.topicRows.push_back({i,item(!blocked)});
      ImGui::PopID();
    }
    ImGui::EndCombo();
  }
  record(CorpusControl::Topic);
  ImGui::SetNextItemWidth(available-54);
  if(ImGui::InputTextWithHint("##search","Find a title",ui.query.data(),ui.query.size()))ui.refresh=true;
  record(CorpusControl::Search);ImGui::SameLine();
  if(ImGui::Button("Clear",{48,22})){ui.query={};ui.refresh=true;}
  record(CorpusControl::Clear);
  if(ui.questions && ui.practice && ui.math) {
    drawCorpusQuestions(ui,corpus,blocked);
    if(!blocked && !editing && !ui.equations.open && ImGui::IsKeyPressed(ImGuiKey_Escape,false) && !ImGui::IsAnyItemActive())ui.open=false;
    ImGui::EndDisabled();ImGui::PopFont();
    drawEquations(ui.equations,*ui.math,blocked);
    ui.equations.open=ImGui::IsPopupOpen("Native equations");
    ImGui::End();ImGui::PopStyleVar(3);ImGui::PopStyleColor();return;
  }
  if(ui.refresh) {
    ui.trail.clear();ui.restoreScroll.reset();ui.restoreHorizontal.reset();ui.focusReader=false;
    ui.matches=corpus.find(ui.subject,ui.topic,ui.query.data());
    if(!ui.entry || std::find(ui.matches.begin(),ui.matches.end(),*ui.entry)==ui.matches.end())
      ui.entry=ui.matches.empty()?std::nullopt:std::optional{ui.matches.front()};
    ui.top=true;ui.follow=true;ui.refresh=false;
  }
  const auto reviewed=std::count_if(ui.matches.begin(),ui.matches.end(),[&](auto i){return corpus.entries[i].review.has_value();});
  ImGui::TextDisabled("%zu entries · %zu reviewed notes",ui.matches.size(),static_cast<std::size_t>(reviewed));
  const float height=ImGui::GetContentRegionAvail().y;
  const float listWidth=narrow?available:std::clamp(available*.37F,250.0F,400.0F);
  if(ui.follow) {
    const auto selected=ui.entry?std::find(ui.matches.begin(),ui.matches.end(),*ui.entry):ui.matches.begin();
    ImGui::SetNextWindowScroll({0,static_cast<float>(selected-ui.matches.begin())*43});ui.follow=false;
  }
  ImGui::BeginChild("Entries",{listWidth,narrow?std::min(140.0F,height*.42F):height},ImGuiChildFlags_Borders|ImGuiChildFlags_NavFlattened);
  ui.list=panel();
  if(ui.matches.empty())ImGui::TextWrapped("No matching titles. Clear the search or choose another topic.");
  ImGuiListClipper clipper;clipper.Begin(static_cast<int>(ui.matches.size()),43);
  while(clipper.Step())for(int row=clipper.DisplayStart;row<clipper.DisplayEnd;++row) {
    const auto index=ui.matches[row];const auto& entry=corpus.entries[index];
    ImGui::PushID(entry.id.c_str());
    if(ImGui::Selectable("##entry",ui.entry==index,0,{0,38})){ui.entry=index;ui.top=true;ui.trail.clear();}
    const auto bounds=item(!blocked);ui.rows.push_back({index,bounds});
    auto* draw=ImGui::GetWindowDrawList();
    draw->PushClipRect({bounds.x,bounds.y},{bounds.x+bounds.width-2,bounds.y+36},true);
    draw->AddText(ImGui::GetFont(),13,{bounds.x+3,bounds.y+3},entry.review?IM_COL32(102,230,166,255):ImGui::GetColorU32(ImGuiCol_Text),entry.title.c_str(),nullptr,bounds.width-8);
    draw->PopClipRect();
    if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",entry.title.c_str());
    ImGui::PopID();
  }
  ImGui::EndChild();
  if(!narrow)ImGui::SameLine();
  ImGui::BeginChild("Reading",{0,0},ImGuiChildFlags_Borders|ImGuiChildFlags_NavFlattened);
  const auto found=ui.entry?std::find(ui.matches.begin(),ui.matches.end(),*ui.entry):ui.matches.end();
  const auto index=static_cast<std::size_t>(found-ui.matches.begin());
  const std::array labels{"<",">","Up","Dn"};
  const std::array enabled{found!=ui.matches.end() && index>0,found!=ui.matches.end() && index+1<ui.matches.size(),ui.scroll>0,ui.scroll<ui.scrollMax};
  std::optional<float> scroll;
  for(std::size_t i=0;i<labels.size();++i) {
    if(i)ImGui::SameLine();ImGui::BeginDisabled(!enabled[i]);
    if(ImGui::Button(labels[i],{30,22})) {
      switch(i) {
        case 0:ui.entry=ui.matches[index-1];ui.top=true;ui.follow=true;ui.trail.clear();break;
        case 1:ui.entry=ui.matches[index+1];ui.top=true;ui.follow=true;ui.trail.clear();break;
        case 2:scroll=std::max(0.0F,ui.scroll-ui.pageHeight*.8F);break;
        case 3:scroll=std::min(ui.scrollMax,ui.scroll+ui.pageHeight*.8F);break;
      }
    }
    record(static_cast<CorpusControl>(static_cast<std::size_t>(CorpusControl::Previous)+i),enabled[i]);ImGui::EndDisabled();
    // A paging control can disable itself at the end of a note. Move focus
    // onward so Tab is not anchored to an item that no longer participates.
    if(!enabled[i] && ImGui::IsItemFocused())ImGui::SetKeyboardFocusHere();
  }
  if(ui.shown!=ui.entry){ui.shown=ui.entry;ui.original=false;}
  if(ui.entry && corpus.entries[*ui.entry].review) {
    ImGui::SameLine();
    if(ui.focusReader){ImGui::SetKeyboardFocusHere();ui.focusReader=false;}
    if(ImGui::Button(ui.original?"Reviewed###source_view":"Original###source_view",{68,22})){ui.original=!ui.original;ui.top=true;}
    record(CorpusControl::Source);
  }
  if(!ui.trail.empty()) {
    ImGui::SameLine();back=ImGui::Button("Back",{52,22});record(CorpusControl::Back);
  }
  if(ui.math && ui.entry && (!corpus.entries[*ui.entry].review || ui.original)) {
    ImGui::PushStyleColor(ImGuiCol_Button,{.15F,.27F,.46F,1});
    if(ImGui::Button(ui.raw?"Typeset###format":"Raw source###format",{84,22})){ui.raw=!ui.raw;ui.top=true;}
    record(CorpusControl::Format);ImGui::PopStyleColor();
  }
  if(ui.math && ui.entry && corpus.entries[*ui.entry].document) {
    ImGui::SameLine();ImGui::PushStyleColor(ImGuiCol_Button,{.15F,.27F,.46F,1});
    if(ImGui::Button("Open lesson",{92,22}))ui.focusDocument=true;
    record(CorpusControl::OpenDocument);ImGui::PopStyleColor();
  }
  if(ui.top){scroll=0;ui.restoreScroll.reset();ui.restoreHorizontal.reset();}
  if(scroll)ImGui::SetNextWindowScroll({ui.top?0.0F:-1.0F,*scroll});
  ImGui::BeginChild("Source notes",{0,0},ImGuiChildFlags_NavFlattened,ImGuiWindowFlags_HorizontalScrollbar);
  ui.reader=panel();
  ui.reviewedVisible=false;ui.body={};ui.bodyEquations=0;ui.bodyFallbacks=0;
  // Reserve a vertical scrollbar consistently; horizontal scrolling must not
  // change prose wrapping or move the equation farther away on the next frame.
  const float wrapWidth=std::max(1.0F,ImGui::GetWindowWidth()-2*ImGui::GetStyle().WindowPadding.x-ImGui::GetStyle().ScrollbarSize);
  if(ui.entry) {
    const auto& entry=corpus.entries[*ui.entry];const auto& topic=corpus.topics[entry.topic];
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+wrapWidth);ImGui::TextColored(violet,"%s / %s",corpus.subjects[topic.subject].title.c_str(),topic.title.c_str());
    ImGui::TextColored(gold,"%s",entry.title.c_str());
    if(entry.review && !ui.original) {
      const auto& review=*entry.review;ui.reviewedVisible=true;
      ImGui::TextColored({.4F,.9F,.65F,1},"Reviewed teaching note · v%u",review.term.version);
      ImGui::TextWrapped("%s",review.term.meaning.c_str());ImGui::Separator();
      ImGui::TextWrapped("%s",review.term.definition.c_str());
      for(const auto& [label,text]:std::array{std::pair{"CONDITIONS",&review.conditions},std::pair{"EXAMPLE",&review.term.example},std::pair{"REVIEW NOTE",&review.change}}) {
        ImGui::TextDisabled("%s",label);ImGui::TextWrapped("%s",text->c_str());
      }
      ImGui::TextDisabled("REFERENCES · %s",review.reviewedOn.c_str());
      for(const auto& source:review.sources) {
        ImGui::TextWrapped("%s — %s",source.title.c_str(),source.locator.c_str());
        ImGui::TextWrapped("%s",source.url.c_str());
      }
    } else {
      ImGui::TextDisabled("%s · Not yet fact-checked",entry.kind.c_str());ImGui::Separator();
      if(ui.math && !ui.raw) {
        const auto& document=ui.math->layoutDocument(entry.body,wrapWidth);
        ui.bodyEquations=document.equations;ui.bodyFallbacks=document.fallbacks;
        if(document.fallbacks)ImGui::TextColored({1,.78F,.3F,1},"Amber notation is shown as source. Hover for the reason.");
        const auto p=ImGui::GetCursorScreenPos();
        ui.math->draw(document,p.x,p.y,ImGui::GetColorU32(ImGuiCol_Text),IM_COL32(89,217,255,255),IM_COL32(255,199,77,255));
        ImGui::Dummy({document.width,document.height});ui.body=item();
      } else {ImGui::TextUnformatted(entry.body.c_str());ui.body=item();}
    }
    if(!entry.related.empty()) {
      ImGui::Separator();ImGui::TextDisabled("RELATED NOTES");
      ImGui::PushStyleColor(ImGuiCol_Text,{.35F,.85F,1,1});
      for(const auto target:entry.related) {
        const auto& related=corpus.entries[target];ImGui::PushID(related.id.c_str());
        if(ImGui::Button(related.title.c_str(),{wrapWidth,22}))linkedEntry=target;
        ui.relatedRows.push_back({target,item(!blocked)});ImGui::PopID();
      }
      ImGui::PopStyleColor();
    }
    ImGui::Separator();
    ImGui::TextDisabled("%s : %zu–%zu",entry.source.c_str(),entry.firstLine,entry.lastLine);ImGui::PopTextWrapPos();
  } else ImGui::TextWrapped("Choose a title to read its notes.");
  // Restore after submitting the destination so the next frame clamps against
  // its height, not the shorter note we just left.
  if(ui.restoreScroll){ImGui::SetScrollY(*ui.restoreScroll);ui.restoreScroll.reset();}
  if(ui.restoreHorizontal){ImGui::SetScrollX(*ui.restoreHorizontal);ui.restoreHorizontal.reset();}
  ui.scroll=ImGui::GetScrollY();ui.scrollMax=ImGui::GetScrollMaxY();ui.pageHeight=ui.reader.height;ui.top=false;
  ui.horizontal=ImGui::GetScrollX();ui.horizontalMax=ImGui::GetScrollMaxX();
  ImGui::EndChild();ImGui::EndChild();
  if(!blocked && !editing && !ui.equations.open && ImGui::IsKeyPressed(ImGuiKey_Escape,false) && !ImGui::IsAnyItemActive()) {
    if(!ui.trail.empty())back=true;else ui.open=false;
  }
  if(back) {
    const auto previous=ui.trail.back();ui.trail.pop_back();
    ui.entry=previous.entry;ui.shown=ui.entry;ui.original=previous.original;ui.raw=previous.raw;
    ui.restoreScroll=previous.scroll;ui.restoreHorizontal=previous.horizontal;ui.focusReader=true;
  } else if(linkedEntry && ui.entry) {
    if(ui.trail.size()==16)ui.trail.erase(ui.trail.begin());
    ui.trail.push_back({*ui.entry,ui.scroll,ui.original,ui.raw,ui.horizontal});ui.entry=linkedEntry;ui.top=true;ui.focusReader=true;
  }
  ImGui::EndDisabled();ImGui::PopFont();
  if(ui.math)drawEquations(ui.equations,*ui.math,blocked);
  ui.equations.open=ImGui::IsPopupOpen("Native equations");
  ImGui::End();ImGui::PopStyleVar(3);ImGui::PopStyleColor();
}
} // namespace paths

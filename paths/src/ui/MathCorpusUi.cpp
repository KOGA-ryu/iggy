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
}
void drawMathCorpus(MathCorpusUiState& ui,const MathCorpus& corpus,bool blocked) {
  auto& io=ImGui::GetIO();blocked=blocked || io.AppFocusLost;
  const bool narrow=io.DisplaySize.x<700;
  ui.controls={};ui.rows.clear();ui.subjectRows.clear();ui.topicRows.clear();ui.relatedRows.clear();
  std::optional<std::size_t> linkedEntry;bool back=false;
  ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::PushStyleColor(ImGuiCol_WindowBg,{.055F,.065F,.085F,1});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{12,10});
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{5,3});
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{6,5});
  ImGui::Begin("Math library",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
  ImGui::PushFont(nullptr,13);ImGui::BeginDisabled(blocked);
  const auto record=[&](CorpusControl control,bool enabled=true){ui.controls[static_cast<std::size_t>(control)]=item(enabled && !blocked);};
  ImGui::PushStyleColor(ImGuiCol_Button,{.15F,.27F,.46F,1});
  if(ImGui::Button("Practice",{68,22}))ui.open=false;
  record(CorpusControl::Practice);ImGui::PopStyleColor();ImGui::SameLine();
  ImGui::TextColored(violet,"Math library");
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
  if(ui.refresh) {
    ui.trail.clear();ui.restoreScroll.reset();ui.focusReader=false;
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
  if(ui.top){scroll=0;ui.restoreScroll.reset();}
  if(scroll)ImGui::SetNextWindowScroll({0,*scroll});
  ImGui::BeginChild("Source notes",{0,0},ImGuiChildFlags_NavFlattened);
  ui.reader=panel();
  ui.reviewedVisible=false;
  if(ui.entry) {
    const auto& entry=corpus.entries[*ui.entry];const auto& topic=corpus.topics[entry.topic];
    ImGui::PushTextWrapPos();ImGui::TextColored(violet,"%s / %s",corpus.subjects[topic.subject].title.c_str(),topic.title.c_str());
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
      ImGui::TextUnformatted(entry.body.c_str());
    }
    if(!entry.related.empty()) {
      ImGui::Separator();ImGui::TextDisabled("RELATED NOTES");
      ImGui::PushStyleColor(ImGuiCol_Text,{.35F,.85F,1,1});
      for(const auto target:entry.related) {
        const auto& related=corpus.entries[target];ImGui::PushID(related.id.c_str());
        if(ImGui::Button(related.title.c_str(),{ImGui::GetContentRegionAvail().x,22}))linkedEntry=target;
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
  ui.scroll=ImGui::GetScrollY();ui.scrollMax=ImGui::GetScrollMaxY();ui.pageHeight=ui.reader.height;ui.top=false;
  ImGui::EndChild();ImGui::EndChild();
  if(!blocked && ImGui::IsKeyPressed(ImGuiKey_Escape,false) && !ImGui::IsAnyItemActive()) {
    if(!ui.trail.empty())back=true;else ui.open=false;
  }
  if(back) {
    const auto previous=ui.trail.back();ui.trail.pop_back();
    ui.entry=previous.entry;ui.shown=ui.entry;ui.original=previous.original;ui.restoreScroll=previous.scroll;ui.focusReader=true;
  } else if(linkedEntry && ui.entry) {
    if(ui.trail.size()==16)ui.trail.erase(ui.trail.begin());
    ui.trail.push_back({*ui.entry,ui.scroll,ui.original});ui.entry=linkedEntry;ui.top=true;ui.focusReader=true;
  }
  ImGui::EndDisabled();ImGui::PopFont();ImGui::End();ImGui::PopStyleVar(3);ImGui::PopStyleColor();
}
} // namespace paths

#include "TextbookUi.hpp"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>

namespace paths {
namespace {
constexpr auto flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings;
void window(const char* name,ImVec2 at,ImVec2 size){
  ImGui::SetNextWindowPos(at);ImGui::SetNextWindowSize(size);ImGui::Begin(name,nullptr,flags);
}
void heading(const char* text){ImGui::Spacing();ImGui::TextColored({.45f,.82f,.78f,1},"%s",text);ImGui::Separator();}
bool contains(std::string_view haystack,std::string_view needle){
  return std::search(haystack.begin(),haystack.end(),needle.begin(),needle.end(),[](char a,char b){return std::tolower(static_cast<unsigned char>(a))==std::tolower(static_cast<unsigned char>(b));})!=haystack.end();
}
}
bool drawTextbook(Textbook& book,TextbookUiState& ui){
  applyMatrixBoardPending(book.board(),ui.boards[book.exerciseIndex()]);
  if(ui.hasPending){const auto result=book.dispatch(ui.pending);if(!result.accepted)ui.message=result.reason;ui.hasPending=false;}
  const auto v=book.view();const auto& sections=matrixChapter();const auto& section=sections[v.section];
  const auto send=[&](BookAction a){if(!ui.hasPending){ui.pending=a;ui.hasPending=true;}};
  const auto screen=ImGui::GetIO().DisplaySize;const float top=92,footer=58,sidebar=std::clamp(screen.x*.22f,205.f,290.f);
  bool objects=false;
  window("Textbook header",{0,0},{screen.x,top-4});
  ImGui::TextUnformatted("PATHS / INTERACTIVE MATHEMATICS");ImGui::Spacing();
  if(ImGui::Button("Contents"))send({BookActionKind::Contents});ImGui::SameLine();
  if(ImGui::Button("Index"))send({BookActionKind::Index});ImGui::SameLine();
  if(ImGui::Button("Explore 3D objects"))objects=true;ImGui::SameLine();
  ImGui::BeginDisabled(v.textScale<=.9001);if(ImGui::Button("A-"))send({BookActionKind::SetTextScale,0,std::max(.9,v.textScale-.1)});ImGui::EndDisabled();ImGui::SameLine();
  ImGui::BeginDisabled(v.textScale>=1.4999);if(ImGui::Button("A+"))send({BookActionKind::SetTextScale,0,std::min(1.5,v.textScale+.1)});ImGui::EndDisabled();
  ImGui::End();

  window("Textbook contents",{0,top},{sidebar,screen.y-top});
  ImGui::TextUnformatted("CONTENTS");ImGui::Spacing();
  if(ImGui::Button("Continue reading",{-1,30}))send({BookActionKind::Resume});
  ImGui::TextWrapped("Bookmark: %s",section.title);ImGui::Separator();
  for(unsigned p=0;p<textbookParts().size();++p){
    ImGui::PushID(static_cast<int>(p));
    if(p==3){
      if(ImGui::TreeNodeEx(textbookParts()[p],ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::TextWrapped("Chapter 1\nMatrices and Elimination");
        for(unsigned i=0;i<sections.size();++i){
          const bool selected=v.page==BookPage::Section&&v.section==i;
          // Wrap long section titles in the narrow contents column.
          const float width=ImGui::GetContentRegionAvail().x;
          const auto height=ImGui::CalcTextSize(sections[i].title,nullptr,false,width).y+10;
          ImGui::PushID(static_cast<int>(i));
          if(ImGui::Selectable("##section",selected,0,{width,height}))send({BookActionKind::OpenSection,i});
          const auto bounds=ImGui::GetItemRectMin();ImGui::GetWindowDrawList()->AddText(nullptr,0,{bounds.x+3,bounds.y+4},ImGui::GetColorU32(ImGuiCol_Text),sections[i].title,nullptr,width-6);
          ImGui::PopID();
        }ImGui::TreePop();
      }
    }else {ImGui::TextWrapped("%s",textbookParts()[p]);ImGui::TextDisabled("Outline / chapters to come");ImGui::Spacing();}
    ImGui::PopID();
  }
  ImGui::End();

  const bool reading=v.page==BookPage::Section&&v.mode==BookMode::Reading;
  if(reading&&(!ui.wasReading||ui.displayedSection!=v.section))ui.restoreFrames=2;
  ui.wasReading=reading;ui.displayedSection=v.section;
  if(reading&&ui.restoreFrames)ImGui::SetNextWindowScroll({0,static_cast<float>(v.scroll)});
  const std::string name=v.page==BookPage::Section?std::string(section.id)+(reading?" / reading":" / exercise"):(v.page==BookPage::Index?"Textbook index":"Textbook introduction");
  window(name.c_str(),{sidebar+6,top},{screen.x-sidebar-6,screen.y-top-footer});
  ImGui::PushFont(nullptr,ImGui::GetFontSize()*static_cast<float>(v.textScale));
  ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+std::min(850.f,ImGui::GetContentRegionAvail().x));
  switch(v.page){
    case BookPage::Contents:
      heading("An interactive textbook");
      ImGui::TextWrapped("Read an idea, explore its figure, and work through an exercise. The contents give you a sequence; the index lets you look up a term directly.");
      heading("Part IV / Linear Algebra");ImGui::TextUnformatted("Chapter 1 / Matrices and Elimination");
      ImGui::TextWrapped("Begin with entries and dimensions, then develop row operations, pivoting, LU factors, sparsity and block elimination. This first chapter has seven sections and six connected exercise cards.");
      if(ImGui::Button("Begin chapter",{-1,36}))send({BookActionKind::OpenSection,0});
      if(ImGui::Button("Continue from reading bookmark",{-1,36}))send({BookActionKind::Resume});
      heading("How to use a section");
      ImGui::TextWrapped("Each section contains an explanation, definitions, an interactive matrix figure, an optional worked example and an exercise. Reading and Exercise stay together, and changing sections preserves your board work during this session.");
      ImGui::TextWrapped("The reading bookmark remembers your section, scroll position and text size between launches. Opening a page never marks an exercise complete. Board attempts currently last for this session.");
      heading("The rest of the book");
      ImGui::TextWrapped("The other six parts establish the outline. Their chapters will be assembled using the same reading structure. The existing object collection remains available through Explore 3D objects.");
      break;
    case BookPage::Index:{
      heading("Index / Matrices and Elimination");ImGui::SetNextItemWidth(-1);ImGui::InputTextWithHint("##term","Find a term...",ui.search,sizeof(ui.search));
      unsigned matches=0;
      for(unsigned i=0;i<sections.size();++i)for(unsigned j=0;j<sections[i].terms.size();++j){const auto& term=sections[i].terms[j];
        if(!contains(term.name,ui.search)&&!contains(term.definition,ui.search))continue;
        ++matches;ImGui::PushID(static_cast<int>(i*3+j));heading(term.name);ImGui::TextWrapped("%s",term.definition);
        if(ImGui::Button(sections[i].title))send({BookActionKind::OpenSection,i});ImGui::PopID();
      }
      if(!matches)ImGui::TextUnformatted("No matching term in this chapter.");break;
    }
    case BookPage::Section:{
      ImGui::TextDisabled("PART IV / LINEAR ALGEBRA / CHAPTER 1");heading(section.title);
      ImGui::TextWrapped("%s",section.purpose);ImGui::Spacing();
      if(ImGui::Button("Reading"))send({BookActionKind::Read});ImGui::SameLine();
      char exerciseLabel[48];std::snprintf(exerciseLabel,sizeof(exerciseLabel),"Exercise / card %03u",section.card);
      if(ImGui::Button(exerciseLabel))send({BookActionKind::Exercise});
      auto& board=book.board();auto& boardUi=ui.boards[book.exerciseIndex()];
      if(v.mode==BookMode::Exercise){
        ImGui::TextWrapped("%s",section.exercisePrompt);ImGui::Separator();
        ImGui::BeginChild("Section exercise",{0,0});drawMatrixBoardContents(board,boardUi,true);ImGui::EndChild();
      }else {
        for(const auto paragraph:section.explanation){ImGui::Spacing();ImGui::TextWrapped("%s",paragraph);}
        heading("Definitions");for(const auto& term:section.terms){ImGui::TextColored({.9f,.78f,.49f,1},"%s",term.name);ImGui::TextWrapped("%s",term.definition);ImGui::Spacing();}
        heading("Interactive figure");ImGui::TextWrapped("%s",section.figurePrompt);
        const auto boardView=board.view();ImGui::Text("Card %03u / %s",section.card,boardView.working?"your current working matrix":"given matrix");
        ImGui::BeginChild("Reading figure",{0,255},ImGuiChildFlags_Borders);
        drawMatrixBoardGrid("Matrix",boardView.working?boardView.current:boardView.given,boardView,boardUi,!boardView.working);ImGui::EndChild();
        ImGui::BeginDisabled(boardView.complete||boardView.blocked);if(ImGui::Button("Perform next pivot")){boardUi.pending={BoardActionKind::Step};boardUi.hasPending=true;}ImGui::EndDisabled();ImGui::SameLine();
        if(ImGui::Button("Open full exercise"))send({BookActionKind::Exercise});
        ImGui::TextWrapped("%s",boardView.status.c_str());
        heading("Worked example");
        if(ImGui::CollapsingHeader(section.exampleTitle)){
          ImGui::TextDisabled("Separate teaching example / reveal is optional");
          for(unsigned i=0;i<section.exampleSteps.size();++i)ImGui::TextWrapped("%u. %s",i+1,section.exampleSteps[i]);
        }
        heading("Your exercise");ImGui::TextWrapped("%s",section.exercisePrompt);
        heading("Source and conventions");ImGui::TextWrapped("%s",section.reference);
        ImGui::TextWrapped("These are authored teaching notes. The original problem pages and learner fields remain unchanged.");
      }
      break;
    }
  }
  if(!ui.message.empty()){ImGui::Separator();ImGui::TextWrapped("%s",ui.message.c_str());}
  ImGui::PopTextWrapPos();ImGui::PopFont();
  if(reading){
    if(ui.restoreFrames)--ui.restoreFrames;
    else if(std::abs(static_cast<double>(ImGui::GetScrollY())-v.scroll)>.25)
      static_cast<void>(book.dispatch({BookActionKind::RememberScroll,v.section,ImGui::GetScrollY()}));
  }
  ImGui::End();

  window("Textbook navigation",{sidebar+6,screen.y-footer},{screen.x-sidebar-6,footer});
  ImGui::BeginDisabled(v.page!=BookPage::Section||v.section==0);if(ImGui::Button("< Previous"))send({BookActionKind::Previous});ImGui::EndDisabled();ImGui::SameLine();
  ImGui::Text("Section %u of %zu",v.section+1,sections.size());ImGui::SameLine();
  ImGui::BeginDisabled(v.page!=BookPage::Section||v.section+1==sections.size());if(ImGui::Button("Next >"))send({BookActionKind::Next});ImGui::EndDisabled();
  ImGui::End();return objects;
}
} // namespace paths

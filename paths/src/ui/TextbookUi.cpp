#include "TextbookUi.hpp"
#include "NativeMath.hpp"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>

namespace paths {
namespace {
constexpr auto flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings;
void window(const char* name,ImVec2 at,ImVec2 size){
  ImGui::SetNextWindowPos(at);ImGui::SetNextWindowSize(size);ImGui::Begin(name,nullptr,flags);
}
void heading(const char* text){
  ImGui::Spacing();ImGui::PushStyleColor(ImGuiCol_Text,{.45f,.82f,.78f,1});
  ImGui::TextWrapped("%s",text);ImGui::PopStyleColor();ImGui::Separator();
}
bool contains(std::string_view haystack,std::string_view needle){
  return std::search(haystack.begin(),haystack.end(),needle.begin(),needle.end(),[](char a,char b){return std::tolower(static_cast<unsigned char>(a))==std::tolower(static_cast<unsigned char>(b));})!=haystack.end();
}
void queue(TextbookUiState& ui,BookAction action){if(!ui.hasPending){ui.pending=action;ui.hasPending=true;}}
void nextControl(const char* label,float width){
  const float required=ImGui::CalcTextSize(label).x+2*ImGui::GetStyle().FramePadding.x;
  const float edge=ImGui::GetWindowPos().x+ImGui::GetCursorPosX()+width;
  if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+required<edge)ImGui::SameLine();
}
const NativeMath::Equation& equation(NativeMath& math,TextbookUiState& ui,const std::string& source){
  const float pixels=ImGui::GetFontSize()*1.1f;
  if(pixels!=ui.equationPixels){ui.equations.clear();ui.equationPixels=pixels;}
  const auto found=std::find_if(ui.equations.begin(),ui.equations.end(),[&](const auto& e){return e.source==source;});
  if(found!=ui.equations.end())return found->layout;
  // Retain shared geometry for a long section beyond the typesetter's small LRU.
  if(ui.equations.size()==64)ui.equations.clear();
  ui.equations.push_back({source,math.layout(source,pixels,true)});return ui.equations.back().layout;
}
void passages(NativeMath& math,TextbookUiState& ui,std::span<const BookPassage> content,float width,const char* id){
  ImGui::PushID(id);
  for(unsigned i=0;i<content.size();++i){
    const auto& p=content[i];ImGui::PushID(static_cast<int>(i));
    if(p.kind==BookPassage::Kind::Prose){ImGui::TextWrapped("%s",p.text.c_str());ImGui::Spacing();}
    else {
      // Use the current reading size explicitly: display mathematics scales with prose.
      const auto& layout=equation(math,ui,p.text);
      if(!layout.error.empty()){
        ImGui::TextWrapped("Equation source: %s",p.text.c_str());
      }else {
        const std::string number=*p.number?"("+std::string(p.number)+")":"";
        const float labelWidth=number.empty()?0:ImGui::CalcTextSize(number.c_str()).x+24;
        const float inset=8;
        const bool overflow=layout.width+labelWidth+2*inset>width;
        const float height=layout.height+2*inset+(overflow?ImGui::GetStyle().ScrollbarSize+4:0);
        // Only a wide equation scrolls horizontally; its surrounding prose still wraps.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{inset,inset});
        ImGui::BeginChild("Display equation",{width,height},ImGuiChildFlags_AlwaysUseWindowPadding,ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground);
        const auto start=ImGui::GetCursorScreenPos();
        const float room=std::max(layout.width,width-2*inset-labelWidth);
        const float x=start.x+std::max(0.f,(room-layout.width)*.5f);
        math.draw(layout,x,start.y,ImGui::GetColorU32(ImGuiCol_Text));
        if(!number.empty())ImGui::GetWindowDrawList()->AddText({start.x+room+12,start.y+std::max(0.f,(layout.height-ImGui::GetFontSize())*.5f)},ImGui::GetColorU32(ImGuiCol_TextDisabled),number.c_str());
        ImGui::Dummy({room+labelWidth,layout.height});ImGui::EndChild();ImGui::PopStyleVar();
      }
      ImGui::Spacing();
    }
    ImGui::PopID();
  }
  ImGui::PopID();
}
void figure(Textbook& book,TextbookUiState& ui,float width,const char* number=""){
  const auto card=matrixChapter()[book.view().section].card;
  const auto view=book.board().view();auto& boardUi=ui.boards[book.exerciseIndex()];
  ImGui::BeginChild("Reading figure",{width,255*static_cast<float>(book.view().textScale)},ImGuiChildFlags_Borders);
  drawMatrixBoardGrid("Matrix",view.working?view.current:view.given,view,boardUi,!view.working);ImGui::EndChild();
  if(*number)ImGui::TextWrapped("Figure %s. Card %03u, part (%c), %u by %u. %s.",number,card,'a'+view.example,view.given.rows,view.given.cols,view.working?"Your current matrix after row operations":"The supplied matrix before any row operations");
  else ImGui::TextWrapped("Card %03u / %s",card,view.working?"your current working matrix":"given matrix");
  ImGui::BeginDisabled(view.complete||view.blocked);
  if(ImGui::Button("Perform next pivot")){boardUi.pending={BoardActionKind::Step};boardUi.hasPending=true;}
  ImGui::EndDisabled();nextControl("Undo",width);
  ImGui::BeginDisabled(!view.steps);if(ImGui::Button("Undo")){boardUi.pending={BoardActionKind::Undo};boardUi.hasPending=true;}ImGui::EndDisabled();
  nextControl("Open full exercise",width);if(ImGui::Button("Open full exercise"))queue(ui,{BookActionKind::Exercise});
  ImGui::TextWrapped("%s",view.status.c_str());
}
void lesson(Textbook& book,TextbookUiState& ui,NativeMath& math,float width){
  static constexpr std::array<const char*,7> names{"","Definition","Proposition","Example","Figure","Exercise",""};
  static constexpr std::array<const char*,4> helpNames{"proof","hint","answer","solution"};
  const auto view=book.view();
  for(const auto& b:book.lessonView()){
    ImGui::PushID(b.id);ImGui::Spacing();
    const float blockTop=ImGui::GetCursorScreenPos().y-ImGui::GetWindowPos().y;
    const std::string title=*b.number?std::string(names[static_cast<unsigned>(b.kind)])+" "+b.number+" / "+b.title:b.title;
    heading(title.c_str());
    if(view.anchorRevision!=ui.seenAnchorRevision&&view.anchor==b.id){
      ImGui::SetScrollFromPosY(blockTop,0.f);ui.seenAnchorRevision=view.anchorRevision;ui.restoreFrames=0;
    }
    passages(math,ui,b.body,width,"body");
    if(b.kind==BookBlockKind::Figure){
      if(matrixChapter()[view.section].figure.kind!=BookFigureKind::None){
        if(ImGui::Button("Explore this figure"))ui.presentation=LessonPresentation::Figure;
        ImGui::TextWrapped("The live figure follows its example's matrix and row-operation history. Use Read + figure to keep it beside the explanation, or Figure only for a larger workspace.");
      }else figure(book,ui,width,b.number);
    }
    for(unsigned h=0;h<b.help.size();++h){
      const auto& help=b.help[h];if(!help.available)continue;
      ImGui::PushID(static_cast<int>(h));
      const std::string label=std::string(help.open?"Hide ":"Show ")+helpNames[h];
      if(ImGui::Button(label.c_str()))queue(ui,{BookActionKind::ToggleHelp,view.section,0,b.id,static_cast<BookHelp>(h)});
      if(help.open){ImGui::Spacing();passages(math,ui,help.passages,width,"disclosure");}
      ImGui::PopID();
    }
    if(!b.references.empty()){
      ImGui::TextDisabled("Refer back to");
      for(const auto& ref:b.references){
        // A wrapped link remains usable with 200% text in a narrow column.
        const auto size=ImGui::CalcTextSize(ref.label,nullptr,false,width-12);
        ImGui::PushID(ref.target);
        const auto start=ImGui::GetCursorScreenPos();
        if(ImGui::Selectable("##reference",false,0,{width,size.y+10}))queue(ui,{BookActionKind::OpenBlock,view.section,0,ref.target});
        ImGui::GetWindowDrawList()->AddText(nullptr,0,{start.x+5,start.y+5},ImGui::GetColorU32(ImVec4{.45f,.82f,.78f,1}),ref.label,nullptr,width-12);
        ImGui::PopID();
      }
    }
    ImGui::Spacing();ImGui::PopID();
  }
}
}
bool drawTextbook(Textbook& book,TextbookUiState& ui,NativeMath& math,SceneFrame& presented){
  applySystemLessonPending(book.systems(),ui.figure);
  applyMatrixBoardPending(book.board(),ui.boards[book.exerciseIndex()]);
  if(ui.hasPending){const auto result=book.dispatch(ui.pending);ui.message=result.accepted?"":result.reason;ui.hasPending=false;}
  const auto v=book.view();const auto& sections=matrixChapter();const auto& section=sections[v.section];
  if(!v.anchor.empty()&&v.anchorRevision!=ui.seenAnchorRevision)ui.presentation=LessonPresentation::Together;
  const auto send=[&](BookAction a){queue(ui,a);};
  const auto screen=ImGui::GetIO().DisplaySize;
  const bool hasProvider=v.page==BookPage::Section&&section.figure.kind!=BookFigureKind::None;
  const bool hasFigure=hasProvider&&v.mode==BookMode::Reading;
  auto layout=planLessonSpread({screen.x,screen.y,ui.showContents,hasFigure,ui.presentation,ui.readingFraction});
  bool objects=false;
  window("Textbook header",{layout.header.x,layout.header.y},{layout.header.width,layout.header.height});
  ImGui::TextUnformatted("PATHS / INTERACTIVE MATHEMATICS");ImGui::Spacing();
  if(ImGui::Button("Contents"))send({BookActionKind::Contents});ImGui::SameLine();
  if(ImGui::Button("Index"))send({BookActionKind::Index});ImGui::SameLine();
  if(ImGui::Button("Explore 3D objects"))objects=true;ImGui::SameLine();
  if(!hasFigure||screen.x>=1200){if(ImGui::Button(ui.showContents?"Hide contents":"Show contents"))ui.showContents=!ui.showContents;ImGui::SameLine();}
  ImGui::BeginDisabled(v.textScale<=.9001);if(ImGui::Button("A-"))send({BookActionKind::SetTextScale,0,std::max(.9,v.textScale-.1)});ImGui::EndDisabled();ImGui::SameLine();
  ImGui::BeginDisabled(v.textScale>=1.9999);if(ImGui::Button("A+"))send({BookActionKind::SetTextScale,0,std::min(2.0,v.textScale+.1)});ImGui::EndDisabled();ImGui::SameLine();ImGui::Text("%.0f%%",100*v.textScale);
  if(hasProvider){
    if(ImGui::Button("Read + figure")){ui.presentation=LessonPresentation::Together;if(v.mode!=BookMode::Reading)send({BookActionKind::Read});}ImGui::SameLine();
    if(ImGui::Button("Reading only")){ui.presentation=LessonPresentation::Reading;if(v.mode!=BookMode::Reading)send({BookActionKind::Read});}ImGui::SameLine();
    if(ImGui::Button("Figure only")){ui.presentation=LessonPresentation::Figure;if(v.mode!=BookMode::Reading)send({BookActionKind::Read});}
    ImGui::SameLine();if(ImGui::Button("Exercise"))send({BookActionKind::Exercise});
    if(layout.compact&&ui.presentation==LessonPresentation::Together){ImGui::SameLine();ImGui::TextDisabled("Widen for split view");}
  }
  ImGui::End();

  layout=planLessonSpread({screen.x,screen.y,ui.showContents,hasFigure,ui.presentation,ui.readingFraction});
  if(layout.showContents){
  window("Textbook contents",{layout.contents.x,layout.contents.y},{layout.contents.width,layout.contents.height});
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
  }

  const bool reading=v.page==BookPage::Section&&v.mode==BookMode::Reading&&layout.showReading;
  if(reading&&(!ui.wasReading||ui.displayedSection!=v.section))ui.restoreFrames=2;
  ui.wasReading=reading;ui.displayedSection=v.section;
  const bool followingAnchor=reading&&!v.anchor.empty()&&v.anchorRevision!=ui.seenAnchorRevision;
  if(layout.showReading){
  if(reading&&ui.restoreFrames&&!followingAnchor)ImGui::SetNextWindowScroll({0,static_cast<float>(v.scroll)});
  const std::string name=v.page==BookPage::Section?std::string(section.id)+(reading?" / reading":" / exercise"):(v.page==BookPage::Index?"Textbook index":"Textbook introduction");
  window(name.c_str(),{layout.reading.x,layout.reading.y},{layout.reading.width,layout.reading.height});
  ImGui::PushFont(nullptr,ImGui::GetFontSize()*static_cast<float>(v.textScale));
  const float available=ImGui::GetContentRegionAvail().x;
  const float column=std::min(available,ImGui::CalcTextSize("abcdefghijklmnopqrstuvwxyz").x/26*72);
  const float inset=v.mode==BookMode::Exercise&&v.page==BookPage::Section?0:std::max(0.f,(available-column)*.5f);
  if(inset>0)ImGui::Indent(inset);
  ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+column);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{ImGui::GetStyle().ItemSpacing.x,8*static_cast<float>(v.textScale)});
  switch(v.page){
    case BookPage::Contents:
      heading("An interactive textbook");
      ImGui::TextWrapped("Read an idea, explore its figure, and work through an exercise. The contents give you a sequence; the index lets you look up a term directly.");
      heading("Part IV / Linear Algebra");ImGui::TextUnformatted("Chapter 1 / Matrices and Elimination");
      ImGui::TextWrapped("Begin with entries and dimensions, then develop row operations, solution sets, pivoting, LU factors, sparsity and block elimination. This first chapter has eight sections, six connected exercise cards, and a practice lesson on solution sets.");
      if(ImGui::Button("Begin chapter",{-1,36}))send({BookActionKind::OpenSection,0});
      if(ImGui::Button("Continue from reading bookmark",{-1,36}))send({BookActionKind::Resume});
      heading("How to use a section");
      ImGui::TextWrapped("Sections bring together definitions, reasoning, worked examples and interactive figures. Section 1.2 introduces the numbered lesson format, with a proof and three practice questions whose hints, answers and solutions open independently. Reading and Exercise stay together, and changing sections preserves your board work during this session.");
      ImGui::TextWrapped("The reading bookmark remembers your section, scroll position and text size between launches. Opening a page never marks an exercise complete. Board attempts currently last for this session.");
      heading("The rest of the book");
      ImGui::TextWrapped("The other six parts establish the outline. Their chapters will be assembled using the same reading structure. The existing object collection remains available through Explore 3D objects.");
      break;
    case BookPage::Index:{
      heading("Index / Matrices and Elimination");ImGui::SetNextItemWidth(-1);ImGui::InputTextWithHint("##term","Find a term...",ui.search,sizeof(ui.search));
      unsigned matches=0;
      for(unsigned i=0;i<sections.size();++i)for(unsigned j=0;j<sections[i].terms.size();++j){const auto& term=sections[i].terms[j];
        if(!contains(term.name,ui.search)&&!contains(term.definition,ui.search))continue;
        ++matches;ImGui::PushID(static_cast<int>(i));ImGui::PushID(static_cast<int>(j));heading(term.name);ImGui::TextWrapped("%s",term.definition);
        if(ImGui::Button(sections[i].title))send(*term.blockId?BookAction{BookActionKind::OpenBlock,i,0,term.blockId}:BookAction{BookActionKind::OpenSection,i});
        ImGui::PopID();ImGui::PopID();
      }
      if(!matches)ImGui::TextUnformatted("No matching term in this chapter.");break;
    }
    case BookPage::Section:{
      ImGui::TextDisabled("PART IV / LINEAR ALGEBRA / CHAPTER 1");heading(section.title);
      ImGui::TextWrapped("%s",section.purpose);ImGui::Spacing();
      if(ImGui::Button("Reading"))send({BookActionKind::Read});
      char exerciseLabel[48];if(section.card)std::snprintf(exerciseLabel,sizeof(exerciseLabel),"Exercise / card %03u",section.card);else std::snprintf(exerciseLabel,sizeof(exerciseLabel),"Exercise / predict a solution set");
      nextControl(exerciseLabel,column);if(ImGui::Button(exerciseLabel))send({BookActionKind::Exercise});
      auto& board=book.board();auto& boardUi=ui.boards[book.exerciseIndex()];
      if(v.mode==BookMode::Exercise){
        ImGui::TextWrapped("%s",section.exercisePrompt);ImGui::Separator();
        ImGui::BeginChild("Section exercise",{0,0});
        if(section.figure.kind==BookFigureKind::AffinePlanes)drawSystemExercise(book.systems(),ui.figure,boardUi,math);else drawMatrixBoardContents(board,boardUi,true);
        ImGui::EndChild();
      }else {
        if(!section.lesson.empty())lesson(book,ui,math,column);
        else {
        for(const auto paragraph:section.explanation){ImGui::Spacing();ImGui::TextWrapped("%s",paragraph);}
        heading("Definitions");for(const auto& term:section.terms){ImGui::TextColored({.9f,.78f,.49f,1},"%s",term.name);ImGui::TextWrapped("%s",term.definition);ImGui::Spacing();}
        heading("Interactive figure");ImGui::TextWrapped("%s",section.figurePrompt);
        figure(book,ui,column);
        heading("Worked example");
        if(ImGui::CollapsingHeader(section.exampleTitle)){
          ImGui::TextDisabled("Separate teaching example / reveal is optional");
          for(unsigned i=0;i<section.exampleSteps.size();++i)ImGui::TextWrapped("%u. %s",i+1,section.exampleSteps[i]);
        }
        }
        heading("Your exercise");ImGui::TextWrapped("%s",section.exercisePrompt);
        heading("Source and conventions");ImGui::TextWrapped("%s",section.reference);
        ImGui::TextWrapped("These are authored teaching notes. The original problem pages and learner fields remain unchanged.");
      }
      break;
    }
  }
  if(!ui.message.empty()){ImGui::Separator();ImGui::TextWrapped("%s",ui.message.c_str());}
  ImGui::PopStyleVar();ImGui::PopTextWrapPos();if(inset>0)ImGui::Unindent(inset);ImGui::PopFont();
  if(reading&&!followingAnchor){
    if(ui.restoreFrames)--ui.restoreFrames;
    else if(std::abs(static_cast<double>(ImGui::GetScrollY())-v.scroll)>.25)
      static_cast<void>(book.dispatch({BookActionKind::RememberScroll,v.section,ImGui::GetScrollY()}));
  }
  ImGui::End();
  }

  if(layout.split){
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    window("Textbook pane divider",{layout.divider.x,layout.divider.y},{layout.divider.width,layout.divider.height});
    ImGui::InvisibleButton("Resize panes",{layout.divider.width,layout.divider.height});
    if(ImGui::IsItemHovered()||ImGui::IsItemActive())ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if(ImGui::IsItemActive())ui.readingFraction=std::clamp(ui.readingFraction+ImGui::GetIO().MouseDelta.x/(layout.reading.width+layout.figure.width),.3f,.7f);
    ImGui::End();ImGui::PopStyleVar();
  }
  if(layout.showFigure&&drawTextbookFigure(section.figure,book,ui.boards[book.exerciseIndex()],ui.figure,math,layout.figure,static_cast<float>(v.textScale)))presented=ui.figure.scene.frame();

  window("Textbook navigation",{layout.footer.x,layout.footer.y},{layout.footer.width,layout.footer.height});
  ImGui::BeginDisabled(v.page!=BookPage::Section||v.section==0);if(ImGui::Button("< Previous"))send({BookActionKind::Previous});ImGui::EndDisabled();ImGui::SameLine();
  ImGui::Text("Section %u of %zu",v.section+1,sections.size());ImGui::SameLine();
  ImGui::BeginDisabled(v.page!=BookPage::Section||v.section+1==sections.size());if(ImGui::Button("Next >"))send({BookActionKind::Next});ImGui::EndDisabled();
  ImGui::End();return objects;
}
} // namespace paths

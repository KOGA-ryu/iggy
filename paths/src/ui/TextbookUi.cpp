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
  ImGui::TextWrapped("%s",text);drawTextCopyMenu(text,{{"Copy heading",text}});ImGui::PopStyleColor();ImGui::Separator();
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
const NativeMath::Equation& equation(NativeMath& math,BookReadingUiState& ui,const std::string& source){
  const float pixels=ImGui::GetFontSize()*1.1f;
  if(pixels!=ui.equationPixels){ui.equations.clear();ui.equationPixels=pixels;}
  const auto found=std::find_if(ui.equations.begin(),ui.equations.end(),[&](const auto& e){return e.source==source;});
  if(found!=ui.equations.end())return found->layout;
  // Retain shared geometry for a long section beyond the typesetter's small LRU.
  if(ui.equations.size()==64)ui.equations.clear();
  ui.equations.push_back({source,math.layout(source,pixels,true)});return ui.equations.back().layout;
}
void passages(NativeMath& math,BookReadingUiState& ui,std::span<const BookPassage> content,float width,const char* id){
  ImGui::PushID(id);
  for(unsigned i=0;i<content.size();++i){
    const auto& p=content[i];ImGui::PushID(static_cast<int>(i));
    if(p.kind==BookPassage::Kind::Prose){ImGui::TextWrapped("%s",p.text.c_str());drawTextCopyMenu("copy",{{"Copy paragraph",p.text}});ImGui::Spacing();}
    else {
      // Use the current reading size explicitly: display mathematics scales with prose.
      const auto& layout=equation(math,ui,p.text);
      if(!layout.error.empty()){
        ++ui.fallbacks;ImGui::TextWrapped("Equation source: %s",p.text.c_str());drawTextCopyMenu("copy",{{"Copy equation (LaTeX)",p.text}});
      }else {
        const std::string number=p.number.empty()?"":"("+p.number+")";
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
        ImGui::Dummy({room+labelWidth,layout.height});drawTextCopyMenu("copy",{{"Copy equation (LaTeX)",p.text}});ImGui::EndChild();ImGui::PopStyleVar();
      }
      ImGui::Spacing();
    }
    ImGui::PopID();
  }
  ImGui::PopID();
}
void figure(Textbook& book,TextbookUiState& ui,float width,const char* number=""){
  const auto card=book.sections()[book.view().section].card;
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
  const auto view=book.view();
  for(const auto& b:book.lessonView()){
    if(view.anchorRevision!=ui.seenAnchorRevision&&view.anchor==b.id){
      ImGui::SetScrollFromPosY(ImGui::GetCursorScreenPos().y-ImGui::GetWindowPos().y,0.f);
      ui.seenAnchorRevision=view.anchorRevision;ui.restoreFrames=0;
    }
    const auto action=drawBookBlock(b,ui,math,width,[&]{
      if(book.sections()[view.section].figure.kind!=BookFigureKind::None){
        if(ImGui::Button("Explore this figure"))ui.presentation=LessonPresentation::Figure;
        ImGui::TextWrapped("The live figure follows the current teaching example. Use Read + figure to keep it beside the explanation, or Figure only for a larger workspace.");
      }else if(book.hasBoard())figure(book,ui,width,b.number);
    });
    if(action){auto a=*action;a.section=view.section;queue(ui,a);}
  }
}
void copyOption(const TextCopyOption& option){
  if(ImGui::MenuItem(option.label,nullptr,false,!option.text.empty())){
    const std::string text(option.text);ImGui::SetClipboardText(text.c_str());
  }
}
}
void drawTextCopyMenu(const char* id,std::initializer_list<TextCopyOption> options){
  if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))ImGui::SetTooltip("Right-click to copy text");
  if(ImGui::BeginPopupContextItem(id,ImGuiPopupFlags_MouseButtonRight)){
    ImGui::PushStyleColor(ImGuiCol_Text,{.35f,.85f,1,1});
    for(const auto& option:options)copyOption(option);
    ImGui::PopStyleColor();ImGui::EndPopup();
  }
}
TextCopyOption documentCopyText(const NativeMath::Document& doc,std::size_t placement){
  const auto& at=doc.placements.at(placement);const auto& part=doc.parts.at(at.part);
  const std::string_view source=doc.source;
  if(part.kind!=NativeMath::Document::Part::Kind::Text)
    return {part.kind==NativeMath::Document::Part::Kind::Source?"Copy source":"Copy equation (LaTeX)",source.substr(part.begin,part.end-part.begin)};
  // Preserve inline mathematics and original line endings in the whole paragraph.
  std::size_t begin=0,end=source.size();
  for(std::size_t line=0;line<source.size();){
    const auto newline=source.find('\n',line);const auto stop=newline==source.npos?source.size():newline;
    if(source.substr(line,stop-line).find_first_not_of(" \t\r")==source.npos){
      if(stop<at.begin)begin=stop+1;
      else {end=line;break;}
    }
    if(newline==source.npos)break;line=newline+1;
  }
  while(end>begin && (source[end-1]=='\n'||source[end-1]=='\r'))--end;
  return {"Copy paragraph",source.substr(begin,end-begin)};
}
void drawDocumentCopyMenu(const NativeMath::Document& doc,float x,float y){
  if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))ImGui::SetTooltip("Right-click to copy text");
  if(ImGui::BeginPopupContextItem("document-copy",ImGuiPopupFlags_MouseButtonRight)){
    ImGui::PushStyleColor(ImGuiCol_Text,{.35f,.85f,1,1});
    const auto mouse=ImGui::GetMousePosOnOpeningCurrentPopup();
    for(std::size_t i=0;i<doc.placements.size();++i){
      const auto& p=doc.placements[i];
      if(mouse.x>=x+p.x && mouse.x<x+p.x+p.width && mouse.y>=y+p.y && mouse.y<y+p.y+p.height){copyOption(documentCopyText(doc,i));break;}
    }
    copyOption({"Copy reading",doc.source});ImGui::PopStyleColor();ImGui::EndPopup();
  }
}
std::optional<BookAction> drawBookBlock(const BookBlockView& b,BookReadingUiState& ui,NativeMath& math,float width,const std::function<void()>& drawFigure){
  static constexpr std::array<const char*,7> names{"","Definition","Proposition","Example","Figure","Exercise",""};
  static constexpr std::array<const char*,4> helpNames{"proof","hint","answer","solution"};
  std::optional<BookAction> action;
  ImGui::PushID(b.id);ImGui::Spacing();
  const std::string title=*b.number?std::string(names[static_cast<unsigned>(b.kind)])+" "+b.number+" / "+b.title:b.title;
  heading(title.c_str());passages(math,ui,b.body,width,"body");
  if(b.kind==BookBlockKind::Figure && drawFigure)drawFigure();
  for(unsigned h=0;h<b.help.size();++h){
    const auto& help=b.help[h];if(!help.available)continue;
    ImGui::PushID(static_cast<int>(h));
    const std::string label=std::string(help.open?"Hide ":"Show ")+helpNames[h];
    if(ImGui::Button(label.c_str()))action=BookAction{BookActionKind::ToggleHelp,0,0,b.id,static_cast<BookHelp>(h)};
    if(help.open){ImGui::Spacing();passages(math,ui,help.passages,width,"disclosure");}
    ImGui::PopID();
  }
  if(!b.references.empty()){
    ImGui::TextDisabled("Refer back to");
    for(const auto& ref:b.references){
      const auto size=ImGui::CalcTextSize(ref.label.c_str(),nullptr,false,std::max(1.f,width-12));
      ImGui::PushID(ref.target.c_str());const auto start=ImGui::GetCursorScreenPos();
      if(ImGui::Selectable("##reference",false,0,{width,size.y+10}))action=BookAction{BookActionKind::OpenBlock,0,0,ref.target};
      ImGui::GetWindowDrawList()->AddText(nullptr,0,{start.x+5,start.y+5},ImGui::GetColorU32(ImVec4{.45f,.82f,.78f,1}),ref.label.c_str(),nullptr,std::max(1.f,width-12));
      ImGui::PopID();
    }
  }
  ImGui::Spacing();ImGui::PopID();return action;
}
bool drawTextbook(Textbook& book,TextbookUiState& ui,NativeMath& math,SceneFrame& presented,const TextbookContentUi* content){
  applySystemLessonPending(book.systems(),ui.figure);
  applyObjectLessonPending(book,ui.figure);
  if(book.hasBoard())applyMatrixBoardPending(book.board(),ui.boards.at(book.exerciseIndex()));
  if(ui.hasPending){const auto result=book.dispatch(ui.pending);if(result.accepted && content && content->action)content->action(ui.pending);ui.message=result.accepted?"":result.reason;ui.hasPending=false;}
  const auto v=book.view();const auto sections=book.sections();const auto& section=sections[v.section];
  if(!v.anchor.empty()&&v.anchorRevision!=ui.seenAnchorRevision)ui.presentation=LessonPresentation::Together;
  const auto send=[&](BookAction a){queue(ui,a);};
  const auto screen=ImGui::GetIO().DisplaySize;
  const bool hasProvider=v.page==BookPage::Section&&section.figure.kind!=BookFigureKind::None;
  const bool objectExercise=v.mode==BookMode::Exercise&&section.exercise==BookExerciseKind::Object;
  const bool hasFigure=hasProvider&&(v.mode==BookMode::Reading||objectExercise);
  const auto presentation=objectExercise?LessonPresentation::Figure:ui.presentation;
  auto* boardUi=book.hasBoard()?&ui.boards.at(book.exerciseIndex()):nullptr;
  auto layout=planLessonSpread({screen.x,screen.y,ui.showContents,hasFigure,presentation,ui.readingFraction});
  bool objects=false;
  window("Textbook header",{layout.header.x,layout.header.y},{layout.header.width,layout.header.height});
  ImGui::TextUnformatted("PATHS / INTERACTIVE MATHEMATICS");ImGui::Spacing();
  if(ImGui::Button("Contents"))send({BookActionKind::Contents});nextControl("Index",screen.x-16);
  if(ImGui::Button("Index"))send({BookActionKind::Index});nextControl(content?content->exitLabel:"Explore 3D objects",screen.x-16);
  if(ImGui::Button(content?content->exitLabel:"Explore 3D objects"))objects=true;nextControl(ui.showContents?"Hide contents":"Show contents",screen.x-16);
  if(!hasFigure||screen.x>=1200){if(ImGui::Button(ui.showContents?"Hide contents":"Show contents"))ui.showContents=!ui.showContents;nextControl("A-",screen.x-16);}
  ImGui::BeginDisabled(v.textScale<=.9001);if(ImGui::Button("A-"))send({BookActionKind::SetTextScale,0,std::max(.9,v.textScale-.1)});ImGui::EndDisabled();nextControl("A+",screen.x-16);
  ImGui::BeginDisabled(v.textScale>=1.9999);if(ImGui::Button("A+"))send({BookActionKind::SetTextScale,0,std::min(2.0,v.textScale+.1)});ImGui::EndDisabled();nextControl("200%",screen.x-16);ImGui::Text("%.0f%%",100*v.textScale);
  if(hasProvider){
    if(ImGui::Button("Read + figure")){ui.presentation=LessonPresentation::Together;if(v.mode!=BookMode::Reading)send({BookActionKind::Read});}nextControl("Reading only",screen.x-16);
    if(ImGui::Button("Reading only")){ui.presentation=LessonPresentation::Reading;if(v.mode!=BookMode::Reading)send({BookActionKind::Read});}nextControl("Figure only",screen.x-16);
    if(ImGui::Button("Figure only")){ui.presentation=LessonPresentation::Figure;if(v.mode!=BookMode::Reading)send({BookActionKind::Read});}
    nextControl("Exercise",screen.x-16);ImGui::BeginDisabled(!*section.exercisePrompt);if(ImGui::Button("Exercise"))send({BookActionKind::Exercise});ImGui::EndDisabled();
    if(layout.compact&&!objectExercise&&ui.presentation==LessonPresentation::Together){ImGui::SameLine();ImGui::TextDisabled("Widen for split view");}
  }
  ImGui::End();

  layout=planLessonSpread({screen.x,screen.y,ui.showContents,hasFigure,objectExercise?LessonPresentation::Figure:ui.presentation,ui.readingFraction});
  const auto contents=[&]{
  ImGui::TextUnformatted("CONTENTS");ImGui::Spacing();
  if(ImGui::Button("Continue reading",{-1,30}))send({BookActionKind::Resume});
  ImGui::TextWrapped("Bookmark: %s",section.title);ImGui::Separator();
  std::vector<std::string_view> parts;
  if(book.nativeCatalogue())for(const auto* part:textbookParts())parts.push_back(part);
  else for(const auto& item:sections)if(std::find(parts.begin(),parts.end(),item.part)==parts.end())parts.push_back(item.part);
  for(const auto part:parts){
    ImGui::PushID(part.data());
    if(ImGui::TreeNodeEx(part.data(),part==section.part?ImGuiTreeNodeFlags_DefaultOpen:0)){
      std::vector<std::string_view> chapters;
      for(const auto& item:sections)if(part==item.part && std::find(chapters.begin(),chapters.end(),item.chapter)==chapters.end())chapters.push_back(item.chapter);
      if(chapters.empty())ImGui::TextDisabled("Outline / chapters to come");
      for(const auto chapter:chapters){
        const bool only=chapters.size()==1;
        if(only)ImGui::TextWrapped("%s",chapter.data());
        if(only || ImGui::TreeNodeEx(chapter.data(),chapter==section.chapter?ImGuiTreeNodeFlags_DefaultOpen:0)){
          for(unsigned i=0;i<sections.size();++i)if(part==sections[i].part && chapter==sections[i].chapter){
            const bool selected=v.page==BookPage::Section&&v.section==i;
            const float width=ImGui::GetContentRegionAvail().x;
            const auto height=ImGui::CalcTextSize(sections[i].title,nullptr,false,width).y+10;
            ImGui::PushID(static_cast<int>(i));
            if(ImGui::Selectable("##section",selected,0,{width,height}))send({BookActionKind::OpenSection,i});
            const auto bounds=ImGui::GetItemRectMin();ImGui::GetWindowDrawList()->AddText(nullptr,0,{bounds.x+3,bounds.y+4},ImGui::GetColorU32(ImGuiCol_Text),sections[i].title,nullptr,width-6);
            ImGui::PopID();
          }
          if(!only)ImGui::TreePop();
        }
      }
      ImGui::TreePop();
    }
    ImGui::PopID();
  }
  };
  if(layout.showContents){
  window("Textbook contents",{layout.contents.x,layout.contents.y},{layout.contents.width,layout.contents.height});
  contents();
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
      if(!layout.showContents){contents();break;}
      heading("An interactive textbook");
      ImGui::TextWrapped("Read an idea, explore its figure, and work through an exercise. The contents give you a sequence; the index lets you look up a term directly.");
      heading(content?"Your library":"Part IV / Linear Algebra");if(!content)ImGui::TextUnformatted("Chapter 1 / Matrices and Elimination");
      if(content)ImGui::TextWrapped("Choose a subject and chapter in Contents, or look up a definition in Index. Lessons and their exercises use the same textbook pages. %zu sections are available.",sections.size());
      else ImGui::TextWrapped("Begin with entries and dimensions, then develop row operations, solution sets, factorizations and geometric maps. This chapter has %zu sections and %zu connected source exercise cards, alongside separate authored practice.",sections.size(),matrixCards().size());
      if(ImGui::Button("Begin chapter",{-1,36}))send({BookActionKind::OpenSection,content?v.section:0});
      if(ImGui::Button("Continue from reading bookmark",{-1,36}))send({BookActionKind::Resume});
      heading("How to use a section");
      if(content)ImGui::TextWrapped("Read the definitions, reasoning and worked examples, then select Exercise in that section. Hints, answers and solutions open separately. Reading does not complete a question, and returning to Reading keeps your checked work.");
      else ImGui::TextWrapped("Sections bring together definitions, reasoning, worked examples and interactive figures. Section 1.2 introduces the numbered lesson format, with a proof and three practice questions whose hints, answers and solutions open independently. Reading and Exercise stay together, and changing sections preserves your board work during this session.");
      ImGui::TextWrapped("The reading bookmark remembers your section, scroll position and text size between launches. Opening a page never marks an exercise complete.");
      if(!content){heading("The rest of the book");
      ImGui::TextWrapped("The other six parts establish the outline. Their chapters will be assembled using the same reading structure. The existing object collection remains available through Explore 3D objects.");}
      break;
    case BookPage::Index:{
      heading(content?"Index":"Index / Matrices and Elimination");ImGui::SetNextItemWidth(-1);ImGui::InputTextWithHint("##term","Find a term...",ui.search,sizeof(ui.search));
      unsigned matches=0;
      for(unsigned i=0;i<sections.size();++i)for(unsigned j=0;j<sections[i].terms.size();++j){const auto& term=sections[i].terms[j];
        if(!contains(term.name,ui.search)&&!contains(term.definition,ui.search))continue;
        ++matches;ImGui::PushID(static_cast<int>(i));ImGui::PushID(static_cast<int>(j));heading(term.name);ImGui::TextWrapped("%s",term.definition);drawTextCopyMenu("definition-copy",{{"Copy definition",term.definition}});
        if(ImGui::Button(sections[i].title))send(*term.blockId?BookAction{BookActionKind::OpenBlock,i,0,term.blockId}:BookAction{BookActionKind::OpenSection,i});
        ImGui::PopID();ImGui::PopID();
      }
      if(!matches)ImGui::TextUnformatted("No matching term in this chapter.");break;
    }
    case BookPage::Section:{
      ImGui::TextDisabled("%s / %s",section.part,section.chapter);heading(section.title);
      ImGui::TextWrapped("%s",section.purpose);drawTextCopyMenu("purpose-copy",{{"Copy paragraph",section.purpose}});ImGui::Spacing();
      if(ImGui::Button("Reading"))send({BookActionKind::Read});
      char exerciseLabel[64];
      switch(section.exercise){
        case BookExerciseKind::MatrixBoard:std::snprintf(exerciseLabel,sizeof(exerciseLabel),"Exercise / card %03u",section.card);break;
        case BookExerciseKind::Systems:std::snprintf(exerciseLabel,sizeof(exerciseLabel),"Exercise / predict a solution set");break;
        case BookExerciseKind::Object:std::snprintf(exerciseLabel,sizeof(exerciseLabel),"Exercise / manipulate the model");break;
        case BookExerciseKind::External:std::snprintf(exerciseLabel,sizeof(exerciseLabel),"Exercise");break;
      }
      nextControl(exerciseLabel,column);ImGui::BeginDisabled(!*section.exercisePrompt);if(ImGui::Button(exerciseLabel))send({BookActionKind::Exercise});ImGui::EndDisabled();
      if(v.mode==BookMode::Exercise){
        ImGui::TextWrapped("%s",section.exercisePrompt);drawTextCopyMenu("exercise-copy",{{"Copy question",section.exercisePrompt}});ImGui::Separator();
        ImGui::BeginChild("Section exercise",{0,0});
        switch(section.exercise){
          case BookExerciseKind::MatrixBoard:drawMatrixBoardContents(book.board(),*boardUi,true);break;
          case BookExerciseKind::Systems:drawSystemExercise(book.systems(),ui.figure,*boardUi,math);break;
          case BookExerciseKind::Object:break; // The full figure pane owns this workspace.
          case BookExerciseKind::External:if(content && content->exercise)content->exercise(v.section,column);break;
        }
        ImGui::EndChild();
      }else {
        if(!section.lesson.empty())lesson(book,ui,math,column);
        else if(content && content->reading)content->reading(v.section,column);
        else {
        for(unsigned i=0;i<section.explanation.size();++i){ImGui::PushID(static_cast<int>(i));const auto paragraph=section.explanation[i];ImGui::Spacing();ImGui::TextWrapped("%s",paragraph);drawTextCopyMenu("paragraph-copy",{{"Copy paragraph",paragraph}});ImGui::PopID();}
        heading("Definitions");for(const auto& term:section.terms){ImGui::TextColored({.9f,.78f,.49f,1},"%s",term.name);ImGui::TextWrapped("%s",term.definition);drawTextCopyMenu(term.name,{{"Copy definition",term.definition}});ImGui::Spacing();}
        heading("Interactive figure");ImGui::TextWrapped("%s",section.figurePrompt);drawTextCopyMenu("figure-copy",{{"Copy paragraph",section.figurePrompt}});
        figure(book,ui,column);
        heading("Worked example");
        if(ImGui::CollapsingHeader(section.exampleTitle)){
          ImGui::TextDisabled("Separate teaching example / reveal is optional");
          for(unsigned i=0;i<section.exampleSteps.size();++i){ImGui::PushID(static_cast<int>(i));ImGui::TextWrapped("%u. %s",i+1,section.exampleSteps[i]);drawTextCopyMenu("example-copy",{{"Copy step",section.exampleSteps[i]}});ImGui::PopID();}
        }
        }
        if(*section.exercisePrompt){heading("Your exercise");ImGui::TextWrapped("%s",section.exercisePrompt);drawTextCopyMenu("exercise-copy",{{"Copy question",section.exercisePrompt}});}
        heading("Source and conventions");ImGui::TextWrapped("%s",section.reference);drawTextCopyMenu("reference-copy",{{"Copy reference",section.reference}});
        if(!content)ImGui::TextWrapped("These are authored teaching notes. The original problem pages and learner fields remain unchanged.");
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
  if(layout.showFigure){
    if(content && content->figure)content->figure(v.section,layout.figure,static_cast<float>(v.textScale));
    else if(drawTextbookFigure(section.figure,book,boardUi,ui.figure,math,layout.figure,static_cast<float>(v.textScale)))presented=ui.figure.scene.frame();
  }

  window("Textbook navigation",{layout.footer.x,layout.footer.y},{layout.footer.width,layout.footer.height});
  ImGui::BeginDisabled(v.page!=BookPage::Section||v.section==0);if(ImGui::Button("< Previous"))send({BookActionKind::Previous});ImGui::EndDisabled();ImGui::SameLine();
  ImGui::Text("Section %u of %zu",v.section+1,sections.size());ImGui::SameLine();
  ImGui::BeginDisabled(v.page!=BookPage::Section||v.section+1==sections.size());if(ImGui::Button("Next >"))send({BookActionKind::Next});ImGui::EndDisabled();
  ImGui::End();return objects;
}
} // namespace paths

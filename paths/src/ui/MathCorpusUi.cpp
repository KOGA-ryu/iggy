#include "ui/MathCorpusUi.hpp"
#include <algorithm>
#include <set>
#include <imgui.h>

namespace paths {
std::unique_ptr<CorpusTextbook> makeCorpusTextbook(const MathCorpus& corpus,std::span<const CorpusStarter> questions) {
  auto result=std::make_unique<CorpusTextbook>();
  const auto text=[&](std::string s){result->strings.push_back(std::move(s));return result->strings.back().c_str();};
  struct Page {CorpusTextbook::Binding binding;std::size_t subject;std::optional<std::size_t> topic;};
  std::vector<Page> pages;std::set<std::size_t> attached;
  for(std::size_t i=0;i<corpus.entries.size();++i){
    const auto& e=corpus.entries[i];Page page{{i,{}},corpus.topics[e.topic].subject,e.topic};
    for(const auto& id:e.questions){const auto q=std::find_if(questions.begin(),questions.end(),[&](const auto& q){return q.id==id;});if(q!=questions.end()){page.binding.questions.push_back(q-questions.begin());attached.insert(q-questions.begin());}}
    pages.push_back(std::move(page));
  }
  for(std::size_t i=0;i<questions.size();++i)if(!attached.contains(i))pages.push_back({{{},{i}},questions[i].subject,questions[i].topic});
  std::stable_sort(pages.begin(),pages.end(),[](const auto& a,const auto& b){return std::pair{a.subject,a.topic?*a.topic+1:0}<std::pair{b.subject,b.topic?*b.topic+1:0};});
  for(const auto& page:pages){
    const auto* e=page.binding.entry?&corpus.entries[*page.binding.entry]:nullptr;
    const auto* q=page.binding.questions.empty()?nullptr:&questions[page.binding.questions.front()];
    BookSection section{text(e?e->id:"question."+q->id),text(e?e->title:q->title),text(e?(e->review?e->review->term.meaning:e->kind):q->question.description),{}, {},"","",{},
      q?"Choose a question, then work through its steps. Checked working stays until Next.":"",
      text(e?e->source+" : "+std::to_string(e->firstLine)+"-"+std::to_string(e->lastLine):"Question content and answer checks use the existing Paths question model."),0};
    section.exercise=BookExerciseKind::External;
    section.part=text(corpus.subjects[page.subject].title);
    section.chapter=text(page.topic?corpus.topics[*page.topic].title:"Starting questions");
    const CorpusEntry* reading=e;
    if(!reading && q)for(const auto& id:q->readingRefs){auto found=std::find_if(corpus.entries.begin(),corpus.entries.end(),[&](const auto& e){return e.id==id;});if(found!=corpus.entries.end()){reading=&*found;break;}}
    std::vector<BookBlock> blocks;
    if(reading && !reading->lesson.empty())blocks=reading->lesson;
    else if(!e && q) {
      // Legacy native references keep the original textbook definitions and
      // their cross references. No exercise answers are copied into a question.
      if(std::any_of(q->readingRefs.begin(),q->readingRefs.end(),[](const auto& id){return id.starts_with("rref.");}))
        for(const auto& b:rrefLesson())if(b.kind==BookBlockKind::Definition || b.kind==BookBlockKind::Proposition)blocks.push_back(b);
      for(const auto& id:q->readingRefs)for(const auto& native:matrixChapter())if(id==native.id && !native.explanation.empty()){
        BookBlock block{id,BookBlockKind::Introduction,"",native.title,{}};
        for(const auto* p:native.explanation)block.body.push_back({BookPassage::Kind::Prose,p,{}});
        blocks.push_back(std::move(block));
      }
    }
    if(!blocks.empty()){
      result->lessons.push_back(std::move(blocks));section.lesson=result->lessons.back();
      for(const auto& block:section.lesson)if(block.kind==BookBlockKind::Definition){
        const auto prose=std::find_if(block.body.begin(),block.body.end(),[](const auto& p){return p.kind==BookPassage::Kind::Prose;});
        section.terms.push_back({text(block.title),text(prose==block.body.end()?"Open the definition to read its notation.":prose->text),text(block.id)});
      }
    }
    if(section.terms.empty())section.terms.push_back({section.title,text(e&&e->review?e->review->term.meaning:"Open this section for its definitions, notation and exercises.")});
    if(e && e->figure)section.figure={BookFigureKind::Object,text(e->id+".figure"),section.title,text(e->figure->caption)};
    result->sections.push_back(section);result->bindings.push_back(page.binding);
  }
  if(result->sections.empty())throw std::runtime_error("The library has no textbook sections.");
  result->book=std::make_unique<Textbook>(result->sections);return result;
}
void refreshMathCorpusPreview(MathCorpusUiState& ui,const MathCorpus& corpus) {
  ui.document.entry.clear();ui.document.presented=false;ui.document.parameterControls.clear();
  if(ui.textbook && ui.practice){
    auto next=makeCorpusTextbook(corpus,ui.practice->questions());
    next->book->retainReading(*ui.textbook->book);
    ui.textbook=std::move(next);ui.textbookUi=TextbookUiState{};
  }
  ui.questionMatches.clear();++ui.previewRevision;
}
void drawMathCorpus(MathCorpusUiState& ui,const MathCorpus& corpus,bool blocked) {
  if(!ui.math || !ui.practice)return;
  blocked=blocked || ImGui::GetIO().AppFocusLost;ui.document.presented=false;
  if(!ui.textbook){
    ui.textbook=makeCorpusTextbook(corpus,ui.practice->questions());
    auto& book=*ui.textbook->book;
    if(ui.practice->selected())for(unsigned i=0;i<ui.textbook->bindings.size();++i){const auto& q=ui.textbook->bindings[i].questions;if(std::find(q.begin(),q.end(),*ui.practice->selected())!=q.end()){(void)book.dispatch({BookActionKind::OpenSection,i});(void)book.dispatch({BookActionKind::Contents});break;}}
    if(!ui.bookmarkPath.empty()){const auto result=readTextbookBookmark(ui.bookmarkPath,book);if(!result.accepted)ui.textbookUi.message=result.reason;}
    ui.lastBookmark=book.bookmark();
  }
  auto& book=*ui.textbook->book;
  const auto reading=[&](unsigned section,float width){
    const auto& binding=ui.textbook->bindings[section];const CorpusEntry* entry=binding.entry?&corpus.entries[*binding.entry]:nullptr;
    if(!entry && !binding.questions.empty())for(const auto& id:ui.practice->questions()[binding.questions.front()].readingRefs){const auto at=std::find_if(corpus.entries.begin(),corpus.entries.end(),[&](const auto& e){return e.id==id;});if(at!=corpus.entries.end()){entry=&*at;break;}}
    std::string source;
    if(entry){
      if(entry->review){const auto& r=*entry->review;source=r.term.meaning+"\n\n"+r.term.definition+"\n\nConditions\n"+r.conditions+"\n\nExample\n"+r.term.example;for(const auto& citation:r.sources)source+="\n\n"+citation.title+" / "+citation.locator+"\n"+citation.url;}
      else source=entry->body;
    }else if(!binding.questions.empty()){
      const auto& q=ui.practice->questions()[binding.questions.front()];source=q.question.description+"\n\n$$"+q.question.equation+"$$";
      if(q.question.support)source+="\n\n"+q.question.support->steps.front().definitions;
    }
    const auto& doc=ui.math->layoutDocument(source,width);
    const bool overflow=doc.width>width;
    if(overflow)ImGui::BeginChild("Reading equations",{width,doc.height+ImGui::GetStyle().ScrollbarSize+16},ImGuiChildFlags_None,ImGuiWindowFlags_HorizontalScrollbar);
    const auto at=ImGui::GetCursorScreenPos();
    ui.math->draw(doc,at.x,at.y,ImGui::GetColorU32(ImGuiCol_Text),IM_COL32(115,209,199,255),IM_COL32(255,199,77,255));ImGui::Dummy({doc.width,doc.height});drawDocumentCopyMenu(doc,at.x,at.y);
    if(overflow)ImGui::EndChild();
  };
  TextbookContentUi content;
  content.reading=reading;
  content.exercise=[&](unsigned section,float){
    const auto& indices=ui.textbook->bindings[section].questions;
    if(indices.empty())return;
    if(!ui.practice->selected() || std::find(indices.begin(),indices.end(),*ui.practice->selected())==indices.end())ui.practice->open(indices.front());
    ImGui::SetNextItemWidth(-1);
    if(indices.size()>1 && ImGui::BeginCombo("##section-question",ui.practice->questions()[*ui.practice->selected()].title.c_str())){
      for(const auto index:indices)if(ImGui::Selectable(ui.practice->questions()[index].title.c_str(),ui.practice->selected()==index))ui.practice->open(index);
      ImGui::EndCombo();
    }
    ui.questionMatches=indices;
    unsigned following=section+1;
    while(following<ui.textbook->bindings.size() && ui.textbook->bindings[following].questions.empty())++following;
    if(following<ui.textbook->bindings.size())ui.questionMatches.push_back(ui.textbook->bindings[following].questions.front());
    if(const auto next=drawCorpusQuestions(ui,corpus,blocked)){
      if(std::find(indices.begin(),indices.end(),*next)==indices.end()){
        (void)book.dispatch({BookActionKind::OpenSection,following});(void)book.dispatch({BookActionKind::Exercise});
      }
      ui.practice->open(*next);
    }
  };
  content.figure=[&](unsigned section,SceneViewport area,float scale){const auto entry=ui.textbook->bindings[section].entry;if(entry)drawDocumentFigure(corpus.entries[*entry],ui.document,area,scale,blocked);};
  content.action=[&](const BookAction& action){
    if(action.kind!=BookActionKind::ToggleHelp || !ui.practice->active())return;
    const auto& indices=ui.textbook->bindings[book.view().section].questions;
    if(!ui.practice->selected() || std::find(indices.begin(),indices.end(),*ui.practice->selected())==indices.end())return;
    for(const auto& block:book.lessonView())if(action.target==block.id && block.help[static_cast<unsigned>(action.help)].open)(void)recordQuestionReadingHelp(ui,action.help);
  };
  if(ui.livePreview || ui.importFailed)ui.textbookUi.message=ui.importMessage;
  if(blocked)ui.textbookUi.hasPending=false;
  ImGui::BeginDisabled(blocked);SceneFrame unused;
  if(drawTextbook(book,ui.textbookUi,*ui.math,unused,&content))ui.open=false;
  ImGui::EndDisabled();
  if(!blocked && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Escape,false) && !ImGui::IsAnyItemActive()){
    if(book.view().mode==BookMode::Exercise)(void)book.dispatch({BookActionKind::Read});
    else if(book.view().page==BookPage::Section)(void)book.dispatch({BookActionKind::Contents});else ui.open=false;
  }
  if(!ui.bookmarkPath.empty()){
    const auto saved=book.bookmark();if(saved!=ui.lastBookmark){const auto result=writeTextbookBookmark(ui.bookmarkPath,book);if(result.accepted)ui.lastBookmark=saved;else ui.textbookUi.message=result.reason;}
  }
}
} // namespace paths

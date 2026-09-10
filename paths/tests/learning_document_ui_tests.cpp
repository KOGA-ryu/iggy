#include "ui/MathCorpusUi.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <set>
#include <imgui.h>

using namespace paths;
namespace fm=iggy3d::first_move;
namespace {
void expect(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
unsigned sectionFor(const CorpusTextbook& book,std::string_view id) {
  for(unsigned i=0;i<book.sections.size();++i)if(id==book.sections[i].id)return i;
  throw std::runtime_error("Missing textbook section: "+std::string(id));
}
void catalogue(const MathCorpus& corpus,const std::vector<CorpusStarter>& questions) {
  expect(!ImGui::GetCurrentContext(),"Adapter checks must not create fonts or an ImGui context");
  auto adapter=makeCorpusTextbook(corpus,questions);auto& book=*adapter->book;
  expect(!book.nativeCatalogue() && book.sections().data()==adapter->sections.data(),"The native Textbook owns imported navigation, without a second screen model");
  std::set<std::string> ids;std::set<std::size_t> entries,linked;
  unsigned figures=0,lessons=0;
  for(unsigned i=0;i<adapter->sections.size();++i) {
    const auto& s=adapter->sections[i];const auto& binding=adapter->bindings[i];
    expect(ids.insert(s.id).second,"Each section has a unique stable identity");
    expect(book.dispatch({BookActionKind::OpenSection,i}).accepted && !book.hasBoard(),"Every imported section opens without binding an unrelated matrix exercise");
    expect(book.dispatch({BookActionKind::Exercise}).accepted==!binding.questions.empty(),"Only a section with questions enables Exercise");
    expect(book.dispatch({BookActionKind::Read}).accepted,"Exercise returns through the native Reading action");
    if(binding.entry){
      entries.insert(*binding.entry);const auto& e=corpus.entries[*binding.entry];
      expect(std::string(s.id)==e.id && std::string(s.title)==e.title,"Imported section retains its source identity and title");
      expect(std::string(s.part)==corpus.subjects[corpus.topics[e.topic].subject].title && std::string(s.chapter)==corpus.topics[e.topic].title,"Contents preserve subject/chapter ownership");
      expect(s.lesson.size()==e.lesson.size(),"Structured source blocks reach the native renderer without a different format");
      if(!e.lesson.empty())++lessons;
      for(unsigned b=0;b<s.lesson.size();++b){expect(s.lesson[b].id==e.lesson[b].id && s.lesson[b].body.size()==e.lesson[b].body.size(),"Numbered blocks are copied in order");}
      expect((s.figure.kind!=BookFigureKind::None)==e.figure.has_value(),"Only a registered source figure creates a figure pane");
      if(e.figure){auto object=instantiateDocumentFigure(*e.figure);MathObjectScene scene;(void)scene.publish(object.snapshot(),{0,0,600,400});expect(!scene.frame().vertices.empty(),"Figure adapter emits existing geometry");++figures;}
    }
    for(auto index:binding.questions){expect(index<questions.size(),"Exercise binding resolves");linked.insert(index);}
    for(const auto& b:book.lessonView())for(const auto& help:b.help)expect(!help.open && help.passages.empty(),"Opening reading does not expose optional answers");
  }
  expect(entries.size()==corpus.entries.size() && linked.size()==questions.size(),"All readings and all questions are reachable in the native book");
  for(const auto size:{std::pair{1440.f,860.f},std::pair{800.f,600.f},std::pair{360.f,480.f}})for(bool figure:{false,true}) {
    const auto layout=planLessonSpread({size.first,size.second,true,figure});
    for(const auto area:{layout.header,layout.reading,layout.footer})if(area.width>0)expect(area.x>=0 && area.y>=0 && area.x+area.width<=size.first && area.y+area.height<=size.second,"Native page regions fit the requested size");
  }
  const auto bookmark=book.bookmark();Textbook restored(adapter->sections);
  expect(restored.restoreBookmark(bookmark).accepted && restored.bookmark()==bookmark,"Whole-library reading bookmark round trips");
  std::cout<<"TEXTBOOK_CATALOGUE {\"sections\":"<<adapter->sections.size()<<",\"readings\":"<<entries.size()<<",\"questions\":"<<linked.size()<<",\"structured_lessons\":"<<lessons<<",\"figures\":"<<figures<<",\"bookmark_bytes\":"<<bookmark.size()<<",\"windows\":0,\"font_probes\":0}\n";
}
void reloadStateOnly() {
  expect(!ImGui::GetCurrentContext(),"State reconciliation is data only");
  MathCorpus corpus;corpus.subjects={{"math","Math"}};corpus.topics={{"chapter","Chapter",0}};
  CorpusEntry a;a.id="a";a.title="A";a.topic=0;CorpusEntry b=a;b.id="b";b.title="B";corpus.entries={a,b};
  CorpusPractice practice({});MathCorpusUiState ui;ui.practice=&practice;ui.textbook=makeCorpusTextbook(corpus,practice.questions());
  auto& book=*ui.textbook->book;
  expect(book.dispatch({BookActionKind::OpenSection,1}).accepted,"Open B");
  expect(book.dispatch({BookActionKind::RememberScroll,1,123}).accepted && book.dispatch({BookActionKind::SetTextScale,0,1.4}).accepted,"Native owner stores position and size");
  const auto saved=book.bookmark();ui.document.entry="b";
  CorpusEntry c=a;c.id="c";c.title="C";corpus.entries={c,b,a};
  refreshMathCorpusPreview(ui,corpus);
  expect(ui.document.entry.empty() && ui.previewRevision==1,"Preview invalidates old figure and widget bindings");
  const auto view=ui.textbook->book->view();
  expect(std::string(ui.textbook->sections[view.section].id)=="b" && view.scroll==123 && view.textScale==1.4,"Reordered content retains native reading position and size");
  Textbook grown(ui.textbook->sections);expect(grown.restoreBookmark(saved).accepted && grown.view().scroll==123,"A complete older library bookmark survives additions");
  const auto intact=grown.bookmark();
  expect(!grown.restoreBookmark(saved.substr(0,saved.rfind("b "))).accepted && grown.bookmark()==intact,"Truncation is rejected atomically");
  expect(!grown.restoreBookmark(saved+"a 0\n").accepted && grown.bookmark()==intact,"Duplicate positions are rejected atomically");
  corpus.entries={c,a};refreshMathCorpusPreview(ui,corpus);
  expect(ui.textbook->book->view().page==BookPage::Contents && ui.textbook->book->view().textScale==1.4,"Removing the active section returns safely to native Contents");
  expect(!ImGui::GetCurrentContext(),"Reconciliation created no context");
  std::cout<<"TEXTBOOK_RELOAD {\"reorder\":true,\"addition\":true,\"removal\":true,\"truncation_rejected\":true,\"windows\":0}\n";
}
void textbookReadingState(const std::filesystem::path& source) {
  expect(!ImGui::GetCurrentContext(),"Textbook integration check must remain data-only");
  auto corpus=loadMathCorpus(CORPUS_FIXTURE);auto questions=loadCorpusStarters(STARTER_FIXTURE,corpus);
  const auto loaded=importLearningDocuments(source,corpus,questions);expect(loaded.accepted,loaded.message);
  catalogue(corpus,questions);
  const auto at=std::find_if(questions.begin(),questions.end(),[](const auto& q){return q.id=="linear_book_worked";});expect(at!=questions.end(),"Reference question exists");
  const auto save=std::filesystem::temp_directory_path()/("paths-book-help-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".json");
  CorpusPractice practice(questions);practice.loadProgress(save);practice.open(at-questions.begin());
  MathCorpusUiState ui;ui.practice=&practice;ui.textbook=makeCorpusTextbook(corpus,questions);
  auto& book=*ui.textbook->book;const auto section=sectionFor(*ui.textbook,"linear_textbook_reading");
  expect(book.dispatch({BookActionKind::OpenSection,section}).accepted,"Open reference through the actual Textbook owner");
  expect(book.dispatch({BookActionKind::ToggleHelp,section,0,"worked",BookHelp::Hint}).accepted,"Native disclosure opens a real hint");
  const auto blocks=book.lessonView();const auto worked=std::find_if(blocks.begin(),blocks.end(),[](const auto& b){return std::string_view(b.id)=="worked";});
  expect(worked!=blocks.end() && worked->help[1].open && !worked->help[2].open && worked->help[3].passages.empty(),"Opening a hint does not expose an answer or full solution");
  expect(book.dispatch({BookActionKind::OpenBlock,section,0,"balance"}).accepted && book.view().anchor=="balance","Cross reference resolves to the same definition");
  expect(!book.dispatch({BookActionKind::ToggleHelp,section,0,"worked",BookHelp::Proof}).accepted,"Unavailable disclosure is rejected");
  expect(!book.dispatch({BookActionKind::OpenBlock,section,0,"missing"}).accepted,"Stale reference cannot navigate");
  const auto journalBefore=practice.active()->journal().size();
  expect(book.dispatch({BookActionKind::Exercise}).accepted && book.lessonView().empty() && book.dispatch({BookActionKind::Read}).accepted,"Native Exercise/Reading transition retains separate disclosure ownership");
  expect(practice.active()->journal().size()==journalBefore,"Reading navigation does not create question evidence");
  unsigned checks=0;
  for(unsigned level=0;level<4;++level) {
    fm::LayeredQuestionCommand c{fm::LayeredQuestionCommandKind::Support};c.support=practice.active()->supportView()->command;
    c.support.action=fm::SupportAction::SelectLevel;c.support.value=level;expect(practice.dispatch(c),"Support level changes through the owner");
    const auto working=practice.active()->visibleWorking();const auto nodes=practice.active()->currentRun().support->nodes.size();
    const auto reading=practice.active()->supportView()->reading;const auto selectedHelp=practice.active()->supportView()->help;
    for(const auto h:{BookHelp::Proof,BookHelp::Hint,BookHelp::Answer,BookHelp::Solution}) {
      expect(recordQuestionReadingHelp(ui,h),"Textbook help uses existing guarded guidance commands at every support level");
      expect(practice.active()->visibleWorking()==working && !practice.active()->currentRun().completed && practice.active()->currentRun().support->nodes.size()==nodes,"Reading never advances or grades the question");
      expect(practice.active()->supportView()->reading==reading && practice.active()->supportView()->help==selectedHelp,"A separate example reveal never opens the active question's solution or changes its help tab");
      ++checks;
    }
  }
  expect(!recordQuestionReadingHelp(ui,BookHelp::Count),"Unknown reading guidance is rejected");
  const auto journal=practice.active()->journal().size();
  for(unsigned value:{0U,5U}) {
    fm::LayeredQuestionCommand c{fm::LayeredQuestionCommandKind::Support};c.support=practice.active()->supportView()->command;
    c.support.action=fm::SupportAction::ReadReference;c.support.value=value;
    expect(!practice.dispatch(c) && practice.active()->journal().size()==journal,"Invalid reference exposure creates no history");
  }
  fm::LayeredQuestionCommand stale{fm::LayeredQuestionCommandKind::Support};stale.support=practice.active()->supportView()->command;
  stale.support.action=fm::SupportAction::ReadReference;stale.support.value=1;--stale.support.revision;
  expect(!practice.dispatch(stale) && practice.active()->journal().size()==journal,"Stale textbook commands cannot mark a newer attempt");
  const auto exposure=practice.active()->currentRun().support->exposure;
  expect((exposure&32) && (exposure&64) && (exposure&256),"Reference definitions, hints and solutions are recorded separately");
  const auto summary=fm::summarizeLayeredQuestionRun(practice.active()->currentRun());
  expect(summary.assisted && !summary.shownAnswers && !(exposure&16),"A reference example counts as guidance without claiming the question's answer was shown");
  practice.saveProgress();CorpusPractice restored(questions);restored.loadProgress(save);
  expect(restored.active() && restored.active()->currentRun().support->exposure==exposure && !restored.active()->currentRun().completed,"Guidance history survives reopening without completing the question");
  std::filesystem::remove(save);std::filesystem::remove(save.string()+".lock");
  expect(!ImGui::GetCurrentContext(),"No ImGui context or fonts were initialized");
  std::cout<<"TEXTBOOK_READING_STATE {\"guidance_checks\":"<<checks<<",\"disclosures_independent\":true,\"references\":true,\"save_replay\":true,\"windows\":0,\"font_probes\":0}\n";
}
}
int main(int argc,char** argv) {
  try {
    if(argc==3 && std::string_view(argv[1])=="--textbook-reading-state"){textbookReadingState(argv[2]);return 0;}
    if(argc==2 && std::string_view(argv[1])=="--reload-state-only"){reloadStateOnly();return 0;}
    expect(argc==1 || (argc==3 && std::string_view(argv[1])=="--published-store"),"Use --published-store FOLDER or no arguments");
    auto corpus=loadMathCorpus(CORPUS_FIXTURE);auto questions=loadCorpusStarters(STARTER_FIXTURE,corpus);
    if(argc==3)for(const auto* name:{"matrix_reasoning.json","linear_support.json"}){
      auto more=loadCorpusStarters(std::filesystem::path(CORPUS_FIXTURE).parent_path()/name,corpus);questions.insert(questions.end(),more.begin(),more.end());
    }
    const auto result=argc==3?importLearningStore(argv[2],corpus,questions):importLearningDocuments(DOCUMENT_FIXTURE,corpus,questions);
    expect(result.accepted,result.message);catalogue(corpus,questions);reloadStateOnly();
    expect(!ImGui::GetCurrentContext(),"No test initializes the renderer or fonts");
  } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

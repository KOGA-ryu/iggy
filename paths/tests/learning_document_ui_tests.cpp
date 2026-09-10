#include "ui/MathCorpusUi.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <imgui.h>

using namespace paths;
namespace fm=iggy3d::first_move;
namespace {
std::filesystem::path publishedStore;
void expect(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
std::vector<CorpusStarter> bank(MathCorpus& corpus) {
  auto q=loadCorpusStarters(STARTER_FIXTURE,corpus);
  if(!publishedStore.empty())for(const auto* name:{"matrix_reasoning.json","linear_support.json"}) {
    auto next=loadCorpusStarters(std::filesystem::path(CORPUS_FIXTURE).parent_path()/name,corpus);
    q.insert(q.end(),std::make_move_iterator(next.begin()),std::make_move_iterator(next.end()));
  }
  const auto report=publishedStore.empty()?importLearningDocuments(DOCUMENT_FIXTURE,corpus,q):importLearningStore(publishedStore,corpus,q);
  expect(report.accepted,report.message);return q;
}
void reloadStateOnly() {
  expect(!ImGui::GetCurrentContext(),"State reconciliation must not create an ImGui context or fonts");
  MathCorpus corpus;corpus.subjects={{"b","B"},{"a","A"}};corpus.topics={{"tb","TB",0},{"ta","TA",1}};
  CorpusEntry b;b.id="b";b.title="B";b.topic=0;CorpusEntry a;a.id="a";a.title="A";a.topic=1;corpus.entries={b,a};
  MathCorpusUiState ui;ui.entry=0;ui.shown=0;ui.subject=0;ui.topic=0;ui.focusDocument=true;ui.scroll=123;ui.horizontal=7;ui.top=false;
  ui.trail={{0,12,false},{1,24,true}};ui.document.entry="a";ui.document.textScale=1.4f;ui.document.helpMasks={15};ui.document.anchor="old";
  DocumentRemap remap{{1,0},{1,0},{1,std::nullopt}};
  refreshMathCorpusPreview(ui,corpus,remap);
  expect(ui.entry==1 && ui.shown==1 && ui.subject==1 && ui.topic==1 && ui.focusDocument,"Stable selection remaps across reordered catalogues");
  expect(ui.restoreScroll==123 && ui.restoreHorizontal==7 && !ui.top && ui.trail.size()==1 && ui.trail[0].entry==1,"Reading position and surviving bookmarks are preserved");
  expect(ui.document.entry.empty() && ui.document.helpMasks.empty() && ui.document.anchor.empty() && ui.document.textScale==1.4f,"Reload invalidates lesson/figure bindings and hidden disclosures while keeping text size");
  expect(!ui.refresh && ui.previewRevision==1 && ui.matches==std::vector<std::size_t>{1},"Visible lists refresh through the same corpus query");
  refreshMathCorpusPreview(ui,corpus,DocumentRemap{{std::nullopt,std::nullopt},{std::nullopt,std::nullopt},{std::nullopt,std::nullopt}});
  expect(!ui.focusDocument && !ui.subject && !ui.topic && ui.top && ui.previewRevision==2,"Removed selection falls back safely to browsing");
  expect(!ImGui::GetCurrentContext(),"Reconciliation stayed data-only");
  std::cout<<"LIVE_DOCUMENT_UI_STATE {\"identity_remap\":true,\"scroll_retained\":true,\"bindings_invalidated\":true,\"imgui_contexts\":0,\"font_probes\":0,\"windows\":0}\n";
}
void textbookReadingState(const std::filesystem::path& source) {
  expect(!ImGui::GetCurrentContext(),"Textbook integration check must remain data-only");
  auto corpus=loadMathCorpus(CORPUS_FIXTURE);auto questions=loadCorpusStarters(STARTER_FIXTURE,corpus);
  const auto loaded=importLearningDocuments(source,corpus,questions);expect(loaded.accepted,loaded.message);
  const auto at=std::find_if(questions.begin(),questions.end(),[](const auto& q){return q.id=="linear_book_worked";});
  expect(at!=questions.end(),"Reference question exists");
  const auto entry=std::find_if(corpus.entries.begin(),corpus.entries.end(),[](const auto& e){return e.id=="linear_textbook_reading";});
  expect(entry!=corpus.entries.end(),"Reference textbook exists");
  const auto save=std::filesystem::temp_directory_path()/("paths-book-help-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".json");
  CorpusPractice practice(questions);practice.loadProgress(save);practice.open(at-questions.begin());
  MathCorpusUiState ui;ui.practice=&practice;
  refreshQuestionReading(ui,*at,*practice.active(),true);
  expect(ui.readingOpen,"Learn opens a linked textbook automatically");
  ui.questionReading.entry=entry->id;ui.questionReading.textScale=1.4f;
  BookAction hint{BookActionKind::ToggleHelp,0,0,"worked",BookHelp::Hint};
  expect(applyDocumentReadingAction(ui.questionReading,*entry,hint),"Shared reading action opens a real hint");
  const auto blocks=bookLessonView(entry->lesson,ui.questionReading.helpMasks);
  const auto worked=std::find_if(blocks.begin(),blocks.end(),[](const auto& b){return std::string_view(b.id)=="worked";});
  expect(worked->help[1].open && !worked->help[2].open && !worked->help[3].open && worked->help[3].passages.empty(),"Opening a hint does not expose an answer or full solution");
  expect(applyDocumentReadingAction(ui.questionReading,*entry,{BookActionKind::OpenBlock,0,0,"balance"}) && ui.questionReading.anchor=="balance","Cross reference resolves to the existing definition");
  expect(!applyDocumentReadingAction(ui.questionReading,*entry,{BookActionKind::ToggleHelp,0,0,"worked",BookHelp::Proof}),"Unavailable disclosure cannot be opened");
  expect(!applyDocumentReadingAction(ui.questionReading,*entry,{BookActionKind::OpenBlock,0,0,"missing"}),"Stale reference cannot navigate");
  const auto masks=ui.questionReading.helpMasks;
  refreshQuestionReading(ui,*at,*practice.active(),true);
  expect(ui.questionReading.helpMasks==masks && ui.questionReading.anchor=="balance","Ordinary frames retain disclosure and reference state");
  unsigned checks=0;
  for(unsigned level=0;level<4;++level) {
    fm::LayeredQuestionCommand c{fm::LayeredQuestionCommandKind::Support};c.support=practice.active()->supportView()->command;
    c.support.action=fm::SupportAction::SelectLevel;c.support.value=level;expect(practice.dispatch(c),"Support level changes through the owner");
    refreshQuestionReading(ui,*at,*practice.active(),true);
    expect(ui.readingOpen==(level==0),"Only Learn automatically opens the textbook");
    if(level)expect(ui.questionReading.helpMasks.empty() && ui.questionReading.anchor.empty() && ui.questionReading.textScale==1.4f,"Less guidance closes reveals but retains the user's reading size");
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
struct Harness {
  MathCorpus corpus=loadMathCorpus(CORPUS_FIXTURE);
  CorpusPractice practice{bank(corpus)};
  std::unique_ptr<NativeMath> math;
  MathCorpusUiState ui;
  explicit Harness(ImVec2 size) {
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize=size;io.DeltaTime=1.0F/60;
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendFlags|=ImGuiBackendFlags_RendererHasTextures|ImGuiBackendFlags_RendererHasVtxOffset;
    math=std::make_unique<NativeMath>(MATH_RESOURCES);ui.math=math.get();ui.practice=&practice;ui.open=true;frame(3);
  }
  ~Harness(){math.reset();ImGui::DestroyContext();}
  void frame(int count=1) {
    for(int i=0;i<count;++i) {
      ImGui::NewFrame();drawMathCorpus(ui,corpus,false);ImGui::Render();
      // CPU-only font acknowledgement. No renderer, native window or image output.
      for(auto* t:ImGui::GetPlatformIO().Textures) {
        if(t->Status==ImTextureStatus_WantCreate || t->Status==ImTextureStatus_WantUpdates){t->SetTexID(static_cast<ImTextureID>(t->UniqueID+1));t->SetStatus(ImTextureStatus_OK);}
        else if(t->Status==ImTextureStatus_WantDestroy){t->SetTexID(ImTextureID_Invalid);t->SetStatus(ImTextureStatus_Destroyed);}
      }
      for(const auto* list:ImGui::GetDrawData()->CmdLists) {
        for(const auto& v:list->VtxBuffer)expect(std::isfinite(v.pos.x) && std::isfinite(v.pos.y),"Finite UI coordinates");
        for(const auto& c:list->CmdBuffer)if(c.ElemCount)expect(c.GetTexID()!=ImTextureID_Invalid,"Typesetting has an acknowledged atlas");
      }
      expect(ui.document.fallbacks==0 && ui.readingFallbacks==0,"Imported lesson and help notation typesets");
    }
  }
  NotationBounds control(CorpusControl c)const{return ui.controls[static_cast<std::size_t>(c)];}
  void visible(NotationBounds b,const std::string& name)const {
    const auto size=ImGui::GetIO().DisplaySize;
    expect(b.available && b.x>=0 && b.y>=0 && b.x+b.width<=size.x+1 && b.y+b.height<=size.y+1,
      name+" fits "+std::to_string(static_cast<int>(size.x))+"x"+std::to_string(static_cast<int>(size.y))+" ("+std::to_string(b.x)+","+std::to_string(b.y)+","+std::to_string(b.width)+","+std::to_string(b.height)+")");
  }
  void click(NotationBounds b,const std::string& name="Input") {
    visible(b,name);auto& io=ImGui::GetIO();io.AddMousePosEvent(b.x+b.width/2,b.y+b.height/2);frame();
    io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame(3);
  }
  void lesson(const char* search) {
    click(control(CorpusControl::Search),"Title search");ImGui::GetIO().AddInputCharactersUTF8(search);frame(3);
    expect(ui.matches.size()==1 && corpus.entries[*ui.entry].document,"Typed search finds the imported lesson in the existing ToC");
    click(control(CorpusControl::OpenDocument),"Open lesson");expect(ui.focusDocument,"Document uses the lesson workspace");
    expect(ui.document.reading.available,"Reading is presented beside or above the diagram");
    expect(ui.document.questionLinks.size()==1,"Authored practice reference becomes a real button");
  }
  void write(const std::string& text) {
    click(ui.supportEditor,"Written answer");ImGui::GetIO().AddInputCharactersUTF8(text.c_str());frame(2);
    expect(practice.active()->supportView()->draft==text,"Written input reaches the existing question owner");
  }
};
void prepared(ImVec2 size,bool physics) {
  Harness h(size);h.lesson(physics?"Reading a rotating phasor":"The boundary of an animal cell");
  if(physics) {
    h.visible(h.ui.document.viewport,"3D viewport");expect(h.ui.document.presented,"Named diagram is submitted for scene rendering");
    const auto& frame=h.ui.document.scene.frame();expect(!frame.vertices.empty() && !frame.indices.empty(),"Existing 3D object emits real geometry");
    for(const auto& v:frame.vertices)expect(std::isfinite(v.position[0]) && std::isfinite(v.position[1]) && std::isfinite(v.position[2]),"Finite 3D vertices");
    const auto projected=h.ui.document.scene.project({1,0,0});const auto body=h.corpus.entries[*h.ui.entry].body;
    const auto viewport=h.ui.document.viewport;auto& io=ImGui::GetIO();
    io.AddMousePosEvent(viewport.x+viewport.width*.5F,viewport.y+viewport.height*.5F);h.frame();io.AddMouseButtonEvent(0,true);h.frame();
    io.AddMousePosEvent(viewport.x+viewport.width*.5F+20,viewport.y+viewport.height*.5F+10);h.frame(2);io.AddMouseButtonEvent(0,false);h.frame(3);
    const auto moved=h.ui.document.scene.project({1,0,0});expect(std::abs(moved.x-projected.x)+std::abs(moved.y-projected.y)>.1F,"Actual pointer drag orbits the diagram");
    expect(h.corpus.entries[*h.ui.entry].body==body && !h.practice.active(),"Exploring the diagram does not create or alter question evidence");
  } else expect(!h.ui.document.presented,"A text-only subject needs no diagram implementation");
  h.click(h.ui.document.questionLinks.front().second,"Authored question link");
  expect(h.ui.questions && h.ui.focusQuestion && !h.ui.document.presented,"Lesson opens its own question in the existing solver and releases the diagram");
  expect(h.practice.active()->content().id==(physics?"document_sine_question":"document_membrane_question"),"Practice link resolves by stable ID");
  const auto pinned=h.ui.questionInk;
  h.click(h.control(CorpusControl::Method),"Inline method reading");expect(!h.ui.readingSource.empty(),"@read resolves to authored prose");
  h.click(h.control(CorpusControl::Method));
  while(!h.practice.active()->currentRun().completed) {
    const auto& s=*h.practice.active();const auto& step=s.content().steps[s.currentRun().currentStep];const auto correct=fm::firstAcceptedOption(step);
    const auto working=std::string(s.visibleWorking());h.click(h.ui.answerTiles[(correct+1)%step.options.size()],"Wrong imported choice");
    expect(s.visibleWorking()==working,"Wrong imported choice keeps the working");h.click(h.ui.answerTiles[correct],"Correct imported choice");
    expect(h.ui.questionInk.x==pinned.x && h.ui.questionInk.y==pinned.y,"Given stays pinned through imported solving steps");
  }
  h.frame(3);expect(h.practice.active()->currentRun().completed,"Result waits for explicit navigation");
  h.click(h.control(CorpusControl::FocusQuestion),"Browse questions");h.click(h.control(CorpusControl::Questions),"Return to definitions");
  expect(h.ui.focusDocument && !h.ui.questions && h.practice.active()->currentRun().completed,"Returning to the lesson preserves completed work");
  if(physics)expect(h.ui.document.presented,"Diagram returns with its lesson");
}
void linear(ImVec2 size) {
  Harness h(size);h.lesson("Keeping an equation balanced");h.click(h.ui.document.questionLinks.front().second);
  expect(h.practice.active()->content().id=="document_balance_question","Imported linear workflow selected");
  for(unsigned level=0;level<4;++level) {
    if(level)h.click(h.control(CorpusControl::ReplayStarter),"Again");
    h.click(h.ui.supportLevels[level],"Support level");
    if(level<2)for(std::size_t step=0;step<2;++step) {
      if(!level) {
        const auto& s=h.practice.active()->content().steps[step];h.click(h.ui.answerTiles[fm::firstAcceptedOption(s)],"Imported Learn choice");
      } else {h.write(step?"5":"20");h.click(h.control(CorpusControl::CheckWork),"Imported Practice blank");}
    } else {h.write("4x=20\nx=5");h.click(h.control(CorpusControl::CheckWork),"Imported written work");}
    expect(h.practice.active()->currentRun().completed,"Imported workflow completes at every support level");
    h.visible(h.ui.currentWorking,"Finished working");h.frame(3);
  }
}
void matrix(ImVec2 size) {
  const bool published=!publishedStore.empty();
  Harness h(size);h.lesson(published?"Matrix foundations: two equations":"Solving two equations with matrix rows");h.click(h.ui.document.questionLinks.front().second);
  expect(h.practice.active()->content().id==(published?"published_matrix_question":"document_matrix_question"),"Document opens its matrix question");
  const auto given=h.practice.active()->supportView()->given;
  for(unsigned level=0;level<4;++level) {
    if(level)h.click(h.control(CorpusControl::ReplayStarter),"Again keeps matrix run");
    h.click(h.ui.supportLevels[level],"Matrix support level");
    const auto pinned=h.ui.questionInk;
    if(level<2)for(std::size_t step=0;step<3;++step) {
      const auto& s=h.practice.active()->content().steps[step];const auto correct=fm::firstAcceptedOption(s);
      if(!level) {
        expect(!h.ui.readingSource.empty(),"Learn gives the full row explanation in place");
        const auto before=h.practice.active()->supportView()->working;
        h.click(h.ui.answerTiles[(correct+1)%s.options.size()],"Wrong symbolic row operation");
        expect(h.practice.active()->supportView()->working==before,"Wrong row choice keeps the cyan working");
        h.click(h.ui.answerTiles[correct],"Correct symbolic row operation");
      } else {
        expect(h.practice.active()->supportView()->responseCue.find('?')!=std::string::npos,"Practice formats the multiplier/divisor as a symbolic blank");
        const auto text=h.practice.active()->content().support->steps[step].responses[correct];
        h.write(text);h.click(h.control(CorpusControl::CheckWork),"Check matrix operand");
      }
      expect(h.ui.questionInk.x==pinned.x && h.ui.questionInk.y==pinned.y,"Original matrix remains beside the working at every step");
    } else {
      expect(h.ui.readingSource.empty() && h.ui.answerTiles.empty(),"Written modes hide prepared teaching and choices");
      if(level==3)expect(h.practice.active()->supportView()->prompt.empty(),"Write starts with just the problem and empty working area");
      h.write(published?"[1, 1 | 5] [0, -3 | -9]\n[1, 1 | 5] [0, 1 | 3]\n[1, 0 | 2] [0, 1 | 3]":"[1, 1 | 3] [0, -3 | -6]\n[1, 1 | 3] [0, 1 | 2]\n[1, 0 | 1] [0, 1 | 2]");
      h.click(h.control(CorpusControl::CheckWork),"Check matrix working");
    }
    expect(h.practice.active()->currentRun().completed && h.practice.active()->supportView()->given==given,"All four UI routes finish with the unchanged original matrix");
    h.visible(h.ui.questionInk,"Gold given matrix");h.visible(h.ui.currentWorking,"Green finished matrix");h.frame(4);
    expect(h.practice.active()->currentRun().completed,"Green matrix result waits for Next");
  }
  h.click(h.control(CorpusControl::UndoWork),"Undo completed matrix");
  expect(!h.practice.active()->currentRun().completed && !h.practice.active()->supportView()->draft.empty(),"Real Undo retains the written matrix draft and prior branch");
}
}
int main(int argc,char** argv) {
  try {
    if(argc==3 && std::string_view(argv[1])=="--textbook-reading-state"){textbookReadingState(argv[2]);return 0;}
    if(argc==2 && std::string_view(argv[1])=="--reload-state-only"){reloadStateOnly();return 0;}
    if(argc==3 && std::string_view(argv[1])=="--published-store") {
      publishedStore=argv[2];for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}})matrix(size);
      std::cout<<"PUBLISHED_DOCUMENT_UI {\"matrix_routes\":12,\"sizes\":[[1440,860],[800,600],[360,480]],\"fallbacks\":0,\"native_windows\":0,\"captures\":0}\n";return 0;
    }
    expect(argc==1,"Use --published-store FOLDER or no arguments");
    for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}){prepared(size,true);prepared(size,false);linear(size);matrix(size);}
    std::cout<<"DOCUMENT_UI {\"solving_routes\":30,\"matrix_routes\":12,\"diagram_orbit_routes\":3,\"sizes\":[[1440,860],[800,600],[360,480]],\"fallbacks\":0,\"native_windows\":0,\"captures\":0}\n";
  } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

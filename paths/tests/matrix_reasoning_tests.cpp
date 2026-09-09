#include "ui/MathCorpusUi.hpp"
#include "runtime/textbook/Textbook.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <imgui.h>
#include <nlohmann/json.hpp>

using namespace paths;
namespace fm=iggy3d::first_move;
namespace {
void expect(bool yes,const std::string& why){if(!yes)throw std::runtime_error(why);}
std::string read(const std::filesystem::path& path){std::ifstream in(path);return {std::istreambuf_iterator<char>(in),{}};}
void answer(CorpusPractice& p,bool correct) {
  const auto& step=p.active()->content().steps[p.active()->currentRun().currentStep];
  const auto option=std::find_if(step.options.begin(),step.options.end(),[&](const auto& o){return fm::acceptsOption(step,&o-step.options.data())==correct;});
  expect(option!=step.options.end(),"A tile of the requested kind exists");
  expect(p.dispatch(fm::LayeredQuestionCommand::submitOption(option->id)),"Canonical question owner accepts the input");
  if(correct)expect(p.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Correct answer advances through the canonical owner");
}
void model(const MathCorpus& corpus,const std::vector<CorpusStarter>& bank,const std::filesystem::path& folder) {
  expect(bank.size()==8,"Eight follow-up questions");std::size_t steps=0,wrong=0;CorpusPractice p(bank);
  for(std::size_t i=0;i<bank.size();++i) {
    p.open(i);expect(bank[i].level=="practice" && bank[i].topic,"Practice question has a real chapter");
    for(const auto& ref:bank[i].readingRefs) {
      bool found=false;
      for(const auto& b:rrefLesson())if(ref==b.id)found=b.kind==BookBlockKind::Definition || b.kind==BookBlockKind::Proposition;
      for(const auto& section:matrixChapter())if(ref==section.id)found=!section.explanation.empty();
      expect(found,"Every reference resolves to existing teaching material, never an exercise answer");
    }
    while(!p.active()->currentRun().completed) {
      const auto n=p.active()->currentRun().currentStep;const auto& step=p.active()->content().steps[n];
      const auto working=std::string(p.active()->visibleWorking());
      for(std::size_t j=0;j<step.options.size();++j)if(!fm::acceptsOption(step,j)) {
        expect(p.dispatch(fm::LayeredQuestionCommand::submitOption(step.options[j].id)),"Wrong tile records an attempt");
        expect(p.active()->visibleWorking()==working && p.active()->currentRun().currentStep==n,"Wrong tile cannot change working or step");++wrong;
      }
      answer(p,true);++steps;
    }
    expect(p.active()->content().id==bank[i].id,"Completion stays on the same question");
    expect(p.dispatch({fm::LayeredQuestionCommandKind::RestartQuestion}),"Again restarts a completed problem");
    expect(p.active()->archivedRuns().size()==1,"Again retains the complete solving route");
  }
  expect(steps==23 && wrong==46,"All decisions and distractors are covered");
  auto starters=loadCorpusStarters(STARTER_FIXTURE,corpus);const auto path=folder/"progress.json";
  CorpusPractice old(starters);old.loadProgress(path);old.open(0);answer(old,true);answer(old,false);old.saveProgress();
  const auto oldId=old.active()->content().id,oldWorking=std::string(old.active()->visibleWorking());
  const auto oldJournal=old.active()->journal().size();
  starters.insert(starters.end(),bank.begin(),bank.end());std::reverse(starters.begin(),starters.end());
  CorpusPractice grown(starters);grown.loadProgress(path);
  expect(grown.active() && grown.active()->content().id==oldId && grown.active()->visibleWorking()==oldWorking && grown.active()->journal().size()==oldJournal,"Earlier unfinished starter survives additions and reordering");
  const auto index=static_cast<std::size_t>(std::find_if(starters.begin(),starters.end(),[](const auto& q){return q.id=="matrix_reasoning_03";})-starters.begin());
  grown.open(index);answer(grown,false);answer(grown,true);grown.saveProgress();
  CorpusPractice restored(starters);restored.loadProgress(path);
  expect(restored.active() && restored.active()->content().id=="matrix_reasoning_03" && restored.active()->currentRun().currentStep==1,"A multi-step follow-up resumes at its next decision");
  expect(restored.active()->journal().size()==grown.active()->journal().size(),"Wrong and correct evidence survive reopen");
  const auto disk=read(path);auto changed=starters;changed[index].readingRefs={"future.reading"};
  CorpusPractice references(changed);references.loadProgress(path);expect(references.active() && references.active()->currentRun().currentStep==1,"Reference metadata cannot invalidate mathematical progress");
  expect(read(path)==disk,"Reading restore leaves the save unchanged");
  bool rejected=false;try{auto duplicate=bank;duplicate.push_back(bank[0]);CorpusPractice invalid(duplicate);}catch(const std::exception&){rejected=true;}
  expect(rejected,"Duplicate identities across loaded packs fail before play");
  std::cout<<"REASONING_MODEL {\"questions\":8,\"steps\":23,\"wrong_tiles\":46,\"save_migration\":true,\"reference_boundary\":true}\n";
}
struct Harness {
  MathCorpus corpus=loadMathCorpus(CORPUS_FIXTURE);
  CorpusPractice practice{loadCorpusStarters(REASONING_FIXTURE,corpus)};
  std::unique_ptr<NativeMath> math;
  MathCorpusUiState ui;
  Harness(ImVec2 size) {
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize=size;io.DeltaTime=1.0F/60;
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendFlags|=ImGuiBackendFlags_RendererHasTextures|ImGuiBackendFlags_RendererHasVtxOffset;
    math=std::make_unique<NativeMath>(MATH_RESOURCES);ui.math=math.get();ui.practice=&practice;ui.open=true;ui.questions=true;frame(3);
  }
  ~Harness(){math.reset();ImGui::DestroyContext();}
  void frame(int count=1) {
    for(int i=0;i<count;++i) {
      ImGui::NewFrame();drawMathCorpus(ui,corpus,false);ImGui::Render();
      for(auto* texture:ImGui::GetPlatformIO().Textures) {
        if(texture->Status==ImTextureStatus_WantCreate || texture->Status==ImTextureStatus_WantUpdates){texture->SetTexID(static_cast<ImTextureID>(texture->UniqueID+1));texture->SetStatus(ImTextureStatus_OK);}
        else if(texture->Status==ImTextureStatus_WantDestroy){texture->SetTexID(ImTextureID_Invalid);texture->SetStatus(ImTextureStatus_Destroyed);}
      }
      for(const auto* list:ImGui::GetDrawData()->CmdLists) {
        for(const auto& vertex:list->VtxBuffer)expect(std::isfinite(vertex.pos.x) && std::isfinite(vertex.pos.y),"Finite native draw coordinates");
        for(const auto& command:list->CmdBuffer)if(command.ElemCount)expect(command.GetTexID()!=ImTextureID_Invalid,"Font atlases are acknowledged in memory");
      }
      expect(ui.readingFallbacks==0,"Shared definitions typeset without fallback");
    }
  }
  NotationBounds control(CorpusControl c){return ui.controls[static_cast<std::size_t>(c)];}
  void click(NotationBounds b) {
    auto& io=ImGui::GetIO();
    expect(b.available && b.y>=0 && b.y+b.height/2<io.DisplaySize.y && b.x+b.width/2<io.DisplaySize.x,"Pointer target is visible and enabled");
    io.AddMousePosEvent(b.x+b.width/2,b.y+b.height/2);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame(3);
  }
  void visible(NotationBounds b,const char* name) {
    const auto size=ImGui::GetIO().DisplaySize;
    expect(b.available && b.x>=0 && b.y>=0 && b.x+b.width<=size.x+1 && b.y+b.height<=size.y+1,std::string(name)+" fits the viewport");
  }
};
void inputs(ImVec2 size) {
  Harness h(size);h.click(h.control(CorpusControl::FocusQuestion));expect(h.ui.focusQuestion,"Actual Focus button opens a question workspace");
  for(std::size_t i=0;i<h.practice.questions().size();++i) {
    expect(h.practice.selected()==i,"Next selects the next question");
    const auto pinned=h.ui.questionInk;
    while(!h.practice.active()->currentRun().completed) {
      auto* session=h.practice.active();const auto n=session->currentRun().currentStep;
      h.visible(h.ui.questionInk,"Given matrix");h.visible(h.ui.currentWorking,"Current working");
      expect(h.ui.currentWorking.y<h.ui.choiceArea.y,"Current working appears above the choices");
      const auto journal=session->journal().size(),step=n;const auto working=std::string(session->visibleWorking());
      h.click(h.control(CorpusControl::Method));expect(h.ui.readingOpen && !h.ui.readingSource.empty(),"Actual Method button opens shared reference text");
      expect(session->journal().size()==journal && session->currentRun().currentStep==step && session->visibleWorking()==working,"Opening teaching material cannot judge or alter the exercise");
      h.click(h.control(CorpusControl::Method));
      const auto& authored=session->content().steps[n];const auto correct=fm::firstAcceptedOption(authored);
      h.click(h.ui.answerTiles[(correct+1)%authored.options.size()]);expect(session->visibleWorking()==working && session->currentRun().currentStep==n,"Actual wrong tile keeps the working");
      h.click(h.ui.answerTiles[correct]);expect(session->currentRun().completed || session->currentRun().currentStep==n+1,"Actual correct tile advances exactly once");
      expect(h.ui.questionInk.x==pinned.x && h.ui.questionInk.y==pinned.y,"Given matrix stays pinned through all steps");
    }
    h.frame(3);expect(h.practice.selected()==i,"Complete result is sticky");
    if(i+1<h.practice.questions().size())h.click(h.control(CorpusControl::NextStarter));
  }
  const auto finished=h.practice.active()->content().id;
  h.click(h.control(CorpusControl::FocusQuestion));expect(!h.ui.focusQuestion,"Browse restores navigation");
  expect(h.practice.active()->content().id==finished && h.practice.active()->currentRun().completed,"Browsing preserves completed working");
  h.click(h.control(CorpusControl::Questions));h.click(h.control(CorpusControl::Questions));
  expect(h.practice.active()->content().id==finished && h.practice.active()->currentRun().completed,"Definitions round trip preserves the same result");
}
std::size_t notation() {
  Harness h({1440,860});std::size_t count=0;
  for(const auto& q:h.practice.questions()) {
    std::vector<std::string> formulas{q.question.equation};
    for(const auto& state:q.question.workingStates)formulas.push_back(state.display);
    for(const auto& step:q.question.steps)for(const auto& option:step.options)formulas.push_back(option.label);
    ImGui::NewFrame();ImGui::Begin("Question notation check");
    for(const auto& text:formulas){const auto& formula=h.math->layout(text,16);expect(formula.error.empty(),"Valid native notation: "+text);++count;}
    ImGui::End();ImGui::Render();h.frame();
  }
  h.click(h.control(CorpusControl::FocusQuestion));
  for(std::size_t i=0;i<h.practice.questions().size();++i) {
    h.practice.open(i);h.frame();h.click(h.control(CorpusControl::Method));
    for(std::size_t j=0;j<h.practice.questions()[i].readingRefs.size();++j){h.ui.readingIndex=j;h.frame();expect(!h.ui.readingSource.empty(),"Every referenced passage is displayable");}
  }
  for(const auto size:{ImVec2{360,480},ImVec2{800,600},ImVec2{1440,860}}){ImGui::GetIO().DisplaySize=size;h.frame(3);h.visible(h.ui.questionInk,"Given matrix after live resize");}
  return count;
}
}
int main() {
  const auto folder=std::filesystem::temp_directory_path()/("paths-reasoning-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    std::filesystem::create_directories(folder);const auto corpus=loadMathCorpus(CORPUS_FIXTURE);const auto bank=loadCorpusStarters(REASONING_FIXTURE,corpus);
    model(corpus,bank,folder);const auto formulas=notation();
    for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}})inputs(size);
    std::filesystem::remove_all(folder);
    std::cout<<"REASONING_UI {\"complete_routes\":24,\"sizes\":[[1440,860],[800,600],[360,480]],\"formulas\":"<<formulas<<",\"fallbacks\":0,\"windows\":0,\"captures\":0}\n";
  } catch(const std::exception& e){std::filesystem::remove_all(folder);std::cerr<<e.what()<<'\n';return 1;}
}

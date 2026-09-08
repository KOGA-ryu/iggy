#include "ui/MathCorpusUi.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <imgui.h>
#include <nlohmann/json.hpp>

using namespace paths;
namespace fm=iggy3d::first_move;
namespace {
void expect(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
struct Harness {
  MathCorpus corpus=loadMathCorpus(CORPUS_FIXTURE);
  CorpusPractice practice{loadCorpusStarters(STARTER_FIXTURE,corpus)};
  std::unique_ptr<NativeMath> math;
  MathCorpusUiState ui;
  Harness(float width,float height) {
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendFlags|=ImGuiBackendFlags_RendererHasTextures|ImGuiBackendFlags_RendererHasVtxOffset;
    io.DisplaySize={width,height};io.DeltaTime=1.0F/60;
    math=std::make_unique<NativeMath>(MATH_RESOURCES);ui.math=math.get();ui.practice=&practice;ui.open=true;
    frame(3);
  }
  ~Harness(){math.reset();ImGui::DestroyContext();}
  void begin(){ImGui::NewFrame();}
  void end() {
    ImGui::Render();
    // In-memory font atlas acknowledgement; no window, renderer or captures.
    for(auto* texture:ImGui::GetPlatformIO().Textures) {
      if(texture->Status==ImTextureStatus_WantCreate || texture->Status==ImTextureStatus_WantUpdates) {
        texture->SetTexID(static_cast<ImTextureID>(texture->UniqueID+1));texture->SetStatus(ImTextureStatus_OK);
      } else if(texture->Status==ImTextureStatus_WantDestroy) {
        texture->SetTexID(ImTextureID_Invalid);texture->SetStatus(ImTextureStatus_Destroyed);
      }
    }
    for(const auto* list:ImGui::GetDrawData()->CmdLists) {
      for(const auto& v:list->VtxBuffer)expect(std::isfinite(v.pos.x) && std::isfinite(v.pos.y),"Finite native vertices");
      for(const auto& cmd:list->CmdBuffer)if(cmd.ElemCount)expect(cmd.GetTexID()!=ImTextureID_Invalid,"Every draw command has an acknowledged font atlas");
    }
  }
  void frame(int count=1){for(int i=0;i<count;++i){begin();drawMathCorpus(ui,corpus,false);end();}}
  NotationBounds control(CorpusControl c){return ui.controls[static_cast<std::size_t>(c)];}
  void click(NotationBounds b) {
    expect(b.available,"Control is visible and enabled");auto& io=ImGui::GetIO();
    expect(b.y+b.height/2<io.DisplaySize.y,"Pointer target fits the screen height");
    io.AddMousePosEvent(b.x+b.width/2,b.y+b.height/2);frame();
    io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame(3);
  }
};
void input(Harness& h) {
  h.click(h.control(CorpusControl::Questions));
  expect(h.ui.questions && h.ui.questionMatches.size()==278,"Actual Questions control opens the complete bank");
  for(std::size_t subject=0;subject<6;++subject) {
    h.ui.subject=subject;h.ui.topic.reset();h.ui.refresh=true;h.frame(3);
    auto* session=h.practice.active();const auto initial=session->content().id;const auto pinned=h.ui.questionInk;
    for(std::size_t stepIndex=0;stepIndex<2;++stepIndex) {
      const auto& step=session->content().steps[stepIndex];const auto answer=fm::firstAcceptedOption(step);
      expect(h.ui.answerTiles.size()==step.options.size(),"Every option has a real input tile");
      const auto working=std::string(session->visibleWorking());
      h.click(h.ui.answerTiles[(answer+1)%step.options.size()]);
      expect(session->visibleWorking()==working && session->currentRun().currentStep==stepIndex,"Real wrong input does not mutate working");
      h.click(h.ui.answerTiles[answer]);
      expect(session->currentRun().completed || session->currentRun().currentStep==stepIndex+1,"Correct pointer input advances once");
      expect(std::abs(h.ui.questionInk.x-pinned.x)<1 && std::abs(h.ui.questionInk.y-pinned.y)<1,"Question remains pinned while solving");
    }
    expect(session->currentRun().completed,"Both inputs finish the subject starter");
    h.frame(5);expect(h.practice.active()->content().id==initial,"Finished problem does not auto-advance");
    h.click(h.control(CorpusControl::Questions));expect(!h.ui.questions,"Definitions remain reachable");
    h.click(h.control(CorpusControl::Questions));expect(h.practice.active()->content().id==initial && h.practice.active()->currentRun().completed,"Reading round trip retains question and evidence");
    h.click(h.control(CorpusControl::NextStarter));expect(h.practice.active()->content().id!=initial,"Only explicit Next changes the question");
  }
}
}
int main() {
  try {
    nlohmann::json report{{"formulas",0},{"errors",nlohmann::json::array()},{"sizes",nlohmann::json::array()}};
    {
      Harness h(1440,860);
      for(const auto& starter:h.practice.questions()) {
        std::vector<std::string> formulas{starter.question.equation};
        for(const auto& state:starter.question.workingStates)formulas.push_back(state.display);
        for(const auto& step:starter.question.steps)for(const auto& option:step.options)formulas.push_back(option.label);
        h.begin();ImGui::Begin("Starter notation probe");
        for(const auto& source:formulas) {
          const auto& e=h.math->layout(source,16);report["formulas"]=report["formulas"].get<int>()+1;
          if(!e.error.empty())report["errors"].push_back({{"id",starter.id},{"source",source},{"error",e.error}});
          else {expect(e.width>0 && e.height>0,"Positive equation extents");h.math->draw(e,10,10,IM_COL32_WHITE);}
        }
        ImGui::End();h.end();
      }
    }
    std::cout<<"NOTATION "<<report.dump()<<'\n';
    expect(report["errors"].empty(),"Every authored equation must typeset without fallback");
    for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}) {
      Harness h(size.x,size.y);input(h);
      // Submit every question at each size to exercise actual view reflow and
      // dynamic glyph collection, including wide matrices and long conditions.
      h.ui.subject.reset();h.ui.topic.reset();h.ui.refresh=true;h.frame();
      for(std::size_t i=0;i<h.practice.questions().size();++i){h.practice.open(i);h.frame(2);expect(h.ui.questionInk.available,"Pinned question is visible");}
      report["sizes"].push_back({size.x,size.y});
    }
    std::cout<<"NATIVE_STARTERS "<<report.dump()<<'\n';
  } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

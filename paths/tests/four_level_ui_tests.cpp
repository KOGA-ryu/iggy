#include "ui/MathCorpusUi.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <imgui.h>

namespace fm=iggy3d::first_move;
using namespace paths;
namespace {
void expect(bool yes,const std::string& why){if(!yes)throw std::runtime_error(why);}
struct Harness {
  MathCorpus corpus=loadMathCorpus(CORPUS_FIXTURE);
  CorpusPractice practice{loadCorpusStarters(SUPPORT_FIXTURE,corpus)};
  std::unique_ptr<NativeMath> math;
  MathCorpusUiState ui;
  bool gold=false,cyan=false,green=false;
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
      // Acknowledge font atlas allocations in memory. No renderer, native window,
      // screenshot, capture, saved texture or other image artifact is created.
      for(auto* texture:ImGui::GetPlatformIO().Textures) {
        if(texture->Status==ImTextureStatus_WantCreate || texture->Status==ImTextureStatus_WantUpdates){texture->SetTexID(static_cast<ImTextureID>(texture->UniqueID+1));texture->SetStatus(ImTextureStatus_OK);}
        else if(texture->Status==ImTextureStatus_WantDestroy){texture->SetTexID(ImTextureID_Invalid);texture->SetStatus(ImTextureStatus_Destroyed);}
      }
      for(const auto* list:ImGui::GetDrawData()->CmdLists) {
        for(const auto& vertex:list->VtxBuffer) {
          expect(std::isfinite(vertex.pos.x) && std::isfinite(vertex.pos.y),"Finite draw coordinates");
          gold|=vertex.col==IM_COL32(255,199,77,255);cyan|=vertex.col==IM_COL32(89,217,255,255);green|=vertex.col==IM_COL32(102,230,166,255);
        }
        for(const auto& c:list->CmdBuffer)if(c.ElemCount)expect(c.GetTexID()!=ImTextureID_Invalid,"Native math font atlases acknowledged");
      }
      expect(ui.readingFallbacks==0,"All shown teaching renders with native math");
    }
  }
  NotationBounds control(CorpusControl c){return ui.controls[static_cast<std::size_t>(c)];}
  void visible(NotationBounds b,const std::string& name) {
    const auto size=ImGui::GetIO().DisplaySize;
    expect(b.available && b.x>=0 && b.y>=0 && b.x+b.width<=size.x+1 && b.y+b.height<=size.y+1,
      name+" fits "+std::to_string(static_cast<int>(size.x))+"x"+std::to_string(static_cast<int>(size.y))+"; bounds "+std::to_string(b.x)+","+std::to_string(b.y)+","+std::to_string(b.width)+","+std::to_string(b.height));
  }
  void click(NotationBounds b,const std::string& name="Pointer target") {
    visible(b,name);auto& io=ImGui::GetIO();io.AddMousePosEvent(b.x+b.width/2,b.y+b.height/2);frame();
    io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame(3);
  }
  void key(ImGuiKey key){auto& io=ImGui::GetIO();io.AddKeyEvent(key,true);frame();io.AddKeyEvent(key,false);frame(2);}
  void type(const std::string& text){ImGui::GetIO().AddInputCharactersUTF8(text.c_str());frame(2);}
  void replace(const std::string& text) {
    click(ui.supportEditor,"Written input");auto& io=ImGui::GetIO();const auto mod=io.ConfigMacOSXBehaviors?ImGuiMod_Super:ImGuiMod_Ctrl;
    io.AddKeyEvent(mod,true);key(ImGuiKey_A);io.AddKeyEvent(mod,false);frame();type(text);
    expect(practice.active()->supportView()->draft==text,"Actual keyboard edits update the canonical draft: "+text);
  }
  void again(){click(control(CorpusControl::ReplayStarter),"Again");expect(!practice.active()->currentRun().completed,"Again begins another attempt");}
};
void inputs(ImVec2 size,const std::filesystem::path& folder) {
  Harness h(size);h.click(h.control(CorpusControl::FocusQuestion),"Focus");
  expect(h.ui.focusQuestion,"Focus opens the same workspace");const auto pinned=h.ui.questionInk;
  for(std::size_t level=0;level<4;++level) {
    if(level)h.again();h.click(h.ui.supportLevels[level],"Support level");
    expect(h.practice.active()->supportView()->level==static_cast<fm::SupportLevel>(level),"Actual support button selects the intended level");
    for(const auto b:h.ui.supportLevels)h.visible(b,"Compact support selector");
    h.visible(h.ui.questionInk,"Gold given");h.visible(h.ui.currentWorking,"Working area");
    expect(h.ui.currentWorking.y==h.ui.questionInk.y,"Given and working are adjacent");
    expect(h.ui.questionInk.x==pinned.x && h.ui.questionInk.y==pinned.y,"Question position survives level changes");
    expect(h.ui.readingSource.empty()==(level!=0),"Teaching defaults reflect the selected level");
    if(level==0) {
      expect(h.ui.readingBody.available,"Learn has immediately visible teaching");
      for(std::size_t step=0;step<2;++step) {
        const auto& content=h.practice.active()->content().steps[step];const auto correct=fm::firstAcceptedOption(content);
        const auto before=h.practice.active()->supportView()->working;
        h.click(h.ui.answerTiles[(correct+1)%content.options.size()],"Wrong symbolic choice");
        expect(h.practice.active()->supportView()->working==before,"Wrong tile keeps the working");
        h.click(h.ui.answerTiles[correct],"Correct symbolic choice");
      }
    } else if(level==1) {
      expect(!h.ui.supportEditor.available && !h.ui.answerTiles.empty(),"Practice starts with symbolic choices and optional typing closed");
      const auto& step=h.practice.active()->content().steps[0];const auto correct=fm::firstAcceptedOption(step);
      h.click(h.ui.answerTiles[(correct+1)%step.options.size()],"Wrong Practice tile");
      expect(h.practice.active()->supportView()->working.empty(),"Wrong Practice tile retains working");
      h.click(h.ui.answerTiles[correct],"Correct Practice tile");
      expect(h.practice.active()->supportView()->working=="3x=15" && h.ui.readingSource.empty(),"Practice tile advances with teaching still closed");
      h.click(h.control(CorpusControl::TypeAnswer),"Optional typing");
      h.replace("fifteen");h.click(h.control(CorpusControl::CheckWork),"Check blank");
      expect(h.practice.active()->supportView()->status==fm::WrittenCheckStatus::Unsupported,"Non-numeric blank is not a wrong-math answer");
      h.replace("10/2");h.key(ImGuiKey_Enter);
      expect(h.practice.active()->supportView()->draft.empty(),"Optional typing still submits equivalent fractions and clears the completed blank");
    } else {
      expect(h.ui.answerTiles.empty() && h.practice.active()->supportView()->draft.empty(),"Written levels begin with a blank editor and no answer tiles");
      h.replace("3x=15");h.key(ImGuiKey_Enter);h.type("x=5");
      expect(h.practice.active()->supportView()->draft=="3x=15\nx=5" && h.practice.active()->currentRun().support->submissions.empty(),"Enter inserts a newline without checking or advancing");
      const auto before=h.practice.active()->supportView()->working;
      h.click(h.control(CorpusControl::Method),"Purple Help");
      expect(!h.ui.readingSource.empty() && h.practice.active()->supportView()->draft=="3x=15\nx=5" && h.practice.active()->supportView()->working==before,"Help reads inline and keeps draft and working");
      h.click(h.ui.supportHelp[2],"Next-line help");expect(h.practice.active()->supportView()->help==fm::SupportHelp::NextLine,"Specific help request is recorded");
      h.click(h.control(CorpusControl::Method),"Close help");
      if(level==2) {
        h.replace("3x=15\nx=6");h.click(h.control(CorpusControl::CheckWork),"Check incorrect working");
        expect(!h.practice.active()->currentRun().completed && h.practice.active()->supportView()->working==before,"Incorrect written chain cannot partially commit");
        h.replace("x+5/3=20/3\nx=5");
      } else h.replace("3x=15\nx=5\ncheck: 3*5+5=20");
      h.click(h.control(CorpusControl::CheckWork),"Check written work");
    }
    expect(h.practice.active()->currentRun().completed,"Actual input completes each support level");
    h.frame(3);expect(h.practice.selected()==0,"Completed result stays until Next");
    h.visible(h.ui.currentWorking,"Green finished result");
    expect(h.ui.questionInk.x==pinned.x && h.ui.questionInk.y==pinned.y,"Given stays fixed through completion");
  }
  h.click(h.control(CorpusControl::UndoWork),"Undo finished work");
  expect(!h.practice.active()->currentRun().completed && !h.practice.active()->supportView()->draft.empty(),"Actual Undo restores a step and preserves draft");
  h.replace("5=x");h.click(h.control(CorpusControl::CheckWork),"Check alternate final form");
  expect(h.practice.active()->currentRun().completed,"Equivalent reversed final equation passes from the editor");
  h.click(h.control(CorpusControl::NextStarter),"Next");
  expect(h.practice.selected()==1 && h.practice.active()->supportView()->level==fm::SupportLevel::Independent && !h.practice.active()->supportView()->assisted,"Next keeps independent level for a fresh question");
  h.replace("x=");const auto selected=h.practice.active()->content().id;
  const auto save=folder/(std::to_string(static_cast<int>(size.x))+".json");h.practice.loadProgress(save);h.practice.saveProgress();
  CorpusPractice resumed(h.practice.questions());resumed.loadProgress(save);h.ui.practice=&resumed;h.ui.refresh=false;h.frame(3);
  expect(resumed.active()->content().id==selected && resumed.active()->supportView()->draft=="x=","Actual editor draft returns after save and reopen");
  expect(resumed.active()->supportView()->level==fm::SupportLevel::Independent && h.ui.readingSource.empty(),"Restored independent editor does not reveal teaching");
  h.click(h.ui.supportEditor,"Restored editor");h.key(ImGuiKey_End);h.type("6");h.key(ImGuiKey_Escape);
  expect(h.ui.focusQuestion,"Escape first leaves active text editing without changing the question workspace");
  expect(resumed.active()->supportView()->draft=="x=6","Escape retains the latest automatically saved draft instead of reverting edits");
  h.click(h.control(CorpusControl::FocusQuestion),"Browse");expect(!h.ui.focusQuestion && resumed.active()->content().id==selected,"Browsing retains the active question");
  expect(h.gold && h.cyan && h.green,"Gold given, cyan working and green completion colours all reach draw output");
  h.ui.practice=&h.practice;
}
std::size_t notation() {
  Harness h({1440,860});h.click(h.control(CorpusControl::FocusQuestion));std::size_t count=0;
  for(std::size_t i=0;i<h.practice.questions().size();++i) {
    const auto& q=h.practice.questions()[i];h.practice.open(i);h.frame(2);
    std::vector<std::string> formulas{q.question.equation};
    for(const auto& w:q.question.workingStates)formulas.push_back(w.display);
    for(const auto& step:q.question.steps)for(const auto& o:step.options)formulas.push_back(o.label);
    ImGui::NewFrame();ImGui::Begin("Notation metrics");
    for(const auto& text:formulas){expect(h.math->layout(text,17).error.empty(),"Every published formula has native notation: "+text);++count;}
    ImGui::End();ImGui::Render();h.frame();
    h.click(h.control(CorpusControl::Method));
    for(std::size_t help=0;help<4;++help){h.click(h.ui.supportHelp[help]);expect(!h.ui.readingSource.empty() && h.ui.readingFallbacks==0,"Each help depth typesets");}
  }
  for(const auto size:{ImVec2{360,480},ImVec2{800,600},ImVec2{1440,860}}) {
    ImGui::GetIO().DisplaySize=size;h.frame(3);h.visible(h.ui.questionInk,"Given after live resize");
    for(const auto b:h.ui.supportLevels)h.visible(b,"Level selector after live resize");
  }
  return count;
}
}
int main() {
  const auto folder=std::filesystem::temp_directory_path()/("paths-four-level-ui-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    std::filesystem::create_directories(folder);const auto formulas=notation();
    for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}})inputs(size,folder);
    std::filesystem::remove_all(folder);
    std::cout<<"FOUR_LEVEL_UI {\"complete_routes\":12,\"sizes\":[[1440,860],[800,600],[360,480]],\"formulas\":"<<formulas<<",\"fallbacks\":0,\"windows\":0,\"captures\":0}\n";
  } catch(const std::exception& e){std::filesystem::remove_all(folder);std::cerr<<e.what()<<'\n';return 1;}
}

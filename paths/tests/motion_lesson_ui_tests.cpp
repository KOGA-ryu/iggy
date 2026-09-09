#include "ui/EquationSorterUi.hpp"
#include "content/EquationSorterContentIO.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <imgui.h>

using namespace paths;
namespace {
void expect(bool yes,const std::string& why){if(!yes)throw std::runtime_error(why);}
struct Harness {
  EquationSorterSession sorter{loadSorterContent(SORTER_STUDY_FIXTURE)};
  MathCorpus corpus=loadMathCorpus(CORPUS_FIXTURE);
  MotionLesson lesson;
  std::unique_ptr<NativeMath> math;
  EquationSorterUiState ui;
  Harness(ImVec2 size) {
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize=size;io.DeltaTime=1.0F/60;
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendFlags|=ImGuiBackendFlags_RendererHasTextures|ImGuiBackendFlags_RendererHasVtxOffset;
    math=std::make_unique<NativeMath>(MATH_RESOURCES);ui.corpus=&corpus;ui.library.math=math.get();ui.motion.lesson=&lesson;ui.motion.math=math.get();
    expect(sorter.dispatch({SorterActionKind::OpenStudy,SorterBucket::A,0,sorter.view().revision}).accepted,"Contents opens");frame(3);
  }
  ~Harness(){math.reset();ImGui::DestroyContext();}
  void frame(int count=1,double elapsed=0) {
    for(int n=0;n<count;++n) {
      ImGui::NewFrame();beginEquationSorterFrame(ui,sorter,static_cast<float>(elapsed));
      drawEquationSorter(ui,sorter.view(),sorter.content(),sorter.activeSolve());ImGui::Render();
      for(auto* t:ImGui::GetPlatformIO().Textures) {
        if(t->Status==ImTextureStatus_WantCreate || t->Status==ImTextureStatus_WantUpdates){t->SetTexID(static_cast<ImTextureID>(t->UniqueID+1));t->SetStatus(ImTextureStatus_OK);}
        else if(t->Status==ImTextureStatus_WantDestroy){t->SetTexID(ImTextureID_Invalid);t->SetStatus(ImTextureStatus_Destroyed);}
      }
      for(const auto* list:ImGui::GetDrawData()->CmdLists) {
        for(const auto& v:list->VtxBuffer)expect(std::isfinite(v.pos.x) && std::isfinite(v.pos.y),"Finite native UI geometry");
        for(const auto& cmd:list->CmdBuffer)if(cmd.ElemCount)expect(cmd.GetTexID()!=ImTextureID_Invalid,"Every font texture acknowledged in memory");
      }
      expect(!ui.motion.formulaErrors,"Every visible motion formula typesets");
    }
  }
  void visible(NotationBounds b,const char* name) {
    const auto size=ImGui::GetIO().DisplaySize;
    expect(b.available && b.width>0 && b.height>0,std::string(name)+" available");
    expect(b.x>=-.5 && b.y>=-.5 && b.x+b.width<=size.x+.5 && b.y+b.height<=size.y+.5,std::string(name)+" fits viewport at "+std::to_string(size.x)+"x"+std::to_string(size.y));
  }
  void click(NotationBounds b) {
    visible(b,"Click target");auto& io=ImGui::GetIO();io.AddMousePosEvent(b.x+b.width/2,b.y+b.height/2);frame();
    io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame(3);
  }
  void click(SorterCardBounds b){click(NotationBounds{b.x,b.y,b.width,b.height,b.available});}
  NotationBounds control(MotionControl c){return ui.motion.controls[static_cast<std::size_t>(c)];}
  void drag(NotationBounds b,ImVec2 target) {
    visible(b,"Drag handle");auto& io=ImGui::GetIO();io.AddMousePosEvent(b.x+b.width/2,b.y+b.height/2);frame();
    io.AddMouseButtonEvent(0,true);frame();io.AddMousePosEvent(target.x,target.y);frame(3);io.AddMouseButtonEvent(0,false);frame(3);
  }
  void chapter(std::size_t index) {
    if(ImGui::GetIO().DisplaySize.x<680){click(control(MotionControl::Chapter));click(ui.motion.chapters[index]);}
    else click(ui.motion.chapters[index]);
    expect(lesson.progress().selected==index,"Real chapter selection reaches requested lesson");
  }
  void finish() {for(int n=0;n<80 && lesson.playing();++n)frame(1,.25);expect(!lesson.playing(),"Native playback reaches the end");frame(2);}
};
void exercise(ImVec2 size) {
  Harness h(size);const auto oldProgress=h.sorter.view().revision;
  h.click(h.ui.motionEntry);expect(h.ui.motion.open && h.ui.motion.presented,"Contents Motion control opens the workspace");
  for(std::size_t chapter=0;chapter<motionChapterCount;++chapter) {
    h.chapter(chapter);const auto pinned=h.ui.motion.question;
    for(const auto c:{MotionControl::Run,MotionControl::Rewind,MotionControl::Speed,MotionControl::Timeline})h.visible(h.control(c),"Playback control");
    h.visible(h.ui.motion.question,"Pinned question");h.visible(h.ui.motion.graph,"Graph");h.visible(h.ui.motion.track,"Cart viewport");
    for(std::size_t i=0;i<h.lesson.chapter().parameters.size();++i)h.visible(h.ui.motion.parameters[i],"Parameter slider");
    h.click(h.control(MotionControl::Run));h.finish();expect(!h.lesson.run().solved && h.lesson.run().attempts.size()==1,"Wrong native run retains a failed attempt");
    const auto& graph=h.ui.motion.graph;const double lo=chapter==0?-2:-4,hi=chapter==0?14:8;
    const auto move=[&](std::size_t index,double value,double time) {
      h.drag(h.ui.motion.handles[index],{graph.x+static_cast<float>(time/h.lesson.chapter().duration)*graph.width,graph.y+static_cast<float>((hi-value)/(hi-lo))*graph.height});
    };
    switch(chapter) {
      case 0:move(0,8,1);break;
      case 1:move(0,2,2);break;
      case 2:move(1,3,3);break;
      case 3:move(0,3,1);move(1,-3,3);break;
      case 4:move(0,6,2);break;
    }
    expect(evaluateMotion(chapter,h.lesson.run().plan).passed,"Actual graph dragging constructs a valid plan");
    expect(h.lesson.ghost()!=nullptr,"Failed prior run is available as ghost");
    h.click(h.control(MotionControl::Undo));expect(!evaluateMotion(chapter,h.lesson.run().plan).passed,"Actual Undo restores the prior plan");
    switch(chapter) {
      case 0:move(0,8,1);break;
      case 1:move(0,2,2);break;
      case 2:move(1,3,3);break;
      case 3:move(1,-3,3);break;
      case 4:move(0,6,2);break;
    }
    h.click(h.control(MotionControl::Run));h.frame(2,.25);h.click(h.control(MotionControl::Pause));
    const auto held=h.lesson.sample().position;h.frame(4,.25);expect(h.lesson.sample().position==held,"Actual Pause freezes the cart");
    h.click(h.control(MotionControl::Pause));h.finish();expect(h.lesson.run().solved && h.lesson.run().attempts.size()==2,"Real controls finish the current chapter");
    expect(h.lesson.progress().selected==chapter,"Completion never advances itself");
    expect(h.ui.motion.question.x==pinned.x && h.ui.motion.question.y==pinned.y,"Problem stays pinned through the whole solve");
    h.click(h.control(MotionControl::Run));h.finish();expect(h.lesson.run().attempts.size()==2,"Actual Replay preserves evidence count");
    if(chapter+1<motionChapterCount){h.click(h.control(MotionControl::Next));expect(h.lesson.progress().selected==chapter+1,"Explicit Next advances one chapter");}
  }
  h.click(h.control(MotionControl::Back));expect(!h.ui.motion.open && !h.ui.motion.presented,"Contents returns to the existing game");
  expect(h.sorter.view().revision==oldProgress,"Motion navigation does not mutate existing practice");
  h.click(h.ui.libraryEntry);expect(h.ui.library.open,"Existing Library remains reachable after Motion");
}
void symbolicAndResize() {
  Harness h({1440,860});h.click(h.ui.motionEntry);
  for(std::size_t chapter=0;chapter<motionChapterCount;++chapter) {
    h.chapter(chapter);h.ui.motion.symbols=true;h.frame(3);
    // Open via actual pointer if this chapter inherited a closed disclosure.
    if(!h.ui.motion.choices[0].available)h.click(h.control(MotionControl::Symbols));
    constexpr std::array<std::size_t,5> solution{1,2,3,1,2};
    h.click(h.ui.motion.choices[solution[chapter]]);h.click(h.control(MotionControl::Run));h.finish();expect(h.lesson.run().solved,"Symbolic tiles use the same motion checker");
  }
  for(const auto size:{ImVec2{360,480},ImVec2{800,600},ImVec2{1440,860}}) {
    ImGui::GetIO().DisplaySize=size;h.frame(3);h.visible(h.ui.motion.question,"Question after live resize");h.visible(h.control(MotionControl::Run),"Run after live resize");
    expect(h.lesson.run().solved && h.lesson.progress().selected==4,"Live resize preserves finished working");
  }
}
}
int main() {
  try {
    for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}})exercise(size);
    symbolicAndResize();
    std::cout<<"MOTION_UI {\"sizes\":[[1440,860],[800,600],[360,480]],\"graph_routes\":15,\"symbolic_routes\":5,\"live_resize\":true,\"windows\":0,\"captures\":0}\n";
  } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}

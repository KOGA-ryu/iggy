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
    if(argc==3 && std::string_view(argv[1])=="--published-store") {
      publishedStore=argv[2];for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}})matrix(size);
      std::cout<<"PUBLISHED_DOCUMENT_UI {\"matrix_routes\":12,\"sizes\":[[1440,860],[800,600],[360,480]],\"fallbacks\":0,\"native_windows\":0,\"captures\":0}\n";return 0;
    }
    expect(argc==1,"Use --published-store FOLDER or no arguments");
    for(const auto size:{ImVec2{1440,860},ImVec2{800,600},ImVec2{360,480}}){prepared(size,true);prepared(size,false);linear(size);matrix(size);}
    std::cout<<"DOCUMENT_UI {\"solving_routes\":30,\"matrix_routes\":12,\"diagram_orbit_routes\":3,\"sizes\":[[1440,860],[800,600],[360,480]],\"fallbacks\":0,\"native_windows\":0,\"captures\":0}\n";
  } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

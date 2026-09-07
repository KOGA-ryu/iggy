#include "content/EquationSorterContentIO.hpp"
#include "content/StudyProgressIO.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>

using namespace paths;
namespace fm=iggy3d::first_move;
namespace fs=std::filesystem;
using Json=nlohmann::json;
using Op=fm::MathOperation;
void expect(bool yes,const char* why) {if(!yes)throw std::runtime_error(why);}
EquationSorterSession fresh() {return EquationSorterSession(loadSorterContent(SORTER_STUDY_FIXTURE));}
void action(EquationSorterSession& s,SorterActionKind kind,std::uint32_t id=0,std::uint32_t value=0) {
  const auto r=s.dispatch({kind,SorterBucket::A,id,s.view().revision,value});
  if(!r.accepted)throw std::runtime_error(std::string(r.reason));
}
void select(EquationSorterSession& s,const std::set<std::uint32_t>& ids) {
  action(s,SorterActionKind::OpenStudy);
  for(std::size_t i=0;i<s.view().study.types.size();++i)
    if(!s.view().study.includedTypes[i])action(s,SorterActionKind::ToggleStudyType,0,i);
  action(s,SorterActionKind::SetStudyMode,0,static_cast<unsigned>(StudyMode::Specific));
  for(const auto& c:s.content())if(s.view().study.selected[c.homeIndex]!=ids.contains(c.id))action(s,SorterActionKind::ToggleStudyQuestion,c.id);
  action(s,SorterActionKind::StartStudy);
}
void move(GallerySession& g,Op operation,std::string operand,std::string entry,bool correct=true,fm::MathMoveKind kind=fm::MathMoveKind::Submit) {
  const auto& r=g.question().currentRun();
  const auto result=g.dispatch(MathematicalMove{g.view().challenge,{kind,operation,std::move(operand),std::move(entry),r.runNumber,r.math->revision,r.questionId,r.contentVersion}});
  expect(result.accepted,"saved mathematical move is accepted");
  if(kind==fm::MathMoveKind::Submit)expect(g.question().currentRun().math->events.back().correct==correct,"expected mathematical verdict");
}
void help(GallerySession& g,GalleryHelpKind kind) {expect(g.dispatch(GalleryHelp{g.view().challenge,kind}).accepted,"help accepted");}
void choose(GallerySession& g,bool correct) {
  const auto& step=g.question().content().steps[g.question().currentRun().currentStep];
  std::size_t index=0;while(index<step.options.size() && fm::acceptsOption(step,index)!=correct)++index;
  expect(index<step.options.size(),"prepared choice exists");
  expect(g.dispatch(ChooseAnswer{g.view().challenge,step.options[index].id}).accepted,"prepared choice accepted");
}
std::string read(const fs::path& p) {std::ifstream f(p);return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
void write(const fs::path& p,std::string_view text) {std::ofstream f(p);f<<text;expect(bool(f),"fixture write succeeded");}
void checkScalar(const GallerySession& game) {
  const auto& run=game.question().currentRun();
  expect(run.completed && game.view().wrongHits==1 && run.math->nodes.size()==5 && run.math->events.size()==6,
      "scalar result, wrong attempt and alternate branch survive");
  expect(!game.view().verification.empty() && run.math->nodes[1].parent==0 && run.math->nodes[2].parent==0,
      "original checked branches and final verification are reconstructed");
}
void roundTrip(const fs::path& path,EquationSorterSession& s,StudyProgressFile& store) {
  store.save(s);expect(!store.failed(),"autosave succeeds");
  auto reopened=fresh();StudyProgressFile disk(path);disk.load(reopened);
  expect(!disk.failed() && reopened.view().studying && reopened.view().study.canResume && !reopened.activeSolve(),"restart offers Resume without starting play");
  expect(reopened.studyProgress().queue==s.studyProgress().queue && reopened.studyProgress().selected==s.studyProgress().selected,
      "frozen queue and pending selection survive independently");
  if(s.savedSolve()->question().currentRun().math)checkScalar(*reopened.savedSolve());
  else {
    const auto& run=reopened.savedSolve()->question().currentRun();
    expect(run.currentStep==1 && run.steps[0].answerShown && run.steps[0].hintRequested && run.steps[0].nextMoveRequested &&
        run.steps[0].attempts.size()==1 && !run.steps[0].attempts[0].correct,"graph stage, help exposure and wrong answer survive");
    expect(reopened.savedSolve()->question().coordinateGraph()->stage==s.savedSolve()->question().coordinateGraph()->stage,
        "graph resumes at the same authored stage");
  }
}
void createPractice(const fs::path& path) {
  fs::remove(path);
  auto s=fresh();StudyProgressFile store(path);store.load(s);select(s,{3001,4001,6001});
  auto& a=*s.activeSolve();
  move(a,Op::Divide,"3","x+2=8",false);move(a,Op::Divide,"3","x+2=7");
  move(a,Op::Expand,"","",false,fm::MathMoveKind::Undo);
  move(a,Op::Expand,"","3x+6=21");move(a,Op::Subtract,"6","3x=15");move(a,Op::Divide,"3","x=5");
  checkScalar(a);roundTrip(path,s,store);
  action(s,SorterActionKind::NextSolve);
  auto& graph=*s.activeSolve();choose(graph,false);help(graph,GalleryHelpKind::Hint);help(graph,GalleryHelpKind::NextMove);help(graph,GalleryHelpKind::DoStep);
  roundTrip(path,s,store);
  while(!graph.view().completed)choose(graph,true);
  action(s,SorterActionKind::NextSolve);
  auto& m=*s.activeSolve();move(m,Op::SwapRows,"","[2,1|7] [1,-1|-1]",false);move(m,Op::SwapRows,"","[1,-1|-1] [2,1|7]");
  expect(m.dispatch(ReplayQuestion{true}).accepted,"unfinished run archived");
  move(m,Op::SwapRows,"","[1,-1|-1] [2,1|7]");move(m,Op::Expand,"","",false,fm::MathMoveKind::Undo);
  move(m,Op::DivideRow1,"2","[1,1/2|7/2] [1,-1|-1]");
  // Edit the next-set draft; the existing frozen practice still resumes at 3/3.
  action(s,SorterActionKind::ReturnToStudy);action(s,SorterActionKind::ToggleStudyQuestion,3001);
  store.save(s);expect(!store.failed(),"mixed practice saved for the next process");
  const auto lastWrite=fs::last_write_time(path);
  for(int i=0;i<120;++i)store.save(s);
  expect(fs::last_write_time(path)==lastWrite,"idle frames do not rewrite saved progress");
}
void resumePractice(const fs::path& path) {
  auto s=fresh();StudyProgressFile store(path);store.load(s);
  expect(!store.failed() && s.view().studying && !s.activeSolve() && s.view().study.canResume,"separate process opens contents with Resume");
  expect(s.studyProgress().queue==std::vector<SorterEquationId>{3001,4001,6001} && s.studyProgress().selected==std::vector<SorterEquationId>{4001,6001},"selection edits do not replace frozen questions");
  expect(s.savedSolve()->view().paused && s.view().solveNumber==3 && s.view().solveCount==3,"saved position is restored safely paused");
  action(s,SorterActionKind::ResumeStudy);auto& g=*s.activeSolve();const auto& run=g.question().currentRun();
  expect(run.runNumber==2 && run.priorExposure && !run.completed && run.math->events.size()==3 && run.math->nodes.size()==3 && run.math->active==2,
      "matrix resume retains current run, Undo branch and active node");
  expect(g.question().archivedRuns().size()==1 && g.question().archivedRuns()[0].math->events.size()==2 && g.view().wrongHits==1,
      "archived unfinished matrix and its wrong attempt survive");
  expect(std::get<fm::AugmentedMatrix>(run.math->nodes[run.math->active].equation).rows[0][1]==fm::ExactNumber{1,2},"matrix fractions retain exact values");
  const auto commands=g.question().journal().size();
  expect(!g.dispatch(MathematicalMove{g.view().challenge,{}}).accepted && g.question().journal().size()==commands,"stale input cannot enter the saved journal");
  move(g,Op::AddRow1ToRow2,"-1","[1,1/2|7/2] [0,-3/2|-9/2]");
  move(g,Op::DivideRow2,"-3/2","[1,1/2|7/2] [0,1|3]");move(g,Op::AddRow2ToRow1,"-1/2","[1,0|2] [0,1|3]");
  expect(g.view().completed && !g.view().verification.empty() && !s.view().nextSolve,"restored matrix can finish through the ordinary checker");
  for(int i=0;i<8;++i)expect(g.dispatch(GalleryTick{.25F}).accepted,"complete tick accepted");
  expect(g.view().completed && g.question().currentRun().runNumber==2,"completed problem waits for explicit action");
  store.save(s);expect(!store.failed(),"completed progress replaces the last good save");
  auto again=fresh();StudyProgressFile last(path);last.load(again);expect(!last.failed(),"completed save reopens");
  expect(again.savedSolve()->question().currentRun().completed,"completed working remains completed after another restart");
  action(again,SorterActionKind::ResumeStudy);expect(again.activeSolve()->dispatch(ReplayQuestion{}).accepted,"Play again works after reload");
  last.save(again);auto third=fresh();StudyProgressFile thirdFile(path);thirdFile.load(third);
  expect(!thirdFile.failed() && third.savedSolve()->question().currentRun().runNumber==3 && third.savedSolve()->question().archivedRuns().size()==2,
      "repeated saves and reloads neither lose nor duplicate archived runs");
}
void failureCases(const fs::path& folder) {
  const auto source=folder/"source.json";createPractice(source);const auto good=Json::parse(read(source));
  const auto reject=[&](const Json& bad) {
    const auto path=folder/"bad.json";write(path,bad.is_string()?bad.get<std::string>():bad.dump());const auto bytes=read(path);
    auto s=fresh();select(s,{3001});move(*s.activeSolve(),Op::Divide,"3","x+2=7");
    const auto before=s.progressRevision();const std::string working(s.activeSolve()->view().working);
    StudyProgressFile store(path);store.load(s);
    expect(store.failed() && s.progressRevision()==before && s.activeSolve()->view().working==working,"failed restore preserves the entire live session");
    move(*s.activeSolve(),Op::Subtract,"2","x=5");store.save(s);
    expect(read(path)==bytes,"failed or incompatible saved file is never overwritten");
  };
  reject(Json("{"));auto bad=good;bad["version"]=2;reject(bad);
  bad=good;bad["catalog"][0]["version"]=999;reject(bad);
  bad=good;bad["questions"][2]["version"]=999;reject(bad);
  bad=good;bad["questions"][2]["commands"].back()["revision"]=999;reject(bad);
  bad=good;bad["questions"][1]["commands"][1]["option"]=999;reject(bad);
  bad=good;bad["questions"][0]["commands"][0]["action"]="made_up";reject(bad);
  bad=good;bad["queue"].push_back(6001);reject(bad);
  bad=good;bad["questions"].erase(2);reject(bad);
  bad=good;bad["position"]=-1;reject(bad);
  bad=good;bad["selected"].push_back(999);reject(bad);
  auto changed=loadSorterContent(SORTER_STUDY_FIXTURE);
  for(auto& card:changed)if(card.id==3001) {auto q=std::make_shared<fm::LayeredQuestionContent>(*card.solution);q->steps[0].acceptedOptions^=3;card.solution=q;}
  EquationSorterSession changedSession(std::move(changed));StudyProgressFile changedStore(source);changedStore.load(changedSession);
  expect(changedStore.failed(),"an edited answer key without a version bump cannot reinterpret saved attempts");
  auto s=fresh();StudyProgressFile a(source);a.load(s);expect(!a.failed(),"valid file still loads after rejected fixtures");
  write(source,read(source)+" ");const auto other=read(source);action(s,SorterActionKind::ResumeStudy);
  move(*s.activeSolve(),Op::AddRow1ToRow2,"-1","[1,1/2|7/2] [0,-3/2|-9/2]");a.save(s);
  expect(a.failed() && read(source)==other,"an externally changed save is preserved");
  const auto parent=folder/"unwritable";fs::create_directory(parent);auto unsaved=fresh();StudyProgressFile b(parent/"practice.json");b.load(unsaved);
  fs::remove(parent);write(parent,"obstruction");select(unsaved,{3001});b.save(unsaved);
  expect(b.failed() && unsaved.activeSolve(),"save failure reports a problem without stopping play");
  fs::remove(parent);move(*unsaved.activeSolve(),Op::Divide,"3","x+2=7");b.save(unsaved);
  expect(!b.failed() && fs::exists(parent/"practice.json"),"next accepted change retries a failed write");
}
void randomDraft(const fs::path& folder) {
  const auto path=folder/"random.json";auto s=fresh();StudyProgressFile store(path);store.load(s);
  action(s,SorterActionKind::OpenStudy);action(s,SorterActionKind::SetStudyMode,0,static_cast<unsigned>(StudyMode::Random));
  action(s,SorterActionKind::SetStudyCount,0,2);store.save(s);
  auto again=fresh();StudyProgressFile restored(path);restored.load(again);
  expect(!restored.failed() && again.view().study.mode==StudyMode::Random && again.view().study.randomCount==2 &&
      again.studyProgress().selected==s.studyProgress().selected && !again.view().study.canResume,
      "an unstarted random draft reopens with exactly its chosen questions");
  action(again,SorterActionKind::StartStudy);restored.save(again);
  auto third=fresh();StudyProgressFile thirdFile(path);thirdFile.load(third);
  expect(!thirdFile.failed() && third.studyProgress().queue==s.studyProgress().selected && third.view().study.canResume,
      "starting a restored random draft freezes that same selection");
}
void partialCollection() {
  auto content=loadSorterContent(SORTER_STUDY_FIXTURE);
  const auto card=std::find_if(content.begin(),content.end(),[](const auto& c){return c.id==3001;});
  auto q=*card->solution;q.supportsMathMoves=false;
  q.steps[0].semantics.purpose=fm::StepPurpose::Calculation;q.steps[0].semantics.completion=fm::CompletionRule::AllAccepted;q.steps[0].acceptedOptions=3;
  GalleryConfig config;config.stopAfterQuestion=true;config.variation=GalleryVariation::EquationChain;
  GallerySession game(config,{q},{0});
  const auto shoot=[&](GallerySession& g,fm::OptionId id) {
    expect(g.dispatch(GalleryViewport{{0,0,1000,700}}).accepted,"restored target viewport accepted");(void)g.publishFrame();
    const auto view=g.view();const auto binding=std::find_if(view.answers.begin(),view.answers.begin()+view.choiceCount,[&](const auto& r){return r.binding.option==id;})->binding;
    const auto bodies=g.scene().objects();const auto body=std::find_if(bodies.begin(),bodies.end(),[&](const auto& b){return b.id==binding.object;});
    const auto point=g.scene().project(body->position);
    return g.dispatch(Shoot{g.scene().frame().id,view.challenge,point.x,point.y});
  };
  expect(shoot(game,q.steps[0].options[0].id).accepted && game.view().collected==1 && game.view().step==1,"first answer partially collects the set");
  GallerySession restored(config,{q},{0},game.question().journal());(void)restored.publishFrame();
  expect(restored.view().ready && restored.view().collected==1 && restored.view().correctHits==1,"partial collection resumes with the remaining targets ready");
  const auto journal=restored.question().journal().size();
  expect(!shoot(restored,q.steps[0].options[0].id).accepted && restored.question().journal().size()==journal,"collected target cannot be answered twice after reload");
  expect(shoot(restored,q.steps[0].options[1].id).accepted && restored.view().step==2 && restored.view().correctHits==2,
      "remaining restored target completes through the same shooting and progression route");
}
int main(int argc,char** argv) {
  try {
    if(argc==3) {
      if(std::string_view(argv[1])=="write")createPractice(argv[2]);
      else if(std::string_view(argv[1])=="read")resumePractice(argv[2]);
      else throw std::runtime_error("unknown test phase");
    } else {
      const auto folder=fs::temp_directory_path()/("paths-practice-test-"+std::to_string(std::random_device{}()));fs::create_directory(folder);
      failureCases(folder);randomDraft(folder);partialCollection();fs::remove_all(folder);
    }
    std::cout<<"Practice persistence, checked restoration and saved-file protection passed\n";
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}

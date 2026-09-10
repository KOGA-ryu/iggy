#include "content/CorpusPractice.hpp"
#include "runtime/textbook/Textbook.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
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
}

int main() {
  const auto folder=std::filesystem::temp_directory_path()/("paths-reasoning-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    std::filesystem::create_directories(folder);const auto corpus=loadMathCorpus(CORPUS_FIXTURE);const auto bank=loadCorpusStarters(REASONING_FIXTURE,corpus);
    model(corpus,bank,folder);std::filesystem::remove_all(folder);
  } catch(const std::exception& e){std::filesystem::remove_all(folder);std::cerr<<e.what()<<'\n';return 1;}
}

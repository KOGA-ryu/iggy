#include "content/CorpusPractice.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

using namespace paths;
namespace fm=iggy3d::first_move;
namespace {
void expect(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
std::string read(const std::filesystem::path& p){std::ifstream in(p);return {std::istreambuf_iterator<char>(in),{}};}
void write(const std::filesystem::path& p,const std::string& s){std::ofstream out(p);out<<s;}
void answer(CorpusPractice& practice,bool correct) {
  const auto* session=practice.active();const auto& step=session->content().steps[session->currentRun().currentStep];
  const auto option=std::find_if(step.options.begin(),step.options.end(),[&](const auto& o){return fm::acceptsOption(step,&o-step.options.data())==correct;});
  expect(option!=step.options.end(),"Each step has both a correct and an incorrect tile");
  expect(practice.dispatch(fm::LayeredQuestionCommand::submitOption(option->id)),"Answer routed through the canonical owner");
}
}
int main() {
  try {
    const auto corpus=loadMathCorpus(CORPUS_FIXTURE);const auto bank=loadCorpusStarters(STARTER_FIXTURE,corpus);
    expect(bank.size()==278,"Every authored starter is loaded");
    std::set<std::size_t> subjects,chapters;std::size_t subcategories=0;
    CorpusPractice practice(bank);std::size_t wrongChecks=0;
    for(std::size_t i=0;i<bank.size();++i) {
      const auto& q=bank[i];if(q.level=="subject")subjects.insert(q.subject);if(q.level=="chapter")chapters.insert(*q.topic);if(q.level=="subcategory")++subcategories;
      practice.open(i);auto* session=practice.active();expect(session->progress()==fm::QuestionProgress::NotStarted,"Opening is not an attempt");
      expect(!practice.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Unresolved steps cannot advance");
      std::size_t wrong=0;
      for(std::size_t step=0;step<q.question.steps.size();++step) {
        const auto before=std::string(session->visibleWorking());const auto& options=q.question.steps[step].options;
        for(std::size_t option=0;option<options.size();++option)if(!fm::acceptsOption(q.question.steps[step],option)) {
          expect(practice.dispatch(fm::LayeredQuestionCommand::submitOption(options[option].id)),"Every wrong tile is checked");++wrong;++wrongChecks;
          expect(session->visibleWorking()==before && session->currentRun().currentStep==step,"Wrong tile retains exact working and step");
          expect(session->progress()==fm::QuestionProgress::InProgress,"A checked wrong tile counts as started");
        }
        answer(practice,true);expect(practice.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Resolved step advances");
      }
      expect(session->currentRun().completed && session->visibleWorking()==q.question.workingStates.back().display,"Every complete route reaches the authored final working");
      expect(session->review()->wrongAttempts==wrong,"Wrong attempts remain in review");
      expect(!practice.dispatch({fm::LayeredQuestionCommandKind::Continue}),"Completed result remains until explicit navigation");
    }
    expect(subjects.size()==6 && chapters.size()==178 && subcategories==94,"Full hierarchy has separate questions");
    for(std::size_t s=0;s<6;++s)for(auto i:practice.find(s,{},""))expect(bank[i].subject==s,"Subject filter respects ownership");
    for(std::size_t t=0;t<178;++t)expect(!practice.find({},t,"").empty(),"Every chapter is reachable by its filter");
    expect(!practice.find({}, {}, "Fourier").empty(),"Title and prompt search reach advanced material");

    const auto folder=std::filesystem::temp_directory_path()/("paths-starters-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);const auto save=folder/"progress.json";
    practice.loadProgress(save);practice.saveProgress();
    const auto original=read(save);expect(!original.empty(),"Progress is written");
    auto reordered=bank;std::reverse(reordered.begin(),reordered.end());
    auto added=bank.front();added.id="starter_new_question";added.question.id=added.id;
    auto stamp=nlohmann::json::parse(added.stamp);stamp["id"]=added.id;added.stamp=stamp.dump();reordered.insert(reordered.begin(),added);
    CorpusPractice restored(reordered);restored.loadProgress(save);
    expect(restored.active() && restored.active()->currentRun().questionId==practice.active()->currentRun().questionId,"Selection restores by identity after reorder");
    expect(restored.active()->review()->wrongAttempts==3,"Checked history restores by replay after additions and reordering");
    expect(restored.dispatch({fm::LayeredQuestionCommandKind::RestartQuestion}),"Explicit replay starts a new run");
    answer(restored,false);restored.saveProgress();CorpusPractice resumed(bank);resumed.loadProgress(save);
    expect(resumed.active()->archivedRuns().size()==1 && resumed.active()->review()->wrongAttempts==1,"Archived completion and unfinished retry survive reopen");
    auto changed=bank;changed[0].stamp="{}";CorpusPractice incompatible(changed);const auto preserved=read(save);
    incompatible.loadProgress(save);expect(incompatible.message().find(changed[0].id)!=std::string::npos,"Changed mathematics identifies the saved question");
    incompatible.open(0);incompatible.saveProgress();expect(read(save)==preserved,"Incompatible save is never overwritten");
    auto malformed=nlohmann::json::parse(preserved);malformed["runs"][0]["commands"][0][0]=99;
    write(save,malformed.dump());CorpusPractice invalid(bank);invalid.loadProgress(save);invalid.open(1);invalid.saveProgress();
    expect(read(save)==malformed.dump(),"Malformed command save remains intact");
    write(save,preserved);CorpusPractice external(bank);external.loadProgress(save);external.open(0);write(save,"external edit");external.saveProgress();
    expect(read(save)=="external edit","External edits are not overwritten");
    std::filesystem::remove_all(folder);
    std::cout<<"278 complete routes; "<<wrongChecks<<" wrong-tile checks; full hierarchy; stable identity save/replay and failure preservation passed\n";
  } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

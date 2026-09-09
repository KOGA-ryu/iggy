#include "content/CorpusPractice.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <sys/file.h>
#include <thread>

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
void saveRecovery(const std::filesystem::path& folder,const std::vector<CorpusStarter>& bank) {
  const auto save=folder/"recovery.json",orphan=folder/"recovery.json.tmp";
  CorpusPractice first(bank);first.loadProgress(save);first.open(0);first.saveProgress();
  const auto original=read(save);expect(!original.empty(),"Recovery starts with a valid save");
  write(orphan,"Interrupted previous write");
  CorpusPractice resumed(bank);resumed.loadProgress(save);answer(resumed,false);resumed.saveProgress();
  expect(read(save)!=original,"An abandoned temporary file cannot disable autosave after reopening");
  expect(read(orphan)=="Interrupted previous write","Recovery preserves abandoned bytes");
  CorpusPractice reopened(bank);reopened.loadProgress(save);
  expect(reopened.active() && reopened.active()->review()->wrongAttempts==1,"New evidence survives reopening after an abandoned write");
  const auto target=folder/"untouched.txt";write(target,"Unrelated bytes");
  std::filesystem::remove(orphan);std::filesystem::create_symlink(target,orphan);
  answer(resumed,false);resumed.saveProgress();
  expect(read(target)=="Unrelated bytes" && std::filesystem::is_symlink(orphan),"An abandoned temporary symlink is neither followed nor removed");
  std::filesystem::remove(orphan);std::filesystem::create_directory(orphan);write(orphan/"kept.txt","Keep this directory");
  answer(resumed,false);resumed.saveProgress();
  expect(read(orphan/"kept.txt")=="Keep this directory","A directory at the old temporary path cannot block or be removed by saving");
  CorpusPractice latest(bank);latest.loadProgress(save);
  expect(latest.active()->review()->wrongAttempts==3,"All updates after orphan recovery persist through the same owner");
  const auto fresh=folder/"first-save.json";write(fresh.string()+".tmp","Interrupted first save");write(fresh.string()+".lock","");
  CorpusPractice firstSave(bank);firstSave.loadProgress(fresh);firstSave.open(0);answer(firstSave,false);firstSave.saveProgress();
  CorpusPractice firstReload(bank);firstReload.loadProgress(fresh);
  expect(firstReload.active() && firstReload.active()->review()->wrongAttempts==1 && read(fresh.string()+".tmp")=="Interrupted first save","Abandoned files do not prevent the first committed save");
}
void saveRetry(const std::filesystem::path& folder,const MathCorpus& corpus) {
  const auto bank=loadCorpusStarters(std::filesystem::path(CORPUS_FIXTURE).parent_path()/"linear_support.json",corpus);
  const auto send=[](CorpusPractice& p,fm::SupportAction action,std::string text={},unsigned value=0) {
    fm::LayeredQuestionCommand command{fm::LayeredQuestionCommandKind::Support};command.support=p.active()->supportView()->command;
    command.support.action=action;command.support.text=std::move(text);command.support.value=value;
    expect(p.dispatch(command),"Draft and mode changes still use the canonical support owner");
  };
  const auto savedDraft=[&](const std::filesystem::path& path) {
    CorpusPractice restored(bank);restored.loadProgress(path);
    expect(restored.active() && restored.active()->supportView().has_value(),"Retried save restores a supported question");
    expect(restored.active()->supportView()->level==fm::SupportLevel::Independent,"Retry retains the selected support level");
    return restored.active()->supportView()->draft;
  };
  const auto save=folder/"retry.json";CorpusPractice p(bank);p.loadProgress(save);p.open(0);
  send(p,fm::SupportAction::SelectLevel,{},3);send(p,fm::SupportAction::EditDraft,"x=1");p.saveProgress();
  const auto original=read(save);expect(!original.empty(),"Retry fixture has a valid original save");
  const auto hold=[&] {
    std::unique_ptr<std::FILE,decltype(&std::fclose)> lock(std::fopen((save.string()+".lock").c_str(),"r+b"),&std::fclose);
    expect(lock && ::flock(::fileno(lock.get()),LOCK_EX|LOCK_NB)==0,"A separate descriptor can hold the published save lock");return lock;
  };
  auto lock=hold();send(p,fm::SupportAction::EditDraft,"x=2");p.saveProgress();
  expect(read(save)==original && p.message().find("retry automatically")!=std::string::npos,"Busy saves retain both the previous file and an actionable retry message");
  lock.reset();send(p,fm::SupportAction::EditDraft,"x=3");p.saveProgress();
  expect(read(save)==original,"A transient failure backs off instead of writing on every frame");
  std::this_thread::sleep_for(std::chrono::milliseconds(2100));p.saveProgress();
  expect(savedDraft(save)=="x=3" && p.message()=="Starter progress saved.","Automatic retry commits the newest draft without another edit or restart");
  const auto afterRetry=read(save);lock=hold();send(p,fm::SupportAction::EditDraft,"x=4");p.saveProgress();
  expect(read(save)==afterRetry,"Another interrupted save keeps the last complete draft");
  lock.reset();send(p,fm::SupportAction::EditDraft,"x=5");p.saveProgress(true);
  expect(savedDraft(save)=="x=5","Closing retries immediately even during the backoff interval");

  CorpusPractice other(bank);other.loadProgress(save);send(p,fm::SupportAction::EditDraft,"x=6");p.saveProgress();
  const auto winner=read(save);send(other,fm::SupportAction::EditDraft,"x=7");other.saveProgress();other.saveProgress(true);
  expect(read(save)==winner && savedDraft(save)=="x=6" && other.active()->supportView()->draft=="x=7","A stale window retains its draft and cannot overwrite another window's committed work, even on close");
  expect(other.message().find("changed outside this window")!=std::string::npos,"A competing writer is identified in the failure message");

  const auto unavailable=folder/"missing-parent/progress.json";CorpusPractice delayed(bank);delayed.loadProgress(unavailable);delayed.open(0);
  send(delayed,fm::SupportAction::SelectLevel,{},3);send(delayed,fm::SupportAction::EditDraft,"x=8");delayed.saveProgress();
  expect(!std::filesystem::exists(unavailable) && delayed.message().find("retry automatically")!=std::string::npos,"A missing destination reports an I/O error while retaining working");
  std::filesystem::create_directory(unavailable.parent_path());delayed.saveProgress(true);
  expect(savedDraft(unavailable)=="x=8","Repairing a destination lets the same session save its retained draft");

  const auto linked=folder/"linked-progress.json",target=folder/"lock-target.txt";write(target,"Retain unrelated file");
  std::filesystem::create_symlink(target,linked.string()+".lock");CorpusPractice guarded(bank);guarded.loadProgress(linked);guarded.open(0);
  send(guarded,fm::SupportAction::SelectLevel,{},3);send(guarded,fm::SupportAction::EditDraft,"x=9");guarded.saveProgress();
  expect(!std::filesystem::exists(linked) && read(target)=="Retain unrelated file","A symbolic lock path never touches its target or publishes unlocked progress");
  std::filesystem::remove(linked.string()+".lock");guarded.saveProgress(true);
  expect(savedDraft(linked)=="x=9","Removing a bad lock path allows retry without restarting");
  const auto alias=folder/"save-alias.json";std::filesystem::create_symlink(linked,alias);
  const auto protectedBytes=read(linked);CorpusPractice rejected(bank);rejected.loadProgress(alias);rejected.open(0);rejected.saveProgress(true);
  expect(std::filesystem::is_symlink(alias) && read(linked)==protectedBytes,"Closing cannot override a save rejected during loading");
  for(const auto& entry:std::filesystem::directory_iterator(folder))
    expect(!entry.path().filename().string().starts_with("retry.json.") || entry.path().filename()=="retry.json.lock","Successful and rejected writes leave no owned temporary file behind");
  std::cout<<"AUTOSAVE_RECOVERY {\"orphan_forms\":3,\"first_save\":true,\"automatic_retry\":true,\"closing_retry\":true,\"competing_writer_preserved\":true,\"destination_repaired\":true,\"symlinks_preserved\":true,\"draft_replay\":true,\"captures\":0}\n";
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
    incompatible.open(0);incompatible.saveProgress(true);expect(read(save)==preserved,"Incompatible save is never overwritten");
    auto malformed=nlohmann::json::parse(preserved);malformed["runs"][0]["commands"][0][0]=99;
    write(save,malformed.dump());CorpusPractice invalid(bank);invalid.loadProgress(save);invalid.open(1);invalid.saveProgress(true);
    expect(read(save)==malformed.dump(),"Malformed command save remains intact");
    write(save,preserved);CorpusPractice external(bank);external.loadProgress(save);external.open(0);write(save,"external edit");external.saveProgress();
    expect(read(save)=="external edit","External edits are not overwritten");
    saveRecovery(folder,bank);
    saveRetry(folder,corpus);
    std::filesystem::remove_all(folder);
    std::cout<<"278 complete routes; "<<wrongChecks<<" wrong-tile checks; full hierarchy; stable identity save/replay and failure preservation passed\n";
  } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

#include "content/CorpusPractice.hpp"
#include "content/QuestionContentIO.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace fm=iggy3d::first_move;
using namespace paths;
using Json=nlohmann::json;
namespace {
void expect(bool yes,const std::string& why){if(!yes)throw std::runtime_error(why);}
std::string read(const std::filesystem::path& path){std::ifstream in(path);return {std::istreambuf_iterator<char>(in),{}};}
void write(const std::filesystem::path& path,const std::string& text){std::ofstream out(path);out<<text;}
fm::LayeredQuestionCommand command(const fm::LayeredQuestionSession& s,fm::SupportAction action,std::uint32_t value=0,std::string text={}) {
  fm::LayeredQuestionCommand c{fm::LayeredQuestionCommandKind::Support};c.support=s.supportView()->command;
  c.support.action=action;c.support.value=value;c.support.text=std::move(text);return c;
}
void send(fm::LayeredQuestionSession& s,fm::SupportAction action,std::uint32_t value=0,std::string text={}) {
  expect(s.dispatch(command(s,action,value,std::move(text))).accepted,"Canonical owner accepts the supported action");
}
void submit(fm::LayeredQuestionSession& s,const std::string& text,fm::SupportAction action=fm::SupportAction::CheckWork) {
  send(s,fm::SupportAction::EditDraft,0,text);send(s,action,0,text);
}
fm::LayeredQuestionSession start(const CorpusStarter& q,fm::SupportLevel level) {
  fm::LayeredQuestionSession s({q.question},fm::QuestionInteraction::Supported,0,true);
  send(s,fm::SupportAction::SelectLevel,static_cast<std::uint32_t>(level));
  expect(s.dispatch({fm::LayeredQuestionCommandKind::OpenQuestion}).accepted,"Question opens");return s;
}
void kernel() {
  const auto original=*fm::parseLinearEquation("3x+5=20").equation;
  for(const auto& text:{"3x=15\nx=5", "x+5/3=20/3\nx=5", "5=x", "x=10/2", "x=2+3", "6x+10=40\nx=5\ncheck: 3*5+5=20", " x = 5\r\n\r\n"}) {
    const auto result=fm::checkLinearWork(original,text);
    expect(result.status==fm::WrittenCheckStatus::Correct && result.solved && !result.verification.empty(),std::string("Valid alternative solution: ")+text);
  }
  for(const auto& text:{"x=6", "0=0", "3x=15\n3x=16\nx=5", "x=1/0", "x=5\ncheck: 3*5+5=19", "x=5\ncheck: 0=0"})
    expect(fm::checkLinearWork(original,text).status==fm::WrittenCheckStatus::Incorrect,std::string("Incorrect mathematics: ")+text);
  for(const auto& text:{"", "x*x=25", "sin(x)=0", "subtract five", "x=5\nQED", "check: 3*5+5=20"})
    expect(fm::checkLinearWork(original,text).status==fm::WrittenCheckStatus::Unsupported,std::string("Uncheckable syntax stays distinct: ")+text);
  const auto partial=fm::checkLinearWork(original,"3x=15");
  expect(partial.status==fm::WrittenCheckStatus::Correct && !partial.solved,"Valid intermediate checkpoint is accepted without completion");
  expect(fm::checkLinearWork(original,std::string(8193,'x')).status==fm::WrittenCheckStatus::Unsupported,"Oversized work is not evaluated");
  std::string lines;for(int i=0;i<33;++i)lines+="3x=15\n";
  expect(fm::checkLinearWork(original,lines).status==fm::WrittenCheckStatus::Unsupported,"Line limit is explicit");
}
void routes(const std::vector<CorpusStarter>& bank) {
  expect(bank.size()==25,"Golden example and 24 varied repetitions are published");
  for(const auto& q:bank)for(std::uint32_t route=0;route<5;++route) {
    const auto level=route==4?1U:route;const bool tiles=level==0 || route==4;
    auto s=start(q,static_cast<fm::SupportLevel>(level));const auto& source=*q.question.support;
    expect(s.currentRun().steps.empty() && !s.currentRun().math,"Typed support owns its own evidence, not pretend tile attempts");
    expect(s.supportView()->given==q.question.equation && s.supportView()->working.empty(),"Given is present with no manufactured learner working");
    expect(s.supportView()->reading.empty()==(level!=0),"Only Learn shows teaching by default");
    expect(s.supportView()->choices.empty()==(level>=2),"Learn and Practice offer tiles; written levels disclose none");
    expect(!s.dispatch(fm::LayeredQuestionCommand::submitOption(q.question.steps[0].options[0].id)).accepted,"Legacy prepared route cannot judge this run");
    if(level<2) {
      for(std::size_t i=0;i<2;++i) {
        const auto& step=q.question.steps[i];const auto accepted=fm::firstAcceptedOption(step);
        const auto old=s.supportView()->working;
        for(std::size_t j=0;j<step.options.size();++j)if(j!=accepted) {
          if(tiles)send(s,fm::SupportAction::Choose,step.options[j].id.value);
          else submit(s,source.steps[i].responses[j],fm::SupportAction::SubmitBlank);
          expect(s.supportView()->working==old && !s.currentRun().completed,"Every wrong response preserves working");
        }
        if(tiles)send(s,fm::SupportAction::Choose,step.options[accepted].id.value);
        else submit(s,source.steps[i].responses[accepted],fm::SupportAction::SubmitBlank);
        expect(s.supportView()->history.size()==i+1,"Correct guided response advances one checked step");
      }
      expect(s.review()->wrongAttempts==4,"All four wrong numeric options remain in review");
    } else submit(s,source.steps[0].equation+"\n"+source.steps[1].equation);
    const auto view=*s.supportView();
    expect(view.completed && !view.verification.empty() && !view.canRespond,"Every level reaches a verified sticky completion");
    expect(s.currentRun().support->submissions.back().action==
      (tiles?fm::SupportAction::Choose:level==1?fm::SupportAction::SubmitBlank:fm::SupportAction::CheckWork),"Submission records how the learner actually answered");
    expect(!s.dispatch({fm::LayeredQuestionCommandKind::Continue}).accepted,"Completion requires explicit Next navigation");
    const auto original=*fm::parseLinearEquation(source.equation).equation;
    const auto answer=std::get<fm::LinearEquation>(s.currentRun().support->nodes.back().equation).right.constant;
    const auto a=original.left.coefficient,b=original.left.constant,c=original.right.constant;
    expect(a.denominator==1 && b.denominator==1 &&
      (a.numerator*answer.numerator+b.numerator*answer.denominator)*c.denominator==c.numerator*answer.denominator,
      "Independent integer cross multiplication verifies the original equation for every generated answer");
  }
}
void practiceChoices(const std::vector<CorpusStarter>& bank,const std::filesystem::path& folder) {
  CorpusPractice p(bank);const auto file=folder/"practice-choices.json";p.loadProgress(file);p.open(0);
  const auto sendP=[&](fm::SupportAction a,std::uint32_t value=0,std::string text={}) {
    expect(p.dispatch(command(*p.active(),a,value,std::move(text))),"Practice command reaches the existing owner");
  };
  const auto& q=bank.front().question;const auto& first=q.steps.front();const auto correct=fm::firstAcceptedOption(first);
  sendP(fm::SupportAction::SelectLevel,1);
  expect(p.active()->supportView()->reading.empty() && !p.active()->supportView()->choices.empty(),"Practice choices do not automatically open teaching");
  sendP(fm::SupportAction::ReadHelp,1);expect(p.active()->supportView()->reading==q.support->steps[0].definitions,"Practice Terms reads the current definition");
  sendP(fm::SupportAction::ReadHelp,2);expect(p.active()->supportView()->reading==q.support->steps[0].teaching,"Practice Hint opens the current explanation on demand");
  sendP(fm::SupportAction::ReadHelp,0);expect(p.active()->supportView()->reading.empty(),"Closing Practice Help hides teaching again");
  sendP(fm::SupportAction::EditDraft,0,"unfinished optional input");
  auto stale=command(*p.active(),fm::SupportAction::Choose,first.options[correct].id.value);
  sendP(fm::SupportAction::Choose,first.options[(correct+1)%first.options.size()].id.value);
  expect(p.active()->supportView()->working.empty() && p.active()->supportView()->draft=="unfinished optional input" &&
    p.active()->review()->wrongAttempts==1,"Wrong Practice tile retains working and the optional draft, recording one wrong attempt");
  const auto attempts=p.active()->currentRun().support->submissions.size();
  expect(!p.dispatch(stale) && p.active()->currentRun().support->submissions.size()==attempts,"A stale tile cannot replay or create a second submission");
  expect(!p.dispatch(command(*p.active(),fm::SupportAction::Choose,0)),"Unknown Practice choice is rejected");
  for(unsigned level:{2U,3U}) {
    sendP(fm::SupportAction::SelectLevel,level);
    expect(p.active()->supportView()->choices.empty() && !p.dispatch(command(*p.active(),fm::SupportAction::Choose,first.options[correct].id.value)),"Written levels cannot submit hidden choices");
  }
  sendP(fm::SupportAction::SelectLevel,1);sendP(fm::SupportAction::Choose,first.options[correct].id.value);
  expect(p.active()->supportView()->draft.empty() && p.active()->supportView()->history.size()==1 && p.active()->supportView()->reading.empty(),"Correct Practice tile clears its obsolete input, advances once and keeps teaching closed");
  const auto nodes=p.active()->currentRun().support->nodes.size();sendP(fm::SupportAction::Undo);
  const auto typed=q.support->steps[0].responses[correct];sendP(fm::SupportAction::EditDraft,0,typed);sendP(fm::SupportAction::SubmitBlank,0,typed);
  expect(p.active()->currentRun().support->nodes.size()==nodes+1,"Typed answer after Undo keeps the former tile branch");
  sendP(fm::SupportAction::EditDraft,0,"10/");p.saveProgress();const auto bytes=read(file);
  auto reordered=bank;std::reverse(reordered.begin(),reordered.end());CorpusPractice restored(reordered);restored.loadProgress(file);
  expect(restored.active() && restored.active()->content().id==bank.front().id,"Practice restores by stable question identity");
  const auto& run=*restored.active()->currentRun().support;
  expect(run.level==fm::SupportLevel::Practice && run.help==fm::SupportHelp::None && run.draft=="10/" &&
    run.nodes.size()==nodes+1 && restored.active()->review()->wrongAttempts==1,"Practice level, typed draft, wrong attempt and Undo branches survive replay");
  expect(run.submissions.size()==4 && run.submissions[0].action==fm::SupportAction::Choose &&
    run.submissions[1].action==fm::SupportAction::Choose && run.submissions[2].action==fm::SupportAction::Undo &&
    run.submissions[3].action==fm::SupportAction::SubmitBlank &&
    std::all_of(run.submissions.begin(),run.submissions.end(),[](const auto& e){return e.level==fm::SupportLevel::Practice;}),"Replay distinguishes Practice choices, Undo and typed input");
  expect((run.exposure&7)==7 && restored.active()->supportView()->reading.empty() &&
    !restored.active()->supportView()->choices.empty(),"Help exposure persists without reopening it");
  restored.saveProgress();expect(read(file)==bytes,"An unchanged Practice save stays byte-identical");
  const auto& second=q.steps[1];const auto answer=second.options[fm::firstAcceptedOption(second)].id.value;
  expect(restored.dispatch(command(*restored.active(),fm::SupportAction::Choose,answer)),"Resume can finish using a symbolic tile");
  expect(restored.active()->currentRun().completed && restored.active()->supportView()->draft.empty() && restored.selected().has_value(),"Resumed tile completion clears only the stale draft and remains selected");
  restored.saveProgress();CorpusPractice completed(bank);completed.loadProgress(file);
  expect(completed.active() && completed.active()->currentRun().completed && completed.active()->supportView()->choices.empty(),"Saved completion stays finished until explicit Next");
}
void state(const CorpusStarter& q) {
  auto s=start(q,fm::SupportLevel::Independent);
  expect(!s.supportView()->assisted && s.supportView()->prompt.empty() && s.supportView()->reading.empty(),"Independent begins with no guidance or future working");
  const auto stamp=s.supportView()->command;
  send(s,fm::SupportAction::EditDraft,0,"3x=");send(s,fm::SupportAction::EditDraft,0,"3x=15");
  expect(s.journal().size()==3,"Consecutive draft keystrokes are coalesced, alongside level and open");
  auto stale=command(s,fm::SupportAction::CheckWork,0,"x=5");
  expect(!s.dispatch(stale).accepted,"Submission must match the current draft");
  submit(s,"3x=15");expect(s.supportView()->working=="3x=15" && !s.currentRun().completed,"Partial written work can be checked");
  const auto nodes=s.currentRun().support->nodes.size();
  submit(s,"3x=15\n3x=16\nx=5");expect(s.currentRun().support->nodes.size()==nodes && s.supportView()->working=="3x=15","A wrong middle line commits no part of its submission");
  const auto wrong=s.review()->wrongAttempts;
  submit(s,"x*x=25");expect(s.review()->wrongAttempts==wrong && s.supportView()->draft=="x*x=25","Unsupported syntax retains text without a wrong-math mark");
  submit(s,"x+5/3=20/3");expect(s.supportView()->working=="x+\\frac{5}{3}=\\frac{20}{3}","Different valid normalization creates actual checked working");
  send(s,fm::SupportAction::SelectLevel,1);expect(!s.supportView()->canRespond && s.supportView()->draft=="x+5/3=20/3","A nonmatching guided checkpoint preserves alternate work");
  send(s,fm::SupportAction::SelectLevel,3);send(s,fm::SupportAction::ReadHelp,4);
  expect(s.supportView()->reading.find("Reference solution")!=std::string::npos && s.supportView()->assisted,"Only explicitly requested help reveals the solution");
  send(s,fm::SupportAction::ReadHelp,0);expect(s.supportView()->reading.empty() && s.supportView()->assisted,"Closing help cannot erase exposure");
  submit(s,"5=x\ncheck: 3*5+5=20");expect(s.currentRun().completed,"Swapped-side solution and genuine substitution finish");
  const auto finishedNodes=s.currentRun().support->nodes.size();
  send(s,fm::SupportAction::Undo);expect(!s.currentRun().completed && s.supportView()->draft=="5=x\ncheck: 3*5+5=20" && s.currentRun().support->nodes.size()==finishedNodes,"Undo retains the entire submitted draft and branch");
  submit(s,"x=10/2");expect(s.currentRun().completed && s.currentRun().support->nodes.size()>finishedNodes,"A new route after Undo keeps the former branch");
  for(int bad=0;bad<4;++bad) {
    auto c=command(s,fm::SupportAction::ReadHelp,1);
    if(bad==0)c.support.questionId="another-question";if(bad==1)++c.support.contentVersion;
    if(bad==2)++c.support.runNumber;if(bad==3)c.support.revision=stamp.revision;
    expect(!s.dispatch(c).accepted,"Question, content version, run and revision reject stale inputs");
  }
  fm::LayeredQuestionCommand again{fm::LayeredQuestionCommandKind::RestartQuestion};again.archiveUnfinished=true;
  expect(s.dispatch(again).accepted && s.archivedRuns().size()==1,"Again archives the finished run");
  expect(s.supportView()->draft.empty() && s.supportView()->working.empty() && s.supportView()->seenBefore && s.supportView()->assisted,"New attempt is blank and retains prior exposure");
  submit(s,"3x=15");send(s,fm::SupportAction::EditDraft,0,"x=");
  expect(s.dispatch(again).accepted && s.archivedRuns().back().support->draft=="x=" && !s.archivedRuns().back().completed,"Explicit Again preserves unfinished work and its draft");
  expect(!s.dispatch(command(s,fm::SupportAction::EditDraft,0,std::string(8193,'x'))).accepted,"Model bounds apply even outside the UI");
}
void persistence(const MathCorpus& corpus,const std::vector<CorpusStarter>& bank,const std::filesystem::path& folder) {
  auto legacy=loadCorpusStarters(STARTER_FIXTURE,corpus);const auto save=folder/"mixed.json";
  // Version-1 wire format: original two-element commands and frozen question stamp.
  const Json oldRun{{"id",legacy.front().id},{"question",Json::parse(legacy.front().stamp)},
    {"commands",Json::array({Json::array({0,0})})}};
  Json v1{{"format","paths_corpus_starters"},{"version",1},{"selected",legacy.front().id},{"runs",Json::array({oldRun})}};
  write(save,v1.dump());auto mixed=legacy;mixed.insert(mixed.end(),bank.begin(),bank.end());
  CorpusPractice p(mixed);p.loadProgress(save);expect(p.active() && p.active()->content().id==legacy.front().id,"Version-1 prepared save loads with the new bank added");
  p.saveProgress();expect(read(save)==v1.dump(),"Reading an older save does not rewrite it");
  p.open(legacy.size());
  auto sendP=[&](fm::SupportAction a,std::uint32_t v=0,std::string t={}){expect(p.dispatch(command(*p.active(),a,v,std::move(t))),"Adapter dispatch succeeds");};
  sendP(fm::SupportAction::SelectLevel,3);sendP(fm::SupportAction::ReadHelp,3);sendP(fm::SupportAction::ReadHelp,0);
  sendP(fm::SupportAction::EditDraft,0,"3x=15\nx=5");sendP(fm::SupportAction::CheckWork,0,"3x=15\nx=5");sendP(fm::SupportAction::Undo);
  fm::LayeredQuestionCommand again{fm::LayeredQuestionCommandKind::RestartQuestion};again.archiveUnfinished=true;
  expect(p.dispatch(again),"Unfinished run archives through the adapter");
  sendP(fm::SupportAction::EditDraft,0,"x^2=25");sendP(fm::SupportAction::CheckWork,0,"x^2=25");
  sendP(fm::SupportAction::EditDraft,0,"3x=15\nx=");sendP(fm::SupportAction::SelectLevel,2);sendP(fm::SupportAction::ReadHelp,1);
  p.saveProgress();const auto bytes=read(save);const auto saved=Json::parse(bytes);expect(saved["version"]==2,"New actions save as version 2");
  expect(saved["runs"][1]["commands"][1].is_object(),"Written actions have named, guarded records");
  std::reverse(mixed.begin(),mixed.end());CorpusPractice restored(mixed);restored.loadProgress(save);
  expect(restored.active() && restored.active()->content().id==bank.front().id,"Stable identity restores active question after reordering");
  const auto& r=*restored.active()->currentRun().support;
  expect(r.draft=="3x=15\nx=" && r.level==fm::SupportLevel::Solve && r.help==fm::SupportHelp::Definitions,"Draft, level and open help survive reopen");
  expect(r.submissions.size()==1 && r.status==fm::WrittenCheckStatus::Unsupported && restored.active()->review()->wrongAttempts==0,"Unsupported evidence replays without a wrong-answer mark");
  expect(restored.active()->archivedRuns().size()==1 && restored.active()->archivedRuns()[0].support->nodes.size()==3 && !restored.active()->archivedRuns()[0].completed,"Undo branches and archived runs survive replay");
  expect(restored.active()->supportView()->assisted && restored.active()->supportView()->seenBefore,"Exposure survives archives and restart");
  restored.saveProgress();expect(read(save)==bytes,"Unchanged restored save stays byte-identical");
  const auto next=std::find_if(mixed.begin(),mixed.end(),[&](const auto& q){return q.id==bank[1].id;})-mixed.begin();
  restored.open(next);expect(restored.active()->supportView()->level==fm::SupportLevel::Solve && !restored.active()->supportView()->assisted,"Next unattempted question inherits level without fabricated guidance");
  for(int bad=0;bad<4;++bad) {
    auto corrupt=saved;
    if(bad==0)corrupt["version"]=999;
    if(bad==1)corrupt["runs"][1]["commands"][1]["revision"]=999;
    if(bad==2)corrupt["runs"][1]["commands"][1]["value"]=-1;
    if(bad==3)corrupt["runs"][1]["commands"][1]["extra"]=true;
    const auto badBytes=corrupt.dump();write(save,badBytes);CorpusPractice invalid(mixed);invalid.loadProgress(save);
    expect(invalid.message().find("Original save retained")!=std::string::npos,"Invalid replay explains retained original");
    invalid.open(0);invalid.saveProgress();expect(read(save)==badBytes,"Malformed save is never overwritten");
  }
  write(save,bytes);auto changed=mixed;for(auto& q:changed)if(q.id==bank.front().id)q.stamp="{}";
  CorpusPractice incompatible(changed);incompatible.loadProgress(save);incompatible.open(0);incompatible.saveProgress();
  expect(incompatible.message().find(bank.front().id)!=std::string::npos && read(save)==bytes,"Changed mathematics identifies the question and retains original data");
}
void content(const CorpusStarter& q) {
  for(int bad=0;bad<6;++bad) {
    auto changed=q.question;
    if(bad==0)changed.support->steps[0].equation="3x=16";
    if(bad==1)changed.support->steps[0].responses[0]=changed.support->steps[0].responses[1];
    if(bad==2)changed.steps[0].options[0].label="a different value";
    if(bad==3)changed.support->steps[0].responsePrefix="4x=";
    if(bad==4)changed.support->steps[0].teaching.clear();
    if(bad==5)changed.equation="4x+5=20";
    expect(!fm::validateQuestion(changed,fm::QuestionInteraction::Supported).valid(),"Inconsistent support mathematics or teaching fails validation");
  }
  auto row=Json::parse(q.stamp);row["support"]["family"]="unknown_family";bool rejected=false;
  try{(void)parseQuestionContent(row.dump(),"test");}catch(const std::exception&){rejected=true;}
  expect(rejected,"Unsupported content family is rejected by the real loader");
}
void limits(const CorpusStarter& q) {
  auto s=start(q,fm::SupportLevel::Independent);
  for(std::size_t i=1;i<fm::kMathNodeCapacity-1;++i)submit(s,"3x=15");
  const auto nodes=s.currentRun().support->nodes.size();
  submit(s,"3x=15\nx=5");
  expect(s.currentRun().support->nodes.size()==nodes && s.review()->wrongAttempts==0 &&
    s.supportView()->feedback.find("history")!=std::string::npos && s.supportView()->draft=="3x=15\nx=5",
    "A history-capacity refusal explains how to continue and preserves all work without a wrong-math mark");
  submit(s,"x=5");expect(s.currentRun().completed,"A shorter submission still fits the remaining history");
  send(s,fm::SupportAction::Undo);
  expect(!s.supportView()->canRespond && s.supportView()->prompt.find("Again")!=std::string::npos,"Full working history exposes a usable next action");
  auto attempts=start(q,fm::SupportLevel::Independent);
  for(std::size_t i=0;i<fm::kSupportSubmissionCapacity;++i)submit(attempts,"x=6");
  expect(!attempts.supportView()->canRespond && attempts.supportView()->draft=="x=6" &&
    attempts.supportView()->prompt.find("Again")!=std::string::npos,"Full submission history keeps the draft and explains restart");
}
}
int main() {
  const auto folder=std::filesystem::temp_directory_path()/("paths-four-level-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    std::filesystem::create_directories(folder);const auto corpus=loadMathCorpus(CORPUS_FIXTURE);
    const auto bank=loadCorpusStarters(SUPPORT_FIXTURE,corpus);
    kernel();routes(bank);practiceChoices(bank,folder);state(bank.front());persistence(corpus,bank,folder);content(bank.front());limits(bank.front());
    std::filesystem::remove_all(folder);
    std::cout<<"125 complete routes; 300 wrong-response checks; Practice tiles and optional typing; help disclosure; mixed-input save/Undo/replay; exact alternative working; guarded input; v1/v2 save replay and original-file preservation passed\n";
  } catch(const std::exception& e){std::filesystem::remove_all(folder);std::cerr<<e.what()<<'\n';return 1;}
}

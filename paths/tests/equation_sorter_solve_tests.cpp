#include "content/EquationSorterContentIO.hpp"
#include "content/QuestionContentIO.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using namespace paths;
namespace fm=iggy3d::first_move;
using Json=nlohmann::json;
void expect(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
void apply(EquationSorterSession& session,SorterActionKind kind,SorterEquationId id=0) {
  const auto result=session.dispatch({kind,SorterBucket::A,id,session.view().revision});
  if(!result.accepted)throw std::runtime_error(std::string(result.reason));
}
void apply(GallerySession& game,const GalleryCommand& command) {
  const auto result=game.dispatch(command);
  if(!result.accepted)throw std::runtime_error(std::string(result.reason));
}
void tick(GallerySession& game,float seconds) {
  apply(game,GalleryTick{seconds});(void)game.publishFrame();
}
void help(GallerySession& game,GalleryHelpKind kind) {apply(game,GalleryHelp{game.view().challenge,kind});}
void choose(GallerySession& game,std::uint32_t id) {apply(game,ChooseAnswer{game.view().challenge,{id}});}
Shoot shot(GallerySession& game,std::uint32_t id) {
  (void)game.publishFrame();
  const auto view=game.view();
  const auto end=view.answers.begin()+view.choiceCount;
  const auto row=std::find_if(view.answers.begin(),end,[&](const auto& a){return a.binding.option.value==id;});
  expect(row!=end,"answer has a target binding");
  const auto objects=game.scene().objects();
  const auto body=std::find_if(objects.begin(),objects.end(),[&](const auto& o){return o.id==row->binding.object;});
  expect(body!=objects.end(),"binding has a real sphere");
  const auto point=game.scene().project(body->position);
  const auto hit=game.scene().hitTestPresentedFrame(game.scene().frame().id,point.x,point.y);
  expect(hit.accepted && hit.object==body->id,"projected answer hits the published sphere");
  return {game.scene().frame().id,view.challenge,point.x,point.y};
}
const fm::LayeredQuestionStepRecord& step(const GallerySession& game) {
  return game.question().currentRun().steps[game.question().currentRun().currentStep];
}

// Keep the prepared-question contract covered independently of the new default
// bracket mode. The direct-move suite loads the published pack without changes.
std::vector<SorterEquation> preparedContent(const std::filesystem::path& path) {
  auto result=loadSorterContent(path);
  for(auto& equation:result)if(equation.solution && equation.solution->supportsMathMoves) {
    auto prepared=std::make_shared<fm::LayeredQuestionContent>(*equation.solution);
    prepared->supportsMathMoves=false;equation.solution=std::move(prepared);
  }
  return result;
}

void playerLoop() {
  EquationSorterSession session(preparedContent(SORTER_MIXED_FIXTURE));
  apply(session,SorterActionKind::AutoSort);
  apply(session,SorterActionKind::SelectBucket);
  apply(session,SorterActionKind::SelectBucket);
  apply(session,SorterActionKind::ActivateEquation,1012);
  const auto groups=session.view();
  expect(groups.inventory && groups.solveCandidate==1012,"linked card is available inside its group");
  apply(session,SorterActionKind::OpenSolve,1012);
  auto& game=*session.activeSolve();
  apply(game,GalleryViewport{{0,160,980,740}});tick(game,.15F);
  expect(game.view().step==1 && game.view().working=="-2(x + 1) = 22" && game.view().verification.empty(),
      "solver opens the selected equation with its check hidden");
  expect(game.view().workingHighlights.size()==2 &&
      game.view().working.substr(game.view().workingHighlights[0].offset,game.view().workingHighlights[0].length)=="-2" &&
      game.view().working.substr(game.view().workingHighlights[1].offset,game.view().workingHighlights[1].length)=="(x + 1)",
      "initial working identifies the outside factor and the whole bracket from prepared spans");
  expect(!game.dispatch(shot(game,101)).accepted,"operation decisions cannot be bypassed with a sphere shot");
  choose(game,108);
  expect(game.view().step==1 && game.view().wrongHits==1 && game.view().feedback==GalleryFeedback::Incorrect &&
      game.view().working=="-2(x + 1) = 22" && !step(game).awaitingRecoveryChoice,
      "wrong operation briefly signals and keeps the same working active");
  tick(game,.25F);
  expect(!game.dispatch(ChooseAnswer{game.view().challenge,{9999}}).accepted &&
      game.view().feedback==GalleryFeedback::Incorrect && step(game).attempts.size()==1,
      "an unknown operation leaves the existing feedback and attempt evidence intact");
  apply(game,GalleryPause{true});tick(game,.25F);
  expect(game.view().feedback==GalleryFeedback::Incorrect &&
      !game.dispatch(ChooseAnswer{game.view().challenge,{101}}).accepted,
      "paused button feedback keeps its remaining active time and rejects answers");
  apply(game,GalleryPause{false});tick(game,.25F);
  expect(game.view().feedback==GalleryFeedback::None && game.view().step==1 && game.view().wrongHits==1 &&
      step(game).attempts.size()==1,"rejected button input cannot extend feedback expiry or add attempts");
  const auto stale=GalleryHelp{game.view().challenge,GalleryHelpKind::DoStep};
  choose(game,101);
  expect(game.view().step==2 && game.view().working=="x + 1 = 22 / (-2)" && game.view().feedback==GalleryFeedback::None,
      "operation reveals the arithmetic to perform and clears the old feedback immediately");
  expect(!game.dispatch(stale).accepted && game.view().step==2,"old help cannot skip a new decision");
  expect(!game.dispatch(ChooseAnswer{game.view().challenge,{101}}).accepted,"arithmetic requires the target route");
  (void)game.publishFrame();
  expect(game.view().ready && game.view().workingHighlights.empty(),"arithmetic is available in its first presented frame");
  auto previousShot=shot(game,108);
  (void)game.publishFrame();
  expect(!game.dispatch(previousShot).accepted && game.view().wrongHits==1,"stale geometry never records an answer");
  apply(game,shot(game,108));
  expect(game.view().step==2 && game.view().wrongHits==2 && !game.view().transitioning && game.view().ready &&
      game.view().feedback==GalleryFeedback::Incorrect,
      "wrong arithmetic leaves every target and the current calculation active");
  const auto correctShot=shot(game,101);
  const auto oldHelp=GalleryHelp{game.view().challenge,GalleryHelpKind::DoStep};
  apply(game,correctShot);
  expect(game.view().step==3 && game.view().working=="x + 1 = -11" && !game.view().transitioning &&
      game.view().feedback==GalleryFeedback::None,
      "correct arithmetic immediately updates working and opens the next operation without a clock tick");
  expect(!game.dispatch(correctShot).accepted && !game.dispatch(oldHelp).accepted && game.view().step==3,
      "the previous shot and help cannot skip the newly opened decision");
  const auto savedChallenge=game.view().challenge;
  apply(session,SorterActionKind::ReturnToSorter);
  expect(!session.activeSolve() && session.savedSolve()->view().paused && session.view().owners==groups.owners &&
      session.view().inventorySlots==groups.inventorySlots && session.view().undoDepth==groups.undoDepth &&
      session.view().inspected==groups.inspected,"return preserves groups, slots, inspection and Undo");
  apply(session,SorterActionKind::OpenSolve,1012);
  expect(session.activeSolve()==&game && !game.view().paused && game.view().challenge==savedChallenge,
      "returning to the equation resumes the same owner and challenge");
  choose(game,101);
  expect(game.view().step==4 && game.view().working=="x = -11 - 1","inverse addition opens its arithmetic");
  apply(game,shot(game,101));
  expect(game.view().completed && game.view().working=="x = -12" && game.view().correctHits==4 &&
      game.view().wrongHits==2 && !game.view().verification.empty(),"four decisions immediately reach the solved equation and substitution check");
  const auto summary=fm::summarizeLayeredQuestionRun(game.question().currentRun());
  expect(summary.completed && !summary.assisted && summary.correctedAfterRetry==2 && summary.correctOnFirstTry==2,
      "player evidence distinguishes retries from assistance");
  tick(game,.25F);tick(game,.25F);
  expect(game.view().completed && game.question().archivedRuns().empty(),"finite practice waits for an explicit replay");
  expect(!game.dispatch(GalleryHelp{game.view().challenge,GalleryHelpKind::DoStep}).accepted,"completed practice cannot apply another step");
  apply(game,ReplayQuestion{});
  expect(game.view().step==1 && !game.view().completed && game.view().priorExposure &&
      game.question().archivedRuns().size()==1 && game.question().currentRun().runNumber==2 && step(game).attempts.empty(),
      "replay starts clean while preserving the completed run");
  apply(session,SorterActionKind::ReturnToSorter);
  apply(session,SorterActionKind::Undo);
  expect(session.view().counts[0]==100,"the original grouping Undo still works after solving and replay");
}

void assistanceLoop() {
  EquationSorterSession session(preparedContent(SORTER_MIXED_FIXTURE));
  apply(session,SorterActionKind::OpenSolve,1012);
  auto& game=*session.activeSolve();
  for(std::size_t index=1;index<=4;++index) {
    const auto challenge=game.view().challenge;
    const std::string working(game.view().working);
    expect(game.view().canHint && game.view().canReveal && game.view().hint.empty() && game.view().nextMove.empty(),
        "every fresh decision has optional authored help");
    help(game,GalleryHelpKind::Hint);
    expect(!game.view().hint.empty() && game.view().nextMove.empty() && step(game).attempts.empty() &&
        fm::summarizeLayeredQuestionRun(game.question().currentRun()).assisted,"hint records assistance without an attempt");
    help(game,GalleryHelpKind::NextMove);
    expect(game.view().step==index && game.view().challenge==challenge && game.view().working==working &&
        !game.view().nextMove.empty() && !step(game).answerShown,"reveal explains the next move without advancing it");
    apply(game,GalleryPause{true});
    expect(!game.dispatch(GalleryHelp{challenge,GalleryHelpKind::DoStep}).accepted,"paused help cannot mutate the equation");
    apply(game,GalleryPause{false});
    help(game,GalleryHelpKind::DoStep);
    const auto& record=game.question().currentRun().steps[index-1];
    expect(record.answerShown && record.nextMoveRequested && record.hintRequested && !record.resolvedByPlayer &&
        record.attempts.empty(),"Do this step records explicit assistance and no fabricated correct answer");
  }
  const auto summary=fm::summarizeLayeredQuestionRun(game.question().currentRun());
  expect(game.view().completed && game.view().working=="x = -12" && game.view().correctHits==0 &&
      summary.assisted && summary.shownAnswers==4,"assistance can complete every decision without inventing score");
}

void contentBoundary() {
  const auto content=preparedContent(SORTER_MIXED_FIXTURE);
  const auto e=std::find_if(content.begin(),content.end(),[](const auto& item){return item.id==1012;});
  expect(e!=content.end() && e->solution && e->solution->equation==e->text,"file reference resolves the selected card");
  const auto& question=*e->solution;
  expect(question.steps.size()==4 && question.workingStates.size()==5,"prepared content declares the complete method");
  // Verify the arithmetic and unique solution independently of the answer masks.
  const int quotient=22/-2,answer=quotient-1;
  expect(quotient==-11 && answer==-12 && -2*(answer+1)==22,"division, subtraction and substitution agree");
  for(const auto& decision:question.steps) {
    expect(!decision.hint.empty() && !decision.nextMove.empty(),"every authored decision is assistable");
    if(decision.semantics.purpose==fm::StepPurpose::Calculation) {
      const int expected=decision.id.value==2?quotient:answer;
      for(std::size_t i=0;i<decision.options.size();++i)
        expect((std::stoi(decision.options[i].label)==expected)==bool(decision.acceptedOptions & (1U<<i)),
            "every accepted and rejected arithmetic choice matches independent integer arithmetic");
    }
  }
  auto broken=content;
  auto q=std::make_shared<fm::LayeredQuestionContent>(question);
  const auto index=static_cast<std::size_t>(e-content.begin());
  broken[index].solution=q;
  q->equation="x = 7";
  expect(validateSorterContent(broken)->field=="/equations/3/solve_pack","mismatched equation fails the shared content boundary");
  *q=question;q->steps[2].hint.clear();
  expect(validateSorterContent(broken).has_value(),"linked solver refuses a step without assistance");
  *q=question;q->workingStates[0].highlights[0].length=100;
  expect(!fm::validateQuestion(*q,fm::QuestionInteraction::ArcadeCollect).valid() && validateSorterContent(broken).has_value(),
      "constructor and linked-content validation reject highlight spans outside the working");

  EquationSorterSession selected(content);
  const auto unprepared=std::find_if(content.begin(),content.end(),[](const auto& item){return !item.solution;});
  apply(selected,SorterActionKind::ActivateEquation,unprepared->id);
  expect(!selected.view().solveCandidate,"inspecting an unprepared card never offers to solve a different equation");
  apply(selected,SorterActionKind::ActivateEquation,1012);
  expect(selected.view().solveCandidate==1012,"inspecting the prepared card selects that exact problem");

  std::ifstream source(SORTER_MIXED_FIXTURE);Json document;source>>document;
  for(const auto& bad: {Json(7),Json(""),Json("/absolute.json")}) {
    auto invalid=document;invalid["equations"][index]["solve_pack"]=bad;
    bool failed=false;
    try {(void)parseSorterContent(invalid.dump(),"bad.json");}
    catch(const EquationSorterContentError& error) {failed=error.field=="/equations/3/solve_pack";}
    expect(failed,"malformed solve reference has an exact field error");
  }
  const auto cardPath=std::filesystem::path(SORTER_MIXED_FIXTURE).parent_path()/"../cards/sorter_linear_bracket.json";
  std::ifstream cardFile(cardPath);Json card;cardFile>>card;
  for(const auto& span: {Json{{"offset",0},{"length",0},{"label","Empty"}},
                        Json{{"offset",1},{"length",100},{"label","Overflow"}},
                        Json{{"offset",0},{"length",2},{"label",""}}}) {
    auto invalid=card;invalid["working_states"][0]["highlights"][0]=span;
    bool failed=false;
    try {(void)parseQuestionContent(invalid.dump(),cardPath);}
    catch(const QuestionContentError& error) {failed=error.field=="/working_states/0/highlights";}
    expect(failed,"invalid prepared highlight fails at its source working state");
  }
  auto overlap=card;overlap["working_states"][0]["highlights"][1]["offset"]=1;
  auto splitUtf8=card;splitUtf8["working_states"][0]["display"]="π = 3.14";
  splitUtf8["working_states"][0]["highlights"]=Json::array({{{"offset",1},{"length",1},{"label","Split character"}}});
  for(const auto& invalid:{overlap,splitUtf8}) {
    bool failed=false;
    try {(void)parseQuestionContent(invalid.dump(),cardPath);}
    catch(const QuestionContentError& error) {failed=error.field=="/working_states/0/highlights";}
    expect(failed,"highlight spans cannot overlap or split a UTF-8 character");
  }
  for(const auto field:{"hint","next_move"}) {
    auto invalid=card;invalid["steps"][0][field]=12;
    bool failed=false;
    try {(void)parseQuestionContent(invalid.dump(),cardPath);}
    catch(const QuestionContentError& error) {failed=error.field==std::string("/steps/0/")+field;}
    expect(failed,"help fields require text at the shared parser boundary");
  }
  for(auto& decision:card["steps"]) {decision.erase("hint");decision.erase("next_move");}
  card["working_states"][0].erase("highlights");
  expect(parseQuestionContent(card.dump(),cardPath).steps[0].hint.empty(),"legacy question files still load without optional help");
  expect(parseQuestionContent(card.dump(),cardPath).workingStates[0].highlights.empty(),"legacy working states need no highlighting metadata");

  const auto temp=std::filesystem::temp_directory_path()/("paths-solve-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(temp);
  struct Cleanup {std::filesystem::path path;~Cleanup(){std::filesystem::remove_all(path);}} cleanup{temp};
  document["equations"][index]["solve_pack"]="missing.json";
  {std::ofstream out(temp/"sorter.json");out<<document;}
  bool failed=false;
  try {(void)preparedContent(temp/"sorter.json");}
  catch(const EquationSorterContentError& error) {failed=error.field=="/equations/3/solve_pack";}
  expect(failed,"missing linked files fail before a session or renderer starts");
}
void preparedFamily() {
  const auto content=preparedContent(SORTER_BRACKET_FIXTURE);
  const std::array<SorterEquationId,6> ids{1012,3001,3002,3003,3004,3005};
  const std::array answers{"x = -12","x = 5","x = 5","x = 0","x = 3/2","x = -7/2"};
  expect(std::count_if(content.begin(),content.end(),[](const auto& e){return bool(e.solution);})==6,
      "practice pack offers exactly six linked questions");
  EquationSorterSession session(content);
  apply(session,SorterActionKind::AutoSort);
  apply(session,SorterActionKind::SelectBucket);
  const auto groups=session.view();
  std::array<GallerySession*,6> saved{};
  std::array<ChallengeId,6> challenges{};
  for(std::size_t i=0;i<ids.size();++i) {
    apply(session,SorterActionKind::ActivateEquation,ids[i]);
    expect(session.view().solveCandidate==ids[i] && session.view().solveLabel.starts_with("Solve:"),
        "inspecting a new prepared card offers that exact question");
    apply(session,SorterActionKind::OpenSolve,ids[i]);
    auto& game=*session.activeSolve();saved[i]=&game;
    apply(game,GalleryViewport{{0,160,980,740}});tick(game,.15F);
    expect(game.view().equation==content[i].text && game.view().working==content[i].text,
        "displayed question and initial working match the selected recipe");
    help(game,GalleryHelpKind::Hint);choose(game,108);choose(game,101);
    challenges[i]=game.view().challenge;
    expect(game.view().step==2 && game.view().correctHits==1 && game.view().wrongHits==1,
        "each question retains its own partial answer and help evidence");
    apply(session,SorterActionKind::ReturnToSorter);
    expect(game.view().paused && session.view().owners==groups.owners && session.view().inventorySlots==groups.inventorySlots &&
        session.view().undoDepth==groups.undoDepth,"switching questions preserves groups, slots and grouping history");
  }
  for(std::size_t i=0;i<ids.size();++i) {
    // Clear inspection so an already inspected card cannot be returned by the
    // sorter's intentionally distinct second-click action.
    apply(session,SorterActionKind::ClearInspection);
    apply(session,SorterActionKind::ActivateEquation,ids[i]);
    expect(session.view().solveLabel.starts_with("Resume:"),"every previously opened card offers Resume");
    apply(session,SorterActionKind::OpenSolve,ids[i]);
    auto& game=*session.activeSolve();
    expect(&game==saved[i] && game.view().challenge==challenges[i] && game.view().step==2 &&
        game.question().currentRun().steps[0].hintRequested,"switching back resumes the original owner and evidence");
    for(std::size_t j=0;j<ids.size();++j)if(j!=i)expect(saved[j]->view().paused,"inactive questions stay paused");
    apply(game,shot(game,108));apply(game,shot(game,101));choose(game,101);apply(game,shot(game,101));
    expect(game.view().completed && game.view().working==answers[i] && game.view().correctHits==4 && game.view().wrongHits==2 &&
        !game.view().verification.empty(),"every integer, zero and fractional answer completes through real sphere hits");
    apply(session,SorterActionKind::ReturnToSorter);
  }
  apply(session,SorterActionKind::OpenSolve,3004);
  apply(*session.activeSolve(),ReplayQuestion{});
  expect(saved[4]->view().step==1 && saved[4]->question().archivedRuns().size()==1,
      "replay archives only the selected equation's evidence");
  for(std::size_t i=0;i<ids.size();++i)if(i!=4)
    expect(saved[i]->view().completed && saved[i]->question().archivedRuns().empty(),"other completed equations survive replay");
  apply(session,SorterActionKind::ReturnToSorter);apply(session,SorterActionKind::Undo);
  expect(session.view().counts[0]==100,"original Auto sort remains undoable after playing the whole family");
}
void nextPreparedProblem() {
  auto content=preparedContent(SORTER_BRACKET_FIXTURE);
  std::reverse(content.begin(),content.end());
  EquationSorterSession session(std::move(content));
  apply(session,SorterActionKind::AutoSort);
  const auto groups=session.view();
  const auto next=[&] {return session.dispatch({SorterActionKind::NextSolve,SorterBucket::A,0,session.view().revision});};
  expect(!next().accepted,"Next is unavailable outside the solving workspace");
  apply(session,SorterActionKind::OpenSolve,3001);
  auto* partial=session.activeSolve();help(*partial,GalleryHelpKind::Hint);choose(*partial,101);
  apply(session,SorterActionKind::ReturnToSorter);
  apply(session,SorterActionKind::OpenSolve,1012);
  const std::array ids{1012U,3001U,3002U,3003U,3004U,3005U};
  for(std::size_t i=0;i<ids.size();++i) {
    auto* game=session.activeSolve();
    expect(session.view().solveNumber==i+1 && session.view().solveCount==ids.size() &&
        game->view().equation==session.content()[i].text,"Next follows immutable home order, independent of input file order");
    if(i==1)expect(game==partial && game->view().step==2 && game->question().currentRun().steps[0].hintRequested,
        "Next resumes a previously started card through its original question owner");
    expect(!next().accepted && !session.view().nextSolve,"Next cannot skip an unfinished question");
    apply(*game,GalleryViewport{{0,240,520,360}});tick(*game,.15F);
    while(!game->view().completed) {
      if(game->view().purpose==fm::StepPurpose::OperationChoice)choose(*game,101);
      else apply(*game,shot(*game,101));
    }
    const auto working=std::string(game->view().working),check=std::string(game->view().verification);
    const auto run=game->question().currentRun().runNumber;
    const auto revision=session.view().revision;
    for(int j=0;j<240;++j)tick(*game,.25F);
    expect(session.activeSolve()==game && game->view().working==working && game->view().verification==check &&
        game->question().currentRun().runNumber==run,"completion remains unchanged after a minute of ticks until explicit Next");
    if(i+1<ids.size()) {
      expect(session.view().nextSolve==ids[i+1],"completion offers the following prepared card");
      expect(!session.dispatch({SorterActionKind::NextSolve,SorterBucket::A,0,revision-1}).accepted,
          "a stale Next action is rejected without replacing the completed question");
      expect(next().accepted && game->view().paused,"explicit Next pauses and retains the completed question");
      expect(!session.dispatch({SorterActionKind::NextSolve,SorterBucket::A,0,revision}).accepted,
          "repeating a previous Next cannot advance another question");
    } else {
      expect(!session.view().nextSolve && !next().accepted && session.activeSolve()==game,
          "the last prepared card stays complete and never wraps to the first");
    }
    expect(session.view().owners==groups.owners && session.view().undoDepth==groups.undoDepth,
        "Next never changes grouping or its Undo history");
  }
  apply(session,SorterActionKind::ReturnToSorter);
  expect(session.view().solveCandidate==3005,"returning from Next offers the last active question for Resume");
  apply(session,SorterActionKind::OpenSolve,1012);
  expect(session.activeSolve()->view().completed && !session.activeSolve()->question().currentRun().steps[0].attempts.empty(),
      "earlier finished working and answer evidence survive the whole sequence");
  apply(session,SorterActionKind::ReturnToSorter);apply(session,SorterActionKind::Undo);
  expect(session.view().counts[0]==100,"the original grouping transaction is still undoable");
}
void studySelection() {
  EquationSorterSession session(preparedContent(SORTER_BRACKET_FIXTURE));
  const auto act=[&](SorterActionKind kind,std::uint32_t value=0,SorterEquationId id=0) {
    const auto result=session.dispatch({kind,SorterBucket::A,id,session.view().revision,value});
    expect(result.accepted,"study action accepted");
  };
  apply(session,SorterActionKind::AutoSort);const auto groups=session.view();
  apply(session,SorterActionKind::OpenSolve,1012);help(*session.activeSolve(),GalleryHelpKind::Hint);
  apply(session,SorterActionKind::ReturnToSorter);act(SorterActionKind::OpenStudy);
  auto v=session.view();
  expect(v.studying && v.study.types.size()==1 && v.study.types[0].chapter=="Linear equations" &&
      v.study.types[0].title=="Bracket equations" && v.study.selectedCount==6,
      "authored titles expose the six prepared questions without guessing from text");
  act(SorterActionKind::ToggleStudySubject,static_cast<unsigned>(SorterSubject::Algebra));
  expect(!session.view().study.availableCount && !session.dispatch({SorterActionKind::StartStudy,SorterBucket::A,0,session.view().revision}).accepted,
      "an empty selection cannot replace a paused question or start a set");
  act(SorterActionKind::ToggleStudyChapter,0);
  const auto stale=session.view().revision;
  act(SorterActionKind::SetStudyMode,static_cast<unsigned>(StudyMode::Random));
  expect(!session.dispatch({SorterActionKind::SetStudyMode,SorterBucket::A,0,stale,0}).accepted,
      "old selection controls cannot change a newer draft");
  for(std::uint32_t count:{1U,3U,6U,100U}) {
    act(SorterActionKind::SetStudyCount,count);
    const auto selected=session.view().study.selected;
    expect(session.view().study.selectedCount==std::min(count,6U),"random selection is bounded by prepared availability");
    for(int i=0;i<10;++i)expect(session.view().study.selected==selected,"reading a random preview never rerolls it");
    act(SorterActionKind::ShuffleStudy);
    const auto preview=session.view().study;
    for(std::size_t i=0;i<100;++i)expect(!preview.selected[i] || preview.available[i],"random selection has no out-of-scope question");
  }
  for(auto count:{0U,101U})expect(!session.dispatch({SorterActionKind::SetStudyCount,SorterBucket::A,0,session.view().revision,count}).accepted,
      "invalid counts are refused");
  act(SorterActionKind::SetStudyMode,static_cast<unsigned>(StudyMode::All));
  for(const auto id:{3001U,3003U,3005U})act(SorterActionKind::ToggleStudyQuestion,0,id);
  expect(session.view().study.mode==StudyMode::Specific && session.view().study.selectedCount==3,
      "individual problem choices switch to a specific selection");
  act(SorterActionKind::StartStudy);
  const std::array expected{1012U,3002U,3004U};
  for(std::size_t i=0;i<expected.size();++i) {
    auto* game=session.activeSolve();v=session.view();
    expect(v.studyRun && v.solveNumber==i+1 && v.solveCount==3 && game->view().equation==session.content()[i*2].text,
        "the frozen set follows the selected questions in contents order");
    if(i==0) {
      expect(game->question().archivedRuns().size()==1 && game->question().currentRun().runNumber==2 &&
          game->question().archivedRuns()[0].steps[0].hintRequested,"starting a new set archives earlier work rather than discarding it");
      help(*game,GalleryHelpKind::DoStep);const auto challenge=game->view().challenge;
      act(SorterActionKind::ReturnToStudy);act(SorterActionKind::SetStudyMode,static_cast<unsigned>(StudyMode::All));
      expect(session.view().study.selectedCount==6,"returning lets the user edit the next draft");
      act(SorterActionKind::ResumeStudy);
      expect(session.activeSolve()==game && game->view().challenge==challenge && session.view().solveCount==3,
          "Resume retains the current question and frozen set despite draft edits");
    }
    expect(!session.dispatch({SorterActionKind::NextSolve,SorterBucket::A,0,v.revision}).accepted,
        "unfinished selected questions cannot be skipped");
    while(!game->view().completed)help(*game,GalleryHelpKind::DoStep);
    if(i+1<expected.size()) {
      expect(session.view().nextSolve==expected[i+1],"Next skips unselected prepared questions");
      apply(session,SorterActionKind::NextSolve);
    } else expect(!session.view().nextSolve && !session.dispatch({SorterActionKind::NextSolve,SorterBucket::A,0,session.view().revision}).accepted,
        "a selected set ends before later unselected questions and never wraps");
  }
  act(SorterActionKind::ReturnToStudy);act(SorterActionKind::StartStudy);
  expect(session.view().solveCount==6 && session.activeSolve()->view().step==1 && !session.activeSolve()->view().completed &&
      session.activeSolve()->question().archivedRuns().size()==2,"Start set begins fresh runs for the new draft while preserving history");
  act(SorterActionKind::ReturnToStudy);act(SorterActionKind::CloseStudy);
  expect(session.view().owners==groups.owners && session.view().undoDepth==groups.undoDepth,"selection and study never change grouping history");
  apply(session,SorterActionKind::Undo);expect(session.view().counts[0]==100,"grouping Undo survives the entire selection flow");

  auto content=preparedContent(SORTER_BRACKET_FIXTURE);
  // Synthetic catalogue labels isolate multi-title routing from mathematical content.
  content[2].study->type="Second type";
  for(std::size_t i=3;i<6;++i) {content[i].subject=SorterSubject::Trig;content[i].study->chapter="Second chapter";}
  EquationSorterSession multiple(std::move(content));
  const auto scope=[&](SorterActionKind kind,std::uint32_t value=0) {
    expect(multiple.dispatch({kind,SorterBucket::A,0,multiple.view().revision,value}).accepted,"multiple-title scope accepted");
  };
  scope(SorterActionKind::OpenStudy);
  expect(multiple.view().study.types.size()==3 && multiple.view().study.selectedCount==2,"distinct authored types form separate catalogue entries");
  scope(SorterActionKind::ToggleStudyChapter,0);expect(multiple.view().study.selectedCount==3,"a chapter selects all its types");
  scope(SorterActionKind::ToggleStudySubject,static_cast<unsigned>(SorterSubject::Trig));
  expect(multiple.view().study.selectedCount==6,"multiple subjects combine without duplicate problems");
  scope(SorterActionKind::ToggleStudyType,0);expect(multiple.view().study.selectedCount==4,"one type can be excluded without clearing another chapter");
  scope(SorterActionKind::ToggleStudyChapter,0);scope(SorterActionKind::ToggleStudySubject,static_cast<unsigned>(SorterSubject::Algebra));
  expect(multiple.view().study.selectedCount==3,"deselecting a subject removes only that subject");
  expect(!multiple.dispatch({SorterActionKind::ToggleStudyQuestion,SorterBucket::A,1012,multiple.view().revision}).accepted,
      "individual selection cannot bypass the chosen titles");
}
void linkedValues(const fm::LayeredQuestionSession& question,const fm::CoordinateGraphView& graph) {
  expect(graph.valueTable.has_value()==graph.probeAvailable,"sample values stay hidden until the graph can be explored");
  if(!graph.valueTable)return;
  const auto& rows=*graph.valueTable;const auto& g=graph.axes;
  expect(rows.front().x==graph.probeMin && rows.back().x==graph.probeMax && rows[0].x<rows[1].x && rows[1].x<rows[2].x,
      "three distinct sample inputs span the shared visible range in order");
  for(const auto& row:rows) {
    expect(std::isfinite(row.y) && row.y>=g.yMin-.0001F && row.y<=g.yMax+.0001F,"first table column remains finite and on the board");
    expect(std::abs(row.y*g.run-row.x*g.rise-g.intercept*g.run)<.0001F,"table y satisfies the first equation independently");
    expect(row.secondY.has_value()==g.second.has_value(),"only systems have a second y column");
    if(row.secondY) {
      const auto& line=*g.second;
      expect(std::isfinite(*row.secondY) && *row.secondY>=g.yMin-.0001F && *row.secondY<=g.yMax+.0001F,"second table column remains finite and on the board");
      expect(std::abs(*row.secondY*line.run-row.x*line.rise-line.intercept*line.run)<.0001F,"second table column satisfies its own equation");
    }
    const auto selected=*question.coordinateGraph(row.x);
    expect(selected.probe.x==row.x && selected.probe.y==row.y && (!row.secondY || selected.second->probe.y==*row.secondY),
        "selecting a table input projects exactly the listed coordinates");
    for(std::size_t i=0;i<rows.size();++i)expect((*selected.valueTable)[i].x==rows[i].x && (*selected.valueTable)[i].y==rows[i].y,
        "moving the guide does not change the prepared sample positions or values");
  }
}
void coordinateGraphs() {
  const auto content=loadSorterContent(SORTER_STUDY_FIXTURE);
  EquationSorterSession session(content);
  expect(session.view().study.types.size()==7,"combined contents retains graph types and includes all three new matrix practice types");
  for(std::uint32_t id=4001;id<=4004;++id) {
    apply(session,SorterActionKind::OpenSolve,id);auto& game=*session.activeSolve();
    const auto equation=std::string(game.view().equation);const auto& source=game.question().content();
    expect(source.lineGraph && game.question().coordinateGraph()->stage==fm::GraphStage::Grid,"every graph starts blank");
    const auto initial=game.view().challenge;
    choose(game,108);
    expect(game.view().step==1 && game.question().coordinateGraph()->stage==fm::GraphStage::Grid && game.view().wrongHits==1,
        "a wrong graph answer records the actual attempt and reveals no geometry");
    for(unsigned stage=0;stage<4;++stage) {
      const auto old=game.view().challenge;
      help(game,GalleryHelpKind::Hint);
      linkedValues(game.question(),*game.question().coordinateGraph());
      expect(static_cast<unsigned>(game.question().coordinateGraph()->stage)==stage,"hint does not reveal the next graph state");
      choose(game,101);
      expect(static_cast<unsigned>(game.question().coordinateGraph()->stage)==stage+1 && game.view().equation==equation,
          "one accepted answer reveals one graph stage with the original equation fixed");
      expect(!game.dispatch(ChooseAnswer{old,{101}}).accepted,"old graph choices cannot advance a new challenge");
    }
    const auto summary=fm::summarizeLayeredQuestionRun(game.question().currentRun());
    expect(game.view().completed && summary.correctOnFirstTry==3 && summary.correctedAfterRetry==1,"graph answers use the existing evidence owner");
    for(float x:{-1000.0F,-1.25F,0.0F,2.5F,1000.0F,std::numeric_limits<float>::quiet_NaN()}) {
      const auto graph=*game.question().coordinateGraph(x);const auto& g=graph.axes;
      linkedValues(game.question(),graph);
      expect(std::isfinite(graph.probe.x) && std::isfinite(graph.probe.y) &&
          graph.probe.x>=g.xMin-.0001F && graph.probe.x<=g.xMax+.0001F &&
          graph.probe.y>=g.yMin-.0001F && graph.probe.y<=g.yMax+.0001F,"probe clips safely to the visible line");
      for(const auto p:{graph.lineStart,graph.lineEnd,graph.tip,graph.probe})
        expect(std::abs(p.y*g.run-(p.x*g.rise+g.intercept*g.run))<.0001F,"every displayed line point obeys rise/run and intercept");
    }
    for(int i=0;i<100;++i)tick(game,.25F);
    expect(game.view().completed && game.view().correctHits==4 && game.view().equation==equation &&
        fm::summarizeLayeredQuestionRun(game.question().currentRun()).incorrectCheckedAttempts==1,"probing and time never change evidence or advance Next");
    apply(game,ReplayQuestion{});
    expect(game.question().coordinateGraph()->stage==fm::GraphStage::Grid && game.question().archivedRuns().size()==1,
        "Play again archives evidence and resets graph reveals");
    help(game,GalleryHelpKind::DoStep);
    expect(game.question().coordinateGraph()->stage==fm::GraphStage::Intercept && game.view().correctHits==4 &&
        step(game).attempts.empty(),"Do step reveals the prepared geometry without fabricating a new correct answer");
    auto invalid=source;
    invalid.lineGraph->run=0;
    expect(!fm::validateQuestion(invalid,fm::QuestionInteraction::ArcadeCollect).valid(),"zero run is rejected before projection");
    invalid=source;invalid.workingStates[2].graphStage=fm::GraphStage::Line;
    expect(!fm::validateQuestion(invalid,fm::QuestionInteraction::ArcadeCollect).valid(),"out-of-order reveal chain is rejected");
    invalid=source;invalid.lineGraph->yMax=invalid.lineGraph->intercept;
    expect(!fm::validateQuestion(invalid,fm::QuestionInteraction::ArcadeCollect).valid(),"off-board intercept is rejected");
    invalid=source;invalid.steps[0].semantics.purpose=fm::StepPurpose::Calculation;
    expect(!fm::validateQuestion(invalid,fm::QuestionInteraction::ArcadeCollect).valid(),"graph content cannot silently enter sphere calculations");
    const auto path=std::filesystem::path(SORTER_STUDY_FIXTURE).parent_path()/"../cards"/(source.id+".json");
    Json document;std::ifstream(path)>>document;document["line_graph"]["run"]=0;
    bool rejected=false;try {(void)parseQuestionContent(document.dump(),path);}catch(const QuestionContentError& e) {rejected=true;}
    expect(rejected,"JSON loader shares graph structural validation");
    expect(!game.dispatch(ChooseAnswer{initial,{101}}).accepted,"replay refuses prior-run choices");
    apply(session,SorterActionKind::ReturnToSorter);
  }
}
void simultaneousGraphs() {
  const auto content=loadSorterContent(SORTER_STUDY_FIXTURE);EquationSorterSession session(content);
  constexpr std::array stages{fm::GraphStage::Grid,fm::GraphStage::FirstLine,fm::GraphStage::BothLines,fm::GraphStage::Classified,fm::GraphStage::SystemSolution};
  constexpr std::array relations{fm::GraphRelation::Intersecting,fm::GraphRelation::Intersecting,fm::GraphRelation::Parallel,fm::GraphRelation::Coincident};
  for(std::uint32_t id=5001;id<=5004;++id) {
    apply(session,SorterActionKind::OpenSolve,id);auto& game=*session.activeSolve();
    const auto original=std::string(game.view().equation);
    for(std::size_t index=0;index<4;++index) {
      auto graph=*game.question().coordinateGraph(.75F);
      linkedValues(game.question(),graph);
      expect(graph.second && graph.stage==stages[index] && graph.probeAvailable==(index>=2),"two-line reveals gate the shared probe");
      expect(graph.relation.has_value()==(index>=3) && !graph.intersection,"classification and marked solution are withheld until reached");
      const auto challenge=game.view().challenge;const auto working=game.question().visibleWorkingId();
      help(game,GalleryHelpKind::Hint);
      expect(game.question().coordinateGraph()->stage==stages[index],"help alone never reveals a system stage");
      apply(game,GalleryPause{true});
      expect(!game.dispatch(ChooseAnswer{challenge,{101}}).accepted,"paused system choices cannot change evidence");
      apply(game,GalleryPause{false});choose(game,108);
      expect(game.question().coordinateGraph()->stage==stages[index] && game.question().visibleWorkingId()==working,
          "wrong system answers record attempts without revealing the next line or result");
      for(float x:{-1000.0F,-1.25F,0.0F,.75F,2.5F,1000.0F,std::numeric_limits<float>::quiet_NaN()}) {
        const auto view=*game.question().coordinateGraph(x);const auto& g=view.axes;
        linkedValues(game.question(),view);
        expect(view.probe.x==view.second->probe.x && std::isfinite(view.probe.y) && std::isfinite(view.second->probe.y),"both probe points share one finite x");
        const auto onLine=[&](fm::GraphPoint p,int rise,int run,int intercept) {
          expect(std::abs(p.y*run-(p.x*rise+intercept*run))<.0001F,"all clipped and probe points satisfy their own equation");
          expect(p.x>=g.xMin-.0001F && p.x<=g.xMax+.0001F && p.y>=g.yMin-.0001F && p.y<=g.yMax+.0001F,"every projected point remains on the authored board");
        };
        for(const auto p:{view.lineStart,view.lineEnd,view.probe})onLine(p,g.rise,g.run,g.intercept);
        for(const auto p:{view.second->start,view.second->end,view.second->probe})onLine(p,g.second->rise,g.second->run,g.second->intercept);
        if(id==5003)expect(std::abs(view.probe.y-view.second->probe.y-2)<.0001F,"parallel lines retain their gap for every probe x");
        if(id==5004)expect(view.probe.y==view.second->probe.y,"scaled coincident equations agree for every probe x");
      }
      expect(game.view().challenge==challenge && game.question().visibleWorkingId()==working && game.view().correctHits==index && game.view().wrongHits==index+1,
          "exploration never submits an answer or advances progress");
      choose(game,101);
      expect(game.question().coordinateGraph()->stage==stages[index+1] && game.view().equation==original,"one accepted choice advances one reveal and retains both original equations");
      expect(!game.dispatch(ChooseAnswer{challenge,{101}}).accepted,"stale system choices are refused");
    }
    const auto graph=*game.question().coordinateGraph();
    expect(game.view().completed && graph.relation==relations[id-5001] && graph.intersection.has_value()==(id<5003),"all three system outcomes are distinguished without inventing an intersection");
    if(id<5003) {
      const fm::GraphPoint expected=id==5001?fm::GraphPoint{1,3}:fm::GraphPoint{.75F,1.5F};
      expect(std::abs(graph.intersection->x-expected.x)<.0001F && std::abs(graph.intersection->y-expected.y)<.0001F,"integer and fractional intersections agree with independent solutions");
    }
    for(int i=0;i<100;++i)tick(game,.25F);
    expect(game.view().completed && game.view().correctHits==4 && game.view().wrongHits==4 && game.view().equation==original,"completed systems remain until explicit Next");
    auto invalid=game.question().content();invalid.lineGraph->second->run=0;
    const auto rejected=fm::validateQuestion(invalid,fm::QuestionInteraction::ArcadeCollect);
    expect(!rejected.valid() && rejected.field=="line_graph/second","bad second-line parameters fail at their owning field");
    invalid=game.question().content();invalid.workingStates[2].graphStage=fm::GraphStage::Line;
    expect(!fm::validateQuestion(invalid,fm::QuestionInteraction::ArcadeCollect).valid(),"single-line stages cannot enter a system reveal chain");
    invalid=game.question().content();invalid.lineGraph=fm::LineGraph{7,8,-8,-20,20,-20,20,fm::GraphLine{6,7,8}};
    expect(!fm::validateQuestion(invalid,fm::QuestionInteraction::ArcadeCollect).valid(),"an off-board intersection cannot appear to be a no-solution example");
    const auto path=std::filesystem::path(SORTER_STUDY_FIXTURE).parent_path()/"../cards"/(game.question().content().id+".json");
    Json document;std::ifstream(path)>>document;document["line_graph"]["second"]["run"]=0;
    bool refused=false;try {(void)parseQuestionContent(document.dump(),path);}catch(const QuestionContentError&) {refused=true;}
    expect(refused,"JSON decoding reuses the same second-line validation");
    apply(game,ReplayQuestion{});
    expect(game.question().coordinateGraph()->stage==fm::GraphStage::Grid && game.question().archivedRuns().size()==1,"fresh system attempts retain previous records and clear all reveals");
    for(int i=0;i<4;++i)help(game,GalleryHelpKind::DoStep);
    const auto summary=fm::summarizeLayeredQuestionRun(game.question().currentRun());
    expect(summary.completed && summary.shownAnswers==4 && summary.correctOnFirstTry==0,"assisted system completion adds no fabricated correct answers");
    apply(session,SorterActionKind::ReturnToSorter);
  }
}
int main() {
  try {contentBoundary();playerLoop();assistanceLoop();preparedFamily();nextPreparedProblem();studySelection();coordinateGraphs();simultaneousGraphs();std::cout<<"Prepared equations, single-line and simultaneous graphs, selection, assistance and replay passed\n";}
  catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}

#include "runtime/gallery/GallerySession.hpp"
#include "content/QuestionContentIO.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace {
using namespace paths;
namespace fm=iggy3d::first_move;
int failures=0;
void expect(bool condition,std::string_view message) {
  if(!condition) {++failures;std::cerr<<"FAIL: "<<message<<'\n';}
}
void apply(GallerySession& session,const GalleryCommand& command) {
  const auto result=session.dispatch(command);expect(result.accepted,result.reason);
}
void publish(GallerySession& session) {static_cast<void>(session.publishFrame());}
GallerySession makeGame(GalleryVariation variation,RouteKind motion=RouteKind::Stationary,
                        const std::filesystem::path& packPath=PATHS_TEST_PACK) {
  const auto pack=loadQuestionPack(packPath);
  const auto modes=galleryVariations();
  const auto mode=std::find_if(modes.begin(),modes.end(),[&](const auto& d){return d.variation==variation;});
  GallerySession game({variation,19,true,motion,0.7F},pack.catalog,pack.deck(mode->id));
  apply(game,GalleryViewport{{0,78,1030,822}});publish(game);
  expect(!game.view().ready,"new challenge is unavailable until all balls have spawned and been published");
  apply(game,GalleryTick{0.15F});publish(game);
  expect(game.view().ready,"published active balls open the challenge");
  return game;
}
Shoot shotFor(GallerySession& game,OptionId option) {
  publish(game);
  const auto view=game.view();
  const auto& current=game.challenges().back();
  const auto bindings=std::span(current.bindings.data(),current.count);
  const auto binding=std::find_if(bindings.begin(),bindings.end(),[&](auto b){return b.option==option;});
  if(binding==bindings.end())throw std::runtime_error("test fixture option has no target");
  const auto bodies=game.scene().objects();
  const auto body=std::find_if(bodies.begin(),bodies.end(),[&](const auto& o){return o.id==binding->object;});
  const auto point=game.scene().project(body->position);
  const auto hit=game.scene().hitTestPresentedFrame(game.scene().frame().id,point.x,point.y);
  expect(hit.accepted && hit.object==body->id,"shot goes through the real published sphere mesh");
  return {game.scene().frame().id,view.challenge,point.x,point.y};
}
OptionId correctOption(const GallerySession& game) {
  const auto& step=game.question().content().steps[game.question().currentRun().currentStep];
  return step.options[fm::firstAcceptedOption(step)].id;
}
DisplayToken tokenFor(const GallerySession& game,OptionId option) {
  const auto& current=game.challenges().back();
  for(std::size_t i=0;i<current.count;++i)if(current.bindings[i].option==option)return current.bindings[i].token;
  throw std::runtime_error("missing token");
}
void finishTransition(GallerySession& game) {
  apply(game,GalleryTick{0.25F});apply(game,GalleryTick{0.05F});publish(game);
  expect(!game.view().ready,"newly committed challenge waits for its own spawn");
  apply(game,GalleryTick{0.15F});publish(game);
  expect(game.view().ready,"next challenge publishes its complete mapping and active targets");
}
void testSweepAndStaleInput() {
  auto game=makeGame(GalleryVariation::EqualitySweep);
  const auto working=game.view().working;
  expect(game.view().required==2 && working=="Equal to 24", "AllAccepted board publishes its required set and initial working");
  const auto firstChallenge=game.view().challenge;
  const auto wrong=shotFor(game,{115});apply(game,wrong);
  expect(game.view().wrongHits==1 && !game.view().transitioning,"wrong answer records one attempt without a modal or transition");
  const auto correct=shotFor(game,{101});apply(game,correct);
  expect(game.view().correctHits==1 && game.view().collected==1 && game.view().challenge==firstChallenge,
    "first member collects under the same equality prompt");
  expect(game.view().working==working && !game.view().transitioning, "partial collection cannot reveal later working");
  expect(game.scene().selectedId()==SceneObjectId{},"shooting does not mutate editor selection");
  expect(!game.dispatch(correct).accepted && game.view().correctHits==1,"replayed correct shot cannot duplicate a judged fact");
  auto popping=shotFor(game,{101});
  expect(!game.dispatch(popping).accepted && game.view().correctHits==1,"visible popping ball cannot submit again");
  const auto second=shotFor(game,{108});apply(game,second);
  expect(game.view().correctHits==2 && game.view().transitioning,"last accepted member resolves exactly once before animation");
  expect(game.view().working==working, "resolving AllAccepted preserves working through the pop interval");
  apply(game,GalleryTick{0.25F});
  expect(game.view().challenge==firstChallenge,"question remains while its pop is still visible");
  apply(game,GalleryPause{true});
  const auto pausedTick=game.scene().tickCount();
  apply(game,GalleryTick{0.25F});publish(game);
  expect(game.scene().tickCount()==pausedTick && game.view().challenge==firstChallenge,"pause freezes pop and question transition together");
  expect(!game.dispatch(Shoot{game.scene().frame().id,game.view().challenge,0.5F,0.5F}).accepted,"pause rejects answers");
  apply(game,GalleryPause{false});apply(game,GalleryTick{0.05F});publish(game);
  expect(game.view().challenge!=firstChallenge && game.view().collected==0 && game.view().completedQuestions==1,
    "one commit resets the collected colours and changes question identity");
  expect(game.question().archivedRuns().size()==1 && game.question().archivedRuns()[0].steps[0].attempts.size()==3,
    "archived question retains both right answers and the wrong answer once");
  apply(game,GalleryTick{0.15F});publish(game);
  expect(!game.dispatch(Shoot{game.scene().frame().id,firstChallenge,correct.u,correct.v}).accepted,
    "old challenge is rejected even with the latest frame");
  expect(!game.dispatch(Shoot{correct.frame,game.view().challenge,correct.u,correct.v}).accepted,
    "old frame is rejected even with the latest challenge");
  expect(game.view().correctHits==2 && game.view().wrongHits==1 && game.view().priorExposure,
    "stale inputs cannot rewrite evidence or reset exposure");
  const auto tick=game.scene().tickCount();const auto records=game.challenges().size();
  for(int i=0;i<100;++i)static_cast<void>(game.view());
  expect(game.scene().tickCount()==tick && game.challenges().size()==records,"view reads do not move the run or draw another assignment");
}
void testRelayAndColourConstraint() {
  auto game=makeGame(GalleryVariation::QuestionRelay);
  expect(game.question().content().steps[0].semantics.completion==fm::CompletionRule::AnyAccepted &&
    game.view().required==1 && game.view().collected==0, "AnyAccepted relay projects a one-answer requirement");
  const auto previous=tokenFor(game,correctOption(game));
  const auto questionId=game.question().content().id;
  apply(game,shotFor(game,correctOption(game)));finishTransition(game);
  expect(game.question().content().id!=questionId && game.view().step==1 && game.view().completedQuestions==1,
    "relay advances to the other whole question");
  expect(tokenFor(game,correctOption(game))!=previous,"bounded assignment swap prevents repeating the successful colour");
  apply(game,shotFor(game,correctOption(game)));finishTransition(game);
  expect(game.question().content().id==questionId && game.view().priorExposure && game.view().completedQuestions==2,
    "two-question relay loops with retained evidence and explicit repeat exposure");
  expect(game.challenges().size()==3,"each new prompt records exactly one complete assignment");
  for(const auto& challenge:game.challenges()) {
    expect(challenge.count==4 && challenge.assignmentRuleVersion==1,"assignment records count and rule version");
    for(std::size_t i=0;i<challenge.count;++i)for(std::size_t j=0;j<i;++j)
      expect(challenge.bindings[i].option!=challenge.bindings[j].option && challenge.bindings[i].token!=challenge.bindings[j].token &&
        challenge.bindings[i].object!=challenge.bindings[j].object,"assignment is a complete bijection of options tokens and bodies");
  }
}
void testWorkingChangesWithChallengeCommit() {
  auto game=makeGame(GalleryVariation::EquationChain);
  const auto before=game.view();
  const auto shot=shotFor(game,correctOption(game));
  apply(game,shot);
  expect(game.view().transitioning && game.view().challenge==before.challenge && game.view().working==before.working,
    "resolving an operation preserves the old working and challenge during popping");
  finishTransition(game);
  const auto calculation=game.view();
  expect(calculation.challenge!=before.challenge && calculation.step==2 &&
    calculation.working=="2x + 3 - 3 = 11 - 3" && calculation.working==game.question().visibleWorking() &&
    calculation.prompt==game.question().content().steps[1].prompt && calculation.collected==0,
    "one challenge commit publishes its calculation prompt, prepared working, and reset answer set");
  expect(!game.dispatch(shot).accepted && game.view().working==calculation.working,
    "stale operation shot cannot change the new mathematical working");
  apply(game,shotFor(game,{108}));
  expect(game.view().wrongHits==1 && !game.view().transitioning && game.view().working==calculation.working,
    "wrong gallery calculation records evidence without a working transition");
  apply(game,shotFor(game,{101}));
  expect(game.view().working==calculation.working && game.view().transitioning,
    "correct gallery calculation waits for the existing pop and Continue boundary");
  finishTransition(game);
  expect(game.view().step==3 && game.view().working=="2x = 8" &&
    game.view().required==fm::requiredAnswerCount(game.question().content().steps[2]),
    "the next calculation reads working and completion requirements from the question owner");
  apply(game,shotFor(game,{115}));finishTransition(game);
  expect(game.view().step==1 && game.view().working==before.working && game.question().archivedRuns().size()==1 &&
    game.question().archivedRuns()[0].steps[1].attempts.size()==2,
    "endless restart restores initial working with the completed wrong/right evidence archived");
}
void testSubstitutionGallerySequence() {
  auto game=makeGame(GalleryVariation::SubstitutionChain);
  expect(game.question().content().id=="foundation_substitution_integral_6x" && game.view().stepCount==6,
    "substitution startup selects the file-loaded six-decision fixture");
  const std::array<std::string_view,7> working{
    "Integral of 6x(x^2 + 1)^2 dx", "Integral of 6x(x^2 + 1)^2 dx", "Integral of 6x(x^2 + 1)^2 dx",
    "3 * integral u^2 du", "u^3 + C", "(x^2 + 1)^3 + C", "Integral of 6x(x^2 + 1)^2 dx"};
  for(std::size_t i=0;i<6;++i) {
    const auto before=game.view();
    expect(before.step==i+1 && before.required==1 && before.working==working[i],
      "gallery presents the substitution decision and its prepared before state");
    if(i==1) {
      apply(game,shotFor(game,{108}));
      expect(game.view().wrongHits==1 && !game.view().transitioning && game.view().working==working[i],
        "wrong derivative hit preserves the original integral");
    }
    apply(game,shotFor(game,correctOption(game)));
    expect(game.view().transitioning && game.view().challenge==before.challenge && game.view().working==working[i],
      "the authored substitution step waits for popping before advancing");
    finishTransition(game);
    expect(game.view().challenge!=before.challenge && game.view().working==working[i+1],
      "substitution working and targets publish together after each authored decision");
  }
  const auto& archive=game.question().archivedRuns();
  expect(game.view().completedQuestions==1 && game.view().correctHits==6 && game.view().wrongHits==1 &&
    game.view().step==1 && game.view().priorExposure && archive.size()==1 && archive[0].steps.size()==6 &&
    archive[0].steps[1].attempts.size()==2 && archive[0].steps[5].resolvedByPlayer,
    "endless substitution play preserves all six decisions and the wrong derivative before restarting");
}
void checkSourceGallerySequence(GallerySession& game,std::string_view identity,std::span<const std::uint32_t> steps,
                                std::span<const std::uint32_t> correct,std::span<const std::string_view> working) {
  if(steps.size()!=correct.size() || steps.size()!=working.size())throw std::runtime_error("inconsistent source-card expectations");
  expect(game.question().content().id==identity && game.question().content().version==1 && game.view().stepCount==steps.size(),
    "the separate pack starts its source card with the original question identity and every authored decision");
  if(game.question().content().steps.size()!=steps.size())return;
  for(std::size_t i=0;i<steps.size();++i) {
    const auto before=game.view();
    expect(before.step==i+1 && before.required==1 && before.working==working[i] &&
      game.challenges().back().step.value==steps[i], "each authored decision starts with exactly its prepared working");
    const OptionId wrong{correct[i]==101?108U:101U};
    apply(game,shotFor(game,wrong));
    expect(game.view().working==working[i] && game.view().challenge==before.challenge &&
      !game.view().transitioning && game.view().collected==0 && game.view().wrongHits==i+1,
      "a wrong target in every source-card step preserves the working, answers and current challenge");
    const auto correctShot=shotFor(game,{correct[i]});apply(game,correctShot);
    expect(game.view().transitioning && game.view().working==working[i] && game.view().challenge==before.challenge,
      "a correct source-card choice waits for the pop before revealing the next working block");
    finishTransition(game);
    expect(game.view().working==working[(i+1)%steps.size()] && !game.dispatch(correctShot).accepted,
      "the next working and target assignment commit together, rejecting the previous step's shot");
  }
  const auto& archive=game.question().archivedRuns();
  expect(archive.size()==1 && archive[0].completed && archive[0].questionId==identity && archive[0].contentVersion==1 &&
    archive[0].steps.size()==steps.size() && game.view().completedQuestions==1 && game.view().correctHits==steps.size() &&
    game.view().wrongHits==steps.size() && game.view().aimMisses==0,
    "completion retains every wrong/correct pair under the source card's identity");
  if(archive.size()!=1 || archive[0].steps.size()!=steps.size())return;
  for(std::size_t i=0;i<archive[0].steps.size();++i) {
    const auto& record=archive[0].steps[i];
    expect(record.id.value==steps[i] && record.resolvedByPlayer && !record.answerShown &&
      record.attempts.size()==2 && !record.attempts[0].correct && record.attempts[1].correct &&
      record.attempts[1].option.value==correct[i], "archived decisions preserve stable step/option identities and judged attempts");
  }
  expect(game.view().step==1 && game.view().priorExposure && game.view().collected==0 && game.question().currentRun().runNumber==2,
    "endless play starts a fresh run of the same source card with repeat exposure recorded");
  for(const auto& step:game.question().currentRun().steps)
    expect(step.attempts.empty() && !step.resolvedByPlayer, "the new run does not inherit answers from its completed archive");
}
void testSource002GallerySequence() {
  const auto path=std::filesystem::path(PATHS_TEST_PACK).parent_path()/"source_002.json";
  auto game=makeGame(GalleryVariation::EquationChain,RouteKind::Stationary,path);
  constexpr std::array<std::uint32_t,13> steps{10,20,30,40,50,60,70,80,90,100,110,120,130};
  constexpr std::array<std::uint32_t,13> correct{108,115,122,101,115,108,122,101,115,108,122,101,115};
  constexpr std::array<std::string_view,13> working{
    "f(x) = a*x^2 + b*x + c\nGiven points: (-1, 1), (0, 0), (1, 2)",
    "f(x) = a*x^2 + b*x + c",
    "f(x) = a*x^2 + b*x + c\nEach observation supplies a value of x.",
    "a*x^2 + b*x + c", "a*0^2 + b*0 + c = 0", "a*(-1)^2 + b*(-1) + c = 1",
    "a*1^2 + b*1 + c = 2", "c = 0\na - b = 1\na + b = 2", "2a = 3", "3/2 + b = 2",
    "a = 3/2\nb = 1/2\nc = 0\nf(x) = a*x^2 + b*x + c", "f(-1) = 1\nf(0) = 0\nf(1) = 2",
    "Coefficient rows: [1, -1, 1], [0, 0, 1], [1, 1, 1]\ndeterminant = -2"};
  expect(game.view().equation.find("Three points, one formula")!=std::string_view::npos &&
    game.view().equation.find("Meckes & Meckes")!=std::string_view::npos &&
    game.view().equation.find("1.1.7")!=std::string_view::npos,
    "the persistent board identifies the guided adaptation and its source");
  checkSourceGallerySequence(game,"rb_math_002_quadratic_three_points_guided",steps,correct,working);
}
void testSource013GallerySequence() {
  const auto path=std::filesystem::path(PATHS_TEST_PACK).parent_path()/"source_013.json";
  auto game=makeGame(GalleryVariation::EquationChain,RouteKind::Stationary,path);
  constexpr std::array<std::uint32_t,14> steps{10,20,30,40,50,60,70,80,90,100,110,120,130,140};
  constexpr std::array<std::uint32_t,14> correct{122,108,101,115,122,108,101,122,115,108,101,122,108,115};
  constexpr std::array<std::string_view,14> working{
    "Target point: ((a+m*b)/(1+m^2), m*(a+m*b)/(1+m^2))",
    "Line: y = m*x\nGiven point: p = (a,b)", "Moving point: (t,m*t)", "Let t be the first coordinate.",
    "U = span{(1,m)}\nClosest means Euclidean distance.", "v = (1,m)\nm is any real number.",
    "p = (a,b)\nv = (1,m)", "p dot v = a*1 + b*m", "v dot v = 1*1 + m*m",
    "t = (a+m*b)/(1+m^2)\nq = t*(1,m)", "q = t*(1,m)\nt = (a+m*b)/(1+m^2)\np-q = (a-t, b-m*t)",
    "p-s*v = (p-t*v) + (t-s)*v\nThe two terms on the right are perpendicular.",
    "Distance gap = (s-t)^2*(1+m^2)\n1+m^2 > 0 for every real m.",
    "Checked examples agree with the formula.\nThe exercise concerns every real m, a, and b."};
  expect(game.view().equation.find("Guided derivation - target supplied")!=std::string_view::npos &&
    game.view().equation.find("unique Euclidean closest")!=std::string_view::npos &&
    game.view().equation.find("Meckes & Meckes, Ch. 4, ex. 4.3.9")!=std::string_view::npos,
    "the board states the derivation objective, distance and source without claiming independent proof writing");
  checkSourceGallerySequence(game,"rb_math_013_closest_point_line_guided",steps,correct,working);
}
void testEquationOrderAndRandomIsolation() {
  auto quiet=makeGame(GalleryVariation::EquationChain);
  auto moving=makeGame(GalleryVariation::EquationChain,RouteKind::SeededRoam);
  for(std::size_t step=1;step<=3;++step) {
    expect(quiet.view().step==step && moving.view().step==step,"equation follows authored step order");
    for(std::size_t i=0;i<4;++i)expect(quiet.view().answers[i].binding.option==moving.view().answers[i].binding.option,
      "roaming and render reads cannot change the assignment stream");
    apply(quiet,shotFor(quiet,correctOption(quiet)));apply(moving,shotFor(moving,correctOption(moving)));
    finishTransition(quiet);finishTransition(moving);
  }
  expect(quiet.view().completedQuestions==1 && quiet.view().correctHits==3 && quiet.view().step==1,
    "last equation step archives the full problem before starting its repeat");
  const auto& run=quiet.question().archivedRuns()[0];
  expect(run.steps.size()==3 && run.steps[0].attempts[0].option.value==108 &&
    run.steps[1].attempts[0].option.value==101 && run.steps[2].attempts[0].option.value==115,
    "equation evidence preserves the actual accepted operations in order");
}
void testUninterruptedFeedback() {
  auto game=makeGame(GalleryVariation::QuestionRelay,RouteKind::Horizontal);
  const auto challenge=game.view().challenge;
  const auto start=game.scene().objects()[0].position;
  const auto wrong=[&] {
    apply(game,shotFor(game,{correctOption(game).value==101?108U:101U}));
  };
  expect(game.view().feedback==GalleryFeedback::None,"a fresh challenge has no persistent instruction feedback");
  for(int i=0;i<3;++i) {wrong();apply(game,GalleryTick{0.1F});}
  expect(game.view().wrongHits==3 && game.view().challenge==challenge && game.view().ready &&
    !game.view().paused && !game.view().transitioning && game.view().feedback==GalleryFeedback::Incorrect,
    "repeated wrong clicks keep the same question active without a pause, correction dialog or input lock");
  expect(!iggy3d::nearlyEqual(start,game.scene().objects()[0].position),"targets keep moving through wrong-answer feedback");
  apply(game,shotFor(game,correctOption(game)));
  expect(game.view().correctHits==1 && game.view().feedback==GalleryFeedback::Correct && game.view().transitioning,
    "the next correct shot is accepted while wrong feedback was still visible");
  finishTransition(game);
  expect(game.view().completedQuestions==1 && game.view().wrongHits==3 && game.view().ready &&
    game.view().feedback==GalleryFeedback::None,"the next question starts automatically with no carried feedback or correction round");
  wrong();const auto nextChallenge=game.view().challenge;
  apply(game,GalleryTick{0.25F});
  expect(game.view().feedback==GalleryFeedback::Incorrect,"feedback is briefly visible without stopping play");
  apply(game,GalleryPause{true});const auto tick=game.scene().tickCount();
  apply(game,GalleryTick{0.25F});
  expect(game.scene().tickCount()==tick && game.view().wrongHits==4 && game.view().correctHits==1,
    "stopping preserves the accumulated results and common clock");
  apply(game,GalleryPause{false});apply(game,GalleryTick{0.25F});publish(game);
  expect(game.view().feedback==GalleryFeedback::None && game.view().challenge==nextChallenge && game.view().ready,
    "wrong feedback disappears after half a second of active play without advancing or locking the question");
  apply(game,Shoot{game.scene().frame().id,game.view().challenge,0.01F,0.99F});
  expect(game.view().feedback==GalleryFeedback::Miss && game.view().aimMisses==1 && game.view().wrongHits==4,
    "an aiming miss gets a brief signal without becoming a wrong mathematical answer");
  wrong();
  expect(game.view().feedback==GalleryFeedback::Incorrect && game.view().wrongHits==5,
    "further clicks immediately replace the previous signal and keep recording attempts");
}
void testClockAndFailedPreparation() {
  auto a=makeGame(GalleryVariation::QuestionRelay,RouteKind::Circle);
  auto b=makeGame(GalleryVariation::QuestionRelay,RouteKind::Circle);
  for(int i=0;i<60;++i)apply(a,GalleryTick{1.0F/60});
  for(int i=0;i<120;++i)apply(b,GalleryTick{1.0F/120});
  expect(a.scene().tickCount()==b.scene().tickCount(),"session retains a single scene clock across render rates");
  for(std::size_t i=0;i<a.scene().objects().size();++i)
    expect(iggy3d::nearlyEqual(a.scene().objects()[i].position,b.scene().objects()[i].position),"sphere motion is render-rate independent");
  auto config=a.config();config.pace=-1;
  bool rejected=false;
  try {
    const auto pack=loadQuestionPack(PATHS_TEST_PACK);
    GallerySession invalid(config,pack.catalog,pack.deck("question_relay"));
  } catch(const std::invalid_argument&) {rejected=true;}
  expect(rejected,"invalid initial route cannot publish a partial challenge");
  const auto before=a.scene().tickCount();
  expect(!a.dispatch(GalleryTick{1}).accepted && a.scene().tickCount()==before,"invalid gallery delta is rejected before scene mutation");
  const auto viewport=a.scene().frame().viewport;
  expect(!a.dispatch(GalleryViewport{{0,0,0,600}}).accepted && a.scene().frame().viewport==viewport,
    "invalid viewport is rejected without replacing the published context");
  GalleryScene scene(false);
  const std::array<iggy3d::Vec3,4> good{{{1,0,0},{0,1,0},{0,0,1},{1,1,0}}};
  RouteSpec spec;spec.kind=RouteKind::Circle;
  expect(scene.dispatch(ResetTargets{good,spec}).accepted,"target batch prepares");
  const auto id=scene.objects()[0].id;const auto pose=scene.objects()[0].position;
  auto bad=good;bad[3]={2,0,0};
  expect(!scene.dispatch(ResetTargets{bad,spec}).accepted && scene.objects()[0].id==id &&
    iggy3d::nearlyEqual(scene.objects()[0].position,pose),"failure in the final slot cannot partially reset earlier targets");
}
static_assert(std::is_nothrow_move_assignable_v<GalleryScene>);
static_assert(std::is_nothrow_move_assignable_v<fm::LayeredQuestionSession>);
}
int main() {
  testUninterruptedFeedback();
  testSweepAndStaleInput();testRelayAndColourConstraint();testEquationOrderAndRandomIsolation();testClockAndFailedPreparation();
  testWorkingChangesWithChallengeCommit();
  testSubstitutionGallerySequence();
  testSource002GallerySequence();
  testSource013GallerySequence();
  if(failures)return 1;
  std::cout<<"paths_gallery_tests: shared visible shots, collection, atomic challenge reset, replay guards and common clock passed\n";
}

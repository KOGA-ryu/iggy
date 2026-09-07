#include "ui/GalleryMenu.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>

namespace {
using namespace paths;
using Kind=GalleryMenuActionKind;
namespace fs=std::filesystem;
int failures=0;
void expect(bool value,const char* message) {if(!value){++failures;std::cerr<<"FAIL: "<<message<<'\n';}}
void act(GalleryMenu& menu,GalleryMenuAction action) {
  expect(menu.dispatch(action),"menu action accepted");
  if(!menu.error().empty())std::cerr<<menu.error()<<'\n';
}
GalleryConfig config() {GalleryConfig c;c.seed=19;c.motion=RouteKind::Stationary;return c;}
const fs::path packs=fs::path(PATHS_TEST_PACK).parent_path();
void ready(GallerySession& game) {
  expect(game.dispatch(GalleryViewport{{0,78,1051,822}}).accepted,"game viewport prepares");
  static_cast<void>(game.publishFrame());
  expect(game.dispatch(GalleryTick{0.15F}).accepted,"target spawning advances");
  static_cast<void>(game.publishFrame());
  expect(game.view().ready,"targets ready after the first presented frame");
}
void shoot(GallerySession& game,std::uint32_t option) {
  const auto& assignment=game.challenges().back();
  const auto binding=std::find_if(assignment.bindings.begin(),assignment.bindings.begin()+assignment.count,
    [&](const auto& b){return b.option.value==option;});
  if(binding==assignment.bindings.begin()+assignment.count)throw std::runtime_error("missing target");
  const auto bodies=game.scene().objects();
  const auto body=std::find_if(bodies.begin(),bodies.end(),[&](const auto& b){return b.id==binding->object;});
  const auto p=game.scene().project(body->position);
  expect(game.dispatch(Shoot{game.scene().frame().id,game.view().challenge,p.x,p.y}).accepted,"shot judged through the gallery");
}
void testSwitchAndResume() {
  GalleryMenu menu(packs,config());
  expect(menu.screen()==GalleryScreen::ChooseQuestions && !menu.activeGame() && menu.games().empty(),"default menu has no accidental game run");
  expect(menu.packs().size()==3 && menu.packs()[1].id=="source_002" && menu.packs()[2].id=="source_013","three named packs are offered");
  act(menu,{Kind::SelectPack,0});
  expect(menu.modes().size()==4,"starter practice types come from its actual deck list");
  act(menu,{Kind::Play});
  auto* sweep=menu.activeGame();ready(*sweep);shoot(*sweep,101);
  expect(sweep->view().collected==1 && sweep->view().required==2,"starter collection begins normally");
  const auto tick=sweep->scene().tickCount();
  act(menu,{Kind::ChooseQuestions});
  expect(sweep->scene().paused() && menu.canResume(),"returning to the menu pauses and offers resume");
  expect(sweep->dispatch(GalleryTick{0.25F}).accepted && sweep->scene().tickCount()==tick,"menu cannot advance the paused game clock");
  act(menu,{Kind::SelectPack,1});
  expect(menu.modes().size()==1 && menu.selectedMode()==GalleryVariation::EquationChain,"source 002 only offers its authored chain");
  act(menu,{Kind::Play});auto* quadratic=menu.activeGame();ready(*quadratic);shoot(*quadratic,101);
  expect(quadratic->question().content().id=="rb_math_002_quadratic_three_points_guided" && quadratic->view().wrongHits==1,
    "source 002 starts through the loader and records a wrong answer");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectPack,2});act(menu,{Kind::Play});
  auto* projection=menu.activeGame();ready(*projection);
  expect(projection->view().stepCount==14 && projection->question().content().id=="rb_math_013_closest_point_line_guided",
    "source 013 starts with its fourteen authored decisions");
  expect(!menu.dispatch({Kind::SelectPack,0}),"selection cannot silently switch a running game");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectPack,1});act(menu,{Kind::Play});
  expect(menu.activeGame()==quadratic && quadratic->view().wrongHits==1 && !quadratic->scene().paused() &&
    quadratic->question().currentRun().steps[0].attempts.size()==1,"resume keeps the original session and attempts");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectPack,0});act(menu,{Kind::Play});
  expect(menu.activeGame()==sweep && sweep->view().collected==1 && sweep->view().correctHits==1 && menu.games().size()==3,
    "switching back keeps a partially collected answer set without a duplicate run");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectMode,3});act(menu,{Kind::Play});
  expect(menu.activeGame()->config().variation==GalleryVariation::SubstitutionChain && menu.games().size()==4,
    "another starter practice type gets an independent game");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::Workshop});
  expect(menu.screen()==GalleryScreen::Workshop && menu.activeGame()->scene().paused(),"developer workshop is reachable without losing question sessions");
  act(menu,{Kind::ChooseQuestions});
  expect(!menu.dispatch({Kind::SelectPack,99}) && !menu.dispatch({Kind::SelectMode,99}),"invalid menu indices are rejected");
}
struct Scratch {
  fs::path root=fs::temp_directory_path()/("paths-menu-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Scratch() {fs::create_directories(root);fs::copy(packs.parent_path()/"cards",root/"cards",fs::copy_options::recursive);fs::copy(packs,root/"packs",fs::copy_options::recursive);}
  ~Scratch(){std::error_code error;fs::remove_all(root,error);}
};
void testFailureAndFrozenResume() {
  Scratch scratch;
  GalleryMenu menu(scratch.root/"packs",config());
  act(menu,{Kind::SelectPack,1});act(menu,{Kind::Play});auto* original=menu.activeGame();
  act(menu,{Kind::ChooseQuestions});
  const auto broken=scratch.root/"packs/source_013.json";
  std::ofstream(broken)<<"{";
  expect(!menu.dispatch({Kind::SelectPack,2}) && menu.screen()==GalleryScreen::ChooseQuestions && menu.modes().empty() &&
    menu.error().find("source_013.json")!=std::string::npos,"a broken pack stays in the menu with a source diagnostic");
  expect(!menu.dispatch({Kind::Play}) && menu.activeGame()==original && menu.games().size()==1,"failed Play preserves the previous game");
  fs::copy_file(packs/"source_013.json",broken,fs::copy_options::overwrite_existing);
  act(menu,{Kind::SelectPack,2});act(menu,{Kind::Play});
  expect(menu.error().empty() && menu.games().size()==2,"retry loads a repaired pack without restarting the app");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectPack,1});
  fs::remove(scratch.root/"packs/source_002.json");
  act(menu,{Kind::Play});
  expect(menu.activeGame()==original && menu.games().size()==2,"resume retains frozen content even if its source file disappears");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectPack,0});
  nlohmann::json pack;
  {std::ifstream input(scratch.root/"packs/gallery_foundation.json");input>>pack;}
  pack["decks"].erase("equality_sweep");
  std::ofstream(scratch.root/"packs/gallery_foundation.json")<<pack;
  act(menu,{Kind::SelectPack,0});
  expect(menu.modes().size()==3,"available modes follow edited deck contents rather than UI guesses");
  expect(!menu.launch(GalleryVariation::EqualitySweep) && menu.error().find("/decks/equality_sweep")!=std::string::npos,
    "explicit startup keeps missing-deck rejection instead of choosing a fallback");
}
void testDirectStartup() {
  GalleryMenu known(packs,config(),packs/"source_013.json");
  expect(known.packs().size()==3 && known.selectedPack()==2,"a bundled CLI pack selects its existing menu entry");
  expect(known.launch(GalleryVariation::EquationChain) && known.activeGame()->view().stepCount==14,"direct CLI launch shares the Play boundary");
  Scratch scratch;
  GalleryMenu custom(packs,config(),scratch.root/"packs/source_002.json");
  expect(custom.packs().size()==4 && custom.selectedPack()==3 && custom.launch(GalleryVariation::EquationChain),"an explicit custom pack remains playable and resumable");
  GalleryMenu absent(scratch.root/"absent",config());
  expect(!absent.dispatch({Kind::SelectPack,0}) && absent.screen()==GalleryScreen::ChooseQuestions && !absent.error().empty(),
    "missing bundled content does not prevent the menu from opening");
  act(absent,{Kind::Workshop});
  expect(absent.screen()==GalleryScreen::Workshop,"workshop does not require question files");
}
GalleryMenuAction motion(RouteKind kind) {
  const auto routes=targetRouteDescriptors();
  const auto found=std::find_if(routes.begin(),routes.end(),[&](const auto& route){return route.kind==kind;});
  return {Kind::SelectMotion,static_cast<std::size_t>(found-routes.begin())};
}
void testMovementChoices() {
  std::size_t count=0;
  for(const auto& route:targetRouteDescriptors()) {
    if(route.kind==RouteKind::Waypoints)continue;
    ++count;
    GalleryMenu menu(packs,config());act(menu,{Kind::SelectPack,0});
    act(menu,motion(route.kind));act(menu,{Kind::SetPace,0,0.6F});act(menu,{Kind::Play});
    auto& game=*menu.activeGame();
    expect(game.config().motion==route.kind && game.config().pace==0.6F,"Play uses the selected movement and speed");
    const auto initial=game.scene().objects().front().position;
    ready(game);
    for(int i=0;i<4;++i)expect(game.dispatch(GalleryTick{0.25F}).accepted,"chosen movement advances in play");
    static_cast<void>(game.publishFrame());
    const bool moved=!iggy3d::nearlyEqual(initial,game.scene().objects().front().position);
    expect(moved==(route.kind!=RouteKind::Stationary),"every moving preset moves; stationary stays still");
    shoot(game,115);shoot(game,101);
    expect(game.view().wrongHits==1 && game.view().correctHits==1 && game.view().collected==1,
      "moving targets retain wrong-answer judgment and partial collection");
  }
  expect(count==13,"thirteen gameplay movement choices are exercised");
  GalleryMenu slow(packs,config()),fast(packs,config());
  for(auto* menu:{&slow,&fast}){act(*menu,{Kind::SelectPack,0});act(*menu,motion(RouteKind::Horizontal));}
  act(slow,{Kind::SetPace,0,0.5F});act(fast,{Kind::SetPace,0,1.0F});
  act(slow,{Kind::Play});act(fast,{Kind::Play});ready(*slow.activeGame());ready(*fast.activeGame());
  const auto& a=slow.activeGame()->scene().objects().front();
  const auto& b=fast.activeGame()->scene().objects().front();
  expect(std::abs((b.position.x-b.origin.x)-2*(a.position.x-a.origin.x))<0.00001F,
    "double speed produces double displacement before the first turn");
  const auto& bindings=slow.activeGame()->challenges().back();
  const auto& faster=fast.activeGame()->challenges().back();
  for(std::size_t i=0;i<bindings.count;++i)expect(bindings.bindings[i].option==faster.bindings[i].option &&
    bindings.bindings[i].token==faster.bindings[i].token,"speed does not change the seeded answer assignment");
}
void testSetupValidation() {
  GalleryMenu menu(packs,config());act(menu,{Kind::SelectPack,0});
  for(float pace:{0.0F,-1.0F,100.1F,std::numeric_limits<float>::infinity(),
                 -std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
    auto invalid=config();invalid.pace=pace;
    expect(!validateGalleryConfig(invalid).accepted,"shared game validation rejects invalid speeds");
    expect(!menu.dispatch({Kind::SetPace,0,pace}) && menu.selectedConfig().pace==config().pace && menu.games().empty(),
      "invalid speed leaves the pending setup and sessions unchanged");
    GalleryMenu direct(packs,invalid);
    expect(!direct.launch(GalleryVariation::EqualitySweep) && !direct.activeGame(),"game construction enforces the same speed limits");
  }
  expect(!menu.dispatch(motion(RouteKind::Waypoints)) && menu.selectedConfig().motion==RouteKind::Stationary,
    "custom waypoints cannot become a gameplay preset");
  expect(!menu.dispatch({Kind::SelectMotion,std::numeric_limits<std::size_t>::max()}),"an invalid movement index cannot wrap into a valid route");
  for(const auto route:{RouteKind::Waypoints,RouteKind::Count,static_cast<RouteKind>(255)}) {
    auto invalid=config();invalid.motion=route;
    GalleryMenu direct(packs,invalid);
    expect(!validateGalleryConfig(invalid).accepted && !direct.launch(GalleryVariation::EqualitySweep),
      "shared validation and construction reject custom or unknown routes");
  }
  auto invalid=config();invalid.variation=static_cast<GalleryVariation>(255);
  expect(!validateGalleryConfig(invalid).accepted,"shared configuration validation rejects an unknown practice type");
  for(float pace:{0.005F,100.0F}) {
    GalleryMenu boundary(packs,config());act(boundary,{Kind::SelectPack,0});
    act(boundary,motion(RouteKind::Horizontal));act(boundary,{Kind::SetPace,0,pace});act(boundary,{Kind::Play});
    expect(boundary.activeGame()->scene().objects().front().route.spec.settings.speedMetersPerSecond==pace,
      "small positive and maximum engine speeds reach the prepared route without a UI clamp");
  }
}
void testFrozenMovementSetup() {
  auto cli=config();cli.motion=RouteKind::Sine;cli.pace=1.5F;cli.seed=27;
  GalleryMenu menu(packs,cli);act(menu,{Kind::SelectPack,0});
  expect(menu.selectedConfig().motion==cli.motion && menu.selectedConfig().pace==cli.pace && menu.selectedConfig().seed==cli.seed,
    "menu inherits command-line movement, speed and seed");
  act(menu,{Kind::Play});auto* original=menu.activeGame();ready(*original);shoot(*original,101);
  const auto tick=original->scene().tickCount(),challenge=original->view().challenge.value;
  const auto position=original->scene().objects().front().position;
  expect(!menu.dispatch(motion(RouteKind::Circle)) && !menu.dispatch({Kind::SetPace,0,3}),"gameplay rejects setup editing");
  act(menu,{Kind::ChooseQuestions});
  expect(!menu.dispatch(motion(RouteKind::Circle)) && !menu.dispatch({Kind::SetPace,0,3}) && menu.error().find("Resume keeps")!=std::string::npos,
    "Resume selection rejects setup editing with an explanation at the dispatcher");
  act(menu,{Kind::SelectPack,1});act(menu,motion(RouteKind::Circle));act(menu,{Kind::SetPace,0,2.4F});
  act(menu,{Kind::Play});auto* other=menu.activeGame();
  expect(other!=original && other->config().motion==RouteKind::Circle && other->config().pace==2.4F,"new pack receives its chosen setup");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectPack,0});
  expect(menu.canResume() && menu.selectedConfig().motion==RouteKind::Sine && menu.selectedConfig().pace==1.5F,
    "Resume shows that game's saved setup rather than the next-game draft");
  act(menu,{Kind::Play});
  expect(menu.activeGame()==original && original->scene().tickCount()==tick && original->view().challenge.value==challenge &&
    iggy3d::nearlyEqual(original->scene().objects().front().position,position) && original->view().collected==1 &&
    original->question().currentRun().steps[0].attempts.size()==1,"Resume preserves motion position, time, challenge and attempts");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectMode,1});
  expect(!menu.canResume() && menu.selectedConfig().motion==RouteKind::Circle && menu.selectedConfig().pace==2.4F,
    "an unstarted practice type keeps the pending setup across pack switches");
  act(menu,{Kind::SetPace,0,0.9F});act(menu,{Kind::Play});
  expect(menu.games().size()==3 && menu.activeGame()->config().pace==0.9F && original->config().pace==1.5F && other->config().pace==2.4F,
    "each started game owns its immutable speed");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::Workshop});
  expect(!menu.dispatch(motion(RouteKind::Vertical)) && !menu.dispatch({Kind::SetPace,0,5}),"workshop edits cannot use the game-setup route");
}
void testNewGameAndCancel() {
  GalleryMenu menu(packs,config());act(menu,{Kind::SelectPack,0});
  expect(!menu.dispatch({Kind::NewGame}) && !menu.dispatch({Kind::StartNewGame}) && !menu.dispatch({Kind::CancelNewGame}),
    "replacement actions require a started game and an explicit setup");
  act(menu,{Kind::Play});auto* original=menu.activeGame();ready(*original);shoot(*original,115);shoot(*original,101);
  const auto tick=original->scene().tickCount();const auto position=original->scene().objects().front().position;
  expect(!menu.dispatch({Kind::NewGame}),"a running game cannot be replaced directly");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectMode,1});act(menu,{Kind::Play});
  auto* relay=menu.activeGame();ready(*relay);const auto relayTick=relay->scene().tickCount();
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectPack,1});
  act(menu,motion(RouteKind::Circle));act(menu,{Kind::SetPace,0,2.4F});act(menu,{Kind::Play});
  auto* quadratic=menu.activeGame();ready(*quadratic);shoot(*quadratic,101);
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectPack,0});act(menu,{Kind::SelectMode,0});
  act(menu,{Kind::NewGame});
  expect(menu.preparingNewGame() && menu.canResume() && menu.selectedConfig().motion==RouteKind::Stationary &&
    menu.selectedConfig().pace==config().pace,"replacement draft starts from the selected game, not the last active game's setup");
  act(menu,motion(RouteKind::Horizontal));act(menu,{Kind::SetPace,0,1.25F});
  expect(!menu.dispatch({Kind::SetPace,0,0}) && menu.selectedConfig().pace==1.25F,"invalid replacement settings preserve the draft");
  for(const auto action:{GalleryMenuAction{Kind::SelectPack,1},GalleryMenuAction{Kind::SelectMode,1},
      GalleryMenuAction{Kind::Workshop},GalleryMenuAction{Kind::ChooseQuestions},GalleryMenuAction{Kind::NewGame},GalleryMenuAction{Kind::Play}})
    expect(!menu.dispatch(action),"setup cannot switch its replacement target or bypass confirmation");
  expect(!menu.launch(GalleryVariation::QuestionRelay),"direct launch cannot bypass an open replacement setup");
  act(menu,{Kind::CancelNewGame});
  expect(!menu.preparingNewGame() && menu.selectedConfig().motion==RouteKind::Stationary &&
    original->view().collected==1 && original->view().wrongHits==1 && original->scene().tickCount()==tick &&
    iggy3d::nearlyEqual(original->scene().objects().front().position,position),"Cancel preserves the old progress, position and clock");
  act(menu,{Kind::SelectMode,2});
  expect(menu.selectedConfig().motion==RouteKind::Circle && menu.selectedConfig().pace==2.4F,
    "cancelled replacement edits do not leak into the next-game draft");
  act(menu,{Kind::SelectMode,0});act(menu,{Kind::Play});
  expect(menu.activeGame()==original && original->question().currentRun().steps[0].attempts.size()==2,
    "the cancelled game's original answer history resumes");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::NewGame});
  act(menu,motion(RouteKind::Horizontal));act(menu,{Kind::SetPace,0,1.25F});act(menu,{Kind::StartNewGame});
  auto& fresh=*menu.activeGame();
  expect(menu.games().size()==3 && !menu.preparingNewGame() && menu.screen()==GalleryScreen::Playing &&
    fresh.config().motion==RouteKind::Horizontal && fresh.config().pace==1.25F && fresh.config().seed==19,
    "confirmed replacement uses the chosen setup without adding another retained slot");
  expect(fresh.view().step==1 && fresh.view().correctHits==0 && fresh.view().wrongHits==0 && fresh.view().collected==0 &&
    fresh.view().completedQuestions==0 && fresh.scene().tickCount()==0 && !fresh.view().ready &&
    fresh.question().currentRun().steps[0].attempts.empty() && fresh.question().archivedRuns().empty(),
    "replacement starts from the first question with fresh targets and no previous progress or attempts");
  expect(relay->scene().paused() && relay->scene().tickCount()==relayTick && quadratic->scene().paused() &&
    quadratic->view().wrongHits==1,"other practice types and packs retain their sessions and evidence");
  expect(!menu.dispatch({Kind::StartNewGame}),"a repeated start action cannot replace the new game again");
  ready(fresh);shoot(fresh,101);expect(fresh.view().collected==1,"the new game accepts normal judged shots after spawning");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::SelectMode,2});
  expect(menu.selectedConfig().pace==1.25F,"successful replacement promotes its choices to the next-game draft");
  act(menu,{Kind::SelectMode,1});act(menu,{Kind::Play});
  static_cast<void>(relay->publishFrame());
  const auto& step=relay->question().content().steps[0];
  shoot(*relay,step.options[iggy3d::first_move::firstAcceptedOption(step)].id.value);
  for(float seconds:{0.25F,0.05F,0.15F})expect(relay->dispatch(GalleryTick{seconds}).accepted,"relay advances to its next question");
  expect(relay->view().completedQuestions==1 && !relay->question().archivedRuns().empty() &&
    relay->question().content().id=="foundation_relay_notation","restart fixture has completed evidence and a later deck question");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::NewGame});act(menu,{Kind::StartNewGame});
  expect(menu.activeGame()->question().content().id=="foundation_relay_add" && menu.activeGame()->view().completedQuestions==0 &&
    menu.activeGame()->question().archivedRuns().empty(),"New game resets the deck position, completed count and previous run history");
}
void testFailedReplacement() {
  Scratch scratch;const auto pack=scratch.root/"packs/source_002.json";
  GalleryMenu menu(scratch.root/"packs",config());act(menu,{Kind::SelectPack,1});act(menu,{Kind::Play});
  auto* original=menu.activeGame();ready(*original);shoot(*original,101);shoot(*original,108);
  expect(original->view().transitioning,"replacement failure fixture has an unfinished pop");
  const auto tick=original->scene().tickCount();act(menu,{Kind::ChooseQuestions});act(menu,{Kind::NewGame});
  act(menu,motion(RouteKind::Circle));act(menu,{Kind::SetPace,0,2});
  for(int failure=0;failure<3;++failure) {
    switch(failure) {
      case 0:fs::remove(pack);break;
      case 1:std::ofstream(pack)<<"{";break;
      case 2:{nlohmann::json data;std::ifstream(packs/"source_002.json")>>data;data["decks"].erase("equation_chain");std::ofstream(pack)<<data;break;}
    }
    expect(!menu.dispatch({Kind::StartNewGame}) && !menu.error().empty(),"replacement reports missing, malformed or incompatible pack content");
    expect(menu.preparingNewGame() && menu.activeGame()==original && menu.games().size()==1 && original->scene().paused() &&
      original->view().transitioning && original->view().correctHits==1 && original->view().wrongHits==1 &&
      original->scene().tickCount()==tick && menu.selectedConfig().pace==2,"failed preparation preserves the complete paused run and editable draft");
  }
  act(menu,{Kind::CancelNewGame});act(menu,{Kind::Play});
  expect(menu.activeGame()==original && original->view().transitioning,"cancel after failure resumes frozen content despite the broken pack");
  act(menu,{Kind::ChooseQuestions});act(menu,{Kind::NewGame});
  expect(menu.selectedConfig().motion==RouteKind::Stationary,"reopening setup discards the failed draft");
  fs::copy_file(packs/"source_002.json",pack,fs::copy_options::overwrite_existing);
  const auto card=scratch.root/"cards/source_002_quadratic_three_points.json";
  nlohmann::json data;{std::ifstream input(card);input>>data;}data["description"]="Reloaded for a fresh game";std::ofstream(card)<<data;
  act(menu,{Kind::StartNewGame});
  expect(menu.activeGame()->question().content().description=="Reloaded for a fresh game" &&
    menu.activeGame()->view().correctHits==0 && !menu.preparingNewGame() && menu.error().empty(),
    "a repaired file starts fresh through the same loader and clears the failed setup");
}
void testAnswerReviewNavigation() {
  Scratch scratch;
  GalleryMenu menu(scratch.root/"packs",config());
  expect(!menu.dispatch({Kind::OpenReview}) && !menu.dispatch({Kind::CloseReview}),"review requires a started visible game");
  act(menu,{Kind::SelectPack,0});act(menu,{Kind::Play});
  auto* game=menu.activeGame();ready(*game);shoot(*game,115);shoot(*game,101);
  const std::string explanation=game->question().content().steps[0].explanation;
  const auto tick=game->scene().tickCount();const auto challenge=game->view().challenge;
  act(menu,{Kind::OpenReview});
  expect(menu.screen()==GalleryScreen::Review && game->view().paused && menu.reviewRun()==0 && menu.expandedReviewStep()==0,
    "review pauses the active game and expands the current reached step");
  const auto shot=Shoot{game->scene().frame().id,challenge,0.5F,0.5F};
  expect(!game->dispatch(shot).accepted && game->dispatch(GalleryTick{0.25F}).accepted && game->scene().tickCount()==tick,
    "review's paused game rejects shooting and cannot advance a pop animation");
  for(auto kind:{Kind::Play,Kind::ChooseQuestions,Kind::Workshop,Kind::NewGame,Kind::SelectPack,Kind::OpenReview})
    expect(!menu.dispatch({kind}),"review navigation cannot bypass Back to game");
  expect(!menu.launch(GalleryVariation::EqualitySweep) && !menu.dispatch({Kind::SelectReviewRun,1}) &&
    !menu.dispatch({Kind::ToggleReviewStep,1}),"direct launch and nonexistent history or steps are rejected during review");
  act(menu,{Kind::ToggleReviewStep,0});expect(!menu.expandedReviewStep(),"review row collapses");
  act(menu,{Kind::ToggleReviewStep,0});
  fs::remove_all(scratch.root/"cards");
  expect(game->question().review()->steps[0].attempts[1].label=="6 * 4","review uses frozen labels after source files disappear");
  expect(game->question().review()->steps[0].explanation.empty(),"paused partial collection does not reveal an explanation");
  act(menu,{Kind::CloseReview});
  expect(menu.screen()==GalleryScreen::Playing && game->view().paused && menu.activeGame()==game &&
    game->view().correctHits==1 && game->view().wrongHits==1,"Back to game preserves evidence and requires explicit Resume");
  expect(game->dispatch(GalleryPause{false}).accepted,"explicit Resume succeeds");static_cast<void>(game->publishFrame());
  shoot(*game,108);
  expect(game->dispatch(GalleryTick{0.25F}).accepted && game->dispatch(GalleryTick{0.05F}).accepted,"completed question transitions");
  static_cast<void>(game->publishFrame());
  act(menu,{Kind::OpenReview});act(menu,{Kind::SelectReviewRun,1});
  expect(game->question().review(menu.reviewRun())->completed && game->question().review(menu.reviewRun())->steps[0].attempts.size()==3,
    "completed run is selectable separately from the new current question");
  expect(game->question().review(menu.reviewRun())->steps[0].explanation==explanation && !explanation.empty(),
    "completed review retains its loaded explanation after source files disappear");
  act(menu,{Kind::CloseReview});act(menu,{Kind::OpenReview});
  expect(menu.reviewRun()==0 && game->question().review()->steps[0].attempts.empty(),"reopening defaults to the current question");
  expect(game->question().review()->steps[0].explanation.empty(),"reopening at a fresh run hides the previous run's explanation");
  act(menu,{Kind::CloseReview});act(menu,{Kind::ChooseQuestions});
  fs::copy(packs.parent_path()/"cards",scratch.root/"cards",fs::copy_options::recursive);
  act(menu,{Kind::NewGame});act(menu,{Kind::StartNewGame});act(menu,{Kind::OpenReview});
  expect(menu.reviewRun()==0 && !menu.activeGame()->question().review(1) && menu.activeGame()->question().review()->steps[0].attempts.empty(),
    "New game clears its review history and selects fresh evidence");
  expect(menu.activeGame()->question().review()->steps[0].explanation.empty(),"New game resets explanation eligibility with its progress");
}
}
int main() {
  try {testAnswerReviewNavigation();testSwitchAndResume();testFailureAndFrozenResume();testDirectStartup();testMovementChoices();testSetupValidation();testFrozenMovementSetup();testNewGameAndCancel();testFailedReplacement();}
  catch(const std::exception& e){++failures;std::cerr<<e.what()<<'\n';}
  if(!failures)std::cout<<"Gallery menu: selection, movement setup, shared launch, preserved sessions and recoverable file failures passed\n";
  return failures?1:0;
}

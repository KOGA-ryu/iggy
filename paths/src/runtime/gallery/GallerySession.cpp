#include "runtime/gallery/GallerySession.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace paths {
namespace fm=iggy3d::first_move;
namespace {
constexpr std::uint64_t feedbackTicks=kGalleryTickRate/2;
constexpr std::array colours{
  DisplayColour{"A","Blue",{0.24F,0.62F,1.0F}},
  DisplayColour{"B","Gold",{1.0F,0.73F,0.18F}},
  DisplayColour{"C","Pink",{0.95F,0.34F,0.65F}},
  DisplayColour{"D","Mint",{0.23F,0.90F,0.68F}},
  DisplayColour{"E","Violet",{0.68F,0.46F,0.95F}},
  DisplayColour{"F","Orange",{1.0F,0.43F,0.16F}},
  DisplayColour{"G","Ice",{0.55F,0.91F,1.0F}},
  DisplayColour{"H","White",{0.93F,0.93F,0.90F}},
};
constexpr std::array variations{
  GalleryVariationDescriptor{GalleryVariation::EqualitySweep,"equality_sweep","EQUALITY SWEEP"},
  GalleryVariationDescriptor{GalleryVariation::QuestionRelay,"question_relay","QUESTION RELAY"},
  GalleryVariationDescriptor{GalleryVariation::EquationChain,"equation_chain","EQUATION CHAIN"},
  GalleryVariationDescriptor{GalleryVariation::SubstitutionChain,"substitution_chain","SUBSTITUTION CHAIN"},
};
const GalleryVariationDescriptor& descriptor(GalleryVariation variation) {
  const auto found=std::find_if(variations.begin(),variations.end(),[&](const auto& d){return d.variation==variation;});
  if(found==variations.end())throw std::invalid_argument("unknown_gallery_variation");
  return *found;
}
std::vector<std::size_t> checkedDeck(std::vector<std::size_t> deck,std::size_t catalogSize) {
  if(deck.empty() || deck.size()>fm::kQuestionCatalogCapacity)throw std::invalid_argument("invalid_gallery_deck_size");
  for(const auto index:deck)if(index>=catalogSize)throw std::invalid_argument("gallery_deck_index_out_of_range");
  return deck;
}
RouteSpec makeRoute(const GalleryConfig& config) {
  RouteSpec route;
  route.kind=config.motion;route.settings.speedMetersPerSecond=config.pace;
  route.seed=config.seed ^ 0x9e3779b9U;
  // Compact disjoint lanes for the foundation presets. Custom route editing
  // remains in the independent workshop, outside an active question.
  route.extent={0.5F,0.4F,0.15F};
  return route;
}
std::array<OptionId,8> makeColourAssignment(const fm::LayeredQuestionStepContent& step,
    std::mt19937& engine,std::optional<DisplayToken> previous,bool changeCorrectColour) {
  std::array<OptionId,8> options{};
  for(std::size_t i=0;i<step.options.size();++i)options[i]=step.options[i].id;
  std::shuffle(options.begin(),options.begin()+static_cast<std::ptrdiff_t>(step.options.size()),engine);
  if(changeCorrectColour && previous && previous->value<step.options.size() && std::popcount(step.acceptedOptions)==1) {
    const auto correct=step.options[fm::firstAcceptedOption(step)].id;
    if(options[previous->value]==correct) {
      std::uniform_int_distribution<std::size_t> distribution(0,step.options.size()-2);
      auto other=distribution(engine);
      if(other>=previous->value)++other;
      std::swap(options[other],options[previous->value]);
    }
  }
  return options;
}
}
std::span<const DisplayColour> galleryDisplayColours() {return colours;}
std::span<const GalleryVariationDescriptor> galleryVariations() {return variations;}
GalleryResult validateGalleryConfig(const GalleryConfig& config) {
  if(config.mathematicalMoves && !config.stopAfterQuestion)return {false,"math_moves_require_finite_workspace"};
  if(std::none_of(variations.begin(),variations.end(),[&](const auto& mode){return mode.variation==config.variation;}))
    return {false,"unknown_gallery_variation"};
  if(config.motion==RouteKind::Waypoints)return {false,"gameplay_requires_route_preset"};
  PreparedRoute prepared;
  const auto result=prepareRoute(makeRoute(config),{},prepared);
  return {result.accepted,result.reason};
}

struct GallerySession::PreparedChallenge {
  GalleryScene scene;
  fm::LayeredQuestionSession question;
  std::mt19937 engine;
  ChallengeRecord record;
  std::size_t deckIndex;
};
GallerySession::GallerySession(GalleryConfig config,std::vector<fm::LayeredQuestionContent> catalog,
                               std::vector<std::size_t> resolvedDeck,
                               std::span<const fm::LayeredQuestionCommand> savedProgress)
    : config_(config),deck_(checkedDeck(std::move(resolvedDeck),catalog.size())),
      question_(std::move(catalog),config.mathematicalMoves?fm::QuestionInteraction::MathMoves:fm::QuestionInteraction::ArcadeCollect,deck_.front(),config.stopAfterQuestion),assignmentEngine_(config.seed) {
  const auto valid=validateGalleryConfig(config);
  if(!valid.accepted)throw std::invalid_argument(std::string(valid.reason));
  if(!savedProgress.empty()) {
    if(!config_.stopAfterQuestion || deck_.size()!=1)throw std::invalid_argument("saved_progress_requires_single_question");
    for(const auto& command:savedProgress) {
      const auto result=question_.dispatch(command);
      if(!result.accepted || !result.changed)throw std::invalid_argument("invalid_saved_progress: "+std::string(result.reason));
    }
    const auto& run=question_.currentRun();
    if(run.phase==fm::LayeredQuestionPhase::Grid || (!run.math && !run.completed &&
        fm::layeredQuestionStepResolved(run.steps[run.currentStep])))
      throw std::invalid_argument("saved_progress_between_steps");
  }
  commitChallenge(prepareChallenge(false));
}
GallerySession::PreparedChallenge GallerySession::prepareChallenge(bool advance, bool replay, bool archiveUnfinished) const {
  PreparedChallenge next{scene_,question_,assignmentEngine_,{},deckIndex_};
  if (replay) {
    fm::LayeredQuestionCommand restart{fm::LayeredQuestionCommandKind::RestartQuestion};restart.archiveUnfinished=archiveUnfinished;
    const auto restarted=next.question.dispatch(restart);
    if(!restarted.accepted)throw std::logic_error(std::string(restarted.reason));
  }
  if(advance) {
    const auto continued=next.question.dispatch({fm::LayeredQuestionCommandKind::Continue});
    if(!continued.accepted)throw std::logic_error(std::string(continued.reason));
    if(next.question.currentRun().completed) {
      next.deckIndex=(deckIndex_+1)%deck_.size();
      fm::LayeredQuestionCommand restart{fm::LayeredQuestionCommandKind::RestartQuestion};
      restart.questionIndex=deck_[next.deckIndex];
      const auto started=next.question.dispatch(restart);
      if(!started.accepted)throw std::logic_error(std::string(started.reason));
    }
  } else if (!replay && next.question.currentRun().phase==fm::LayeredQuestionPhase::Grid) {
    const auto opened=next.question.dispatch({fm::LayeredQuestionCommandKind::OpenQuestion});
    if(!opened.accepted)throw std::logic_error(std::string(opened.reason));
  }
  const auto& run=next.question.currentRun();
  if(run.math) {
    next.record.id={challenges_.empty()?1:challenges_.back().id.value+1};
    next.record.questionId=run.questionId;next.record.contentVersion=run.contentVersion;next.record.runNumber=run.runNumber;
    return next;
  }
  const auto& step=next.question.content().steps[run.currentStep];
  if(step.options.size()<2 || step.options.size()>colours.size())throw std::invalid_argument("invalid_challenge_choice_count");
  const auto assignment=makeColourAssignment(step,next.engine,previousCorrectColour_,config_.changeCorrectColour);
  std::array<iggy3d::Vec3,8> rgb;
  for(std::size_t i=0;i<step.options.size();++i)rgb[i]=colours[i].rgb;
  ResetTargets reset{{rgb.data(),step.options.size()},std::nullopt,!config_.stopAfterQuestion};
  if(challenges_.empty() || scene_.objects().size()!=step.options.size())reset.route=makeRoute(config_);
  const auto result=next.scene.dispatch(reset);
  if(!result.accepted)throw std::invalid_argument(std::string(result.reason));
  if(config_.stopAfterQuestion) {
    const auto framed=next.scene.dispatch({GalleryActionKind::FrameTargets});
    if(!framed.accepted)throw std::invalid_argument(std::string(framed.reason));
  }
  next.record.id={challenges_.empty()?1:challenges_.back().id.value+1};
  next.record.questionId=run.questionId;next.record.contentVersion=run.contentVersion;
  next.record.runNumber=run.runNumber;next.record.step=step.id;next.record.count=step.options.size();
  for(std::size_t i=0;i<step.options.size();++i)
    next.record.bindings[i]={next.scene.objects()[i].id,assignment[i],DisplayToken{static_cast<std::uint8_t>(i)}};
  // A restored partial answer set must not respawn its already collected targets.
  for(const auto& binding:std::span(next.record.bindings.data(),next.record.count)) {
    const auto option=std::find_if(step.options.begin(),step.options.end(),[&](const auto& o){return o.id==binding.option;});
    if(run.steps[run.currentStep].collectedOptions & (1U << (option-step.options.begin()))) {
      const auto popped=next.scene.dispatch({GalleryActionKind::Pop,{},0,binding.object.value});
      if(!popped.accepted)throw std::logic_error("restored_target_pop_failed");
    }
  }
  return next;
}
void GallerySession::commitChallenge(PreparedChallenge&& next) {
  // All fallible content, assignment and geometry work happened in the staged
  // owners. Appending can allocate, so do it before the no-throw moves commit.
  challenges_.push_back(std::move(next.record));
  scene_=std::move(next.scene);question_=std::move(next.question);assignmentEngine_=std::move(next.engine);
  deckIndex_=next.deckIndex;transitionAt_.reset();ready_=false;presentedChallenge_={};
  feedback_=GalleryFeedback::None;
}
GalleryResult GallerySession::submitAnswer(OptionId option, const TargetBinding* target) {
  const auto verdict=question_.dispatch(fm::LayeredQuestionCommand::submitOption(option));
  if(!verdict.accepted)return {false,verdict.reason};
  const auto& record=question_.currentRun().steps[question_.currentRun().currentStep];
  feedback_=record.attempts.back().correct?GalleryFeedback::Correct:GalleryFeedback::Incorrect;
  feedbackUntilTick_=scene_.tickCount()+feedbackTicks;
  // Consume the model's judged fact. No label, RGB value or second answer key.
  if(record.attempts.back().correct) {
    if(target) {
      const auto popped=scene_.dispatch({GalleryActionKind::Pop,{},0,target->object.value});
      if(!popped.accepted)throw std::logic_error("validated_target_pop_failed");
      const auto& step=question_.content().steps[question_.currentRun().currentStep];
      if(std::popcount(step.acceptedOptions)==1)previousCorrectColour_=target->token;
    }
    if(record.resolvedByPlayer) {
      if(!target || config_.stopAfterQuestion)advanceChallenge();
      else transitionAt_=scene_.tickCount()+kTargetPopTicks;
    }
  }
  return {true,verdict.reason};
}
GalleryResult GallerySession::tick(float seconds) {
  if(!std::isfinite(seconds) || seconds<0 || seconds>0.25F)return {false,"invalid_tick_delta"};
  if(scene_.paused() || question_.currentRun().completed)return {true,"paused_or_complete"};
  // Split a long render delta only to observe transitions at the accepted
  // scene tick. GalleryScene still has the sole persistent time accumulator.
  double remaining=seconds;
  while(remaining>1e-10) {
    const auto delta=static_cast<float>(std::min(remaining,1.0/kGalleryTickRate));
    const auto result=scene_.dispatch({GalleryActionKind::Tick,{},delta});
    if(!result.accepted)return result;
    remaining-=delta;
    if(transitionAt_ && scene_.tickCount()>=*transitionAt_)advanceChallenge();
  }
  return {true,"ticked"};
}
void GallerySession::advanceChallenge() {
  const auto& run=question_.currentRun();
  if(config_.stopAfterQuestion && run.currentStep+1==run.steps.size()) {
    const auto result=question_.dispatch({fm::LayeredQuestionCommandKind::Continue});
    if(!result.accepted)throw std::logic_error(std::string(result.reason));
    transitionAt_.reset(); ready_=false; feedback_=GalleryFeedback::None;
  } else commitChallenge(prepareChallenge(true));
}
GalleryResult GallerySession::dispatch(const GalleryCommand& command) {
  return std::visit([&](const auto& action)->GalleryResult {
    using T=std::decay_t<decltype(action)>;
    if constexpr(std::is_same_v<T,GalleryTick>)return tick(action.seconds);
    else if constexpr(std::is_same_v<T,GalleryPause>)
      return scene_.dispatch({GalleryActionKind::Pause,{},action.paused?1.0F:0.0F});
    else if constexpr(std::is_same_v<T,GalleryViewport>) {
      const bool resized=action.viewport!=scene_.frame().viewport;
      try {scene_.setViewport(action.viewport);}
      catch(const std::invalid_argument&) {return {false,"invalid_viewport"};}
      if(config_.stopAfterQuestion && resized && !question_.currentRun().math)return scene_.dispatch({GalleryActionKind::FrameTargets});
      return {true,"viewport_set"};
    } else if constexpr(std::is_same_v<T,Shoot>) {
      if(question_.currentRun().math)return {false,"math_move_required"};
      if(action.challenge!=challenges_.back().id || action.challenge!=presentedChallenge_)
        return {false,"stale_challenge"};
      if(scene_.paused())return {false,"paused"};
      if(!ready_ || transitionAt_ || question_.currentRun().completed)return {false,"challenge_not_answering"};
      if(config_.stopAfterQuestion && question_.content().steps[question_.currentRun().currentStep].semantics.purpose!=fm::StepPurpose::Calculation)
        return {false,"choose_an_operation"};
      const auto hit=scene_.hitTestPresentedFrame(action.frame,action.u,action.v);
      if(!hit.accepted)return {false,hit.reason};
      const auto& challenge=challenges_.back();
      const auto bindings=std::span(challenge.bindings.data(),challenge.count);
      const auto binding=std::find_if(bindings.begin(),bindings.end(),[&](const auto& b){return b.object==hit.object;});
      if(binding==bindings.end()) {
        ++aimMisses_;feedback_=GalleryFeedback::Miss;feedbackUntilTick_=scene_.tickCount()+feedbackTicks;
        return {true,"aim_miss"};
      }
      if(hit.phase!=VisualPhase::Active)return {false,"target_not_active"};
      return submitAnswer(binding->option,&*binding);
    } else if constexpr(std::is_same_v<T,ChooseAnswer> || std::is_same_v<T,GalleryHelp>) {
      if(question_.currentRun().math)return {false,"math_move_required"};
      if(action.challenge!=challenges_.back().id)return {false,"stale_challenge"};
      if(scene_.paused() || transitionAt_ || question_.currentRun().completed)return {false,"challenge_not_answering"};
      const auto& run=question_.currentRun();
      const auto& step=question_.content().steps[run.currentStep];
      if constexpr(std::is_same_v<T,ChooseAnswer>) {
        if(step.semantics.purpose!=fm::StepPurpose::OperationChoice && step.semantics.purpose!=fm::StepPurpose::GraphChoice)
          return {false,"button_choice_not_available"};
        return submitAnswer(action.option);
      } else {
        constexpr std::array commands{fm::LayeredQuestionCommandKind::RequestHint,
            fm::LayeredQuestionCommandKind::RevealNextMove,fm::LayeredQuestionCommandKind::ApplyPreparedStep};
        const auto index=static_cast<unsigned>(action.kind);
        if(index>=commands.size())return {false,"invalid_help_kind"};
        const auto result=question_.dispatch({commands[index]});
        if(!result.accepted)return {false,result.reason};
        if(action.kind==GalleryHelpKind::DoStep)advanceChallenge();
        return {true,result.reason};
      }
    } else if constexpr(std::is_same_v<T,MathematicalMove>) {
      if(action.challenge!=challenges_.back().id)return {false,"stale_challenge"};
      if(scene_.paused())return {false,"paused"};
      fm::LayeredQuestionCommand command{fm::LayeredQuestionCommandKind::MathematicalMove};command.math=action.move;
      const auto result=question_.dispatch(command);return {result.accepted,result.reason};
    } else if constexpr(std::is_same_v<T,ReplayQuestion>) {
      if(!config_.stopAfterQuestion || (!question_.currentRun().completed && !action.archiveUnfinished))return {false,"question_not_complete"};
      commitChallenge(prepareChallenge(false,true,action.archiveUnfinished));
      return {true,"question_restarted"};
    }
  },command);
}
const SceneFrame& GallerySession::publishFrame() {
  const auto& frame=scene_.publishFrame();
  presentedChallenge_=challenges_.back().id;
  if(!ready_ && !question_.currentRun().completed && std::all_of(scene_.objects().begin(),scene_.objects().end(),[](const auto& o){return o.phase!=VisualPhase::Spawning;})) {
    ready_=true;
  }
  return frame;
}
GallerySessionView GallerySession::view() const {
  GallerySessionView view;
  const auto& run=question_.currentRun();
  view.challenge=challenges_.back().id;view.equation=question_.content().equation;view.working=question_.visibleWorking();
  view.paused=scene_.paused();view.completed=run.completed;view.priorExposure=run.priorExposure;
  const auto count=[&](const fm::LayeredQuestionRunRecord& evidence) {
    view.completedQuestions+=evidence.completed;
    if(run.math) {
      if(evidence.math)for(const auto& event:evidence.math->events)if(event.kind==fm::MathMoveKind::Submit)
        event.correct?++view.correctHits:++view.wrongHits;
    } else for(const auto& step:evidence.steps)for(const auto& attempt:step.attempts)
      attempt.correct?++view.correctHits:++view.wrongHits;
  };
  count(run);for(const auto& archived:question_.archivedRuns())count(archived);
  if(run.math) {
    const auto& math=*run.math;const auto& node=math.nodes[math.active];
    view.title="MATHEMATICAL MOVES";view.verification=node.verification;view.ready=!view.paused;
    view.step=math.active+1;view.stepCount=math.nodes.size();
    return view;
  }
  const auto& content=question_.content();const auto& step=content.steps[run.currentStep];
  const auto& record=run.steps[run.currentStep];const auto& challenge=challenges_.back();
  view.title=descriptor(config_.variation).title;view.prompt=step.prompt;
  view.workingHighlights=question_.visibleWorkingState().highlights;
  view.feedback=scene_.tickCount()<feedbackUntilTick_?feedback_:GalleryFeedback::None;
  view.step=run.currentStep+1;view.stepCount=run.steps.size();view.choiceCount=step.options.size();
  view.required=fm::requiredAnswerCount(step);view.collected=std::popcount(record.collectedOptions);
  view.ready=ready_;view.transitioning=transitionAt_.has_value();view.purpose=step.semantics.purpose;
  view.canHint=!run.completed && !step.hint.empty();view.canReveal=!run.completed && !step.nextMove.empty();
  if(record.hintRequested)view.hint=step.hint;
  if(record.nextMoveRequested)view.nextMove=step.nextMove;
  if(run.completed)view.verification=step.explanation;
  view.aimMisses=aimMisses_;
  for(std::size_t i=0;i<challenge.count;++i) {
    const auto& binding=challenge.bindings[i];
    const auto option=std::find_if(step.options.begin(),step.options.end(),[&](const auto& o){return o.id==binding.option;});
    const auto index=static_cast<std::size_t>(option-step.options.begin());
    view.answers[i]={binding,option->label,(record.collectedOptions & (1U << index))!=0};
  }
  return view;
}
}  // namespace paths

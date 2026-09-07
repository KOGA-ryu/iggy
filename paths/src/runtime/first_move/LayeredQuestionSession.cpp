#include "runtime/first_move/LayeredQuestionSession.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace iggy3d::first_move {
namespace {

const LayeredQuestionContent kQuestion=[] {
  LayeredQuestionContent question{
    std::string(kLayeredQuestionId),
    kLayeredQuestionVersion,
    "3a + 5 = 20",
    "linear_equation_one_unknown",
    "Vocabulary and solving · 7 short steps",
    {
        {"NAME IT",
         "What kind of mathematical statement is 3a + 5 = 20?",
         {{"An expression with no equals sign"},
           {"A linear equation in one unknown"},
           {"A quadratic equation"},
           {"An inequality"}},
         static_cast<std::uint8_t>(1U << 1U),
         "That choice does not fit this step. You can try again or see the answer and why.",
         "It is an equation because it has an equals sign. It is linear in one unknown because a appears only to the first power."},
        {"FIND THE UNKNOWN",
         "Which symbol is the unknown—the value we are trying to find?",
         {{"3"}, {"5"}, {"a"}, {"20"}},
         static_cast<std::uint8_t>(1U << 2U),
         "That choice does not fit this step. You can try again or see the answer and why.",
         "a is the unknown. The numbers 3, 5, and 20 are known values."},
        {"NAME A PART",
         "What is the role of 3 in the term 3a?",
         {{"The number being added to a"},
           {"The coefficient multiplying a"},
           {"The solution of the equation"},
           {"The right-hand side"}},
         static_cast<std::uint8_t>(1U << 1U),
         "That choice does not fit this step. You can try again or see the answer and why.",
         "3 is the coefficient of a. The term 3a means 3 times a."},
        {"CHOOSE THE FIRST MOVE",
         "Which operation removes +5 from the left side while keeping both sides equal?",
         {{"Add 5 to both sides"},
           {"Subtract 3 from both sides"},
           {"Subtract 5 from both sides"},
           {"Divide only the left side by 3"}},
         static_cast<std::uint8_t>(1U << 2U),
         "That choice does not fit this step. You can try again or see the answer and why.",
         "Subtracting 5 from both sides preserves equality and removes the +5 from the left side."},
        {"SIMPLIFY",
         "After subtracting 5 from both sides, what equation remains?",
         {{"3a = 15"}, {"3a = 25"}, {"a = 15"}, {"3a = 20"}},
         static_cast<std::uint8_t>(1U << 0U),
         "That choice does not fit this step. You can try again or see the answer and why.",
         "3a + 5 - 5 = 20 - 5 simplifies to 3a = 15."},
        {"CHOOSE THE SECOND MOVE",
         "What move now isolates a?",
         {{"Multiply both sides by 3"},
           {"Divide both sides by 3"},
           {"Subtract 3 from both sides"},
           {"Divide both sides by 5"}},
         static_cast<std::uint8_t>(1U << 1U),
         "That choice does not fit this step. You can try again or see the answer and why.",
         "Dividing both sides by 3 leaves a by itself and preserves equality."},
        {"STATE THE SOLUTION",
         "What is the solution?",
         {{"a = 15"}, {"a = 8"}, {"a = 25"}, {"a = 5"}},
         static_cast<std::uint8_t>(1U << 3U),
         "That choice does not fit this step. You can try again or see the answer and why.",
         "3a = 15 gives a = 5. Substitution checks it: 3(5) + 5 = 20."},
    }};
  question.workingStates={
    {{10},question.equation}, {{20},"3a + 5 - 5 = 20 - 5"},
    {{30},"3a = 15"}, {{40},"3a / 3 = 15 / 3"},
    {{50},"3a + 5 = 20\n3a = 15    subtract 5 from both sides\na = 5      divide both sides by 3"},
  };
  const std::array semantics{
    StepSemantics{StepPurpose::AnswerChoice,CompletionRule::AllAccepted,{10},{10}},
    StepSemantics{StepPurpose::AnswerChoice,CompletionRule::AllAccepted,{10},{10}},
    StepSemantics{StepPurpose::AnswerChoice,CompletionRule::AllAccepted,{10},{10}},
    StepSemantics{StepPurpose::OperationChoice,CompletionRule::AllAccepted,{10},{20}},
    StepSemantics{StepPurpose::Calculation,CompletionRule::AllAccepted,{20},{30}},
    StepSemantics{StepPurpose::OperationChoice,CompletionRule::AllAccepted,{30},{40}},
    StepSemantics{StepPurpose::Calculation,CompletionRule::AllAccepted,{40},{50}},
  };
  for(std::size_t i=0;i<question.steps.size();++i) {
    auto& step=question.steps[i];step.id={static_cast<std::uint32_t>(i+1)};step.semantics=semantics[i];
    for(std::size_t j=0;j<step.options.size();++j)step.options[j].id={static_cast<std::uint32_t>(j+1)};
  }
  return question;
}();

[[nodiscard]] LayeredQuestionDispatchResult accepted(
    bool changed,
    std::string_view reason) noexcept {
  return {true, changed, reason};
}

[[nodiscard]] LayeredQuestionDispatchResult rejected(
    std::string_view reason) noexcept {
  return {false, false, reason};
}

struct GraphSolution { GraphRelation relation; std::optional<GraphPoint> point; };
GraphSolution graphSolution(const LineGraph& graph) {
  const auto& second=*graph.second;
  const int determinant=graph.rise*second.run-second.rise*graph.run;
  if(!determinant)return {graph.intercept==second.intercept?GraphRelation::Coincident:GraphRelation::Parallel,std::nullopt};
  const double x=static_cast<double>(second.intercept-graph.intercept)*graph.run*second.run/determinant;
  return {GraphRelation::Intersecting,GraphPoint{static_cast<float>(x),static_cast<float>(graph.rise*x/graph.run+graph.intercept)}};
}

// The shared question validator checks the existing question/answer structure
// first. This adds prepared-state integrity without interpreting the text.
QuestionValidationResult validateStepChain(const LayeredQuestionContent& question) noexcept {
  using Code = QuestionValidationCode;
  const auto& states=question.workingStates;
  if(states.empty() || states.size()>kQuestionWorkingStateCapacity)
    return {Code::InvalidWorkingStateCount,"workingStates",0};
  if(question.lineGraph) {
    const auto& g=*question.lineGraph;
    if(g.run<1 || g.run>8 || g.rise<-8 || g.rise>8 || g.intercept<-12 || g.intercept>12 ||
        g.xMin<-20 || g.xMin>=0 || g.xMax<=g.run || g.xMax>20 ||
        g.yMin<-20 || g.yMin>=std::min({0,g.intercept,g.intercept+g.rise}) ||
        g.yMax<=std::max({0,g.intercept,g.intercept+g.rise}) || g.yMax>20)
      return {Code::InvalidGraph,"line_graph",0};
    if(g.second) {
      const auto& second=*g.second;
      if(second.run<1 || second.run>8 || second.rise<-8 || second.rise>8 || second.intercept<-12 || second.intercept>12 ||
          second.intercept<=g.yMin || second.intercept>=g.yMax)
        return {Code::InvalidGraph,"line_graph/second",0};
      if(const auto solution=graphSolution(g);solution.point) {
        const auto [x,y]=*solution.point;
        if(x<=g.xMin || x>=g.xMax || y<=g.yMin || y>=g.yMax)
          return {Code::InvalidGraph,"line_graph",0};
      }
    }
    if(states.front().graphStage!=GraphStage::Grid || states.back().graphStage!=(g.second?GraphStage::SystemSolution:GraphStage::Line))
      return {Code::InvalidGraph,"graph_stage",0,std::nullopt,std::nullopt,0};
  }
  for(std::size_t i=0;i<states.size();++i) {
    if(!states[i].id.value)return {Code::MissingWorkingStateId,"id",0,std::nullopt,std::nullopt,i};
    for(std::size_t previous=0;previous<i;++previous)
      if(states[previous].id==states[i].id)
        return {Code::DuplicateWorkingStateIdentity,"id",0,std::nullopt,std::nullopt,i};
    const auto& state=states[i];
    if(state.graphStage.has_value()!=question.lineGraph.has_value() ||
        (state.graphStage && static_cast<unsigned>(*state.graphStage)>static_cast<unsigned>(GraphStage::SystemSolution)))
      return {Code::InvalidGraph,"graph_stage",0,std::nullopt,std::nullopt,i};
    if(state.highlights.size()>kWorkingHighlightCapacity)
      return {Code::InvalidWorkingHighlight,"highlights",0,std::nullopt,std::nullopt,i};
    std::size_t end=0;
    const auto boundary=[&](std::size_t at) {
      return at==state.display.size() || (static_cast<unsigned char>(state.display[at]) & 0xC0)!=0x80;
    };
    for(const auto& span:state.highlights) {
      if(!span.length || span.offset<end || span.offset>state.display.size() ||
          span.length>state.display.size()-span.offset || span.label.empty() ||
          !boundary(span.offset) || !boundary(static_cast<std::size_t>(span.offset)+span.length))
        return {Code::InvalidWorkingHighlight,"highlights",0,std::nullopt,std::nullopt,i};
      end=static_cast<std::size_t>(span.offset)+span.length;
    }
  }
  const auto exists=[&](WorkingStateId id) {
    return std::any_of(states.begin(),states.end(),[&](const auto& state){return state.id==id;});
  };
  for(std::size_t i=0;i<question.steps.size();++i) {
    const auto& semantics=question.steps[i].semantics;
    switch(semantics.purpose) {
      case StepPurpose::AnswerChoice:case StepPurpose::OperationChoice:
      case StepPurpose::Calculation:case StepPurpose::Verification:case StepPurpose::GraphChoice:break;
      default:return {Code::InvalidStepPurpose,"semantics.purpose",0,i};
    }
    switch(semantics.completion) {
      case CompletionRule::AnyAccepted:case CompletionRule::AllAccepted:break;
      default:return {Code::InvalidCompletionRule,"semantics.completion",0,i};
    }
    if(!exists(semantics.before))return {Code::UnknownWorkingState,"semantics.before",0,i};
    if(!exists(semantics.after))return {Code::UnknownWorkingState,"semantics.after",0,i};
    if(i && question.steps[i-1].semantics.after!=semantics.before)
      return {Code::BrokenStepChain,"semantics.before",0,i};
    if(question.lineGraph.has_value()!=(semantics.purpose==StepPurpose::GraphChoice))
      return {Code::InvalidGraph,"semantics.purpose",0,i};
    if(question.lineGraph) {
      constexpr std::array single{GraphStage::Grid,GraphStage::Intercept,GraphStage::Run,GraphStage::Rise,GraphStage::Line};
      constexpr std::array system{GraphStage::Grid,GraphStage::FirstLine,GraphStage::BothLines,GraphStage::Classified,GraphStage::SystemSolution};
      const auto& stages=question.lineGraph->second?system:single;
      const auto stage=[&](WorkingStateId id) {
        return *std::find_if(states.begin(),states.end(),[&](const auto& s){return s.id==id;})->graphStage;
      };
      if(question.steps.size()!=4 || i>=4 || stage(semantics.before)!=stages[i] || stage(semantics.after)!=stages[i+1])
        return {Code::InvalidGraph,"semantics.after",0,i};
    }
  }
  return {};
}

}  // namespace

const LayeredQuestionContent& layeredQuestion() noexcept {
  return kQuestion;
}

std::string_view layeredQuestionPhaseName(
    LayeredQuestionPhase phase) noexcept {
  switch (phase) {
    case LayeredQuestionPhase::Grid: return "grid";
    case LayeredQuestionPhase::Answering: return "answering";
    case LayeredQuestionPhase::Complete: return "complete";
  }
  return "unknown";
}

bool layeredQuestionStepResolved(
    const LayeredQuestionStepRecord& step) noexcept {
  return step.resolvedByPlayer || step.answerShown;
}

LayeredQuestionRunSummary summarizeLayeredQuestionRun(
    const LayeredQuestionRunRecord& run) noexcept {
  LayeredQuestionRunSummary summary;
  summary.completed = run.completed;
  if(run.math) {
    std::array<bool,kMathNodeCapacity> retried{};
    for(const auto& event:run.math->events)if(event.kind==MathMoveKind::Submit) {
      if(!event.correct) {++summary.incorrectCheckedAttempts;retried[event.from]=true;}
      else if(retried[event.from])++summary.correctedAfterRetry;
      else ++summary.correctOnFirstTry;
    }
    return summary;
  }
  for (const LayeredQuestionStepRecord& step : run.steps) {
    summary.assisted = summary.assisted || step.hintRequested || step.nextMoveRequested;
    if (step.resolvedByPlayer && step.incorrectCheckedAttempts == 0) {
      ++summary.correctOnFirstTry;
    } else if (step.resolvedByPlayer && step.firstCorrect.has_value()) {
      ++summary.correctedAfterRetry;
    }
    if (step.answerShown) {
      ++summary.shownAnswers;
    }
    summary.incorrectCheckedAttempts += step.incorrectCheckedAttempts;
  }
  summary.assisted = summary.assisted || summary.shownAnswers > 0U;
  return summary;
}

std::string_view QuestionValidationResult::reason() const noexcept {
  switch(code) {
    case QuestionValidationCode::Valid:return "question_content_valid";
    case QuestionValidationCode::InvalidCatalogSize:
    case QuestionValidationCode::InvalidInteraction:return "invalid_question_catalog";
    case QuestionValidationCode::MissingQuestionId:
    case QuestionValidationCode::MissingQuestionVersion:
    case QuestionValidationCode::InvalidStepCount:return "invalid_question_content";
    case QuestionValidationCode::DuplicateQuestionIdentity:return "duplicate_question_identity";
    case QuestionValidationCode::MissingStepId:
    case QuestionValidationCode::MissingPrompt:
    case QuestionValidationCode::InvalidOptionCount:
    case QuestionValidationCode::EmptyAcceptedOptions:
    case QuestionValidationCode::AcceptedOptionsOutOfRange:
    case QuestionValidationCode::GuidedRequiresSingleAnswer:return "invalid_question_step";
    case QuestionValidationCode::DuplicateStepIdentity:return "duplicate_step_identity";
    case QuestionValidationCode::MissingOptionId:
    case QuestionValidationCode::MissingOptionLabel:return "invalid_option";
    case QuestionValidationCode::DuplicateOptionIdentity:return "duplicate_option_identity";
    case QuestionValidationCode::InvalidWorkingStateCount:return "invalid_working_state_count";
    case QuestionValidationCode::MissingWorkingStateId:return "invalid_working_state";
    case QuestionValidationCode::DuplicateWorkingStateIdentity:return "duplicate_working_state_identity";
    case QuestionValidationCode::InvalidWorkingHighlight:return "invalid_working_highlight";
    case QuestionValidationCode::InvalidGraph:return "invalid_coordinate_graph";
    case QuestionValidationCode::InvalidStepPurpose:
    case QuestionValidationCode::InvalidCompletionRule:return "invalid_step_semantics";
    case QuestionValidationCode::UnknownWorkingState:return "unknown_working_state";
    case QuestionValidationCode::BrokenStepChain:return "broken_step_chain";
    case QuestionValidationCode::InvalidMathMoves:return "invalid_math_moves";
  }
  return "unknown_question_validation_code";
}

QuestionValidationResult validateQuestion(const LayeredQuestionContent& question,
                                          QuestionInteraction interaction) noexcept {
  using Code = QuestionValidationCode;
  if(interaction!=QuestionInteraction::Guided && interaction!=QuestionInteraction::ArcadeCollect && interaction!=QuestionInteraction::MathMoves)
    return {Code::InvalidInteraction,"interaction"};
  if(question.id.empty())return {Code::MissingQuestionId,"id",0};
  if(!question.version)return {Code::MissingQuestionVersion,"version",0};
  if(question.supportsMathMoves || interaction==QuestionInteraction::MathMoves) {
    if(!question.supportsMathMoves || question.lineGraph || !prepareMathWorking(question.mathModel,question.equation).result)
      return {Code::InvalidMathMoves,"working_model",0};
  }
  if(question.steps.empty() || question.steps.size()>kQuestionStepCapacity)
    return {Code::InvalidStepCount,"steps",0};
  for(std::size_t i=0;i<question.steps.size();++i) {
    const auto& step=question.steps[i];
    if(!step.id.value)return {Code::MissingStepId,"id",0,i};
    if(step.prompt.empty())return {Code::MissingPrompt,"prompt",0,i};
    if(step.options.size()<2 || step.options.size()>kQuestionChoiceCapacity)
      return {Code::InvalidOptionCount,"options",0,i};
    if(!step.acceptedOptions)return {Code::EmptyAcceptedOptions,"acceptedOptions",0,i};
    if(step.acceptedOptions >> step.options.size())
      return {Code::AcceptedOptionsOutOfRange,"acceptedOptions",0,i};
    if(interaction==QuestionInteraction::Guided && requiredAnswerCount(step)>1)
      return {Code::GuidedRequiresSingleAnswer,"acceptedOptions",0,i};
    for(std::size_t previous=0;previous<i;++previous)
      if(question.steps[previous].id==step.id)return {Code::DuplicateStepIdentity,"id",0,i};
    for(std::size_t j=0;j<step.options.size();++j) {
      const auto& option=step.options[j];
      if(!option.id.value)return {Code::MissingOptionId,"id",0,i,j};
      if(option.label.empty())return {Code::MissingOptionLabel,"label",0,i,j};
      for(std::size_t previous=0;previous<j;++previous)
        if(step.options[previous].id==option.id)return {Code::DuplicateOptionIdentity,"id",0,i,j};
    }
  }
  return validateStepChain(question);
}

QuestionValidationResult validateCatalog(std::span<const LayeredQuestionContent> catalog,
                                         QuestionInteraction interaction) noexcept {
  using Code = QuestionValidationCode;
  if(catalog.empty() || catalog.size()>kQuestionCatalogCapacity)
    return {Code::InvalidCatalogSize,"catalog"};
  if(interaction!=QuestionInteraction::Guided && interaction!=QuestionInteraction::ArcadeCollect && interaction!=QuestionInteraction::MathMoves)
    return {Code::InvalidInteraction,"interaction"};
  for(std::size_t q=0;q<catalog.size();++q) {
    auto result=validateQuestion(catalog[q],interaction);
    if(!result.valid())result.questionIndex=q;
    // Preserve constructor precedence: question fields, duplicate catalog
    // identity, then the question's step/option failure, if any.
    if(!result.valid() && !result.stepIndex)return result;
    for(std::size_t previous=0;previous<q;++previous)
      if(catalog[previous].id==catalog[q].id && catalog[previous].version==catalog[q].version)
        return {Code::DuplicateQuestionIdentity,"id",q};
    if(!result.valid())return result;
  }
  return {};
}

LayeredQuestionSession::LayeredQuestionSession()
    : LayeredQuestionSession({layeredQuestion()},QuestionInteraction::Guided) {}

LayeredQuestionSession::LayeredQuestionSession(std::vector<LayeredQuestionContent> catalog,
    QuestionInteraction interaction,std::size_t initialQuestion)
    : contentIndex_(initialQuestion),interaction_(interaction) {
  if(initialQuestion>=catalog.size())
    throw std::invalid_argument("invalid_question_catalog");
  const auto validation=validateCatalog(catalog,interaction);
  if(!validation.valid())throw std::invalid_argument(std::string(validation.reason()));
  catalog_=std::make_shared<const std::vector<LayeredQuestionContent>>(std::move(catalog));
  beginRun(1U,false,LayeredQuestionPhase::Grid);
}

const LayeredQuestionRunRecord& LayeredQuestionSession::currentRun()
    const noexcept {
  return current_;
}

const std::vector<LayeredQuestionRunRecord>&
LayeredQuestionSession::archivedRuns() const noexcept {
  return archived_;
}

WorkingStateId LayeredQuestionSession::visibleWorkingId() const noexcept {
  if(current_.math)return current_.math->nodes[current_.math->active].working.id;
  const auto& semantics=content().steps[current_.currentStep].semantics;
  return current_.completed ? semantics.after : semantics.before;
}
const WorkingState& LayeredQuestionSession::visibleWorkingState() const noexcept {
  if(current_.math)return current_.math->nodes[current_.math->active].working;
  const auto& states=content().workingStates;
  const auto id=visibleWorkingId();
  // Immutable, validated content guarantees that the reference resolves.
  return *std::find_if(states.begin(),states.end(),[&](const auto& state){return state.id==id;});
}
std::optional<CoordinateGraphView> LayeredQuestionSession::coordinateGraph(float probeX) const noexcept {
  if(!content().lineGraph)return std::nullopt;
  const auto& g=*content().lineGraph;
  const auto point=[](const GraphLine& line,float x) {return GraphPoint{x,static_cast<float>(line.rise)*x/line.run+line.intercept};};
  const auto project=[&](const GraphLine& line) {
    float low=g.xMin,high=g.xMax;
    if(line.rise) {
      const float a=static_cast<float>(g.yMin-line.intercept)*line.run/line.rise;
      const float b=static_cast<float>(g.yMax-line.intercept)*line.run/line.rise;
      low=std::max(low,std::min(a,b));high=std::min(high,std::max(a,b));
    }
    return GraphLineProjection{point(line,low),point(line,high),{}};
  };
  const GraphLine first{g.rise,g.run,g.intercept};const auto line=project(first);
  CoordinateGraphView view{g,*visibleWorkingState().graphStage,{0,static_cast<float>(g.intercept)},
      {static_cast<float>(g.run),static_cast<float>(g.intercept)},
      {static_cast<float>(g.run),static_cast<float>(g.intercept+g.rise)},line.start,line.end,{}};
  view.probeMin=line.start.x;view.probeMax=line.end.x;
  if(g.second) {
    view.second=project(*g.second);
    view.probeMin=std::max(view.probeMin,view.second->start.x);view.probeMax=std::min(view.probeMax,view.second->end.x);
    if(view.stage>=GraphStage::Classified) {
      const auto solution=graphSolution(g);view.relation=solution.relation;
      if(view.stage==GraphStage::SystemSolution)view.intersection=solution.point;
    }
  }
  const float x=std::clamp(std::isfinite(probeX)?probeX:0.0F,view.probeMin,view.probeMax);
  view.probe=point(first,x);if(view.second)view.second->probe=point(*g.second,x);
  view.probeAvailable=g.second?view.stage>=GraphStage::BothLines:view.stage==GraphStage::Line;
  if(view.probeAvailable) {
    view.valueTable.emplace();
    for(std::size_t i=0;i<view.valueTable->size();++i) {
      const float sampleX=std::lerp(view.probeMin,view.probeMax,i/2.0F);
      auto& row=(*view.valueTable)[i];row={sampleX,point(first,sampleX).y,std::nullopt};
      if(g.second)row.secondY=point(*g.second,sampleX).y;
    }
  }
  return view;
}

std::string_view LayeredQuestionSession::visibleWorking() const noexcept {
  return visibleWorkingState().display;
}

std::optional<QuestionReview> LayeredQuestionSession::review(std::size_t runIndex) const {
  if(runIndex>archived_.size())return std::nullopt;
  const auto& run=runIndex==0?current_:archived_[runIndex-1];
  const auto& question=*std::find_if(catalog_->begin(),catalog_->end(),[&](const auto& q) {
    return q.id==run.questionId && q.version==run.contentVersion;
  });
  QuestionReview result;
  result.questionId=question.id;result.equation=question.equation;
  result.version=question.version;result.runNumber=run.runNumber;result.completed=run.completed;
  if(run.math) {
    result.math=&*run.math;
    for(const auto& event:run.math->events)if(event.kind==MathMoveKind::Submit && !event.correct)++result.wrongAttempts;
    return result;
  }
  for(std::size_t i=0;i<=run.currentStep;++i) {
    const auto& record=run.steps[i];const auto& content=question.steps[i];
    QuestionReviewStep step;
    step.id=record.id;step.name=content.layerName;step.prompt=content.prompt;
    step.working=std::find_if(question.workingStates.begin(),question.workingStates.end(),
      [&](const auto& state){return state.id==content.semantics.before;})->display;
    step.required=requiredAnswerCount(content);
    step.collected=static_cast<std::size_t>(std::popcount(record.collectedOptions));
    if(layeredQuestionStepResolved(record))step.explanation=content.explanation;
    if(record.answerShown)step.outcome=QuestionReviewOutcome::AnswerShown;
    else if(record.resolvedByPlayer)step.outcome=record.incorrectCheckedAttempts==0?
      QuestionReviewOutcome::CorrectFirstTry:QuestionReviewOutcome::CorrectAfterRetry;
    result.wrongAttempts+=record.incorrectCheckedAttempts;
    if(record.incorrectCheckedAttempts>0)++result.stepsNeedingRetry;
    for(const auto& attempt:record.attempts)
      step.attempts.push_back({content.options[attempt.optionIndex].label,attempt.correct});
    result.steps.push_back(std::move(step));
  }
  return result;
}

void LayeredQuestionSession::beginRun(std::uint32_t runNumber,
                                      bool priorExposure,
                                      LayeredQuestionPhase phase) {
  current_ = {};
  current_.questionId=content().id;current_.contentVersion=content().version;
  current_.steps.resize(content().steps.size());
  for(std::size_t i=0;i<current_.steps.size();++i)current_.steps[i].id=content().steps[i].id;
  current_.runNumber = runNumber;
  current_.priorExposure = priorExposure;
  current_.phase = phase;
  if(interaction_==QuestionInteraction::MathMoves) {
    current_.steps.clear();
    current_.math.emplace();
    auto equation=*prepareMathWorking(content().mathModel,content().equation).result;
    const auto display=std::visit([](const auto& value){return value.display;},equation);
    const auto instruction=content().mathModel==MathWorkingModel::RowReduction?
        "Columns are x, y and the right-hand value. Reduce the left block to the identity matrix.":
        "Find x using operations that preserve the solutions.";
    current_.math->nodes.push_back({0,{{1},display},std::move(equation),"Original problem",instruction,{}});
  }
}

LayeredQuestionDispatchResult LayeredQuestionSession::dispatch(
    const LayeredQuestionCommand& command) {
  if(current_.math) {
    switch(command.kind) {
      case LayeredQuestionCommandKind::OpenQuestion:case LayeredQuestionCommandKind::BackToGrid:
      case LayeredQuestionCommandKind::RestartQuestion:case LayeredQuestionCommandKind::MathematicalMove:break;
      default:return rejected("prepared_actions_unavailable");
    }
  }
  switch(command.kind) {
    case LayeredQuestionCommandKind::SelectOption:case LayeredQuestionCommandKind::CheckAnswer:
    case LayeredQuestionCommandKind::TryAgain:case LayeredQuestionCommandKind::ShowAnswer:
      if(interaction_!=QuestionInteraction::Guided)return rejected("guided_action_unavailable");
      break;
    default:break;
  }
  switch (command.kind) {
    case LayeredQuestionCommandKind::MathematicalMove:return applyMathMove(command.math);
    case LayeredQuestionCommandKind::OpenQuestion: {
      if (current_.phase != LayeredQuestionPhase::Grid) {
        return rejected("question_not_on_grid");
      }
      current_.phase = current_.completed ? LayeredQuestionPhase::Complete
                                          : LayeredQuestionPhase::Answering;
      return accepted(true, current_.completed ? "summary_opened"
                                               : "question_opened");
    }
    case LayeredQuestionCommandKind::SelectOption: {
      if (current_.phase != LayeredQuestionPhase::Answering) {
        return rejected("question_not_answering");
      }
      if (command.optionIndex >= content().steps[current_.currentStep].options.size()) {
        return rejected("option_index_invalid");
      }
      LayeredQuestionStepRecord& step = current_.steps[current_.currentStep];
      if (layeredQuestionStepResolved(step)) {
        return rejected("step_already_resolved");
      }
      if (step.awaitingRecoveryChoice) {
        return rejected("recovery_choice_required");
      }
      const bool changed = step.selectedOption != command.optionIndex;
      step.selectedOption = command.optionIndex;
      return accepted(changed,
                      changed ? "option_selected" : "selection_unchanged");
    }
    case LayeredQuestionCommandKind::CheckAnswer: {
      if (current_.phase != LayeredQuestionPhase::Answering) {
        return rejected("question_not_answering");
      }
      LayeredQuestionStepRecord& step = current_.steps[current_.currentStep];
      if (layeredQuestionStepResolved(step)) {
        return rejected("step_already_resolved");
      }
      if (step.awaitingRecoveryChoice) {
        return rejected("recovery_choice_required");
      }
      if (!step.selectedOption.has_value()) {
        return rejected("option_not_selected");
      }
      return judgeOption(*step.selectedOption);
    }
    case LayeredQuestionCommandKind::SubmitOption: {
      if(interaction_!=QuestionInteraction::ArcadeCollect)return rejected("arcade_action_unavailable");
      if(current_.phase!=LayeredQuestionPhase::Answering)return rejected("question_not_answering");
      const auto& choices=content().steps[current_.currentStep].options;
      const auto option=std::find_if(choices.begin(),choices.end(),[&](const auto& o){return o.id==command.option;});
      if(option==choices.end())return rejected("unknown_option_id");
      return judgeOption(static_cast<std::size_t>(option-choices.begin()));
    }
    case LayeredQuestionCommandKind::TryAgain: {
      if (current_.phase != LayeredQuestionPhase::Answering) {
        return rejected("question_not_answering");
      }
      LayeredQuestionStepRecord& step = current_.steps[current_.currentStep];
      if (layeredQuestionStepResolved(step) ||
          !step.awaitingRecoveryChoice) {
        return rejected("try_again_not_offered");
      }
      step.selectedOption.reset();
      step.awaitingRecoveryChoice = false;
      return accepted(true, "try_again_started");
    }
    case LayeredQuestionCommandKind::ShowAnswer: {
      if (current_.phase != LayeredQuestionPhase::Answering) {
        return rejected("question_not_answering");
      }
      LayeredQuestionStepRecord& step = current_.steps[current_.currentStep];
      if (layeredQuestionStepResolved(step) ||
          !step.awaitingRecoveryChoice) {
        return rejected("show_answer_not_offered");
      }
      step.answerShown = true;
      step.awaitingRecoveryChoice = false;
      step.selectedOption =
          firstAcceptedOption(content().steps[current_.currentStep]);
      return accepted(true, "answer_shown");
    }
    case LayeredQuestionCommandKind::RequestHint:
    case LayeredQuestionCommandKind::RevealNextMove:
    case LayeredQuestionCommandKind::ApplyPreparedStep: {
      if (current_.phase != LayeredQuestionPhase::Answering)
        return rejected("question_not_answering");
      auto& step = current_.steps[current_.currentStep];
      if (layeredQuestionStepResolved(step)) return rejected("step_already_resolved");
      const auto& prepared = content().steps[current_.currentStep];
      if (command.kind == LayeredQuestionCommandKind::RequestHint) {
        if (prepared.hint.empty()) return rejected("hint_not_prepared");
        const bool changed = !step.hintRequested; step.hintRequested = true;
        return accepted(changed,"hint_requested");
      }
      if (prepared.nextMove.empty()) return rejected("next_move_not_prepared");
      if (command.kind == LayeredQuestionCommandKind::RevealNextMove) {
        const bool changed = !step.nextMoveRequested; step.nextMoveRequested = true;
        return accepted(changed,"next_move_requested");
      }
      step.answerShown = true;
      step.nextMoveRequested = true;
      step.awaitingRecoveryChoice = false;
      step.selectedOption = firstAcceptedOption(prepared);
      return accepted(true,"prepared_step_applied");
    }
    case LayeredQuestionCommandKind::Continue:return advanceResolvedStep();
    case LayeredQuestionCommandKind::BackToGrid: {
      if (current_.phase == LayeredQuestionPhase::Grid) {
        return rejected("already_on_grid");
      }
      current_.phase = LayeredQuestionPhase::Grid;
      return accepted(true, "question_grid_opened");
    }
    case LayeredQuestionCommandKind::RestartQuestion: {
      if (!command.archiveUnfinished && (current_.phase != LayeredQuestionPhase::Complete ||
          !current_.completed)) {
        return rejected("question_not_complete");
      }
      const auto nextQuestion=command.questionIndex.value_or(contentIndex_);
      if(nextQuestion>=catalog_->size())return rejected("unknown_question_index");
      const std::uint32_t nextRun = current_.runNumber + 1U;
      archived_.push_back(current_);
      contentIndex_=nextQuestion;
      const bool prior=std::any_of(archived_.begin(),archived_.end(),[&](const auto& run) {
        return run.questionId==content().id && run.contentVersion==content().version;
      });
      beginRun(nextRun, prior, LayeredQuestionPhase::Answering);
      return accepted(true, "question_restarted");
    }
  }
  return rejected("command_unknown");
}

std::vector<MathMoveChoice> LayeredQuestionSession::mathMoveChoices() const {
  if(!current_.math || current_.completed)return {};
  const auto& run=*current_.math;
  return std::visit([](const auto& before){return availableMathMoves(before);},run.nodes[run.active].equation);
}

LayeredQuestionDispatchResult LayeredQuestionSession::applyMathMove(const MathMoveCommand& command) {
  if(!current_.math)return rejected("math_moves_unavailable");
  auto& run=*current_.math;
  if(command.questionId!=current_.questionId || command.contentVersion!=current_.contentVersion ||
      command.runNumber!=current_.runNumber || command.revision!=run.revision)return rejected("stale_math_move");
  if(current_.phase==LayeredQuestionPhase::Grid)return rejected("question_not_answering");
  if(command.kind!=MathMoveKind::Submit && command.kind!=MathMoveKind::Undo)return rejected("unknown_math_move");
  if(run.events.size()>=kMathEventCapacity)return rejected("math_event_capacity_reached");
  if(command.kind==MathMoveKind::Undo) {
    if(!run.active)return rejected("nothing_to_undo");
    const auto parent=run.nodes[run.active].parent;
    run.events.push_back({MathMoveKind::Undo,run.active,parent,{}, {},{},false,"Returned to the earlier working; all attempts are retained."});
    run.active=parent;++run.revision;current_.completed=false;current_.phase=LayeredQuestionPhase::Answering;
    return accepted(true,"math_move_undone");
  }
  if(current_.completed)return rejected("question_complete");
  if(run.nodes.size()>=kMathNodeCapacity)return rejected("math_node_capacity_reached");
  if(command.entry.size()>160 || command.operand.size()>48)return rejected("math_input_too_long");
  std::optional<MathWorkingNode> node;
  const auto feedback=std::visit([&](const auto& before) {
    const auto checked=checkMathMove(before,command.operation,command.operand,command.entry);
    if(checked.result)node=MathWorkingNode{run.active,{{static_cast<std::uint32_t>(run.nodes.size()+1)},checked.result->display},
        *checked.result,checked.operation,checked.explanation,
        verifyMathSolution(std::get<std::decay_t<decltype(before)>>(run.nodes.front().equation),*checked.result)};
    return checked.feedback;
  },run.nodes[run.active].equation);
  const bool correct=node.has_value();
  const auto next=correct?run.nodes.size():run.active;
  MathMoveEvent event{MathMoveKind::Submit,run.active,next,command.operation,command.operand,command.entry,
      correct,std::string(feedback)};
  // Reserve both append-only stores before publishing either part of a checked move.
  run.events.reserve(run.events.size()+1);
  if(node)run.nodes.reserve(run.nodes.size()+1);
  run.events.push_back(std::move(event));
  if(node) {
    run.nodes.push_back(std::move(*node));run.active=next;
    current_.completed=!run.nodes.back().verification.empty();
    if(current_.completed)current_.phase=LayeredQuestionPhase::Complete;
  }
  ++run.revision;
  return accepted(true,correct?"math_move_correct":"math_move_incorrect");
}

LayeredQuestionDispatchResult LayeredQuestionSession::advanceResolvedStep() {
  if(current_.phase!=LayeredQuestionPhase::Answering)return rejected("question_not_answering");
  if(!layeredQuestionStepResolved(current_.steps[current_.currentStep]))return rejected("step_not_resolved");
  if(current_.currentStep+1<content().steps.size()) {
    ++current_.currentStep;
    return accepted(true,"next_step_opened");
  }
  current_.completed=true;current_.phase=LayeredQuestionPhase::Complete;
  return accepted(true,"question_completed");
}

LayeredQuestionDispatchResult LayeredQuestionSession::judgeOption(std::size_t index) {
  auto& record=current_.steps[current_.currentStep];
  const auto& step=content().steps[current_.currentStep];
  if(layeredQuestionStepResolved(record))return rejected("step_already_resolved");
  if(record.awaitingRecoveryChoice)return rejected("recovery_choice_required");
  if(index>=step.options.size())return rejected("option_index_invalid");
  const auto bit=static_cast<std::uint8_t>(1U << index);
  if(record.collectedOptions & bit)return rejected("option_already_collected");
  const bool correct=acceptsOption(step,index);
  record.attempts.push_back({index,correct,step.options[index].id});
  record.selectedOption=index;
  if(!record.firstCheckedOption) {record.firstCheckedOption=index;record.firstCorrect=correct;}
  if(correct) {
    record.collectedOptions|=bit;
    record.resolvedByPlayer=answerSetComplete(step,record.collectedOptions);
    return accepted(true,record.resolvedByPlayer?"answer_correct":"answer_collected");
  }
  ++record.incorrectCheckedAttempts;
  record.awaitingRecoveryChoice=interaction_==QuestionInteraction::Guided;
  return accepted(true,"answer_incorrect");
}
bool acceptsOption(const LayeredQuestionStepContent& step,std::size_t index) noexcept {
  return index<step.options.size() && index<kQuestionChoiceCapacity && (step.acceptedOptions & (1U << index));
}
std::size_t firstAcceptedOption(const LayeredQuestionStepContent& step) noexcept {
  for(std::size_t i=0;i<step.options.size();++i)if(acceptsOption(step,i))return i;
  return step.options.size();
}
bool answerSetComplete(const LayeredQuestionStepContent& step,std::uint8_t collected) noexcept {
  const auto acceptedCollected=collected & step.acceptedOptions;
  switch(step.semantics.completion) {
    case CompletionRule::AnyAccepted:return acceptedCollected!=0;
    case CompletionRule::AllAccepted:return acceptedCollected==step.acceptedOptions;
  }
  return false;
}
std::size_t requiredAnswerCount(const LayeredQuestionStepContent& step) noexcept {
  switch(step.semantics.completion) {
    case CompletionRule::AnyAccepted:return 1;
    case CompletionRule::AllAccepted:return std::popcount(step.acceptedOptions);
  }
  return 0;
}
}  // namespace iggy3d::first_move

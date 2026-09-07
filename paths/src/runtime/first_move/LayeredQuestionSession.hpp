#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "runtime/first_move/LinearEquation.hpp"

namespace iggy3d::first_move {

inline constexpr std::size_t kLayeredQuestionStepCount = 7U;
inline constexpr std::size_t kLayeredQuestionOptionCount = 4U;
inline constexpr std::string_view kLayeredQuestionId =
    "linear_one_unknown_3a_plus_5_eq_20_v1";
inline constexpr std::uint32_t kLayeredQuestionVersion = 1U;
inline constexpr std::size_t kQuestionChoiceCapacity = 8;
inline constexpr std::size_t kQuestionStepCapacity = 32;
inline constexpr std::size_t kQuestionCatalogCapacity = 64;
// A linear chain of at most 32 steps needs at most 33 distinct snapshots.
inline constexpr std::size_t kQuestionWorkingStateCapacity = kQuestionStepCapacity + 1;
struct OptionId {
  std::uint32_t value = 0;
  bool operator==(const OptionId&) const = default;
};
struct QuestionStepId {
  std::uint32_t value = 0;
  bool operator==(const QuestionStepId&) const = default;
};
struct WorkingStateId {
  std::uint32_t value = 0;
  bool operator==(const WorkingStateId&) const = default;
};
inline constexpr std::size_t kWorkingHighlightCapacity = 8;
struct WorkingHighlight {
  std::uint32_t offset = 0, length = 0; // Ordered, nonoverlapping UTF-8 byte spans.
  std::string label;
};
enum class GraphStage : std::uint8_t { Grid, Intercept, Run, Rise, Line, FirstLine, BothLines, Classified, SystemSolution };
struct GraphPoint { float x=0, y=0; };
struct GraphLine { int rise=0, run=1, intercept=0; };
struct LineGraph {
  int rise=0, run=1, intercept=0;
  int xMin=-4, xMax=4, yMin=-4, yMax=4;
  std::optional<GraphLine> second;
};
enum class GraphRelation : std::uint8_t { Intersecting, Parallel, Coincident };
struct GraphLineProjection { GraphPoint start, end, probe; };
struct GraphValueRow { float x=0, y=0; std::optional<float> secondY; };
struct CoordinateGraphView {
  LineGraph axes;
  GraphStage stage=GraphStage::Grid;
  GraphPoint intercept, corner, tip, lineStart, lineEnd, probe;
  std::optional<GraphLineProjection> second;
  bool probeAvailable=false;
  float probeMin=0, probeMax=0;
  std::optional<GraphRelation> relation;
  std::optional<GraphPoint> intersection;
  std::optional<std::array<GraphValueRow,3>> valueTable;
};
struct WorkingState {
  WorkingStateId id;
  std::string display;  // Prepared text; an empty display is allowed.
  std::vector<WorkingHighlight> highlights;
  std::optional<GraphStage> graphStage;
};
enum class StepPurpose : std::uint8_t { AnswerChoice, OperationChoice, Calculation, Verification, GraphChoice };
enum class CompletionRule : std::uint8_t { AnyAccepted, AllAccepted };
struct StepSemantics {
  StepPurpose purpose = StepPurpose::AnswerChoice;
  CompletionRule completion = CompletionRule::AllAccepted;
  WorkingStateId before, after;
};

struct LayeredQuestionOptionContent {
  std::string label;
  OptionId id;
};

struct LayeredQuestionStepContent {
  std::string layerName;
  std::string prompt;
  std::vector<LayeredQuestionOptionContent> options;
  std::uint8_t acceptedOptions = 0;
  std::string wrongHint;
  std::string explanation;
  QuestionStepId id;
  StepSemantics semantics;
  std::string hint, nextMove;
};

struct LayeredQuestionContent {
  std::string id;
  std::uint32_t version = 0U;
  std::string equation;
  std::string skill;
  std::string description;
  std::vector<LayeredQuestionStepContent> steps;
  std::vector<WorkingState> workingStates;
  std::optional<LineGraph> lineGraph;
  bool supportsMathMoves=false; // Explicit content opt-in; prepared arcade consumers still use their authored chain.
  MathWorkingModel mathModel=MathWorkingModel::LinearEquation;
};

[[nodiscard]] const LayeredQuestionContent& layeredQuestion() noexcept;
[[nodiscard]] bool acceptsOption(const LayeredQuestionStepContent&,std::size_t index) noexcept;
[[nodiscard]] std::size_t firstAcceptedOption(const LayeredQuestionStepContent&) noexcept;
// Content validation establishes a nonempty accepted set and a known rule.
[[nodiscard]] bool answerSetComplete(const LayeredQuestionStepContent&, std::uint8_t collected) noexcept;
[[nodiscard]] std::size_t requiredAnswerCount(const LayeredQuestionStepContent&) noexcept;
enum class QuestionInteraction : std::uint8_t { Guided, ArcadeCollect, MathMoves };

enum class QuestionValidationCode : std::uint8_t {
  Valid, InvalidCatalogSize, InvalidInteraction, MissingQuestionId,
  MissingQuestionVersion, InvalidStepCount, DuplicateQuestionIdentity,
  MissingStepId, MissingPrompt, InvalidOptionCount, EmptyAcceptedOptions,
  AcceptedOptionsOutOfRange, GuidedRequiresSingleAnswer, DuplicateStepIdentity,
  MissingOptionId, MissingOptionLabel, DuplicateOptionIdentity,
  InvalidWorkingStateCount, MissingWorkingStateId, DuplicateWorkingStateIdentity,
  InvalidWorkingHighlight,
  InvalidStepPurpose, InvalidCompletionRule, UnknownWorkingState, BrokenStepChain, InvalidGraph, InvalidMathMoves,
};

struct QuestionValidationResult {
  QuestionValidationCode code = QuestionValidationCode::Valid;
  std::string_view field;  // Static member/parameter name; never borrows content.
  std::optional<std::size_t> questionIndex, stepIndex, optionIndex;
  std::optional<std::size_t> workingStateIndex;  // For a definition; step references use stepIndex.

  [[nodiscard]] bool valid() const noexcept { return code == QuestionValidationCode::Valid; }
  // Retains the constructor's existing exception reasons for invalid content.
  [[nodiscard]] std::string_view reason() const noexcept;
};

// Read-only structural checks. Return the first failure; indices are zero-based
// and absent where not applicable. A standalone question has index 0.
// Success has no field or indices. No mathematical or display-text verification.
[[nodiscard]] QuestionValidationResult validateQuestion(
    const LayeredQuestionContent&, QuestionInteraction) noexcept;
[[nodiscard]] QuestionValidationResult validateCatalog(
    std::span<const LayeredQuestionContent>, QuestionInteraction) noexcept;

enum class LayeredQuestionPhase : std::uint8_t {
  Grid,
  Answering,
  Complete,
};

[[nodiscard]] std::string_view layeredQuestionPhaseName(
    LayeredQuestionPhase phase) noexcept;

struct LayeredQuestionAttemptRecord {
  std::size_t optionIndex = 0U;
  bool correct = false;
  OptionId option;
};

struct LayeredQuestionStepRecord {
  QuestionStepId id;
  std::uint8_t collectedOptions = 0;
  std::vector<LayeredQuestionAttemptRecord> attempts;
  std::optional<std::size_t> firstCheckedOption;
  std::optional<bool> firstCorrect;
  std::optional<std::size_t> selectedOption;
  bool resolvedByPlayer = false;
  bool answerShown = false;
  bool awaitingRecoveryChoice = false;
  std::size_t incorrectCheckedAttempts = 0U;
  bool hintRequested = false, nextMoveRequested = false;
};

inline constexpr std::size_t kMathNodeCapacity=128, kMathEventCapacity=1024;
enum class MathMoveKind : std::uint8_t { Submit, Undo };
struct MathMoveCommand {
  MathMoveKind kind=MathMoveKind::Submit;
  MathOperation operation=MathOperation::Expand;
  std::string operand, entry;
  std::uint32_t runNumber=0;
  std::uint64_t revision=0;
  std::string questionId;
  std::uint32_t contentVersion=0;
};
struct MathWorkingNode {
  std::size_t parent=0;
  WorkingState working;
  MathWorkingValue equation;
  std::string operation, explanation, verification;
};
struct MathMoveEvent {
  MathMoveKind kind=MathMoveKind::Submit;
  std::size_t from=0, to=0;
  MathOperation operation=MathOperation::Expand;
  std::string operand, entry;
  bool correct=false;
  std::string feedback;
};
struct MathMoveRun {
  std::vector<MathWorkingNode> nodes;
  std::vector<MathMoveEvent> events; // Append-only: Undo records a move to a parent; it never deletes evidence.
  std::size_t active=0;
  std::uint64_t revision=1;
};

[[nodiscard]] bool layeredQuestionStepResolved(
    const LayeredQuestionStepRecord& step) noexcept;

struct LayeredQuestionRunRecord {
  std::string questionId;
  std::uint32_t contentVersion = 0;
  std::uint32_t runNumber = 1U;
  bool priorExposure = false;
  std::size_t currentStep = 0U;
  LayeredQuestionPhase phase = LayeredQuestionPhase::Grid;
  bool completed = false;
  std::vector<LayeredQuestionStepRecord> steps;
  std::optional<MathMoveRun> math;
};

struct LayeredQuestionRunSummary {
  std::size_t correctOnFirstTry = 0U;
  std::size_t correctedAfterRetry = 0U;
  std::size_t shownAnswers = 0U;
  std::size_t incorrectCheckedAttempts = 0U;
  bool assisted = false;
  bool completed = false;
};

[[nodiscard]] LayeredQuestionRunSummary summarizeLayeredQuestionRun(
    const LayeredQuestionRunRecord& run) noexcept;

// Review text borrows this session's frozen catalog. Only attempted answers and
// reached steps are exposed; explanations stay empty until that step resolves.
enum class QuestionReviewOutcome { InProgress, CorrectFirstTry, CorrectAfterRetry, AnswerShown };
struct QuestionReviewAttempt { std::string_view label; bool correct; };
struct QuestionReviewStep {
  QuestionStepId id;
  std::string_view name, prompt, working, explanation;
  QuestionReviewOutcome outcome=QuestionReviewOutcome::InProgress;
  std::size_t collected=0, required=0;
  std::vector<QuestionReviewAttempt> attempts;
};
struct QuestionReview {
  std::string_view questionId, equation;
  std::uint32_t version=0, runNumber=0;
  bool completed=false;
  std::size_t wrongAttempts=0, stepsNeedingRetry=0;
  std::vector<QuestionReviewStep> steps;
  const MathMoveRun* math=nullptr; // Same frozen run evidence; no prepared answers for a mathematical-move run.
};

enum class LayeredQuestionCommandKind : std::uint8_t {
  OpenQuestion,
  SelectOption,
  CheckAnswer,
  TryAgain,
  ShowAnswer,
  Continue,
  BackToGrid,
  RestartQuestion,
  SubmitOption,
  RequestHint,
  RevealNextMove,
  ApplyPreparedStep,
  MathematicalMove,
};

struct LayeredQuestionCommand {
  LayeredQuestionCommandKind kind = LayeredQuestionCommandKind::OpenQuestion;
  std::size_t optionIndex = 0U;
  OptionId option;
  std::optional<std::size_t> questionIndex;
  bool archiveUnfinished = false; // Explicit new-set action; ordinary replay still requires completion.
  MathMoveCommand math;

  [[nodiscard]] static constexpr LayeredQuestionCommand selectOption(
      std::size_t index) noexcept {
    return {LayeredQuestionCommandKind::SelectOption, index};
  }
  [[nodiscard]] static constexpr LayeredQuestionCommand submitOption(OptionId id) noexcept {
    LayeredQuestionCommand result;
    result.kind=LayeredQuestionCommandKind::SubmitOption;result.option=id;return result;
  }
};

struct LayeredQuestionDispatchResult {
  bool accepted = false;
  bool changed = false;
  std::string_view reason = "command_rejected";
};

class LayeredQuestionSession {
public:
  LayeredQuestionSession();
  LayeredQuestionSession(std::vector<LayeredQuestionContent> catalog,
                         QuestionInteraction interaction,std::size_t initialQuestion = 0);

  [[nodiscard]] LayeredQuestionDispatchResult dispatch(
      const LayeredQuestionCommand& command);

  [[nodiscard]] const LayeredQuestionRunRecord& currentRun() const noexcept;
  [[nodiscard]] const std::vector<LayeredQuestionRunRecord>& archivedRuns()
      const noexcept;
  [[nodiscard]] const LayeredQuestionContent& content() const noexcept { return (*catalog_)[contentIndex_]; }
  [[nodiscard]] std::size_t contentIndex() const noexcept { return contentIndex_; }
  [[nodiscard]] QuestionInteraction interaction() const noexcept { return interaction_; }
  [[nodiscard]] WorkingStateId visibleWorkingId() const noexcept;
  // Borrows immutable content for the lifetime of this session's catalog.
  [[nodiscard]] const WorkingState& visibleWorkingState() const noexcept;
  [[nodiscard]] std::string_view visibleWorking() const noexcept;
  // Read-only mathematical projection. Moving the probe does not submit an
  // answer, change the working state, or create attempt/assistance evidence.
  [[nodiscard]] std::optional<CoordinateGraphView> coordinateGraph(float probeX=0) const noexcept;
  [[nodiscard]] std::vector<MathMoveChoice> mathMoveChoices() const;

  // 0 selects the current run; 1..N select archived runs in their stored order.
  // Invalid selection returns nullopt. This does not change progression or evidence.
  [[nodiscard]] std::optional<QuestionReview> review(std::size_t runIndex=0) const;

private:
  void beginRun(std::uint32_t runNumber,
                bool priorExposure,
                LayeredQuestionPhase phase);
  [[nodiscard]] LayeredQuestionDispatchResult judgeOption(std::size_t index);
  [[nodiscard]] LayeredQuestionDispatchResult advanceResolvedStep();
  [[nodiscard]] LayeredQuestionDispatchResult applyMathMove(const MathMoveCommand&);

  std::shared_ptr<const std::vector<LayeredQuestionContent>> catalog_;
  std::size_t contentIndex_ = 0;
  QuestionInteraction interaction_ = QuestionInteraction::Guided;
  LayeredQuestionRunRecord current_{};
  std::vector<LayeredQuestionRunRecord> archived_;
};

}  // namespace iggy3d::first_move

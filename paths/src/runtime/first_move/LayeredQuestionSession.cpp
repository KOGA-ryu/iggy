#include "runtime/first_move/LayeredQuestionSession.hpp"

namespace iggy3d::first_move {
namespace {

constexpr LayeredQuestionContent kQuestion{
    kLayeredQuestionId,
    kLayeredQuestionVersion,
    "3a + 5 = 20",
    "linear_equation_one_unknown",
    "Vocabulary and solving · 7 short steps",
    {{
        {"NAME IT",
         "What kind of mathematical statement is 3a + 5 = 20?",
         {{{"An expression with no equals sign"},
           {"A linear equation in one unknown"},
           {"A quadratic equation"},
           {"An inequality"}}},
         1U,
         "That choice does not fit this step. You can try again or see the answer and why.",
         "It is an equation because it has an equals sign. It is linear in one unknown because a appears only to the first power."},
        {"FIND THE UNKNOWN",
         "Which symbol is the unknown—the value we are trying to find?",
         {{{"3"}, {"5"}, {"a"}, {"20"}}},
         2U,
         "That choice does not fit this step. You can try again or see the answer and why.",
         "a is the unknown. The numbers 3, 5, and 20 are known values."},
        {"NAME A PART",
         "What is the role of 3 in the term 3a?",
         {{{"The number being added to a"},
           {"The coefficient multiplying a"},
           {"The solution of the equation"},
           {"The right-hand side"}}},
         1U,
         "That choice does not fit this step. You can try again or see the answer and why.",
         "3 is the coefficient of a. The term 3a means 3 times a."},
        {"CHOOSE THE FIRST MOVE",
         "Which operation removes +5 from the left side while keeping both sides equal?",
         {{{"Add 5 to both sides"},
           {"Subtract 3 from both sides"},
           {"Subtract 5 from both sides"},
           {"Divide only the left side by 3"}}},
         2U,
         "That choice does not fit this step. You can try again or see the answer and why.",
         "Subtracting 5 from both sides preserves equality and removes the +5 from the left side."},
        {"SIMPLIFY",
         "After subtracting 5 from both sides, what equation remains?",
         {{{"3a = 15"}, {"3a = 25"}, {"a = 15"}, {"3a = 20"}}},
         0U,
         "That choice does not fit this step. You can try again or see the answer and why.",
         "3a + 5 - 5 = 20 - 5 simplifies to 3a = 15.", "3a + 5 - 5 = 20 - 5"},
        {"CHOOSE THE SECOND MOVE",
         "What move now isolates a?",
         {{{"Multiply both sides by 3"},
           {"Divide both sides by 3"},
           {"Subtract 3 from both sides"},
           {"Divide both sides by 5"}}},
         1U,
         "That choice does not fit this step. You can try again or see the answer and why.",
         "Dividing both sides by 3 leaves a by itself and preserves equality.", "3a = 15"},
        {"STATE THE SOLUTION",
         "What is the solution?",
         {{{"a = 15"}, {"a = 8"}, {"a = 25"}, {"a = 5"}}},
         3U,
         "That choice does not fit this step. You can try again or see the answer and why.",
         "3a = 15 gives a = 5. Substitution checks it: 3(5) + 5 = 20.", "3a / 3 = 15 / 3"},
    }}};

[[nodiscard]] LayeredQuestionDispatchResult accepted(
    bool changed,
    std::string_view reason) noexcept {
  return {true, changed, reason};
}

[[nodiscard]] LayeredQuestionDispatchResult rejected(
    std::string_view reason) noexcept {
  return {false, false, reason};
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
  for (const LayeredQuestionStepRecord& step : run.steps) {
    if (step.firstCorrect.value_or(false)) {
      ++summary.correctOnFirstTry;
    } else if (step.resolvedByPlayer && step.firstCorrect.has_value()) {
      ++summary.correctedAfterRetry;
    }
    if (step.answerShown) {
      ++summary.shownAnswers;
    }
    summary.incorrectCheckedAttempts += step.incorrectCheckedAttempts;
  }
  summary.assisted = summary.shownAnswers > 0U;
  return summary;
}

LayeredQuestionSession::LayeredQuestionSession() {
  beginRun(1U, false, LayeredQuestionPhase::Grid);
}

const LayeredQuestionRunRecord& LayeredQuestionSession::currentRun()
    const noexcept {
  return current_;
}

const std::vector<LayeredQuestionRunRecord>&
LayeredQuestionSession::archivedRuns() const noexcept {
  return archived_;
}

void LayeredQuestionSession::beginRun(std::uint32_t runNumber,
                                      bool priorExposure,
                                      LayeredQuestionPhase phase) {
  current_ = {};
  current_.runNumber = runNumber;
  current_.priorExposure = priorExposure;
  current_.phase = phase;
}

LayeredQuestionDispatchResult LayeredQuestionSession::dispatch(
    const LayeredQuestionCommand& command) {
  switch (command.kind) {
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
      if (command.optionIndex >= kLayeredQuestionOptionCount) {
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
      const bool correct =
          *step.selectedOption ==
          layeredQuestion().steps[current_.currentStep].correctOption;
      step.attempts.push_back({*step.selectedOption, correct});
      if (!step.firstCheckedOption.has_value()) {
        step.firstCheckedOption = step.selectedOption;
        step.firstCorrect = correct;
      }
      if (correct) {
        step.resolvedByPlayer = true;
        return accepted(true, "answer_correct");
      }
      ++step.incorrectCheckedAttempts;
      step.awaitingRecoveryChoice = true;
      return accepted(true, "answer_incorrect");
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
          layeredQuestion().steps[current_.currentStep].correctOption;
      return accepted(true, "answer_shown");
    }
    case LayeredQuestionCommandKind::Continue: {
      if (current_.phase != LayeredQuestionPhase::Answering) {
        return rejected("question_not_answering");
      }
      if (!layeredQuestionStepResolved(
              current_.steps[current_.currentStep])) {
        return rejected("step_not_resolved");
      }
      if (current_.currentStep + 1U < kLayeredQuestionStepCount) {
        ++current_.currentStep;
        return accepted(true, "next_step_opened");
      }
      current_.completed = true;
      current_.phase = LayeredQuestionPhase::Complete;
      return accepted(true, "question_completed");
    }
    case LayeredQuestionCommandKind::BackToGrid: {
      if (current_.phase == LayeredQuestionPhase::Grid) {
        return rejected("already_on_grid");
      }
      current_.phase = LayeredQuestionPhase::Grid;
      return accepted(true, "question_grid_opened");
    }
    case LayeredQuestionCommandKind::RestartQuestion: {
      if (current_.phase != LayeredQuestionPhase::Complete ||
          !current_.completed) {
        return rejected("question_not_complete");
      }
      const std::uint32_t nextRun = current_.runNumber + 1U;
      archived_.push_back(current_);
      beginRun(nextRun, true, LayeredQuestionPhase::Answering);
      return accepted(true, "question_restarted");
    }
  }
  return rejected("command_unknown");
}

}  // namespace iggy3d::first_move

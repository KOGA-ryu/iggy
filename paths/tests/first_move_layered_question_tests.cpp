#include "runtime/first_move/LayeredQuestionSession.hpp"

#include <array>
#include <iostream>
#include <string_view>

namespace fm = iggy3d::first_move;
namespace {

int failures = 0;
void expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

fm::LayeredQuestionDispatchResult send(
    fm::LayeredQuestionSession& session,
    fm::LayeredQuestionCommandKind kind) {
  return session.dispatch({kind});
}

void open(fm::LayeredQuestionSession& session) {
  expect(send(session, fm::LayeredQuestionCommandKind::OpenQuestion).accepted,
         "question opens");
}

void answerCorrect(fm::LayeredQuestionSession& session) {
  const std::size_t step = session.currentRun().currentStep;
  const std::size_t option = fm::layeredQuestion().steps[step].correctOption;
  expect(session.dispatch(fm::LayeredQuestionCommand::selectOption(option)).accepted,
         "correct option selects");
  expect(send(session, fm::LayeredQuestionCommandKind::CheckAnswer).accepted,
         "correct option checks");
}

void completeCorrect(fm::LayeredQuestionSession& session) {
  for (std::size_t step = 0; step < fm::kLayeredQuestionStepCount; ++step) {
    answerCorrect(session);
    expect(send(session, fm::LayeredQuestionCommandKind::Continue).accepted,
           "resolved step continues");
  }
}

void testExactContent() {
  const fm::LayeredQuestionContent& q = fm::layeredQuestion();
  expect(q.id == "linear_one_unknown_3a_plus_5_eq_20_v1" && q.version == 1U &&
             q.equation == "3a + 5 = 20" &&
             q.skill == "linear_equation_one_unknown" &&
             q.description == "Vocabulary and solving · 7 short steps",
         "question identity is exact");
  constexpr std::array<std::string_view, 7U> prompts{{
      "What kind of mathematical statement is 3a + 5 = 20?",
      "Which symbol is the unknown—the value we are trying to find?",
      "What is the role of 3 in the term 3a?",
      "Which operation removes +5 from the left side while keeping both sides equal?",
      "After subtracting 5 from both sides, what equation remains?",
      "What move now isolates a?", "What is the solution?"}};
  constexpr std::array<std::size_t, 7U> correct{{1U, 2U, 1U, 2U, 0U, 1U, 3U}};
  constexpr std::array<std::array<std::string_view, 4U>, 7U> options{{
      {{"An expression with no equals sign", "A linear equation in one unknown", "A quadratic equation", "An inequality"}},
      {{"3", "5", "a", "20"}},
      {{"The number being added to a", "The coefficient multiplying a", "The solution of the equation", "The right-hand side"}},
      {{"Add 5 to both sides", "Subtract 3 from both sides", "Subtract 5 from both sides", "Divide only the left side by 3"}},
      {{"3a = 15", "3a = 25", "a = 15", "3a = 20"}},
      {{"Multiply both sides by 3", "Divide both sides by 3", "Subtract 3 from both sides", "Divide both sides by 5"}},
      {{"a = 15", "a = 8", "a = 25", "a = 5"}},
  }};
  constexpr std::string_view neutralRecovery =
      "That choice does not fit this step. You can try again or see the answer and why.";
  constexpr std::array<std::string_view, 7U> workingLines{{
      "", "", "", "", "3a + 5 - 5 = 20 - 5", "3a = 15", "3a / 3 = 15 / 3"}};
  constexpr std::array<std::string_view, 7U> explanations{{
      "It is an equation because it has an equals sign. It is linear in one unknown because a appears only to the first power.",
      "a is the unknown. The numbers 3, 5, and 20 are known values.",
      "3 is the coefficient of a. The term 3a means 3 times a.",
      "Subtracting 5 from both sides preserves equality and removes the +5 from the left side.",
      "3a + 5 - 5 = 20 - 5 simplifies to 3a = 15.",
      "Dividing both sides by 3 leaves a by itself and preserves equality.",
      "3a = 15 gives a = 5. Substitution checks it: 3(5) + 5 = 20."}};
  for (std::size_t step = 0; step < q.steps.size(); ++step) {
    expect(q.steps[step].prompt == prompts[step] &&
               q.steps[step].correctOption == correct[step] &&
               q.steps[step].wrongHint == neutralRecovery &&
               q.steps[step].workingLine == workingLines[step] &&
               q.steps[step].explanation == explanations[step],
           "step prompt, answer, hint, and explanation are exact");
    for (std::size_t option = 0; option < 4U; ++option) {
      expect(q.steps[step].options[option].label == options[step][option],
             "option order is exact");
    }
  }
}

void testIndependentRunAndIdempotence() {
  fm::LayeredQuestionSession session;
  open(session);
  completeCorrect(session);
  const fm::LayeredQuestionRunSummary summary =
      fm::summarizeLayeredQuestionRun(session.currentRun());
  expect(session.currentRun().phase == fm::LayeredQuestionPhase::Complete &&
             summary.completed && summary.correctOnFirstTry == 7U &&
             summary.correctedAfterRetry == 0U && summary.shownAnswers == 0U &&
             summary.incorrectCheckedAttempts == 0U && !summary.assisted,
         "fully correct run completes without answer reveals");
  expect(!send(session, fm::LayeredQuestionCommandKind::Continue).accepted,
         "repeated continue is rejected");
  expect(send(session, fm::LayeredQuestionCommandKind::RestartQuestion).accepted,
         "completed question restarts");
  expect(session.archivedRuns().size() == 1U &&
             session.currentRun().runNumber == 2U &&
             session.currentRun().priorExposure,
         "practice archives one completed run and marks prior exposure");
  expect(!send(session, fm::LayeredQuestionCommandKind::RestartQuestion).accepted &&
             session.archivedRuns().size() == 1U,
         "repeated or early restart cannot duplicate archives");
}

void testRetryPreservesAttemptsAndResume() {
  fm::LayeredQuestionSession session;
  expect(!send(session, fm::LayeredQuestionCommandKind::RestartQuestion).accepted,
         "restart is refused before completion");
  open(session);
  expect(session.dispatch(fm::LayeredQuestionCommand::selectOption(0U)).accepted,
         "wrong choice selects");
  expect(send(session, fm::LayeredQuestionCommandKind::CheckAnswer).accepted,
         "wrong choice checks");
  expect(!send(session, fm::LayeredQuestionCommandKind::CheckAnswer).accepted,
         "repeated check cannot duplicate an attempt");
  expect(send(session, fm::LayeredQuestionCommandKind::BackToGrid).accepted &&
             session.currentRun().currentStep == 0U,
         "back to grid retains current step");
  open(session);
  expect(session.currentRun().steps[0U].attempts.size() == 1U &&
             session.currentRun().steps[0U].awaitingRecoveryChoice,
         "resume retains wrong attempt and recovery choice");
  expect(send(session, fm::LayeredQuestionCommandKind::TryAgain).accepted,
         "try again is offered");
  expect(!send(session, fm::LayeredQuestionCommandKind::TryAgain).accepted,
         "try again cannot repeat");
  answerCorrect(session);
  const fm::LayeredQuestionStepRecord& step = session.currentRun().steps[0U];
  const fm::LayeredQuestionRunSummary summary =
      fm::summarizeLayeredQuestionRun(session.currentRun());
  expect(step.firstCheckedOption == 0U && step.firstCorrect == false &&
             step.attempts.size() == 2U &&
             step.attempts[0U].optionIndex == 0U && !step.attempts[0U].correct &&
             step.attempts[1U].optionIndex == 1U && step.attempts[1U].correct &&
             step.resolvedByPlayer && !step.answerShown &&
             step.incorrectCheckedAttempts == 1U &&
             summary.correctedAfterRetry == 1U && !summary.assisted,
         "retry keeps immutable ordered evidence and independent correction credit");
}

void testShowAnswerRecordsAssistance() {
  fm::LayeredQuestionSession session;
  open(session);
  static_cast<void>(session.dispatch(fm::LayeredQuestionCommand::selectOption(0U)));
  static_cast<void>(send(session, fm::LayeredQuestionCommandKind::CheckAnswer));
  expect(send(session, fm::LayeredQuestionCommandKind::ShowAnswer).accepted,
         "show answer resolves after a wrong check");
  expect(!send(session, fm::LayeredQuestionCommandKind::ShowAnswer).accepted,
         "show answer cannot repeat");
  const fm::LayeredQuestionStepRecord& step = session.currentRun().steps[0U];
  const fm::LayeredQuestionRunSummary summary =
      fm::summarizeLayeredQuestionRun(session.currentRun());
  expect(step.firstCheckedOption == 0U && step.firstCorrect == false &&
             !step.resolvedByPlayer && step.answerShown &&
             step.selectedOption == 1U && step.attempts.size() == 1U &&
             summary.correctedAfterRetry == 0U && summary.shownAnswers == 1U &&
             summary.assisted,
         "shown answer preserves first error without false success credit");
}

}  // namespace

int main() {
  testExactContent();
  testIndependentRunAndIdempotence();
  testRetryPreservesAttemptsAndResume();
  testShowAnswerRecordsAssistance();
  if (failures != 0) {
    std::cerr << failures << " first_move_layered_question_tests failure(s)\n";
    return 1;
  }
  std::cout << "first_move_layered_question_tests passed\n";
  return 0;
}

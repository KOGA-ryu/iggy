#include "runtime/first_move/LayeredQuestionSession.hpp"
#include "content/QuestionContentIO.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace fm = iggy3d::first_move;
namespace {

const auto& foundationQuestions() {
  static const auto pack = paths::loadQuestionPack(PATHS_TEST_PACK);
  return pack.catalog;
}

int failures = 0;
void expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

void expectValidation(const fm::QuestionValidationResult& actual,
                      const fm::QuestionValidationResult& expected) {
  const bool matches = actual.code == expected.code && actual.field == expected.field &&
      actual.questionIndex == expected.questionIndex && actual.stepIndex == expected.stepIndex &&
      actual.optionIndex == expected.optionIndex && actual.workingStateIndex == expected.workingStateIndex &&
      actual.valid() == expected.valid();
  if (!matches) {
    std::cerr << "Validation mismatch: expected code " << static_cast<int>(expected.code)
              << " at " << expected.field << ", got " << static_cast<int>(actual.code)
              << " at " << actual.field << '\n';
  }
  expect(matches, "validation returns the expected code, field, and full location");
}

void expectInvalidCatalog(const std::vector<fm::LayeredQuestionContent>& catalog,
                          fm::QuestionInteraction interaction,
                          const fm::QuestionValidationResult& expected,
                          std::string_view reason) {
  const auto result = fm::validateCatalog(catalog, interaction);
  expectValidation(result, expected);
  expect(!result.valid() && result.reason() == reason, "validation retains the constructor reason");
  try {
    fm::LayeredQuestionSession invalid(catalog, interaction);
    expect(false, "constructor must reject structurally invalid content");
  } catch (const std::invalid_argument& error) {
    expect(error.what() == reason, "constructor consumes the shared validation reason");
  }
}

void testValidStructuralContent() {
  for (const auto interaction : {fm::QuestionInteraction::Guided, fm::QuestionInteraction::ArcadeCollect}) {
    expectValidation(fm::validateQuestion(fm::layeredQuestion(), interaction), {});
    const std::vector catalog{fm::layeredQuestion()};
    expectValidation(fm::validateCatalog(catalog, interaction), {});
  }
  expectValidation(fm::validateCatalog(foundationQuestions(), fm::QuestionInteraction::ArcadeCollect), {});

  auto content = fm::layeredQuestion();
  content.id = " ";  // Existing string checks require nonempty values, without trimming.
  content.equation.clear();
  content.skill.clear();
  content.description.clear();
  for (auto& step : content.steps) {
    step.layerName.clear();
    step.wrongHint.clear();
    step.explanation.clear();
    step.prompt = " ";
    for (auto& option : step.options) option.label = " ";
  }
  for (auto& working : content.workingStates) working.display.clear();
  // Step IDs repeat across questions; option IDs repeat across steps. Labels may repeat.
  std::vector catalog{content, content};
  ++catalog[1].version;
  for (const auto interaction : {fm::QuestionInteraction::Guided, fm::QuestionInteraction::ArcadeCollect}) {
    expectValidation(fm::validateCatalog(catalog, interaction), {});
    fm::LayeredQuestionSession session(catalog, interaction, 1);
    expect(session.contentIndex() == 1 && session.currentRun().contentVersion == 2 &&
               session.content().equation.empty() && session.currentRun().steps[0].attempts.empty(),
           "optional text, local identity scopes, and distinct versions retain constructor compatibility");
  }
}

void testMissingRequiredContent() {
  using Code = fm::QuestionValidationCode;
  struct Case {
    void (*change)(fm::LayeredQuestionContent&);
    fm::QuestionValidationResult expected;
    std::string_view reason;
  };
  const std::array cases{
      Case{[](auto& q) { q.id.clear(); }, {Code::MissingQuestionId, "id", 0}, "invalid_question_content"},
      Case{[](auto& q) { q.version = 0; }, {Code::MissingQuestionVersion, "version", 0}, "invalid_question_content"},
      Case{[](auto& q) { q.steps[1].id = {}; }, {Code::MissingStepId, "id", 0, 1}, "invalid_question_step"},
      Case{[](auto& q) { q.steps[1].prompt.clear(); }, {Code::MissingPrompt, "prompt", 0, 1}, "invalid_question_step"},
      Case{[](auto& q) { q.steps[1].options[2].id = {}; }, {Code::MissingOptionId, "id", 0, 1, 2}, "invalid_option"},
      Case{[](auto& q) { q.steps[1].options[2].label.clear(); }, {Code::MissingOptionLabel, "label", 0, 1, 2}, "invalid_option"},
  };
  for (const auto& entry : cases) {
    std::vector catalog{fm::layeredQuestion(), fm::layeredQuestion()};
    catalog[1].id += "_second";
    entry.change(catalog[1]);
    for (const auto interaction : {fm::QuestionInteraction::Guided, fm::QuestionInteraction::ArcadeCollect}) {
      expectValidation(fm::validateQuestion(catalog[1], interaction), entry.expected);
      auto expected = entry.expected;
      expected.questionIndex = 1;
      expectInvalidCatalog(catalog, interaction, expected, entry.reason);
    }
  }
}

void testDuplicateIdentitiesAndFailureOrder() {
  using Code = fm::QuestionValidationCode;
  const auto interaction = fm::QuestionInteraction::Guided;
  std::vector catalog{fm::layeredQuestion(), fm::layeredQuestion()};
  expectInvalidCatalog(catalog, interaction, {Code::DuplicateQuestionIdentity, "id", 1}, "duplicate_question_identity");
  catalog[1].steps[1].prompt.clear();
  expectInvalidCatalog(catalog, interaction, {Code::DuplicateQuestionIdentity, "id", 1}, "duplicate_question_identity");
  catalog[1].steps.clear();
  expectInvalidCatalog(catalog, interaction, {Code::InvalidStepCount, "steps", 1}, "invalid_question_content");

  catalog[1] = fm::layeredQuestion();
  catalog[1].id += "_second";
  catalog[1].steps[1].id = catalog[1].steps[0].id;
  expectValidation(fm::validateQuestion(catalog[1], interaction), {Code::DuplicateStepIdentity, "id", 0, 1});
  expectInvalidCatalog(catalog, interaction, {Code::DuplicateStepIdentity, "id", 1, 1}, "duplicate_step_identity");
  catalog[1].steps[1].id = {2};
  catalog[1].steps[1].options[2].id = catalog[1].steps[1].options[0].id;
  expectValidation(fm::validateQuestion(catalog[1], interaction), {Code::DuplicateOptionIdentity, "id", 0, 1, 2});
  expectInvalidCatalog(catalog, interaction, {Code::DuplicateOptionIdentity, "id", 1, 1, 2}, "duplicate_option_identity");

  catalog[0].steps[1].prompt.clear();
  catalog[1].id.clear();
  expectInvalidCatalog(catalog, interaction, {Code::MissingPrompt, "prompt", 0, 1}, "invalid_question_step");
}

void testAcceptedMasksAndInteractionValidation() {
  using Code = fm::QuestionValidationCode;
  const auto guided = fm::QuestionInteraction::Guided;
  const auto arcade = fm::QuestionInteraction::ArcadeCollect;
  auto content = fm::layeredQuestion();
  for (const auto interaction : {guided, arcade}) {
    content.steps[1].acceptedOptions = 0;
    expectValidation(fm::validateQuestion(content, interaction), {Code::EmptyAcceptedOptions, "acceptedOptions", 0, 1});
    expectInvalidCatalog({content}, interaction, {Code::EmptyAcceptedOptions, "acceptedOptions", 0, 1}, "invalid_question_step");
    content.steps[1].acceptedOptions = 0x10;
    expectValidation(fm::validateQuestion(content, interaction), {Code::AcceptedOptionsOutOfRange, "acceptedOptions", 0, 1});
    expectInvalidCatalog({content}, interaction, {Code::AcceptedOptionsOutOfRange, "acceptedOptions", 0, 1}, "invalid_question_step");
  }
  content.steps[1].acceptedOptions = 3;
  expectValidation(fm::validateQuestion(content, arcade), {});
  expectValidation(fm::validateCatalog(std::vector{content}, arcade), {});
  expectValidation(fm::validateQuestion(content, guided), {Code::GuidedRequiresSingleAnswer, "acceptedOptions", 0, 1});
  expectInvalidCatalog({content}, guided, {Code::GuidedRequiresSingleAnswer, "acceptedOptions", 0, 1}, "invalid_question_step");

  auto& step = content.steps[1];
  for (std::uint32_t id = 5; id <= 8; ++id) step.options.push_back({"extra choice", {id}});
  for (unsigned bit = 0; bit < 8; ++bit) {
    step.acceptedOptions = static_cast<std::uint8_t>(1U << bit);
    for (const auto interaction : {guided, arcade})
      expectValidation(fm::validateQuestion(content, interaction), {});
  }
  step.acceptedOptions = 0xff;
  expectValidation(fm::validateQuestion(content, arcade), {});
  fm::LayeredQuestionSession allAccepted({content}, arcade);
  expect(allAccepted.content().steps[1].acceptedOptions == 0xff, "ArcadeCollect permits every option to be accepted");
  expectInvalidCatalog({content}, guided, {Code::GuidedRequiresSingleAnswer, "acceptedOptions", 0, 1}, "invalid_question_step");

  const auto unknown = static_cast<fm::QuestionInteraction>(255);
  expectValidation(fm::validateQuestion(content, unknown), {Code::InvalidInteraction, "interaction"});
  expectInvalidCatalog({content}, unknown, {Code::InvalidInteraction, "interaction"}, "invalid_question_catalog");
}

void testStructuralCapacityBoundaries() {
  using Code = fm::QuestionValidationCode;
  expect(fm::kQuestionCatalogCapacity == 64 && fm::kQuestionStepCapacity == 32 && fm::kQuestionChoiceCapacity == 8,
         "published limits preserve the existing 64/32/8 capacities");
  for (const auto interaction : {fm::QuestionInteraction::Guided, fm::QuestionInteraction::ArcadeCollect}) {
    auto content = fm::layeredQuestion();
    content.steps.resize(1);
    for (const std::size_t count : {0U, 1U, 9U, 64U}) {
      content.steps[0].options.resize(count);
      expectValidation(fm::validateQuestion(content, interaction), {Code::InvalidOptionCount, "options", 0, 0});
      expectInvalidCatalog({content}, interaction, {Code::InvalidOptionCount, "options", 0, 0}, "invalid_question_step");
    }
    for (const std::size_t count : {2U, 8U}) {
      content.steps[0].options.clear();
      for (std::uint32_t id = 1; id <= count; ++id) content.steps[0].options.push_back({"choice", {id}});
      content.steps[0].acceptedOptions = static_cast<std::uint8_t>(1U << (count - 1));
      expectValidation(fm::validateQuestion(content, interaction), {});
      fm::LayeredQuestionSession session({content}, interaction);
      expect(session.content().steps[0].options.size() == count, "minimum and maximum option counts construct unchanged");
    }
    const auto step = content.steps[0];
    for (const std::size_t count : {0U, 33U}) {
      content.steps.resize(count, step);
      expectValidation(fm::validateQuestion(content, interaction), {Code::InvalidStepCount, "steps", 0});
      expectInvalidCatalog({content}, interaction, {Code::InvalidStepCount, "steps", 0}, "invalid_question_content");
    }
    content.steps.resize(32, step);
    content.workingStates.clear();
    for (std::uint32_t id = 100; id <= 132; ++id) content.workingStates.push_back({{id}, "prepared working"});
    for (std::size_t i = 0; i < content.steps.size(); ++i) {
      content.steps[i].id = {static_cast<std::uint32_t>(i + 1)};
      content.steps[i].semantics.before = {static_cast<std::uint32_t>(100 + i)};
      content.steps[i].semantics.after = {static_cast<std::uint32_t>(101 + i)};
    }
    expectValidation(fm::validateQuestion(content, interaction), {});
    std::vector catalog(64, content);
    for (std::size_t i = 0; i < catalog.size(); ++i) catalog[i].version = static_cast<std::uint32_t>(i + 1);
    expectValidation(fm::validateCatalog(catalog, interaction), {});
    fm::LayeredQuestionSession maximum(catalog, interaction, 63);
    expect(maximum.currentRun().steps.size() == 32 && maximum.content().steps[31].options.size() == 8 &&
               maximum.currentRun().contentVersion == 64 && maximum.content().workingStates.size() == 33,
           "all maximum capacities construct together with the final initial question");
    catalog.push_back(content);
    expectInvalidCatalog(catalog, interaction, {Code::InvalidCatalogSize, "catalog"}, "invalid_question_catalog");
    expectInvalidCatalog({}, interaction, {Code::InvalidCatalogSize, "catalog"}, "invalid_question_catalog");
  }
}

void testInitialQuestionRemainsConstructorSpecific() {
  const std::vector catalog{fm::layeredQuestion()};
  expectValidation(fm::validateCatalog(catalog, fm::QuestionInteraction::Guided), {});
  try {
    fm::LayeredQuestionSession invalid(catalog, fm::QuestionInteraction::Guided, catalog.size());
    expect(false, "constructor must still reject an initial question outside the valid catalog");
  } catch (const std::invalid_argument& error) {
    expect(std::string_view(error.what()) == "invalid_question_catalog", "initial-index rejection keeps its existing reason");
  }
}

void testWorkingChainValidation() {
  using Code = fm::QuestionValidationCode;
  struct Case {
    void (*change)(fm::LayeredQuestionContent&);
    fm::QuestionValidationResult expected;
    std::string_view reason;
  };
  const std::array cases{
    Case{[](auto& q) { q.workingStates.clear(); }, {Code::InvalidWorkingStateCount, "workingStates", 0}, "invalid_working_state_count"},
    Case{[](auto& q) { q.workingStates.resize(34); }, {Code::InvalidWorkingStateCount, "workingStates", 0}, "invalid_working_state_count"},
    Case{[](auto& q) { q.workingStates[1].id = {}; }, {Code::MissingWorkingStateId, "id", 0, {}, {}, 1}, "invalid_working_state"},
    Case{[](auto& q) { q.workingStates[1].id = q.workingStates[0].id; },
         {Code::DuplicateWorkingStateIdentity, "id", 0, {}, {}, 1}, "duplicate_working_state_identity"},
    Case{[](auto& q) { q.steps[1].semantics.before = {}; }, {Code::UnknownWorkingState, "semantics.before", 0, 1}, "unknown_working_state"},
    Case{[](auto& q) { q.steps[1].semantics.after = {999}; }, {Code::UnknownWorkingState, "semantics.after", 0, 1}, "unknown_working_state"},
    Case{[](auto& q) { q.steps[1].semantics.before = {20}; }, {Code::BrokenStepChain, "semantics.before", 0, 1}, "broken_step_chain"},
    Case{[](auto& q) { q.steps[1].semantics.purpose = static_cast<fm::StepPurpose>(255); },
         {Code::InvalidStepPurpose, "semantics.purpose", 0, 1}, "invalid_step_semantics"},
    Case{[](auto& q) { q.steps[1].semantics.completion = static_cast<fm::CompletionRule>(255); },
         {Code::InvalidCompletionRule, "semantics.completion", 0, 1}, "invalid_step_semantics"},
  };
  for (const auto& entry : cases) {
    std::vector catalog{fm::layeredQuestion(), fm::layeredQuestion()};
    ++catalog[1].version;
    entry.change(catalog[1]);
    for (const auto interaction : {fm::QuestionInteraction::Guided, fm::QuestionInteraction::ArcadeCollect}) {
      expectValidation(fm::validateQuestion(catalog[1], interaction), entry.expected);
      auto expected = entry.expected;
      expected.questionIndex = 1;
      expectInvalidCatalog(catalog, interaction, expected, entry.reason);
    }
  }
  auto content = fm::layeredQuestion();
  std::reverse(content.workingStates.begin(), content.workingStates.end());
  expectValidation(fm::validateQuestion(content, fm::QuestionInteraction::Guided), {});
  fm::LayeredQuestionSession session({content}, fm::QuestionInteraction::Guided);
  expect(session.visibleWorkingId() == fm::WorkingStateId{10} && session.visibleWorking() == content.equation,
         "working references use stable IDs rather than vector positions");
}

void testCompletionRulesAreIndependentOfPurpose() {
  auto step = foundationQuestions()[0].steps[0];
  expect(step.semantics.completion == fm::CompletionRule::AllAccepted, "existing collection remains the default");
  for (const auto purpose : {fm::StepPurpose::AnswerChoice, fm::StepPurpose::OperationChoice,
                             fm::StepPurpose::Calculation, fm::StepPurpose::Verification}) {
    step.semantics.purpose = purpose;
    for (const auto rule : {fm::CompletionRule::AnyAccepted, fm::CompletionRule::AllAccepted}) {
      step.semantics.completion = rule;
      const bool any = rule == fm::CompletionRule::AnyAccepted;
      expect(fm::requiredAnswerCount(step) == (any ? 1U : 2U) &&
                 !fm::answerSetComplete(step, 0) && !fm::answerSetComplete(step, 4) &&
                 fm::answerSetComplete(step, 1) == any && fm::answerSetComplete(step, 2) == any &&
                 fm::answerSetComplete(step, 3) && fm::answerSetComplete(step, 7),
             "completion and its count share the explicit rule and ignore unaccepted bits");
    }
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
  const std::size_t option = fm::firstAcceptedOption(fm::layeredQuestion().steps[step]);
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

void testAnyAcceptedUsesOneDecisionInBothInteractions() {
  auto content = foundationQuestions()[0];
  content.steps[0].prompt = "Choose any expression equal to 24.";
  content.steps[0].semantics.completion = fm::CompletionRule::AnyAccepted;
  for (const auto interaction : {fm::QuestionInteraction::Guided, fm::QuestionInteraction::ArcadeCollect}) {
    for (const std::size_t chosen : {0U, 1U}) {
      expectValidation(fm::validateQuestion(content, interaction), {});
      fm::LayeredQuestionSession session({content}, interaction);
      open(session);
      const auto before = session.visibleWorkingId();
      if (interaction == fm::QuestionInteraction::Guided) {
        expect(session.dispatch(fm::LayeredQuestionCommand::selectOption(chosen)).accepted, "guided alternative selects");
        expect(send(session, fm::LayeredQuestionCommandKind::CheckAnswer).accepted, "guided alternative is checked");
        expect(!send(session, fm::LayeredQuestionCommandKind::CheckAnswer).accepted, "guided resolution rejects a repeated check");
      } else {
        const auto command = fm::LayeredQuestionCommand::submitOption(content.steps[0].options[chosen].id);
        expect(session.dispatch(command).accepted, "arcade alternative is judged immediately");
        expect(!session.dispatch(command).accepted, "arcade resolution rejects a repeated hit");
      }
      const auto& record = session.currentRun().steps[0];
      expect(record.resolvedByPlayer && record.attempts.size() == 1 && record.collectedOptions == (1U << chosen) &&
                 session.visibleWorkingId() == before && !session.currentRun().completed,
             "either accepted alternative resolves once without revealing working or advancing");
      expect(send(session, fm::LayeredQuestionCommandKind::Continue).accepted && session.currentRun().completed &&
                 session.visibleWorking() == "6 * 4 = 24 and 18 + 6 = 24",
             "both accepted choices reach the same prepared continuation");
      expect(!send(session, fm::LayeredQuestionCommandKind::Continue).accepted, "completed working cannot advance twice");
      expect(send(session, fm::LayeredQuestionCommandKind::BackToGrid).accepted, "completed run returns to grid");
      expect(session.visibleWorkingId() == content.steps[0].semantics.after, "grid preserves the completed working");
      open(session);
      expect(send(session, fm::LayeredQuestionCommandKind::RestartQuestion).accepted && session.visibleWorkingId() == before &&
                 session.archivedRuns()[0].steps[0].attempts.size() == 1,
             "restart restores initial working while archiving the accepted decision");
    }
  }
}

void testOperationAndCalculationWorkingBoundary() {
  auto content = foundationQuestions()[3];
  // Selecting the operation establishes the next prompt but does not do its arithmetic.
  content.steps[0].semantics.after = content.steps[0].semantics.before;
  content.steps[1].semantics.before = content.steps[0].semantics.after;
  fm::LayeredQuestionSession session({content}, fm::QuestionInteraction::ArcadeCollect);
  open(session);
  expect(session.dispatch(fm::LayeredQuestionCommand::submitOption({108})).accepted &&
             session.visibleWorking() == "2x + 3 = 11" && session.currentRun().currentStep == 0,
         "operation selection preserves the working until advancement");
  expect(send(session, fm::LayeredQuestionCommandKind::Continue).accepted &&
             session.currentRun().currentStep == 1 && session.visibleWorking() == "2x + 3 = 11",
         "operation and calculation can share the same before/after working");
  expect(session.dispatch(fm::LayeredQuestionCommand::submitOption({108})).accepted &&
             !session.currentRun().steps[1].attempts.back().correct && session.visibleWorking() == "2x + 3 = 11" &&
             !send(session, fm::LayeredQuestionCommandKind::Continue).accepted,
         "wrong calculation records its attempt and cannot advance or reveal working");
  expect(session.dispatch(fm::LayeredQuestionCommand::submitOption({101})).accepted &&
             session.visibleWorking() == "2x + 3 = 11", "correct calculation waits for Continue before revealing its result");
  expect(send(session, fm::LayeredQuestionCommandKind::Continue).accepted && session.currentRun().currentStep == 2 &&
             session.visibleWorking() == "2x = 8", "Continue publishes the next prompt and prepared working together");
  expect(!send(session, fm::LayeredQuestionCommandKind::Continue).accepted && session.visibleWorking() == "2x = 8",
         "repeated Continue cannot skip an unresolved calculation");
  expect(session.dispatch(fm::LayeredQuestionCommand::submitOption({115})).accepted && session.visibleWorking() == "2x = 8",
         "final answer also waits for advancement");
  expect(send(session, fm::LayeredQuestionCommandKind::Continue).accepted && session.visibleWorking() == "x = 4" &&
             session.currentRun().completed && session.currentRun().steps[1].attempts.size() == 2,
         "final Continue reveals the final snapshot and preserves wrong/right attempt order");
}

void testGuidedRevealKeepsWorkingUntilContinue() {
  fm::LayeredQuestionSession session({foundationQuestions()[1]}, fm::QuestionInteraction::Guided);
  open(session);
  expect(session.dispatch(fm::LayeredQuestionCommand::selectOption(1)).accepted, "wrong guided answer selects");
  expect(send(session, fm::LayeredQuestionCommandKind::CheckAnswer).accepted, "wrong guided answer checks");
  expect(send(session, fm::LayeredQuestionCommandKind::ShowAnswer).accepted && session.visibleWorking() == "7 + 5" &&
             !session.currentRun().steps[0].resolvedByPlayer, "answer reveal records assistance without revealing the next working");
  expect(send(session, fm::LayeredQuestionCommandKind::Continue).accepted && session.visibleWorking() == "7 + 5 = 12",
         "assisted Continue retains the established advancement behaviour");
  const auto summary = fm::summarizeLayeredQuestionRun(session.currentRun());
  expect(summary.completed && summary.assisted && summary.shownAnswers == 1 && summary.correctOnFirstTry == 0 &&
             session.currentRun().steps[0].attempts.size() == 1 && !session.currentRun().steps[0].resolvedByPlayer,
         "showing and advancing never turns assistance into a player solution");
}

void testSubstitutionIntegralFixture() {
  const auto& content = foundationQuestions()[4];
  expect(content.id == "foundation_substitution_integral_6x" && content.steps.size() == 6 && content.workingStates.size() == 4,
         "the file-loaded substitution fixture contains the six authored decisions and four working states");
  constexpr std::array<std::size_t, 6> correct{1, 0, 2, 1, 3, 0};
  constexpr std::array<std::string_view, 6> answers{
      "u = x^2 + 1", "2x", "3", "u^3 + C", "(x^2 + 1)^3 + C", "6x(x^2 + 1)^2"};
  constexpr std::array<std::string_view, 7> working{
      "Integral of 6x(x^2 + 1)^2 dx", "Integral of 6x(x^2 + 1)^2 dx", "Integral of 6x(x^2 + 1)^2 dx",
      "3 * integral u^2 du", "u^3 + C", "(x^2 + 1)^3 + C", "(x^2 + 1)^3 + C"};
  constexpr std::array purposes{
      fm::StepPurpose::OperationChoice, fm::StepPurpose::Calculation, fm::StepPurpose::Calculation,
      fm::StepPurpose::Calculation, fm::StepPurpose::Calculation, fm::StepPurpose::Verification};
  for (const auto interaction : {fm::QuestionInteraction::Guided, fm::QuestionInteraction::ArcadeCollect}) {
    expectValidation(fm::validateQuestion(content, interaction), {});
    fm::LayeredQuestionSession session({content}, interaction);
    open(session);
    for (std::size_t i = 0; i < correct.size(); ++i) {
      const auto& step = content.steps[i];
      expect(step.options[correct[i]].label == answers[i] && step.acceptedOptions == (1U << correct[i]) &&
                 step.semantics.purpose == purposes[i] && fm::requiredAnswerCount(step) == 1,
             "substitution answer, purpose, and completion requirement match the authored route");
      expect(session.currentRun().currentStep == i && session.visibleWorking() == working[i],
             "each prompt opens with only its established working");
      const auto answer = [&](std::size_t index) {
        if (interaction == fm::QuestionInteraction::Guided) {
          expect(session.dispatch(fm::LayeredQuestionCommand::selectOption(index)).accepted, "fixture guided choice selects");
          return send(session, fm::LayeredQuestionCommandKind::CheckAnswer);
        }
        return session.dispatch(fm::LayeredQuestionCommand::submitOption(step.options[index].id));
      };
      const bool retry = i == 1 || i == 2;
      if (retry) {
        expect(answer((correct[i] + 1) % step.options.size()).accepted &&
                   !session.currentRun().steps[i].attempts.back().correct && session.visibleWorking() == working[i] &&
                   !send(session, fm::LayeredQuestionCommandKind::Continue).accepted,
               "wrong derivative or multiplier cannot reveal the transformed integral");
        if (interaction == fm::QuestionInteraction::Guided)
          expect(send(session, fm::LayeredQuestionCommandKind::TryAgain).accepted, "guided calculation retry remains explicit");
      }
      expect(answer(correct[i]).accepted && session.currentRun().steps[i].resolvedByPlayer &&
                 session.visibleWorking() == working[i] && !session.currentRun().completed,
             "a correct fixture answer resolves its decision and waits for Continue");
      expect(send(session, fm::LayeredQuestionCommandKind::Continue).accepted && session.visibleWorking() == working[i + 1],
             "Continue reveals the authored working, including unchanged substitution and derivative steps");
      expect(!send(session, fm::LayeredQuestionCommandKind::Continue).accepted,
             "a repeated Continue cannot skip the next decision or repeat completion");
    }
    const auto summary = fm::summarizeLayeredQuestionRun(session.currentRun());
    expect(summary.completed && !summary.assisted && summary.correctOnFirstTry == 4 && summary.correctedAfterRetry == 2 &&
               summary.incorrectCheckedAttempts == 2 && session.visibleWorking() == "(x^2 + 1)^3 + C",
           "verification retains the final antiderivative and both wrong/right calculation records");
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
  constexpr std::array<std::string_view, 7U> beforeWorking{{
      "3a + 5 = 20", "3a + 5 = 20", "3a + 5 = 20", "3a + 5 = 20",
      "3a + 5 - 5 = 20 - 5", "3a = 15", "3a / 3 = 15 / 3"}};
  constexpr std::array<std::string_view, 7U> explanations{{
      "It is an equation because it has an equals sign. It is linear in one unknown because a appears only to the first power.",
      "a is the unknown. The numbers 3, 5, and 20 are known values.",
      "3 is the coefficient of a. The term 3a means 3 times a.",
      "Subtracting 5 from both sides preserves equality and removes the +5 from the left side.",
      "3a + 5 - 5 = 20 - 5 simplifies to 3a = 15.",
      "Dividing both sides by 3 leaves a by itself and preserves equality.",
      "3a = 15 gives a = 5. Substitution checks it: 3(5) + 5 = 20."}};
  for (std::size_t step = 0; step < q.steps.size(); ++step) {
    const auto working = std::find_if(q.workingStates.begin(), q.workingStates.end(),
        [&](const auto& state) { return state.id == q.steps[step].semantics.before; });
    expect(q.steps[step].prompt == prompts[step] &&
               q.steps[step].acceptedOptions == (1U << correct[step]) &&
               q.steps[step].wrongHint == neutralRecovery &&
               working != q.workingStates.end() && working->display == beforeWorking[step] &&
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

void testExplicitNewSetArchivesUnfinishedWork() {
  fm::LayeredQuestionSession session;open(session);
  (void)session.dispatch(fm::LayeredQuestionCommand::selectOption(0));
  (void)send(session,fm::LayeredQuestionCommandKind::CheckAnswer);
  const auto before=session.currentRun();
  fm::LayeredQuestionCommand restart{fm::LayeredQuestionCommandKind::RestartQuestion};
  restart.archiveUnfinished=true;restart.questionIndex=999;
  expect(!session.dispatch(restart).accepted && session.archivedRuns().empty(),
      "an invalid new-set destination cannot archive or replace current work");
  restart.questionIndex.reset();
  expect(session.dispatch(restart).accepted && session.currentRun().currentStep==0 &&
      session.currentRun().runNumber==2 && session.currentRun().priorExposure,
      "an explicit new set starts a fresh attempt with prior exposure recorded");
  const auto& archived=session.archivedRuns();
  expect(archived.size()==1 && !archived[0].completed && archived[0].phase==before.phase &&
      archived[0].steps[0].attempts.size()==before.steps[0].attempts.size() &&
      !archived[0].steps[0].attempts[0].correct && session.currentRun().steps[0].attempts.empty(),
      "unfinished evidence is retained without being marked complete or becoming a new attempt");
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
void testImmediateCollectionAndIdentity() {
  fm::LayeredQuestionSession session(foundationQuestions(),fm::QuestionInteraction::ArcadeCollect);
  open(session);
  expect(!session.dispatch(fm::LayeredQuestionCommand::selectOption(0)).accepted,"arcade does not mix guided selection policy");
  expect(!session.dispatch(fm::LayeredQuestionCommand::submitOption({1})).accepted,"stable option ID is not a local index");
  expect(session.dispatch(fm::LayeredQuestionCommand::submitOption({115})).accepted,"arcade judges a wrong hit immediately");
  expect(!session.currentRun().steps[0].awaitingRecoveryChoice,"wrong arcade hit leaves play open");
  expect(session.dispatch(fm::LayeredQuestionCommand::submitOption({101})).accepted,"first accepted option collects");
  expect(!session.currentRun().steps[0].resolvedByPlayer,"partial collection cannot resolve the set");
  expect(session.visibleWorking()=="Equal to 24", "partial collection preserves its before working");
  expect(!send(session,fm::LayeredQuestionCommandKind::Continue).accepted,"incomplete collection cannot advance");
  expect(!session.dispatch(fm::LayeredQuestionCommand::submitOption({101})).accepted,"collection rejects a repeated option");
  expect(session.dispatch(fm::LayeredQuestionCommand::submitOption({108})).accepted,"last accepted option resolves the set");
  expect(session.visibleWorking()=="Equal to 24", "full collection still waits for Continue to reveal working");
  const auto& evidence=session.currentRun();
  expect(evidence.questionId=="foundation_equality_24" && evidence.contentVersion==1 && evidence.steps[0].id.value==10 &&
    evidence.steps[0].attempts.size()==3 && evidence.steps[0].attempts[0].option.value==115 &&
    evidence.steps[0].collectedOptions==3,"attempt evidence retains question version step and option identity");
  expect(send(session,fm::LayeredQuestionCommandKind::Continue).accepted,"collected question completes");
  expect(session.visibleWorking()=="6 * 4 = 24 and 18 + 6 = 24", "AllAccepted final working appears only after advancement");
  fm::LayeredQuestionCommand next{fm::LayeredQuestionCommandKind::RestartQuestion};next.questionIndex=1;
  expect(session.dispatch(next).accepted && !session.currentRun().priorExposure,"unseen relay question has distinct exposure");
  expect(session.archivedRuns().size()==1 && session.archivedRuns()[0].steps[0].attempts.size()==3,"changing content preserves the completed evidence");
  fm::LayeredQuestionSession guided;open(guided);
  expect(!guided.dispatch(fm::LayeredQuestionCommand::submitOption({2})).accepted && guided.currentRun().steps[0].attempts.empty(),
    "immediate submission cannot bypass Guided check and recovery");
}
void testEightChoiceCapacityAndValidation() {
  auto content=foundationQuestions()[0];
  for(unsigned i=0;i<4;++i)content.steps[0].options.push_back({"fixture distractor",{200+i}});
  content.steps[0].acceptedOptions=129;
  fm::LayeredQuestionSession session({content},fm::QuestionInteraction::ArcadeCollect);open(session);
  expect(session.dispatch(fm::LayeredQuestionCommand::submitOption({101})).accepted &&
    session.dispatch(fm::LayeredQuestionCommand::submitOption({203})).accepted && session.currentRun().steps[0].resolvedByPlayer,
    "accepted-set masks support the first and eighth stable options");
  content.steps[0].options.push_back({"ninth choice",{300}});
  bool rejected=false;
  try {fm::LayeredQuestionSession invalid({content},fm::QuestionInteraction::ArcadeCollect);}
  catch(const std::invalid_argument&) {rejected=true;}
  expect(rejected,"over-capacity content cannot open a partial challenge");
  content=foundationQuestions()[0];content.steps[0].options[1].id=content.steps[0].options[0].id;
  rejected=false;
  try {fm::LayeredQuestionSession invalid({content},fm::QuestionInteraction::ArcadeCollect);}
  catch(const std::invalid_argument&) {rejected=true;}
  expect(rejected,"duplicate stable option IDs are rejected before a run begins");
}

void testReviewProjection() {
  using Outcome=fm::QuestionReviewOutcome;
  auto content=foundationQuestions()[0];
  auto future=content.steps[0];future.id={999};future.layerName="FUTURE NAME";future.prompt="FUTURE PROMPT";
  future.explanation="FUTURE EXPLANATION";
  future.semantics.before=content.steps[0].semantics.after;
  content.steps.push_back(future);
  auto otherVersion=content;otherVersion.version=2;
  otherVersion.steps[0].options[0].label="Version two answer";
  otherVersion.steps[0].explanation="Version two explanation";
  fm::LayeredQuestionSession session({content,otherVersion},fm::QuestionInteraction::ArcadeCollect);open(session);
  auto view=*session.review();
  expect(view.steps.size()==1 && view.steps[0].attempts.empty() && view.steps[0].outcome==Outcome::InProgress,
    "new review includes the current unattempted step without future names or prompts");
  expect(!session.review(1) && !session.review(999),"review rejects nonexistent archived runs");
  expect(view.steps[0].explanation.empty(),"unattempted step does not reveal its prepared explanation");
  const auto submit=[&](unsigned id){expect(session.dispatch(fm::LayeredQuestionCommand::submitOption({id})).accepted,"review fixture submits real answers");};
  submit(115);
  expect(session.review()->steps[0].explanation.empty(),"a wrong answer does not unlock the explanation");
  submit(115);submit(101);
  view=*session.review();
  expect(view.steps[0].outcome==Outcome::InProgress && view.steps[0].collected==1 && view.steps[0].required==2,
    "partial collection stays in progress after a correct click");
  expect(view.steps[0].explanation.empty(),"a partially collected set cannot reveal its remaining answers through explanation");
  expect(view.wrongAttempts==2 && view.stepsNeedingRetry==1,"wrong clicks and affected steps are counted separately");
  const auto& attempts=view.steps[0].attempts;
  expect(attempts.size()==3 && attempts[0].label==content.steps[0].options[2].label && !attempts[0].correct &&
    attempts[1].label==attempts[0].label && attempts[2].label==content.steps[0].options[0].label && attempts[2].correct,
    "review retains actual answer text, repeated wrong attempts, order and recorded judgments");
  expect(std::none_of(attempts.begin(),attempts.end(),[&](const auto& a){return a.label==content.steps[0].options[1].label;}),
    "uncollected correct answer text is absent from the projection");
  expect(view.steps[0].working==session.visibleWorking(),"review uses the working shown before the current decision");
  submit(108);view=*session.review();
  expect(view.steps.size()==1 && view.steps[0].outcome==Outcome::CorrectAfterRetry && !view.completed,
    "resolved step awaiting Continue has its result but does not expose the next step");
  expect(view.steps[0].explanation==content.steps[0].explanation,"resolved step exposes the exact prepared explanation before Continue");
  expect(send(session,fm::LayeredQuestionCommandKind::Continue).accepted,"review fixture reaches second step");
  expect(session.review()->steps.size()==2 && session.review()->steps[1].attempts.empty(),"next step becomes visible only when reached");
  expect(session.review()->steps[0].explanation==content.steps[0].explanation && session.review()->steps[1].explanation.empty(),
    "an earlier explanation stays available while the new unfinished step remains hidden");
  submit(101);submit(108);expect(send(session,fm::LayeredQuestionCommandKind::Continue).accepted,"fixture completes");
  view=*session.review();
  expect(view.completed && view.steps[1].outcome==Outcome::CorrectFirstTry,"multi-answer completion without mistakes is first try");
  expect(view.steps[1].explanation==future.explanation,"each completed step uses its own explanation");
  fm::LayeredQuestionCommand next{fm::LayeredQuestionCommandKind::RestartQuestion};next.questionIndex=1;
  expect(session.dispatch(next).accepted,"fixture switches to another content version");
  submit(101);
  expect(session.review()->version==2 && session.review()->steps[0].attempts[0].label=="Version two answer" &&
    session.review(1)->version==1 && session.review(1)->steps[0].attempts[2].label==content.steps[0].options[0].label,
    "archived review resolves the frozen question identity and version, not current content");
  expect(session.review(1)->steps[0].working==view.steps[0].working && session.review()->steps.size()==1 &&
    session.currentRun().steps[0].attempts.size()==1 && session.archivedRuns()[0].steps[0].attempts.size()==4,
    "repeated review reads preserve working, evidence and progression");
  expect(session.review()->steps[0].explanation.empty() && session.review(1)->steps[0].explanation==content.steps[0].explanation,
    "an archived explanation does not unlock the same step in a new run");
  submit(108);
  expect(session.review()->steps[0].explanation==otherVersion.steps[0].explanation &&
    session.review(1)->steps[0].explanation==content.steps[0].explanation,"current and archived explanations use their own frozen versions");

  auto any=foundationQuestions()[0];any.steps[0].semantics.completion=fm::CompletionRule::AnyAccepted;
  fm::LayeredQuestionSession single({any},fm::QuestionInteraction::ArcadeCollect);open(single);
  expect(single.dispatch(fm::LayeredQuestionCommand::submitOption({108})).accepted,"single-decision fixture submits");
  expect(single.review()->steps[0].required==1 && single.review()->steps[0].outcome==Outcome::CorrectFirstTry,
    "AnyAccepted review respects one required decision with multiple accepted alternatives");
  expect(single.review()->steps[0].explanation==any.steps[0].explanation,"AnyAccepted unlocks explanation after one accepted alternative");
  any.steps[0].explanation.clear();
  fm::LayeredQuestionSession noExplanation({any},fm::QuestionInteraction::ArcadeCollect);open(noExplanation);
  expect(noExplanation.dispatch(fm::LayeredQuestionCommand::submitOption({108})).accepted &&
    noExplanation.review()->steps[0].explanation.empty(),"empty authored explanations remain valid after resolution");

  fm::LayeredQuestionSession guided;open(guided);
  expect(guided.dispatch(fm::LayeredQuestionCommand::selectOption(0)).accepted,"guided review fixture selects");
  expect(send(guided,fm::LayeredQuestionCommandKind::CheckAnswer).accepted && guided.review()->steps[0].explanation.empty(),
    "Guided recovery does not reveal the explanation before the player chooses help");
  expect(send(guided,fm::LayeredQuestionCommandKind::ShowAnswer).accepted,"guided fixture reveals after an incorrect attempt");
  expect(guided.review()->steps[0].outcome==Outcome::AnswerShown && guided.review()->steps[0].attempts.size()==1 &&
    !guided.review()->steps[0].attempts[0].correct,"Guided assistance is not misreported as a player answer");
  expect(guided.review()->steps[0].explanation==guided.content().steps[0].explanation,
    "an already shown Guided answer includes its explanation without changing its assistance outcome");
}

}  // namespace

int main() {
  testReviewProjection();
  testValidStructuralContent();
  testMissingRequiredContent();
  testDuplicateIdentitiesAndFailureOrder();
  testAcceptedMasksAndInteractionValidation();
  testStructuralCapacityBoundaries();
  testInitialQuestionRemainsConstructorSpecific();
  testWorkingChainValidation();
  testCompletionRulesAreIndependentOfPurpose();
  testAnyAcceptedUsesOneDecisionInBothInteractions();
  testOperationAndCalculationWorkingBoundary();
  testGuidedRevealKeepsWorkingUntilContinue();
  testSubstitutionIntegralFixture();
  testExactContent();
  testIndependentRunAndIdempotence();
  testExplicitNewSetArchivesUnfinishedWork();
  testRetryPreservesAttemptsAndResume();
  testShowAnswerRecordsAssistance();
  testImmediateCollectionAndIdentity();testEightChoiceCapacityAndValidation();
  if (failures != 0) {
    std::cerr << failures << " first_move_layered_question_tests failure(s)\n";
    return 1;
  }
  std::cout << "first_move_layered_question_tests passed\n";
  return 0;
}

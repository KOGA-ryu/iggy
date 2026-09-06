#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace iggy3d::first_move {

inline constexpr std::size_t kLayeredQuestionStepCount = 7U;
inline constexpr std::size_t kLayeredQuestionOptionCount = 4U;
inline constexpr std::string_view kLayeredQuestionId =
    "linear_one_unknown_3a_plus_5_eq_20_v1";
inline constexpr std::uint32_t kLayeredQuestionVersion = 1U;

struct LayeredQuestionOptionContent {
  std::string_view label;
};

struct LayeredQuestionStepContent {
  std::string_view layerName;
  std::string_view prompt;
  std::array<LayeredQuestionOptionContent, kLayeredQuestionOptionCount> options{};
  std::size_t correctOption = 0U;
  std::string_view wrongHint;
  std::string_view explanation;
  std::string_view workingLine;
};

struct LayeredQuestionContent {
  std::string_view id;
  std::uint32_t version = 0U;
  std::string_view equation;
  std::string_view skill;
  std::string_view description;
  std::array<LayeredQuestionStepContent, kLayeredQuestionStepCount> steps{};
};

[[nodiscard]] const LayeredQuestionContent& layeredQuestion() noexcept;

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
};

struct LayeredQuestionStepRecord {
  std::vector<LayeredQuestionAttemptRecord> attempts;
  std::optional<std::size_t> firstCheckedOption;
  std::optional<bool> firstCorrect;
  std::optional<std::size_t> selectedOption;
  bool resolvedByPlayer = false;
  bool answerShown = false;
  bool awaitingRecoveryChoice = false;
  std::size_t incorrectCheckedAttempts = 0U;
};

[[nodiscard]] bool layeredQuestionStepResolved(
    const LayeredQuestionStepRecord& step) noexcept;

struct LayeredQuestionRunRecord {
  std::uint32_t runNumber = 1U;
  bool priorExposure = false;
  std::size_t currentStep = 0U;
  LayeredQuestionPhase phase = LayeredQuestionPhase::Grid;
  bool completed = false;
  std::array<LayeredQuestionStepRecord, kLayeredQuestionStepCount> steps{};
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

enum class LayeredQuestionCommandKind : std::uint8_t {
  OpenQuestion,
  SelectOption,
  CheckAnswer,
  TryAgain,
  ShowAnswer,
  Continue,
  BackToGrid,
  RestartQuestion,
};

struct LayeredQuestionCommand {
  LayeredQuestionCommandKind kind = LayeredQuestionCommandKind::OpenQuestion;
  std::size_t optionIndex = 0U;

  [[nodiscard]] static constexpr LayeredQuestionCommand selectOption(
      std::size_t index) noexcept {
    return {LayeredQuestionCommandKind::SelectOption, index};
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

  [[nodiscard]] LayeredQuestionDispatchResult dispatch(
      const LayeredQuestionCommand& command);

  [[nodiscard]] const LayeredQuestionRunRecord& currentRun() const noexcept;
  [[nodiscard]] const std::vector<LayeredQuestionRunRecord>& archivedRuns()
      const noexcept;

private:
  void beginRun(std::uint32_t runNumber,
                bool priorExposure,
                LayeredQuestionPhase phase);

  LayeredQuestionRunRecord current_{};
  std::vector<LayeredQuestionRunRecord> archived_;
};

}  // namespace iggy3d::first_move

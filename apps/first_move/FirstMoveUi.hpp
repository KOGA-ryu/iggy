#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "runtime/first_move/HuntSession.hpp"
#include "runtime/first_move/LayeredQuestionSession.hpp"

union SDL_Event;

namespace iggy3d::first_move {

inline constexpr std::size_t kFirstMovePendingInputCapacity = 32U;

enum class FirstMoveMode : std::uint8_t { GuidedQuestion, QuickHunt };
[[nodiscard]] std::string_view firstMoveModeName(FirstMoveMode mode) noexcept;

enum class FirstMoveActionKind : std::uint8_t {
  SwitchToGuided, SwitchToHunt,
  HuntSelectCell, HuntMoveLeft, HuntMoveRight, HuntMoveUp, HuntMoveDown,
  HuntToggleMark, HuntCommitRow, HuntClearReady, HuntOpenReview,
  HuntCloseReview, HuntReleaseReviewed, HuntRestart,
  ReviewPrevious, ReviewNext, ReviewFirst, ReviewLast,
  GuidedOpen, GuidedSelectOption, GuidedMovePreviousOption,
  GuidedMoveNextOption, GuidedCheck, GuidedTryAgain, GuidedShowAnswer,
  GuidedContinue, GuidedBackToGrid, GuidedRestart,
};

struct FirstMoveAction {
  FirstMoveActionKind kind = FirstMoveActionKind::SwitchToGuided;
  std::size_t firstIndex = 0U;
  std::size_t secondIndex = 0U;

  [[nodiscard]] static constexpr FirstMoveAction huntSelectCell(
      std::size_t row, std::size_t column) noexcept {
    return {FirstMoveActionKind::HuntSelectCell, row, column};
  }
  [[nodiscard]] static constexpr FirstMoveAction guidedSelectOption(
      std::size_t option) noexcept {
    return {FirstMoveActionKind::GuidedSelectOption, option, 0U};
  }
};

struct FirstMoveDispatchResult {
  bool accepted = false;
  bool changed = false;
  std::string_view reason = "action_rejected";
};

struct FirstMoveUiState {
  FirstMoveMode mode = FirstMoveMode::GuidedQuestion;
  float textScale = 1.0F;
  std::array<FirstMoveAction, kFirstMovePendingInputCapacity> pendingActions{};
  std::size_t pendingActionCount = 0U;
  std::size_t droppedCommandCount = 0U;
  bool scaleControlActive = false;
  bool lastActionAccepted = true;
  std::string_view lastActionReason = "ready";
  bool presentedFocusValid = false;
  std::size_t presentedFocusRow = 0U;
  std::size_t presentedFocusColumn = 0U;
  bool reviewFocusValid = false;
  std::size_t reviewFocusRow = 0U;
  std::size_t reviewExplanationIndex = 0U;
  bool reviewRevealRequested = false;
  float presentedReviewWidth = 0.0F;
  float presentedReviewHeight = 0.0F;
  float presentedReviewScale = 0.0F;
  std::size_t guidedOptionFocus = 0U;
  bool guidedRevealRequested = true;
  float presentedGuidedWidth = 0.0F;
  float presentedGuidedHeight = 0.0F;
  float presentedGuidedScale = 0.0F;
};

[[nodiscard]] FirstMoveDispatchResult dispatchFirstMoveAction(
    FirstMoveUiState& ui, HuntSession& hunt, LayeredQuestionSession& guided,
    const FirstMoveAction& action);

void queueFirstMoveInput(FirstMoveUiState& ui,
                         const HuntSession& hunt,
                         const LayeredQuestionSession& guided,
                         const SDL_Event& event) noexcept;

[[nodiscard]] std::size_t drainFirstMoveInput(
    FirstMoveUiState& ui, HuntSession& hunt, LayeredQuestionSession& guided,
    bool suppressShortcuts = false);

void renderFirstMoveUiFrame(FirstMoveUiState& ui,
                            HuntSession& hunt,
                            LayeredQuestionSession& guided);

}  // namespace iggy3d::first_move

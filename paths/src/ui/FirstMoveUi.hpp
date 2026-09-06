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
enum class PathsScreen : std::uint8_t { Title, Playing, Stats };
[[nodiscard]] std::string_view pathsScreenName(PathsScreen screen) noexcept;

enum class FirstMoveActionKind : std::uint8_t {
  BackToTitle, OpenStats, CloseStats, Quit,
  TitlePrevious, TitleNext, TitleOpen,
  SwitchToGuided, SwitchToHunt,
  HuntSelectCell, HuntMoveLeft, HuntMoveRight, HuntMoveUp, HuntMoveDown,
  HuntToggleMark, HuntCommitRow, HuntClearReady, HuntOpenReview,
  HuntCloseReview, HuntReleaseReviewed, HuntRestart,
  ReviewPrevious, ReviewNext, ReviewFirst, ReviewLast,
  GuidedOpen, GuidedSelectOption, GuidedMovePreviousOption,
  GuidedMoveNextOption, GuidedCheck, GuidedTryAgain, GuidedShowAnswer,
  GuidedContinue, GuidedBackToGrid, GuidedRestart,
  GuidedPageUp, GuidedPageDown, GuidedReadStart, GuidedReadEnd,
};

struct FirstMoveModeDescriptor {
  FirstMoveMode mode;
  std::string_view id, title, description;
  FirstMoveActionKind openAction;
};
[[nodiscard]] const std::array<FirstMoveModeDescriptor, 2>& firstMoveModes() noexcept;
[[nodiscard]] const FirstMoveModeDescriptor* findFirstMoveMode(std::string_view id) noexcept;

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

struct FirstMoveInputContext {
  PathsScreen screen{};
  FirstMoveMode mode{};
  std::uint64_t epoch = 0;
  std::uint32_t run = 0;
  LayeredQuestionPhase phase{};
  std::size_t step = 0, review = 0;
  bool resolved = false, recovering = false;
  bool operator==(const FirstMoveInputContext&) const = default;
};

enum class GuidedReveal { Prompt, Option, Feedback };
enum class GuidedReading { None, PageUp, PageDown, Start, End };

struct FirstMoveUiState {
  PathsScreen screen = PathsScreen::Title;
  FirstMoveMode mode = FirstMoveMode::GuidedQuestion;
  std::size_t titleFocus = 0;
  bool quitRequested = false;
  std::uint64_t contextEpoch = 0;
  float textScale = 1.0F;
  std::array<FirstMoveAction, kFirstMovePendingInputCapacity> pendingActions{};
  std::array<FirstMoveInputContext, kFirstMovePendingInputCapacity> pendingContexts{};
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
  GuidedReveal guidedReveal = GuidedReveal::Prompt;
  GuidedReading guidedReading = GuidedReading::None;
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

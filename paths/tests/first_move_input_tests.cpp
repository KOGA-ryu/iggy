#include "FirstMoveUi.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string_view>

#include <SDL3/SDL.h>

#include "imgui.h"
#include "imgui_internal.h"

namespace fm = iggy3d::first_move;

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

SDL_Event keyDown(SDL_Keycode key, bool repeat = false) {
  SDL_Event event{};
  event.type = SDL_EVENT_KEY_DOWN;
  event.key.type = SDL_EVENT_KEY_DOWN;
  event.key.key = key;
  event.key.down = true;
  event.key.repeat = repeat;
  return event;
}

SDL_Event focusLost() {
  SDL_Event event{};
  event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
  event.window.type = SDL_EVENT_WINDOW_FOCUS_LOST;
  return event;
}

void toggleDirect(fm::HuntSession& session,
                  std::size_t row,
                  std::size_t column) {
  static_cast<void>(session.dispatch(fm::Command::selectCell(row, column)));
  static_cast<void>(session.dispatch({fm::CommandKind::ToggleMark}));
}

void testRepeatedEnterIsRejectedAtAdapter() {
  fm::HuntSession session;
  fm::LayeredQuestionSession guided;
  fm::FirstMoveUiState ui;
  ui.screen = fm::PathsScreen::Playing;
  ui.mode = fm::FirstMoveMode::QuickHunt;
  toggleDirect(session, 0U, 0U);
  toggleDirect(session, 0U, 2U);
  const SDL_Event enter = keyDown(SDLK_RETURN);
  const SDL_Event repeatedEnter = keyDown(SDLK_RETURN, true);
  fm::queueFirstMoveInput(ui, session, guided, enter);
  fm::queueFirstMoveInput(ui, session, guided, repeatedEnter);
  expect(fm::drainFirstMoveInput(ui, session, guided) == 1U,
         "repeated Enter produces only one semantic command");
  expect(session.currentRun().rows[0U].state == fm::RowState::Ready &&
             !session.currentRun().rows[1U].initialSelectionMask.has_value(),
         "one Enter commits one row without submitting another");
}

void testHeldSpaceDoesNotRetoggle() {
  fm::HuntSession session;
  fm::LayeredQuestionSession guided;
  fm::FirstMoveUiState ui;
  ui.screen = fm::PathsScreen::Playing;
  ui.mode = fm::FirstMoveMode::QuickHunt;
  fm::queueFirstMoveInput(ui, session, guided, keyDown(SDLK_SPACE));
  fm::queueFirstMoveInput(ui, session, guided, keyDown(SDLK_SPACE, true));
  fm::queueFirstMoveInput(ui, session, guided, keyDown(SDLK_SPACE, true));
  expect(fm::drainFirstMoveInput(ui, session, guided) == 1U,
         "held Space produces one semantic toggle");
  expect(session.currentRun().rows[0U].selectedMask == 1U,
         "held Space cannot toggle the cell back off");
}

void testFocusLossDropsPendingCommands() {
  fm::HuntSession session;
  fm::LayeredQuestionSession guided;
  fm::FirstMoveUiState ui;
  ui.screen = fm::PathsScreen::Playing;
  ui.mode = fm::FirstMoveMode::QuickHunt;
  fm::queueFirstMoveInput(ui, session, guided, keyDown(SDLK_SPACE));
  fm::queueFirstMoveInput(ui, session, guided, focusLost());
  expect(fm::drainFirstMoveInput(ui, session, guided) == 0U,
         "focus loss clears queued shortcuts before drain");
  expect(session.currentRun().rows[0U].selectedMask == 0U,
         "focus loss prevents a stale mark after refocus");
}

void testKeyboardAndPointerSemanticsMatch() {
  fm::HuntSession keyboardSession;
  fm::LayeredQuestionSession keyboardGuided;
  fm::FirstMoveUiState keyboardUi;
  keyboardUi.screen = fm::PathsScreen::Playing;
  keyboardUi.mode = fm::FirstMoveMode::QuickHunt;
  static_cast<void>(keyboardSession.dispatch(fm::Command::selectCell(0U, 0U)));
  fm::queueFirstMoveInput(keyboardUi, keyboardSession, keyboardGuided,
                          keyDown(SDLK_SPACE));
  fm::queueFirstMoveInput(keyboardUi, keyboardSession, keyboardGuided,
                          keyDown(SDLK_RETURN));
  expect(fm::drainFirstMoveInput(keyboardUi, keyboardSession,
                                 keyboardGuided) == 2U,
         "keyboard adapter drains toggle then commit in order");

  fm::HuntSession pointerSession;
  static_cast<void>(pointerSession.dispatch(fm::Command::selectCell(0U, 0U)));
  static_cast<void>(pointerSession.dispatch({fm::CommandKind::ToggleMark}));
  static_cast<void>(pointerSession.dispatch({fm::CommandKind::CommitRow}));

  const fm::HuntRowRecord& keyboard = keyboardSession.currentRun().rows[0U];
  const fm::HuntRowRecord& pointer = pointerSession.currentRun().rows[0U];
  expect(keyboard.selectedMask == pointer.selectedMask &&
             keyboard.initialSelectionMask == pointer.initialSelectionMask &&
             keyboard.initialCorrect == pointer.initialCorrect &&
             keyboard.state == pointer.state,
         "keyboard and pointer semantic routes preserve identical first-response evidence");
}

void testShortcutSuppressionDoesNotReplay() {
  fm::HuntSession session;
  fm::LayeredQuestionSession guided;
  fm::FirstMoveUiState ui;
  ui.screen = fm::PathsScreen::Playing;
  ui.mode = fm::FirstMoveMode::QuickHunt;
  fm::queueFirstMoveInput(ui, session, guided, keyDown(SDLK_SPACE));
  expect(fm::drainFirstMoveInput(ui, session, guided, true) == 0U,
         "active UI control suppresses gameplay shortcut");
  expect(fm::drainFirstMoveInput(ui, session, guided, false) == 0U &&
             session.currentRun().rows[0U].selectedMask == 0U,
         "suppressed shortcut is discarded instead of replayed later");
}

struct ReviewWindows {
  ImGuiWindow* viewport = nullptr;
  std::array<ImGuiWindow*, fm::kHuntCellsPerRow> cards{};
  std::size_t cardCount = 0U;
};

ReviewWindows findReviewWindows() {
  ReviewWindows result;
  for (ImGuiWindow* window : GImGui->Windows) {
    if (std::strstr(window->Name, "##hunt_review_") != nullptr &&
        std::strstr(window->Name, "##review_cell_") == nullptr) {
      result.viewport = window;
    }
  }
  if (result.viewport == nullptr) {
    return result;
  }
  for (ImGuiWindow* window : GImGui->Windows) {
    if (window->ParentWindow == result.viewport &&
        std::strstr(window->Name, "##review_cell_") != nullptr &&
        result.cardCount < result.cards.size()) {
      result.cards[result.cardCount++] = window;
    }
  }
  std::sort(result.cards.begin(), result.cards.begin() + result.cardCount,
            [](const ImGuiWindow* left, const ImGuiWindow* right) {
              return left->Pos.y < right->Pos.y;
            });
  return result;
}

bool cardIsFullyVisible(const ReviewWindows& windows, std::size_t index) {
  if (windows.viewport == nullptr || index >= windows.cardCount ||
      windows.cards[index] == nullptr) {
    return false;
  }
  constexpr float tolerance = 2.0F;
  const ImGuiWindow* card = windows.cards[index];
  return card->Pos.y >= windows.viewport->InnerClipRect.Min.y - tolerance &&
         card->Pos.y + card->Size.y <=
             windows.viewport->InnerClipRect.Max.y + tolerance;
}

bool cardsHaveNoNestedOverflow(const ReviewWindows& windows) {
  if (windows.cardCount != windows.cards.size()) {
    return false;
  }
  constexpr float tolerance = 2.0F;
  return std::all_of(
      windows.cards.begin(), windows.cards.end(),
      [](const ImGuiWindow* card) {
        return card != nullptr && card->ScrollMax.y <= tolerance &&
               card->ContentSize.y <=
                   card->InnerRect.GetHeight() + tolerance;
      });
}

void testReviewExplanationsAreKeyboardReachable() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.DisplaySize = {1024.0F, 768.0F};
  io.DeltaTime = 1.0F / 60.0F;
  unsigned char* pixels = nullptr;
  int atlasWidth = 0;
  int atlasHeight = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &atlasWidth, &atlasHeight);
  io.Fonts->SetTexID(static_cast<ImTextureID>(1));

  fm::HuntSession session;
  fm::LayeredQuestionSession guided;
  fm::FirstMoveUiState ui;
  ui.screen = fm::PathsScreen::Playing;
  ui.mode = fm::FirstMoveMode::QuickHunt;
  ui.textScale = 1.5F;
  const auto frame = [&]() {
    ImGui::NewFrame();
    fm::renderFirstMoveUiFrame(ui, session, guided);
    ImGui::Render();
  };
  const auto frames = [&](int count) {
    for (int index = 0; index < count; ++index) {
      frame();
    }
  };
  const auto press = [&](SDL_Keycode key) {
    fm::queueFirstMoveInput(ui, session, guided, keyDown(key));
    frames(2);
  };

  frames(3);
  press(SDLK_RETURN);
  press(SDLK_E);
  expect(session.reviewRowIndex() == std::optional<std::size_t>{0U} &&
             ui.reviewFocusValid && ui.reviewExplanationIndex == 0U,
         "opening review initializes explanation focus at the first card");

  const fm::HuntRowRecord& initialRow = session.currentRun().rows[0U];
  const std::uint8_t initialMask = initialRow.initialSelectionMask.value_or(255U);
  const bool initialCorrect = initialRow.initialCorrect.value_or(true);
  const std::uint32_t initialScore = session.currentRun().score;
  ReviewWindows review = findReviewWindows();
  expect(review.viewport != nullptr &&
             review.cardCount == fm::kHuntCellsPerRow,
         "review renders one scroll viewport and four explanation cards");
  expect(cardIsFullyVisible(review, 0U),
         "the initially selected explanation is fully visible");
  expect(cardsHaveNoNestedOverflow(review),
         "content-sized explanation cards do not create nested scrolling");

  for (std::size_t explanation = 1U;
       explanation < fm::kHuntCellsPerRow; ++explanation) {
    press(SDLK_DOWN);
    review = findReviewWindows();
    expect(ui.reviewExplanationIndex == explanation,
           "Down selects the next explanation");
    expect(cardIsFullyVisible(review, explanation),
           "Down brings the complete selected explanation into view");
    expect(cardsHaveNoNestedOverflow(review),
           "every visited explanation fits its content-sized card");
  }

  press(SDLK_PAGEUP);
  expect(ui.reviewExplanationIndex == 2U,
         "Page Up selects the previous explanation");
  press(SDLK_PAGEDOWN);
  expect(ui.reviewExplanationIndex == 3U,
         "Page Down selects the next explanation");
  press(SDLK_HOME);
  expect(ui.reviewExplanationIndex == 0U,
         "Home selects the first explanation");
  press(SDLK_END);
  expect(ui.reviewExplanationIndex == fm::kHuntCellsPerRow - 1U &&
             cardIsFullyVisible(findReviewWindows(),
                                fm::kHuntCellsPerRow - 1U),
         "End selects and reveals the last explanation");

  review = findReviewWindows();
  ImGui::SetScrollY(review.viewport, 0.0F);
  frames(2);
  expect(!cardIsFullyVisible(findReviewWindows(),
                             fm::kHuntCellsPerRow - 1U),
         "idle frames preserve manual review scrolling");
  press(SDLK_END);
  expect(cardIsFullyVisible(findReviewWindows(),
                            fm::kHuntCellsPerRow - 1U),
         "an endpoint action re-reveals an already-selected explanation");

  io.DisplaySize = {1200.0F, 800.0F};
  ui.textScale = 1.25F;
  frames(3);
  expect(cardIsFullyVisible(findReviewWindows(),
                            fm::kHuntCellsPerRow - 1U),
         "viewport and text-scale changes restore selected-card visibility");
  io.DisplaySize = {1024.0F, 768.0F};
  ui.textScale = 1.5F;
  frames(3);
  expect(cardIsFullyVisible(findReviewWindows(),
                            fm::kHuntCellsPerRow - 1U),
         "returning to the required enlarged layout keeps the card visible");

  fm::queueFirstMoveInput(ui, session, guided, keyDown(SDLK_HOME));
  fm::queueFirstMoveInput(ui, session, guided, focusLost());
  frames(2);
  expect(ui.reviewExplanationIndex == fm::kHuntCellsPerRow - 1U,
         "focus loss clears a queued review-navigation action");
  expect(session.currentRun().rows[0U].initialSelectionMask == initialMask &&
             session.currentRun().rows[0U].initialCorrect == initialCorrect &&
             session.currentRun().score == initialScore,
         "review navigation leaves first-attempt evidence and score unchanged");

  press(SDLK_ESCAPE);
  expect(!session.reviewRowIndex().has_value() && !ui.reviewFocusValid,
         "Escape closes review and clears presentation focus");
  press(SDLK_E);
  expect(session.reviewRowIndex() == std::optional<std::size_t>{0U} &&
             ui.reviewExplanationIndex == 0U &&
             cardIsFullyVisible(findReviewWindows(), 0U),
         "reopening review starts at a visible first explanation");

  fm::queueFirstMoveInput(ui, session, guided, keyDown(SDLK_DOWN));
  fm::queueFirstMoveInput(ui, session, guided, keyDown(SDLK_DOWN, true));
  frames(2);
  expect(ui.reviewExplanationIndex == 1U,
         "held Down advances review focus only once");
  press(SDLK_X);
  const fm::HuntRowRecord& released = session.currentRun().rows[0U];
  expect(released.state == fm::RowState::Released &&
             released.initialSelectionMask == initialMask &&
             released.initialCorrect == initialCorrect &&
             session.currentRun().score == initialScore &&
             !ui.reviewFocusValid,
         "release retains immutable evidence and resets review presentation");

  ImGui::DestroyContext();
}

void testGuidedKeyboardAndPointerShareDispatcher() {
  fm::HuntSession keyboardHunt;
  fm::LayeredQuestionSession keyboardGuided;
  fm::FirstMoveUiState keyboardUi;
  keyboardUi.screen = fm::PathsScreen::Playing;
  fm::queueFirstMoveInput(keyboardUi, keyboardHunt, keyboardGuided,
                          keyDown(SDLK_RETURN));
  const std::size_t opened = fm::drainFirstMoveInput(
      keyboardUi, keyboardHunt, keyboardGuided);
  fm::queueFirstMoveInput(keyboardUi, keyboardHunt, keyboardGuided,
                          keyDown(SDLK_DOWN));
  const std::size_t selected = fm::drainFirstMoveInput(
      keyboardUi, keyboardHunt, keyboardGuided);
  fm::queueFirstMoveInput(keyboardUi, keyboardHunt, keyboardGuided,
                          keyDown(SDLK_RETURN));
  const std::size_t checked = fm::drainFirstMoveInput(
      keyboardUi, keyboardHunt, keyboardGuided);
  expect(opened + selected + checked == 3U,
         "guided keyboard actions drain through app dispatcher");

  fm::HuntSession pointerHunt;
  fm::LayeredQuestionSession pointerGuided;
  fm::FirstMoveUiState pointerUi;
  pointerUi.screen = fm::PathsScreen::Playing;
  static_cast<void>(fm::dispatchFirstMoveAction(
      pointerUi, pointerHunt, pointerGuided,
      {fm::FirstMoveActionKind::GuidedOpen}));
  static_cast<void>(fm::dispatchFirstMoveAction(
      pointerUi, pointerHunt, pointerGuided,
      fm::FirstMoveAction::guidedSelectOption(0U)));
  static_cast<void>(fm::dispatchFirstMoveAction(
      pointerUi, pointerHunt, pointerGuided,
      {fm::FirstMoveActionKind::GuidedCheck}));
  const fm::LayeredQuestionStepRecord& keyboard =
      keyboardGuided.currentRun().steps[0U];
  const fm::LayeredQuestionStepRecord& pointer =
      pointerGuided.currentRun().steps[0U];
  expect(keyboard.selectedOption == pointer.selectedOption &&
             keyboard.firstCheckedOption == pointer.firstCheckedOption &&
             keyboard.firstCorrect == pointer.firstCorrect &&
             keyboard.attempts.size() == pointer.attempts.size() &&
             keyboard.resolvedByPlayer == pointer.resolvedByPlayer,
         "guided keyboard and pointer routes produce identical evidence");
}

void testGuidedRepeatedKeysAndFocusLoss() {
  fm::HuntSession hunt;
  fm::LayeredQuestionSession guided;
  fm::FirstMoveUiState ui;
  ui.screen = fm::PathsScreen::Playing;
  static_cast<void>(fm::dispatchFirstMoveAction(
      ui, hunt, guided, {fm::FirstMoveActionKind::GuidedOpen}));
  static_cast<void>(fm::dispatchFirstMoveAction(
      ui, hunt, guided, fm::FirstMoveAction::guidedSelectOption(0U)));
  fm::queueFirstMoveInput(ui, hunt, guided, keyDown(SDLK_RETURN));
  fm::queueFirstMoveInput(ui, hunt, guided, keyDown(SDLK_RETURN, true));
  expect(fm::drainFirstMoveInput(ui, hunt, guided) == 1U &&
             guided.currentRun().steps[0U].attempts.size() == 1U,
         "held Enter cannot check a guided answer twice");
  fm::queueFirstMoveInput(ui, hunt, guided, keyDown(SDLK_T));
  fm::queueFirstMoveInput(ui, hunt, guided, keyDown(SDLK_T, true));
  expect(fm::drainFirstMoveInput(ui, hunt, guided) == 1U &&
             !guided.currentRun().steps[0U].selectedOption.has_value(),
         "held T cannot repeat guided recovery");
  fm::queueFirstMoveInput(ui, hunt, guided, keyDown(SDLK_DOWN));
  fm::queueFirstMoveInput(ui, hunt, guided, focusLost());
  expect(fm::drainFirstMoveInput(ui, hunt, guided) == 0U &&
             !guided.currentRun().steps[0U].selectedOption.has_value(),
         "focus loss discards queued guided selection");
}

void testModeSwitchingPreservesSessionsAndReviewGate() {
  fm::HuntSession hunt;
  fm::LayeredQuestionSession guided;
  fm::FirstMoveUiState ui;
  ui.screen = fm::PathsScreen::Playing;
  static_cast<void>(fm::dispatchFirstMoveAction(
      ui, hunt, guided, {fm::FirstMoveActionKind::GuidedOpen}));
  static_cast<void>(fm::dispatchFirstMoveAction(
      ui, hunt, guided, fm::FirstMoveAction::guidedSelectOption(1U)));
  expect(fm::dispatchFirstMoveAction(
             ui, hunt, guided,
             {fm::FirstMoveActionKind::SwitchToHunt}).accepted,
         "guided mode switches to Hunt");
  static_cast<void>(fm::dispatchFirstMoveAction(
      ui, hunt, guided, {fm::FirstMoveActionKind::HuntCommitRow}));
  static_cast<void>(fm::dispatchFirstMoveAction(
      ui, hunt, guided, {fm::FirstMoveActionKind::HuntOpenReview}));
  expect(!fm::dispatchFirstMoveAction(
              ui, hunt, guided,
              {fm::FirstMoveActionKind::SwitchToGuided}).accepted &&
             ui.mode == fm::FirstMoveMode::QuickHunt,
         "open Hunt review blocks a mode switch");
  static_cast<void>(fm::dispatchFirstMoveAction(
      ui, hunt, guided, {fm::FirstMoveActionKind::HuntCloseReview}));
  expect(fm::dispatchFirstMoveAction(
             ui, hunt, guided,
             {fm::FirstMoveActionKind::SwitchToGuided}).accepted &&
             guided.currentRun().steps[0U].selectedOption == 1U &&
             hunt.currentRun().rows[0U].initialSelectionMask.has_value(),
         "mode switch preserves Guided and Hunt evidence");
}

void testGuidedEnlargedLayoutUsesOneScroller() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.DisplaySize = {1024.0F, 768.0F};
  io.DeltaTime = 1.0F / 60.0F;
  unsigned char* pixels = nullptr;
  int atlasWidth = 0;
  int atlasHeight = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &atlasWidth, &atlasHeight);
  io.Fonts->SetTexID(static_cast<ImTextureID>(1));
  fm::HuntSession hunt;
  fm::LayeredQuestionSession guided;
  fm::FirstMoveUiState ui;
  ui.screen = fm::PathsScreen::Playing;
  ui.textScale = 1.5F;
  static_cast<void>(fm::dispatchFirstMoveAction(
      ui, hunt, guided, {fm::FirstMoveActionKind::GuidedOpen}));
  static_cast<void>(fm::dispatchFirstMoveAction(
      ui, hunt, guided, fm::FirstMoveAction::guidedSelectOption(0U)));
  static_cast<void>(fm::dispatchFirstMoveAction(
      ui, hunt, guided, {fm::FirstMoveActionKind::GuidedCheck}));
  for (int frame = 0; frame < 3; ++frame) {
    ImGui::NewFrame();
    fm::renderFirstMoveUiFrame(ui, hunt, guided);
    ImGui::Render();
  }
  ImGuiWindow* outer = ImGui::FindWindowByName(
      "First Move###first_move_root/##guided_outer_scroll_7F7329BE");
  if (outer == nullptr) {
    for (ImGuiWindow* window : GImGui->Windows) {
      if (std::strstr(window->Name, "##guided_outer_scroll") != nullptr) {
        outer = window;
      }
    }
  }
  std::size_t nestedGuidedScrollers = 0U;
  for (ImGuiWindow* window : GImGui->Windows) {
    if (window != outer && std::strstr(window->Name, "##guided_") != nullptr &&
        window->ScrollMax.y > 2.0F) {
      ++nestedGuidedScrollers;
    }
  }
  expect(outer != nullptr && nestedGuidedScrollers == 0U,
         "enlarged Guided view has one outer vertical scroller and no nested overflow");
  fm::queueFirstMoveInput(ui, hunt, guided, keyDown(SDLK_S));
  expect(fm::drainFirstMoveInput(ui, hunt, guided) == 1U &&
             guided.currentRun().steps[0U].answerShown,
         "Show Me action remains keyboard reachable in enlarged layout");
  ImGui::DestroyContext();
}

void testTitleStatsAndResumeKeepEvidence() {
  fm::FirstMoveUiState ui;
  fm::HuntSession hunt;
  fm::LayeredQuestionSession guided;
  const auto act = [&](fm::FirstMoveActionKind kind) {
    return fm::dispatchFirstMoveAction(ui, hunt, guided, {kind});
  };
  using A = fm::FirstMoveActionKind;
  expect(ui.screen == fm::PathsScreen::Title, "Paths defaults to its title");
  expect(!act(A::GuidedOpen).accepted && guided.currentRun().phase == fm::LayeredQuestionPhase::Grid,
         "title cannot forward hidden game actions");
  fm::queueFirstMoveInput(ui, hunt, guided, keyDown(SDLK_RETURN));
  static_cast<void>(fm::drainFirstMoveInput(ui, hunt, guided));
  expect(ui.screen == fm::PathsScreen::Playing && ui.mode == fm::FirstMoveMode::GuidedQuestion,
         "title Enter opens Guided without checking a question");
  act(A::GuidedOpen);
  static_cast<void>(fm::dispatchFirstMoveAction(ui, hunt, guided, fm::FirstMoveAction::guidedSelectOption(0)));
  act(A::GuidedCheck);
  act(A::BackToTitle);
  expect(act(A::OpenStats).accepted && ui.screen == fm::PathsScreen::Stats,
         "stats is reachable from title");
  const auto before = fm::summarizeLayeredQuestionRun(guided.currentRun());
  act(A::CloseStats);
  act(A::SwitchToGuided);
  const auto& step = guided.currentRun().steps[0];
  expect(step.awaitingRecoveryChoice && step.attempts.size() == 1 && step.firstCheckedOption == 0 &&
             step.incorrectCheckedAttempts == before.incorrectCheckedAttempts && guided.archivedRuns().empty(),
         "title and stats preserve the pending recovery and first-attempt evidence");
  act(A::BackToTitle);
  act(A::SwitchToHunt);
  act(A::HuntCommitRow);
  act(A::HuntOpenReview);
  const auto firstMask = hunt.currentRun().rows[0].initialSelectionMask;
  act(A::BackToTitle);
  act(A::OpenStats);
  expect(!act(A::SwitchToGuided).accepted && ui.screen == fm::PathsScreen::Stats,
         "stats does not bypass the paused Hunt explanation gate");
  act(A::CloseStats);
  act(A::SwitchToHunt);
  expect(hunt.reviewRowIndex() == 0 && hunt.currentRun().rows[0].initialSelectionMask == firstMask,
         "Hunt resumes its paused explanation without losing its initial response");
  act(A::HuntCloseReview);
  expect(act(A::SwitchToGuided).accepted && guided.currentRun().steps[0].awaitingRecoveryChoice,
         "closing the Hunt explanation allows the original Guided recovery to resume");
}

void testQueuedContextsAndRejectedFocus() {
  using A = fm::FirstMoveActionKind;
  fm::FirstMoveUiState ui;
  fm::HuntSession hunt;
  fm::LayeredQuestionSession guided;
  const auto act = [&](A kind) { return fm::dispatchFirstMoveAction(ui, hunt, guided, {kind}); };
  act(A::SwitchToGuided);
  act(A::GuidedOpen);
  expect(!act(A::GuidedCheck).accepted && guided.currentRun().steps[0].attempts.empty(),
         "Enter without a choice does not invent an answer");
  act(A::GuidedMovePreviousOption);
  expect(guided.currentRun().steps[0].selectedOption == 3, "Up from no selection starts at D");
  act(A::GuidedMoveNextOption);
  expect(guided.currentRun().steps[0].selectedOption == 0, "Down wraps D to A");
  act(A::GuidedMovePreviousOption);
  expect(guided.currentRun().steps[0].selectedOption == 3, "Up wraps A to D");
  static_cast<void>(fm::dispatchFirstMoveAction(ui, hunt, guided, fm::FirstMoveAction::guidedSelectOption(1)));
  fm::queueFirstMoveInput(ui, hunt, guided, keyDown(SDLK_RETURN));
  fm::queueFirstMoveInput(ui, hunt, guided, keyDown(SDLK_RETURN));
  static_cast<void>(fm::drainFirstMoveInput(ui, hunt, guided));
  expect(guided.currentRun().currentStep == 0 && guided.currentRun().steps[0].attempts.size() == 1 &&
             guided.currentRun().steps[0].resolvedByPlayer && ui.lastActionReason == "stale_input_context",
         "two queued Enter keys cannot check and skip the explanation");
  const auto focus = ui.guidedOptionFocus;
  ui.guidedRevealRequested = false;
  expect(!act(A::GuidedRestart).accepted && ui.guidedOptionFocus == focus && !ui.guidedRevealRequested,
         "rejected restart cannot reset presentation focus");
  expect(!fm::dispatchFirstMoveAction(ui, hunt, guided, fm::FirstMoveAction::guidedSelectOption(9)).accepted &&
             ui.guidedOptionFocus == focus && !ui.guidedRevealRequested,
         "rejected option cannot move or reveal presentation focus");
  fm::queueFirstMoveInput(ui, hunt, guided, keyDown(SDLK_H));
  fm::queueFirstMoveInput(ui, hunt, guided, keyDown(SDLK_RETURN));
  static_cast<void>(fm::drainFirstMoveInput(ui, hunt, guided));
  expect(ui.mode == fm::FirstMoveMode::QuickHunt && guided.currentRun().currentStep == 0 &&
             !hunt.currentRun().rows[0].initialSelectionMask,
         "H then stale Enter changes mode without submitting either game");
  act(A::SwitchToGuided);
  act(A::GuidedContinue);
  act(A::GuidedMoveNextOption);
  expect(guided.currentRun().steps[1].selectedOption == 0, "Down from no selection starts at A");
  act(A::GuidedCheck);
  const auto attempts = guided.currentRun().steps[1].attempts.size();
  expect(!act(A::GuidedCheck).accepted && guided.currentRun().steps[1].attempts.size() == attempts,
         "Enter in recovery cannot implicitly choose Try Again or Show Me");
  act(A::GuidedShowAnswer);
  expect(guided.currentRun().steps[1].answerShown && guided.currentRun().steps[1].attempts.size() == attempts &&
             guided.currentRun().steps[1].firstCorrect == false,
         "explicit reveal preserves the wrong first answer without a synthetic correct check");
}

}  // namespace

int main() {
  testTitleStatsAndResumeKeepEvidence();
  testQueuedContextsAndRejectedFocus();
  testRepeatedEnterIsRejectedAtAdapter();
  testHeldSpaceDoesNotRetoggle();
  testFocusLossDropsPendingCommands();
  testKeyboardAndPointerSemanticsMatch();
  testShortcutSuppressionDoesNotReplay();
  testReviewExplanationsAreKeyboardReachable();
  testGuidedKeyboardAndPointerShareDispatcher();
  testGuidedRepeatedKeysAndFocusLoss();
  testModeSwitchingPreservesSessionsAndReviewGate();
  testGuidedEnlargedLayoutUsesOneScroller();
  if (failures != 0) {
    std::cerr << failures << " first_move_input_tests failure(s)\n";
    return 1;
  }
  std::cout << "first_move_input_tests passed\n";
  return 0;
}

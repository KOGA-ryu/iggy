#include "LayeredQuestionUi.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string_view>

#include "FirstMoveUi.hpp"
#include "imgui.h"

namespace iggy3d::first_move {
namespace {

constexpr ImVec4 kInk{0.035F, 0.055F, 0.075F, 1.0F};
constexpr ImVec4 kPanel{0.065F, 0.095F, 0.115F, 1.0F};
constexpr ImVec4 kRaised{0.09F, 0.125F, 0.145F, 1.0F};
constexpr ImVec4 kWhite{0.94F, 0.92F, 0.84F, 1.0F};
constexpr ImVec4 kMuted{0.62F, 0.67F, 0.66F, 1.0F};
constexpr ImVec4 kMint{0.35F, 0.91F, 0.72F, 1.0F};
constexpr ImVec4 kMintDark{0.08F, 0.29F, 0.24F, 1.0F};
constexpr ImVec4 kGold{0.92F, 0.77F, 0.38F, 1.0F};
constexpr ImVec4 kAmber{0.96F, 0.55F, 0.23F, 1.0F};
constexpr ImVec4 kAmberDark{0.31F, 0.14F, 0.07F, 1.0F};
constexpr ImVec4 kBorder{0.19F, 0.27F, 0.29F, 1.0F};

void wrapped(std::string_view text) {
  ImGui::PushTextWrapPos(0.0F);
  ImGui::TextUnformatted(text.data(), text.data() + text.size());
  ImGui::PopTextWrapPos();
}

void buttonColors(const ImVec4& normal, const ImVec4& edge) {
  ImGui::PushStyleColor(ImGuiCol_Button, normal);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, edge);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, edge);
}

void dispatch(FirstMoveUiState& ui, HuntSession& hunt,
              LayeredQuestionSession& guided, FirstMoveAction action) {
  static_cast<void>(dispatchFirstMoveAction(ui, hunt, guided, action));
}

const char* gridStatus(const LayeredQuestionRunRecord& run) {
  if (run.completed) {
    return summarizeLayeredQuestionRun(run).assisted
               ? "COMPLETED WITH HELP"
               : "COMPLETED";
  }
  if (run.currentStep == 0U && run.steps[0U].attempts.empty() &&
      !run.steps[0U].selectedOption.has_value()) {
    return "NOT STARTED";
  }
  return nullptr;
}

void drawGrid(FirstMoveUiState& ui, HuntSession& hunt,
              LayeredQuestionSession& guided, float scale) {
  const LayeredQuestionRunRecord& run = guided.currentRun();
  ImGui::PushFont(nullptr, 18.0F * scale);
  ImGui::PushStyleColor(ImGuiCol_Text, kWhite);
  wrapped("Choose a question. Each question opens into small steps. Wrong answers do not cost points.");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::Dummy({0.0F, 10.0F * scale});

  const ImVec2 origin = ImGui::GetCursorScreenPos();
  const float width = std::min(ImGui::GetContentRegionAvail().x, 720.0F * scale);
  const float height = 188.0F * scale;
  if (ImGui::InvisibleButton("##guided_question_card", {width, height})) {
    dispatch(ui, hunt, guided, {FirstMoveActionKind::GuidedOpen});
  }
  ImDrawList* draw = ImGui::GetWindowDrawList();
  draw->AddRectFilled(origin, {origin.x + width, origin.y + height},
                      ImGui::ColorConvertFloat4ToU32(kPanel), 9.0F * scale);
  draw->AddRect(origin, {origin.x + width, origin.y + height},
                ImGui::ColorConvertFloat4ToU32(kMint), 9.0F * scale, 0,
                2.0F);
  ImGui::SetCursorScreenPos({origin.x + 18.0F * scale,
                             origin.y + 16.0F * scale});
  ImGui::PushFont(nullptr, 14.0F * scale);
  ImGui::PushStyleColor(ImGuiCol_Text, kMint);
  ImGui::TextUnformatted("QUESTION 1");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::PushFont(nullptr, 32.0F * scale);
  ImGui::TextUnformatted(layeredQuestion().equation.data());
  ImGui::PopFont();
  ImGui::PushFont(nullptr, 16.0F * scale);
  ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
  ImGui::TextUnformatted(layeredQuestion().description.data());
  ImGui::PopStyleColor();
  const char* status = gridStatus(run);
  std::array<char, 48U> progress{};
  if (status == nullptr) {
    static_cast<void>(std::snprintf(progress.data(), progress.size(),
                                    "STEP %zu OF %zu", run.currentStep + 1U,
                                    kLayeredQuestionStepCount));
    status = progress.data();
  }
  ImGui::PushStyleColor(ImGuiCol_Text, kGold);
  ImGui::TextUnformatted(status);
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::SetCursorScreenPos({origin.x, origin.y + height + 8.0F * scale});
  ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
  ImGui::TextUnformatted("ENTER opens the selected question");
  ImGui::PopStyleColor();
}

bool optionHadIncorrectAttempt(const LayeredQuestionStepRecord& record,
                               std::size_t option) {
  return std::any_of(record.attempts.begin(), record.attempts.end(),
                     [option](const LayeredQuestionAttemptRecord& attempt) {
                       return attempt.optionIndex == option && !attempt.correct;
                     });
}

void drawOption(FirstMoveUiState& ui, HuntSession& hunt,
                LayeredQuestionSession& guided,
                const LayeredQuestionStepContent& content,
                const LayeredQuestionStepRecord& record,
                std::size_t option, float scale) {
  const bool selected = record.selectedOption == option;
  const bool resolved = layeredQuestionStepResolved(record);
  const bool correct = resolved && option == content.correctOption;
  const bool incorrect = optionHadIncorrectAttempt(record, option);
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  const float width = ImGui::GetContentRegionAvail().x;
  const float height = 58.0F * scale;
  ImGui::PushID(static_cast<int>(option));
  if (ImGui::InvisibleButton("##guided_option", {width, height})) {
    dispatch(ui, hunt, guided, FirstMoveAction::guidedSelectOption(option));
  }
  ImVec4 fill = selected ? kMintDark : kRaised;
  ImVec4 edge = selected ? kGold : kBorder;
  if (correct) {
    fill = kMintDark;
    edge = kMint;
  } else if (incorrect) {
    edge = kAmber;
  }
  ImDrawList* draw = ImGui::GetWindowDrawList();
  draw->AddRectFilled(origin, {origin.x + width, origin.y + height},
                      ImGui::ColorConvertFloat4ToU32(fill), 7.0F * scale);
  draw->AddRect(origin, {origin.x + width, origin.y + height},
                ImGui::ColorConvertFloat4ToU32(edge), 7.0F * scale, 0,
                selected ? 3.0F : 1.5F);
  std::array<char, 24U> label{};
  static_cast<void>(std::snprintf(label.data(), label.size(), "%c  %s",
                                  static_cast<int>('A' + option),
                                  selected ? "SELECTED" : ""));
  draw->AddText({origin.x + 12.0F * scale, origin.y + 8.0F * scale},
                ImGui::ColorConvertFloat4ToU32(selected ? kGold : kMuted),
                label.data());
  draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                {origin.x + 12.0F * scale, origin.y + 29.0F * scale},
                ImGui::ColorConvertFloat4ToU32(kWhite),
                content.options[option].label.data(),
                content.options[option].label.data() +
                    content.options[option].label.size(),
                width - 24.0F * scale);
  if (ui.guidedOptionFocus == option && ui.guidedRevealRequested &&
      !record.awaitingRecoveryChoice && !resolved) {
    ImGui::SetScrollHereY(0.5F);
    ui.guidedRevealRequested = false;
  }
  ImGui::PopID();
}

void drawAnswering(FirstMoveUiState& ui, HuntSession& hunt,
                   LayeredQuestionSession& guided, float scale) {
  const LayeredQuestionRunRecord& run = guided.currentRun();
  const LayeredQuestionStepContent& content =
      layeredQuestion().steps[run.currentStep];
  const LayeredQuestionStepRecord& record = run.steps[run.currentStep];
  std::array<char, 40U> stepLabel{};
  static_cast<void>(std::snprintf(stepLabel.data(), stepLabel.size(),
                                  "STEP %zu OF %zu", run.currentStep + 1U,
                                  kLayeredQuestionStepCount));
  ImGui::PushFont(nullptr, 14.0F * scale);
  ImGui::PushStyleColor(ImGuiCol_Text, kGold);
  ImGui::TextUnformatted(stepLabel.data());
  ImGui::PopStyleColor();
  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Text, kMint);
  ImGui::TextUnformatted(content.layerName.data());
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::PushFont(nullptr, 34.0F * scale);
  ImGui::TextUnformatted(layeredQuestion().equation.data());
  ImGui::PopFont();
  ImGui::Separator();
  ImGui::PushFont(nullptr, 19.0F * scale);
  wrapped(content.prompt);
  ImGui::PopFont();
  for (std::size_t option = 0U; option < kLayeredQuestionOptionCount; ++option) {
    drawOption(ui, hunt, guided, content, record, option, scale);
  }

  if (record.awaitingRecoveryChoice) {
    ImGui::PushStyleColor(ImGuiCol_Text, kAmber);
    wrapped(content.wrongHint);
    ImGui::PopStyleColor();
    buttonColors(kAmberDark, kAmber);
    if (ImGui::Button("TRY AGAIN  [T]", {190.0F * scale, 44.0F * scale})) {
      dispatch(ui, hunt, guided, {FirstMoveActionKind::GuidedTryAgain});
    }
    ImGui::SameLine();
    if (ImGui::Button("SHOW ME  [S]", {190.0F * scale, 44.0F * scale})) {
      dispatch(ui, hunt, guided, {FirstMoveActionKind::GuidedShowAnswer});
    }
    ImGui::PopStyleColor(3);
    if (ui.guidedRevealRequested) {
      ImGui::SetScrollHereY(1.0F);
      ui.guidedRevealRequested = false;
    }
  } else if (layeredQuestionStepResolved(record)) {
    ImGui::PushStyleColor(ImGuiCol_Text, kMint);
    ImGui::TextUnformatted(record.answerShown ? "ANSWER SHOWN" : "CORRECT");
    ImGui::PopStyleColor();
    wrapped(content.explanation);
    buttonColors(kMintDark, kMint);
    const char* next = run.currentStep + 1U == kLayeredQuestionStepCount
                           ? "SEE SUMMARY  [ENTER]"
                           : "NEXT STEP  [ENTER]";
    if (ImGui::Button(next, {250.0F * scale, 44.0F * scale})) {
      dispatch(ui, hunt, guided, {FirstMoveActionKind::GuidedContinue});
    }
    ImGui::PopStyleColor(3);
    if (ui.guidedRevealRequested) {
      ImGui::SetScrollHereY(1.0F);
      ui.guidedRevealRequested = false;
    }
  } else {
    buttonColors(kMintDark, kMint);
    if (ImGui::Button("CHECK ANSWER  [ENTER]",
                      {250.0F * scale, 44.0F * scale})) {
      dispatch(ui, hunt, guided, {FirstMoveActionKind::GuidedCheck});
    }
    ImGui::PopStyleColor(3);
  }
  ImGui::SameLine();
  if (ImGui::Button("BACK TO QUESTIONS  [ESC]")) {
    dispatch(ui, hunt, guided, {FirstMoveActionKind::GuidedBackToGrid});
  }
}

void drawComplete(FirstMoveUiState& ui, HuntSession& hunt,
                  LayeredQuestionSession& guided, float scale) {
  const LayeredQuestionRunSummary summary =
      summarizeLayeredQuestionRun(guided.currentRun());
  ImGui::PushFont(nullptr, 30.0F * scale);
  ImGui::PushStyleColor(ImGuiCol_Text, kMint);
  ImGui::TextUnformatted("QUESTION COMPLETE");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::PushFont(nullptr, 22.0F * scale);
  ImGui::TextUnformatted("3a + 5 = 20");
  ImGui::TextUnformatted("3a = 15        subtract 5 from both sides");
  ImGui::TextUnformatted("a = 5          divide both sides by 3");
  ImGui::PopFont();
  std::array<char, 256U> evidence{};
  static_cast<void>(std::snprintf(
      evidence.data(), evidence.size(),
      "First-try correct: %zu / 7\nCorrected after trying again: %zu\nAnswers shown: %zu\nIncorrect attempts: %zu",
      summary.correctOnFirstTry, summary.correctedAfterRetry,
      summary.shownAnswers, summary.incorrectCheckedAttempts));
  ImGui::PushFont(nullptr, 18.0F * scale);
  ImGui::TextUnformatted(evidence.data());
  ImGui::PushStyleColor(ImGuiCol_Text, summary.assisted ? kAmber : kMint);
  ImGui::TextUnformatted(summary.assisted ? "COMPLETED WITH HELP"
                                         : "COMPLETED WITHOUT ANSWER REVEALS");
  ImGui::PopStyleColor();
  wrapped("This records work on this question. It is not a general mastery score.");
  ImGui::PopFont();
  if (ImGui::Button("BACK TO QUESTIONS", {220.0F * scale, 44.0F * scale})) {
    dispatch(ui, hunt, guided, {FirstMoveActionKind::GuidedBackToGrid});
  }
  ImGui::SameLine();
  buttonColors(kMintDark, kMint);
  if (ImGui::Button("PRACTICE AGAIN", {220.0F * scale, 44.0F * scale})) {
    dispatch(ui, hunt, guided, {FirstMoveActionKind::GuidedRestart});
  }
  ImGui::PopStyleColor(3);
}

}  // namespace

void drawLayeredQuestionUi(FirstMoveUiState& ui, HuntSession& hunt,
                           LayeredQuestionSession& guided, float scale) {
  const float width = ImGui::GetContentRegionAvail().x;
  const float height = ImGui::GetContentRegionAvail().y;
  if (std::fabs(ui.presentedGuidedWidth - width) > 0.5F ||
      std::fabs(ui.presentedGuidedHeight - height) > 0.5F ||
      std::fabs(ui.presentedGuidedScale - scale) > 0.01F) {
    ui.guidedRevealRequested = true;
    ui.presentedGuidedWidth = width;
    ui.presentedGuidedHeight = height;
    ui.presentedGuidedScale = scale;
  }
  ImGui::PushStyleColor(ImGuiCol_ChildBg, kInk);
  if (ImGui::BeginChild("##guided_outer_scroll", {0.0F, 0.0F},
                        ImGuiChildFlags_Borders)) {
    switch (guided.currentRun().phase) {
      case LayeredQuestionPhase::Grid:
        drawGrid(ui, hunt, guided, scale);
        break;
      case LayeredQuestionPhase::Answering:
        drawAnswering(ui, hunt, guided, scale);
        break;
      case LayeredQuestionPhase::Complete:
        drawComplete(ui, hunt, guided, scale);
        break;
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();
}

}  // namespace iggy3d::first_move

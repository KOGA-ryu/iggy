#include "LayeredQuestionUi.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

#include "FirstMoveUi.hpp"
#include "imgui.h"

namespace iggy3d::first_move {
namespace {
constexpr ImVec4 kPanel{0.065F, 0.095F, 0.115F, 1};
constexpr ImVec4 kRaised{0.09F, 0.125F, 0.145F, 1};
constexpr ImVec4 kWhite{0.94F, 0.92F, 0.84F, 1};
constexpr ImVec4 kMuted{0.62F, 0.67F, 0.66F, 1};
constexpr ImVec4 kMint{0.35F, 0.91F, 0.72F, 1};
constexpr ImVec4 kMintDark{0.08F, 0.29F, 0.24F, 1};
constexpr ImVec4 kGold{0.92F, 0.77F, 0.38F, 1};
constexpr ImVec4 kAmber{0.96F, 0.55F, 0.23F, 1};
constexpr ImVec4 kBorder{0.19F, 0.27F, 0.29F, 1};

void wrapped(std::string_view text) {
  ImGui::PushTextWrapPos(0);
  ImGui::TextUnformatted(text.data(), text.data() + text.size());
  ImGui::PopTextWrapPos();
}
void dispatch(FirstMoveUiState& ui, HuntSession& hunt, LayeredQuestionSession& guided, FirstMoveAction action) {
  static_cast<void>(dispatchFirstMoveAction(ui, hunt, guided, action));
}
void drawGrid(FirstMoveUiState& ui, HuntSession& hunt, LayeredQuestionSession& guided, float scale) {
  const auto& run = guided.currentRun();
  wrapped("Choose a question. Each question opens into small steps. Wrong answers do not cost points.");
  ImGui::Dummy({0, 12 * scale});
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  const float width = std::min(ImGui::GetContentRegionAvail().x, 720 * scale);
  const float height = 182 * scale;
  if (ImGui::InvisibleButton("##guided_question_card", {width, height}))
    dispatch(ui, hunt, guided, {FirstMoveActionKind::GuidedOpen});
  auto* draw = ImGui::GetWindowDrawList();
  draw->AddRectFilled(origin, {origin.x + width, origin.y + height}, ImGui::ColorConvertFloat4ToU32(kPanel), 9 * scale);
  draw->AddRect(origin, {origin.x + width, origin.y + height}, ImGui::ColorConvertFloat4ToU32(kMint), 9 * scale, 2.0F, 0);
  const auto text = [&](float y, float size, ImVec4 color, std::string_view value) {
    ImGui::PushFont(nullptr, size * scale);
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), {origin.x + 18 * scale, origin.y + y * scale},
        ImGui::ColorConvertFloat4ToU32(color), value.data(), value.data() + value.size(), width - 36 * scale);
    ImGui::PopFont();
  };
  text(16, 14, kMint, "QUESTION 1");
  text(44, 32, kWhite, layeredQuestion().equation);
  text(96, 16, kMuted, layeredQuestion().description);
  std::string status = "NOT STARTED";
  if (run.completed) status = summarizeLayeredQuestionRun(run).assisted ? "COMPLETED WITH HELP" : "COMPLETED";
  else if (run.currentStep || !run.steps[0].attempts.empty() || run.steps[0].selectedOption)
    status = "STEP " + std::to_string(run.currentStep + 1) + " OF " + std::to_string(kLayeredQuestionStepCount);
  text(142, 14, kGold, status);
  ImGui::TextColored(kMuted, "ENTER opens the selected question");
}

void drawOption(FirstMoveUiState& ui, HuntSession& hunt, LayeredQuestionSession& guided,
                const LayeredQuestionStepContent& content, const LayeredQuestionStepRecord& record,
                std::size_t option, float scale) {
  const bool selected = record.selectedOption == option;
  const bool resolved = layeredQuestionStepResolved(record);
  const bool correct = resolved && acceptsOption(content, option);
  const bool incorrect = std::any_of(record.attempts.begin(), record.attempts.end(), [option](const auto& attempt) {
    return attempt.optionIndex == option && !attempt.correct;
  });
  const std::string_view status = correct ? (record.answerShown ? "ANSWER SHOWN" : "CORRECT")
      : incorrect ? "YOUR INCORRECT CHOICE" : selected ? "SELECTED" : "";
  const std::string label = std::string(1, static_cast<char>('A' + option)) + "  " + std::string(status);
  const float width = ImGui::GetContentRegionAvail().x;
  const float padding = 12 * scale;
  const float wrap = std::max(1.0F, width - 2 * padding);
  ImGui::PushFont(nullptr, 14 * scale);
  const float labelHeight = ImGui::CalcTextSize(label.c_str(), nullptr, false, wrap).y;
  ImGui::PopFont();
  const auto optionText = content.options[option].label;
  const float textHeight = ImGui::CalcTextSize(optionText.data(), optionText.data() + optionText.size(), false, wrap).y;
  const float height = labelHeight + textHeight + 2 * padding + 4 * scale;
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  ImGui::PushID(static_cast<int>(option));
  if (ImGui::InvisibleButton("##guided_option", {width, height}))
    dispatch(ui, hunt, guided, FirstMoveAction::guidedSelectOption(option));
  const auto edge = correct ? kMint : incorrect ? kAmber : selected ? kGold : kBorder;
  auto* draw = ImGui::GetWindowDrawList();
  draw->AddRectFilled(origin, {origin.x + width, origin.y + height},
      ImGui::ColorConvertFloat4ToU32(selected ? kMintDark : kRaised), 7 * scale);
  draw->AddRect(origin, {origin.x + width, origin.y + height}, ImGui::ColorConvertFloat4ToU32(edge), 7 * scale, 0, selected ? 3 : 1.5F);
  ImGui::PushFont(nullptr, 14 * scale);
  draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), {origin.x + padding, origin.y + padding},
      ImGui::ColorConvertFloat4ToU32(status.empty() ? kMuted : edge), label.c_str(), nullptr, wrap);
  ImGui::PopFont();
  draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), {origin.x + padding, origin.y + padding + labelHeight + 4 * scale},
      ImGui::ColorConvertFloat4ToU32(kWhite), optionText.data(), optionText.data() + optionText.size(), wrap);
  if (ui.guidedRevealRequested && ui.guidedReveal == GuidedReveal::Option && ui.guidedOptionFocus == option) {
    ImGui::SetScrollHereY(0.5F);
    ui.guidedRevealRequested = false;
  }
  ImGui::PopID();
}

void drawAnswering(FirstMoveUiState& ui, HuntSession& hunt, LayeredQuestionSession& guided, float scale) {
  const auto& run = guided.currentRun();
  const auto& content = guided.content().steps[run.currentStep];
  const auto& record = run.steps[run.currentStep];
  if (ui.guidedRevealRequested && ui.guidedReveal == GuidedReveal::Prompt) {
    ImGui::SetScrollY(0);
    ui.guidedRevealRequested = false;
  }
  const auto working = guided.visibleWorking();
  if (!working.empty() && working != guided.content().equation) {
    ImGui::PushFont(nullptr, 14 * scale);
    ImGui::TextColored(kMint, "CURRENT WORK");
    ImGui::PopFont();
    ImGui::PushFont(nullptr, 28 * scale);
    wrapped(working);
    ImGui::PopFont();
    ImGui::Separator();
  }
  ImGui::PushFont(nullptr, 19 * scale);
  wrapped(content.prompt);
  ImGui::PopFont();
  for (std::size_t i = 0; i < kLayeredQuestionOptionCount; ++i)
    drawOption(ui, hunt, guided, content, record, i, scale);
  if (record.awaitingRecoveryChoice || layeredQuestionStepResolved(record)) {
    ImGui::Dummy({0, 6 * scale});
    ImGui::TextColored(record.awaitingRecoveryChoice ? kAmber : kMint,
        record.awaitingRecoveryChoice ? "LET'S TAKE ANOTHER LOOK" : record.answerShown ? "ANSWER SHOWN" : "CORRECT");
    if (ui.guidedRevealRequested && ui.guidedReveal == GuidedReveal::Feedback) {
      ImGui::SetScrollHereY(0);
      ui.guidedRevealRequested = false;
    }
    wrapped(record.awaitingRecoveryChoice ? content.wrongHint : content.explanation);
  }
}
void drawComplete(const LayeredQuestionSession& guided, float scale) {
  const auto summary = summarizeLayeredQuestionRun(guided.currentRun());
  ImGui::PushFont(nullptr, 28 * scale);
  ImGui::TextColored(kMint, "QUESTION COMPLETE");
  ImGui::PopFont();
  ImGui::PushFont(nullptr, 22 * scale);
  wrapped(guided.visibleWorking());
  ImGui::PopFont();
  ImGui::Separator();
  ImGui::Text("First-try correct: %zu / %zu", summary.correctOnFirstTry, kLayeredQuestionStepCount);
  ImGui::Text("Corrected after trying again: %zu", summary.correctedAfterRetry);
  ImGui::Text("Answers shown: %zu", summary.shownAnswers);
  ImGui::Text("Incorrect checks: %zu", summary.incorrectCheckedAttempts);
  ImGui::TextColored(summary.assisted ? kAmber : kMint, summary.assisted ? "COMPLETED WITH HELP" : "COMPLETED WITHOUT ANSWER REVEALS");
  wrapped("This records work on this question. It is not a general mastery score.");
}
struct FooterAction {
  const char* label;
  FirstMoveActionKind kind;
  bool enabled = true;
};
std::vector<FooterAction> footerActions(const LayeredQuestionRunRecord& run) {
  using A = FirstMoveActionKind;
  if (run.phase == LayeredQuestionPhase::Grid) return {};
  if (run.phase == LayeredQuestionPhase::Complete)
    return {{"BACK TO QUESTIONS  [ESC]", A::GuidedBackToGrid}, {"PRACTICE AGAIN  [R]", A::GuidedRestart}};
  const auto& step = run.steps[run.currentStep];
  std::vector<FooterAction> actions;
  if (step.awaitingRecoveryChoice)
    actions = {{"TRY AGAIN  [T]", A::GuidedTryAgain}, {"SHOW ME  [S]", A::GuidedShowAnswer}};
  else if (layeredQuestionStepResolved(step))
    actions = {{run.currentStep + 1 == kLayeredQuestionStepCount ? "SEE SUMMARY  [ENTER]" : "NEXT STEP  [ENTER]", A::GuidedContinue}};
  else actions = {{"CHECK ANSWER  [ENTER]", A::GuidedCheck, step.selectedOption.has_value()}};
  actions.push_back({"BACK TO QUESTIONS  [ESC]", A::GuidedBackToGrid});
  return actions;
}
}

void drawLayeredQuestionUi(FirstMoveUiState& ui, HuntSession& hunt, LayeredQuestionSession& guided, float scale) {
  const float width = ImGui::GetContentRegionAvail().x;
  const float height = ImGui::GetContentRegionAvail().y;
  const auto& run = guided.currentRun();
  if (std::fabs(ui.presentedGuidedWidth - width) > 0.5F ||
      std::fabs(ui.presentedGuidedHeight - height) > 0.5F ||
      std::fabs(ui.presentedGuidedScale - scale) > 0.01F) {
    ui.guidedRevealRequested = true;
    ui.presentedGuidedWidth = width;
    ui.presentedGuidedHeight = height;
    ui.presentedGuidedScale = scale;
  }
  if (run.phase == LayeredQuestionPhase::Answering) {
    ImGui::PushFont(nullptr, 14 * scale);
    ImGui::TextColored(kGold, "STEP %zu OF %zu  /  %s", run.currentStep + 1, kLayeredQuestionStepCount,
                       layeredQuestion().steps[run.currentStep].layerName.data());
    ImGui::PopFont();
    ImGui::PushFont(nullptr, 28 * scale);
    ImGui::TextUnformatted(layeredQuestion().equation.data());
    ImGui::PopFont();
  }
  const auto actions = footerActions(run);
  const float spacing = ImGui::GetStyle().ItemSpacing.x;
  const float buttonHeight = ImGui::GetFrameHeight() + 10 * scale;
  std::vector<float> widths;
  float x = 0;
  std::size_t rows = actions.empty() ? 0 : 1;
  for (const auto& action : actions) {
    const float buttonWidth = ImGui::CalcTextSize(action.label).x + 24 * scale;
    widths.push_back(buttonWidth);
    if (x && x + buttonWidth > width) { ++rows; x = 0; }
    x += buttonWidth + spacing;
  }
  const float footerHeight = rows * (buttonHeight + ImGui::GetStyle().ItemSpacing.y) + (rows ? 8 * scale : 0);
  const float centerHeight = std::max(1.0F, ImGui::GetContentRegionAvail().y - footerHeight);
  if (ImGui::BeginChild("##guided_outer_scroll", {0, centerHeight}, ImGuiChildFlags_Borders)) {
    switch (run.phase) {
      case LayeredQuestionPhase::Grid: drawGrid(ui, hunt, guided, scale); break;
      case LayeredQuestionPhase::Answering: drawAnswering(ui, hunt, guided, scale); break;
      case LayeredQuestionPhase::Complete: drawComplete(guided, scale); break;
    }
    switch (ui.guidedReading) {
      case GuidedReading::None: break;
      case GuidedReading::PageUp: ImGui::SetScrollY(std::max(0.0F, ImGui::GetScrollY() - centerHeight * 0.8F)); break;
      case GuidedReading::PageDown: ImGui::SetScrollY(std::min(ImGui::GetScrollMaxY(), ImGui::GetScrollY() + centerHeight * 0.8F)); break;
      case GuidedReading::Start: ImGui::SetScrollY(0); break;
      case GuidedReading::End: ImGui::SetScrollY(ImGui::GetScrollMaxY()); break;
    }
    ui.guidedReading = GuidedReading::None;
  }
  ImGui::EndChild();
  if (rows) ImGui::Dummy({0, 2 * scale});
  x = 0;
  for (std::size_t i = 0; i < actions.size(); ++i) {
    if (x && x + widths[i] <= width) ImGui::SameLine(); else x = 0;
    const auto& action = actions[i];
    ImGui::BeginDisabled(!action.enabled);
    if (ImGui::Button(action.label, {widths[i], buttonHeight})) dispatch(ui, hunt, guided, {action.kind});
    ImGui::EndDisabled();
    x += widths[i] + spacing;
  }
}
}  // namespace iggy3d::first_move

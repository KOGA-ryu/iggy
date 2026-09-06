#include "FirstMoveUi.hpp"
#include "LayeredQuestionUi.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include "imgui.h"

namespace iggy3d::first_move {
namespace {

constexpr ImVec4 kInk{0.035F, 0.055F, 0.075F, 1.0F};
constexpr ImVec4 kPanel{0.065F, 0.095F, 0.115F, 1.0F};
constexpr ImVec4 kPanelRaised{0.09F, 0.125F, 0.145F, 1.0F};
constexpr ImVec4 kWarmWhite{0.94F, 0.92F, 0.84F, 1.0F};
constexpr ImVec4 kMuted{0.62F, 0.67F, 0.66F, 1.0F};
constexpr ImVec4 kMint{0.35F, 0.91F, 0.72F, 1.0F};
constexpr ImVec4 kMintDark{0.08F, 0.29F, 0.24F, 1.0F};
constexpr ImVec4 kGold{0.92F, 0.77F, 0.38F, 1.0F};
constexpr ImVec4 kGoldDark{0.28F, 0.23F, 0.10F, 1.0F};
constexpr ImVec4 kAmber{0.96F, 0.55F, 0.23F, 1.0F};
constexpr ImVec4 kAmberDark{0.31F, 0.14F, 0.07F, 1.0F};
constexpr ImVec4 kBorder{0.19F, 0.27F, 0.29F, 1.0F};

struct PlayerFeedback {
  std::string_view reason;
  std::string_view message;
};

constexpr std::array<PlayerFeedback, 9U> kPlayerFeedback{{
    {"review_modal_blocks_command", "Finish or close the open review first."},
    {"cell_coordinates_invalid", "That cell is not available."},
    {"row_not_visible", "That row has already left the board."},
    {"no_visible_row", "There is no row available for that action."},
    {"row_already_assessed", "That row's first response is already recorded."},
    {"row_does_not_need_review", "The active row does not need an explanation."},
    {"review_not_open", "Open a row explanation before releasing it."},
    {"row_not_explained", "Read the explanation before releasing this row."},
    {"run_not_finished", "Finish every row before starting a new run."},
}};

[[nodiscard]] std::string_view playerFeedbackFor(
    std::string_view reason) noexcept {
  const auto found = std::find_if(
      kPlayerFeedback.begin(), kPlayerFeedback.end(),
      [reason](const PlayerFeedback& feedback) {
        return feedback.reason == reason;
      });
  return found == kPlayerFeedback.end()
             ? std::string_view{"That action is not available right now."}
             : found->message;
}

void wrappedText(std::string_view text) {
  ImGui::PushTextWrapPos(0.0F);
  ImGui::TextUnformatted(text.data(), text.data() + text.size());
  ImGui::PopTextWrapPos();
}

template <typename Result>
void remember(FirstMoveUiState& ui, const Result& result) {
  ui.lastActionAccepted = result.accepted;
  ui.lastActionReason = result.reason;
}

void reconcileReviewFocus(FirstMoveUiState& ui,
                          const HuntSession& session) {
  const std::optional<std::size_t> reviewRow = session.reviewRowIndex();
  if (!reviewRow.has_value()) {
    ui.reviewFocusValid = false;
    ui.reviewFocusRow = 0U;
    ui.reviewExplanationIndex = 0U;
    ui.reviewRevealRequested = false;
    ui.presentedReviewWidth = 0.0F;
    ui.presentedReviewHeight = 0.0F;
    ui.presentedReviewScale = 0.0F;
    return;
  }
  if (!ui.reviewFocusValid || ui.reviewFocusRow != *reviewRow) {
    ui.reviewFocusValid = true;
    ui.reviewFocusRow = *reviewRow;
    ui.reviewExplanationIndex = 0U;
    ui.reviewRevealRequested = true;
    ui.presentedReviewWidth = 0.0F;
    ui.presentedReviewHeight = 0.0F;
    ui.presentedReviewScale = 0.0F;
  }
  ui.reviewExplanationIndex =
      std::min(ui.reviewExplanationIndex, kHuntCellsPerRow - 1U);
}

void selectReviewExplanation(FirstMoveUiState& ui,
                             const HuntSession& session,
                             std::size_t index) {
  reconcileReviewFocus(ui, session);
  if (!ui.reviewFocusValid) {
    return;
  }
  const std::size_t next = std::min(index, kHuntCellsPerRow - 1U);
  ui.reviewExplanationIndex = next;
  ui.reviewRevealRequested = true;
  ui.lastActionAccepted = true;
  ui.lastActionReason = "review_navigation";
}

void dispatchAndRemember(FirstMoveUiState& ui,
                         HuntSession& hunt,
                         LayeredQuestionSession& guided,
                         const FirstMoveAction& action) {
  remember(ui, dispatchFirstMoveAction(ui, hunt, guided, action));
  reconcileReviewFocus(ui, hunt);
}

void pushButtonPalette(const ImVec4& normal,
                       const ImVec4& hovered,
                       const ImVec4& active) {
  ImGui::PushStyleColor(ImGuiCol_Button, normal);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovered);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);
}

void popButtonPalette() {
  ImGui::PopStyleColor(3);
}

bool drawScaleChoice(FirstMoveUiState& ui, const char* label, float scale) {
  const bool selected = std::fabs(ui.textScale - scale) < 0.01F;
  if (selected) {
    pushButtonPalette(kMintDark, {0.11F, 0.39F, 0.31F, 1.0F},
                      {0.14F, 0.46F, 0.36F, 1.0F});
  }
  const bool pressed = ImGui::SmallButton(label);
  const bool active = pressed || ImGui::IsItemActive();
  if (selected) {
    popButtonPalette();
  }
  if (pressed) {
    ui.textScale = scale;
  }
  return active;
}

void drawRuleCard(float scale) {
  ImGui::PushStyleColor(ImGuiCol_ChildBg, kPanel);
  ImGui::PushStyleColor(ImGuiCol_Border, kMintDark);
  const float height = 108.0F * scale;
  if (ImGui::BeginChild("##hunt_rule", {0.0F, height},
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
    ImGui::PushFont(nullptr, 20.0F * scale);
    ImGui::PushStyleColor(ImGuiCol_Text, kWarmWhite);
    wrappedText("Select equations linear in a and b. x is a known real number.");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::PushFont(nullptr, 17.0F * scale);
    ImGui::PushStyleColor(ImGuiCol_Text, kMint);
    wrappedText("Each row is one question. Select every equation that fits the rule. Not selected means your answer is no. Check Row locks that answer.");
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    wrappedText("Notation: a^2 = a squared; ab = a times b");
    ImGui::PopStyleColor();
    ImGui::PopFont();
  }
  ImGui::EndChild();
  ImGui::PopStyleColor(2);
}

void drawStat(std::string_view label,
              std::string_view value,
              const ImVec4& accent,
              float width,
              float scale) {
  ImGui::PushStyleColor(ImGuiCol_ChildBg, kPanel);
  ImGui::PushStyleColor(ImGuiCol_Border, kBorder);
  std::string id = "##stat_";
  id.append(label);
  if (ImGui::BeginChild(id.c_str(), {width, 62.0F * scale},
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
    ImGui::PushFont(nullptr, 14.0F * scale);
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextUnformatted(label.data(), label.data() + label.size());
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::PushFont(nullptr, 20.0F * scale);
    ImGui::PushStyleColor(ImGuiCol_Text, accent);
    ImGui::TextUnformatted(value.data(), value.data() + value.size());
    ImGui::PopStyleColor();
    ImGui::PopFont();
  }
  ImGui::EndChild();
  ImGui::PopStyleColor(2);
}

void drawStats(const HuntSession& session, float scale) {
  const HuntRunRecord& run = session.currentRun();
  const HuntRunSummary summary = summarizeRun(run);
  std::array<char, 32U> points{};
  std::array<char, 32U> ready{};
  std::array<char, 32U> first{};
  std::array<char, 32U> explained{};
  static_cast<void>(std::snprintf(points.data(), points.size(), "%u", run.score));
  static_cast<void>(std::snprintf(ready.data(), ready.size(), "%zu", summary.readyRows));
  static_cast<void>(std::snprintf(first.data(), first.size(), "%zu / %zu",
                                  summary.correctRows,
                                  summary.correctRows + summary.incorrectRows));
  static_cast<void>(std::snprintf(explained.data(), explained.size(), "%zu",
                                  summary.explainedRows));
  const float spacing = ImGui::GetStyle().ItemSpacing.x;
  const float width = std::max(
      110.0F * scale,
      (ImGui::GetContentRegionAvail().x - spacing * 3.0F) / 4.0F);
  drawStat("POINTS", points.data(), kMint, width, scale);
  ImGui::SameLine();
  drawStat("READY", ready.data(), kGold, width, scale);
  ImGui::SameLine();
  drawStat("FIRST CHECK", first.data(), kWarmWhite, width, scale);
  ImGui::SameLine();
  drawStat("EXPLAINED", explained.data(), kAmber, width, scale);
}

float rowCardHeight(const HuntRowContent& content,
                    float cardWidth,
                    float scale) {
  ImGui::PushFont(nullptr, 20.0F * scale);
  float height = 0.0F;
  const float wrapWidth = std::max(40.0F, cardWidth - 20.0F * scale);
  for (const HuntCellContent& cell : content.cells) {
    const ImVec2 measured = ImGui::CalcTextSize(
        cell.expression.data(), cell.expression.data() + cell.expression.size(),
        false, wrapWidth);
    height = std::max(height, measured.y + 42.0F * scale);
  }
  ImGui::PopFont();
  return std::max(height, 72.0F * scale);
}

bool drawCellCard(const HuntCellContent& content,
                  bool marked,
                  bool focused,
                  bool revealFocus,
                  RowState rowState,
                  float width,
                  float height,
                  float scale) {
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  const std::string id = "##" + std::string(content.stableId);
  const bool pressed = ImGui::InvisibleButton(id.c_str(), {width, height});
  const bool hovered = ImGui::IsItemHovered();
  if (focused && revealFocus) {
    ImGui::SetScrollHereX(0.5F);
    ImGui::SetScrollHereY(0.5F);
  }

  ImVec4 fill = kPanelRaised;
  if (marked) {
    fill = kMintDark;
  } else if (rowState == RowState::Ready) {
    fill = kGoldDark;
  } else if (rowState == RowState::NeedsReview) {
    fill = kAmberDark;
  }
  if (hovered) {
    fill.x = std::min(1.0F, fill.x + 0.045F);
    fill.y = std::min(1.0F, fill.y + 0.045F);
    fill.z = std::min(1.0F, fill.z + 0.045F);
  }
  const ImVec4 edge = focused ? kMint : rowState == RowState::Ready
                                             ? kGold
                                             : rowState == RowState::NeedsReview
                                                   ? kAmber
                                                   : kBorder;
  ImDrawList* draw = ImGui::GetWindowDrawList();
  const ImVec2 end{origin.x + width, origin.y + height};
  draw->AddRectFilled(origin, end, ImGui::ColorConvertFloat4ToU32(fill),
                      8.0F * scale);
  draw->AddRect(origin, end, ImGui::ColorConvertFloat4ToU32(edge),
                8.0F * scale, 0, focused ? 3.0F : 1.5F);

  ImGui::PushFont(nullptr, 12.0F * scale);
  const char* markLabel = marked ? "SELECTED" : "NOT SELECTED";
  draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                {origin.x + 10.0F * scale, origin.y + 8.0F * scale},
                ImGui::ColorConvertFloat4ToU32(marked ? kMint : kMuted),
                markLabel);
  ImGui::PopFont();
  ImGui::PushFont(nullptr, 20.0F * scale);
  draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                {origin.x + 10.0F * scale, origin.y + 28.0F * scale},
                ImGui::ColorConvertFloat4ToU32(kWarmWhite),
                content.expression.data(),
                content.expression.data() + content.expression.size(),
                std::max(40.0F, width - 20.0F * scale));
  ImGui::PopFont();
  return pressed;
}

void selectRowForAction(FirstMoveUiState& ui,
                        HuntSession& session,
                        LayeredQuestionSession& guided,
                        std::size_t rowIndex) {
  dispatchAndRemember(
      ui, session, guided,
      FirstMoveAction::huntSelectCell(
          rowIndex, std::min(session.activeColumn(), kHuntCellsPerRow - 1U)));
}

void drawBoardRow(FirstMoveUiState& ui,
                  HuntSession& session,
                  LayeredQuestionSession& guided,
                  std::size_t rowIndex,
                  bool revealFocusedCell,
                  float scale) {
  const HuntRowRecord& row = session.currentRun().rows[rowIndex];
  const HuntRowContent& content = huntPack().rows[rowIndex];
  const float spacing = ImGui::GetStyle().ItemSpacing.x;
  const float rowMetaWidth = std::min(132.0F * scale, 152.0F);
  const float actionWidth = std::min(112.0F * scale, 145.0F);
  const float available = ImGui::GetContentRegionAvail().x;
  const float cardsAvailable =
      available - rowMetaWidth - actionWidth - spacing * 6.0F - 24.0F;
  const float cardWidth = std::max(108.0F, cardsAvailable / 4.0F);
  const float cardHeight = rowCardHeight(content, cardWidth, scale);

  ImGui::PushID(static_cast<int>(rowIndex));
  ImGui::BeginGroup();
  ImGui::PushFont(nullptr, 18.0F * scale);
  std::array<char, 32U> rowLabel{};
  static_cast<void>(std::snprintf(rowLabel.data(), rowLabel.size(), "ROW %zu",
                                  rowIndex + 1U));
  ImGui::PushStyleColor(ImGuiCol_Text, kWarmWhite);
  ImGui::TextUnformatted(rowLabel.data());
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::PushFont(nullptr, 13.0F * scale);
  const char* stateLabel = row.state == RowState::Ready
                               ? "READY"
                               : row.state == RowState::NeedsReview
                                     ? "NEEDS REVIEW"
                                     : "EDITING";
  ImGui::PushStyleColor(ImGuiCol_Text,
                        row.state == RowState::Ready
                            ? kGold
                            : row.state == RowState::NeedsReview ? kAmber : kMuted);
  ImGui::TextUnformatted(stateLabel);
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::Dummy({rowMetaWidth, std::max(0.0F, cardHeight - 52.0F * scale)});
  ImGui::EndGroup();

  for (std::size_t column = 0U; column < kHuntCellsPerRow; ++column) {
    ImGui::SameLine();
    const bool marked = (row.selectedMask & (1U << column)) != 0U;
    const bool focused = session.activeRowIndex() == rowIndex &&
                         session.activeColumn() == column;
    if (drawCellCard(content.cells[column], marked, focused,
                     revealFocusedCell, row.state,
                     cardWidth, cardHeight, scale)) {
      dispatchAndRemember(ui, session, guided,
                          FirstMoveAction::huntSelectCell(rowIndex, column));
      dispatchAndRemember(ui, session, guided,
                          {FirstMoveActionKind::HuntToggleMark});
    }
  }

  ImGui::SameLine();
  ImGui::BeginGroup();
  ImGui::Dummy({0.0F, 14.0F * scale});
  if (row.state == RowState::Editing) {
    pushButtonPalette(kMintDark, {0.11F, 0.39F, 0.31F, 1.0F},
                      {0.14F, 0.46F, 0.36F, 1.0F});
    if (ImGui::Button("CHECK ROW", {actionWidth, 44.0F * scale})) {
      selectRowForAction(ui, session, guided, rowIndex);
      dispatchAndRemember(ui, session, guided,
                          {FirstMoveActionKind::HuntCommitRow});
    }
    popButtonPalette();
  } else if (row.state == RowState::NeedsReview) {
    pushButtonPalette(kAmberDark, {0.43F, 0.20F, 0.08F, 1.0F},
                      {0.52F, 0.24F, 0.09F, 1.0F});
    if (ImGui::Button("EXPLAIN", {actionWidth, 44.0F * scale})) {
      selectRowForAction(ui, session, guided, rowIndex);
      dispatchAndRemember(ui, session, guided,
                          {FirstMoveActionKind::HuntOpenReview});
    }
    popButtonPalette();
  } else {
    ImGui::BeginDisabled();
    ImGui::Button("READY", {actionWidth, 44.0F * scale});
    ImGui::EndDisabled();
    ImGui::PushStyleColor(ImGuiCol_Text, kGold);
    wrappedText("Correct · ready to clear");
    ImGui::PopStyleColor();
  }
  ImGui::EndGroup();
  ImGui::PopID();
  ImGui::Separator();
}

void drawBoard(FirstMoveUiState& ui, HuntSession& session,
               LayeredQuestionSession& guided, float scale) {
  const std::optional<std::size_t> activeRow = session.activeRowIndex();
  const std::size_t activeColumn = session.activeColumn();
  const bool focusChanged =
      activeRow.has_value() &&
      (!ui.presentedFocusValid || ui.presentedFocusRow != *activeRow ||
       ui.presentedFocusColumn != activeColumn);
  const float footerReserve = 124.0F * scale;
  const float height = std::max(220.0F, ImGui::GetContentRegionAvail().y - footerReserve);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, kInk);
  if (ImGui::BeginChild("##hunt_board", {0.0F, height},
                        ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    const std::vector<std::size_t> visible = session.visibleRowIndices();
    for (const std::size_t rowIndex : visible) {
      drawBoardRow(ui, session, guided, rowIndex, focusChanged, scale);
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor();
  if (activeRow.has_value() && session.activeRowIndex() == activeRow &&
      session.activeColumn() == activeColumn) {
    ui.presentedFocusValid = true;
    ui.presentedFocusRow = *activeRow;
    ui.presentedFocusColumn = activeColumn;
  } else if (!session.activeRowIndex().has_value()) {
    ui.presentedFocusValid = false;
  }
}

float reviewCardHeight(const HuntCellContent& cell,
                       float width,
                       float scale) {
  const float paddingX = 12.0F * scale;
  const float paddingY = 10.0F * scale;
  const float wrapWidth = std::max(40.0F, width - paddingX * 2.0F);
  ImGui::PushFont(nullptr, 13.0F * scale);
  const float headingHeight = ImGui::GetTextLineHeight();
  ImGui::PopFont();
  ImGui::PushFont(nullptr, 20.0F * scale);
  const float expressionHeight =
      ImGui::CalcTextSize(cell.expression.data(),
                          cell.expression.data() + cell.expression.size(),
                          false, wrapWidth)
          .y;
  ImGui::PopFont();
  ImGui::PushFont(nullptr, 15.0F * scale);
  const float labelHeight = ImGui::GetTextLineHeight();
  const float explanationHeight =
      ImGui::CalcTextSize(cell.explanation.data(),
                          cell.explanation.data() + cell.explanation.size(),
                          false, wrapWidth)
          .y;
  ImGui::PopFont();
  const float spacing = ImGui::GetStyle().ItemSpacing.y * 4.0F;
  return std::ceil(paddingY * 2.0F + headingHeight + expressionHeight +
                   labelHeight * 2.0F + explanationHeight + spacing +
                   4.0F * scale);
}

void drawReview(FirstMoveUiState& ui, HuntSession& session,
                LayeredQuestionSession& guided, float scale) {
  const std::optional<std::size_t> reviewIndex = session.reviewRowIndex();
  if (!reviewIndex.has_value()) {
    return;
  }
  const HuntRowRecord& row = session.currentRun().rows[*reviewIndex];
  const HuntRowContent& content = huntPack().rows[*reviewIndex];
  const float footerReserve = 108.0F * scale;
  const float height = std::max(250.0F, ImGui::GetContentRegionAvail().y - footerReserve);
  const float width = ImGui::GetContentRegionAvail().x;
  const bool presentationChanged =
      std::fabs(ui.presentedReviewWidth - width) > 0.5F ||
      std::fabs(ui.presentedReviewHeight - height) > 0.5F ||
      std::fabs(ui.presentedReviewScale - scale) > 0.01F;
  if (presentationChanged) {
    ui.reviewRevealRequested = true;
    ui.presentedReviewWidth = width;
    ui.presentedReviewHeight = height;
    ui.presentedReviewScale = scale;
  }
  ImGui::PushStyleColor(ImGuiCol_ChildBg, kPanel);
  ImGui::PushStyleColor(ImGuiCol_Border, kAmber);
  if (ImGui::BeginChild("##hunt_review", {0.0F, height},
                        ImGuiChildFlags_Borders)) {
    ImGui::PushFont(nullptr, 26.0F * scale);
    ImGui::PushStyleColor(ImGuiCol_Text, kAmber);
    std::array<char, 64U> title{};
    static_cast<void>(std::snprintf(title.data(), title.size(),
                                    "Review row %zu", *reviewIndex + 1U));
    ImGui::TextUnformatted(title.data());
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::PushFont(nullptr, 17.0F * scale);
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    wrappedText("Your original choices are preserved. Read each classification, then release this row for zero points.");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Separator();

    const std::uint8_t initial = row.initialSelectionMask.value_or(0U);
    for (std::size_t column = 0U; column < kHuntCellsPerRow; ++column) {
      const HuntCellContent& cell = content.cells[column];
      const bool marked = (initial & (1U << column)) != 0U;
      const bool selected = ui.reviewExplanationIndex == column;
      const float cardWidth = ImGui::GetContentRegionAvail().x;
      const float cardHeight = reviewCardHeight(cell, cardWidth, scale);
      const float paddingX = 12.0F * scale;
      const float paddingY = 10.0F * scale;
      ImGui::PushID(static_cast<int>(column));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                          {paddingX, paddingY});
      ImGui::PushStyleColor(ImGuiCol_ChildBg, kPanelRaised);
      ImGui::PushStyleColor(
          ImGuiCol_Border,
          selected ? kGold : cell.qualifies ? kMint : kAmber);
      if (ImGui::BeginChild(
              "##review_cell", {0.0F, cardHeight},
              ImGuiChildFlags_Borders |
                  ImGuiChildFlags_AlwaysUseWindowPadding,
              ImGuiWindowFlags_NoScrollbar |
                  ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::PushFont(nullptr, 13.0F * scale);
        ImGui::PushStyleColor(ImGuiCol_Text, selected ? kGold : kMuted);
        std::array<char, 64U> explanationLabel{};
        static_cast<void>(std::snprintf(
            explanationLabel.data(), explanationLabel.size(),
            selected ? "EXPLANATION %zu / %zu  [SELECTED]"
                     : "EXPLANATION %zu / %zu",
            column + 1U, kHuntCellsPerRow));
        ImGui::TextUnformatted(explanationLabel.data());
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::PushFont(nullptr, 20.0F * scale);
        ImGui::PushStyleColor(ImGuiCol_Text, kWarmWhite);
        wrappedText(cell.expression);
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::PushFont(nullptr, 15.0F * scale);
        ImGui::PushStyleColor(ImGuiCol_Text, marked ? kMint : kMuted);
        ImGui::TextUnformatted(marked ? "YOUR CHOICE: SELECTED"
                                     : "YOUR CHOICE: NOT SELECTED");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, cell.qualifies ? kMint : kAmber);
        ImGui::TextUnformatted(cell.qualifies ? "QUALIFIES" : "DOES NOT QUALIFY");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, kWarmWhite);
        wrappedText(cell.explanation);
        ImGui::PopStyleColor();
        ImGui::PopFont();
      }
      ImGui::EndChild();
      if (selected && ui.reviewRevealRequested) {
        ImGui::SetScrollHereY(0.5F);
        ui.reviewRevealRequested = false;
      }
      ImGui::PopStyleColor(2);
      ImGui::PopStyleVar();
      ImGui::PopID();
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleColor(2);

  ImGui::PushFont(nullptr, 13.0F * scale);
  ImGui::PushStyleColor(ImGuiCol_Text, kGold);
  wrappedText("REVIEW: UP/DOWN or PGUP/PGDN select   HOME/END jump");
  ImGui::PopStyleColor();
  ImGui::PopFont();

  pushButtonPalette(kPanelRaised, {0.14F, 0.19F, 0.21F, 1.0F},
                    {0.18F, 0.23F, 0.25F, 1.0F});
  if (ImGui::Button("CLOSE  [Esc]", {160.0F * scale, 42.0F * scale})) {
    dispatchAndRemember(ui, session, guided,
                        {FirstMoveActionKind::HuntCloseReview});
  }
  popButtonPalette();
  ImGui::SameLine();
  pushButtonPalette(kAmberDark, {0.43F, 0.20F, 0.08F, 1.0F},
                    {0.52F, 0.24F, 0.09F, 1.0F});
  if (ImGui::Button("RELEASE ROW  [X]", {210.0F * scale, 42.0F * scale})) {
    dispatchAndRemember(ui, session, guided,
                        {FirstMoveActionKind::HuntReleaseReviewed});
  }
  popButtonPalette();
}

void drawFinished(FirstMoveUiState& ui, HuntSession& session,
                  LayeredQuestionSession& guided, float scale) {
  const HuntRunRecord& run = session.currentRun();
  const HuntRunSummary summary = summarizeRun(run);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, kPanel);
  ImGui::PushStyleColor(ImGuiCol_Border, kMint);
  if (ImGui::BeginChild("##hunt_finished", {0.0F, 0.0F},
                        ImGuiChildFlags_Borders)) {
    ImGui::PushFont(nullptr, 32.0F * scale);
    ImGui::PushStyleColor(ImGuiCol_Text, kMint);
    ImGui::TextUnformatted("HUNT COMPLETE");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::PushFont(nullptr, 18.0F * scale);
    ImGui::PushStyleColor(ImGuiCol_Text, kWarmWhite);
    wrappedText("This summary records first responses to this fixed pack. It is not a general mastery score.");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    if (run.priorExposure) {
      ImGui::PushFont(nullptr, 20.0F * scale);
      ImGui::PushStyleColor(ImGuiCol_Text, kGold);
      ImGui::TextUnformatted("Repeated pack: practice");
      ImGui::PopStyleColor();
      ImGui::PopFont();
    }

    std::array<char, 256U> observations{};
    static_cast<void>(std::snprintf(
        observations.data(), observations.size(),
        "Points: %u\nCorrect initial rows: %zu\nIncorrect initial rows: %zu\nExplained rows: %zu\nUntouched rows: %zu",
        run.score, summary.correctRows, summary.incorrectRows,
        summary.explainedRows, summary.untouchedRows));
    ImGui::PushFont(nullptr, 21.0F * scale);
    ImGui::TextUnformatted(observations.data());
    ImGui::PopFont();
    ImGui::Dummy({0.0F, 12.0F * scale});
    pushButtonPalette(kMintDark, {0.11F, 0.39F, 0.31F, 1.0F},
                      {0.14F, 0.46F, 0.36F, 1.0F});
    if (ImGui::Button("NEW RUN  [R]", {220.0F * scale, 50.0F * scale})) {
      dispatchAndRemember(ui, session, guided,
                          {FirstMoveActionKind::HuntRestart});
    }
    popButtonPalette();
  }
  ImGui::EndChild();
  ImGui::PopStyleColor(2);
}

void drawFooter(FirstMoveUiState& ui, HuntSession& session,
                LayeredQuestionSession& guided, float scale) {
  const HuntRunSummary summary = summarizeRun(session.currentRun());
  pushButtonPalette(kGoldDark, {0.39F, 0.32F, 0.12F, 1.0F},
                    {0.47F, 0.38F, 0.14F, 1.0F});
  if (ImGui::Button("CLEAR READY  [C]", {205.0F * scale, 44.0F * scale})) {
    dispatchAndRemember(ui, session, guided,
                        {FirstMoveActionKind::HuntClearReady});
  }
  popButtonPalette();
  ImGui::SameLine();
  ImGui::PushFont(nullptr, 15.0F * scale);
  ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
  wrappedText("Banking: 1 / 2 / 3 ready rows earn 100 / 240 / 420 points; each additional row adds 100. Maximum single clear: 720. No points until clear.");
  ImGui::PopStyleColor();
  ImGui::PopFont();

  ImGui::PushFont(nullptr, 14.0F * scale);
  ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
  wrappedText("ARROWS navigate   SPACE select   ENTER check row   C clear   E explain   ESC close review   X release row   R new run after finish");
  ImGui::PopStyleColor();
  if (!ui.lastActionAccepted) {
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, kAmber);
    const std::string_view feedback = playerFeedbackFor(ui.lastActionReason);
    ImGui::TextUnformatted(feedback.data(), feedback.data() + feedback.size());
    ImGui::PopStyleColor();
  } else if (summary.readyRows > 0U) {
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, kGold);
    ImGui::TextUnformatted("Ready rows are waiting to be cleared.");
    ImGui::PopStyleColor();
  }
  ImGui::PopFont();
}

}  // namespace

std::string_view firstMoveModeName(FirstMoveMode mode) noexcept {
  switch (mode) {
    case FirstMoveMode::GuidedQuestion: return "guided";
    case FirstMoveMode::QuickHunt: return "hunt";
  }
  return "unknown";
}

FirstMoveDispatchResult dispatchFirstMoveAction(
    FirstMoveUiState& ui,
    HuntSession& hunt,
    LayeredQuestionSession& guided,
    const FirstMoveAction& action) {
  const auto finish = [&ui](bool acceptedValue, bool changed,
                            std::string_view reason) {
    ui.lastActionAccepted = acceptedValue;
    ui.lastActionReason = reason;
    return FirstMoveDispatchResult{acceptedValue, changed, reason};
  };
  const auto huntCommand = [&](Command command) {
    if (ui.mode != FirstMoveMode::QuickHunt) {
      return finish(false, false, "hunt_mode_inactive");
    }
    const DispatchResult result = hunt.dispatch(command);
    reconcileReviewFocus(ui, hunt);
    return finish(result.accepted, result.changed, result.reason);
  };
  const auto guidedCommand = [&](LayeredQuestionCommand command) {
    if (ui.mode != FirstMoveMode::GuidedQuestion) {
      return finish(false, false, "guided_mode_inactive");
    }
    const LayeredQuestionDispatchResult result = guided.dispatch(command);
    if (result.changed) {
      ui.guidedRevealRequested = true;
    }
    return finish(result.accepted, result.changed, result.reason);
  };

  switch (action.kind) {
    case FirstMoveActionKind::SwitchToGuided:
      if (hunt.reviewRowIndex().has_value()) {
        return finish(false, false, "review_modal_blocks_mode_switch");
      }
      if (ui.mode == FirstMoveMode::GuidedQuestion) {
        return finish(true, false, "mode_unchanged");
      }
      ui.mode = FirstMoveMode::GuidedQuestion;
      ui.guidedRevealRequested = true;
      return finish(true, true, "guided_mode_opened");
    case FirstMoveActionKind::SwitchToHunt:
      if (ui.mode == FirstMoveMode::QuickHunt) {
        return finish(true, false, "mode_unchanged");
      }
      ui.mode = FirstMoveMode::QuickHunt;
      return finish(true, true, "hunt_mode_opened");
    case FirstMoveActionKind::HuntSelectCell:
      return huntCommand(Command::selectCell(action.firstIndex,
                                             action.secondIndex));
    case FirstMoveActionKind::HuntMoveLeft:
      return huntCommand({CommandKind::MoveLeft});
    case FirstMoveActionKind::HuntMoveRight:
      return huntCommand({CommandKind::MoveRight});
    case FirstMoveActionKind::HuntMoveUp:
      return huntCommand({CommandKind::MoveUp});
    case FirstMoveActionKind::HuntMoveDown:
      return huntCommand({CommandKind::MoveDown});
    case FirstMoveActionKind::HuntToggleMark:
      return huntCommand({CommandKind::ToggleMark});
    case FirstMoveActionKind::HuntCommitRow:
      return huntCommand({CommandKind::CommitRow});
    case FirstMoveActionKind::HuntClearReady:
      return huntCommand({CommandKind::ClearReady});
    case FirstMoveActionKind::HuntOpenReview:
      return huntCommand({CommandKind::OpenReview});
    case FirstMoveActionKind::HuntCloseReview:
      return huntCommand({CommandKind::CloseReview});
    case FirstMoveActionKind::HuntReleaseReviewed:
      return huntCommand({CommandKind::ReleaseReviewed});
    case FirstMoveActionKind::HuntRestart:
      return huntCommand({CommandKind::Restart});
    case FirstMoveActionKind::ReviewPrevious:
    case FirstMoveActionKind::ReviewNext:
    case FirstMoveActionKind::ReviewFirst:
    case FirstMoveActionKind::ReviewLast: {
      if (ui.mode != FirstMoveMode::QuickHunt ||
          !hunt.reviewRowIndex().has_value()) {
        return finish(false, false, "review_not_open");
      }
      std::size_t next = ui.reviewExplanationIndex;
      if (action.kind == FirstMoveActionKind::ReviewPrevious && next > 0U) {
        --next;
      } else if (action.kind == FirstMoveActionKind::ReviewNext) {
        next = std::min(next + 1U, kHuntCellsPerRow - 1U);
      } else if (action.kind == FirstMoveActionKind::ReviewFirst) {
        next = 0U;
      } else if (action.kind == FirstMoveActionKind::ReviewLast) {
        next = kHuntCellsPerRow - 1U;
      }
      const bool changed = next != ui.reviewExplanationIndex;
      selectReviewExplanation(ui, hunt, next);
      return finish(true, changed, "review_navigation");
    }
    case FirstMoveActionKind::GuidedOpen:
      ui.guidedOptionFocus = guided.currentRun().steps[
          guided.currentRun().currentStep].selectedOption.value_or(0U);
      return guidedCommand({LayeredQuestionCommandKind::OpenQuestion});
    case FirstMoveActionKind::GuidedSelectOption:
      ui.guidedOptionFocus =
          std::min(action.firstIndex, kLayeredQuestionOptionCount - 1U);
      return guidedCommand(
          LayeredQuestionCommand::selectOption(action.firstIndex));
    case FirstMoveActionKind::GuidedMovePreviousOption:
      if (ui.guidedOptionFocus > 0U) {
        --ui.guidedOptionFocus;
      }
      return guidedCommand(
          LayeredQuestionCommand::selectOption(ui.guidedOptionFocus));
    case FirstMoveActionKind::GuidedMoveNextOption:
      ui.guidedOptionFocus = std::min(ui.guidedOptionFocus + 1U,
                                     kLayeredQuestionOptionCount - 1U);
      return guidedCommand(
          LayeredQuestionCommand::selectOption(ui.guidedOptionFocus));
    case FirstMoveActionKind::GuidedCheck:
      return guidedCommand({LayeredQuestionCommandKind::CheckAnswer});
    case FirstMoveActionKind::GuidedTryAgain:
      return guidedCommand({LayeredQuestionCommandKind::TryAgain});
    case FirstMoveActionKind::GuidedShowAnswer:
      return guidedCommand({LayeredQuestionCommandKind::ShowAnswer});
    case FirstMoveActionKind::GuidedContinue: {
      const FirstMoveDispatchResult result =
          guidedCommand({LayeredQuestionCommandKind::Continue});
      if (result.accepted) {
        ui.guidedOptionFocus = 0U;
      }
      return result;
    }
    case FirstMoveActionKind::GuidedBackToGrid:
      return guidedCommand({LayeredQuestionCommandKind::BackToGrid});
    case FirstMoveActionKind::GuidedRestart:
      ui.guidedOptionFocus = 0U;
      return guidedCommand({LayeredQuestionCommandKind::RestartQuestion});
  }
  return finish(false, false, "action_unknown");
}

void queueFirstMoveInput(FirstMoveUiState& ui,
                         const HuntSession& hunt,
                         const LayeredQuestionSession& guided,
                         const SDL_Event& event) noexcept {
  if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
    ui.pendingActionCount = 0U;
    return;
  }
  if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
    return;
  }
  std::optional<FirstMoveAction> action;
  if (event.key.key == SDLK_G) {
    action = FirstMoveAction{FirstMoveActionKind::SwitchToGuided};
  } else if (event.key.key == SDLK_H) {
    action = FirstMoveAction{FirstMoveActionKind::SwitchToHunt};
  } else if (ui.mode == FirstMoveMode::QuickHunt) {
    const bool reviewing = hunt.reviewRowIndex().has_value();
    switch (event.key.key) {
      case SDLK_LEFT: action = FirstMoveAction{FirstMoveActionKind::HuntMoveLeft}; break;
      case SDLK_RIGHT: action = FirstMoveAction{FirstMoveActionKind::HuntMoveRight}; break;
      case SDLK_UP: action = FirstMoveAction{reviewing ? FirstMoveActionKind::ReviewPrevious : FirstMoveActionKind::HuntMoveUp}; break;
      case SDLK_DOWN: action = FirstMoveAction{reviewing ? FirstMoveActionKind::ReviewNext : FirstMoveActionKind::HuntMoveDown}; break;
      case SDLK_SPACE: action = FirstMoveAction{FirstMoveActionKind::HuntToggleMark}; break;
      case SDLK_RETURN: action = FirstMoveAction{FirstMoveActionKind::HuntCommitRow}; break;
      case SDLK_C: action = FirstMoveAction{FirstMoveActionKind::HuntClearReady}; break;
      case SDLK_E: action = FirstMoveAction{FirstMoveActionKind::HuntOpenReview}; break;
      case SDLK_ESCAPE: action = FirstMoveAction{FirstMoveActionKind::HuntCloseReview}; break;
      case SDLK_X: action = FirstMoveAction{FirstMoveActionKind::HuntReleaseReviewed}; break;
      case SDLK_R: action = FirstMoveAction{FirstMoveActionKind::HuntRestart}; break;
      case SDLK_PAGEUP: action = FirstMoveAction{FirstMoveActionKind::ReviewPrevious}; break;
      case SDLK_PAGEDOWN: action = FirstMoveAction{FirstMoveActionKind::ReviewNext}; break;
      case SDLK_HOME: action = FirstMoveAction{FirstMoveActionKind::ReviewFirst}; break;
      case SDLK_END: action = FirstMoveAction{FirstMoveActionKind::ReviewLast}; break;
      default: break;
    }
  } else {
    const LayeredQuestionRunRecord& run = guided.currentRun();
    if (event.key.key == SDLK_ESCAPE) {
      action = FirstMoveAction{FirstMoveActionKind::GuidedBackToGrid};
    } else if (run.phase == LayeredQuestionPhase::Grid &&
               event.key.key == SDLK_RETURN) {
      action = FirstMoveAction{FirstMoveActionKind::GuidedOpen};
    } else if (run.phase == LayeredQuestionPhase::Answering) {
      const LayeredQuestionStepRecord& step = run.steps[run.currentStep];
      if (event.key.key == SDLK_UP) {
        action = FirstMoveAction{FirstMoveActionKind::GuidedMovePreviousOption};
      } else if (event.key.key == SDLK_DOWN) {
        action = FirstMoveAction{FirstMoveActionKind::GuidedMoveNextOption};
      } else if (event.key.key == SDLK_RETURN) {
        action = FirstMoveAction{layeredQuestionStepResolved(step)
                                     ? FirstMoveActionKind::GuidedContinue
                                     : FirstMoveActionKind::GuidedCheck};
      } else if (event.key.key == SDLK_T) {
        action = FirstMoveAction{FirstMoveActionKind::GuidedTryAgain};
      } else if (event.key.key == SDLK_S) {
        action = FirstMoveAction{FirstMoveActionKind::GuidedShowAnswer};
      }
    } else if (run.phase == LayeredQuestionPhase::Complete &&
               event.key.key == SDLK_R) {
      action = FirstMoveAction{FirstMoveActionKind::GuidedRestart};
    }
  }
  if (!action.has_value()) {
    return;
  }
  if (ui.pendingActionCount >= ui.pendingActions.size()) {
    ++ui.droppedCommandCount;
    return;
  }
  ui.pendingActions[ui.pendingActionCount++] = *action;
}

std::size_t drainFirstMoveInput(FirstMoveUiState& ui,
                                HuntSession& hunt,
                                LayeredQuestionSession& guided,
                                bool suppressShortcuts) {
  const std::size_t count = ui.pendingActionCount;
  if (suppressShortcuts) {
    ui.pendingActionCount = 0U;
    return 0U;
  }
  for (std::size_t index = 0U; index < count; ++index) {
    static_cast<void>(dispatchFirstMoveAction(
        ui, hunt, guided, ui.pendingActions[index]));
  }
  ui.pendingActionCount = 0U;
  reconcileReviewFocus(ui, hunt);
  return count;
}

void renderFirstMoveUiFrame(FirstMoveUiState& ui, HuntSession& hunt,
                            LayeredQuestionSession& guided) {
  reconcileReviewFocus(ui, hunt);
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                      {22.0F * ui.textScale, 16.0F * ui.textScale});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0F * ui.textScale);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0F * ui.textScale);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, kInk);
  ImGui::PushStyleColor(ImGuiCol_Text, kWarmWhite);
  ImGui::PushStyleColor(ImGuiCol_Border, kBorder);
  constexpr ImGuiWindowFlags windowFlags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNav;
  if (ImGui::Begin("First Move###first_move_root", nullptr, windowFlags)) {
    ImGui::PushFont(nullptr, 34.0F * ui.textScale);
    ImGui::PushStyleColor(ImGuiCol_Text, kMint);
    ImGui::TextUnformatted(ui.mode == FirstMoveMode::GuidedQuestion
                              ? "FIRST MOVE / GUIDED QUESTION"
                              : "FIRST MOVE / QUICK HUNT");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    if (ImGui::GetContentRegionAvail().x > 560.0F * ui.textScale) {
      ImGui::SameLine();
    }
    ImGui::PushFont(nullptr, 15.0F * ui.textScale);
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    wrappedText(ui.mode == FirstMoveMode::GuidedQuestion
                    ? "Understand one equation from its name to its solution."
                    : "Classify the row. Check it. Bank what is ready.");
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::PushFont(nullptr, 13.0F * ui.textScale);
    ImGui::TextUnformatted("TEXT");
    ImGui::SameLine();
    bool scaleActive = drawScaleChoice(ui, "100%##scale_100", 1.0F);
    ImGui::SameLine();
    scaleActive = drawScaleChoice(ui, "125%##scale_125", 1.25F) || scaleActive;
    ImGui::SameLine();
    scaleActive = drawScaleChoice(ui, "150%##scale_150", 1.5F) || scaleActive;
    if (ui.mode == FirstMoveMode::QuickHunt &&
        hunt.currentRun().priorExposure) {
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Text, kGold);
      ImGui::TextUnformatted("Repeated pack: practice");
      ImGui::PopStyleColor();
    }
    ImGui::PopFont();
    ui.scaleControlActive = scaleActive;
    static_cast<void>(drainFirstMoveInput(
        ui, hunt, guided, io.WantTextInput || ui.scaleControlActive));

    if (ImGui::Button("GUIDED QUESTION  [G]")) {
      static_cast<void>(dispatchFirstMoveAction(
          ui, hunt, guided, {FirstMoveActionKind::SwitchToGuided}));
    }
    ImGui::SameLine();
    if (ImGui::Button("QUICK HUNT  [H]")) {
      static_cast<void>(dispatchFirstMoveAction(
          ui, hunt, guided, {FirstMoveActionKind::SwitchToHunt}));
    }

    if (ui.mode == FirstMoveMode::GuidedQuestion) {
      drawLayeredQuestionUi(ui, hunt, guided, ui.textScale);
    } else {
      drawRuleCard(ui.textScale);
      drawStats(hunt, ui.textScale);
      if (hunt.reviewRowIndex().has_value()) {
        drawReview(ui, hunt, guided, ui.textScale);
      } else if (hunt.finished()) {
        drawFinished(ui, hunt, guided, ui.textScale);
      } else {
        drawBoard(ui, hunt, guided, ui.textScale);
        drawFooter(ui, hunt, guided, ui.textScale);
      }
    }
  }
  ImGui::End();
  ImGui::PopStyleColor(3);
  ImGui::PopStyleVar(4);
  ImGui::Render();
}

}  // namespace iggy3d::first_move

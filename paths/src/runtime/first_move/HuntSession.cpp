#include "runtime/first_move/HuntSession.hpp"

#include <algorithm>

namespace iggy3d::first_move {
namespace {

constexpr HuntPack kPack{
    kHuntPackId,
    kHuntPackVersion,
    {{
        {{{
              {"linearity_ab_known_x_v1_v1_r1_c1", "2a + b = 7", true,
               "a and b occur to the first power with constant coefficients."},
              {"linearity_ab_known_x_v1_v1_r1_c2", "a^2 + b = 7", false,
               "Squaring the unknown a violates the stated linear form."},
              {"linearity_ab_known_x_v1_v1_r1_c3", "a x^2 + b = 1", true,
               "x is known, so x squared is a coefficient of a."},
              {"linearity_ab_known_x_v1_v1_r1_c4", "ab = 4", false,
               "Two unknowns multiply each other."},
          }},
         5U},
        {{{
              {"linearity_ab_known_x_v1_v1_r2_c1", "sin(a) + b = 0", false,
               "The unknown a is inside a nonlinear function."},
              {"linearity_ab_known_x_v1_v1_r2_c2",
               "a cos(x) + b sin(x) = 2", true,
               "The known values cos(x) and sin(x) are coefficients."},
              {"linearity_ab_known_x_v1_v1_r2_c3", "a - 3b = x", true,
               "Both unknowns are first degree; x is a known right-hand side."},
              {"linearity_ab_known_x_v1_v1_r2_c4", "0a + b = 3", true,
               "A zero coefficient of a is permitted."},
          }},
         14U},
        {{{
              {"linearity_ab_known_x_v1_v1_r3_c1", "a + b^2 = x", false,
               "Squaring the unknown b violates the linear form."},
              {"linearity_ab_known_x_v1_v1_r3_c2", "abs(a) + b = 2", false,
               "Absolute value of an unknown is not a fixed coefficient times that unknown."},
              {"linearity_ab_known_x_v1_v1_r3_c3",
               "x a + (x + 1)b = 5", true,
               "Both coefficients depend only on the known x."},
              {"linearity_ab_known_x_v1_v1_r3_c4", "cos(b) = a", false,
               "The unknown b appears inside cosine."},
          }},
         4U},
        {{{
              {"linearity_ab_known_x_v1_v1_r4_c1", "a/2 + b/3 = 1", true,
               "Division by the nonzero constants 2 and 3 gives fixed coefficients."},
              {"linearity_ab_known_x_v1_v1_r4_c2", "a^2 + b^2 = 1", false,
               "Both unknowns are squared."},
              {"linearity_ab_known_x_v1_v1_r4_c3", "a = b^3", false,
               "Cubing the unknown b violates the linear form."},
              {"linearity_ab_known_x_v1_v1_r4_c4", "a + b = sin(x)", true,
               "The right-hand side depends only on the known x."},
          }},
         9U},
        {{{
              {"linearity_ab_known_x_v1_v1_r5_c1",
               "(x^2 + 1)a - b = 0", true,
               "x squared plus one is a known coefficient of a."},
              {"linearity_ab_known_x_v1_v1_r5_c2", "a = 7", true,
               "The coefficient of b can be zero."},
              {"linearity_ab_known_x_v1_v1_r5_c3", "a + sin(b) = 1", false,
               "The unknown b is inside a nonlinear function."},
              {"linearity_ab_known_x_v1_v1_r5_c4", "a + exp(x)b = x", true,
               "exp(x) is a known coefficient of b."},
          }},
         11U},
        {{{
              {"linearity_ab_known_x_v1_v1_r6_c1", "(a + b)^2 = 9", false,
               "Squaring a sum of unknowns introduces squared terms and a product."},
              {"linearity_ab_known_x_v1_v1_r6_c2", "3b = x^2", true,
               "b is first degree and x squared is known."},
              {"linearity_ab_known_x_v1_v1_r6_c3", "exp(a) + b = 3", false,
               "The unknown a is inside the exponential function."},
              {"linearity_ab_known_x_v1_v1_r6_c4", "a + b cos(x) = 2x", true,
               "cos(x) is a known coefficient and 2x is known."},
          }},
         10U},
    }}};

constexpr std::array<std::uint32_t, kHuntRowCount + 1U> kClearScores{
    0U, 100U, 240U, 420U, 520U, 620U, 720U};

[[nodiscard]] constexpr bool isVisible(RowState state) noexcept {
  return state == RowState::Editing || state == RowState::Ready ||
         state == RowState::NeedsReview;
}

[[nodiscard]] DispatchResult accepted(bool changed,
                                      std::string_view reason) noexcept {
  return {true, changed, reason};
}

[[nodiscard]] DispatchResult rejected(std::string_view reason) noexcept {
  return {false, false, reason};
}

}  // namespace

const HuntPack& huntPack() noexcept {
  return kPack;
}

std::string_view rowStateName(RowState state) noexcept {
  switch (state) {
    case RowState::Editing: return "editing";
    case RowState::Ready: return "ready";
    case RowState::NeedsReview: return "needs_review";
    case RowState::Cleared: return "cleared";
    case RowState::Released: return "released";
  }
  return "unknown";
}

HuntRunSummary summarizeRun(const HuntRunRecord& run) noexcept {
  HuntRunSummary summary;
  summary.untouchedRows = 0U;
  summary.finished = true;
  for (const HuntRowRecord& row : run.rows) {
    if (row.state == RowState::Ready) {
      ++summary.readyRows;
    }
    if (row.initialCorrect.has_value()) {
      if (*row.initialCorrect) {
        ++summary.correctRows;
      } else {
        ++summary.incorrectRows;
      }
    } else {
      ++summary.untouchedRows;
    }
    if (row.explained) {
      ++summary.explainedRows;
      summary.assisted = true;
    }
    if (row.state != RowState::Cleared && row.state != RowState::Released) {
      summary.finished = false;
    }
  }
  return summary;
}

HuntSession::HuntSession() {
  beginRun(1U, false);
}

const HuntRunRecord& HuntSession::currentRun() const noexcept {
  return current_;
}

const std::vector<HuntRunRecord>& HuntSession::archivedRuns() const noexcept {
  return archived_;
}

std::optional<std::size_t> HuntSession::activeRowIndex() const noexcept {
  return activeRow_;
}

std::size_t HuntSession::activeColumn() const noexcept {
  return activeColumn_;
}

std::optional<std::size_t> HuntSession::reviewRowIndex() const noexcept {
  return reviewRow_;
}

std::vector<std::size_t> HuntSession::visibleRowIndices() const {
  std::vector<std::size_t> rows;
  rows.reserve(kHuntRowCount);
  for (std::size_t index = 0U; index < current_.rows.size(); ++index) {
    if (isVisible(current_.rows[index].state)) {
      rows.push_back(index);
    }
  }
  return rows;
}

bool HuntSession::finished() const noexcept {
  return summarizeRun(current_).finished;
}

void HuntSession::beginRun(std::uint32_t runNumber, bool priorExposure) {
  current_ = {};
  current_.runNumber = runNumber;
  current_.priorExposure = priorExposure;
  for (std::size_t index = 0U; index < current_.rows.size(); ++index) {
    current_.rows[index].rowIndex = index;
  }
  activeRow_ = 0U;
  activeColumn_ = 0U;
  reviewRow_.reset();
}

void HuntSession::repairActiveRow(std::size_t preferredRow) {
  const std::vector<std::size_t> visible = visibleRowIndices();
  if (visible.empty()) {
    activeRow_.reset();
    activeColumn_ = 0U;
    return;
  }
  if (activeRow_.has_value() && isVisible(current_.rows[*activeRow_].state)) {
    return;
  }
  const auto next = std::lower_bound(visible.begin(), visible.end(), preferredRow);
  activeRow_ = next == visible.end() ? visible.back() : *next;
  activeColumn_ = std::min(activeColumn_, kHuntCellsPerRow - 1U);
}

DispatchResult HuntSession::moveHorizontal(int direction) {
  if (!activeRow_.has_value()) {
    return rejected("no_visible_row");
  }
  const std::size_t previous = activeColumn_;
  if (direction < 0 && activeColumn_ > 0U) {
    --activeColumn_;
  } else if (direction > 0 && activeColumn_ + 1U < kHuntCellsPerRow) {
    ++activeColumn_;
  }
  return accepted(activeColumn_ != previous,
                  activeColumn_ == previous ? "at_navigation_edge" : "moved");
}

DispatchResult HuntSession::moveVertical(int direction) {
  if (!activeRow_.has_value()) {
    return rejected("no_visible_row");
  }
  const std::vector<std::size_t> visible = visibleRowIndices();
  const auto found = std::find(visible.begin(), visible.end(), *activeRow_);
  if (found == visible.end()) {
    repairActiveRow(*activeRow_);
    return accepted(true, "focus_repaired");
  }
  const std::size_t position =
      static_cast<std::size_t>(std::distance(visible.begin(), found));
  std::size_t nextPosition = position;
  if (direction < 0 && position > 0U) {
    nextPosition = position - 1U;
  } else if (direction > 0 && position + 1U < visible.size()) {
    nextPosition = position + 1U;
  }
  activeRow_ = visible[nextPosition];
  return accepted(nextPosition != position,
                  nextPosition == position ? "at_navigation_edge" : "moved");
}

DispatchResult HuntSession::dispatch(const Command& command) {
  if (reviewRow_.has_value() && command.kind != CommandKind::CloseReview &&
      command.kind != CommandKind::ReleaseReviewed) {
    return rejected("review_modal_blocks_command");
  }

  switch (command.kind) {
    case CommandKind::SelectCell: {
      if (command.row >= kHuntRowCount || command.column >= kHuntCellsPerRow) {
        return rejected("cell_coordinates_invalid");
      }
      if (!isVisible(current_.rows[command.row].state)) {
        return rejected("row_not_visible");
      }
      const bool changed = activeRow_ != command.row || activeColumn_ != command.column;
      activeRow_ = command.row;
      activeColumn_ = command.column;
      return accepted(changed, changed ? "cell_selected" : "selection_unchanged");
    }
    case CommandKind::MoveLeft: return moveHorizontal(-1);
    case CommandKind::MoveRight: return moveHorizontal(1);
    case CommandKind::MoveUp: return moveVertical(-1);
    case CommandKind::MoveDown: return moveVertical(1);
    case CommandKind::ToggleMark: {
      if (!activeRow_.has_value()) {
        return rejected("no_visible_row");
      }
      HuntRowRecord& row = current_.rows[*activeRow_];
      if (row.state != RowState::Editing) {
        return rejected("row_already_assessed");
      }
      const auto bit = static_cast<std::uint8_t>(1U << activeColumn_);
      row.selectedMask = static_cast<std::uint8_t>(row.selectedMask ^ bit);
      return accepted(true, "mark_toggled");
    }
    case CommandKind::CommitRow: {
      if (!activeRow_.has_value()) {
        return rejected("no_visible_row");
      }
      HuntRowRecord& row = current_.rows[*activeRow_];
      if (row.state != RowState::Editing || row.initialSelectionMask.has_value()) {
        return rejected("row_already_assessed");
      }
      const bool correct =
          row.selectedMask == huntPack().rows[*activeRow_].answerMask;
      row.initialSelectionMask = row.selectedMask;
      row.initialCorrect = correct;
      row.state = correct ? RowState::Ready : RowState::NeedsReview;
      return accepted(true, correct ? "row_ready" : "row_needs_review");
    }
    case CommandKind::ClearReady: {
      std::size_t readyCount = 0U;
      for (const HuntRowRecord& row : current_.rows) {
        readyCount += row.state == RowState::Ready ? 1U : 0U;
      }
      if (readyCount == 0U) {
        return accepted(false, "no_ready_rows");
      }
      const std::size_t preferred = activeRow_.value_or(0U);
      for (HuntRowRecord& row : current_.rows) {
        if (row.state == RowState::Ready) {
          row.state = RowState::Cleared;
        }
      }
      current_.score += kClearScores[readyCount];
      repairActiveRow(preferred);
      return accepted(true, "ready_rows_cleared");
    }
    case CommandKind::OpenReview: {
      if (!activeRow_.has_value() ||
          current_.rows[*activeRow_].state != RowState::NeedsReview) {
        return rejected("row_does_not_need_review");
      }
      reviewRow_ = *activeRow_;
      current_.rows[*activeRow_].explained = true;
      return accepted(true, "review_opened");
    }
    case CommandKind::CloseReview: {
      if (!reviewRow_.has_value()) {
        return rejected("review_not_open");
      }
      reviewRow_.reset();
      return accepted(true, "review_closed");
    }
    case CommandKind::ReleaseReviewed: {
      if (!reviewRow_.has_value()) {
        return rejected("review_not_open");
      }
      const std::size_t releasedRow = *reviewRow_;
      HuntRowRecord& row = current_.rows[releasedRow];
      if (row.state != RowState::NeedsReview || !row.explained) {
        return rejected("row_not_explained");
      }
      row.state = RowState::Released;
      reviewRow_.reset();
      repairActiveRow(releasedRow);
      return accepted(true, "reviewed_row_released");
    }
    case CommandKind::Restart: {
      if (!finished()) {
        return rejected("run_not_finished");
      }
      const std::uint32_t nextRun = current_.runNumber + 1U;
      archived_.push_back(current_);
      beginRun(nextRun, true);
      return accepted(true, "run_restarted");
    }
  }
  return rejected("command_kind_invalid");
}

}  // namespace iggy3d::first_move

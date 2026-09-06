#include "runtime/first_move/HuntSession.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
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

fm::DispatchResult select(fm::HuntSession& session,
                          std::size_t row,
                          std::size_t column) {
  return session.dispatch(fm::Command::selectCell(row, column));
}

void chooseMask(fm::HuntSession& session,
                std::size_t row,
                std::uint8_t mask) {
  static_cast<void>(select(session, row, 0U));
  for (std::size_t column = 0U; column < fm::kHuntCellsPerRow; ++column) {
    if ((mask & (1U << column)) != 0U) {
      static_cast<void>(select(session, row, column));
      static_cast<void>(session.dispatch({fm::CommandKind::ToggleMark}));
    }
  }
}

fm::DispatchResult commitMask(fm::HuntSession& session,
                              std::size_t row,
                              std::uint8_t mask) {
  chooseMask(session, row, mask);
  return session.dispatch({fm::CommandKind::CommitRow});
}

void testReviewedPack() {
  constexpr std::array<std::uint8_t, fm::kHuntRowCount> expectedMasks{
      5U, 14U, 4U, 9U, 11U, 10U};
  constexpr std::array<std::string_view,
                       fm::kHuntRowCount * fm::kHuntCellsPerRow>
      expectedExpressions{
          "2a + b = 7", "a^2 + b = 7", "a x^2 + b = 1", "ab = 4",
          "sin(a) + b = 0", "a cos(x) + b sin(x) = 2", "a - 3b = x",
          "0a + b = 3", "a + b^2 = x", "abs(a) + b = 2",
          "x a + (x + 1)b = 5", "cos(b) = a", "a/2 + b/3 = 1",
          "a^2 + b^2 = 1", "a = b^3", "a + b = sin(x)",
          "(x^2 + 1)a - b = 0", "a = 7", "a + sin(b) = 1",
          "a + exp(x)b = x", "(a + b)^2 = 9", "3b = x^2",
          "exp(a) + b = 3", "a + b cos(x) = 2x"};

  const fm::HuntPack& pack = fm::huntPack();
  expect(pack.id == fm::kHuntPackId, "pack id is pinned");
  expect(pack.version == 1U, "pack version is pinned");
  std::size_t expressionIndex = 0U;
  for (std::size_t row = 0U; row < fm::kHuntRowCount; ++row) {
    std::uint8_t contentMask = 0U;
    for (std::size_t column = 0U; column < fm::kHuntCellsPerRow;
         ++column) {
      const fm::HuntCellContent& cell = pack.rows[row].cells[column];
      expect(cell.expression == expectedExpressions[expressionIndex++],
             "reviewed expression is unchanged");
      expect(!cell.stableId.empty(), "cell has a stable id");
      expect(!cell.explanation.empty(), "cell has an explanation");
      if (cell.qualifies) {
        contentMask = static_cast<std::uint8_t>(contentMask | (1U << column));
      }
    }
    expect(pack.rows[row].answerMask == expectedMasks[row],
           "stored key matches independent reviewed mask");
    expect(contentMask == expectedMasks[row],
           "cell classifications match independent reviewed mask");
  }

  fm::HuntSession session;
  for (std::size_t row = 0U; row < fm::kHuntRowCount; ++row) {
    const fm::DispatchResult result = commitMask(session, row, expectedMasks[row]);
    expect(result.accepted && result.reason == "row_ready",
           "exact mixed judgments make row ready");
    expect(session.currentRun().rows[row].initialSelectionMask ==
               expectedMasks[row],
           "initial selected and unselected judgments are retained");
  }
  expect(session.currentRun().score == 0U,
         "committing all rows awards no points before clear");
  static_cast<void>(session.dispatch({fm::CommandKind::ClearReady}));
  expect(session.currentRun().score == 720U,
         "one six-row bank uses the reviewed 720 point award");
}

void testWrongJudgmentsAreImmutable() {
  fm::HuntSession session;
  const fm::DispatchResult omission = commitMask(session, 0U, 1U);
  expect(omission.accepted && omission.reason == "row_needs_review",
         "omitting one qualifying choice needs review");
  const fm::HuntRowRecord before = session.currentRun().rows[0U];
  const fm::DispatchResult toggle =
      session.dispatch({fm::CommandKind::ToggleMark});
  const fm::DispatchResult recommit =
      session.dispatch({fm::CommandKind::CommitRow});
  expect(!toggle.accepted && !recommit.accepted,
         "assessed row rejects later edits and commits");
  const fm::HuntRowRecord& after = session.currentRun().rows[0U];
  expect(after.selectedMask == before.selectedMask &&
             after.initialSelectionMask == before.initialSelectionMask &&
             after.initialCorrect == before.initialCorrect,
         "rejected repair cannot alter first-response evidence");

  const fm::DispatchResult inclusion = commitMask(session, 1U, 15U);
  expect(inclusion.accepted && inclusion.reason == "row_needs_review",
         "including one nonqualifying choice needs review");
  expect(session.currentRun().rows[1U].initialCorrect == false,
         "wrong inclusion is retained as an incorrect initial response");
}

void testBankingScheduleAndCompaction() {
  constexpr std::array<std::uint8_t, fm::kHuntRowCount> masks{
      5U, 14U, 4U, 9U, 11U, 10U};
  constexpr std::array<std::uint32_t, fm::kHuntRowCount> scores{
      100U, 240U, 420U, 520U, 620U, 720U};
  for (std::size_t count = 1U; count <= fm::kHuntRowCount; ++count) {
    fm::HuntSession session;
    for (std::size_t row = 0U; row < count; ++row) {
      static_cast<void>(commitMask(session, row, masks[row]));
    }
    expect(session.currentRun().score == 0U,
           "ready rows remain visible and unbanked");
    expect(session.visibleRowIndices().size() == fm::kHuntRowCount,
           "ready rows still occupy board space");
    const fm::DispatchResult clear =
        session.dispatch({fm::CommandKind::ClearReady});
    expect(clear.accepted && clear.changed, "clear consumes ready rows atomically");
    expect(session.currentRun().score == scores[count - 1U],
           "clear uses the exact batch scoring schedule");
    expect(session.visibleRowIndices().size() == fm::kHuntRowCount - count,
           "cleared rows compact out of the visible board");
    const fm::DispatchResult repeated =
        session.dispatch({fm::CommandKind::ClearReady});
    expect(repeated.accepted && !repeated.changed &&
               session.currentRun().score == scores[count - 1U],
           "empty repeated clear cannot farm points");
  }
}

void testReviewModalAndRelease() {
  fm::HuntSession session;
  static_cast<void>(commitMask(session, 0U, 5U));
  static_cast<void>(commitMask(session, 1U, 0U));
  const fm::DispatchResult earlyRelease =
      session.dispatch({fm::CommandKind::ReleaseReviewed});
  expect(!earlyRelease.accepted,
         "release requires an open explanation for the reviewed row");
  const fm::DispatchResult open =
      session.dispatch({fm::CommandKind::OpenReview});
  expect(open.accepted && session.currentRun().rows[1U].explained,
         "first review opening records assistance");
  expect(!select(session, 2U, 0U).accepted,
         "review modal blocks selection behind it");
  expect(!session.dispatch({fm::CommandKind::ToggleMark}).accepted,
         "review modal blocks board edits");
  expect(!session.dispatch({fm::CommandKind::ClearReady}).accepted &&
             session.currentRun().rows[0U].state == fm::RowState::Ready,
         "review modal blocks banking behind it");
  static_cast<void>(session.dispatch({fm::CommandKind::CloseReview}));
  expect(session.currentRun().rows[1U].state == fm::RowState::NeedsReview,
         "closing review leaves the erroneous row on the board");
  expect(!session.dispatch({fm::CommandKind::ReleaseReviewed}).accepted,
         "closed review cannot release by a stale action");
  static_cast<void>(session.dispatch({fm::CommandKind::OpenReview}));
  const fm::DispatchResult release =
      session.dispatch({fm::CommandKind::ReleaseReviewed});
  expect(release.accepted &&
             session.currentRun().rows[1U].state == fm::RowState::Released &&
             session.currentRun().score == 0U,
         "explained row releases for zero points");
  expect(!session.dispatch({fm::CommandKind::ReleaseReviewed}).accepted,
         "repeated release cannot change evidence or score");
  const fm::HuntRunSummary summary = fm::summarizeRun(session.currentRun());
  expect(summary.incorrectRows == 1U && summary.explainedRows == 1U &&
             summary.assisted,
         "review observations remain distinct in the run summary");
}

void testCompletionRestartAndFocusRepair() {
  constexpr std::array<std::uint8_t, fm::kHuntRowCount> masks{
      5U, 14U, 4U, 9U, 11U, 10U};
  fm::HuntSession session;
  expect(session.activeRowIndex() == 0U && session.activeColumn() == 0U,
         "initial focus is the first cell of the first row");
  static_cast<void>(commitMask(session, 0U, masks[0U]));
  static_cast<void>(session.dispatch({fm::CommandKind::ClearReady}));
  expect(session.activeRowIndex().has_value() &&
             *session.activeRowIndex() == 1U,
         "clearing the active row repairs focus to a remaining row");
  expect(!session.dispatch({fm::CommandKind::Restart}).accepted,
         "restart is unavailable before finish");
  for (std::size_t row = 1U; row < fm::kHuntRowCount; ++row) {
    static_cast<void>(commitMask(session, row, masks[row]));
  }
  static_cast<void>(session.dispatch({fm::CommandKind::ClearReady}));
  expect(session.finished() && !session.activeRowIndex().has_value(),
         "all terminal rows finish the run and leave no board focus");
  const fm::DispatchResult restart =
      session.dispatch({fm::CommandKind::Restart});
  expect(restart.accepted && session.archivedRuns().size() == 1U,
         "restart archives the finished run exactly once");
  expect(session.archivedRuns()[0U].runNumber == 1U &&
             !session.archivedRuns()[0U].priorExposure,
         "archived first run remains unexposed-at-start evidence");
  expect(session.currentRun().runNumber == 2U &&
             session.currentRun().priorExposure &&
             session.currentRun().score == 0U &&
             fm::summarizeRun(session.currentRun()).untouchedRows ==
                 fm::kHuntRowCount,
         "new run resets play state and marks prior exposure");
}

void testInvalidCoordinatesDoNotCorruptState() {
  fm::HuntSession session;
  const fm::HuntRunRecord before = session.currentRun();
  expect(!select(session, fm::kHuntRowCount, 0U).accepted,
         "row coordinate outside the pack is rejected");
  expect(!select(session, 0U, fm::kHuntCellsPerRow).accepted,
         "column coordinate outside the row is rejected");
  expect(session.currentRun().rows[0U].selectedMask ==
             before.rows[0U].selectedMask &&
             session.activeRowIndex() == 0U && session.activeColumn() == 0U,
         "invalid coordinates leave state and focus unchanged");
}

}  // namespace

int main() {
  testReviewedPack();
  testWrongJudgmentsAreImmutable();
  testBankingScheduleAndCompaction();
  testReviewModalAndRelease();
  testCompletionRestartAndFocusRepair();
  testInvalidCoordinatesDoNotCorruptState();
  if (failures != 0) {
    std::cerr << failures << " first_move_hunt_tests failure(s)\n";
    return 1;
  }
  std::cout << "first_move_hunt_tests passed\n";
  return 0;
}

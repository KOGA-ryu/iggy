#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace iggy3d::first_move {

inline constexpr std::size_t kHuntRowCount = 6U;
inline constexpr std::size_t kHuntCellsPerRow = 4U;
inline constexpr std::string_view kHuntPackId = "linearity_ab_known_x_v1";
inline constexpr std::uint32_t kHuntPackVersion = 1U;

struct HuntCellContent {
  std::string_view stableId;
  std::string_view expression;
  bool qualifies = false;
  std::string_view explanation;
};

struct HuntRowContent {
  std::array<HuntCellContent, kHuntCellsPerRow> cells{};
  std::uint8_t answerMask = 0U;
};

struct HuntPack {
  std::string_view id;
  std::uint32_t version = 0U;
  std::array<HuntRowContent, kHuntRowCount> rows{};
};

[[nodiscard]] const HuntPack& huntPack() noexcept;

enum class RowState : std::uint8_t {
  Editing,
  Ready,
  NeedsReview,
  Cleared,
  Released,
};

[[nodiscard]] std::string_view rowStateName(RowState state) noexcept;

struct HuntRowRecord {
  std::size_t rowIndex = 0U;
  RowState state = RowState::Editing;
  std::uint8_t selectedMask = 0U;
  std::optional<std::uint8_t> initialSelectionMask;
  std::optional<bool> initialCorrect;
  bool explained = false;
};

struct HuntRunRecord {
  std::uint32_t runNumber = 1U;
  bool priorExposure = false;
  std::uint32_t score = 0U;
  std::array<HuntRowRecord, kHuntRowCount> rows{};
};

struct HuntRunSummary {
  std::size_t readyRows = 0U;
  std::size_t correctRows = 0U;
  std::size_t incorrectRows = 0U;
  std::size_t explainedRows = 0U;
  std::size_t untouchedRows = kHuntRowCount;
  bool assisted = false;
  bool finished = false;
};

[[nodiscard]] HuntRunSummary summarizeRun(const HuntRunRecord& run) noexcept;

enum class CommandKind : std::uint8_t {
  SelectCell,
  MoveLeft,
  MoveRight,
  MoveUp,
  MoveDown,
  ToggleMark,
  CommitRow,
  ClearReady,
  OpenReview,
  CloseReview,
  ReleaseReviewed,
  Restart,
};

struct Command {
  CommandKind kind = CommandKind::ToggleMark;
  std::size_t row = 0U;
  std::size_t column = 0U;

  [[nodiscard]] static constexpr Command selectCell(std::size_t rowIndex,
                                                     std::size_t columnIndex) {
    return {CommandKind::SelectCell, rowIndex, columnIndex};
  }
};

struct DispatchResult {
  bool accepted = false;
  bool changed = false;
  std::string_view reason = "command_rejected";
};

class HuntSession {
public:
  HuntSession();

  [[nodiscard]] DispatchResult dispatch(const Command& command);

  [[nodiscard]] const HuntRunRecord& currentRun() const noexcept;
  [[nodiscard]] const std::vector<HuntRunRecord>& archivedRuns() const noexcept;
  [[nodiscard]] std::optional<std::size_t> activeRowIndex() const noexcept;
  [[nodiscard]] std::size_t activeColumn() const noexcept;
  [[nodiscard]] std::optional<std::size_t> reviewRowIndex() const noexcept;
  [[nodiscard]] std::vector<std::size_t> visibleRowIndices() const;
  [[nodiscard]] bool finished() const noexcept;

private:
  void beginRun(std::uint32_t runNumber, bool priorExposure);
  void repairActiveRow(std::size_t preferredRow);
  [[nodiscard]] DispatchResult moveHorizontal(int direction);
  [[nodiscard]] DispatchResult moveVertical(int direction);

  HuntRunRecord current_{};
  std::vector<HuntRunRecord> archived_;
  std::optional<std::size_t> activeRow_;
  std::size_t activeColumn_ = 0U;
  std::optional<std::size_t> reviewRow_;
};

}  // namespace iggy3d::first_move
